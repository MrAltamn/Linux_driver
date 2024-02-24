#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "100ask_key.h"


int major = 11;
int minor = 0;
int w100askkey2_num  = 1;

struct w100askkey2_dev
{
	struct cdev mydev;

	int gpio;
	int irqno;

	struct keyvalue data;
	int newflag;
	spinlock_t lock;

	wait_queue_head_t rq;
};

struct w100askkey2_dev *pgmydev = NULL;

int w100askkey2_open(struct inode *pnode,struct file *pfile)
{
	pfile->private_data =(void *) (container_of(pnode->i_cdev,struct w100askkey2_dev,mydev));
	return 0;
}

int w100askkey2_close(struct inode *pnode,struct file *pfile)
{

	return 0;
}

ssize_t w100askkey2_read(struct file *pfile,char __user *puser,size_t count,loff_t *p_pos)
{
	struct w100askkey2_dev *pmydev = (struct w100askkey2_dev *)pfile->private_data;
	int size = 0;
	int ret = 0;

	if(count < sizeof(struct keyvalue))
	{
		printk("expect read size is invalid\n");
		return -1;
	}

	spin_lock(&pmydev->lock);
	if(!pmydev->newflag)
	{
		if(pfile->f_flags & O_NONBLOCK)
		{//非阻塞
			spin_unlock(&pmydev->lock);
			printk("O_NONBLOCK No Data Read\n");
			return -1;
		}
		else
		{//阻塞
			spin_unlock(&pmydev->lock);
			ret = wait_event_interruptible(pmydev->rq,pmydev->newflag == 1);
			if(ret)
			{
				printk("Wake up by signal\n");
				return -ERESTARTSYS;
			}
			spin_lock(&pmydev->lock);
		}
	}

	if(count > sizeof(struct keyvalue))
	{
		size = sizeof(struct keyvalue);
	}
	else
	{
		size = count;
	}

	ret = copy_to_user(puser,&pmydev->data,size);
	if(ret)
	{
		spin_unlock(&pmydev->lock);
		printk("copy_to_user failed\n");
		return -1;
	}

	pmydev->newflag = 0;

	spin_unlock(&pmydev->lock);

	return size;
}

/*
unsigned int w100askkey2_poll(struct file *pfile,poll_table *ptb)
{
	struct fs4412key2_dev *pmydev = (struct fs4412key2_dev *)pfile->private_data;
	unsigned int mask = 0;

	poll_wait(pfile,&pmydev->rq,ptb);

	spin_lock(&pmydev->lock);
	if(pmydev->newflag)
	{
		mask |= POLLIN | POLLRDNORM;
	}
	spin_unlock(&pmydev->lock);

	return mask;
}



*/

struct file_operations myops = {
	.owner = THIS_MODULE,
	.open = w100askkey2_open,
	.release = w100askkey2_close,
	.read = w100askkey2_read,
	//.poll = w100askkey2_poll,
};

irqreturn_t key2_irq_handle(int no,void *arg)
{
	struct w100askkey2_dev *pmydev = (struct w100askkey2_dev *)arg;
	int status1 = 0;
	int status2 = 0;
	int status = 0;

	status1 = gpio_get_value(pmydev->gpio);
	mdelay(1);
	status2 = gpio_get_value(pmydev->gpio);

	if(status1 != status2)
	{
		return IRQ_NONE;
	}

	status = status1;

	spin_lock(&pmydev->lock);
	if(status == pmydev->data.status)
	{
		spin_unlock(&pmydev->lock);
		return IRQ_NONE;
	}

	pmydev->data.code = KEY2;
	pmydev->data.status = status;
	pmydev->newflag = 1;

	spin_unlock(&pmydev->lock);
	wake_up(&pmydev->rq);

	return IRQ_HANDLED;
}

int __init w100askkey2_init(void)
{
	int ret = 0;
	dev_t devno = MKDEV(major,minor);

	struct device_node *pnode = NULL;

	pnode = of_find_node_by_path("/100ask_key2");
	if(NULL == pnode)
	{
		printk("find node failed\n");
		return -1;
	}


	pgmydev = (struct w100askkey2_dev *)kmalloc(sizeof(struct w100askkey2_dev),GFP_KERNEL);
	if(NULL == pgmydev)
	{
		printk("kmallc for struct w100askkey2_dev failed\n");
		return -1;
	}

	pgmydev->gpio = of_get_named_gpio(pnode,"gpio-key2",0);

	pgmydev->irqno = irq_of_parse_and_map(pnode,0);

	/*申请设备号*/
	ret = register_chrdev_region(devno,w100askkey2_num,"w100askkey2");
	if(ret)
	{
		ret = alloc_chrdev_region(&devno,minor,w100askkey2_num,"w100askkey2");
		if(ret)
		{
			kfree(pgmydev);
			pgmydev = NULL;
			printk("get devno failed\n");
			return -1;
		}
		major = MAJOR(devno);//容易遗漏，注意
	}

	/*给struct cdev对象指定操作函数集*/	
	cdev_init(&pgmydev->mydev,&myops);

	/*将struct cdev对象添加到内核对应的数据结构里*/
	pgmydev->mydev.owner = THIS_MODULE;
	cdev_add(&pgmydev->mydev,devno,w100askkey2_num);


	init_waitqueue_head(&pgmydev->rq);

	spin_lock_init(&pgmydev->lock);
	
	ret = request_irq(pgmydev->irqno,key2_irq_handle,IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,"w100askkey2",pgmydev);
	if(ret)
	{
		printk("request_irq failed\n");
		cdev_del(&pgmydev->mydev);
		kfree(pgmydev);
		pgmydev = NULL;
		unregister_chrdev_region(devno,w100askkey2_num);
		return -1;
	}
	return 0;
}

void __exit w100askkey2_exit(void)
{
	dev_t devno = MKDEV(major,minor);

	free_irq(pgmydev->irqno,pgmydev);

	cdev_del(&pgmydev->mydev);

	unregister_chrdev_region(devno,w100askkey2_num);

	kfree(pgmydev);
	pgmydev = NULL;
}


MODULE_LICENSE("GPL");

module_init(w100askkey2_init);
module_exit(w100askkey2_exit);
