# 改进前

```
这里贴改进前的运行结果。
matrix_randomize: 0.026172s
matrix_transpose: 0.0467481s
matrix_multiply: 1.20873s
matrix_RtAR: 2.38471s
```

# 改进后

```
这里贴改进后的运行结果。
matrix_randomize: 0.00358723s
matrix_transpose: 0.000846697s
matrix_multiply: 0.433022s
matrix_RtAR: 0.870843s
```

# 加速比
```
matrix_randomize: 74x
matrix_transpose: 55x
matrix_multiply: 2.79x
matrix_RtAR: 2.73x

```

> 如果记录了多种优化方法，可以做表格比较

# 优化方法

下面这些函数你是如何优化的？是什么思路？用了老师上课的哪个知识点？

> matrix_randomize

教给了tbb， 写连续地址，希望tbb能帮我优化成一次写一个缓存行。

> matrix_transpose

交给了tbb， 我知道访问内存不连续，而且还有写不连续， 希望tbb 给我一个完美答案

> matrix_multiply

交给了tbb 来做， 我尝试了不同的粒度和partitioner的方法， 最后确定simple 的在我机器上跑的快点。

> matrix_RtAR

我并没有什么思路， 感觉预分配一个大小会快点。 😊 

# 我的创新点

如果有，请说明。
