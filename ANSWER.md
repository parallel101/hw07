# 环境
CPU(s):              48
On-line CPU(s) list: 0-47
Thread(s) per core:  2
L1d cache:           32K
L1i cache:           32K
L2 cache:            256K
L3 cache:            30720K


# randomize
## 改进前
```
原始数据 size_t n = 32 * (rng.next_uint64() % 16 + 24)
t=0: n=1120
matrix_randomize: 0.00722261s
matrix_randomize: 0.00122988s

t=1: n=928
matrix_randomize: 0.000710999s
matrix_randomize: 0.000410166s

t=2: n=1024
matrix_randomize: 0.000646529s
matrix_randomize: 0.000539234s

t=3: n=1056
matrix_randomize: 0.00059311s
matrix_randomize: 0.000655608s

```
(0.00722261 + 0.00122988 + 0.000710999 + 0.000410166 + 0.000646529 + 0.000539234 + 0.00059311 + 0.000655608 ) / 8 = 0.001501017


```
1G数据 size_t n = 32 * (rng.next_uint64() % 16 + 24) * 20;
t=0: n=22400
matrix_randomize: 1.37512s
matrix_randomize: 1.2348s
test_func: 4.62349s
t=1: n=18560
matrix_randomize: 0.762454s
matrix_randomize: 0.709083s
test_func: 2.82452s
t=2: n=20480
matrix_randomize: 1.22947s
matrix_randomize: 1.39852s
test_func: 4.28212s
t=3: n=21120
matrix_randomize: 1.04415s
matrix_randomize: 1.06759s
test_func: 3.82594s
overall: 16.2411s
```
(1.37512+1.2348+0.762454+0.709083+1.22947+1.39852+1.04415+1.06759)/8 = 1.1026483749999998

## 改进后

```
原始数据 size_t n = 32 * (rng.next_uint64() % 16 + 24)
t=0: n=1120
matrix_randomize: 0.00737349s
matrix_randomize: 0.000541591s

t=1: n=928
matrix_randomize: 0.000393746s
matrix_randomize: 0.000375455s

t=2: n=1024
matrix_randomize: 0.000367141s
matrix_randomize: 0.000707518s

t=3: n=1056
matrix_randomize: 0.000370469s
matrix_randomize: 0.000512963s
```
(0.00737349+0.000541591+0.000393746+0.000375455+0.000367141+0.000707518+0.000370469+0.000512963) / 8 = 0.001330296625


```
1G 数据：size_t n = 32 * (rng.next_uint64() % 16 + 24) * 20;


t=0: n=22400
matrix_randomize: 0.0764535s
matrix_randomize: 0.183382s
test_func: 2.33063s
t=1: n=18560
matrix_randomize: 0.125141s
matrix_randomize: 0.0510746s
test_func: 1.58189s
t=2: n=20480
matrix_randomize: 0.0598267s
matrix_randomize: 0.156077s
test_func: 1.93101s
t=3: n=21120
matrix_randomize: 0.165283s
matrix_randomize: 0.0625712s
test_func: 2.09014s
overall: 8.67846s
```
(0.0764535+ 0.183382+ 0.125141+0.0510746+0.0598267+ 0.156077+ 0.165283+0.0625712 ) / 8 = 0.10997612500000001

## 加速比

matrix_randomize: 10.026x ; 对矩阵大小进行了修改，n = 32 * (rng.next_uint64() % 16 + 24) * 20; 因为原来的n太小，三级cache就能把矩阵都放进去了，基本没有加速。






# transpose
## 改进前
```
t=0: n=44800
matrix_randomize: 0.261456s
matrix_randomize: 0.262549s
matrix_transpose: 8.39193s
matrix_transpose: 8.84256s
matrix_transpose: 4.72458s
matrix_transpose: 6.28754s
matrix_transpose: 5.01172s
```

## 改进后
```
t=0: n=44800
matrix_randomize: 0.264891s
matrix_randomize: 0.587853s
matrix_transpose: 4.6243s
matrix_transpose: 1.51406s
matrix_transpose: 1.48309s
matrix_transpose: 2.88639s
matrix_transpose: 2.11405s
```


## 加速比
transpose: 2.3707x



# multiply
## 改进前
```
t=0: n=1120
matrix_randomize: 0.00686121s
matrix_randomize: 0.0020274s
matrix_transpose: 0.00631455s
matrix_multiply: 0.510823s
matrix_multiply: 0.479044s
matrix_RtAR: 0.996241s
matrix_trace: 1.8018e-05s
1.75932e+08
test_func: 1.01514s
t=1: n=928
matrix_randomize: 0.000342635s
matrix_randomize: 0.00167575s
matrix_transpose: 0.00546872s
matrix_multiply: 0.244366s
matrix_multiply: 0.244578s
matrix_RtAR: 0.494883s
matrix_trace: 6.2349e-05s
1.00156e+08
test_func: 0.502805s
t=2: n=1024
matrix_randomize: 0.00041904s
matrix_randomize: 0.000321632s
matrix_transpose: 0.0027552s
matrix_multiply: 0.351989s
matrix_multiply: 0.352222s
matrix_RtAR: 0.707403s
matrix_trace: 0.000112475s
1.34324e+08
test_func: 0.714231s
t=3: n=1056
matrix_randomize: 0.000361293s
matrix_randomize: 0.000411746s
matrix_transpose: 0.00290072s
matrix_multiply: 0.39902s
matrix_multiply: 0.396405s
matrix_RtAR: 0.798926s
matrix_trace: 0.00153154s
1.47405e+08
test_func: 0.806829s
overall: 3.04113s
```
(0.510823+0.479044+0.244366+0.244578+0.351989+0.352222+0.39902+0.396405)/8=0.372305875
## 改进后
```
t=0: n=1120
matrix_randomize: 0.00837036s
matrix_randomize: 0.000462134s
matrix_transpose: 0.0056561s
matrix_multiply: 0.0813393s
matrix_multiply: 0.0748275s
matrix_RtAR: 0.161888s
matrix_trace: 1.9308e-05s
1.76466e+08
test_func: 0.180869s
t=1: n=928
matrix_randomize: 0.000277639s
matrix_randomize: 0.00205225s
matrix_transpose: 0.00221065s
matrix_multiply: 0.0436374s
matrix_multiply: 0.0386557s
matrix_RtAR: 0.084553s
matrix_trace: 0.00322023s
1.00585e+08
test_func: 0.094229s
t=2: n=1024
matrix_randomize: 0.00182822s
matrix_randomize: 0.000406697s
matrix_transpose: 0.0028676s
matrix_multiply: 0.0494244s
matrix_multiply: 0.0495718s
matrix_RtAR: 0.102309s
matrix_trace: 6.6259e-05s
1.34691e+08
test_func: 0.110543s
t=3: n=1056
matrix_randomize: 0.000332161s
matrix_randomize: 0.000432429s
matrix_transpose: 0.00303381s
matrix_multiply: 0.0559908s
matrix_multiply: 0.0543113s
matrix_RtAR: 0.113629s
matrix_trace: 6.0144e-05s
1.47779e+08
test_func: 0.120749s
overall: 0.508416s
```
(0.0813393+0.0748275+0.0436374+0.0386557+0.0494244+0.0495718+0.0559908+0.0543113)/8=0.055969775

## 加速比
multiply: 6.65x


> 如果记录了多种优化方法，可以做表格比较

# 优化方法

下面这些函数你是如何优化的？是什么思路？用了老师上课的哪个知识点？

> matrix_randomize

使用_mm_stream_ps直写。

> matrix_transpose

使用tbb的simple_partitioner，其内置的morton能够充分利用cache。

> matrix_multiply

使用分块的思想。只对x进行了分块足矣。

> matrix_RtAR

Rt 和 RtA 改为static变量，预先分配好空间。


# 我的创新点

如果有，请说明。
