#ifndef __UI_H__
#define __UI_H__

#include <key_const.h>

#define KEY_CMD_MAX             7

struct key_cmd_t {
	char *key_name;
	int key_value;
};

static const struct key_cmd_t options[KEY_CMD_MAX] = {
	{ "up", AMLKEY_UP },
	{ "down", AMLKEY_DOWN },
	{ "menu", AMLKEY_MENU },
	{ "left", AMLKEY_LEFT },
	{ "right", AMLKEY_RIGHT },
	{ "exit", AMLKEY_EXIT },
	{ "enter", AMLKEY_ENTER },
};

struct menu_item_t {
	int id;
	int handle;
	char *name;
	int line;
	int col;
	struct menu_item_t *up;
	struct menu_item_t *down;
	struct menu_item_t *next;
	struct menu_item_t *pre;
	int ( *key_process ) ( struct menu_item_t *item, int key );
	int ( *execute ) ( struct menu_item_t *item );
};

struct menu_item_value_t {
	int handle;
	char *name;
	int line;
	int col;
};

#define MENU_START_LINE 0
#define MENU_START_POSITION 0

#define eInfo_HideLogo		0x0001
#define eInfo_ShowLogo		0x0002
#define eInfo_HideInfo		0x0004
#define eInfo_ShowNosignal	0x0008


typedef struct info_pro{
	int info_do;
	int mode;
}s_info_pro;

extern void init_ui ( void );
extern int UiGetHaveLogoFlag ( void );

extern int KeyFunc ( unsigned int input_type, unsigned int customer_code,
					 unsigned int key_value, unsigned int repeatstatus );

extern void init_osd ( void );
extern void hide_logo ( void );

extern void init_logo_osd ( void );

extern int process_menu ( struct menu_item_t *item, int key );
extern int do_test1 ( struct menu_item_t *item );
extern int do_test2 ( struct menu_item_t *item );
extern int adjust_brightness ( struct menu_item_t *item );
extern int adjust_contrast ( struct menu_item_t *item );
extern int adjust_volume ( struct menu_item_t *item );
extern int adjust_mute ( struct menu_item_t *item );
extern void show_menu ( void );
extern void hide_menu ( void );
extern void invalidate ( struct menu_item_t *last, struct menu_item_t *curr );
extern void invalidateValue ( struct menu_item_t *item, int key );
extern void invalidateInitValue ( void );
extern void display_logo(void);
extern void info_mode(int mode);
#endif
