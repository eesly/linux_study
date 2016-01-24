最近在看无锁结构/非阻塞算法，相比与有锁结构/阻塞算法，其区别可以用乐观和悲观锁简要的概况一下

---
###### 有锁结构/阻塞算法——悲观锁
线程对共享资源进行访问或修改时，**通常会认为在这个时候肯定会有其他线程和我争这个共享资源**，如果不进行锁定，共享资源的值可能出现非预期的情况。在这种悲观的情况下，线程对共享资源进行操作之前先上了一把锁，相当于该线程独占该资源，这个时候其他线程如果想要访问该资源将被阻塞，直到拥有资源的线程进行unlock，其他线程才能继续进行竞争访问。
~~~c++
lock();
some operation;
unlock();
~~~

---
###### 无锁结构/非阻塞算法——乐观锁
乐观的情绪是指线程在访问共享资源时认为应该只有我在对其操作，其他人应该没有在访问，在这种情况下，所有线程对资源就进行直接访问。但竞争的问题还在，即真的发生了多个线程同时访问的情况，这时候会有一个线程访问到了，其他线程都访问失败。针对这种情况，非阻塞算法解决方法也很简单：**线程发现自己没有竞争过的时候就再尝试直接访问直到访问成功**。针对上面逻辑，逻辑代码大概如下
~~~c++
do{
expected = resource；//获取一次预期值
some operation;          
}while(resource == expected && 进行resource访问);
//while条件=如果实际值和预期值一样说明这期间没有其他人访问，则可以对资源进行修改
~~~
上面的逻辑代码中有两点需要注意
- 1、while条件中的两个操作必须具有原子性，不能被打断，否则无法断定在改之前真的没有其他线程对资源进行了修改。总结起来就是cas（compare and set/swap）的原子性
- 2、上面可能出现ABA问题
~~~c++
线程1在最开始拿到了expected=A；然后线程1被调度挂起
线程2拿到了expected=A，然后更新了resource=B;
线程3拿到了expected=B，然后更新了resource=A;
最后线程1被恢复时，此时线程1拿到的预期A和resource中的"A"已经不是同一个A了
~~~
这个时候上面的逻辑就有点问题（特别的在resource是一个指针的时候出现ABA就极大的可能造成程序的不可预期）。ABA问题在有些业务上可能没有影响，可以根据需求进行是否需要解决；通常解决的方案是
（1）使用double-cas；
（2）使用safe_read，即对资源进行引用计数；（针对resource是指针的情况下，由于resource是指针进行了引用计数管理后，可以确保指针指向的区域不会被释放后又复用，即避免了ABA问题）
对应的逻辑代码如下
~~~c++
//double-cas
do{
expected_A_C = resource_A_C;//这个示意不太准确，具体可以Google
some operation;          
｝while(CAS(resource, expected_A_C , new_value) == true);
//safe_read
do{
expected = safe_read(resource)；//获取一次预期值，引用计数+1
some operation;          
｝while(CAS(resource, expected, new_value) == true);
release(); //引用计数-1
~~~

---
###### 总结
- 悲观锁，简单易用，在重度竞争下更有优势
- 乐观锁，就是进行retry-loop + CAS；CAS其实也算一种锁，只不过粒度更小，在轻、中度竞争情况下相比锁性能损耗更小。

---
###### reference
- [无锁队列的实现](http://coolshell.cn/articles/8239.html/comment-page-2#comments)
- [非阻塞同步算法与CAS(Compare and Swap)无锁算法](http://www.cnblogs.com/Mainz/p/3546347.html)
