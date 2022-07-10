# 改进前

```
t=0: n=1120
matrix_randomize: 0.0791472s
matrix_randomize: 0.0525622s
matrix_transpose: 0.0595276s
matrix_multiply: 0.974533s
matrix_multiply: 0.7107s
matrix_RtAR: 1.74504s
matrix_trace: 0.0297299s
1.75932e+08
test_func: 1.91395s
t=1: n=928
matrix_randomize: 0.0284199s
matrix_randomize: 0.0295304s
matrix_transpose: 0.0318627s
matrix_multiply: 0.18651s
matrix_multiply: 0.177179s
matrix_RtAR: 0.395645s
matrix_trace: 0.0200901s
1.00156e+08
test_func: 0.478293s
t=2: n=1024
matrix_randomize: 0.0242934s
matrix_randomize: 0.0295988s
matrix_transpose: 0.0409102s
matrix_multiply: 0.469459s
matrix_multiply: 0.526811s
matrix_RtAR: 1.03738s
matrix_trace: 0.0315037s
1.34324e+08
test_func: 1.12798s
t=3: n=1056
matrix_randomize: 0.0323473s
matrix_randomize: 0.0290094s
matrix_transpose: 0.0358525s
matrix_multiply: 0.528684s
matrix_multiply: 0.538887s
matrix_RtAR: 1.10351s
matrix_trace: 0.03619s
1.47405e+08
test_func: 1.20674s
overall: 4.73322s
```

# 改进后

```
t=0: n=1120
matrix_randomize: 0.000528096s
matrix_randomize: 0.000535718s
matrix_transpose: 0.0022195s
matrix_multiply: 0.160545s
matrix_multiply: 0.127222s
matrix_RtAR: 0.296209s
matrix_trace: 0.0278956s
1.75932e+08
test_func: 0.331247s
t=1: n=928
matrix_randomize: 0.000358425s
matrix_randomize: 0.000367327s
matrix_transpose: 0.0012165s
matrix_multiply: 0.0528992s
matrix_multiply: 0.0683825s
matrix_RtAR: 0.122551s
matrix_trace: 0.043051s
1.00156e+08
test_func: 0.170517s
t=2: n=1024
matrix_randomize: 0.000449409s
matrix_randomize: 0.000480594s
matrix_transpose: 0.0016001s
matrix_multiply: 0.0647215s
matrix_multiply: 0.091688s
matrix_RtAR: 0.158071s
matrix_trace: 0.0399191s
1.34324e+08
test_func: 0.203998s
t=3: n=1056
matrix_randomize: 0.000623919s
matrix_randomize: 0.000620208s
matrix_transpose: 0.00164745s
matrix_multiply: 0.0739627s
matrix_multiply: 0.0657667s
matrix_RtAR: 0.141427s
matrix_trace: 0.0281347s
1.47405e+08
test_func: 0.176396s
overall: 0.883619s
```

# 加速比

matrix_randomize: 46x到150x
matrix_transpose: 21x到27x
matrix_multiply:  2.5x到8.2x
matrix_RtAR:      3.2x到7.8x

# 优化方法

下面这些函数你是如何优化的？是什么思路？用了老师上课的哪个知识点？

> matrix_randomize

尝试过的优化手段
- 先遍历y,后遍历x: 增强空间局部性，提高cpu cache命令率
- _mm_stream_si32或_mm512_stream_ps (无提升)
- matrix对齐到4096 (无提升)


> matrix_transpose

尝试过的优化手段
- 分块计算
 

> matrix_multiply

尝试过的优化手段
- 寄存器分块，步长32 
- openmp多线程
- gcc循环展开
- 去掉对out(x,y)赋值为零的操作。(提升不明显)

> matrix_RtAR

尝试过的优化手段
- 去掉临时变量，改成static, 加上初始化


# 我的创新点
暂无
