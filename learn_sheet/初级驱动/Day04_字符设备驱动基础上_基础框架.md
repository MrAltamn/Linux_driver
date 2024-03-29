# 一、Linux内核对设备的分类

linux的文件种类：

1. -：普通文件
2. d：目录文件
3. p：管道文件
4. s：本地socket文件
5. l：链接文件
6. c：字符设备
7. b：块设备

Linux内核按驱动程序实现模型框架的不同，将设备分为三类：

1. 字符设备：按字节流形式进行数据读写的设备，一般情况下按顺序访问，数据量不大，一般不设缓存
2. 块设备：按整块进行数据读写的设备，最小的块大小为512字节（一个扇区），块的大小必须是扇区的整数倍，Linux系统的块大小一般为4096字节，随机访问，设缓存以提高效率
3. 网络设备：针对网络数据收发的设备

总体框架图：

![Linux系统驱动总体框图](.\Linux系统驱动总体框图.png)



# 二、设备号------内核中同类设备的区分

内核用设备号来区分同类里不同的设备，设备号是一个无符号32位整数，数据类型为dev_t，设备号分为两部分：

1. 主设备号：占高12位，用来表示驱动程序相同的一类设备
2. 次设备号：占低20位，用来表示被操作的哪个具体设备

应用程序打开一个设备文件时，通过设备号来查找定位内核中管理的设备。

MKDEV宏用来将主设备号和次设备号组合成32位完整的设备号，用法：

```
dev_t devno;
int major = 251;//主设备号
int minor = 2;//次设备号
devno = MKDEV(major,minor);
```

MAJOR宏用来从32位设备号中分离出主设备号，用法：

```
dev_t devno = MKDEV(249,1);
int major = MAJOR(devno);
```

MINOR宏用来从32位设备号中分离出次设备号，用法：

```
dev_t devno = MKDEV(249,1);
int minor = MINOR(devno);
```

如果已知一个设备的主次设备号，应用层指定好设备文件名，那么可以用mknod命令在/dev目录创建代表这个设备的文件，即此后应用程序对此文件的操作就是对其代表的设备操作，mknod用法如下：

```
@ cd /dev
@ mknod 设备文件名 设备种类(c为字符设备,b为块设备)  主设备号  次设备号    //ubuntu下需加sudo执行
```

在应用程序中如果要创建设备可以调用系统调用函数mknod，其原型如下：

```
int mknod(const char *pathname,mode_t mode,dev_t dev);
pathname:带路径的设备文件名，无路径默认为当前目录，一般都创建在/dev下
mode：文件权限 位或 S_IFCHR/S_IFBLK
dev:32位设备号
返回值：成功为0，失败-1
```



# 三、申请和注销设备号

字符驱动开发的第一步是通过模块的入口函数向内核添加本设备驱动的代码框架，主要完成：

1. 申请设备号
2. 定义、初始化、向内核添加代表本设备的结构体元素

```
int register_chrdev_region(dev_t from, unsigned count, const char *name)
功能：手动分配设备号，先验证设备号是否被占用，如果没有则申请占用该设备号
参数：
	from：自己指定的设备号
	count：申请的设备数量
	name：/proc/devices文件中与该设备对应的名字，方便用户层查询主设备号
返回值：
	成功为0，失败负数，绝对值为错误码
```

```
int alloc_chrdev_region(dev_t *dev,unsigned baseminor,unsigned count, const char *name)
功能：动态分配设备号，查询内核里未被占用的设备号，如果找到则占用该设备号
参数：
	dev：分配设备号成功后用来存放分配到的设备号
	baseminior：起始的次设备号，一般为0
	count：申请的设备数量
	name：/proc/devices文件中与该设备对应的名字，方便用户层查询主次设备号
返回值：
	成功为0，失败负数，绝对值为错误码
```

分配成功后在/proc/devices 可以查看到申请到主设备号和对应的设备名，mknod时参数可以参考查到的此设备信息

```
void unregister_chrdev_region(dev_t from, unsigned count)
功能：释放设备号
参数：
	from：已成功分配的设备号将被释放
	count：申请成功的设备数量
```

释放后/proc/devices文件对应的记录消失

# 四、函数指针复习

内存的作用-----用来存放程序运行过程中的

1. 数据
2. 指令

## 4.1、 内存四区

堆区

栈区

数据区

代码区

## 4.2、C语言中内存数据的访问方式

直接访问：通过所在空间名称去访问

间接访问：通过所在空间首地址去访问      \*地址值  此时的\*为间接访问运算符

## 4.3、C语言中函数调用方式：

直接调用：通过函数名去调用函数

间接调用：通过函数在代码区所对应的那份空间的首地址去调用



```c
int func(int a,int b)
{
    //......
}

int (int a,int b)  * pf;//语法错误
int *pf(int a,int b);//函数声明语句
int (*pf)(int a,int b);//定义一个函数指针
pf = &func;//&运算符后面如果是函数名的话可以省略不写
pf = func;

y = func(3,4);//直接调用
y = (*pf)(3,4);//间接调用，*运算符后面如果是函数指针类型则可以省略不写
y = pf(3,4);//间接调用

typedef int myint;
typedef int (*)(int,int)  pft;//语法错误
typedef int (*pft)(int,int) ;
pft pt;
```

## 4.4 适用场合

前提：当有很多个同类函数待被调用时

A处：知道所有函数名，由此处来决定B处将会调用哪个函数

B处：负责调用A处指定的函数

思考：A处如何告诉B处被调用的是哪个函数呢，无非两个办法：

1. 告诉B处函数名，怎么做呢？传字符串----“函数名”？ C语言没有对应语法支持
2. 告诉B处对应函数在代码区的地址

# 五、注册字符设备

```
struct cdev
{
	struct kobject kobj;//表示该类型实体是一种内核对象
	struct module *owner;//填THIS_MODULE，表示该字符设备从属于哪个内核模块
	const struct file_operations *ops;//指向空间存放着针对该设备的各种操作函数地址
	struct list_head list;//链表指针域
	dev_t dev;//设备号
	unsigned int count;//设备数量
};
```

自己定义的结构体中必须有一个成员为 struct cdev cdev，两种方法定义一个设备：

1.  直接定义：定义结构体全局变量

2.  动态申请：

   `struct  cdev * cdev_alloc()`

void cdev_init(struct cdev *cdev,const struct file_operations *fops)

```c
struct file_operations 
{
   struct module *owner;           //填THIS_MODULE，表示该结构体对象从属于哪个内核模块
   int (*open) (struct inode *, struct file *);	//打开设备
   int (*release) (struct inode *, struct file *);	//关闭设备
   ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);	//读设备
   ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);    //写设备
   loff_t (*llseek) (struct file *, loff_t, int);		//定位
   long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);//读写设备参数，读设备状态、控制设备
   unsigned int (*poll) (struct file *, struct poll_table_struct *);	//POLL机制，实现多路复用的支持
   int (*mmap) (struct file *, struct vm_area_struct *); //映射内核空间到用户层
   int (*fasync) (int, struct file *, int); //信号驱动
   //......
};
```

该对象各个函数指针成员都对应相应的系统调用函数，应用层通过调用系统函数来间接调用这些函数指针成员指向的设备驱动函数：

![syscall与drvfunc](.\syscall与drvfunc.jpg)

一般定义一个struct file_operations类型的全局变量并用自己实现各种操作函数名对其进行初始化

```
int cdev_add(struct cdev *p,dev_t dev,unsigned int count)
功能：将指定字符设备添加到内核
参数：
	p：指向被添加的设备
	dev：设备号
	count：设备数量，一般填1
```

```
void cdev_del(struct cdev *p)
功能：从内核中移除一个字符设备
参数：
	p：指向被移除的字符设备
```

小结：

字符设备驱动开发步骤：

1.  如果设备有自己的一些控制数据，则定义一个包含struct cdev cdev成员的结构体struct mydev，其它成员根据设备需求，设备简单则直接用struct cdev
2. 定义一个struct mydev或struct cdev的全局变量来表示本设备；也可以定义一个struct mydev或struct cdev的全局指针（记得在init时动态分配）
3. 定义三个全局变量分别来表示主设备号、次设备号、设备数
4. 定义一个struct file_operations结构体变量，其owner成员置成THIS_MODULE
5. module init函数流程：a. 申请设备号 b. 如果是全局设备指针则动态分配代表本设备的结构体元素 c. 初始化struct cdev成员 d. 设置struct cdev的owner成员为THIS_MODULE  e. 添加字符设备到内核
6. module exit函数：a. 注销设备号 b. 从内核中移除struct cdev  c. 如果如果是全局设备指针则释放其指向空间
7. 编写各个操作函数并将函数名初始化给struct file_operations结构体变量



验证操作步骤：

1. 编写驱动代码mychar.c
2. make生成ko文件
3. insmod内核模块
4. 查阅字符设备用到的设备号（主设备号）：cat  /proc/devices  |  grep  申请设备号时用的名字
5. 创建设备文件（设备节点） ： mknod   /dev/???   c   上一步查询到的主设备号    代码中指定初始次设备号
6. 编写app验证驱动（testmychar_app.c）
7. 编译运行app，dmesg命令查看内核打印信息

# 六、字符设备驱动框架解析

设备的操作函数如果比喻是桩的话（性质类似于设备操作函数的函数，在一些场合被称为桩函数），则：

驱动实现设备操作函数 ----------- 做桩

insmod调用的init函数主要作用 --------- 钉桩

rmmod调用的exitt函数主要作用 --------- 拔桩

应用层通过系统调用函数间接调用这些设备操作函数 ------- 用桩

## 6.1 两个操作函数中常用的结构体说明
```c
内核中记录文件元信息的结构体
struct inode
{
	//....
	dev_t  i_rdev;//设备号
	struct cdev  *i_cdev;//如果是字符设备才有此成员，指向对应设备驱动程序中的加入系统的struct cdev对象
	//....
}
/*
	1. 内核中每个该结构体对象对应着一个实际文件,一对一
	2. open一个文件时如果内核中该文件对应的inode对象已存在则不再创建，不存在才创建
	3. 内核中用此类型对象关联到对此文件的操作函数集（对设备而言就是关联到具体驱动代码）
*/
```

```c
读写文件内容过程中用到的一些控制性数据组合而成的对象------文件操作引擎（文件操控器）
struct file
{
	//...
	mode_t f_mode;//不同用户的操作权限，驱动一般不用
	loff_t f_pos;//position 数据位置指示器，需要控制数据开始读写位置的设备有用
	unsigned int f_flags;//open时的第二个参数flags存放在此，驱动中常用
	struct file_operations *f_op;//open时从struct inode中i_cdev的对应成员获得地址，驱动开发中用来协助理解工作原理，内核中使用
	void *private_data;//本次打开文件的私有数据，驱动中常来在几个操作函数间传递共用数据
	struct dentry *f_dentry;//驱动中一般不用，除非需要访问对应文件的inode，用法flip->f_dentry->d_inode
    int refcnt;//引用计数，保存着该对象地址的位置个数，close时发现refcnt为0才会销毁该struct file对象
	//...
};
/*
	1. open函数被调用成功一次，则创建一个该对象，因此可以认为一个该类型的对象对应一次指定文件的操作
	2. open同一个文件多次，每次open都会创建一个该类型的对象
	3. 文件描述符数组中存放的地址指向该类型的对象
	4. 每个文件描述符都对应一个struct file对象的地址
*/
```

## 6.2 字符设备驱动程序框架分析

驱动实现端：

![驱动实现端](.\驱动实现端.jpg)

驱动使用端：

![驱动使用端](.\驱动使用端.jpg)



syscall_open函数实现的伪代码：

```c
int syscall_open(const char *filename,int flag)
{
    dev_t devno;
    struct inode *pnode = NULL;
    struct cdev *pcdev = NULL;
    struct file *pfile = NULL;
    int fd = -1;
    
    /*根据filename在内核中查找该文件对应的struct inode对象地址
        找到则pnode指向该对象
        未找到则创建新的struct inode对象，pnode指向该对象，并从文件系统中读取文件的元信息到该对象*/
    if(/*未找到对应的struct inode对象*/)
    {/*根据文件种类决定如何进行下面的操作，如果是字符设备则执行如下操作*/
    
    	/*从pnode指向对象中得到设备号*/
	    devno = pnode->i_rdev;
    
    	/*用devno在字符设备链表查找对应节点，并将该节点的地址赋值给pcdev*/
    
    	/*pcdev赋值给pnode的i_cdev成员*/
    	pnode->i_cdev = pcdev;
    }
    
    /*创建struct file对象，并将该对象的地址赋值给pfile*/
    
    pfile->f_op = pnode->i_cdev->ops;
    pfile->f_flags = flag;
    
    /*调用驱动程序的open函数*/
    pfile->f_op->open(pnode,pfile,flag);
    
    /*将struct file对象地址填入进程的描述符数组，得到对应位置的下标赋值给fd*/
    
    return fd;
}
```

syscall_read函数实现的伪代码

```c
int syscall_read(int fd,void *pbuf,int size)
{
    struct file *pfile = NULL;
    struct file_operations *fops = NULL;
    int cnt；
    
    /*将fd作为下标，在进程的描述符数组中获得struct file对象的地址赋值给pfile*/
    
    /*从struct file对象的f_op成员中得到操作函数集对象地址赋值给fops*/
    
    /*从操作函数集对象的read成员得到该设备对应的驱动程序中read函数，并调用之*/
    cnt = fops->read(pfile,pbuf,size,&pfile->f_pos);
    
    。。。。
    return cnt;
}
```

## 6.3 参考原理图

![字符设备驱动框架](.\字符设备驱动框架.jpg)

![Linux字符设备驱动工作原理图](.\Linux字符设备驱动工作原理图.png)

## 6.4 常用操作函数说明

```c
int (*open) (struct inode *, struct file *);	//打开设备
/*
	指向函数一般用来对设备进行硬件上的初始化，对于一些简单的设备该函数只需要return 0，对应open系统调用，是open系统调用函数实现过程中调用的函数,
*/

int (*release) (struct inode *, struct file *);	//关闭设备
/*
	,指向函数一般用来对设备进行硬件上的关闭操作，对于一些简单的设备该函数只需要return 0，对应close系统调用，是close系统调用函数实现过程中调用的函数
*/

ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);	//读设备
/*
	指向函数用来将设备产生的数据读到用户空间，对应read系统调用，是read系统调用函数实现过程中调用的函数
*/

ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);    //写设备
/*
	指向函数用来将用户空间的数据写进设备，对应write系统调用，是write系统调用函数实现过程中调用的函数
*/

loff_t (*llseek) (struct file *, loff_t, int);		//数据操作位置的定位
/*
	指向函数用来获取或设置设备数据的开始操作位置（位置指示器），对应lseek系统调用，是lseek系统调用函数实现过程中调用的函数
*/


long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);//读写设备参数，读设备状态、控制设备
/*
	指向函数用来获取、设置设备一些属性或设备的工作方式等非数据读写操作，对应ioctl系统调用，是ioctl系统调用函数实现过程中调用的函数
*/

unsigned int (*poll) (struct file *, struct poll_table_struct *);//POLL机制，实现对设备的多路复用方式的访问
/*
	指向函数用来协助多路复用机制完成对本设备可读、可写数据的监控，对应select、poll、epoll_wait系统调用，是select、poll、epoll_wait系统调用函数实现过程中调用的函数
*/
  
int (*fasync) (int, struct file *, int); //信号驱动
/*
	指向函数用来创建信号驱动机制的引擎，对应fcntl系统调用的FASYNC标记设置，是fcntl系统调用函数FASYNC标记设置过程中调用的函数
*/
```



# 七、读操作实现

```c
ssize_t xxx_read(struct file *filp, char __user *pbuf, size_t count, loff_t *ppos);
完成功能：读取设备产生的数据
参数：
    filp：指向open产生的struct file类型的对象，表示本次read对应的那次open
    pbuf：指向用户空间一块内存，用来保存读到的数据
    count：用户期望读取的字节数
    ppos：对于需要位置指示器控制的设备操作有用，用来指示读取的起始位置，读完后也需要变更位置指示器的指示位置
 返回值：
    本次成功读取的字节数，失败返回-1
```



put_user(x,ptr)

x：char、int类型的简单变量名

unsigned long copy_to_user (void __user * to, const void * from, unsigned long n)

成功为返回0，失败非0

# 八、写操作实现

```c
ssize_t xxx_write (struct file *filp, const char __user *pbuf, size_t count, loff_t *ppos);  
完成功能：向设备写入数据
参数：
    filp：指向open产生的struct file类型的对象，表示本次write对应的那次open
    pbuf：指向用户空间一块内存，用来保存被写的数据
    count：用户期望写入的字节数
    ppos：对于需要位置指示器控制的设备操作有用，用来指示写入的起始位置，写完后也需要变更位置指示器的指示位置
 返回值：
    本次成功写入的字节数，失败返回-1

```

get_user(x,ptr)

x：char、int类型的简单变量名

unsigned long copy_from_user (void * to, const void __user * from, unsigned long n)

成功为返回0，失败非0

# 九、ioctl操作实现

已知成员的地址获得所在结构体变量的地址：container_of(成员地址,结构体类型名，成员在结构体中的名称)



```
long xxx_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);
功能：对相应设备做指定的控制操作（各种属性的设置获取等等）
参数：
	filp：指向open产生的struct file类型的对象，表示本次ioctl对应的那次open
	cmd：用来表示做的是哪一个操作
    arg：和cmd配合用的参数
返回值：成功为0，失败-1
```



cmd组成

![ioctl2](.\ioctl2.png)

1. dir（direction），ioctl 命令访问模式（属性数据传输方向），占据 2 bit，可以为 _IOC_NONE、_IOC_READ、_IOC_WRITE、_IOC_READ | _IOC_WRITE，分别指示了四种访问模式：无数据、读数据、写数据、读写数据；
2. type（device type），设备类型，占据 8 bit，在一些文献中翻译为 “幻数” 或者 “魔数”，可以为任意 char 型字符，例如 
   ‘a’、’b’、’c’ 等等，其主要作用是使 ioctl 命令有唯一的设备标识；
3. nr（number），命令编号/序数，占据 8 bit，可以为任意 unsigned char 型数据，取值范围 0~255，如果定义了多个 ioctl 命令，通常从 0 开始编号递增；
4. size，涉及到 ioctl 函数 第三个参数 arg ，占据 13bit 或者 14bit（体系相关，arm 架构一般为 14 位），指定了 arg 的数据类型及长度，如果在驱动的 ioctl 实现中不检查，通常可以忽略该参数；

```c
#define _IOC(dir,type,nr,size) (((dir)<<_IOC_DIRSHIFT)| \
                               ((type)<<_IOC_TYPESHIFT)| \
                               ((nr)<<_IOC_NRSHIFT)| \
                               ((size)<<_IOC_SIZESHIFT))
/* used to create numbers */

// 定义不带参数的 ioctl 命令
#define _IO(type,nr)   _IOC(_IOC_NONE,(type),(nr),0)

//定义带读参数的ioctl命令（copy_to_user） size为类型名
#define _IOR(type,nr,size)  _IOC(_IOC_READ,(type),(nr),(_IOC_TYPECHECK(size)))

//定义带写参数的 ioctl 命令（copy_from_user） size为类型名
#define _IOW(type,nr,size)  _IOC(_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))

//定义带读写参数的 ioctl 命令 size为类型名
#define _IOWR(type,nr,size) _IOC(_IOC_READ|_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))

/* used to decode ioctl numbers */
#define _IOC_DIR(nr)        (((nr) >> _IOC_DIRSHIFT) & _IOC_DIRMASK)
#define _IOC_TYPE(nr)       (((nr) >> _IOC_TYPESHIFT) & _IOC_TYPEMASK)
#define _IOC_NR(nr)     (((nr) >> _IOC_NRSHIFT) & _IOC_NRMASK)
#define _IOC_SIZE(nr)      (((nr) >> _IOC_SIZESHIFT) & _IOC_SIZEMASK)
```

# 十、printk

```c
//日志级别
#define	KERN_EMERG	"<0>"	/* system is unusable			*/
#define	KERN_ALERT	"<1>"	/* action must be taken immediately	*/
#define	KERN_CRIT	"<2>"	/* critical conditions			*/
#define	KERN_ERR	"<3>"	/* error conditions			*/

#define	KERN_WARNING	"<4>"	/* warning conditions			*/

#define	KERN_NOTICE	"<5>"	/* normal but significant condition	*/
#define	KERN_INFO	"<6>"	/* informational			*/
#define	KERN_DEBUG	"<7>"	/* debug-level messages			*/

用法：printk(KERN_INFO"....",....)
    
    printk(KERN_INFO"Hello World"); =====> printk("<6>""Hello World") ====> printk("<6>Hello World")
  
```

dmesg --level=emerg,alert,crit,err,warn,notice,info,debug

```c
#define HELLO_DEBUG
#undef PDEBUG
#ifdef HELLO_DEBUG
#define PDEBUG(fmt, args...) printk(KERN_DEBUG fmt, ##args)
#else
#define PDEBUG(fmt, args...)
#endif

```



# 十一、多个次设备的支持

每一个具体设备（次设备不一样的设备），必须有一个struct cdev来代表它

cdev_init

cdev.owner赋值

cdev_add

以上三个操作对每个具体设备都要进行
