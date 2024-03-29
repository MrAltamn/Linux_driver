# 一、上下文和并发场合

执行流：有开始有结束总体顺序执行的一段代码  又称上下文

应用编程：任务上下文
内核编程：

1. 任务上下文：五状态 可阻塞
	a. 应用进程或线程运行在用户空间
	b. 应用进程或线程运行在内核空间（通过调用syscall来间接使用内核空间）
	c. 内核线程始终在内核空间
2. 异常上下文：不可阻塞
	中断上下文

竞态：多任务并行执行时，如果在一个时刻同时操作同一个资源，会引起资源的错乱，这种错乱情形被称为竞态

共享资源：可能会被多个任务同时使用的资源

临界区：操作共享资源的代码段

为了解决竞态，需要提供一种控制机制，来避免在同一时刻使用共享资源，这种机制被称为并发控制机制

并发控制机制分类：

1. 原子操作类
2. 忙等待类
3. 阻塞类



通用并发控制机制的一般使用套路：

```c
/*互斥问题：*/
并发控制机制初始化为可用
P操作

临界区

V操作

/*同步问题：*/
//并发控制机制初始化为不可用
//先行方：
。。。。。
V操作
    
//后行方：
P操作
。。。。。
```



# 二、中断屏蔽（了解）

一种同步机制的辅助手段

禁止本cpu中断			    使能本cpu中断
local_irq_disable();	    local_irq_enable();			
local_irq_save(flags);		local_irq_restore(flags);	与cpu的中断位相关
local_bh_disable();			local_bh_enable();			与中断低半部有关，关闭、打开软中断

禁止中断
临界区    //临界区代码不能占用太长时间，需要很快完成
打开中断



适用场合：中断上下文与某任务共享资源时，或多个不同优先级的中断上下文间共享资源时

# 三、原子变量（掌握）

原子变量：存取不可被打断的特殊整型变量
a.设置原子量的值
void atomic_set(atomic_t *v,int i);	//设置原子量的值为i
atomic_t v = ATOMIC_INIT(0);	//定义原子变量v并初始化为0

v = 10;//错误

b.获取原子量的值
atomic_read(atomic_t *v); 		//返回原子量的值

c.原子变量加减
void atomic_add(int i,atomic_t *v);//原子变量增加i
void atomic_sub(int i,atomic_t *v);//原子变量减少i

d.原子变量自增自减
void atomic_inc(atomic_t *v);//原子变量增加1
void atomic_dec(atomic_t *v);//原子变量减少1

e.操作并测试：运算后结果为0则返回真，否则返回假
int atomic_inc_and_test(atomic_t *v);
int atomic_dec_and_test(atomic_t *v);
int atomic_sub_and_test(int i,atomic_t *v);

原子位操作方法：
a.设置位
void set_bit(nr, void *addr);		//设置addr的第nr位为1
b.清除位
void clear_bit(nr , void *addr);	//清除addr的第nr位为0
c.改变位
void change_bit(nr , void *addr);	//改变addr的第nr位为1
d.测试位
void test_bit(nr , void *addr);		//测试addr的第nr位是否为1

适用场合：共享资源为单个整型变量的互斥场合

# 四、自旋锁：基于忙等待的并发控制机制

a.定义自旋锁
spinlock_t  lock;

b.初始化自旋锁
spin_lock_init(spinlock_t *);

c.获得自旋锁
spin_lock(spinlock_t *);	//成功获得自旋锁立即返回，否则自旋在那里直到该自旋锁的保持者释放

spin_trylock(spinlock_t *);	//成功获得自旋锁立即返回真，否则返回假，而不是像上一个那样"在原地打转”

d.释放自旋锁
spin_unlock(spinlock_t *);



```
#include <linux/spinlock.h>
定义spinlock_t类型的变量lock
spin_lock_init(&lock)后才能正常使用spinlock


spin_lock(&lock);
临界区
spin_unlock(&lock);
```



适用场合：

1. 异常上下文之间或异常上下文与任务上下文之间共享资源时
2. 任务上下文之间且临界区执行时间很短时
3. 互斥问题

# 五、信号量：基于阻塞的并发控制机制

a.定义信号量
struct semaphore sem;

b.初始化信号量
void sema_init(struct semaphore *sem, int val);

c.获得信号量P
int down(struct semaphore *sem);//深度睡眠

int down_interruptible(struct semaphore *sem);//浅度睡眠

d.释放信号量V
void up(struct semaphore *sem);



```
#include <linux/semaphore.h>
```

适用场合：任务上下文之间且临界区执行时间较长时的互斥或同步问题

# 六、互斥锁：基于阻塞的互斥机制

a.初始化
struct mutex  my_mutex;
mutex_init(&my_mutex);

b.获取互斥体
void  mutex_lock(struct mutex *lock);

c.释放互斥体
void mutex_unlock(struct mutex *lock);

1. 定义对应类型的变量
2. 初始化对应变量

P/加锁
临界区
V/解锁

```
#include <linux/mutex.h>
```

适用场合：任务上下文之间且临界区执行时间较长时的互斥问题

# 七、选择并发控制机制的原则

1. 不允许睡眠的上下文需要采用忙等待类，可以睡眠的上下文可以采用阻塞类。在异常上下文中访问的竞争资源一定采用忙等待类。
2. 临界区操作较长的应用建议采用阻塞类，临界区很短的操作建议采用忙等待类。
3. 中断屏蔽仅在有与中断上下文共享资源时使用。
4. 共享资源仅是一个简单整型量时用原子变量