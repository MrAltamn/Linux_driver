#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

int major = 11;
int minor = 0;
int mychar_num = 1;

struct mysecond_dev
{
	struct cdev mydev;
	atomic_t openflag; //1 can open, 0 can not open

	int second;

	struct class *cls;
	struct device *dev;
	
	struct timer_list timer;
};

struct mysecond_dev gmydev;

void timer_func(unsigned long arg)
{
	struct mysecond_dev *pmydev = (struct mysecond_dev *)arg;

	pmydev->second++;

	mod_timer(&pmydev->timer,jiffies + HZ * 1);
}


int mysecond_open(struct inode *pnode, struct file *pfile)
{
	struct mysecond_dev *pmydev = NULL;
	pfile->private_data = (void *)(container_of(pnode->i_cdev, struct mysecond_dev, mydev));
	
	pmydev = (struct mysecond_dev *)pfile->private_data;
	
	if(atomic_sub_and_test(1, &pmydev->openflag))
	{
		pmydev->timer.expires = jiffies + HZ * 1;
		pmydev->timer.function = timer_func;
		pmydev->timer.data = (unsigned long)pmydev;

		add_timer(&pmydev->timer);
 		return 0;
	}else
	{
		atomic_add(1, &pmydev->openflag);
		printk("mychar_open has opened!\n");
		return -1;
	}
	
}

int mysecond_close(struct inode *pnode, struct file *pfile)
{
	struct mysecond_dev *pmydev = (struct mysecond_dev *)pfile->private_data;
	del_timer(&pmydev->timer);
	pmydev->second = 0;
	atomic_set(&pmydev->openflag, 1);
 	return 0;
}

ssize_t mysecond_read(struct file *pfile,char __user *puser,size_t size,loff_t *p_pos)
{
	struct mysecond_dev *pmydev = (struct mysecond_dev *)pfile->private_data;
	int ret = 0;

	if(size < sizeof(int))
	{
		printk("the expect read size is invalid\n");
		return -1;
	}

	if(size >= sizeof(int))
	{
		size = sizeof(int);
	}

	ret = copy_to_user(puser,&pmydev->second,size);
	if(ret)
	{
		printk("copy to user failed\n");
		return -1;
	}
	return size;
}


const struct file_operations myops = 
{
	.open = mysecond_open,
	.read = mysecond_read,
	.release = mysecond_close,
	.owner = THIS_MODULE,
};



int __init mychar_init(void)
{
	int ret = 0;
	dev_t devno = MKDEV(major, minor);
	
	ret = register_chrdev_region(devno,mychar_num,"mysecond");
	if(ret)
	{
		ret = alloc_chrdev_region(&devno,minor,mychar_num,"mysecond");
		if(ret)
		{
			printk("get devno failed\n");
		}
		
		major = MAJOR(devno);
	}
	
	cdev_init(&gmydev.mydev,&myops);
	gmydev.mydev.owner = THIS_MODULE;
	cdev_add(&gmydev.mydev, devno, mychar_num);

	init_timer(&gmydev.timer);
	
	atomic_set(&gmydev.openflag, 1);	

	gmydev.cls = class_create(THIS_MODULE, "mysec");
	if(IS_ERR(gmydev.cls))
	{
		printk("class create failed\n");
		cdev_del(&gmydev.mydev);
		unregister_chrdev_region(devno,mychar_num);
		return 0;
	}
	gmydev.dev = device_create(gmydev.cls, NULL, devno, NULL, "mysec0");
	if(gmydev.dev == NULL)
	{
		printk("device create failed\n");
		class_destroy(gmydev.cls);
		cdev_del(&gmydev.mydev);
		unregister_chrdev_region(devno,mychar_num);
		return 0;
	}
	return 0;
}

void __exit mychar_exit(void)
{
	dev_t devno = MKDEV(major, minor);

	device_destroy(gmydev.cls,devno);
	class_destroy(gmydev.cls);

	cdev_del(&gmydev.mydev);
	unregister_chrdev_region(devno,mychar_num);
	
}


MODULE_LICENSE("GPL");

module_init(mychar_init);
module_exit(mychar_exit);
