/**
 * nrf905_send.c - Nordic nRF905 RF module data sender
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
#include <string.h>
#include <time.h>

#include "nrf905.h"
#include "bcm2835.h"

#define PIN_PWR	(22)
#define PIN_CE	(23)
#define PIN_TXEN (27)
#define PIN_DR (25)
#define PIN_AM (7)
#define PIN_CD (24)
#define SPI_CS	(BCM2835_SPI_CS0)

int dehex_nibble(int n)
{
        int x;
        x = n - 0x30;
        if (x > 9)
                x -= 0x07;
        if (x > 15)
                x -= 0x20;

        return x;
}

int dehexify(const char *in, size_t bytes, unsigned char *out)
{
        int i;
        int x;

        if (strlen(in) < bytes*2) {
                return -1;
        }

        memset(out, 0, bytes);
        for (i=0; i<bytes; i++) {
                x = dehex_nibble(in[(i*2)]);
                if (x < 0 || x > 15)
                        return -1;
                out[i] = (x << 4);

                x = dehex_nibble(in[(i*2)+1]);
                if (x < 0 || x > 15)
                        return -1;
                out[i] |= x;
        }

        return 0;
}

int main(int argc, const char *argv[])
{
	nrf905_t nrf;
	int err;
	uint32_t addr = 0x11223344;
	int hex_str_len;
	int len;
	uint8_t buf[32];

//TODO: options for setting frequency, etc.
	// Parse arguments
	if (argc != 3) {
		printf("usage...\n");
		exit(EXIT_FAILURE);
	}

	addr = strtoll(argv[1], NULL, 16);

	hex_str_len = strlen(argv[2]);
	if (hex_str_len > 64) {
		fprintf(stderr, "Data is too long\n");
		exit(EXIT_FAILURE);
	}
	if (hex_str_len < 2) {
		fprintf(stderr, "Data is too short\n");
		exit(EXIT_FAILURE);
	}
	if (hex_str_len & 1) {
		fprintf(stderr, "Uneven data length\n");
		exit(EXIT_FAILURE);
	}
	len = hex_str_len / 2;

	err = dehexify(argv[2], len, buf);
	if (err != 0) {
		fprintf(stderr, "Data contains non hexadecimal characters\n");
		exit(EXIT_FAILURE);
	}

	// Configure module
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

	err = nrf905_set_rx_addr(&nrf, addr);
	if (err != 0) {
		fprintf(stderr, "Failed to set crystal frequency\n");
		exit(EXIT_FAILURE);
	}

	err = nrf905_set_pw(&nrf, len);
	if (err != 0) {
		fprintf(stderr, "Failed to set payload width\n");
		exit(EXIT_FAILURE);
	}

	err = nrf905_write_config(&nrf);
	if (err != 0) {
		fprintf(stderr, "Failed to write config\n");
		exit(EXIT_FAILURE);
	}

        struct timespec duration = { 0, 10000000 };
	err = nrf905_send_to_for(&nrf, addr, buf, len, &duration);
	//err = nrf905_send_to(&nrf, addr, buf, len);
	if (err != 0) {
		fprintf(stderr, "Failed to send data\n");
		exit(EXIT_FAILURE);
	}

	nrf905_destroy(&nrf);
	return 0;
}
