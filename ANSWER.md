# 实验环境
* 16cores 32threads
```
L1d:                   512 KiB (16 instances)
L1i:                   512 KiB (16 instances)
L2:                    8 MiB (16 instances)
L3:                    64 MiB (2 instances)
```

# 改进前

```
t=0: n=1120
matrix_randomize: 0.00971097s
matrix_randomize: 0.00703878s
matrix_transpose: 0.00927667s
matrix_multiply: 0.117476s
matrix_multiply: 0.117298s
matrix_RtAR: 0.244081s
matrix_trace: 0.00835175s
1.75932e+08
test_func: 0.274363s
t=1: n=928
matrix_randomize: 0.006546s
matrix_randomize: 0.00750128s
matrix_transpose: 0.010777s
matrix_multiply: 0.0455778s
matrix_multiply: 0.0537599s
matrix_RtAR: 0.110256s
matrix_trace: 0.00894227s
1.00156e+08
test_func: 0.141392s
t=2: n=1024
matrix_randomize: 0.00713252s
matrix_randomize: 0.00754496s
matrix_transpose: 0.00931327s
matrix_multiply: 0.194533s
matrix_multiply: 0.187665s
matrix_RtAR: 0.391531s
matrix_trace: 0.00910213s
1.34324e+08
test_func: 0.419415s
t=3: n=1056
matrix_randomize: 0.0118236s
matrix_randomize: 0.00950726s
matrix_transpose: 0.00668107s
matrix_multiply: 0.0764283s
matrix_multiply: 0.0805108s
matrix_RtAR: 0.163727s
matrix_trace: 0.00896033s
1.47405e+08
test_func: 0.197871s
overall: 1.0342s
```

# 改进后

```
t=0: n=1120
matrix_randomize: 0.000273793s
matrix_randomize: 0.000278233s
matrix_transpose: 0.00315305s
matrix_multiply: 0.0296057s
matrix_multiply: 0.0279403s
matrix_RtAR: 0.060762s
matrix_trace: 0.00917836s
1.75932e+08
test_func: 0.0749736s
t=1: n=928
matrix_randomize: 0.000221557s
matrix_randomize: 0.000185628s
matrix_transpose: 0.000508399s
matrix_multiply: 0.0319896s
matrix_multiply: 0.0188709s
matrix_RtAR: 0.0516253s
matrix_trace: 0.00931379s
1.00156e+08
test_func: 0.0648803s
t=2: n=1024
matrix_randomize: 0.000262414s
matrix_randomize: 0.000265054s
matrix_transpose: 0.000564065s
matrix_multiply: 0.0198131s
matrix_multiply: 0.0238569s
matrix_RtAR: 0.0443844s
matrix_trace: 0.0134706s
1.34324e+08
test_func: 0.062279s
t=3: n=1056
matrix_randomize: 0.000309561s
matrix_randomize: 0.000290673s
matrix_transpose: 0.000471451s
matrix_multiply: 0.0261711s
matrix_multiply: 0.0282188s
matrix_RtAR: 0.0549805s
matrix_trace: 0.00934559s
1.47405e+08
test_func: 0.0692714s
overall: 0.273484s
```

# 加速比

matrix_randomize: 50x
matrix_transpose: 20x
matrix_multiply: 6x
matrix_RtAR: 4x

> 如果记录了多种优化方法，可以做表格比较

# 优化方法

下面这些函数你是如何优化的？是什么思路？用了老师上课的哪个知识点？

> matrix_randomize
更改循环XY顺序，提高cache命中率即可

> matrix_transpose
tbb+分块

> matrix_multiply
寄存器分块+内部unroll

> matrix_RtAR
static thread local

# 我的创新点

如果有，请说明。
