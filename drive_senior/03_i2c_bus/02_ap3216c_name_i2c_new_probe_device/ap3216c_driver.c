#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/delay.h>

#include <asm/uaccess.h>

#include "ap3216c.h"

#define AP3216C_ADDR     0X1E /* AP3216C器件地址  */
/* AP3316C寄存器 */
#define AP3216C_SYSTEMCONG 	0x00 /* 配置寄存器       */
#define AP3216C_INTSTATUS 	0X01 /* 中断状态寄存器   */
#define AP3216C_INTCLEAR 	0X02 /* 中断清除寄存器   */
#define AP3216C_IRDATALOW 	0x0A /* IR数据低字节     红外 LED*/
#define AP3216C_IRDATAHIGH 	0x0B /* IR数据高字节     红外 LED*/
#define AP3216C_ALSDATALOW 	0x0C /* ALS数据低字节    光强传感器*/
#define AP3216C_ALSDATAHIGH 0X0D /* ALS数据高字节    光强传感器*/
#define AP3216C_PSDATALOW 	0X0E /* PS数据低字节     接近传感器*/
#define AP3216C_PSDATAHIGH 	0X0F /* PS数据高字节     接近传感器*/

int ap3216c_major = 11;
int ap3216c_minor = 0;

struct ap3216c_device {
	struct cdev ap3216c_dev;
	struct i2c_client *pclt;

	struct class *cls;
	struct device *led_dev;
};

struct ap3216c_device *pgap3216c;

static int ap3216c_read_byte(struct i2c_client *pclt, unsigned int reg)
{
	int ret = 0;
	char txbuf[1] = {reg};
	char rxbuf[1];
	
	struct i2c_msg msg[2] = {
		{pclt->addr, 0, 1, txbuf},
		{pclt->addr, I2C_M_RD, 1, rxbuf}
	};

	ret = i2c_transfer(pclt->adapter, msg, ARRAY_SIZE(msg));
	if(ret < 0)
	{
		printk("ret = %d\n", ret);
		return ret;
	}
	return rxbuf[0];
}

static int ap3216c_write_byte(struct i2c_client *pclt, unsigned int reg, unsigned int val)
{
	char txbuf[2] = {reg,val};
	
	struct i2c_msg msg[1] = {
		{pclt->addr, 0, 2, txbuf},
	};

	i2c_transfer(pclt->adapter, msg, ARRAY_SIZE(msg));
	return 0;
}


static int ap3216c_open(struct inode *inode, struct file *file) 
{
	return 0;
}

static int ap3216c_release(struct inode *inode, struct file *file) 
{
	return 0;
}

static long ap3216c_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;	
	union ap3216c_data data;
	struct i2c_client *client = pgap3216c->pclt;
	
	switch(cmd){
		case GET_ALS:
			data.als = ap3216c_read_byte(client,AP3216C_ALSDATALOW);
			data.als |= ap3216c_read_byte(client,AP3216C_ALSDATAHIGH)<<8;
			break;
		case GET_PS:
			data.ps = ap3216c_read_byte(client, AP3216C_PSDATALOW);
			data.ps |= ap3216c_read_byte(client, AP3216C_PSDATAHIGH) << 8;
			break;
		case GET_LED:
			data.led = ap3216c_read_byte(client, AP3216C_IRDATALOW);
			data.led |= ap3216c_read_byte(client, AP3216C_IRDATAHIGH) << 8;
			break;
		default:
			printk("invalid argument\n");
			return -EINVAL;
	}

	ret = copy_to_user((void *)arg,&data,sizeof(data));
	if(ret)
	{
		return -EFAULT;
	}
	
	return sizeof(data);
}


struct file_operations ap3216c_ops = {
	.owner = THIS_MODULE,
	.open = ap3216c_open,
	.release = ap3216c_release,
	.unlocked_ioctl = ap3216c_ioctl,
};

int ap3216c_init(struct i2c_client *pclt)
{
	ap3216c_write_byte(pclt, AP3216C_SYSTEMCONG, 0x04);	/* 复位 AP3216C*/
 	mdelay(50);             							/* AP3216C复位最少10ms  */
	ap3216c_write_byte(pclt, AP3216C_SYSTEMCONG, 0x03); /* 开启ALS、PS、IR*/
	return 0;
}

static int ap3216c_probe(struct i2c_client *pclt, const struct i2c_device_id *pids)
{
	int ret = 0;
	dev_t devno = MKDEV(ap3216c_major, ap3216c_minor);
	
	/*申请设备号*/
	ret = register_chrdev_region(devno, 1, "mpu6050");
	if(ret)
	{
		ret = alloc_chrdev_region(&devno,ap3216c_minor,1,"mpu6050");
		if(ret)
		{
			printk("get devno failed\n");
			return -1;
		}
		ap3216c_major = MAJOR(devno);//容易遗漏，注意
	}

	/*申请内存空间*/
	pgap3216c = (struct ap3216c_device *)kmalloc(sizeof(struct ap3216c_device), GFP_KERNEL);
	if(NULL == pgap3216c)
	{
		unregister_chrdev_region(devno,1);
		printk("kmalloc failed\n");
		return -1;
	}
	/*初始化内存空间*/
	memset(pgap3216c, 0, sizeof(struct ap3216c_device));

	/*注册字符设备ap3216c_dev 即给 struct cdev填充file operation 结构体*/
	cdev_init(&pgap3216c->ap3216c_dev, &ap3216c_ops);

	/*将struct cdev对象添加到内核对应的数据结构里*/
	pgap3216c->ap3216c_dev.owner = THIS_MODULE;
	cdev_add(&pgap3216c->ap3216c_dev, devno, 1);

	/*将驱动程序与设备client匹配*/
	pgap3216c->pclt = pclt;
	/*初始化ap3216c*/
	ap3216c_init(pclt);

	pgap3216c->cls = class_create(THIS_MODULE,"/ap3216c");
	pgap3216c->led_dev = device_create(pgap3216c->cls, NULL,devno, NULL, "ap3216c");	
	return 0;
}

static int ap3216c_remove(struct i2c_client *pclt)
{
	dev_t devno = MKDEV(ap3216c_major, ap3216c_minor);

	/*删除device_creat生成目录*/
	device_destroy(pgap3216c->cls, devno);
	/*删除class_creat生成目录*/
	class_destroy(pgap3216c->cls);
	/*注销字符设备*/
	cdev_del(&pgap3216c->ap3216c_dev);
	/*注销设备号*/
	unregister_chrdev_region(devno,1);
	/*释放内存空间*/
	kfree(pgap3216c);
	pgap3216c = NULL;
	return 0;
}

struct i2c_device_id ap3216c_ids[] = 
{
	[0] = {.name = "ap3216c"},
	[1] = {},
};

struct i2c_driver ap3216c_drv ={
	.driver ={
		.name = "100ask_ap3216c",
		.owner = THIS_MODULE,
	},
	.probe = ap3216c_probe,
	.remove = ap3216c_remove,
	.id_table = ap3216c_ids,
};

int __init ask100_ap3216c_init(void)
{
	i2c_add_driver(&ap3216c_drv);
	return 0;
}

void __exit ask100_ap3216c_exit(void)
{
	i2c_del_driver(&ap3216c_drv);
}

MODULE_LICENSE("GPL");

module_init(ask100_ap3216c_init);
module_exit(ask100_ap3216c_exit);




















































