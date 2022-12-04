// 这是第07课的回家作业，主题是访存优化
// 录播见：https://www.bilibili.com/video/BV1gu41117bW
// 作业的回答推荐写在 ANSWER.md 方便老师批阅，也可在 PR 描述中
// 请晒出程序被你优化前后的运行结果（ticktock 的用时统计）
// 可以比较采用了不同的优化手段后，加速了多少倍，做成表格
// 如能同时贴出 CPU 核心数量，缓存大小等就最好了（lscpu 命令）
// 作业中有很多个问句，请通过注释回答问题，并改进其代码，以使其更快
// 并行可以用 OpenMP 也可以用 TBB

#include <iostream>
#include <cstring>
#include <x86intrin.h>  // _mm 系列指令都来自这个头文件
//#include <xmmintrin.h>  // 如果上面那个不行，试试这个
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>
#include <tbb/partitioner.h>
#include "ndarray.h"
#include "wangsrng.h"
#include "ticktock.h"

// Matrix 是 YX 序的二维浮点数组：mat(x, y) = mat.data()[y * mat.shape(0) + x]
using Matrix = ndarray<2, float>;
// 注意：默认对齐到 64 字节，如需 4096 字节，请用 ndarray<2, float, AlignedAllocator<4096, float>>

static void matrix_randomize(Matrix &out) {
    TICK(matrix_randomize);
    size_t nx = out.shape(0);
    size_t ny = out.shape(1);

    // 这个循环为什么不够高效？如何优化？ 10 分
    // 答：不够高效是因为内部循环遍历 y 导致访存不是连续的，而是跳跃的，降低访存效率。
    // 可以通过改变循环的轴，并采用直写进行优化。
#pragma omp parallel for collapse(2)
    for (int y = 0; y < ny; ++y) {
        for (int x = 0; x < nx; x += 8) { 
            __m256 res = {
                wangsrng(x, y).next_float(),
                wangsrng(x+1, y).next_float(),
                wangsrng(x+2, y).next_float(),
                wangsrng(x+3, y).next_float(),
                wangsrng(x+4, y).next_float(),
                wangsrng(x+5, y).next_float(),
                wangsrng(x+6, y).next_float(),
                wangsrng(x+7, y).next_float()
            };
            _mm256_stream_ps(&out(x, y), res);
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
    // 答：内部循环访存 out(y, x) 是连续的，但访问 in(x, y) 不是。
    // 可以用 loop tiling + morton ordering 的方式优化

    const int BLOCK_SIZE = 32; // 32 * 32 * 4B = 4KB = 1 page and n mod 32 = 0
    tbb::parallel_for(tbb::blocked_range2d<size_t>(0, nx, BLOCK_SIZE, 0, ny, BLOCK_SIZE),
        [&in, &out](const tbb::blocked_range2d<size_t> &r) {
            for (int x = r.rows().begin(); x != r.rows().end(); ++x) {
                for (int y = r.cols().begin(); y != r.cols().end(); ++y) {
                    out(y, x) = in(x, y);
                }
            }
        }, 
        tbb::simple_partitioner{}
    );
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
    // 答：最内部循环访存 rhs(t, y) 是连续的，但访问 out(x, y) 与 lhs(x, t) 不是。
    // 可以用 loop tiling + morton ordering 的方式优化
    const int BLOCK_SIZE = 32; // 32 * 32 * 4B = 4KB = 1 page and n mod 32 = 0
    tbb::parallel_for(tbb::blocked_range2d<size_t>(0, nx, BLOCK_SIZE, 0, ny, BLOCK_SIZE),
        [&out, &lhs, &rhs, nt](const tbb::blocked_range2d<size_t> &r) {
            for (int x = r.rows().begin(); x != r.rows().end(); ++x) {
                for (int y = r.cols().begin(); y != r.cols().end(); ++y) {
                    float tmp = 0.0f;
                    for (int t = 0; t < nt; ++t) {
                        tmp += lhs(x, t) * rhs(t, y);
                    }
                    out(x, y) = tmp; // 有没有必要手动初始化？ 5 分
                    // 没必要初始化，也可以用临时变量记录和。
                }
            }
        }, 
        tbb::simple_partitioner{}
    );
    TOCK(matrix_multiply);
}

// 求出 R^T A R
static void matrix_RtAR(Matrix &RtAR, Matrix const &R, Matrix const &A) {
    TICK(matrix_RtAR);
    // 这两个是临时变量，有什么可以优化的？ 5 分
    // 可以将其局部静态化，以防重复初始化；tread_local 保证多线程安全。
    static thread_local Matrix Rt, RtA;
    
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
