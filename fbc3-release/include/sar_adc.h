
#ifndef __SAR_ADC_H__
#define __SAR_ADC_H__


#include <inputdata.h>

void sar_adc_init ( void );
extern int detect_adc_key ( int chanel, INPUTDATA *inputdata );
extern int set_redetect_flag ( void );
extern int adc2key ( int chan, INPUTDATA *data );

#define SUSPEND_32K
#ifdef SUSPEND_32K
	/* is_32k: 1 32k ; 0 24mhz */
	extern void saradc_init ( int is_32k );
	extern int is_adc_finished ( void );
	/* chan 0 or 1 ... */
	extern void adc_start_sample ( int chan );
	/* chan 0 or 1 ... */
	extern int adc_detect_key ( int chan );
#endif

















#endif