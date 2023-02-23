# 改进前

```
t=0: n=1120
matrix_randomize: 1640300ns
matrix_randomize: 905350ns
matrix_transpose: 2858292ns
matrix_multiply: 124426955ns
matrix_multiply: 122302470ns
matrix_RtAR: 249622821ns
matrix_trace: 6950ns
1.75932e+08
test_func: 261158589ns
t=1: n=928
matrix_randomize: 219398ns
matrix_randomize: 216040ns
matrix_transpose: 1480696ns
matrix_multiply: 69361807ns
matrix_multiply: 69488155ns
matrix_RtAR: 140357708ns
matrix_trace: 4406ns
1.00156e+08
test_func: 143469454ns
t=2: n=1024
matrix_randomize: 387866ns
matrix_randomize: 382464ns
matrix_transpose: 2237437ns
matrix_multiply: 170670256ns
matrix_multiply: 170944211ns
matrix_RtAR: 343932593ns
matrix_trace: 6656ns
1.34324e+08
test_func: 348436713ns
t=3: n=1056
matrix_randomize: 278548ns
matrix_randomize: 303162ns
matrix_transpose: 2244654ns
matrix_multiply: 102034204ns
matrix_multiply: 101822940ns
matrix_RtAR: 206129992ns
matrix_trace: 5291ns
1.47405e+08
test_func: 210509771ns
overall: 965910915ns
```

# 改进后

940,043,114
122,873,140
```
t=0: n=1120
matrix_randomize: 1149905ns
matrix_randomize: 194806ns
matrix_transpose: 3380612ns
matrix_multiply: 22715600ns
matrix_multiply: 16198907ns
matrix_RtAR: 42313253ns
matrix_trace: 4655ns
1.75932e+08
test_func: 53439795ns
t=1: n=928
matrix_randomize: 120288ns
matrix_randomize: 104550ns
matrix_transpose: 1505999ns
matrix_multiply: 9278394ns
matrix_multiply: 9541630ns
matrix_RtAR: 20339630ns
matrix_trace: 3784ns
1.00156e+08
test_func: 23224326ns
t=2: n=1024
matrix_randomize: 132620ns
matrix_randomize: 129305ns
matrix_transpose: 2188643ns
matrix_multiply: 14148323ns
matrix_multiply: 13984732ns
matrix_RtAR: 30337879ns
matrix_trace: 3876ns
1.34324e+08
test_func: 32501406ns
t=3: n=1056
matrix_randomize: 132218ns
matrix_randomize: 133312ns
matrix_transpose: 2499468ns
matrix_multiply: 13799665ns
matrix_multiply: 13568507ns
matrix_RtAR: 29882378ns
matrix_trace: 4531ns
1.47405e+08
test_func: 34571786ns
overall: 144688938ns
```

# 加速比

matrix_randomize: 2.0663
matrix_transpose: 0.9212
matrix_multiply:  8.2222
matrix_RtAR:      7.6505

> 如果记录了多种优化方法，可以做表格比较

# 优化方法

下面这些函数你是如何优化的？是什么思路？用了老师上课的哪个知识点？

> matrix_randomize
> 这个循环为什么不够高效？如何优化？ 10 分
>   跳跃访存，缓存利用率低下
> 优化：
>   1.改变xy序为yx序，使得访存连续，利用空间局域性
>   2.使用直写指令，降低一倍访存带宽

请回答。

> matrix_transpose
>这个循环为什么不够高效？如何优化？ 15 分
>   矩阵转置这种访存读写一方一定存在一个是跳跃访存的，导致缓存利用率低
>优化：
>   1. 先对矩阵进行分块，然后访问块的顺序按照莫顿序
>   2. 直写

请回答。

> matrix_multiply
>这个循环为什么不够高效？如何优化？ 15 分
>    程序是mem-bound，三个矩阵的访存中
>        out(x, y)顺序访问，高效；
>        lhs(x, t)存储是x主序，但是按t主序访问，跨步访问，低效；
>        rhs(t, y)访问同一个地址，一般
>    缓存(L1)大小不够存下三个矩阵，命中率低
> 优化：
>    1. 循环分块，这样缓存可以放下一个块的大小，提高缓存命中率
>    2. 寄存器分块，指令级并行
>    3. 循环展开

请回答。

> matrix_RtAR
>    这两个是临时变量，有什么可以优化的？ 5 分
>    1. 可以省去Rt,使用RtAR代替
>    2. 使用static，手动池化

请回答。

# 我的创新点

对于分块的行或列大小不是2的倍数的情况，使用莫顿序需要判断越界。创新之处在于负优化。
