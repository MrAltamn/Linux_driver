#ifndef ASK100_KEY_H
#define ASK100_KEY_H

enum KEYCODE
{
	KEY2 = 1002,
	KEY3,
	KEY4,
};

enum KEY_STATUS
{
	KEY_DOWN = 0,
	KEY_UP,
};

struct keyvalue
{
	int code;//which KEY
	int status;
};

#endif
