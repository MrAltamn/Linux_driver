#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#define BUF_LEN 100


int major = 11;
int minor = 0;
int mychar_num = 1;

struct mychar_dev
{
	struct cdev mydev;
	char mydev_buf[BUF_LEN];
	int curlen;
};

struct mychar_dev gmydev;

ssize_t mychar_read (struct file *pfile, char __user *puser, size_t count, loff_t *p_pos)
{
	struct mychar_dev *pmydev = (struct mychar_dev *)pfile->private_data;
	int size = 0;
	int ret = 0;
	if(count > pmydev->curlen)
		size = pmydev->curlen;
	}
	else
		size = count;
	}

	ret = copy_to_user(puser, pmydev->mydev_buf, size);
	if(ret)
	{
		printk("copy_to_user failed\n");
 		return -1;
	}

	memcpy(pmydev->mydev_buf, pmydev->mydev_buf + size, pmydev->curlen - size);
	pmydev->curlen -= size;
	
 	return size;
}

ssize_t mychar_write (struct file *pfile, const char __user *puser, size_t count, loff_t *p_pos)
{
	struct mychar_dev *pmydev = (struct mychar_dev *)pfile->private_data;

	int size = 0;
	int ret = 0;
	if(count > BUF_LEN - pmydev->curlen)
		size = BUF_LEN - pmydev->curlen;
	}
	else
	{
		size = count;
	}

	ret = copy_from_user(pmydev->mydev_buf + pmydev->curlen, puser, size);
	if(ret)
		printk("copy_from_user failed\n");
 		return -1;
	}
	pmydev->curlen += size;
	
 	return size;
}


int mychar_open(struct inode *pnode, struct file *pfile)
{
	pfile->private_data =(void *) (container_of(pnode->i_cdev, struct mychar_dev, mydev));
	printk("mychar_open is called\n");
 	return 0;
}

int mychar_close(struct inode *pnode, struct file *pfile)
{
	printk("mychar_close is called\n");
 	return 0;
}


struct file_operations myops = {
	.owner   = THIS_MODULE,				  
	.open 	 = mychar_open,
	.read    = mychar_read,
	.write   = mychar_write,
	.release = mychar_close,
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
