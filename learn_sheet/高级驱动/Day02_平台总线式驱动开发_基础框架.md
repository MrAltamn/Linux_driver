# 一、总线、设备、驱动

硬编码式的驱动开发带来的问题：

1.  垃圾代码太多

2.  结构不清晰

3.  一些统一设备功能难以支持

4.  开发效率低下

## 1.1 初期解决思路：设备和驱动分离

![SOC架构图](.\SOC架构图.jpg)

​	struct device来表示一个具体设备，主要提供具体设备相关的资源（如寄存器地址、GPIO、中断等等）

​	struct device\_driver来表示一个设备驱动，一个驱动可以支持多个操作逻辑相同的设备

​	带来的问题-------怎样将二者进行关联（匹配）？

​	硬件上同一总线上的设备遵循一致的时序通信，在其基础上增加管理设备和驱动的软件功能

​	于是引入总线（bus），各种总线的核心框架由内核来实现，通信时序一般由SOC供应商支持

​	内核中用struct bus\_type来表示一种总线，总线可以是实际存在的总线，也可以是虚拟总线：

1.  实际总线：提供时序通信方式 + 管理设备和驱动

2.  虚拟总线：仅用来管理设备和驱动（最核心的作用之一就是完成设备和驱动的匹配）

理解方式：

设备：提供硬件资源——男方

驱动：提供驱动代码——女方

总线：匹配设备和驱动——婚介所：提供沟通机制，完成拉郎配

## 1.2 升级思路：根据设备树，在系统启动时自动产生每个节点对应的设备

初期方案，各种device需要编码方式注册进内核中的设备管理结构中，为了进一步减少这样的编码，引进设备树

# 二、基本数据类型

2.1  struct device

```c
struct device 
{
	struct bus_type	*bus;	//总线类型
	dev_t			devt;	//设备号
	struct device_driver *driver;	//设备驱动
    struct device_node  *of_node;//设备树中的节点，重要
	void	(*release)(struct device *dev);//删除设备，重要
    //.......
}；
```

2.2 struct device\_driver

```c
struct device_driver 
{
	const char		*name;	//驱动名称，匹配device用，重要
	struct bus_type	*bus;    //总线类型
	struct module		*owner;	//模块THIS_MODULE 
	const struct of_device_id	*of_match_table;//用于设备树匹配 
                //of_match_ptr(某struct of_device_id对象地址) 重要
    //......
};
```

```c
struct of_device_id
{
	char name[32];//设备名
	char type[32];//设备类型
	char compatible[128]; //用于device和driver的match，重点
};
//用到结构体数组，一般不指定大小，初始化时最后加{}表示数组结束
```

# 三、platform总线驱动

platform是一种虚拟总线，主要用来管理那些不需要时序通信的设备

基本结构图：

![platform基本结构](.\platform基本结构.gif)

## 3.1 核心数据类型之platform\_device

```c
struct platform_device 
{
    const char    *name;    //匹配用的名字
    int        id;//设备id,用于在该总线上同名的设备进行编号，如果只有一个设备，则为-1
    struct device    dev;   //设备模块必须包含该结构体
    struct resource    *resource;//资源结构体 指向资源数组
        u32        num_resources;//资源的数量 资源数组的元素个数
    const struct platform_device_id    *id_entry;//设备八字
};
```

```c
struct platform_device_id
{
	char name[20];//匹配用名称
	kernel_ulong_t driver_data;//需要向驱动传输的其它数据
};
```

```c
struct resource 
{
	resource_size_t start;  //资源起始位置   
	resource_size_t end;   //资源结束位置
	const char *name;      
	unsigned long flags;   //区分资源是什么类型的
};
 
#define IORESOURCE_MEM        0x00000200
#define IORESOURCE_IRQ        0x00000400 
/*
flags 指资源类型，我们常用的是 IORESOURCE_MEM、IORESOURCE_IRQ  这两种。start 和 end 的含义会随着 flags而变更，如

a -- flags为IORESOURCE_MEM 时，start 、end 分别表示该platform_device占据的内存的开始地址和结束值；注意不同MEM的地址值不能重叠

b -- flags为 IORESOURCE_IRQ   时，start 、end 分别表示该platform_device使用的中断号的开始地址和结束值
*/
```

```c
/**
 *注册：把指定设备添加到内核中平台总线的设备列表，等待匹配,匹配成功则回调驱动中probe；
 */
int platform_device_register(struct platform_device *);
/**
 *注销：把指定设备从设备列表中删除，如果驱动已匹配则回调驱动方法和设备信息中的release；
 */
void platform_device_unregister(struct platform_device *);
```

```c
struct resource *platform_get_resource(struct platform_device *dev,unsigned int type, unsigned int num);
/*
	功能：获取设备资源
	参数：dev:平台设备
		  type:获取的资源类型
		  num:对应类型资源的序号（如第0个MEM、第2个IRQ等，不是数组下标）
	返回值：成功：资源结构体首地址,失败:NULL
*/
```

## 3.2 核心数据类型之platform\_driver

```c
struct platform_driver 
{
    int (*probe)(struct platform_device *);//设备和驱动匹配成功之后调用该函数
    int (*remove)(struct platform_device *);//设备卸载了调用该函数
    
    void (*shutdown)(struct platform_device *);
    int (*suspend)(struct platform_device *, pm_message_t state);
    int (*resume)(struct platform_device *);
    struct device_driver driver;//内核里所有的驱动必须包含该结构体
    const struct platform_device_id *id_table;  //能够支持的设备八字数组，用到结构体数组，一般不指定大小，初始化时
                                                        //最后加{}表示数组结束
};
```

```c
int platform_driver_register(struct platform_driver*pdrv);
/*
	功能：注册平台设备驱动
	参数：pdrv:平台设备驱动结构体
	返回值：成功：0
	失败：错误码
*/
void platform_driver_unregister(struct platform_driver*pdrv);
```

# 四、platform的三种匹配方式

![driver和device](.\driver和device.jpg)

2.1 名称匹配：一个驱动只对应一个设备 ----- 优先级最低

2.2 id匹配（可想象成八字匹配）：一个驱动可以对应多个设备 ------优先级次低

​     device模块中，platform\_device\_id的name成员必须与struct platform\_device中的name成员内容一致

​     因此device模块中，struct platform\_device中的name成员必须指定

​    driver模块中，struct platform\_driver成员driver的name成员必须指定，但与device模块中name可以不相同

2.3 设备树匹配：内核启动时根据设备树自动产生的设备 ------ 优先级最高

    使用compatible属性进行匹配，注意设备树中compatible属性值不要包含空白字符

​     id\_table可不设置，但struct platform\_driver成员driver的name成员必须设置

# 五、名称匹配之基础框架

```c
/*platform device框架*/
#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>

//定义资源数组

static void device_release(struct device *dev)
{
	printk("platform: device release\n");
}

struct platform_device test_device = {
	.id = -1,
	.name = "test_device",//必须初始化
	.dev.release = device_release, 
};

static int __init platform_device_init(void)
{
	platform_device_register(&test_device);
	return 0;
}

static void __exit platform_device_exit(void)
{
	platform_device_unregister(&test_device);
}

module_init(platform_device_init);
module_exit(platform_device_exit);
MODULE_LICENSE("Dual BSD/GPL");
```

```c
/*platform driver框架*/
#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>

static int driver_probe(struct platform_device *dev)
{
	printk("platform: match ok!\n");
	return 0;
}

static int driver_remove(struct platform_device *dev)
{
	printk("platform: driver remove\n");
	return 0;
}

struct platform_driver test_driver = {
	.probe = driver_probe,
	.remove = driver_remove,
	.driver = {
		.name = "test_device", //必须初始化
	},
};

static int __init platform_driver_init(void)
{
	platform_driver_register(&test_driver);
	return 0;
}

static void __exit platform_driver_exit(void)
{
	platform_driver_unregister(&test_driver);
}

module_init(platform_driver_init);
module_exit(platform_driver_exit);
MODULE_LICENSE("Dual BSD/GPL");

```

设备中增加资源，驱动中访问资源

# 六、名称匹配之led实例

