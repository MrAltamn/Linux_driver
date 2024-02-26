#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>

static unsigned short const addr_list[] = {
	0x68,
	0x1e,
	I2C_CLIENT_END
};

static struct i2c_client *gpap3216c_client = NULL;

int __init ask100_ap3216c_client_init(void)
{
	struct i2c_adapter *padp = NULL;

	struct i2c_board_info ap3216c_info = {""};
	strcpy(ap3216c_info.type, "ap3216c");
	
	padp = i2c_get_adapter(0); //得到client对应的哪一个通道
	gpap3216c_client = i2c_new_probed_device(padp, &ap3216c_info, addr_list, NULL);
	i2c_put_adapter(padp);
	
	return 0;
}

void __exit ask100_ap3216c_client_exit(void)
{
	i2c_unregister_device(gpap3216c_client);
}

MODULE_LICENSE("GPL");

module_init(ask100_ap3216c_client_init);
module_exit(ask100_ap3216c_client_exit);











