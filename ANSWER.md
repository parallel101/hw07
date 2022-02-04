
| CPU 核心 | 6         |
| :------- | --------- |
| L1  数据 | 6  x 32 K |
| L1 指令  | 6  x 32 K |
| L2       | 6 x 256 K |
| L3       | 12 M      |



# 改进前

### 初始结果：（未开启OpenMP优化）

```
t=0: n=1120
matrix_randomize: 0.0047392s
matrix_randomize: 0.0039861s
matrix_transpose: 0.0025276s
matrix_multiply: 3.3746s
matrix_multiply: 3.31677s
matrix_RtAR: 6.69637s
matrix_trace: 3.15e-05s
1.75932e+08
test_func: 6.70862s
t=1: n=928
matrix_randomize: 0.0020972s
matrix_randomize: 0.0021105s
matrix_transpose: 0.0014424s
matrix_multiply: 1.77163s
matrix_multiply: 1.77209s
matrix_RtAR: 3.54777s
matrix_trace: 2.92e-05s
1.00156e+08
test_func: 3.55457s
t=2: n=1024
matrix_randomize: 0.0030934s
matrix_randomize: 0.0029501s
matrix_transpose: 0.0026326s
matrix_multiply: 3.043s
matrix_multiply: 3.06595s
matrix_RtAR: 6.11384s
matrix_trace: 2.95e-05s
1.34324e+08
test_func: 6.12345s
t=3: n=1056
matrix_randomize: 0.0027326s
matrix_randomize: 0.0026625s
matrix_transpose: 0.0019026s
matrix_multiply: 2.6474s
matrix_multiply: 2.62987s
matrix_RtAR: 5.28161s
matrix_trace: 3.22e-05s
1.47405e+08
test_func: 5.29054s
overall: 21.6809s
```

### 开启OpenMP优化

```
t=0: n=1120
matrix_randomize: 0.0068874s
matrix_randomize: 0.002048s
matrix_transpose: 0.0022631s
matrix_multiply: 0.807792s
matrix_multiply: 0.754302s
matrix_RtAR: 1.5655s
matrix_trace: 4.23e-05s
1.75932e+08
test_func: 1.57837s
t=1: n=928
matrix_randomize: 0.0007808s
matrix_randomize: 0.0006188s
matrix_transpose: 0.0010147s
matrix_multiply: 0.363695s
matrix_multiply: 0.395745s
matrix_RtAR: 0.761718s
matrix_trace: 5.13e-05s
1.00156e+08
test_func: 0.766641s
t=2: n=1024
matrix_randomize: 0.0007518s
matrix_randomize: 0.0010314s
matrix_transpose: 0.0024472s
matrix_multiply: 0.698181s
matrix_multiply: 0.687765s
matrix_RtAR: 1.38938s
matrix_trace: 9.16e-05s
1.34324e+08
test_func: 1.39417s
t=3: n=1056
matrix_randomize: 0.0010823s
matrix_randomize: 0.0010954s
matrix_transpose: 0.0020503s
matrix_multiply: 0.667081s
matrix_multiply: 0.682925s
matrix_RtAR: 1.35404s
matrix_trace: 4.5e-05s
1.47405e+08
test_func: 1.36134s
overall: 5.10537s
```



# 改进后

```
t=0: n=1120
matrix_randomize: 0.0022972s
matrix_randomize: 0.0009744s
matrix_transpose: 0.0018461s
matrix_multiply: 0.439565s
matrix_multiply: 0.37543s
matrix_RtAR: 0.818067s
matrix_trace: 5.74e-05s
1.75932e+08
test_func: 0.824487s
t=1: n=928
matrix_randomize: 0.0004208s
matrix_randomize: 0.0004122s
matrix_transpose: 0.0007337s
matrix_multiply: 0.213688s
matrix_multiply: 0.197275s
matrix_RtAR: 0.413488s
matrix_trace: 3.17e-05s
1.00156e+08
test_func: 0.41781s
t=2: n=1024
matrix_randomize: 0.0005401s
matrix_randomize: 0.0006231s
matrix_transpose: 0.0007753s
matrix_multiply: 0.280031s
matrix_multiply: 0.27925s
matrix_RtAR: 0.561334s
matrix_trace: 5.72e-05s
1.34324e+08
test_func: 0.565142s
t=3: n=1056
matrix_randomize: 0.0006056s
matrix_randomize: 0.0005789s
matrix_transpose: 0.0007156s
matrix_multiply: 0.299905s
matrix_multiply: 0.31011s
matrix_RtAR: 0.612472s
matrix_trace: 0.0004518s
1.47405e+08
test_func: 0.618392s
overall: 2.43144s
```



# 加速比

由于`Clion+MSVC`我还没发现如何开启OpenMP优化；

切换到 `Visual studio`后则可以开启OpenMP优化，速率大概提升了四倍左右；



**使用各种优化方法的效果，比较的基准为开启OpenMP的情况；**

`randomize` 和 `transpose`各使用了更改遍历序和TBB的优化方法；

|                  | OpenMP改遍历序/开启OpenMP优化 | TBB/开启OpenMP优化 |
| ---------------- | ----------------------------- | ------------------ |
| matrix_randomize | 2.3                           | 2.3                |
| matrix_transpose | 1.4                           | 2.1                |



| matrix_multiply | 2.13 |
| --------------- | ---- |
| matrix_RtAR     | 2.2  |



> 如果记录了多种优化方法，可以做表格比较

# 优化方法

下面这些函数你是如何优化的？是什么思路？用了老师上课的哪个知识点？

> matrix_randomize

**优化** 

- 使用YX序遍历，即x作为内存循环体，这样其在时间上是连续的。
- 遍历时使用`tbb::parallel_for`

> matrix_transpose

**优化**

- 循环分块，使用`YXyx`序，只要保证`BlockSize^2`小于下缓存容量即可
- `tbb`自带莫顿序遍历功能

> matrix_multiply

**优化**：

- 使用寄存器分块

> matrix_RtAR

优化：

- 使用手动池化 `static thread_local`

# 我的创新点

如果有，请说明。

