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
#include <xmmintrin.h>  // 如果上面那个不行，试试这个
#include <xmmintrin.h>  // 如果上面那个不行，试试这个
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>
#include "ndarray.h"
#include "wangsrng.h"
#include "ticktock.h"

// Matrix 是 YX 序的二维浮点数组：mat(x, y) = mat.data()[y * mat.shape(0) + x]
//using Matrix = ndarray<2, float,0,0,AlignedAllocator<float,4096>>;
using Matrix = ndarray<2, float>;
// 注意：默认对齐到 64 字节，如需 4096 字节，请用 ndarray<2, float, AlignedAllocator<4096, float>>

static void matrix_randomize(Matrix &out) {
    TICK(matrix_randomize);
    size_t nx = out.shape(0);
    size_t ny = out.shape(1);

    // 这个循环为什么不够高效？如何优化？ 10 分
    //原因：对于YX序的数组，X是外层循环体，这样的先后的执行时间是不连续的。
    //优化：使用YX序遍历，即x作为内存循环体，这样其在时间上是连续的。
//#pragma omp parallel for collapse(2)
//    for (int x = 0; x < nx; x++) {
//        for (int y = 0; y < ny; y++) {
//            float val = wangsrng(x, y).next_float();
//            out(x, y) = val;
//        }
//    }

//#pragma omp parallel for collapse(2)
//    for (int y = 0; y < ny; y++) {
//        for (int x = 0; x < nx; x++) {
//            float val = wangsrng(x, y).next_float();
//            out(x, y) = val;
//        }
//    }

    tbb::parallel_for(tbb::blocked_range2d<size_t>(0,nx,0,ny),
            [&](tbb::blocked_range2d<size_t> r){
                for(int y = r.cols().begin(); y < r.cols().end(); y++){
                    for(int x = r.rows().begin(); x < r.rows().end(); x++){
                        float val = wangsrng(x, y).next_float();
                            out(x, y) = val;
                    }
                }
    });

    TOCK(matrix_randomize);
}

static void matrix_transpose(Matrix &out, Matrix const &in) {
    TICK(matrix_transpose);
    size_t nx = in.shape(0);
    size_t ny = in.shape(1);
    out.reshape(ny, nx);

    // 这个循环为什么不够高效？如何优化？ 15 分
    //原因：在内存看来，访存是跳跃的，违背了空间局域性
    //优化：1.循环分块，使用YXyx序，只要保证BlockSize^2小于下缓存容量即可
    //      2.tbb::simple_partitioner自带莫顿序遍历功能
//#pragma omp parallel for collapse(2)
//    for (int x = 0; x < nx; x++) {
//        for (int y = 0; y < ny; y++) {
//            out(y, x) = in(x, y);
//        }
//    }

//#pragma omp parallel for collapse(2)
//    for (int y = 0; y < ny; y++) {
//        for (int x = 0; x < nx; x++) {
//            out(x, y) = in(y, x);
//        }
//    }


    constexpr int blockSize = 64;
    tbb::parallel_for(tbb::blocked_range2d<size_t>(0,nx,blockSize,0,ny,blockSize),
            [&](tbb::blocked_range2d<size_t> const & r){
        for(int y = r.cols().begin(); y < r.cols().end(); y++){
            for(int x = r.rows().begin(); x < r.rows().end(); x++ ){
                out(x,y) = in(y,x);
            }
        }
    },tbb::simple_partitioner{});

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
    // 原因：存在不连续的lhs(t,y) 和 一直不动的 outs(x,y);
    // 优化：寄存器分块
    // 没有必要手动初始化，out的内存已经被分配上了，不会处触发缺页中断；
//#pragma omp parallel for collapse(2)
//    for (int y = 0; y < ny; y++) {
//        for (int x = 0; x < nx; x++) {
//            out(x, y) = 0;  // 有没有必要手动初始化？ 5 分
//            for (int t = 0; t < nt; t++) {
//                out(x, y) += lhs(x, t) * rhs(t, y);
//            }
//        }
//    }

#pragma omp parallel for collapse(2)
    for (int y = 0; y < ny; y++) {
        for (int iBase = 0; iBase < nx; iBase += 32) {
            for (int t = 0; t < nt; t++) {
                for (int x = iBase; x < iBase+32; x++) {
                    out(x, y) += lhs(x, t) * rhs(t, y);
                }
            }

        }
    }

    TOCK(matrix_multiply);
}

// 求出 R^T A R
static void matrix_RtAR(Matrix &RtAR, Matrix const &R, Matrix const &A) {
    TICK(matrix_RtAR);
    // 这两个是临时变量，有什么可以优化的？ 5 分
    //优化： 手动池化，声明其为static变量，thread_local表示如果有个多个线程，每个线程保留对象的副本，防止出错；
    Matrix Rt, RtA;
//    static thread_local Matrix Rt, RtA;
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
