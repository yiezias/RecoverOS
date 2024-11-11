这个代码很多地方看起来可能已经不那么合适了，早晚会重构。新仓库recover-os。等我回来填坑。
重构版本：https://github.com/yiezias/recover-os

# 简介
本项目是基于《操作系统真象还原》实现的AMD64架构（或者叫x86\_64架构）版本的操作系统，主要思路与代码与原书一致。  
《操作系统真象还原》实现的操作系统运行在32位x86架构的处理器上，但是现在我们早已迈入64位时代。于是我~~复制~~参考《操作系统真象还原》中的代码实现了其64位版本。  

# 快速使用
## 步骤
* x64 Linux下安装较新版本的gcc，nasm，bochs，GNU make等程序。
* 克隆本仓库，进入源码目录。  
* 然后`$ make run`就可以编译并启动bochs模拟器了。  
本系统在bochs 2.7，gcc 12.2.0，nasm 2.15.05，make 4.3 环境下测试成功。部分软件版本不同应该也不会有太大影响。
运行效果如下图：

<img src="https://github.com/agrdrg/RecoverOS/blob/main/images/recover_shell.png" width = "600" alt="recover shell" align=center />


## 示例
本操作系统内置2048，贪吃蛇等小游戏。  
用recover写代码：  
（终端没有实现输入输出重定向，下面cat与Linux下的cat行为不大一样）  
（仅仅是展示一下写文本文件功能）

<img src="https://github.com/agrdrg/RecoverOS/blob/main/images/recover_coding.gif" width = "600" alt="recover coding" align=center />

2048小游戏：  

<img src="https://github.com/agrdrg/RecoverOS/blob/main/images/recover_2048.gif" width = "600" alt="recover 2048" align=center />

贪吃蛇（掉帧严重，就不放动图了）：  

<img src="https://github.com/agrdrg/RecoverOS/blob/main/images/recover_snake.png" width = "600" alt="recover snake" align=center />

# 关于《操作系统真象还原》
我读到过的相关书籍中最为通俗易懂的一本，详细又不失趣味性。即使显得有些啰嗦，作者也要变着法子让读者看懂（这么友好的书哪里找）。在众多操作系统相关的书中，我最终选择了这本~~唯一看得懂的~~书实现操作系统。

# 相对原书的不同
主要是AMD64与x86架构的不同
## 函数调用约定
AMD64架构下函数普遍采用寄存器传递参数，x86下的函数调用约定已无参考价值。不过其本身也好理解，具体细节写个C函数用gcc编译成汇编看看就明白了。开发过程中多处需要用到这一知识点。
## 保护模式与长模式
长模式可以理解为64位~~特色~~保护模式，处理器需要从实模式切换到保护模式再到长模式。
* 保护模式的分段机制十分繁琐，长模式中已经大大简化。
* 但长模式中分页机制更为复杂，本系统中仅使用了四级页表。不过作者在书中二级页表相关内容写的非常好，一通百通，明白了二级页表，四级页表也不那么困难。
* 二者中断异常部分基本相同。需要注意的是tss和ist机制，由于函数调用约定的改变，使用作者的代码在进程切换时会发生栈覆盖，通过ist机制可以实现每次进程切换都改变中断栈，从而解决这个问题。具体可以阅读相关代码。  

最方便的获取资料方式当然是搜索引擎，但搜索引擎往往找不到想要的内容，真正权威的资料还是AMD或intel文档。

## 快速系统调用
本系统完全放弃了中断实现系统调用的方式，采用syscall sysret快速系统调用指令实现。具体可参考AMD或intel文档，内容并不复杂。
## 文件系统
作者的文件系统感觉写得并不大好。一方面文件因为系统本来就比较繁琐，但作者动辄近几百行的函数，而且有些代码明明可以复用，却要重复实现，整体上非常混乱。本系统文件系统相当于作者的发生了较大改动，不过结合作者的书和代码应该不难理解。本系统文件系统并没有实现目录层级的功能，不过并不影响系统其他功能实现，于是就省略了。
## 应用层
作者的shell是在内核中实现的。本系统将shell作为应用程序，init进程用execv系统调用加载根目录shell。作者系统的许多内建命令在本系统中也是直接作为应用程序实现的。不过代码层面是相似的。还有特色小游戏，能够调用操作系统接口独立实现应用程序，这个系统就算大致完成了。
