/*****************************************************************************
	Copyright(c) 2017 FCI Inc. All Rights Reserved

	File name : fc8350_spi.c

	Description : source of SPI interface

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	History :
	----------------------------------------------------------------------
*******************************************************************************/
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/module.h>

#include "fci_types.h"
#include "fc8350_regs.h"
#include "fci_oal.h"
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#ifdef EVB
#include <mach/dma.h>
#include <plat/s3c64xx-spi.h>
#endif
#define DRIVER_NAME "tdmb"

#define SPI_LEN             0x00 /* or 0x10 */
#define SPI_REG             0x20
#define SPI_THR             0x30
#define SPI_READ            0x40
#define SPI_WRITE           0x00
#define SPI_AINC            0x80

struct spi_device *fc8350_spi;
static u8 tx_data[32] __cacheline_aligned;
static u8 *wdata_buf;
static u8 *rdata_buf;
#define CHIPID             0
#ifdef EVB
#define S3C64XX_SPI_SWAP_CFG    0x28

#define S3C64XX_SPI_SWAP_RX_HALF_WORD           (1<<7)
#define S3C64XX_SPI_SWAP_RX_BYTE                (1<<6)
#define S3C64XX_SPI_SWAP_RX_BIT                 (1<<5)
#define S3C64XX_SPI_SWAP_RX_EN                  (1<<4)
#define S3C64XX_SPI_SWAP_TX_HALF_WORD           (1<<3)
#define S3C64XX_SPI_SWAP_TX_BYTE                (1<<2)
#define S3C64XX_SPI_SWAP_TX_BIT                 (1<<1)
#define S3C64XX_SPI_SWAP_TX_EN                  (1<<0)

#define S3C64XX_SPI_SWAP_OFF    0

struct s3c64xx_spi_driver_data {
	void __iomem                    *regs;
	struct clk                      *clk;
	struct clk                      *src_clk;
	struct platform_device          *pdev;
	struct spi_master               *master;
	struct workqueue_struct         *workqueue;
	struct s3c64xx_spi_info  *cntrlr_info;
	struct spi_device               *tgl_spi;
	struct work_struct              work;
	struct list_head                queue;
	spinlock_t                      lock;
	enum dma_ch                     rx_dmach;
	enum dma_ch                     tx_dmach;
	unsigned long                   sfr_start;
	struct completion               xfer_completion;
	unsigned                        state;
	unsigned                        cur_mode, cur_bpw;
	unsigned                        cur_speed;
};

static void spi_swap_set(struct spi_device *spi, int swap_cfg)
{
	struct spi_master *master = spi->master;
	struct s3c64xx_spi_driver_data *sdd = spi_master_get_devdata(master);
	void __iomem *regs = sdd->regs;

	writel(swap_cfg, regs + S3C64XX_SPI_SWAP_CFG);
}
#endif

 //lihaiyan@wind-mobi.com 20171030 +++
extern s32 isdbt_chip_id(void);
static DEFINE_MUTEX(fci_spi_lock);
static int fc8350_spi_probe(struct spi_device *spi)
{
	s32 ret = 0;

	print_log(0, "fc8350_spi_probe\n");

	spi->max_speed_hz = 50000000;
	spi->bits_per_word = 8;
	spi->mode =  SPI_MODE_0;

	ret = spi_setup(spi);

	if (ret < 0)
		return ret;

	fc8350_spi = spi;
	/* lihaiyan */
	printk("[wind_dtv] %s ,all init success , start get chip id\n", __func__); 
	isdbt_chip_id();

	return ret;
}
 //lihaiyan@wind-mobi.com 20171030 ---
 
static int fc8350_spi_remove(struct spi_device *spi)
{

	return 0;
}
struct of_device_id fc8350_match_table[] = {
	{
		.compatible = "fci,isdbt",
	},
	{}
};
static struct spi_driver fc8350_spi_driver = {
	.driver = {
		.name		= DRIVER_NAME,
		.owner		= THIS_MODULE,
		.of_match_table = fc8350_match_table,
	},
	.probe		= fc8350_spi_probe,
	.remove		= fc8350_spi_remove,
};
int fci_spi_write_then_read(struct spi_device *spi
	, u8 *txbuf, u16 tx_length, u8 *rxbuf, u16 rx_length)
{
	s32 res;
	struct spi_message  message;
	struct spi_transfer x;

	if (spi == NULL) {
		print_log(0, "[ERROR] FC8350_SPI Handle Fail...........\n");
		return BBM_NOK;
	}
	spi_message_init(&message);
	memset(&x, 0, sizeof(x));
	spi_message_add_tail(&x, &message);
	memcpy(wdata_buf, txbuf, tx_length);
	x.tx_buf = wdata_buf;
	x.rx_buf = rdata_buf;
	x.len = tx_length + rx_length;
	x.cs_change = 0;
#ifdef EVB
	if (x.len > 188) {
		int i;
		x.bits_per_word = 32;
		for (i = tx_length; i >= 0; i--)
			wdata_buf[i-1] = txbuf[tx_length - i];
	} else
#endif
	x.bits_per_word = 8;
#ifdef EVB
	if (x.bits_per_word == 8)
		spi_swap_set(spi, S3C64XX_SPI_SWAP_OFF);
	else
		spi_swap_set(spi,
		S3C64XX_SPI_SWAP_RX_EN
		|S3C64XX_SPI_SWAP_RX_BYTE
		|S3C64XX_SPI_SWAP_RX_HALF_WORD);
#endif
	res = spi_sync(spi, &message);

	if (rxbuf != NULL)
		memcpy(rxbuf, x.rx_buf + tx_length, rx_length);

	return res;
}

static s32 spi_bulkread(HANDLE handle, u8 devid,
		u16 addr, u8 command, u8 *data, u16 length)
{
	s32 res;

	tx_data[0] = addr & 0xff;
	tx_data[1] = (addr >> 8) & 0xff;
	tx_data[2] = (command & 0xf0) | (devid & 0x00ff);
	tx_data[3] = length & 0xff;
	res = fci_spi_write_then_read(fc8350_spi
		, &tx_data[0], 4, &data[0], length);
	if (res) {
		print_log(0, "fc8350_spi_bulkread fail : %d\n", res);
		return BBM_NOK;
	}

	return BBM_OK;
}

static s32 spi_bulkwrite(HANDLE handle, u8 devid,
		u16 addr, u8 command, u8 *data, u16 length)
{
	int i;
	s32 res;

	tx_data[0] = addr & 0xff;
	tx_data[1] = (addr >> 8) & 0xff;
	tx_data[2] = (command & 0xf0) | (devid & 0x00ff);
	tx_data[3] = length & 0xff;

	for (i = 0 ; i < length ; i++)
		tx_data[4+i] = data[i];

	res = fci_spi_write_then_read(fc8350_spi
		, &tx_data[0], length+4, NULL, 0);

	if (res) {
		print_log(0, "fc8350_spi_bulkwrite fail : %d\n", res);
		return BBM_NOK;
	}

	return BBM_OK;
}

static s32 spi_dataread(HANDLE handle, u8 devid,
		u16 addr, u8 command, u8 *data, u32 length)
{
	int res;
	unsigned char rxdiv;
	if (length & 0x80000000)
		rxdiv = 0x20;
	else if (length & 0x40000000)
		rxdiv = 0x00;
	else
		rxdiv = 0x30;
	length = length & 0x0ffff;
	tx_data[0] = addr & 0xff;
	tx_data[1] = (addr >> 8) & 0xff;
	tx_data[2] = (command & 0xc0) | rxdiv | (devid & 0x000f);
	tx_data[3] = length & 0xff;

	res = fci_spi_write_then_read(fc8350_spi
		, &tx_data[0], 4, data, length);

	if (res) {
		print_log(0, "fc8350_spi_dataread fail : %d\n", res);
		return BBM_NOK;
	}

	return res;
}


s32 fc8350_spi_init(HANDLE handle, u16 param1, u16 param2)
{
	int res = 0;
	if (wdata_buf == NULL) {
		wdata_buf = kmalloc(128, GFP_DMA | GFP_KERNEL);
		if (!wdata_buf) {
			print_log(0, "spi wdata_buf kmalloc fail\n");
			return BBM_NOK;
		}
	}
	if (rdata_buf == NULL) {
		rdata_buf = kmalloc(64 * 1024, GFP_DMA | GFP_KERNEL);
		if (!rdata_buf) {
			print_log(0, "spi rdata_buf kmalloc fail\n");
			return BBM_NOK;
		}
	}
	res = spi_register_driver(&fc8350_spi_driver);
	if (res) {
		print_log(0, "fc8350_spi register fail : %d\n", res);
		return BBM_NOK;
	}
	return res;
}

s32 fc8350_spi_byteread(HANDLE handle, DEVICEID devid, u16 addr, u8 *data)
{
	s32 res;
	u8 command = SPI_READ;

	mutex_lock(&fci_spi_lock);
	res = spi_bulkread(handle, (u8) (devid & 0x000f), addr, command,
				data, 1);
	mutex_unlock(&fci_spi_lock);
	return res;
}

s32 fc8350_spi_wordread(HANDLE handle, DEVICEID devid, u16 addr, u16 *data)
{
	s32 res;
	u8 command = SPI_READ | SPI_AINC;

	mutex_lock(&fci_spi_lock);
	res = spi_bulkread(handle, (u8) (devid & 0x000f), addr, command,
				(u8 *)data, 2);
	mutex_unlock(&fci_spi_lock);

	return res;
}

s32 fc8350_spi_longread(HANDLE handle, DEVICEID devid, u16 addr, u32 *data)
{
	s32 res;
	u8 command = SPI_READ | SPI_AINC;

	mutex_lock(&fci_spi_lock);
	res = spi_bulkread(handle, (u8) (devid & 0x000f), addr, command,
				(u8 *)data, 4);
	mutex_unlock(&fci_spi_lock);

	return res;
}

s32 fc8350_spi_bulkread(HANDLE handle, DEVICEID devid,
		u16 addr, u8 *data, u16 length)
{
	s32 res;
	u8 command = SPI_READ | SPI_AINC;

	mutex_lock(&fci_spi_lock);
	res = spi_bulkread(handle, (u8) (devid & 0x000f), addr, command,
				data, length);
	mutex_unlock(&fci_spi_lock);

	return res;
}

s32 fc8350_spi_bytewrite(HANDLE handle, DEVICEID devid, u16 addr, u8 data)
{
	s32 res;
	u8 command = SPI_WRITE;


	mutex_lock(&fci_spi_lock);
	if (addr == BBM_DM_DATA) {
		u8 ifcommand = data;
		u16 ifaddr = data;
		u8 ifdata = data;
		res = spi_bulkwrite(handle, (u8) (devid & 0x000f), ifaddr
			, ifcommand, &ifdata, 1);
	} else
	res = spi_bulkwrite(handle, (u8) (devid & 0x000f), addr, command,
				(u8 *)&data, 1);
	mutex_unlock(&fci_spi_lock);

	return res;
}

s32 fc8350_spi_wordwrite(HANDLE handle, DEVICEID devid, u16 addr, u16 data)
{
	s32 res;
	u8 command = SPI_WRITE | SPI_AINC;

	mutex_lock(&fci_spi_lock);
	res = spi_bulkwrite(handle, (u8) (devid & 0x000f), addr, command,
				(u8 *)&data, 2);
	mutex_unlock(&fci_spi_lock);

	return res;
}

s32 fc8350_spi_longwrite(HANDLE handle, DEVICEID devid, u16 addr, u32 data)
{
	s32 res;
	u8 command = SPI_WRITE | SPI_AINC;

	mutex_lock(&fci_spi_lock);
	res = spi_bulkwrite(handle, (u8) (devid & 0x000f), addr, command,
				(u8 *) &data, 4);
	mutex_unlock(&fci_spi_lock);

	return res;
}

s32 fc8350_spi_bulkwrite(HANDLE handle, DEVICEID devid,
		u16 addr, u8 *data, u16 length)
{
	s32 res;
	u8 command = SPI_WRITE | SPI_AINC;

	mutex_lock(&fci_spi_lock);
	res = spi_bulkwrite(handle, (u8) (devid & 0x000f), addr, command,
				data, length);
	mutex_unlock(&fci_spi_lock);

	return res;
}

s32 fc8350_spi_dataread(HANDLE handle, DEVICEID devid,
		u16 addr, u8 *data, u32 length)
{
	s32 res;
	u8 command = SPI_READ | SPI_THR;

	mutex_lock(&fci_spi_lock);
	res = spi_dataread(handle, (u8) (devid & 0x000f), addr, command,
				data, length);
	mutex_unlock(&fci_spi_lock);

	return res;
}

s32 fc8350_spi_deinit(HANDLE handle)
{
	spi_unregister_driver(&fc8350_spi_driver);
	return BBM_OK;
}

