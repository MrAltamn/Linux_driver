#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <uapi/linux/kdev_t.h>
#include <linux/cdev.h>


int major = 12;
int minor = 0;
int mychar_num = 2;

struct openonce_dev
{
	struct cdev mydev;
	spinlock_t lock;
	int lock_flag;
};

struct openonce_dev gmydev;


int mychar_open(struct inode *pnode, struct file *pfile)
{
	struct openonce_dev *pmydev = NULL;
	pfile->private_data = (void *)(container_of(pnode->i_cdev, struct openonce_dev, mydev));
	
	pmydev = (struct openonce_dev *)pfile->private_data;

	spin_lock(&pmydev->lock);
	if(pmydev->lock_flag)
	{
	 	pmydev->lock_flag = 0;
		printk("mychar_open is called\n");
		spin_unlock(&pmydev->lock);
 		return 0;
	}else
	{
		printk("mychar_open has called\n");
		return -1;
	}
	
}

int mychar_close(struct inode *pnode, struct file *pfile)
{
	struct openonce_dev *pmydev = (struct openonce_dev *)pfile->private_data;
	spin_lock(&pmydev->lock);
	printk("mychar_close is called\n");
	pmydev->lock_flag = 0;
	spin_unlock(&pmydev->lock);
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

	spin_lock_init(&gmydev.lock);
	gmydev.lock_flag = 1;
	cdev_init(&gmydev.mydev,&myops);
	gmydev.mydev.owner = THIS_MODULE;
	cdev_add(&gmydev.mydev, devno, mychar_num);
	
	return 0;
}

void __exit mychar_exit(void)
{
	dev_t devno = MKDEV(major,minor);
	cdev_del(&gmydev.mydev);
	unregister_chrdev_region(devno,mychar_num);
	
}


MODULE_LICENSE("GPL");

module_init(mychar_init);
module_exit(mychar_exit);
