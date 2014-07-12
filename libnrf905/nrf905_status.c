/**
 * nrf905_status.c - Dump status from nRF905 module
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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#include "nrf905.h"
#include "bcm2835.h"

#define PIN_PWR	(22)
#define PIN_CE	(23)
#define PIN_TXEN (27)
#define PIN_DR (25)
#define PIN_AM (7)
#define PIN_CD (24)
#define SPI_CS	(BCM2835_SPI_CS0)

#define bool_to_str(X) ((X == true) ? "True" : "False")

static const char * const pa_pwr_strings[4] = {
		"-10dBm",
		"-2dBm",
		"+6dBm",
		"+10dBm"
	};

const char *pa_pwr_to_str(uint8_t pa_pwr)
{
	assert(pa_pwr < 4);
	return pa_pwr_strings[pa_pwr];
}

static const char * const up_clk_freq_strings[5] = {
		"4MHz",
		"2MHz",
		"1MHz",
		"500kHz",
		"Disabled"
	};

const char *up_clk_freq_to_str(bool up_clk_en, uint8_t up_clk_freq)
{
	assert(up_clk_freq < 4);
	if (!up_clk_en) {
		return up_clk_freq_strings[4];
	}
	return up_clk_freq_strings[up_clk_freq];
}

static const char * const xof_strings[6] = {
		"4MHz",
		"8MHz",
		"12MHz",
		"16MHz",
		"20MHz",
		"Invalid"
	};

const char *xof_to_str(uint8_t xof)
{
	if (xof > 5) {
		xof = 5;
	}
	return xof_strings[xof];
}

static const char * const crc_strings[3] = {
		"CRC-8",
		"CRC-16",
		"Disabled"
	};

const char *crc_to_str(bool crc_en, uint8_t crc_mode)
{
	assert(crc_mode < 2);
	if (!crc_en) {
		return crc_strings[2];
	}
	return crc_strings[crc_mode];
}

int main(int argc, const char *argv[])
{
	nrf905_t nrf;
	int err;

	err = nrf905_init(&nrf, PIN_PWR, PIN_CE, PIN_TXEN, PIN_DR, SPI_CS);
	if (err != 0) {
		fprintf(stderr, "Failed to initialize NRF905, Do you have root permissions?\n");
		exit(EXIT_FAILURE);
	}

	err = nrf905_read_config(&nrf);
	if (err != 0) {
		fprintf(stderr, "Failed to write config\n");
		exit(EXIT_FAILURE);
	}

	printf("Frequency: %.1f\n", nrf905_get_freq(&nrf)/1000000.0);
	printf("Power Amplifier Level: %s\n", pa_pwr_to_str(nrf905_get_pa_pwr(&nrf)));
	printf("Receiver Reduced Power: %s\n", bool_to_str(nrf905_get_rx_red_pwr(&nrf)));
	printf("Receive Address Width: %hhu\n", nrf905_get_rx_afw(&nrf));
	printf("Transmit Address Width: %hhu\n", nrf905_get_tx_afw(&nrf));
	printf("Receive Payload Width: %hhu\n", nrf905_get_rx_pw(&nrf));
	printf("Transmit Payload Width: %hhu\n", nrf905_get_tx_pw(&nrf));
	printf("Receive Address: 0x%.*x\n", nrf905_get_rx_afw(&nrf) * 2, nrf905_get_rx_addr(&nrf));
	printf("CRC Mode: %s\n", crc_to_str(nrf905_get_crc_en(&nrf), nrf905_get_crc_mode(&nrf)));
	printf("Up Clock: %s\n", up_clk_freq_to_str(nrf905_get_up_clk_en(&nrf), nrf905_get_up_clk_freq(&nrf)));
	printf("Crystal Frequence(XOF): %s\n", xof_to_str(nrf905_get_xof(&nrf)));

	nrf905_destroy(&nrf);
	return 0;
}
