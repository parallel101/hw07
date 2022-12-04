# 环境

entry | info
--- | ---
OS | WSL2 of Ubuntu20.04
Architecture |                    x86_64
CPU(s) |                          8
Thread(s) per core |              2
L1d cache |                       128 KiB
L1i cache |                       128 KiB
L2 cache |                        1 MiB
L3 cache |                        6 MiB

# 改进前

```
t=0: n=1120
matrix_randomize: 0.0036149s
matrix_randomize: 0.0073586s
matrix_transpose: 0.0058029s
matrix_multiply: 1.57465s
matrix_multiply: 2.1565s
matrix_RtAR: 3.73763s
matrix_trace: 1.49e-05s
1.75932e+08
test_func: 3.75655s
t=1: n=928
matrix_randomize: 0.0038179s
matrix_randomize: 0.0058834s
matrix_transpose: 0.0037703s
matrix_multiply: 0.463579s
matrix_multiply: 0.385973s
matrix_RtAR: 0.854033s
matrix_trace: 2.59e-05s
1.00156e+08
test_func: 0.86811s
t=2: n=1024
matrix_randomize: 0.0049764s
matrix_randomize: 0.0043571s
matrix_transpose: 0.0112257s
matrix_multiply: 1.86208s
matrix_multiply: 1.89948s
matrix_RtAR: 3.77345s
matrix_trace: 1.25e-05s
1.34324e+08
test_func: 3.78877s
t=3: n=1056
matrix_randomize: 0.0032734s
matrix_randomize: 0.0041083s
matrix_transpose: 0.0088871s
matrix_multiply: 1.37759s
matrix_multiply: 1.42704s
matrix_RtAR: 2.81431s
matrix_trace: 1.3e-05s
1.47405e+08
test_func: 2.82567s
overall: 11.2438s
```

# 改进后

```
t=0: n=1120
matrix_randomize: 0.0044706s
matrix_randomize: 0.0009012s
matrix_transpose: 0.004934s
matrix_multiply: 1.09293s
matrix_multiply: 1.22578s
matrix_RtAR: 2.32385s
matrix_trace: 0.0004732s
1.75932e+08
test_func: 2.34133s
t=1: n=928
matrix_randomize: 0.0010455s
matrix_randomize: 0.0007095s
matrix_transpose: 0.0026342s
matrix_multiply: 0.274808s
matrix_multiply: 0.29029s
matrix_RtAR: 0.56793s
matrix_trace: 0.0004422s
1.00156e+08
test_func: 0.573571s
t=2: n=1024
matrix_randomize: 0.0029974s
matrix_randomize: 0.0044855s
matrix_transpose: 0.001516s
matrix_multiply: 1.6364s
matrix_multiply: 1.66931s
matrix_RtAR: 3.30743s
matrix_trace: 0.0011572s
1.34324e+08
test_func: 3.32015s
t=3: n=1056
matrix_randomize: 0.0014983s
matrix_randomize:0.0014983s
matrix_transpose: 0.0028232s
matrix_multiply: 0.700534s
matrix_multiply: 0.729549s
matrix_RtAR: 1.43312s
matrix_trace: 0.0017723s
1.47405e+08
test_func: 1.44506s
overall: 7.68174s
```

# 加速比

function | time(pre) | time(optimized) |speedup
--- | --- | --- | ---
matrix_randomize | 0.00467375s | 0.0022007875s | 2.124x
matrix_transpose | 0.00742150s | 0.00297685s | 2.493x
matrix_multiply | 1.3933615s | 0.9524501s | 1.463x
matrix_RtAR | 2.79485575s | 1.90808250s | 1.465x

# 优化方法

> matrix_randomize

改成 YX 序遍历；改用 _mm256_stream_ps 直写内存。

> matrix_transpose

用 loop tiling + morton ordering 的方式优化，分块时保证 `BLOCKSIZE ^ 2` 小于L1缓存大小，利用 TBB 的 `tbb::simple_partitioner` 进行 morton ordering 遍历。

> matrix_multiply

同 `matrix_transpose` 的优化，但用一个临时变量额外存累加和，最后向 `out(x, y)` 写结果。

> matrix_RtAR

将两个临时变量局部静态化，即手动池化，防止重复分配内存。p.s. 虽然没有多线程，但还是用课上的 `thread_local` 修饰，保证线程安全。

# 我的创新点

如果有，请说明。
