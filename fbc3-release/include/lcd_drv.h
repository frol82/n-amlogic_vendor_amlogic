#ifndef _LCD_DRV_H_
#define _LCD_DRV_H_

extern void lcd_phy_adjust(unsigned int vswing, unsigned int preem);
extern void set_LVDS_output ( unsigned char clk, unsigned char repack, unsigned char odd_even,
			unsigned char hv_invert, unsigned char pn_swap, unsigned char ports,
			unsigned char bit_size, unsigned char lvds_swap, unsigned char clk_pin_swap );
extern void set_VX1_output ( int lane_num, int byte_num, int region_num, int color_fmt, int hsize, int vsize );


extern void vbyone_training_Handle( void );
extern void lvds_vx1_reset ( void );
extern void vx1_handle_on(int onoff);
extern void vx1_irq_pro(void);
extern void lcd_info_print(void);
extern void lcd_reg_print(void);

extern void lcd_module_init(void);
extern void lcd_signal_ctrl(char flag);
extern void init_lcd(void);

#endif

