#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

#define CCM_CCGR1								0x20C406C
#define IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 0x2290014
#define GPIO5_GDIR								0x020AC000 + 0x4
#define GPIO5_DR								0x020AC000 + 0


void led_dev_release(struct device *pdev)
{
	printk("led_dev_release is called\n");
}

struct resource led_dev_res [] =
{
	[0] = {.start = CCM_CCGR1,.end=CCM_CCGR1+3,.name="CCM_CCGR1",.flags = IORESOURCE_MEM},
	[1] = {.start = IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3,.end=IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3+3,.name="IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3",.flags = IORESOURCE_MEM},
	[2] = {.start = GPIO5_GDIR,.end=GPIO5_GDIR+3,.name="GPIO5_GDIR",.flags = IORESOURCE_MEM},
	[3] = {.start = GPIO5_DR,.end=GPIO5_DR+3,.name="GPIO5_DR",.flags = IORESOURCE_MEM},
};

struct platform_device_id led_id = 
{
	.name = "led5",
};


struct platform_device led_device = 
{
	.name = "led5",
	.dev.release = led_dev_release,
	.resource = led_dev_res,
	.num_resources = ARRAY_SIZE(led_dev_res),
	.id_entry = &led_id,
};

int __init led_device_init(void)
{
	platform_device_register(&led_device);
	return 0;
}

void __exit led_device_exit(void)
{
	platform_device_unregister(&led_device);
}

MODULE_LICENSE("GPL");
module_init(led_device_init);
module_exit(led_device_exit);
