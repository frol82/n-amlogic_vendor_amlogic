#include <string.h>
#include <stdlib.h>
#include <serial.h>
#include <common.h>
#include <reboot.h>
#include <remote.h>
#include <clock.h>
#include <gpio.h>
#include <inputdata.h>
#include <power.h>

#define DEVICE_UART_PORT_0       0
#define DEVICE_UART_PORT_1       1
#define DEVICE_UART_PORT_2       2




/****************************************
* We save gate & store gate for power save.
*
*****************************************/
__attribute__((weak)) void	prepare_suspend(void)
{

}

void save_gate(void)
{
}

void store_gate(void)
{
}

void switch_pll_to_24M(void)
{
	switch_clk_to_24m();
}

void switch_24M_to_32K(void)
{
	switch_clk_to_32k();
}

void switch_32K_to_24M(void)
{
	switch_clk_to_24m();
}


int main ( int argc, char *argv[] )
{
	unsigned int wake_src;
#if 0
	if ( calibrate_internal_osc() ) {
		reboot ( 0 );
	}
#endif

	save_pinmux();
	switch_pll_to_24M();
	prepare_suspend();
	save_gate();
	power_off_at_24M();

#if 0
	switch_24M_to_32K();
	power_off_at_32K();
#endif

	wake_src = detect_wakeup();

#if 0
	power_on_at_32K();
	switch_32K_to_24M();
#endif

	power_on_at_24M();
	store_gate();
/*
	switch_24M_to_pll();
*/
	store_pinmux();

	reboot ( REBOOT_FLAG_FROM_SUSPEND );

	return 0;
}
