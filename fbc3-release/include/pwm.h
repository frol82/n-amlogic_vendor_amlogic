#ifdef PWM_H
#else
	#define PWM_H


	void led_pwm_init ( void );
	//level: 0--255
	void led_bl_level_set ( int level );

#endif // PWM_H


