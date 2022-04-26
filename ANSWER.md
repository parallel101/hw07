# 改进前

```
...
t=2: n=1024
matrix_randomize: 0.00138448s
matrix_randomize: 0.00140546s
matrix_transpose: 0.00400085s
matrix_multiply: 1.32128s
matrix_multiply: 1.37719s
matrix_RtAR: 2.7027s
matrix_trace: 6.573e-06s
1.34324e+08
test_func: 2.70884s
overall: 11.703s
...
```

# 改进后

```
t=2: n=1024
matrix_randomize: 0.000223787s
matrix_randomize: 0.000234784s
matrix_transpose: 0.000950278s
matrix_multiply: 0.0211322s
matrix_multiply: 0.0236634s
matrix_RtAR: 0.045924s
matrix_trace: 6.171e-06s
1.34277e+08
test_func: 0.0502626s
...
overall: 0.228116s

```

# 加速比

matrix_randomize: 6.18x
matrix_transpose: 4.21x
matrix_multiply: 62.52x
matrix_RtAR: 58.85x

> 如果记录了多种优化方法，可以做表格比较

# 优化方法

下面这些函数你是如何优化的？是什么思路？用了老师上课的哪个知识点？

> matrix_randomize

- 在原函数中，yx序的矩阵使用xy序遍历。交换xy遍历顺序即可优化为顺序访问。(O3优化似乎自动帮你做了)
- 可以使用`_mm_stream_si32`绕过缓存写入。
- 也可以使用`_mm_stream_ps`绕过缓存写入，但这要求首先计算4次`random`，有可能变成CPU-bound. 或者可以设计一个每次输出一个128比特向量的random.
  - 如果数组大小不是4的倍数，边界需要特殊处理，或者申请数组时向外扩张4个float以免越界。

> matrix_transpose

- 可以使用简单的分块循环，每次转置一个直径为64的块
  - 需要注意越界问题，需要边缘扩展64字节
- (未使用)可以使用莫顿码遍历，但由于注意到矩阵直径不是2的整数幂所以可以使用tbb的`simple_partitioner`自带的莫顿码遍历
- 使用`_mm_stream_si32`绕过缓存写入

> matrix_multiply

- 需要手动初始化，因为无法保证Matrix全为0，因为它是作为参数传入而不是作为临时变量构造的。
  - 但是可以使用memset进行优化
- 首先可以交换t和x的遍历顺序，因为赋值语句是`out(x, y) += lhs(x, t) * rhs(t, y);`让t和y不要动，否则就是跳着遍历效率低。
- 经测试，在我的电脑上如果进行分块会导致更慢...

> matrix_RtAR

- 这两个是临时变量，有什么可以优化的？ 5 分
- 使用`static`关键字简单池化避免重复分配销毁。

# 我的创新点

如果有，请说明。
