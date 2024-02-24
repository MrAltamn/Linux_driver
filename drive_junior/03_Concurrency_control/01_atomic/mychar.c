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
	atomic_t openflag;
};

struct openonce_dev gmydev;


int mychar_open(struct inode *pnode, struct file *pfile)
{
	struct openonce_dev *pmydev = NULL;
	pfile->private_data = (void *)(container_of(pnode->i_cdev, struct openonce_dev, mydev));
	
	pmydev = (struct openonce_dev *)pfile->private_data;
	
	if(atomic_sub_and_test(1, &pmydev->openflag))
	{
		printk("mychar_open is called\n");
 		return 0;
	}else
	{
		atomic_add(1, &pmydev->openflag);
		printk("mychar_open has opened!\n");
		return -1;
	}
	
}

int mychar_close(struct inode *pnode, struct file *pfile)
{
	struct openonce_dev *pmydev = (struct openonce_dev *)pfile->private_data;

	atomic_set(&pmydev->openflag, 1);
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
	
	atomic_set(&gmydev.openflag, 1);
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
