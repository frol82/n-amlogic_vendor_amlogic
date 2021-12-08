#include <key_const.h>
#include <customer_key_conf.h>

int REMOTE_TYPE = DECODEMODE_NEC;

int remote_coustomer_code = 0xfe01; /* customer code */
float ADC_VOLTAGE_MAX = 3.3;

int send_key_to_soc = 1;

int send_key_to_local_task = 1;

int send_remote_to_soc = 0;

int send_remote_to_local_task = 0;

int CUSTOMER_REMOTE_NUMS = 53;

/*
 remote: send keycode to soc,
 using key mapping from remote.conf
 */
int customer_key_map[][2] = {
	{ 0x00, 1 },
	{ 0x01, 2 },
	{ 0x02, 3 },
	{ 0x03, 4 },
	{ 0x04, 5 },
	{ 0x05, 6 },
	{ 0x06, 7 },
	{ 0x07, 8 },
	{ 0x08, 9 },
	{ 0x09, 10 },
	{ 0x0a, 11 },
	{ 0x0b, 12 },
	{ 0x0c, 13 },
	{ 0x0d, 14 },
	{ 0x0e, 15 },
	{ 0x0f, 16 },
	{ 0x10, 17 },
	{ 0x11, 18 },
	{ 0x12, 19 },
	{ 0x13, 20 },
	{ 0x14, 21 },
	{ 0x15, 22 },
	{ 0x16, 23 },
	{ 0x17, 24 },
	{ 0x18, 25 },
	{ 0x19, 26 },
	{ 0x1a, 27 },
	{ 0x1b, 28 },
	{ 0x1c, 29 },
	{ 0x1d, 30 },
	{ 0x1e, 31 },
	{ 0x1f, 32 },
	{ 0x38, 33 },
	{ 0x39, 34 },
	{ 0x40, 35 },
	{ 0x41, 36 },
	{ 0x42, 37 },
	{ 0x43, 38 },
	{ 0x44, 39 },
	{ 0x46, 40 },
	{ 0x49, 41 },
	{ 0x4b, 42 },
	{ 0x4c, 43 },
	{ 0x4e, 44 },
	{ 0x52, 45 },
	{ 0x53, 46 },
	{ 0x54, 47 },
	{ 0x55, 48 },
	{ 0x57, 49 },
	{ 0x58, 50 },
	{ 0x59, 51 },
	{ 0x5a, 52 },
	{ 0x5b, 53 },
};

int ADC_KEY_NUM_MAX = 5;

/* array num is ADC_KEY_NUM_MAX */
/* down,up,left,right,ok(enter) */
float adc_key_vol[] = { 0.48, 0.97, 1.42, 1.86, 2.8 };

/* array num is ADC_KEY_NUM_MAX */
int adc_key_code[] = { AMLKEY_DOWN, AMLKEY_UP, AMLKEY_LEFT, AMLKEY_RIGHT, AMLKEY_ENTER };

int adc_key_code1[] = { AMLKEY_RIGHT, AMLKEY_NOP, AMLKEY_ENTER, AMLKEY_NOP, AMLKEY_NOP };
