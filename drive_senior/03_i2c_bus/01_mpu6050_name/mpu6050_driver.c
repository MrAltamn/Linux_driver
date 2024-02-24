#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/delay.h>

#include <asm/uaccess.h>

#include "mpu6050.h"

#define SMPLRT_DIV		0x19
#define CONFIG			0x1A
#define GYRO_CONFIG		0x1B
#define ACCEL_CONFIG	0x1C
#define ACCEL_XOUT_H	0x3B
#define ACCEL_XOUT_L	0x3C
#define ACCEL_YOUT_H	0x3D
#define ACCEL_YOUT_L	0x3E
#define ACCEL_ZOUT_H	0x3F
#define ACCEL_ZOUT_L	0x40
#define TEMP_OUT_H		0x41
#define TEMP_OUT_L		0x42
#define GYRO_XOUT_H		0x43
#define GYRO_XOUT_L		0x44
#define GYRO_YOUT_H		0x45
#define GYRO_YOUT_L		0x46
#define GYRO_ZOUT_H		0x47
#define GYRO_ZOUT_L		0x48
#define PWR_MGMT_1		0x6B

int mpu6050_major = 500;
int mpu6050_minor = 0;

struct mpu6050_device {
	struct cdev mpu6050_dev;
	struct i2c_client *pclt;

	struct class *cls;
	struct device *led_dev;
};

struct mpu6050_device *pgmpu6050;

static int mpu6050_read_byte(struct i2c_client *pclt, unsigned int reg)
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

static int mpu6050_write_byte(struct i2c_client *pclt, unsigned int reg, unsigned int val)
{
	char txbuf[2] = {reg,val};
	
	struct i2c_msg msg[1] = {
		{pclt->addr, 0, 2, txbuf},
	};

	i2c_transfer(pclt->adapter, msg, ARRAY_SIZE(msg));
	return 0;
}


static int mpu6050_open(struct inode *inode, struct file *file) 
{
	return 0;
}

static int mpu6050_release(struct inode *inode, struct file *file) 
{
	return 0;
}

static long mpu6050_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;	
	union mpu6050_data data;
	struct i2c_client *client = pgmpu6050->pclt;
	
	switch(cmd){
		case GET_ACCEL:
			data.accel.x = mpu6050_read_byte(client,ACCEL_XOUT_L);
			data.accel.x |= mpu6050_read_byte(client,ACCEL_XOUT_L)<<8;

			data.accel.y = mpu6050_read_byte(client, ACCEL_YOUT_L);
			data.accel.y |= mpu6050_read_byte(client, ACCEL_YOUT_H) << 8;

			data.accel.z = mpu6050_read_byte(client, ACCEL_ZOUT_L);
			data.accel.z |= mpu6050_read_byte(client, ACCEL_ZOUT_H) << 8;
			break;
		case GET_GYRO:
			data.gyro.x = mpu6050_read_byte(client, GYRO_XOUT_L);
			data.gyro.x |= mpu6050_read_byte(client, GYRO_XOUT_H) << 8;

			data.gyro.y = mpu6050_read_byte(client, GYRO_YOUT_L);
			data.gyro.y |= mpu6050_read_byte(client, GYRO_YOUT_H) << 8;

			data.gyro.z = mpu6050_read_byte(client, GYRO_ZOUT_L);
			data.gyro.z |= mpu6050_read_byte(client, GYRO_ZOUT_H) << 8;
			break;
		case GET_TEMP:
			data.temp = mpu6050_read_byte(client, TEMP_OUT_L);
			data.temp |= mpu6050_read_byte(client, TEMP_OUT_H) << 8;
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


struct file_operations mpu6050_ops = {
	.owner = THIS_MODULE,
	.open = mpu6050_open,
	.release = mpu6050_release,
	.unlocked_ioctl = mpu6050_ioctl,
};

int mpu6050_init(struct i2c_client *pclt)
{
	mpu6050_write_byte(pclt, PWR_MGMT_1, 0x00);
	mpu6050_write_byte(pclt, SMPLRT_DIV, 0x07);
	mpu6050_write_byte(pclt, CONFIG, 0x06);
	mpu6050_write_byte(pclt, GYRO_CONFIG, 0xF8);
	mpu6050_write_byte(pclt, ACCEL_CONFIG, 0x19);
	return 0;
}

static int mpu6050_probe(struct i2c_client *pclt, const struct i2c_device_id *pids)
{
	int ret = 0;
	dev_t devno = MKDEV(mpu6050_major, mpu6050_minor);
	
	/*申请设备号*/
	ret = register_chrdev_region(devno, 1, "mpu6050");
	if(ret)
	{
		ret = alloc_chrdev_region(&devno,mpu6050_minor,1,"mpu6050");
		if(ret)
		{
			printk("get devno failed\n");
			return -1;
		}
		mpu6050_major = MAJOR(devno);//容易遗漏，注意
	}

	/*申请内存空间*/
	pgmpu6050 = (struct mpu6050_device *)kmalloc(sizeof(struct mpu6050_device), GFP_KERNEL);
	if(NULL == pgmpu6050)
	{
		unregister_chrdev_region(devno,1);
		printk("kmalloc failed\n");
		return -1;
	}
	/*初始化内存空间*/
	memset(pgmpu6050, 0, sizeof(struct mpu6050_device));

	/*注册字符设备mpu6050_dev 即给 struct cdev填充file operation 结构体*/
	cdev_init(&pgmpu6050->mpu6050_dev, &mpu6050_ops);

	/*将struct cdev对象添加到内核对应的数据结构里*/
	pgmpu6050->mpu6050_dev.owner = THIS_MODULE;
	cdev_add(&pgmpu6050->mpu6050_dev, devno, 1);

	/*将驱动程序与设备client匹配*/
	pgmpu6050->pclt = pclt;
	/*初始化mpu6050*/
	mpu6050_init(pclt);

	pgmpu6050->cls = class_create(THIS_MODULE,"/mpu6050");
	pgmpu6050->led_dev = device_create(pgmpu6050->cls, NULL,devno, NULL, "mpu6050");	
	return 0;
}

static int mpu6050_remove(struct i2c_client *pclt)
{
	dev_t devno = MKDEV(mpu6050_major, mpu6050_minor);

	/*注销字符设备*/
	cdev_del(&pgmpu6050->mpu6050_dev);
	/*注销设备号*/
	unregister_chrdev_region(devno,1);
	/*释放内存空间*/
	kfree(pgmpu6050);
	pgmpu6050 = NULL;
	return 0;
}

struct i2c_device_id mpu6050_ids[] = 
{
	[0] = {.name = "MPU6050"},
	[1] = {},
};

struct i2c_driver mpu6050_drv ={
	.driver ={
		.name = "100ask_MPU6050",
		.owner = THIS_MODULE,
	},
	.probe = mpu6050_probe,
	.remove = mpu6050_remove,
	.id_table = mpu6050_ids,
};

int __init ask100_mpu6050_init(void)
{
	i2c_add_driver(&mpu6050_drv);
	return 0;
}

void __exit ask100_mpu6050_exit(void)
{
	i2c_del_driver(&mpu6050_drv);
}

MODULE_LICENSE("GPL");

module_init(ask100_mpu6050_init);
module_exit(ask100_mpu6050_exit);




















































