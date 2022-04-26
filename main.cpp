// 这是第07课的回家作业，主题是访存优化
// 录播见：https://www.bilibili.com/video/BV1gu41117bW
// 作业的回答推荐写在 ANSWER.md 方便老师批阅，也可在 PR 描述中
// 请晒出程序被你优化前后的运行结果（ticktock 的用时统计）
// 可以比较采用了不同的优化手段后，加速了多少倍，做成表格
// 如能同时贴出 CPU 核心数量，缓存大小等就最好了（lscpu 命令）
// 作业中有很多个问句，请通过注释回答问题，并改进其代码，以使其更快
// 并行可以用 OpenMP 也可以用 TBB

#include <cstring>
#include <iostream>
#include <x86intrin.h>  // _mm 系列指令都来自这个头文件
//#include <xmmintrin.h>  // 如果上面那个不行，试试这个
#include "alignalloc.h"
#include "ndarray.h"
#include "wangsrng.h"
#include "ticktock.h"

// Matrix 是 YX 序的二维浮点数组：mat(x, y) = mat.data()[y * mat.shape(0) + x]
using Matrix = ndarray<2, float, 64>;
// 注意：默认对齐到 64 字节，如需 4096 字节，请用 ndarray<2, float, AlignedAllocator<4096, float>>

static void matrix_randomize(Matrix &out) {
    TICK(matrix_randomize);
    size_t nx = out.shape(0);
    size_t ny = out.shape(1);

    // 这个循环为什么不够高效？如何优化？ 10 分
// 不是顺序写入所以低效
// - 在原函数中，yx序的矩阵使用xy序遍历。交换xy遍历顺序即可优化为顺序访问。(O3优化似乎自动帮你做了)
// - 可以使用`_mm_stream_si32`绕过缓存写入。
// - 也可以使用`_mm_stream_ps`绕过缓存写入，但这要求首先计算4次`random`，有可能变成CPU-bound. 或者可以设计一个每次输出一个128比特向量的random.
//   - 如果数组大小不是4的倍数，边界需要特殊处理，或者申请数组时向外扩张4个float以免越界。
#pragma omp parallel for collapse(2)
    for (int y = 0; y < ny; y++) {
        for (int x = 0; x < nx; x+=4) {
            float val[4];
            for (int i = 0; i != 4; i++)
                val[i] = wangsrng(x, y).next_float();
            _mm_stream_ps(&out(x,y), _mm_load_ps(val));
        // for (int x = 0; x < nx; x++) {
        //     float val = wangsrng(x, y).next_float();
        //     _mm_stream_si32((int*)&out(x, y), *(int*)&val);
        //     out(x, y) = val;
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
// 主要的问题out和in总有一个不是顺序访问所以造成大量cache miss. 可以通过分块遍历优化。
// - 可以使用简单的分块循环，每次转置一个直径为64的块
//   - 需要注意越界问题，需要边缘扩展64字节
// - (未使用)可以使用莫顿码遍历，但由于注意到矩阵直径不是2的整数幂所以可以使用tbb的`simple_partitioner`自带的莫顿码遍历
// - 使用`_mm_stream_si32`绕过缓存写入
#pragma omp parallel for collapse(2)
    for (int yBase = 0; yBase < ny; yBase+=64) {
        for (int xBase = 0; xBase < nx; xBase+=64) {
            for (int y = yBase; y != yBase + 64; y++) {
                for (int x = xBase; x != xBase + 64; x++) {
                    _mm_stream_si32((int*)&out(x, y), *(int*)&in(y, x));
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
    // 由于遍历顺序不佳所以不是顺序访问，可以通过改变t和x的遍历顺序解决
#pragma omp parallel for 
    for (int y = 0; y < ny; y++) {
        // 需要手动初始化，因为无法保证Matrix全为0，因为它是作为参数传入而不是作为临时变量构造的。
        memset(&out(0, y), 0, sizeof(float) * (nx));
        for (int t = 0; t < nt; t++) {
            for (int x = 0; x < nx; x++) {
                out(x, y) += lhs(x, t) * rhs(t, y);
            }
        }
    }
    TOCK(matrix_multiply);
}

// 求出 R^T A R
static void matrix_RtAR(Matrix &RtAR, Matrix const &R, Matrix const &A) {
    TICK(matrix_RtAR);
    // 这两个是临时变量，有什么可以优化的？ 5 分
    // 简单池化避免重复分配销毁。
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
