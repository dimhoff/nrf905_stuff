/**
 * wattcher_pair.c - Program to pair wattcher display with virtual address
 *
 * Copyright (c) 2014, David Imhoff <dimhoff.devel@gmail.com>
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
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <bcm2835.h>
#include <unistd.h>

#include <nrf905.h>

#define WATTCHER_ADDR 0xaa61cc16
#define WATTCHER_PAIR_ADDR 0xc12cc21c

#define PIN_PWR	(22)
#define PIN_CE	(23)
#define PIN_TXEN (27)
#define PIN_DR (25)
#define PIN_AM (7)
#define PIN_CD (24)
#define SPI_CS	(BCM2835_SPI_CS0)

int main(int argc, char **argv)
{
	int i;
	int err;
	nrf905_t nrf;
	uint8_t buf[16] = { 0xff, 0xff, 0xff, 0xff, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint32_t virt_addr = 0x5c27fe22;

	for (i=0 ; i < 4; i++) {
		buf[i] = virt_addr >> (24 - (i * 8)) & 0xff;
	}

	if (!bcm2835_init()) {
		return 1;
	}

	// Configure NRF905 module
	err = nrf905_init(&nrf, PIN_PWR, PIN_CE, PIN_TXEN, PIN_DR, SPI_CS);
	if (err != 0) {
		fprintf(stderr, "Failed to initialize NRF905, Do you have root permissions?\n");
		exit(EXIT_FAILURE);
	}

	err = nrf905_set_xof(&nrf, NRF905_XOF_16MHZ);
	if (err != 0) {
		fprintf(stderr, "Failed to set crystal frequency\n");
		exit(EXIT_FAILURE);
	}

	err = nrf905_set_freq(&nrf, 868400000);
	if (err != 0) {
		fprintf(stderr, "Failed to set carrier frequency\n");
		exit(EXIT_FAILURE);
	}

	err = nrf905_set_tx_pw(&nrf, 16);
	if (err != 0) {
		fprintf(stderr, "Failed to set payload width\n");
		exit(EXIT_FAILURE);
	}

	err = nrf905_set_pa_pwr(&nrf, NRF905_PA_PWR_10);
	if (err != 0) {
		fprintf(stderr, "Failed to set TX power\n");
		exit(EXIT_FAILURE);
	}

	err = nrf905_write_config(&nrf);
	if (err != 0) {
		fprintf(stderr, "Failed to write config\n");
		exit(EXIT_FAILURE);
	}

	// Send
	struct timespec duration = { 0, 20000000 };

	printf("Pairing Wattcher display to virtual address: 0x%.8x\n", virt_addr);
	err = nrf905_send_to_for(&nrf, WATTCHER_PAIR_ADDR, buf, 16, &duration);
	//err = nrf905_send_to(&nrf, WATTCHER_PAIR_ADDR, buf, 16);
	if (err != 0) {
		fprintf(stderr, "Failed to send data\n");
	}

	nrf905_destroy(&nrf);
	return 0;
}
