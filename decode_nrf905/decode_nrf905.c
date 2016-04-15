/**
 * decode_nrf905.c - Find and decode NRF905 frames from a demodulated GFSK stream
 *
 * The input is expected to be the output of nrf905_demod.py.
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
#include <stdint.h>
#include <string.h>

#include "lib_crc.h"

#define NRF905_MAX_FRAME_LEN (4+32+2)

const uint32_t PREAMBLE_ENCODED = 0xAAA66; // apparantly bits are inverted...
const uint16_t PREAMBLE = 0x3F5;
const int max_encoding_errors = 0;

struct frame_decoder_state_t {
	int prev_sample;
	uint8_t byte_val;

	int encoding_errors;
	int sample_cnt;

	int bit_cnt;
	int byte_cnt;
	uint8_t data[NRF905_MAX_FRAME_LEN];
};

void frame_clear_state(struct frame_decoder_state_t *frame)
{
	memset(frame, 0, sizeof(struct frame_decoder_state_t));
}

void frame_finish(struct frame_decoder_state_t *frame)
{
	int i;
        unsigned short crc16 = 0xffff;

	if (frame->byte_cnt == 0)
		return;

	for (i=0; i < frame->byte_cnt; i++) {
		printf("%.2x ", frame->data[i]);
		crc16 = update_crc_ccitt(crc16, frame->data[i]);
	}

	
	// Check CRC-8
	if (frame->byte_cnt >= 3) {
//TODO: check CRC-8
	}
	// Check CRC-16
	if (frame->byte_cnt >= 4) {
		if (crc16 == 0) {
			printf("(CRC-16 OK) ");
		}
	}
	putchar('\n');
}


int main(int argc, char *argv[])
{
	int i, j;
	unsigned int sample_cnt=0;

	int in_sync=0;
	uint32_t preamble=0;

	struct frame_decoder_state_t frame;

	size_t len=0;
	unsigned char buf[1024];

	while ((len = fread(buf, 1, sizeof(buf), stdin)) > 0) {
		for (i=0; i < len; i++) {
			sample_cnt++;

			if (!in_sync) {
				preamble = ((preamble << 1) & 0xFFFFF) | (buf[i] ? 0 : 1);
				if (preamble == PREAMBLE_ENCODED) {
					// Preamble detected
					frame_clear_state(&frame);

					in_sync = 1;
				}
			} else {
				if ((frame.sample_cnt++ & 1) == 0) {
					frame.prev_sample = buf[i];
				} else {
					if (buf[i] == frame.prev_sample) {
						// Manchester decoding error
						if (++frame.encoding_errors > max_encoding_errors) {
							frame_finish(&frame);
							in_sync = 0;
							continue;
						}
					}

					frame.byte_val = frame.byte_val << 1;
					if (frame.prev_sample) {
						frame.byte_val |= 0;
					} else {
						frame.byte_val |= 1;
					}
					frame.bit_cnt++;
					if ((frame.bit_cnt % 8) == 2 && frame.byte_cnt > 1) {
						if (frame.data[frame.byte_cnt-1] == (PREAMBLE >> 2) &&
						    (frame.byte_val & 0x3) == (PREAMBLE & 0x3))
						{
							// Preamble detected in frame, possibly auto-retransmit mode
							frame.byte_cnt--;
							frame_finish(&frame);
							frame_clear_state(&frame);
						}
					}
					if ((frame.bit_cnt % 8) == 0) {
						frame.data[frame.byte_cnt++] = frame.byte_val;
						if (frame.byte_cnt == NRF905_MAX_FRAME_LEN) {
							// Frame at max. length
							frame_finish(&frame);
							in_sync = 0;
						}
					}
				}
			}
		}
	}

	return 0;
}
