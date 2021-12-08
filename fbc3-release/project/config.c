#include <board_config.h>

const char* switch_p = ((void*)0);
int bit10_mode = 0;
int hdmi_420Mode = 0;

void init_configures(void)
{
	switch_p = CONFIG_SWITCH_P;
	bit10_mode = CONFIG_BIT_MODE;
	hdmi_420Mode = CONFIG_HDMITx_YUV420;
}