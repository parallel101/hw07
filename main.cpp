// 这是第07课的回家作业，主题是访存优化
// 录播见：https://www.bilibili.com/video/BV1gu41117bW
// 作业的回答推荐写在 ANSWER.md 方便老师批阅，也可在 PR 描述中
// 请晒出程序被你优化前后的运行结果（ticktock 的用时统计）
// 可以比较采用了不同的优化手段后，加速了多少倍，做成表格
// 如能同时贴出 CPU 核心数量，缓存大小等就最好了（lscpu 命令）
// 作业中有很多个问句，请通过注释回答问题，并改进其代码，以使其更快
// 并行可以用 OpenMP 也可以用 TBB

#include <iostream>
// #include <x86intrin.h>  // _mm 系列指令都来自这个头文件
#include <immintrin.h>  // 如果上面那个不行，试试这个
#include "ndarray.h"
#include "wangsrng.h"
#include "ticktock.h"
#include "morton.h"
#include "mtprint.h"

// Matrix 是 YX 序的二维浮点数组：mat(x, y) = mat.data()[y * mat.shape(0) + x]
using Matrix = ndarray<2, float>;
// 注意：默认对齐到 64 字节，如需 4096 字节，请用 ndarray<2, float, AlignedAllocator<4096, float>>

static void matrix_randomize(Matrix &out) {
    TICK(matrix_randomize);
    size_t nx = out.shape(0);
    size_t ny = out.shape(1);

/*
    这个循环为什么不够高效？如何优化？ 10 分
    跳跃访存，缓存利用率低下
    1.改变xy序为yx序
    2.使用直写指令，降低一倍访存带宽
*/
#pragma omp parallel for collapse(2)
    for (size_t y = 0; y < ny; y++) {
        for (size_t xBase = 0; xBase < nx; xBase+=16) {
            float res[16];
// #pragma GCC unroll 16
            for (size_t x=0;x<16;x++){
                res[x] = wangsrng(xBase+x, y).next_float();
                // out(x, y) = val;
            }
            // for (int x=0;x<16;x++){
            //     _mm_stream_si32(reinterpret_cast<int *>(&out(x+xBase,y)),*(reinterpret_cast<int *>(&res[x])));
            // }
            _mm512_stream_ps(&out(xBase,y),_mm512_load_ps(res));

        }
    }
    TOCK(matrix_randomize);
}

static void matrix_transpose(Matrix &out, Matrix const &in) {
    TICK(matrix_transpose);
    size_t nx = in.shape(0);
    size_t ny = in.shape(1);
    out.reshape(ny, nx);

/*
    这个循环为什么不够高效？如何优化？ 15 分
    矩阵转置这种访存读写一方一定存在一个是跳跃访存的，导致缓存利用率低
*/
//原版代码
// #pragma omp parallel for collapse(2)
//     for (int x = 0; x < nx; x++) {
//         for (int y = 0; y < ny; y++) {
//             out(y, x) = in(x, y);
//         }
//     }

//莫顿码顺序访问，但是由于分块的大小不是2的幂，需要判断越界
    int blockSize=32;
    int xdBlock=nx/blockSize;
    int ydBlock=ny/blockSize;
#pragma omp parallel for
    for (int k = 0; k < morton2d::highestOneBit(xdBlock)*morton2d::highestOneBit(ydBlock); k++){
        auto [xBase, yBase] = morton2d::decode(k);
        // mtprint(xdBlock,morton2d::highestOneBit(xdBlock));
        if (xBase>=xdBlock || yBase>=ydBlock){
            continue;
        }
        xBase*=blockSize;
        yBase*=blockSize;
        for (int x=xBase;x<xBase+blockSize;x++){
            for(int y=yBase;y<yBase+blockSize;y++){
                // 用stream指令反而变慢了，不知道为何
                // _mm_stream_si32((int *)(&out(y,x)),(int &)in(x,y));
                out(y, x) = in(x, y);
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
/*
    这个循环为什么不够高效？如何优化？ 15 分
    程序是mem-bound，三个矩阵的访存中
        out(x, y)顺序访问，高效；
        lhs(x, t)存储是x主序，但是按t主序访问，跨步访问，低效；
        rhs(t, y)访问同一个地址，一般
    缓存(L1)大小不够存下三个矩阵，命中率低
    1. 循环分块，这样缓存可以放下一个块的大小，提高缓存命中率
    2. 寄存器分块，指令级并行
    3. 循环展开
*/
    int blockSize=32;
    // __m128 mout,mlhs,mrhs;
#pragma omp parallel for collapse(3)
    for(int yBase = 0; yBase < ny / blockSize;yBase++){
        for (int xBase = 0; xBase < nx / blockSize; xBase++) {
            for ( int tBase=0 ; tBase< nt / blockSize; tBase ++){
                for (int y = yBase*blockSize; y < (yBase+1)*blockSize; y++) {
                    for (int t = tBase*blockSize; t < (tBase+1)*blockSize; t++) {
                        // out(x, y) = 0;  // 有没有必要手动初始化？ 5 分 答：没必要，clear()->resize()全部初始化为0了
                        // mrhs=_mm_set1_ps(rhs(t,y));
#pragma GCC unroll 32
                        for (int x = xBase*blockSize; x < (xBase+1)*blockSize; x++) {
                            out(x, y) += lhs(x, t) * rhs(t, y);
                            // mout=_mm_load_ps(&out(x,y));
                            // mlhs=_mm_load_ps(&lhs(x,t));
                            // mout=_mm_add_ps(mout,_mm_mul_ps(mlhs,mrhs));
                            // _mm_store_ps(&out(x,y),mout);
                        }
                    }
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
    // 1. 可以省去Rt,使用RtAR代替
    // 2. 使用static关键字，手动池化
    static thread_local Matrix RtA;
    matrix_transpose(RtAR, R);
    matrix_multiply(RtA, RtAR, A);
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
