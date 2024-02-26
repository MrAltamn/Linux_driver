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
#include <linux/input.h>

struct w100askkey2_dev
{
	int gpio;
	int irqno;
	struct input_dev *pdev;
};

struct w100askkey2_dev *pgmydev = NULL;


irqreturn_t key2_irq_handle(int no,void *arg)
{
	struct w100askkey2_dev *pmydev = (struct w100askkey2_dev *)arg;
	int status1 = 0;
	int status2 = 0;

	status1 = gpio_get_value(pmydev->gpio);
	mdelay(1);
	status2 = gpio_get_value(pmydev->gpio);

	if(status1 != status2)
	{
		return IRQ_NONE;
	}

	if(status1)
	{
		input_event(pmydev->pdev, EV_KEY, KEY_2, 0);
		input_sync(pmydev->pdev);
	}
	else
	{
		input_event(pmydev->pdev, EV_KEY, KEY_2, 1);
		input_sync(pmydev->pdev);
	}
	

	return IRQ_HANDLED;
}

int __init w100askkey2_init(void)
{
	int ret = 0;
	struct device_node *pnode = NULL;
	
	/*通过路径查找到设备树节点，并返回首地址*/
	pnode = of_find_node_by_path("/100ask_key2");
	if(NULL == pnode)
	{
		printk("find node failed\n");
		return -1;
	}
	/*申请结构体内存*/
	pgmydev = (struct w100askkey2_dev *)kmalloc(sizeof(struct w100askkey2_dev),GFP_KERNEL);
	if(NULL == pgmydev)
	{
		printk("kmallc for struct w100askkey2_dev failed\n");
		return -1;
	}

	pgmydev->gpio = of_get_named_gpio(pnode, "gpio-key2", 0);
	pgmydev->irqno = irq_of_parse_and_map(pnode, 0);

	/*创建对象*/
	pgmydev->pdev = input_allocate_device();
	/*设置事件类型*/
	set_bit(EV_KEY, pgmydev->pdev->evbit);
	/*指定对象，哪一个按键*/
	set_bit(KEY_2, pgmydev->pdev->keybit);
	/*注册input到内核*/
	ret = input_register_device(pgmydev->pdev);
	
	ret = request_irq(pgmydev->irqno,key2_irq_handle,IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,"w100askkey2",pgmydev);
	if(ret)
	{
		printk("request_irq failed\n");
		input_unregister_device(pgmydev->pdev);
		input_free_device(pgmydev->pdev);
		kfree(pgmydev);
		pgmydev = NULL;
		return -1;
	}
	return 0;
}

void __exit w100askkey2_exit(void)
{
	input_unregister_device(pgmydev->pdev);
	input_free_device(pgmydev->pdev);
	free_irq(pgmydev->irqno,pgmydev);
	kfree(pgmydev);
	pgmydev = NULL;
}


MODULE_LICENSE("GPL");

module_init(w100askkey2_init);
module_exit(w100askkey2_exit);
