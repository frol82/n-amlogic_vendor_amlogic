#include <serial.h>
#include <common.h>
#include <reboot.h>
#include <string.h>
#include <relocate.h>

typedef int ( *secend_boot ) ( int );

int main ( void )
{
	clock_init();
	serial_init ( 0 );
	printf ( "first boot serial init success!\n\n" );
	unsigned boot_flag = get_boot_flag();

	if (REBOOT_FLAG_FROM_WATCHDOG == boot_flag) {
		printf("to restore from section 1!\n");
		section_restore(get_spi_flash_device(0));
		reboot(REBOOT_FLAG_BOOT_ERROR);
	}

	if (copy_partition_to_sram2(SECTION_0, PARTITION_SECOND_BOOT)) {
		printf("second boot crc error!\n");
		reboot(REBOOT_FLAG_FROM_WATCHDOG);
	}
	else {
		printf("enter second boot!\n");
		( ( secend_boot ) SRAM2_BASE ) ( boot_flag );
		return 0;
	}
}
