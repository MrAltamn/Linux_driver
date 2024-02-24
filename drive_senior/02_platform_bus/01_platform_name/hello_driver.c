#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

int hello_driver_probe(struct platform_device *p_pltdev)
{
	struct resource *pres = NULL;

	printk("hello_driver_probe is called\n");

	pres = platform_get_resource(p_pltdev,IORESOURCE_MEM,2);
	printk("res.start = 0x%x\n",(unsigned int)pres->start);

	pres = platform_get_resource(p_pltdev,IORESOURCE_IRQ,1);
	printk("res.start = %d\n",(int)pres->start);
	return 0;
}

int hello_driver_remove(struct platform_device *p_pltdev)
{
	printk("hello_driver_remove is called\n");

	return 0;
}


struct platform_driver hello_driver = 
{
	.driver.name = "hello",
	.probe =hello_driver_probe,
	.remove =hello_driver_remove,
};


int __init hello_driver_init(void)
{
	platform_driver_register(&hello_driver);
	return 0;
}

void __exit hello_driver_exit(void)
{
	platform_driver_unregister(&hello_driver);
}

MODULE_LICENSE("GPL");
module_init(hello_driver_init);
module_exit(hello_driver_exit);
