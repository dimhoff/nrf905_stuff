/**
 * dht_wattcher.c - Read DHT22 sensor and send value to Wattcher Display
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
#define WATTCHER_VIRT_ADDR 0x5c27fe22

#define PIN_PWR	(22)
#define PIN_CE	(23)
#define PIN_TXEN (27)
#define PIN_DR (25)
#define PIN_AM (7)
#define PIN_CD (24)
#define SPI_CS	(BCM2835_SPI_CS0)
#define PIN_DHT (4)

#define DHT11 11
#define DHT22 22
#define AM2302 22

#define MAX_TRY 5
#define MAXTIMINGS 100

int readDHT(int type, int pin, int *temp, int *hum)
{
	int counter = 0;
	int laststate = HIGH;
	int state = HIGH;
	int j=0;
	int bits[250];
	int data[100];
	int bitidx = 0;

	// Set GPIO pin to output
	bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);

	bcm2835_gpio_write(pin, HIGH);
	usleep(500000); // 500 ms
	bcm2835_gpio_write(pin, LOW);
	usleep(20000);

	bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);

	// wait for pin to drop?
	while (bcm2835_gpio_lev(pin) == 1) {
		usleep(1);
	}

	// read data!
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	for (int i=0; i< MAXTIMINGS; i++) {
		for (counter=0; counter < 1000; counter++) {
			state = bcm2835_gpio_lev(pin);
			if (state != laststate) {
				break;
			}
			//nanosleep(1);		// overclocking might change this?
		}
		if (counter == 1000) break;
		laststate = state;
		bits[bitidx++] = counter;

		if ((i>3) && (i%2 == 0)) {
			// shove each bit into the storage bytes
			data[j/8] <<= 1;
			if (counter > 200)
				data[j/8] |= 1;
			j++;
		}
	}

	if ((j >= 39) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
		// yay!
		if (type == DHT11) {
			*temp = data[2] * 10;
			*hum = data[0] * 10;
		} else if (type == DHT22) {
			*hum = data[0] * 256 + data[1];

			*temp = (data[2] & 0x7F) * 256 + data[3];
			if (data[2] & 0x80) *temp *= -1;
		}
		return 0;
	}

	return -1;
}

int main(int argc, char **argv)
{
	int temp, hum;
	int read_temp=1;
	int running=1;
	int i;
	int err;
	nrf905_t nrf;
	uint8_t buf[16] = {0x12, 0x34, 0x56, 0x78, 0xaa, 0x00, 0xab, 0x00, 0x00, 0x75, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00};

	for (i=0 ; i < 4; i++) {
		buf[i] = WATTCHER_VIRT_ADDR >> (24 - (i * 8)) & 0xff;
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

	// Main loop
	while (running) {
		int value;
		for (i=0; i<MAX_TRY; i++) {
			if (readDHT(AM2302, PIN_DHT, &temp, &hum) == 0) {
				printf("temp: %f, hum: %f\n", temp / 10.0, hum / 10.0);
				if (read_temp) {
					value = temp;
				} else {
					value = hum;
				}
				break;
			}
			sleep(2);
		}

		if (i != MAX_TRY) {
			printf("sending value: %d\n", value);
			buf[11] = (value >> 8) & 0xff;
			buf[12] = value & 0xff;
			struct timespec duration = { 0, 20000000 };
			err = nrf905_send_to_for(&nrf, WATTCHER_ADDR, buf, 16, &duration);
			//err = nrf905_send_to(&nrf, WATTCHER_ADDR, buf, 16);
			if (err != 0) {
				fprintf(stderr, "Failed to send data\n");
			}
		} else {
			fprintf(stderr, "Failed to read sensor\n");
		}

		sleep(60);
	}

	nrf905_destroy(&nrf);
	return 0;
}
