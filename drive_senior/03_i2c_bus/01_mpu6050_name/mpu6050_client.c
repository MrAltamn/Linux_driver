#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>

struct i2c_board_info mpu6050_info = {
	I2C_BOARD_INFO("mpu6050",0x68)
};


static struct i2c_client *gpmpu6050_client = NULL;

int __init ask100_mpu6050_client_init(void)
{
	struct i2c_adapter *padp = NULL;

	padp = i2c_get_adapter(5); //得到client对应的哪一个通道
	gpmpu6050_client = i2c_new_device(padp, &mpu6050_info);
	i2c_put_adapter(padp);
	
	return 0;
}

void __exit ask100_mpu6050_client_exit(void)
{
	i2c_unregister_device(gpmpu6050_client);
}

MODULE_LICENSE("GPL");

module_init(ask100_mpu6050_client_init);
module_exit(ask100_mpu6050_client_exit);











