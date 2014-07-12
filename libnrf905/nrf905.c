/**
 * nrf905.c - Nordic nRF905 RF module library
 *
 * Copyright (c) 2014, David Imhoff <dimhoff_devel@xs4all.nl>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the author nor the names of its contributors may
 *       be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <bcm2835.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#include "nrf905.h"

int nrf905_init(nrf905_t *nrf, uint8_t pin_pwr, uint8_t pin_ce,
		uint8_t pin_txen, uint8_t pin_dr, uint8_t spi_cs)
{
	nrf->pin_pwr	= pin_pwr;
	nrf->pin_ce	= pin_ce;
	nrf->pin_txen	= pin_txen;
	nrf->pin_dr	= pin_dr;
	nrf->spi_cs	= spi_cs;

	nrf->status = 0;
	nrf->recv_enabled = false;

	// Defaults
	nrf->ch_no	 = 108;
	nrf->hfreq_pll	 = false;
	nrf->pa_pwr	 = NRF905_PA_PWR_MIN10;
	nrf->rx_red_pwr	 = false;
	nrf->auto_retran = false;
	nrf->rx_afw	 = 4;
	nrf->tx_afw	 = 4;
	nrf->rx_pw	 = 32;
	nrf->tx_pw	 = 32;
	nrf->rx_addr	 = 0xE7E7E7E7;
	nrf->up_clk_freq = NRF905_UP_CLK_FREQ_500KHZ;
	nrf->up_clk_en	 = true;
	nrf->xof	 = NRF905_XOF_20MHZ;
	nrf->crc_en	 = true;
	nrf->crc_mode	 = NRF905_CRC_MODE_CRC16;

	// Init GPIO
	if (!bcm2835_init()) { //TODO: can this be called multiple times? else maybee better to call outside our function...
		return -1;
	}

	bcm2835_spi_begin();
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
	//TODO: maybee use a higher speed?
	// bit rate: 250Mhz / cdev
	bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_65536);
	bcm2835_spi_chipSelect(nrf->spi_cs);
	bcm2835_spi_setChipSelectPolarity(nrf->spi_cs, LOW);

	bcm2835_gpio_fsel(nrf->pin_ce, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(nrf->pin_ce, LOW);
	bcm2835_gpio_fsel(nrf->pin_txen, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(nrf->pin_txen, LOW);

	if (nrf->pin_pwr != NRF905_PIN_NC) {
		bcm2835_gpio_fsel(nrf->pin_pwr, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_write(nrf->pin_pwr, HIGH);
	}

	return 0;
}

void nrf905_destroy(nrf905_t *nrf)
{
	bcm2835_spi_end();
	bcm2835_close(); //TODO: should we do this or the caller?
}

int nrf905_read_config(nrf905_t *nrf)
{
	uint8_t transfer_buf[11] = {0x10, 0x00};

	bcm2835_spi_transfern((char *) transfer_buf, sizeof(transfer_buf));

	nrf->status = transfer_buf[0];
	//TODO: detect incorrect results?

	nrf->ch_no	 = (((uint16_t) transfer_buf[2] & 1) << 8) | transfer_buf[1];
	nrf->hfreq_pll	 = (transfer_buf[2] >> 1) & 0x01;
	nrf->pa_pwr	 = (transfer_buf[2] >> 2) & 0x03;
	nrf->rx_red_pwr	 = (transfer_buf[2] >> 4) & 0x01;
	nrf->auto_retran = (transfer_buf[2] >> 5) & 0x01;
	nrf->rx_afw	 = transfer_buf[3] & 0x07;
	nrf->tx_afw	 = (transfer_buf[3] >> 4) & 0x07;
	nrf->rx_pw	 = transfer_buf[4] & 0x3F;
	nrf->tx_pw	 = transfer_buf[5] & 0x3F;
	nrf->rx_addr	 = (transfer_buf[9] << 24) | (transfer_buf[8] << 16) |
				(transfer_buf[7] << 8) | transfer_buf[6];
	nrf->up_clk_freq = transfer_buf[10] & 0x03;
	nrf->up_clk_en	 = (transfer_buf[10] >> 2) & 0x01;
	nrf->xof	 = (transfer_buf[10] >> 3) & 0x07;
	nrf->crc_en	 = (transfer_buf[10] >> 6) & 0x01;
	nrf->crc_mode	 = (transfer_buf[10] >> 7) & 0x01;

	return 0;
}

int nrf905_write_config(nrf905_t *nrf)
{
	uint8_t transfer_buf[11] = {0x00, 0x00};

	transfer_buf[1]  = (nrf->ch_no & 0xff);
	transfer_buf[2]  = (nrf->ch_no >> 8) & 0x1;
	transfer_buf[2] |= (nrf->hfreq_pll & 0x1) << 1;
	transfer_buf[2] |= (nrf->pa_pwr & 0x3) << 2;
	transfer_buf[2] |= (nrf->rx_red_pwr & 0x1) << 4;
	transfer_buf[2] |= (nrf->auto_retran & 0x1) << 5;
	transfer_buf[3]  = (nrf->rx_afw & 0x7);
	transfer_buf[3] |= (nrf->tx_afw & 0x7) << 4;
	transfer_buf[4]  = (nrf->rx_pw & 0x3F);
	transfer_buf[5]  = (nrf->tx_pw & 0x3F);
	transfer_buf[6]  =  nrf->rx_addr & 0xff;
	transfer_buf[7]  = (nrf->rx_addr >> 8) & 0xff;
	transfer_buf[8]  = (nrf->rx_addr >> 16) & 0xff;
	transfer_buf[9]  = (nrf->rx_addr >> 24) & 0xff;
	transfer_buf[10]  = (nrf->up_clk_freq & 0x3);
	transfer_buf[10] |= (nrf->up_clk_en & 0x1) << 2;
	transfer_buf[10] |= (nrf->xof & 0x7) << 3;
	transfer_buf[10] |= (nrf->crc_en & 0x1) << 6;
	transfer_buf[10] |= (nrf->crc_mode & 0x1) << 7;

	bcm2835_spi_transfern((char *) transfer_buf, sizeof(transfer_buf));

	nrf->status = transfer_buf[0];
	//TODO: detect incorrect results?

	return 0;
}

uint32_t nrf905_get_freq(nrf905_t *nrf)
{
	uint32_t freq;

	freq =  422400000 + nrf->ch_no * 100000;
	if (nrf->hfreq_pll) {
		freq = freq * 2;
	}

	return freq;
}

int nrf905_set_freq(nrf905_t *nrf, uint32_t freq)
{
	bool hfreq_pll = false;
	int ch_no;

	if (freq > 473500000) {
		hfreq_pll = true; // Change hfreq_pll after error checking
		freq = freq / 2;
	}

	if (freq < 422400000 || freq > 473500000)
	{
		errno = EINVAL;
		return -1;
	}

	freq -= 422400000;
	ch_no = freq / 100000;
	if (freq % 100000 >= 50000) {
		ch_no++;
	}

	nrf->hfreq_pll = hfreq_pll;
	nrf->ch_no = ch_no;

	return 0;
}

uint8_t nrf905_get_pa_pwr(nrf905_t *nrf)
{
	return nrf->pa_pwr;
}

bool nrf905_get_rx_red_pwr(nrf905_t *nrf)
{
	return nrf->rx_red_pwr;
}

uint8_t nrf905_get_rx_afw(nrf905_t *nrf)
{
	return nrf->rx_afw;
}

uint8_t nrf905_get_tx_afw(nrf905_t *nrf)
{
	return nrf->tx_afw;
}

uint8_t nrf905_get_rx_pw(nrf905_t *nrf)
{
	return nrf->rx_pw;
}

uint8_t nrf905_get_tx_pw(nrf905_t *nrf)
{
	return nrf->tx_pw;
}

uint32_t nrf905_get_rx_addr(nrf905_t *nrf)
{
	return nrf->rx_addr;
}

uint8_t nrf905_get_up_clk_freq(nrf905_t *nrf)
{
	return nrf->up_clk_freq;
}

bool nrf905_get_up_clk_en(nrf905_t *nrf)
{
	return nrf->up_clk_en;
}

uint8_t nrf905_get_xof(nrf905_t *nrf)
{
	return nrf->xof;
}

bool nrf905_get_crc_en(nrf905_t *nrf)
{
	return nrf->crc_en;
}

uint8_t nrf905_get_crc_mode(nrf905_t *nrf)
{
	return nrf->crc_mode;
}

int nrf905_set_pa_pwr(nrf905_t *nrf, uint8_t pa_pwr)
{
	if (pa_pwr & ~0x3) {
		errno = EINVAL;
		return -1;
	}

	nrf->pa_pwr = pa_pwr;
	return 0;
}

int nrf905_set_rx_red_pwr(nrf905_t *nrf, bool rx_red_pwr)
{
	nrf->rx_red_pwr = rx_red_pwr;
	return 0;
}

int nrf905_set_rx_afw(nrf905_t *nrf, uint8_t rx_afw)
{
	if (rx_afw < 1 && rx_afw > 4) {
		errno = EINVAL;
		return -1;
	}

	nrf->rx_afw = rx_afw;
	return 0;
}

int nrf905_set_tx_afw(nrf905_t *nrf, uint8_t tx_afw)
{
	if (tx_afw < 1 && tx_afw > 4) {
		errno = EINVAL;
		return -1;
	}

	nrf->tx_afw = tx_afw;
	return 0;
}

int nrf905_set_afw(nrf905_t *nrf, uint8_t afw)
{
	int retval = 0;

	retval = nrf905_set_rx_afw(nrf, afw);
	if (retval == 0) {
		retval = nrf905_set_tx_afw(nrf, afw);
	}

	return retval;
}

int nrf905_set_rx_pw(nrf905_t *nrf, uint8_t rx_pw)
{
	if (rx_pw > 32) {
		errno = EINVAL;
		return -1;
	}

	nrf->rx_pw = rx_pw;
	return 0;
}

int nrf905_set_tx_pw(nrf905_t *nrf, uint8_t tx_pw)
{
	if (tx_pw > 32) {
		errno = EINVAL;
		return -1;
	}

	nrf->tx_pw = tx_pw;
	return 0;
}

int nrf905_set_pw(nrf905_t *nrf, uint8_t pw)
{
	int retval = 0;

	retval = nrf905_set_rx_pw(nrf, pw);
	if (retval == 0) {
		retval = nrf905_set_tx_pw(nrf, pw);
	}

	return retval;
}

int nrf905_set_rx_addr(nrf905_t *nrf, uint32_t rx_addr)
{
//TODO: rx_afw == 1: then which byte is used?
	nrf->rx_addr = rx_addr;
	return 0;
}

int nrf905_set_up_clk_freq(nrf905_t *nrf, uint8_t up_clk_freq)
{
	if (up_clk_freq & ~0x3) {
		errno = EINVAL;
		return -1;
	}

	nrf->up_clk_freq = up_clk_freq;
	return 0;
}

int nrf905_set_up_clk_en(nrf905_t *nrf, bool up_clk_en)
{
	nrf->up_clk_en = up_clk_en;
	return 0;
}

int nrf905_set_xof(nrf905_t *nrf, uint8_t xof)
{
	if (xof > 4) {
		errno = EINVAL;
		return -1;
	}

	nrf->xof = xof;
	return 0;
}

int nrf905_set_crc_en(nrf905_t *nrf, bool crc_en)
{
	nrf->crc_en = crc_en;
	return 0;
}

int nrf905_set_crc_mode(nrf905_t *nrf, uint8_t crc_mode)
{
	if (crc_mode & ~0x1) {
		errno = EINVAL;
		return -1;
	}

	nrf->crc_mode = crc_mode;
	return 0;
}

int nrf905_write_tx_addr(nrf905_t *nrf, uint32_t addr)
{
	uint8_t transfer_buf[5] = { 0x22, 0 };

	transfer_buf[1]  =  addr & 0xff;
	transfer_buf[2]  = (addr >> 8) & 0xff;
	transfer_buf[3]  = (addr >> 16) & 0xff;
	transfer_buf[4]  = (addr >> 24) & 0xff;

	bcm2835_spi_transfern((char *) transfer_buf, sizeof(transfer_buf));

	nrf->status = transfer_buf[0];
	//TODO: detect incorrect results?

	return 0;
}

/**
 * Start sending data
 */
static int _nrf905_start_send(nrf905_t *nrf, const void *data, size_t len,
		bool auto_retran)
{
	uint8_t transfer_buf[33] = { 0x20, 0 };
	int err;

	if (len > nrf->tx_pw) {
		errno = EINVAL;
		return -1;
	}
	assert(len <= 32);

	if (nrf->auto_retran != auto_retran) {
		nrf->auto_retran = auto_retran;
		//TODO: write only 2th byte of config
		err = nrf905_write_config(nrf);
		if (err != 0) {
			return -1;
		}
	}

	memcpy(transfer_buf + 1, data, len);

	bcm2835_gpio_write(nrf->pin_ce, LOW);
	bcm2835_gpio_write(nrf->pin_txen, HIGH);

	bcm2835_spi_transfern((char *) transfer_buf, 1 + nrf->tx_pw);

	nrf->status = transfer_buf[0];
	//TODO: detect incorrect results?

	bcm2835_gpio_write(nrf->pin_ce, HIGH);

	return 0;
}

int nrf905_send(nrf905_t *nrf, const void *data, size_t len)
{
	int err = 0;

	err = _nrf905_start_send(nrf, data, len, false);
	if (err != 0) {
		return err;
	}

//TODO: wait DR, either through pin or through spi... or just time based?

	bcm2835_gpio_write(nrf->pin_ce, LOW);
	bcm2835_gpio_write(nrf->pin_txen, LOW);

	return 0;
}

int nrf905_send_to(nrf905_t *nrf, uint32_t addr, const void *data, size_t len)
{
	int retval = 0;

	retval = nrf905_write_tx_addr(nrf, addr);
	if (retval == 0) {
		retval = nrf905_send(nrf, data, len);
	}

	return retval;
}

int nrf905_send_for(nrf905_t *nrf, const void *data, size_t len,
			const struct timespec *duration)
{
	struct timespec ts;
	int err;
	int retval = 0;

	err = _nrf905_start_send(nrf, data, len, true);
	if (err != 0) {
		return -1;
	}

	ts = *duration;
	do {
		err = nanosleep(&ts, &ts);
		if (err != 0 && errno != EINTR) {
			retval = -1;
			break;
		}
	} while (err != 0);
	
	bcm2835_gpio_write(nrf->pin_ce, LOW);
	bcm2835_gpio_write(nrf->pin_txen, LOW);

	return retval;
}

int nrf905_send_to_for(nrf905_t *nrf, uint32_t addr, const void *data, size_t len,
			const struct timespec *duration)
{
	int retval = 0;

	retval = nrf905_write_tx_addr(nrf, addr);
	if (retval == 0) {
		retval = nrf905_send_for(nrf, data, len, duration);
	}

	return retval;
}

int nrf905_recv_enable(nrf905_t *nrf)
{
	bcm2835_gpio_write(nrf->pin_txen, LOW);
	bcm2835_gpio_write(nrf->pin_ce, HIGH);
	nrf->recv_enabled = true;

	return 0;
}

int nrf905_recv_disable(nrf905_t *nrf)
{
	bcm2835_gpio_write(nrf->pin_txen, LOW);
	bcm2835_gpio_write(nrf->pin_ce, LOW);
	nrf->recv_enabled = false;

	return 0;
}

int nrf905_recv(nrf905_t *nrf, void *data, size_t len)
{
	uint8_t transfer_buf[33] = { 0x24, 0 };
	bool old_recv_enabled;
	int err;

	assert(nrf->rx_pw <= 32);

	old_recv_enabled = nrf->recv_enabled;
	if (! old_recv_enabled) {
		err = nrf905_recv_enable(nrf);
		if (err != 0) {
			return -1;
		}
	}

// wait DR
//TODO: in seperate func?
while (bcm2835_gpio_lev(nrf->pin_dr) != HIGH) bcm2835_delayMicroseconds(1000);

	bcm2835_spi_transfern((char *) transfer_buf, 1 + nrf->rx_pw);

	nrf->status = transfer_buf[0];

	if (len < nrf->rx_pw) {
		memcpy(data, &transfer_buf[1], len);
	} else {
		memcpy(data, &transfer_buf[1], nrf->rx_pw);
	}

	if (! old_recv_enabled) {
		err = nrf905_recv_disable(nrf);
		if (err != 0) {
			return -1;
		}
	}

	return 0;
}

int nrf905_recv_nb(nrf905_t *nrf, void *data, size_t len)
{
	uint8_t transfer_buf[33] = { 0x24, 0 };

	assert(nrf->rx_pw <= 32);

// check DR
//TODO: in seperate func?
if (bcm2835_gpio_lev(nrf->pin_dr) != HIGH) { errno = EWOULDBLOCK; return -1; }

	bcm2835_spi_transfern((char *) transfer_buf, 1 + nrf->rx_pw);

	nrf->status = transfer_buf[0];

	if (len < nrf->rx_pw) {
		memcpy(data, &transfer_buf[1], len);
	} else {
		memcpy(data, &transfer_buf[1], nrf->rx_pw);
	}

	return 0;
}


int nrf905_recv_to(nrf905_t *nrf, void *data, size_t len,
			const struct timespec *to)
{
	uint8_t transfer_buf[33] = { 0x24, 0 };
	bool old_recv_enabled;
	int err;

	assert(nrf->rx_pw <= 32);

	old_recv_enabled = nrf->recv_enabled;
	if (! old_recv_enabled) {
		err = nrf905_recv_enable(nrf);
		if (err != 0) {
			return -1;
		}
	}

// wait DR
//TODO: in seperate func?
//TODO: add timeout
while (bcm2835_gpio_lev(nrf->pin_dr) != HIGH) bcm2835_delayMicroseconds(1000);

	bcm2835_spi_transfern((char *) transfer_buf, 1 + nrf->rx_pw);

	nrf->status = transfer_buf[0];

	if (len < nrf->rx_pw) {
		memcpy(data, &transfer_buf[1], len);
	} else {
		memcpy(data, &transfer_buf[1], nrf->rx_pw);
	}

	if (! old_recv_enabled) {
		err = nrf905_recv_disable(nrf);
		if (err != 0) {
			return -1;
		}
	}

	return 0;
}
