#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <uapi/linux/kdev_t.h>

int major = 12;
int minor = 0;
int mychar_num = 2;

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
	
	return 0;
}

void __exit mychar_exit(void)
{
	dev_t devno = MKDEV(major,minor);
	unregister_chrdev_region(devno,mychar_num);
}


MODULE_LICENSE("GPL");

module_init(mychar_init);
module_exit(mychar_exit);
