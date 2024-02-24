#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

void hello_dev_release(struct device *pdev)
{
	printk("hello_dev_release is called\n");
}

struct resource hello_dev_res [] =
{
	[0] = {.start = 0x1000,.end=0x1003,.name="reg1",.flags = IORESOURCE_MEM},
	[1] = {.start = 0x2000,.end=0x2003,.name="reg2",.flags = IORESOURCE_MEM},
	[2] = {.start = 10,.end=10,.name="irq1",.flags = IORESOURCE_IRQ},
	[3] = {.start = 0x3000,.end=0x3003,.name="reg3",.flags = IORESOURCE_MEM},
	[4] = {.start = 100,.end=100,.name="irq2",.flags = IORESOURCE_IRQ},
	[5] = {.start = 62,.end=62,.name="irq3",.flags = IORESOURCE_IRQ},
};

struct platform_device hello_device = 
{
	.name = "hello",
	.dev.release = hello_dev_release,
	.resource = hello_dev_res,
	.num_resources = ARRAY_SIZE(hello_dev_res),
};

int __init hello_device_init(void)
{
	platform_device_register(&hello_device);
	return 0;
}

void __exit hello_device_exit(void)
{
	platform_device_unregister(&hello_device);
}

MODULE_LICENSE("GPL");
module_init(hello_device_init);
module_exit(hello_device_exit);
