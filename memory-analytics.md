# memory-analytics
####linux内存分配模型+malloc/free+共享内存
---
######1、Linux虚拟地址空间分配
Linux提供了两种系统调用用于手动内存分配，分别为brk和mmap

![虚拟内存分配示意图](http://upload-images.jianshu.io/upload_images/46850-6a3ba4bb5642d7e7.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
- 1、调用brk()，进行系统内存分配、回收实质是将上图的brk指针向高地址或向低地址移动，该区域内存就是通常所说的堆。
 - 1.1在堆内部对内存块的管理方式有很多种，都类似于一个链表结构，将每个内存块串起来，由于需要管理结构、页对齐等原因导致采用堆中占用的物理要大于实际分配的内存
 - 1.2堆的分配和回收，只表现了一个brk指针向高低地址的移动，因此堆的释放只能从brk指针开始向下移，这将导致内存锁片（外部锁片），即当靠近brk的top内存块还未释放时，释放堆中其他区域的内存时，实际上不会真正释放内存（即不会真正还给系统，还是占着物理物理空间，系统会进行标记为空闲区域，并按一定策略进行合并，在满足后续需要分配的内存大小时，会直接分配这些空闲区块，而不再向系统申请）
- 2、调用mmap进行内存分配，实际上是申请一段虚拟地址空间（堆和栈之间的空闲区域），然后跟进实际是否进行读写操作，将该段内存区域与实际的物理内存（可以是主存、swap或者磁盘文件）进行映射。
 - 采用这个方式分配的内存在释放的时候，就直接将物理资源还给了系统

######2、malloc/free
malloc和free实现方式有很多种，基于上述模型gcc的malloc/free实现有如下特点
- 2.1对于小的内存块malloc采用brk实现，对于大的内存块malloc采用mmap实现（例外，当采用brk分配内存块中空闲区块的大小足以满足当前的需要分配的大小时，直接分配这些空闲区块，即使此时需要分配的是大内存也不会采用mmap分配），这里内存的大和小可以利用下列函数进行调整
~~~c
mallopt(M_MMAP_THRESHOLD, value) 如果未设置这个阈值为128k
~~~
- 2.2基于上述的实现方式malloc分配的小内存（调用brk获得的）在free的时候就表现为两个特点：（1）free非top的内存块，不会直接还给系统；（2）top内存块释放时，也需要大于一定的阈值才会直接还给系统，设置阈值函数如下
~~~c
mallopt(M_TRIM_THRESHOLD, value) 如果未设置这个阈值为128k
~~~
man page还提到可以采用malloc_trim(pad)这个函数强制将top的空闲区域还给系统，pad是指在top上还保留多少空闲内存，如果为0则保留必要最小top内存（这个函数未准确验证）。此外，malloc在使用brk分配内存时会预分配一小段内存区域。
- 2.3采用mmap分配的内存，在调用free后，就直接还给系统

######3、共享内存
共享内存常用于进程间的通信，linux下的system v共享内存可以实现进程间快速通信，这里简单介绍system v共享内存对进程内存分配的影响
- 1、system v共享内存通过映射特殊文件系统shm中的文件实现进程间的共享内存，基本使用流程如下
~~~
shmget创建共享内存->shmat将共享内存attach到进程地址空间中
->使用->shmdt在进程地址空间detach共享内存
->(如果确定没有其他进程使用，可以用shmctl删除共享内存)
~~~
- 2、在创建共享内存区域时，并不会为在进程的地址空间划分地址段，只有attach的时候，才分配进程虚拟地址段，在具体读写时，才会发生缺页，进行映射实际物理空间，detach后进程会回收分配出去的虚拟地址段。

上述简单介绍了linux下进程内存空间分配管理有关的几个点（内容基本来自网上，不一定准确），在这个基础上，下面介绍一下在发生内存泄漏时候的一些简单检查和定位方法。

####内存泄漏
---
######1、内存泄漏检测
- （1）、linux下内存泄漏检测有一款利器valgrind，简单使用方法如下，详细内容参见[valgrind manual](http://valgrind.org/docs/manual/mc-manual.html)
~~~c
valgrind --track-origins=yes --leak-check=full --show-reachable=yes ./程序cmd
~~~
这个工具可以解决大部问题，但是在一些情况下，不是很好处理，例如daemon进程（或许有这个功能，还没看到），下面介绍一些简单的方法检查进程是否内存泄漏
- （2）、linux下最简单查看内存的工具就是top
对于内存泄漏主要看RES和SHR这两个字段，RES表示进程实际占用物理内存的总大小，SHR表示其中共享内存实际占用的物理内存大小，单位kb,m,g。
![top](http://upload-images.jianshu.io/upload_images/46850-39602bc65640e445.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
 - 对于未手动建立共享内存块的进程，其共享内存块的大小一般相对稳定，可以长时间运行观察RES是不是持续增长
 - 对于手动建立共享内存块的进程，在进程运行过程中会随着对共享内存块读写操作，逐渐将分配的共享内存块一一映射到物理内存上，即SHR不断增大。因此这个时候观察内存泄漏需要扣除SHR的影响，观察RES-SHR是否随时间不断增长
- （3）、查看/proc/pid/smaps文件，抓取其堆段文件内容如下，各个字段的详细解释参见[ THE/procFILESYSTEM](https://www.kernel.org/doc/Documentation/filesystems/proc.txt)
~~~c
>cat /proc/2236/smaps | grep "\[heap\]" -A 20
虚拟内存范围  权限  在内存段中偏移 主次设备号  索引节点号
029e2000-041fe000 rw-p 00000000 00:00 0                                  [heap]
Size:              24688 kB    虚拟地址空间大小
Rss:               24436 kB    实际占用物理内存大小
Pss:               24436 kB    平摊共享内存后的实际占用物理内存大小
Shared_Clean:          0 kB    
Shared_Dirty:          0 kB
Private_Clean:         0 kB
Private_Dirty:     24436 kB
Referenced:        23884 kB
Anonymous:         24436 kB    不来自文件的内存大小
AnonHugePages:     20480 kB
Swap:                  0 kB
KernelPageSize:        4 kB
MMUPageSize:           4 kB
3195600000-3195620000 r-xp 00000000 08:02 654566                      map映射段
~~~
 - 上面的heap段就是采用brk分配的内存段，其中Rss是起实际占用的物理内存，由于brk的工作模式，在heap段占用很大的内存情况不一定是发生内存泄漏。有可能是不合理的malloc 和 free使用导致大量的内存锁片（这种时候通常也是表现为占用的大量的物理内存，但采用工具检查看不出有泄漏地方），但这也是程序的一个问题，此时可以重点关注程序里面分配的小块的内存分配和释放。
 - heap段之后连续的没有标明用途具有rw权限的地址段，通常是mmap分配的内存段，对于一个工作正常长期运行的进程，其mmap段分配占用的物理内存一定个稳定值，如果发现mmap分配的内存段占用的大小随时间增长则可以肯定发生了内存泄漏，并且通常是大的内存快分配未释放导致的。

######2、内存泄漏定位
通过上面方法简单确定内存泄漏是大内存块还是小内存块未释放后，可以通过下面这个方法进行更进一步的定位泄漏的具体位置
- malloc系列函数中有一个malloc_stats()，可以打印出当前程序所占用和正在使用的内存信息，打印信息如下
~~~c
Arena 0:
system bytes     =          0    占用的内存大小，不包括mmap分配的内存
in use bytes     =          0    实际占用的内存大小，不包括mmap分配部分
Total (incl. mmap):              下面两个是包含mmap分配的内存后的统计
system bytes     =          0
in use bytes     =          0
max mmap regions =          0
max mmap bytes   =          0
~~~
据此写一个简单的宏，如下所示，则可以在程序需要知道当前内存分配情况的地方直接调用即可
~~~c
#define PRINT_MEM(formats, args...) printf(formats, ##args);malloc_stats();
~~~
- 还有另外一个函数mallinfo()提供更加详细的内存信息结构mallinfo，其具有如下成员，重点关注其中翻译的部分
~~~c
arena     brk分配的总内存大小byte
ordblks   The number of ordinary (i.e., non-fastbin) free blocks.
smblks    The number of fastbin free blocks (see mallopt(3)).
hblks     The number of blocks currently allocated using mmap(2). 
hblkhd    mmap分配的并正在使用的内存大小.
usmblks   maximum amount of space that was ever allocated.  
fsmblks   The total number of bytes in fastbin free blocks.
uordblks  brk分配的内存中正在使用的大小
fordblks  brk分配的内存中空闲的大小
keepcost  malloc_trim可以free的理论最大值
~~~
据此写一个简单的宏，如下所示，在一个待检查模块前后分别调用M1和M2，则可以提供每个模块对内存分配的影响。从而可以确定模块内是否发生了内存泄漏。从而可以进行逐模块排查。
~~~c
#ifndef LEAKCHECKTOOL_H
#define LEAKCHECKTOOL_H
#include "malloc.h"
struct mallinfo mi1,mi2;
//uordblks是采用brk分配的
#define M1      mi1 = mallinfo()
#define M2(x)   mi2 = mallinfo();  \
                  if (mi1.uordblks != mi2.uordblks) fprintf(stderr, "%s consumed %d bytes\n", (x), mi2.uordblks - mi1.uordblks); \
                  if (mi1.hblkhd != mi2.hblkhd) fprintf(stderr, "%s consumed %d bytes\n", (x), mi2.hblkhd - mi1.hblkhd);
#endif
~~~

####补充1-利用valgrind+gdb在调试时进行内存检查，定位内存泄露
---
基本原理：利用valgrind提供的内存分配函数，替换系统自带的内存分配，从而可以在gdb调试过程中，利用valgrind自带的对内存的分配使用的统计分析指令，进而定位在各个断点处的当前内存使用情况。具体使用步骤如下：
######1、启动valgind
~~~
valgrind --vgdb-error=0 执行文件
说明：其中--vgdb-error=n表示在发生n个错误后调用gdbserver
~~~
######2、启动gdb
~~~
gdb 执行文件
然后在gdb终端中输入
target remote |　/usr/lib/vgdb --pid=41430
说明：/usr/lib/vgdb为vgdb路径; --pid=41430是上面启动的valgrind的进程号; target remote意思是Use a remote computer via a serial line, using a gdb-specific protocol. Specify the serial device it is connected to (e.g. /dev/ttyS0, /dev/ttya, COM1, etc.).
~~~
######3、在程序需要分析的地方设置断点
~~~
b 断点位置
~~~
######4、continue运行程序
######5、当程序运行到待分析的断点处可以采用如下指令进行简单的内存使用情况的检查
~~~
monitor leak_check full reachable any limited 100
monitor block_list num
说明：monitor指令的意思是向远程端口发送一个指令，其后的指令下面将详细说明
~~~
######6、vgdb指令说明
通过monitor向远程端口发送的指令可以通过在gdb中输入monitor help进行查看，查看内容如下
~~~
general valgrind monitor commands:
  help [debug]            : monitor command help. With debug: + debugging commands
  v.wait [<ms>]           : sleep <ms> (default 0) then continue
  v.info all_errors       : show all errors found so far
  v.info last_error       : show last error found
  v.info n_errs_found [msg] : show the nr of errors found so far and the given msg
  v.info open_fds         : show open file descriptors (only if --track-fds=yes)
  v.kill                  : kill the Valgrind process
  v.set gdb_output        : set valgrind output to gdb
  v.set log_output        : set valgrind output to log
  v.set mixed_output      : set valgrind output to log, interactive output to gdb
  v.set merge-recursive-frames <num> : merge recursive calls in max <num> frames
  v.set vgdb-error <errornr> : debug me at error >= <errornr> 

memcheck monitor commands:
  get_vbits <addr> [<len>]
        returns validity bits for <len> (or 1) bytes at <addr>; bit values 0 = valid, 1 = invalid, __ = unaddressable byte
        Example: get_vbits 0x8049c78 10
  make_memory [noaccess|undefined|defined|Definedifaddressable] <addr> [<len>]
        mark <len> (or 1) bytes at <addr> with the given accessibility
  check_memory [addressable|defined] <addr> [<len>]
        check that <len> (or 1) bytes at <addr> have the given accessibility and outputs a description of <addr>
  leak_check [full*|summary]
                [kinds kind1,kind2,...|reachable|possibleleak*|definiteleak]
                [heuristics heur1,heur2,...]
                [increased*|changed|any]
                [unlimited*|limited <max_loss_records_output>]
            * = defaults
       where kind is one of definite indirect possible reachable all none
       where heur is one of stdstring newarray multipleinheritance all none*
        Examples: leak_check
                  leak_check summary any
                  leak_check full kinds indirect,possible
                  leak_check full reachable any limited 100
  block_list <loss_record_nr>
        after a leak search, shows the list of blocks of <loss_record_nr>
  who_points_at <addr> [<len>]
        shows places pointing inside <len> (default 1) bytes at <addr>
        (with len 1, only shows "start pointers" pointing exactly to <addr>, with len > 1, will also show "interior pointers")
~~~

####reference
---
- [ptmalloc](http://blog.csdn.net/phenics/article/details/777053)
- [MALLINFO(3)](http://www.man7.org/linux/man-pages/man3/mallinfo.3.html) 
- [内存管理内幕](http://www.ibm.com/developerworks/cn/linux/l-memory/)
- [内存调试技巧](http://www.ibm.com/developerworks/cn/aix/library/au-memorytechniques.html)
- [glibc下的内存管理](http://www.cnblogs.com/lookof/archive/2013/03/26/2981768.html)
- [Glibc内存管理--ptmalloc2源代码分析](http://mqzhuang.iteye.com/blog/1014287)
- [Linux内存管理和性能学习笔记（一）：内存测量与堆内存](http://blog.chinaunix.net/uid-27167114-id-3559781.html)
- [细说linux IPC（五）：system V共享内存](http://blog.itpub.net/30027480/viewspace-1342864/) 
- [Linux进程间通信——使用共享内存](http://blog.csdn.net/ljianhui/article/details/10253345)
- [linux进程间的通信(C): 共享内存](http://blog.chinaunix.net/uid-26000296-id-3421346.html) 
- [system V 共享内存](http://www.cnblogs.com/jeakon/archive/2012/05/27/2816812.html)
- [Dumping contents of lost memory reported by Valgrind](http://stackoverflow.com/questions/12663283/dumping-contents-of-lost-memory-reported-by-valgrind)

