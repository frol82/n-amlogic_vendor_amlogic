#include <common.h>
#include <spicc.h>
#include <gpio.h>
#include <ldim_dev.h>
#include <panel.h>

unsigned char bl_mapping_iw7019[8] = {
	0, 1, 2, 3,
	4, 5, 6, 7,
	};

unsigned char bl_mapping_iw7027[16] = {
	12, 8,	4, 0,
	13, 9,	5, 1,
	14, 10, 6, 2,
	15, 11, 7, 3
	};


#define ARRAY_SIZE(array) (sizeof (array) / sizeof (array[0]))

extern void config_ldim_pwm_vs(unsigned char index, unsigned short pwm_freq, unsigned short pwm_duty);

unsigned char iw7019_ini_on[] = {
	/*step1:*/
	0x1, 0x8,
	/*step2:disable ocp and otp*/
	0x34, 0x14,
	0x10, 0x53,
	/*step3:*/
	0x11, 0x00,
	0x12, 0x02,
	0x13, 0x00,
	0x14, 0x40,
	0x15, 0x06,
	0x16, 0x00,
	0x17, 0x80,
	0x18, 0x0a,
	0x19, 0x00,
	0x1a, 0xc0,
	0x1b, 0x0e,
	0x1c, 0x00,
	0x1d, 0x00,
	0x1e, 0x50,
	0x1f, 0x00,
	0x20, 0x63,
	0x21, 0xff,
	0x2a, 0xff,
	0x2b, 0x41,
	0x2c, 0x28,
	0x30, 0xff,
	0x31, 0x00,
	0x32, 0x0f,
	0x33, 0x40,
	0x34, 0x40,
	0x35, 0x00,
	0x3f, 0xa3,
	0x45, 0x00,
	0x47, 0x04,
	0x48, 0x60,
	0x4a, 0x0d,
	/*step4:set brightness*/
	0x01, 0xff,
	0x02, 0xff,
	0x03, 0xff,
	0x04, 0xff,
	0x05, 0xff,
	0x06, 0xff,
	0x07, 0xff,
	0x08, 0xff,
	0x09, 0xff,
	0x0a, 0xff,
	0x0b, 0xff,
	0x0c, 0xff,
	/*step5:*/
	0x00, 0x09,
	/*step6:*/
	0x34, 0x00,
};

static void ldim_ctrl_port_init(void)
{
	/*set lamp_err port as input: GPIOZ_8*/
	*P_PREG_PAD_GPIO4_EN_N |= (1 << 8);

	/* init pwm_vsync gpio: GPIOX_13*/
	*P_PREG_PAD_GPIO3_EN_N &= ~(1 << 13);
	*P_PREG_PAD_GPIO3_O &= ~(1 << 13);
	/* init pwm_vsync */
	//config_ldim_pwm_vs(1, 120, 50); /* pwm_index, frequency(Hz), duty(%) */

	/*set LD_EN gpio as output pull_up: GPIOX_11*/
	*P_PERIPHS_PIN_MUX_3 &= ~((1 << 11) | (1 << 19));
	*P_PREG_PAD_GPIO3_EN_N &= ~(1 << 11);
	*P_PAD_PULL_UP_EN_REG3 |= (1 << 11);
	*P_PAD_PULL_UP_REG3 |= (1 << 11);
}

static void ldim_bl_en_ctrl(int flag)
{
	if (flag)
		*P_PREG_PAD_GPIO3_O |= (1 << 11);
	else
		*P_PREG_PAD_GPIO3_O &= ~(1 << 11);
}

static void ldim_pwm_port_ctrl(int flag)
{
	/* GPIOX_13*/
	if (flag)
		*P_PERIPHS_PIN_MUX_3 |= (1 << 13);
	else
		*P_PERIPHS_PIN_MUX_3 &= ~(1 << 13);
}

static unsigned int ldim_get_lamp_err(void)
{
	return ((*P_PREG_PAD_GPIO4_I >> 8) & 1);
}

static struct ldim_dev_config_s iw7019_config = {
	.name = "iw7019",
	.spi_speed = 1000000, /* Hz */
	.spi_mode = SPI_MODE_0,
	.cs_hold_delay = 10, /* us */
	.cs_clk_delay = 100, /* us */

	.en_gpio_on = 1,
	.en_gpio_off = 0,
	.fault_check = 0,
	.write_check = 0,

	.dim_min = 0x7f,
	.dim_max = 0xfff,
	.init_on_len = ARRAY_SIZE(iw7019_ini_on),
	.init_off_len = 0,
	.init_on = iw7019_ini_on,
	.init_off = NULL,

	.ctrl_port_init = ldim_ctrl_port_init,
	.enable_ctrl = ldim_bl_en_ctrl,
	.pwm_ctrl = ldim_pwm_port_ctrl,
	.get_lamp_err = ldim_get_lamp_err,
	.bl_mapping = bl_mapping_iw7019,
};

static struct ldim_dev_config_s iw7027_config = {
	.name = "iw7027",
	.spi_speed = 1000000, /* Hz */
	.spi_mode = SPI_MODE_0,
	.cs_hold_delay = 10, /* us */
	.cs_clk_delay = 100, /* us */

	.en_gpio_on = 1,
	.en_gpio_off = 0,
	.fault_check = 1,
	.write_check = 1,

	.dim_min = 0x10,
	.dim_max = 0xff,
	.init_on_len = 0,
	.init_off_len = 0,
	.init_on = NULL,
	.init_off = NULL,

	.ctrl_port_init = ldim_ctrl_port_init,
	.enable_ctrl = ldim_bl_en_ctrl,
	.pwm_ctrl = ldim_pwm_port_ctrl,
	.get_lamp_err = ldim_get_lamp_err,
	.bl_mapping = bl_mapping_iw7027,
};

struct ldim_dev_config_s *ldim_device_config_init(void)
{
	unsigned char index;
	struct ldim_dev_config_s *lconf = NULL;

	index = panel_param->bl_ldim_dev_index;
	switch (index) {
	case 0:
		lconf = &iw7019_config;
		break;
	case 1:
		lconf = &iw7027_config;
		break;
	default:
		printf("invalid ldim_dev_index %d\n", index);
		break;
	}
	printf("%s: index=%d \n", __func__, index);
	return lconf;
}

