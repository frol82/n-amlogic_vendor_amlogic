#ifndef _FBC_CLOCK_H_
#define _FBC_CLOCK_H_

#define SYS_PLL_CLK_SRC		(0x1)

void set_sys_pll ( int M, int N, int OD, int reset, int enable );
void clock_init();
int clk_get_rate ( unsigned clksrc );
int calibrate_internal_osc();
int switch_clk_to_32k();
int switch_clk_to_24m();
void ctrl_crystal_pad ( int on );
void us_delay ( int us );
void us_delay_volatile ( int us );

extern int switch_clk_to_24M ( void );

#endif
