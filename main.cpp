// 这是第07课的回家作业，主题是访存优化
// 录播见：https://www.bilibili.com/video/BV1gu41117bW
// 作业的回答推荐写在 ANSWER.md 方便老师批阅，也可在 PR 描述中
// 请晒出程序被你优化前后的运行结果（ticktock 的用时统计）
// 可以比较采用了不同的优化手段后，加速了多少倍，做成表格
// 如能同时贴出 CPU 核心数量，缓存大小等就最好了（lscpu 命令）
// 作业中有很多个问句，请通过注释回答问题，并改进其代码，以使其更快
// 并行可以用 OpenMP 也可以用 TBB

#include <cstddef>
#include <iostream>
#include <x86intrin.h>  // _mm 系列指令都来自这个头文件
#include <tbb/blocked_range.h>
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tbb/partitioner.h>
// #include <xmmintrin.h>  // 如果上面那个不行，试试这个
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>
#include "ndarray.h"
#include "wangsrng.h"
#include "ticktock.h"

// Matrix 是 YX 序的二维浮点数组：mat(x, y) = mat.data()[y * mat.shape(0) + x]
// using Matrix = ndarray<2, float>;
using Matrix = ndarray<2, float, 0, 0, AlignedAllocator<float, 4096> >;
// 注意：默认对齐到 64 字节，如需 4096 字节，请用 ndarray<2, float, AlignedAllocator<4096, float>>
// 4096 那么大，即64个缓存行。
// 这样一次随机访问之后会伴随着64次顺序访问，能被CPU检测到，从而启动缓存行预取，避免了等待数据抵达前空转浪费时间



static void matrix_randomize(Matrix &out) {
    TICK(matrix_randomize);
    size_t nx = out.shape(0);
    size_t ny = out.shape(1);

    // 这个循环为什么不够高效？如何优化？ 10 分
    // answer: 这是因为 YX 序的数组，X 方向在内存空间中的排布是连续的
    // YX 序的循环，其 X 是内层循环体，因此在先后执行的时间上是连续的。
    // 对于 YX 序（列主序，C/C++）的数组，请用 YX 序遍历（x变量做内层循环体）

    // #pragma omp parallel for collapse(2)
    //     for (int x = 0; x < nx; x++) {
    //         for (int y = 0; y < ny; y++) {
    //             float val = wangsrng(x, y).next_float();
    //             out(x, y) = val;
    //         }
    //     }


// _mm_stream_ps 可以一次性写入 16 字节到挂起队列，更加高效了
// 他的第二参数是一个 __m128 类型，可以配合其他手写的 SIMD 指令使用
// 不过，_mm_stream_ps 写入的地址必须对齐到 16 字节，否则会产生段错误等异常
// 需要注意，stream 系列指令写入的地址，必须是连续的，中间不能有跨步，否则无法合并写入，会产生有中间数据读的带宽
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < ny; y++) {
        for (int x = 0; x < nx; x+=4) {
    
            __m128 tmp = {wangsrng(x, y).next_float(), wangsrng(x+1, y).next_float(), wangsrng(x+2, y).next_float(), wangsrng(x+3, y).next_float()};
            _mm_stream_ps(&out(x,y), tmp);

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
    // answer: 循环是 XY 序的，虽然 out 也是 XY 序的没问题，但是 in 相当于一个 YX 序的二维数组，
    // 从而在内存看来访存是跳跃的，违背了空间局域性。因为每次跳跃了 ny，所以只要缓存容量小于 ny 就无法命中。

    // #pragma omp parallel for collapse(2)
    //     for (int x = 0; x < nx; x++) {
    //         for (int y = 0; y < ny; y++) {
    //             out(y, x) = in(x, y);
    //         }
    //     }

    // 解决方法当然还是循环分块。
    // 这样只需要块的大小 blockSize^2 小于缓存容量，即可保证全部命中。
    constexpr int block_size = 64;
    tbb::parallel_for(tbb::blocked_range2d<size_t>(0,ny, block_size, 0, nx, block_size),
        [&](const tbb::blocked_range2d<size_t> &r){
            for(size_t y=r.cols().begin(); y<r.cols().end(); y++){
                for(size_t x=r.rows().begin(); x<r.rows().end(); x++){
                    out(x,y) = in(y,x);
                }
            }
        },
        tbb::simple_partitioner{}
        );
//tbb::simple_partitioner 自带莫顿序遍历功能 
// 保证对齐到16字节

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
    // out(x, y) 始终在一个地址不动（一般）。
    // lhs(x, t) 每次跳跃 n 间隔的访问（坏）。
    // rhs(t, y) 连续的顺序访问（好）。
    // 因为存在不连续的 lhs 和一直不动的 out，导致矢量化失败，一次只能处理一个标量，CPU也无法启动指令级并行（ILP）

    // #pragma omp parallel for collapse(2)
    //     for (int y = 0; y < ny; y++) {
    //         for (int x = 0; x < nx; x++) {
    //             out(x, y) = 0;  // 有没有必要手动初始化？ 5 分
    //             for (int t = 0; t < nt; t++) {
    //                 out(x, y) += lhs(x, t) * rhs(t, y);
    //             }
    //         }
    //     }

    #pragma omp parallel for collapse(2)
    for(int j=0; j<ny; j++){
        for(int i=0; i<nx; i+=32){
            for(int t=0; t<nt; t++){
#pragma omp unroll
                for(int i_block=i; i_block<i+32; i_block++){
                    out(i,j) += lhs(i_block, t) *  rhs(t, j);
                }
            }
        }
    }

    TOCK(matrix_multiply);
}


//out(i, j) 连续 32 次顺序访问（好）。
//lhs(i, t) 连续 32 次顺序访问（好）。
//rhs(t, j) 32 次在一个地址不动（一般）。
//这样就消除不连续的访问了，从而内部的 i 循环可以顺利矢量化，且多个循环体之间没有依赖关系，CPU得以启动指令级并行，缓存预取也能正常工作，快好多！


// 求出 R^T A R
static void matrix_RtAR(Matrix &RtAR, Matrix const &R, Matrix const &A) {
    TICK(matrix_RtAR);
    // 这两个是临时变量，有什么可以优化的？ 5 分
    // answer: 预先分配好空间。
    static Matrix Rt(std::array<std::size_t, 2>{1024, 1024}), RtA(std::array<std::size_t, 2>{1024, 1024});
    
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
        // size_t n = 1<<13;
        std::cout << "t=" << t << ": n=" << n << std::endl;
        test_func(n);
    }
    TOCK(overall);
    return 0;
}
