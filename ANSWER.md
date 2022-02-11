# 改进前

```
t=0: n=1120
matrix_randomize: 0.00323458s
matrix_randomize: 0.00244834s
matrix_transpose: 0.00449375s
matrix_multiply: 0.507901s
matrix_multiply: 0.482881s
matrix_RtAR: 0.995444s
matrix_trace: 2.1269e-05s
1.75932e+08
test_func: 1.00941s
t=1: n=928
matrix_randomize: 0.000608694s
matrix_randomize: 0.00140255s
matrix_transpose: 0.00288064s
matrix_multiply: 0.264023s
matrix_multiply: 0.246176s
matrix_RtAR: 0.513248s
matrix_trace: 0.000323195s
1.00156e+08
test_func: 0.519699s
t=2: n=1024
matrix_randomize: 0.00063783s
matrix_randomize: 0.000629075s
matrix_transpose: 0.00313033s
matrix_multiply: 0.358542s
matrix_multiply: 0.359807s
matrix_RtAR: 0.721592s
matrix_trace: 5.6674e-05s
1.34324e+08
test_func: 0.728255s
t=3: n=1056
matrix_randomize: 0.000902491s
matrix_randomize: 0.000780195s
matrix_transpose: 0.00387635s
matrix_multiply: 0.3977s
matrix_multiply: 0.406229s
matrix_RtAR: 0.808082s
matrix_trace: 8.5799e-05s
1.47405e+08
test_func: 0.816765s
overall: 3.07871s
```

# 改进后

```
t=0: n=1120
matrix_randomize: 0.00347719s
matrix_randomize: 0.00243836s
matrix_transpose: 0.00556866s
matrix_multiply: 0.106349s
matrix_multiply: 0.0895811s
matrix_RtAR: 0.208163s
matrix_trace: 0.000257071s
1.76466e+08
test_func: 0.223656s
t=1: n=928
matrix_randomize: 0.00487382s
matrix_randomize: 0.00245537s
matrix_transpose: 0.00080515s
matrix_multiply: 0.0597233s
matrix_multiply: 0.0543553s
matrix_RtAR: 0.115015s
matrix_trace: 0.000258041s
1.00585e+08
test_func: 0.127455s
t=2: n=1024
matrix_randomize: 0.000620257s
matrix_randomize: 0.00175319s
matrix_transpose: 0.00213852s
matrix_multiply: 0.0661278s
matrix_multiply: 0.065029s
matrix_RtAR: 0.133401s
matrix_trace: 0.000278096s
1.34691e+08
test_func: 0.143559s
t=3: n=1056
matrix_randomize: 0.00101717s
matrix_randomize: 0.00216396s
matrix_transpose: 0.00102147s
matrix_multiply: 0.0686898s
matrix_multiply: 0.0643553s
matrix_RtAR: 0.134125s
matrix_trace: 0.000949931s
1.47779e+08
test_func: 0.146622s
overall: 0.643736s
```

# 加速比

matrix_randomize: 10x
matrix_transpose: 3x
matrix_multiply: 7x
matrix_RtAR: 8x

> 如果记录了多种优化方法，可以做表格比较

# 优化方法

下面这些函数你是如何优化的？是什么思路？用了老师上课的哪个知识点？

> matrix_randomize

   answer: 
        这是因为 YX 序的数组，X 方向在内存空间中的排布是连续的
        YX 序的循环，其 X 是内层循环体，因此在先后执行的时间上是连续的。
        对于 YX 序（列主序，C/C++）的数组，请用 YX 序遍历（x变量做内层循环体）

   answer:
        _mm_stream_ps 可以一次性写入 16 字节到挂起队列，更加高效了
        他的第二参数是一个 __m128 类型，可以配合其他手写的 SIMD 指令使用
        不过，_mm_stream_ps 写入的地址必须对齐到 16 字节，否则会产生段错误等异常
        需要注意，stream 系列指令写入的地址，必须是连续的，中间不能有跨步，否则无法合并写入，会产生有中间数据读的带宽
        
> matrix_transpose

    answer:
        循环是 XY 序的，虽然 out 也是 XY 序的没问题，但是 in 相当于一个 YX 序的二维数组，
        从而在内存看来访存是跳跃的，违背了空间局域性。因为每次跳跃了 ny，所以只要缓存容量小于 ny 就无法命中。
        解决方法当然还是循环分块。
        这样只需要块的大小 blockSize^2 小于缓存容量，即可保证全部命中。

    answer:
        tbb::simple_partitioner 自带莫顿序遍历功能 
        保证对齐到16字节

> matrix_multiply

    answer:
        out(x, y) 始终在一个地址不动（一般）。
        lhs(x, t) 每次跳跃 n 间隔的访问（坏）。
        rhs(t, y) 连续的顺序访问（好）。
        因为存在不连续的 lhs 和一直不动的 out，导致矢量化失败，一次只能处理一个标量，CPU也无法启动指令级并行（ILP）

    answer:
        #pragma omp unroll

    answer:
        out(i, j) 连续 32 次顺序访问（好）。
        lhs(i, t) 连续 32 次顺序访问（好）。
        rhs(t, j) 32 次在一个地址不动（一般）。
        这样就消除不连续的访问了，从而内部的 i 循环可以顺利矢量化，且多个循环体之间没有依赖关系，CPU得以启动指令级并行，缓存预取也能正常工作，快好多！


> matrix_RtAR

static  预先分配好空间

请回答。

# 我的创新点

如果有，请说明。

