#ifndef AP_3216C_H
#define AP_3216C_H

#define AP3216C_MAGIC 'K'

union ap3216c_data
{
	unsigned short als;
	unsigned short ps;
	unsigned short led;
};

#define GET_ALS _IOR(AP3216C_MAGIC, 0, union ap3216c_data)
#define GET_PS  _IOR(AP3216C_MAGIC, 1, union ap3216c_data) 
#define GET_LED _IOR(AP3216C_MAGIC, 2, union ap3216c_data)



#endif
