/*
 * Copyright (c) 2015 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */
#ifndef __POWER_H__
#define __POWER_H__

#ifndef IN_FBC_MAIN_CONFIG

extern void save_pinmux(void);
extern void store_pinmux(void);
extern void power_off_at_24M(void);
extern void power_on_at_24M(void);
extern void prepare_suspend(void);
extern unsigned int detect_wakeup(void);

#endif

#endif //__POWER_H__
