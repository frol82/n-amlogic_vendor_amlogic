#include <stdlib.h>
#include <common.h>
#include <command.h>
#include <user_setting.h>
#include <panel.h>
#include <ui.h>
#include <lcd_drv.h>


int do_backlight_debug(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int i = 0, j = 0, val = 0;
	unsigned int tmp_off = 0, tmp_len = 0;
	const char *cmd;
	unsigned char *tmp_buf;
	char *tmp_ptr;
	char *endp;

	if ( argc < 2 ) {
		goto usage;
	}

	cmd = argv[1];

	if ( strcmp ( cmd, "power" ) == 0 ) {
		if ( argc < 3 ) {
			goto usage;
		}
		cmd = argv[2];
		val = strtoul ( cmd, &endp, 10 );
		backlight_power_ctrl((char)val);
		printf("backlight: power %s\n", val ? "on" : "off");
		return 0;
	} else if ( strcmp ( cmd, "set" ) == 0 ) {
		if ( argc < 3 ) {
			goto usage;
		}
		cmd = argv[2];
		val = strtoul ( cmd, &endp, 10 );
		backlight_set_level(val);
		nvm_write_backlight_level(val,1);
		return 0;
	} else if ( strcmp ( cmd, "get" ) == 0 ) {
		if ( argc < 2 ) {
			goto usage;
		}
		//val = backight_get_level();
		val = nvm_read_backlight_level();
		printf("backlight: level = %d\n", val);
		return 0;
	} else if ( strcmp ( cmd, "status" ) == 0 ) {
		if ( argc < 2 ) {
			goto usage;
		}
		backlight_info_print();
		return 0;
	}

usage:
	return cmd_usage ( cmdtp );
}

int do_panel_debug(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int i = 0, j = 0, val = 0;
	int temp[4];
	unsigned int tmp_off = 0, tmp_len = 0;
	const char *cmd;
	unsigned char *tmp_buf;
	char *tmp_ptr;
	char *endp;

	if ( argc < 2 ) {
		goto usage;
	}

	cmd = argv[1];

	if ( strcmp ( cmd, "enable" ) == 0 ) {
		if ( argc < 3 ) {
			goto usage;
		}
		cmd = argv[2];
		val = strtoul ( cmd, &endp, 10 );
		if (val)
			panel_enable();
		else
			panel_disable();
		return 0;
	} else if ( strcmp ( cmd, "test" ) == 0 ) {
		if ( argc < 3 ) {
			goto usage;
		}
		cmd = argv[2];
		val = strtoul ( cmd, &endp, 10 );
		/* to do */
		/* change to free_run mode, and show bist pattern */
		printf("to do\n");
		return 0;
	} else if ( strcmp ( cmd, "reset" ) == 0 ) {
		if ( argc < 2 ) {
			goto usage;
		}
		panel_disable();
		panel_enable();
		return 0;
	} else if ( strcmp ( cmd, "ss" ) == 0 ) {
		if ( argc < 3 ) {
			printf("lcd ss: %s(%d)\n", get_lcd_clk_ss(),
				panel_param->clk_ss_level);
			return 0;
		}
		cmd = argv[2];
		val = strtoul ( cmd, &endp, 10 );
		set_lcd_clk_ss(val);
		return 0;
	} else if ( strcmp ( cmd, "phy" ) == 0 ) {
		if ( argc < 4 ) {
			printf("lcd phy: vswing=%d, preem=%d\n",
				panel_param->vx1_lvds_phy_vswing,
				panel_param->vx1_lvds_phy_preem);
			return 0;
		}
		cmd = argv[2];
		temp[0] = strtoul ( cmd, &endp, 10 );
		cmd = argv[3];
		temp[1] = strtoul ( cmd, &endp, 10 );
		lcd_phy_adjust(temp[0], temp[1]);
		return 0;
	} else if ( strcmp ( cmd, "info" ) == 0 ) {
		if ( argc < 2 ) {
			goto usage;
		}
		lcd_info_print();
		return 0;
	} else if ( strcmp ( cmd, "reg" ) == 0 ) {
		if ( argc < 2 ) {
			goto usage;
		}
		lcd_reg_print();
		return 0;
	} else if ( strcmp ( cmd, "dump" ) == 0 ) {
		if ( argc < 2 ) {
			goto usage;
		}
		lcd_info_print();
		lcd_reg_print();
		return 0;
	} else if ( strcmp ( cmd, "logo" ) == 0 ) {
		if ( argc < 3 ) {
			goto usage;
		}
		cmd = argv[2];
		val = strtoul ( cmd, &endp, 10 );
		if (val)
			display_logo();
		else
			hide_logo();
		return 0;
	} else if ( strcmp ( cmd, "training" ) == 0 ) {
		vbyone_training_Handle();
	}

usage:
	return cmd_usage ( cmdtp );
}

