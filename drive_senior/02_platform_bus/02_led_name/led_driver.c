#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include "leddrv.h"


int major = 11;
int minor = 0;
int myled_num  = 1;

struct myled_dev
{
	struct cdev mydev;

	volatile unsigned int *CCM_CCGR1                              ;
	volatile unsigned int *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3;
	volatile unsigned int *GPIO5_GDIR                             ;
	volatile unsigned int *GPIO5_DR                               ;

	struct class *cls;
	struct device *led_dev;
};

struct myled_dev *pgmydev = NULL;


int myled_open(struct inode *pnode,struct file *pfile)
{
	pfile->private_data =(void *) (container_of(pnode->i_cdev,struct myled_dev,mydev));
	
	return 0;
}

int myled_close(struct inode *pnode,struct file *pfile)
{
	return 0;
}

void led_on(struct myled_dev *pmydev,int ledno)
{
	switch(ledno)
	{
		case 5:
			writel(readl(pmydev->GPIO5_DR) & (~(1<<3)),pmydev->GPIO5_DR);
			break;
	}
}

void led_off(struct myled_dev *pmydev,int ledno)
{
	switch(ledno)
	{
		case 5:
			writel(readl(pmydev->GPIO5_DR) | (1 << 3),pmydev->GPIO5_DR);
			break;
	}
}


long myled_ioctl(struct file *pfile,unsigned int cmd,unsigned long arg)
{
	struct myled_dev *pmydev = (struct myled_dev *)pfile->private_data;

	if(arg < 2 || arg > 5)
	{
		return -1;
	}
	switch(cmd)
	{
		case MY_LED_ON:
			led_on(pmydev,arg);
			break;
		case MY_LED_OFF:
			led_off(pmydev,arg);
			break;
		default:
			return -1;
	}

	return 0;
}

struct file_operations myops = {
	.owner = THIS_MODULE,
	.open = myled_open,
	.release = myled_close,
	.unlocked_ioctl = myled_ioctl,
};

void ioremap_ledreg(struct myled_dev *pmydev, struct platform_device *p_pltdev)
{
	struct resource *pres = NULL;
	
	pres = platform_get_resource(p_pltdev,IORESOURCE_MEM,0);
	pmydev->CCM_CCGR1                               = ioremap(pres->start, 4);

	pres = platform_get_resource(p_pltdev,IORESOURCE_MEM,1);
    pmydev->IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 = ioremap(pres->start, 4);

	pres = platform_get_resource(p_pltdev,IORESOURCE_MEM,2);
    pmydev->GPIO5_GDIR                              = ioremap(pres->start, 4);

	pres = platform_get_resource(p_pltdev,IORESOURCE_MEM,3);
    pmydev->GPIO5_DR                                = ioremap(pres->start, 4);
}

void set_output_ledconreg(struct myled_dev *pmydev)
{
	writel((readl(pmydev->CCM_CCGR1) & (~(0xF << 30))) | (3 << 30),pmydev->CCM_CCGR1);
	writel((readl(pmydev->IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3) & (~(0xF))) | (5),pmydev->IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3);
	writel((readl(pmydev->GPIO5_GDIR) & (~(0xF << 3))) | (0x1 << 3),pmydev->GPIO5_GDIR);
}

void iounmap_ledreg(struct myled_dev *pmydev)
{
	iounmap(pmydev->CCM_CCGR1);
	pmydev->CCM_CCGR1 = NULL;
	iounmap(pmydev->IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3);
	pmydev->IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 = NULL;

	iounmap(pmydev->GPIO5_GDIR);
	pmydev->GPIO5_GDIR = NULL;
	iounmap(pmydev->GPIO5_DR);
	pmydev->GPIO5_DR = NULL;
	
}

int led_driver_probe(struct platform_device *p_pltdev)
{
	int ret = 0;
	dev_t devno = MKDEV(major,minor);

	/*申请设备号*/
	ret = register_chrdev_region(devno,myled_num,"myled");
	if(ret)
	{
		ret = alloc_chrdev_region(&devno,minor,myled_num,"myled");
		if(ret)
		{
			printk("get devno failed\n");
			return -1;
		}
		major = MAJOR(devno);//容易遗漏，注意
	}

	pgmydev = (struct myled_dev *)kmalloc(sizeof(struct myled_dev),GFP_KERNEL);
	if(NULL == pgmydev)
	{
		unregister_chrdev_region(devno,myled_num);
		printk("kmalloc failed\n");
		return -1;
	}
	memset(pgmydev,0,sizeof(struct myled_dev));

	/*给struct cdev对象指定操作函数集*/	
	cdev_init(&pgmydev->mydev,&myops);

	/*将struct cdev对象添加到内核对应的数据结构里*/
	pgmydev->mydev.owner = THIS_MODULE;
	cdev_add(&pgmydev->mydev,devno,myled_num);

	/*ioremap*/
	ioremap_ledreg(pgmydev, p_pltdev);

	/*con-register set output*/
	set_output_ledconreg(pgmydev);

	pgmydev->cls = class_create(THIS_MODULE,"/myled");
	if(IS_ERR(pgmydev->cls))
	{
		printk("class create failed\n");
		iounmap_ledreg(pgmydev);
		cdev_del(&pgmydev->mydev);
		unregister_chrdev_region(devno,myled_num);
		kfree(pgmydev);
		pgmydev = NULL;
	}
	pgmydev->led_dev = device_create(pgmydev->cls, NULL, devno, NULL, "myled0");
	if(pgmydev->led_dev == NULL)
	{
		printk("device create failed\n");
		class_destroy(pgmydev->cls);
	
		iounmap_ledreg(pgmydev);
		cdev_del(&pgmydev->mydev);
		unregister_chrdev_region(devno,myled_num);
		kfree(pgmydev);
		pgmydev = NULL;
	}
	return 0;
}

int led_driver_remove(struct platform_device *p_pltdev)
{
	dev_t devno = MKDEV(major,minor);

	device_destroy(pgmydev->cls, devno);
	class_destroy(pgmydev->cls);

	/*iounmap*/
	iounmap_ledreg(pgmydev);

	cdev_del(&pgmydev->mydev);

	unregister_chrdev_region(devno,myled_num);

	kfree(pgmydev);
	pgmydev = NULL;
	return 0;
}

struct platform_driver led_driver = 
{
	.driver.name = "hello",
	.probe =led_driver_probe,
	.remove =led_driver_remove,
};


int __init led_driver_init(void)
{
	platform_driver_register(&led_driver);
	return 0;
}

void __exit led_driver_exit(void)
{
	platform_driver_unregister(&led_driver);
}


MODULE_LICENSE("GPL");

module_init(led_driver_init);
module_exit(led_driver_exit);
