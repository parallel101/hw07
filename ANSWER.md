# 运行环境
## CPU
Name: Intel(R) Core(TM) i7-6500U CPU @ 2.50GHz
Core Count: 2
Thread Count: 4 
Flags: FPU, SSE, SSE2, ...
L1 Cache: 64KB, Write back, 8-way Set-associative
L2 Cache 512KB, Write back, 4-way Set-associative
L3 Cache 4096KB, Write back, 16-way Set-associative

## Memory
Size: 4096MB x 2
Data Width: 64 bits
Type: LPDDR3
Speed: 1867MT/s

理论极限带宽=频率 * 宽度 * 数量 = 29872 MB/s
$ 1867 * 8 * 2 = 29872 MB/s $ 
# 改进前

```
t=0: n=1120
matrix_randomize: 0.004051s
matrix_randomize: 0.003408s
matrix_transpose: 0.007777s
matrix_multiply: 2.90488s
matrix_multiply: 2.26766s
matrix_RtAR: 5.1804s
matrix_trace: 6.5e-05s
1.75932e+08
test_func: 5.19764s
t=1: n=928
matrix_randomize: 0.004649s
matrix_randomize: 0.005066s
matrix_transpose: 0.00422s
matrix_multiply: 0.848782s
matrix_multiply: 0.835605s
matrix_RtAR: 1.6887s
matrix_trace: 0.000164s
1.00156e+08
test_func: 1.70451s
t=2: n=1024
matrix_randomize: 0.012848s
matrix_randomize: 0.012852s
matrix_transpose: 0.012015s
matrix_multiply: 6.02873s
matrix_multiply: 6.04324s
matrix_RtAR: 12.0866s
matrix_trace: 0.000372s
1.34324e+08
test_func: 12.1157s
t=3: n=1056
matrix_randomize: 0.011363s
matrix_randomize: 0.018065s
matrix_transpose: 0.005886s
matrix_multiply: 2.4236s
matrix_multiply: 2.40505s
matrix_RtAR: 4.84032s
matrix_trace: 0.000177s
1.47405e+08
test_func: 4.87778s
overall: 23.9097s
```

# 改进后

```
t=0: n=1120
matrix_randomize: 0.001592s
matrix_randomize: 0.000908s
matrix_transpose: 0.012604s
matrix_multiply: 0.294772s
matrix_multiply: 0.259419s
matrix_RtAR: 0.566971s
matrix_trace: 6.3e-05s
1.75932e+08
test_func: 0.577756s
t=1: n=928
matrix_randomize: 0.000772s
matrix_randomize: 0.000502s
matrix_transpose: 0.003806s
matrix_multiply: 0.168204s
matrix_multiply: 0.1403s
matrix_RtAR: 0.312448s
matrix_trace: 4.2e-05s
1.00156e+08
test_func: 0.32201s
t=2: n=1024
matrix_randomize: 0.000424s
matrix_randomize: 0.000384s
matrix_transpose: 0.001753s
matrix_multiply: 0.206256s
matrix_multiply: 0.213244s
matrix_RtAR: 0.421353s
matrix_trace: 0.00011s
1.34324e+08
test_func: 0.423929s
t=3: n=1056
matrix_randomize: 0.000696s
matrix_randomize: 0.000604s
matrix_transpose: 0.002052s
matrix_multiply: 0.206916s
matrix_multiply: 0.200554s
matrix_RtAR: 0.409645s
matrix_trace: 5.2e-05s
1.47405e+08
test_func: 0.41374s
overall: 1.74306s
```

# 加速比

matrix_randomize: 12.9x
matrix_transpose: 1.48x
matrix_multiply: 14x
matrix_RtAR: 13.9x

> 如果记录了多种优化方法，可以做表格比较
只是基于OpenMP做了优化，还没有使用其他的框架。

# 优化方法

下面这些函数你是如何优化的？是什么思路？用了老师上课的哪个知识点？

> matrix_randomize

使用正确的YX循环遍历顺序，确保顺序访问。
使用分块循环遍历，充分利用Cache。
适当展开定长循环，确保编译器能够充分自动SIMD。

> matrix_transpose

分块循环减少跨步访问。

> matrix_multiply

分块循环，同时可以使用本地临时变量缓存累加的结果，适当展开小循环。
对写内存做统一集中写入，避免反复读取。

> matrix_RtAR

使用static变量减少变量的反复创建。

# 我的创新点

没有:(

