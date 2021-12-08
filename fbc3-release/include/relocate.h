#ifndef __RELOCATE_H__
#define __RELOCATE_H__

#include <spi_flash.h>
#include <spi_regional_division.h>


#define ICCM_BASE 0x00000000
#define ICCM_SIZE 0x20000
#define DCCM_BASE 0xffff0000
#define DCCM_SIZE 0x10000
#define SRAM1_BASE 0x70020000
#define SRAM2_BASE 0x80020000
#define SRAM_SIZE 0x10000

extern partition_info_t* get_partition_info(int section, int partition);
extern int check_partition(partition_info_t *info);
extern int copy_partition_to_sram2(int section, int partition);
extern int copy_partition_to_ccm(int section, int partition);
extern int move_image(struct spi_flash *flash,unsigned s_offs,unsigned b_offs,unsigned size);
extern int section_backup(struct spi_flash *flash);
extern int section_restore(struct spi_flash *flash);
extern int illegal_address_check(unsigned offset, unsigned size);

#endif