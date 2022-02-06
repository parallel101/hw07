// 这是第07课的回家作业，主题是访存优化
// 录播见：https://www.bilibili.com/video/BV1gu41117bW
// 作业的回答推荐写在 ANSWER.md 方便老师批阅，也可在 PR 描述中
// 请晒出程序被你优化前后的运行结果（ticktock 的用时统计）
// 可以比较采用了不同的优化手段后，加速了多少倍，做成表格
// 如能同时贴出 CPU 核心数量，缓存大小等就最好了（lscpu 命令）
// 作业中有很多个问句，请通过注释回答问题，并改进其代码，以使其更快
// 并行可以用 OpenMP 也可以用 TBB

#include <iostream>
//#include <x86intrin.h>  // _mm 系列指令都来自这个头文件
//#include <xmmintrin.h>  // 如果上面那个不行，试试这个
#include "ndarray.h"
#include "wangsrng.h"
#include "ticktock.h"
#include "alignalloc.h"

// nru 是用来对数据分块处理的粒度
constexpr int nru = 16;
// Matrix 是 YX 序的二维浮点数组：mat(x, y) = mat.data()[y * mat.shape(0) + x]
using Matrix = ndarray<2, float, 0, nru, AlignedAllocator<float, nru*nru*sizeof(float)>>;

// 注意：默认对齐到 64 字节，如需 4096 字节，请用 ndarray<2, float, AlignedAllocator<4096, float>>

static void matrix_randomize(Matrix &out) {
    TICK(matrix_randomize);
    size_t nx = out.shape(0);
    size_t ny = out.shape(1);

    // 这个循环为什么不够高效？如何优化？ 10 分
    // 答：
    // 1. YX 序的数组应该优先X维度的连续访问(但此场景编译器应该能处理好，OpenMP也可以帮助处理, collapse)。
    // 2. 各个内存区域赋值实际是彼此独立没有数据依赖的，可以并行处理(openmp parallel for)
    // 3. 由于求随机数函数的存在，编译器无法解除数据以来关系，手动展开可以帮助编译器理解。(unroll)
    // 4. 写入粒度过小，unroll后写入粒度达到64字节，满足一个Cache行的写入
#pragma omp parallel for collapse(2)
    for (int y = 0; y < ny; y+=nru) {
        for (int x = 0; x < nx; x+=nru) {
#pragma omp SIMD
            for (int yof = 0; yof < nru; yof++) {
#pragma omp unroll
                for (int xof = 0; xof < nru; xof++){
                    float val = wangsrng(x+xof, y+yof).next_float();
                    out(x+xof, y+yof) = val;
                }
            }
        }
    }
    TOCK(matrix_randomize);
}

static void matrix_transpose(Matrix &out, Matrix const &in) {
    TICK(matrix_transpose);
    size_t nx = in.shape(0);
    size_t ny = in.shape(1);
    out.reshape(ny, nx);

    // 这个循环为什么不够高效？如何优化？ 15 分
    // 答：跨步访问导致不高效，可以使用分块遍历，充分利用Cache
#pragma omp parallel for collapse(2)
    for (int x = 0; x < nx; x+=nru) {
        for (int y = 0; y < ny; y+=nru) {
#pragma omp SIMD
            for (int xof = 0; xof < nru; xof++) {
#pragma omp unroll
                for (int yof = 0; yof < nru; yof++) {
                    out(y+yof, x+xof) = in(x+xof, y+yof);
                }
            }
        }
    }
    TOCK(matrix_transpose);
}

static void matrix_multiply(Matrix &out, Matrix const &lhs, Matrix const &rhs) {
    TICK(matrix_multiply);
    size_t nx = lhs.shape(0);
    size_t nt = lhs.shape(1);
    size_t ny = rhs.shape(1);
    if (rhs.shape(0) != nt) {
        std::cerr << "matrix_multiply: shape mismatch" << std::endl;
        throw;
    }
    out.reshape(nx, ny);

    // 这个循环为什么不够高效？如何优化？ 15 分
    // 答： 
#pragma omp parallel for collapse(2)
    for (int y = 0; y < ny; y+=nru) {
        for (int x = 0; x < nx; x+=nru) {
            // out(x, y) = 0;  // 有没有必要手动初始化？ 5 分  答：不需要，可以使用临时数组累加再赋值规避
            float sum[nru][nru] = {0};
#pragma omp SIMD
            for (int t = 0; t < nt; t += nru) {
#pragma omp unroll partial(nru*nru*nru/4)
                for (int yf = 0; yf < nru; yf++)
                    for (int xf = 0; xf < nru; xf++) 
                        for (int tf = 0; tf < nru; tf++)
                            // out(x, y) += lhs(x, t) * rhs(t, y);
                            sum[yf][xf] += lhs(x + xf, t + tf) * rhs(t + tf, y + yf);
            }
#pragma omp unroll
            for (int yf = 0; yf < nru; yf++)
                for (int xf = 0; xf < nru; xf++)
                    out(x + xf, y + yf) = sum[yf][xf];
        }
    }
    TOCK(matrix_multiply);
}

// 求出 R^T A R
static void matrix_RtAR(Matrix &RtAR, Matrix const &R, Matrix const &A) {
    TICK(matrix_RtAR);
    // 这两个是临时变量，有什么可以优化的？ 5 分
    // 答：可以声明为static的，避免重复创建
    static Matrix Rt, RtA;
    matrix_transpose(Rt, R);
    matrix_multiply(RtA, Rt, A);
    matrix_multiply(RtAR, RtA, R);
    TOCK(matrix_RtAR);
}

static float matrix_trace(Matrix const &in) {
    TICK(matrix_trace);
    float res = 0;
    size_t nt = std::min(in.shape(0), in.shape(1));
#pragma omp parallel for reduction(+:res)
    for (int t = 0; t < nt; t++) {
        res += in(t, t);
    }
    TOCK(matrix_trace);
    return res;
}

static void test_func(size_t n) {
    TICK(test_func);
    Matrix R(n, n);
    matrix_randomize(R);
    Matrix A(n, n);
    matrix_randomize(A);

    Matrix RtAR;
    matrix_RtAR(RtAR, R, A);

    std::cout << matrix_trace(RtAR) << std::endl;
    TOCK(test_func);
}

int main() {
    wangsrng rng;
    TICK(overall);
    for (int t = 0; t < 4; t++) {
        size_t n = 32 * (rng.next_uint64() % 16 + 24);
        std::cout << "t=" << t << ": n=" << n << std::endl;
        test_func(n);
    }
    TOCK(overall);
    return 0;
}
