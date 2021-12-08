// ----------------------------------------------------------------------
//
// This file is from c_arc_pointer_reg.h
//
// ----------------------------------------------------------------------
//
#ifndef GPIO_H
#define GPIO_H
// ----------------------------
// Pad conntrols
// ----------------------------
#define P_PAD_PULL_UP_REG2                         (volatile unsigned long *)0x80030540
#define P_PAD_PULL_UP_REG3                         (volatile unsigned long *)0x80030544
#define P_PAD_PULL_UP_REG4                         (volatile unsigned long *)0x80030548
#define P_PAD_PULL_UP_EN_REG2                      (volatile unsigned long *)0x80030560
#define P_PAD_PULL_UP_EN_REG3                      (volatile unsigned long *)0x80030564
#define P_PAD_PULL_UP_EN_REG4                      (volatile unsigned long *)0x80030568
// ----------------------------
// GPIO
// ----------------------------
#define P_PREG_PAD_GPIO2_EN_N                      (volatile unsigned long *)0x80030580
#define P_PREG_PAD_GPIO2_O                         (volatile unsigned long *)0x80030584
#define P_PREG_PAD_GPIO2_I                         (volatile unsigned long *)0x80030588
// ----------------------------
#define P_PREG_PAD_GPIO3_EN_N                      (volatile unsigned long *)0x8003058c
#define P_PREG_PAD_GPIO3_O                         (volatile unsigned long *)0x80030590
#define P_PREG_PAD_GPIO3_I                         (volatile unsigned long *)0x80030594
// ----------------------------
#define P_PREG_PAD_GPIO4_EN_N                      (volatile unsigned long *)0x80030598
#define P_PREG_PAD_GPIO4_O                         (volatile unsigned long *)0x8003059c
#define P_PREG_PAD_GPIO4_I                         (volatile unsigned long *)0x800305a0
// ----------------------------
// Pin Mux
// ----------------------------
#define P_PERIPHS_PIN_MUX_2                        (volatile unsigned long *)0x800305a8
#define P_PERIPHS_PIN_MUX_3                        (volatile unsigned long *)0x800305ac
#define P_PERIPHS_PIN_MUX_4                        (volatile unsigned long *)0x800305b0
#define P_PERIPHS_PIN_MUX_5                        (volatile unsigned long *)0x800305b4

// ----------------------------
// Pad conntrols
// ----------------------------
#define P_PAD_PULL_UP_REG0                         (volatile unsigned long *)0x80030840
#define P_PAD_PULL_UP_REG1                         (volatile unsigned long *)0x80030844
#define P_PAD_PULL_UP_EN_REG0                      (volatile unsigned long *)0x80030860
#define P_PAD_PULL_UP_EN_REG1                      (volatile unsigned long *)0x80030864
// ----------------------------
// GPIO
// ----------------------------
#define P_PREG_PAD_GPIO0_EN_N                      (volatile unsigned long *)0x80030880
#define P_PREG_PAD_GPIO0_O                         (volatile unsigned long *)0x80030884
#define P_PREG_PAD_GPIO0_I                         (volatile unsigned long *)0x80030888
// ----------------------------
#define P_PREG_PAD_GPIO1_EN_N                      (volatile unsigned long *)0x8003088c
#define P_PREG_PAD_GPIO1_O                         (volatile unsigned long *)0x80030890
#define P_PREG_PAD_GPIO1_I                         (volatile unsigned long *)0x80030894
// ----------------------------
// Pin Mux
// ----------------------------
#define P_PERIPHS_PIN_MUX_0                        (volatile unsigned long *)0x800308a0
#define P_PERIPHS_PIN_MUX_1                        (volatile unsigned long *)0x800308a4

// ----------------------------
// GPIO define
// ----------------------------
#define GPIO_OUTPUT_LOW      0
#define GPIO_OUTPUT_HIGH     1
#define GPIO_INPUT           2


#define GPIOAO_0             0
#define GPIOAO_1             1
#define GPIOAO_2             2
#define GPIOAO_3             3
#define GPIOAO_4             4
#define GPIOAO_5             5
#define GPIOAO_6             6
#define GPIOAO_7             7
#define GPIOAO_8             8
#define GPIOAO_9             9
#define GPIOAO_10            10
#define GPIOAO_11            11
#define GPIOAO_12            12
#define GPIOAO_13            13
#define GPIOAO_14            14
#define GPIOAO_15            15

#define BOOT_0               16
#define BOOT_1               17
#define BOOT_2               18
#define BOOT_3               19
#define BOOT_4               20
#define BOOT_5               21

#define GPIOX_0              22
#define GPIOX_1              23
#define GPIOX_2              24
#define GPIOX_3              25
#define GPIOX_4              26
#define GPIOX_5              27
#define GPIOX_6              28
#define GPIOX_7              29
#define GPIOX_8              30
#define GPIOX_9              31
#define GPIOX_10             32
#define GPIOX_11             33
#define GPIOX_12             34
#define GPIOX_13             35

#define GPIOZ_0              36
#define GPIOZ_1              37
#define GPIOZ_2              38
#define GPIOZ_3              39
#define GPIOZ_4              40
#define GPIOZ_5              41
#define GPIOZ_6              42
#define GPIOZ_7              43
#define GPIOZ_8              44
#define GPIOZ_9              45
#define GPIOZ_10             46
#define GPIOZ_11             47
#define GPIOZ_12             48

#define GPIOB_0              49
#define GPIOB_1              50
#define GPIOB_2              51
#define GPIOB_3              52
#define GPIOB_4              53
#define GPIOB_5              54
#define GPIOB_6              55
#define GPIOB_7              56
#define GPIOB_8              57
#define GPIOB_9              58

#define GPIOC_0              59
#define GPIOC_1              60
#define GPIOC_2              61
#define GPIOC_3              62

#define GPIO_MAX             63

#if 1
	extern int aml_gpio_name_map_num ( const char *name );
	extern int aml_gpio_set_output ( int gpio, int flag );
	extern int aml_gpio_set_input ( int gpio );
	extern int aml_gpio_set ( int gpio, int flag );
	extern unsigned int aml_gpio_input_get ( int gpio );

	extern int aml_pinmux_set ( unsigned int mux_index, unsigned int mux_mask );
	extern int aml_pinmux_clr ( unsigned int mux_index, unsigned int mux_mask );
#endif

#endif // GPIO_H

