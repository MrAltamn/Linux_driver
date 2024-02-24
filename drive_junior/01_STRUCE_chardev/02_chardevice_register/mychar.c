#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <uapi/linux/kdev_t.h>
#include <linux/cdev.h>


int major = 12;
int minor = 0;
int mychar_num = 2;

struct cdev mydev;

int mychar_open(struct inode *inode, struct file *file)
{
	printk("mychar_open is called\n");
 	return 0;
}

int mychar_close(struct inode *inode, struct file *file)
{
	printk("mychar_close is called\n");
 	return 0;
}


const struct file_operations myops = 
{
	.open = mychar_open,
	.release = mychar_close,
	.owner = THIS_MODULE,
};


int __init mychar_init(void)
{
	int ret = 0;
	dev_t devno = MKDEV(major, minor);
	
	ret = register_chrdev_region(devno,mychar_num,"mychar");
	if(ret)
	{
		ret = alloc_chrdev_region(&devno,minor,mychar_num,"mychar");
		if(ret)
		{
			printk("get devno failed\n");
		}
		
		major = MAJOR(devno);
	}
	

	cdev_init(&mydev,&myops);
	mydev.owner = THIS_MODULE;
	cdev_add(&mydev, devno, mychar_num);
	
	return 0;
}

void __exit mychar_exit(void)
{
	dev_t devno = MKDEV(major,minor);
	cdev_del(&mydev);
	unregister_chrdev_region(devno,mychar_num);
	
}


MODULE_LICENSE("GPL");

module_init(mychar_init);
module_exit(mychar_exit);
