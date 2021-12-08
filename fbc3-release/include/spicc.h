/*
 * Interface to SPI communication controller
 *
 */
#ifndef __SPI_COMMUNICATION_H__
#define __SPI_COMMUNICATION_H__

#ifndef NULL
	#define NULL	((void *)0)
#endif


#define SPI_COMMUNICATION_NAME	"FBC_SPI_COMMUNICATION"
#define SPICC_CLK_SRC	SYS_PLL_CLK_SRC

#define SPI_CPHA		0x01
#define SPI_CPOL		0x02

#define SPI_MODE_0		(0|0)
#define SPI_MODE_1		(0|SPI_CPHA)
#define SPI_MODE_2		(SPI_CPOL|0)
#define SPI_MODE_3		(SPI_CPOL|SPI_CPHA)

#define SPI_CS_HIGH		0x04
#define SPI_LSB_FIRST	0x08
#define SPI_3WIRE		0x10
#define SPI_LOOP		0x20
#define SPI_NO_CS		0x40
#define SPI_READY		0x80



#define SPICC_FIFO_SIZE 16

#define CON_ENABLE 0
#define CON_ENABLE_LENGTH 1
#define CON_MODE 1
#define CON_MODE_LENGTH 1
#define CON_XCH 2
#define CON_XCH_LENGTH 1
#define CON_SMC 3
#define CON_SMC_LENGTH 1
#define CON_CLK_POL 4
#define CON_CLK_POL_LENGTH 1
#define CON_CLK_PHA 5
#define CON_CLK_PHA_LENGTH 1
#define CON_SS_CTL 6
#define CON_SS_CTL_LENGTH 1
#define CON_SS_POL 7
#define CON_SS_POL_LENGTH 1
#define CON_DRCTL 8
#define CON_DRCTL_LENGTH 2
#define CON_CHIP_SELECT 12
#define CON_CHIP_SELECT_LENGTH 2
#define CON_DATA_RATE_DIV 16
#define CON_DATA_RATE_DIV_LENGTH 3
#define CON_BITS_PER_WORD 19
#define CON_BITS_PER_WORD_LENGTH 6
#define CON_BURST_LEN 25
#define CON_BURST_LEN_LENGTH 7

#define INT_TX_EMPTY_EN 0
#define INT_TX_EMPTY_EN_LENGTH 1
#define INT_TX_HALF_EN 1
#define INT_TX_HALF_EN_LENGTH 1
#define INT_TX_FULL_EN 2
#define INT_TX_FULL_EN_LENGTH 1
#define INT_RX_READY_EN 3
#define TNT_RX_READY_EN_LENGTH 1
#define INT_RX_HALF_EN 4
#define INT_RX_HALF_EN_LENGTH 1
#define INT_RX_FULL_EN 5
#define INT_RX_FULL_EN_LENGTH 1
#define INT_RX_OF_EN 6
#define INT_RX_OF_EN_LENGTH 1
#define INT_XFER_COM_EN 7
#define INT_XFER_COM_EN_LENGTH 1

#define DMA_EN 0
#define DMA_EN_LENGTH 1
#define DMA_TX_FIFO_TH 1
#define DMA_TX_FIFO_TH_LENGTH 5
#define DMA_RX_FIFO_TH 6
#define DMA_RX_FIFO_TH_LENGTH 5
#define DMA_NUM_RD_BURST 11
#define DMA_NUM_RD_BURST_LENGTH 4
#define DMA_NUM_WR_BURST 15
#define DMA_NUM_WR_BURST_LENGTH 4
#define DMA_URGENT 19
#define DMA_URGENT_LENGTH 1
#define DMA_THREAD_ID 20
#define DMA_THREAD_ID_LENGTH 6
#define DMA_BURST_NUM 26
#define DMA_BURST_NUM_LENGTH 6

#define STA_TX_EMPTY 0
#define STA_TX_EMPTY_LENGTH 1
#define STA_TX_HALF 1
#define STA_TX_HALF_LENGTH 1
#define STA_TX_FULL 2
#define STA_TX_FULL_LENGTH 1
#define STA_RX_READY 3
#define STA_RX_READY_LENGTH 1
#define STA_RX_HALF 4
#define STA_RX_HALF_LENGTH 1
#define STA_RX_FULL 5
#define STA_RX_FULL_LENGTH 1
#define STA_RX_OF 6
#define STA_RX_OF_LENGTH 1
#define STA_XFER_COM 7
#define STA_XFER_COM_LENGTH 1

/*#define CLK_FREE_EN bits_desc(SPICC_REG_TEST, 24, 1)*/
#define CLK_FREE_EN 24
#define CLK_FREE_EN_LENGTH 1



struct spi_communication {
	const char *name;
	int clk_src;
	int bits_per_word;
	int mode;
	int speed;
	int ( *read ) ( struct spi_communication *spicc, unsigned char *buf, int len );
	int ( *write ) ( struct spi_communication *spicc, unsigned char *buf, int len );
	int ( *xfer ) ( struct spi_communication *spicc, unsigned char *tx_buf, unsigned char *rx_buf, int len );
	void ( *select ) ( struct spi_communication *spicc, int select );
	int ( *setup ) ( struct spi_communication *spicc );
	int ( *init ) ( struct spi_communication *spicc );
};

static int spicc_read ( struct spi_communication *spicc, unsigned char *buf, int len );
static int spicc_write ( struct spi_communication *spicc, unsigned char *buf, int len );
static int spicc_xfer ( struct spi_communication *spicc, unsigned char *tx_buf, unsigned char *rx_buf, int len );
static void spicc_chip_select ( struct spi_communication *spicc, int select );
static int spicc_setup ( struct spi_communication *spicc );
static int spicc_init ( struct spi_communication *spicc );
struct spi_communication *get_spicc_device ( int index );


#endif /*__SPI_COMMUNICATION_H__*/
