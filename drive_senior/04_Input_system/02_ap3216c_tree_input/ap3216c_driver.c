#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/input.h>



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

struct ap3216c_device {
	struct i2c_client *pclt;
	struct input_dev *pdev;
	struct delayed_work work;
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

void ap3216c_work_func(struct work_struct *pwork)
{
	struct ap3216c_device *ap3216c_pdev = (struct ap3216c_device *)(container_of((struct delayed_work *)pwork,struct ap3216c_device,work));
	unsigned short als = 0;
	unsigned short ps = 0;
	unsigned short led = 0;

	als = ap3216c_read_byte(ap3216c_pdev->pclt,AP3216C_ALSDATALOW);
	als |= ap3216c_read_byte(ap3216c_pdev->pclt,AP3216C_ALSDATAHIGH)<<8;
	input_event(ap3216c_pdev->pdev, EV_LED, LED_MUTE, als);
	
	ps = ap3216c_read_byte(ap3216c_pdev->pclt, AP3216C_PSDATALOW);
	ps |= ap3216c_read_byte(ap3216c_pdev->pclt, AP3216C_PSDATAHIGH) << 8;
	input_event(ap3216c_pdev->pdev, EV_LED, LED_MAIL, ps);

	led = ap3216c_read_byte(ap3216c_pdev->pclt, AP3216C_IRDATALOW);
	led |= ap3216c_read_byte(ap3216c_pdev->pclt, AP3216C_IRDATAHIGH) << 8;
	input_event(ap3216c_pdev->pdev, EV_LED, LED_MISC, led);

	input_sync(ap3216c_pdev->pdev);
	schedule_delayed_work(&ap3216c_pdev->work,msecs_to_jiffies(1000));
}


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

	/*申请内存空间*/
	pgap3216c = (struct ap3216c_device *)kmalloc(sizeof(struct ap3216c_device), GFP_KERNEL);
	if(NULL == pgap3216c)
	{
		printk("kmalloc failed\n");
		return -1;
	}
	/*初始化内存空间*/
	memset(pgap3216c, 0, sizeof(struct ap3216c_device));

	/*将驱动程序与设备client匹配*/
	pgap3216c->pclt = pclt;
	/*初始化ap3216c*/
	ap3216c_init(pclt);

	pgap3216c->pdev = input_allocate_device(); 	//创建对象
	set_bit(EV_LED, pgap3216c->pdev->evbit);	//设置事件类型
	set_bit(LED_MUTE, pgap3216c->pdev->ledbit);	//设置指定对象
	
	set_bit(LED_MISC, pgap3216c->pdev->ledbit);
	set_bit(LED_MAIL, pgap3216c->pdev->ledbit);

	ret= input_register_device(pgap3216c->pdev);

	INIT_DELAYED_WORK(&pgap3216c->work, ap3216c_work_func);
	schedule_delayed_work(&pgap3216c->work,msecs_to_jiffies(1000));
	return 0;
}

static int ap3216c_remove(struct i2c_client *pclt)
{
	cancel_delayed_work(&pgap3216c->work);
	input_unregister_device(pgap3216c->pdev);
	input_free_device(pgap3216c->pdev);

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


struct of_device_id ap3216c_dt_match[] = {
	[0] = {.compatible = "ap3216c"},
	[1] = {},
};


struct i2c_driver ap3216c_drv ={
	.driver ={
		.name = "100ask_ap3216c",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ap3216c_dt_match),
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




















































