/**
 * nrf905.h - Nordic nRF905 RF module library
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

#ifndef __NRF905_H__
#define __NRF905_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Value to indicate a pin isn't used
 */
#define NRF905_PIN_NC (0xFF)

/**
 * Transmit power
 */
enum {
	NRF905_PA_PWR_MIN10 = 0,
	NRF905_PA_PWR_MIN2 = 1,
	NRF905_PA_PWR_6 = 2,
	NRF905_PA_PWR_10 = 3,
};

/**
 * Clock out frequency
 */
enum {
	NRF905_UP_CLK_FREQ_4MHZ = 0,
	NRF905_UP_CLK_FREQ_2MHZ = 1,
	NRF905_UP_CLK_FREQ_1MHZ = 2,
	NRF905_UP_CLK_FREQ_500KHZ = 3,
};

/**
 * Crystal frequency
 */
enum {
	NRF905_XOF_4MHZ	 = 0,
	NRF905_XOF_8MHZ	 = 1,
	NRF905_XOF_12MHZ = 2,
	NRF905_XOF_16MHZ = 3,
	NRF905_XOF_20MHZ = 4,
};

/**
 * CRC mode
 */
enum {
	NRF905_CRC_MODE_CRC8 = 0,
	NRF905_CRC_MODE_CRC16 = 1,
};

/**
 * NRF905 data object structure
 */
typedef struct {
	// Pin mapping
	uint8_t pin_pwr;
	uint8_t pin_ce;
	uint8_t pin_txen;
	uint8_t pin_dr;
	uint8_t spi_cs;

	// status
	uint8_t status;
	bool recv_enabled;

	// config
	uint16_t ch_no;
	bool hfreq_pll;
	uint8_t pa_pwr;
	bool rx_red_pwr;
	bool auto_retran;
	uint8_t rx_afw;
	uint8_t tx_afw;
	uint8_t rx_pw;
	uint8_t tx_pw;
	uint32_t rx_addr;
	uint8_t up_clk_freq;
	bool up_clk_en;
	uint8_t xof;
	bool crc_en;
	uint8_t crc_mode;
} nrf905_t;


/**
 * Initialize a NRF905 object on the given pins
 *
 * @param nrf		NRF905 object to initialize
 * @param pin_pwr	GPIO pin connected to the NRF905 'pwr_up' pin. If pin is
 *			hard wired to Vcc, then use NRF905_PIN_NC.
 * @param pin_ce	GPIO pin connected to the NRF905 'trx_ce' pin.
 * @param pin_txen	GPIO pin connected to the NRF905 'tx_en' pin.
 * @param pin_dr	GPIO pin connected to the NRF905 'dr' pin. If pin is not
 *			connected use NRF905_PIN_NC, in this case the status
 *			register will be polled to get the data ready status.
 * @param spi_cs	SPI Chip Select pin to use
 */
int nrf905_init(nrf905_t *nrf, uint8_t pin_pwr, uint8_t pin_ce,
		uint8_t pin_txen, uint8_t pin_dr, uint8_t spi_cs);

/**
 * Destroy NRF905 object
 */
void nrf905_destroy(nrf905_t *nrf);

/**
 * Read current configuration from device
 *
 * Read current configuration from device into the configuration cache. This
 * function can be used to:
 *  - Get the device running configuration just after init
 *  - Revert changes to the configuration cache
 *
 * @param nrf	NRF905 object to initialize
 */
int nrf905_read_config(nrf905_t *nrf);

/**
 * Apply current configuration to device
 *
 * All configuration changing functions only alter the configuration cache, to
 * prevent excessive SPI transactions. This function should be called to make
 * the new configuration active.
 *
 * @param nrf	NRF905 object to initialize
 */
int nrf905_write_config(nrf905_t *nrf);

/**
 * Get Carrier Frequency
 *
 * Get the carrier frequency from the cached configuration.
 * Note: If no call to nrf905_write_config() or nrf905_read_config() has been
 * made since last changing the frequency, the actual device might be
 * configured differently.
 *
 * @returns	Configured frequency in Hz
 */
uint32_t nrf905_get_freq(nrf905_t *nrf);

/**
 * Set Carrier Frequency
 *
 * Set's the ch_no and hfreq_pll config option to match freq as close as
 * possible. Frequency must be within the range of 422.4-473.5 Mhz or
 * 844.8-947 Mhz.
 * Note: This function does NOT apply the configuration! Call
 * nrf905_write_config() to actually reconfigure the device.
 *
 * @param nrf	object
 * @param freq	Frequency in Hz to tune to
 *
 * @returns	0 on success, -1 and set errno to EINVAL if frequency is
 *		outside of tuneable range
 */
int nrf905_set_freq(nrf905_t *nrf, uint32_t freq);

///@{
/**
 * Configuration set functions
 *
 * These functions set a certain configuration parameter in the config cache.
 * Note: This functions only change the cached configuration! Use
 * nrf905_write_config() to actually reconfigure the device.
 */
int nrf905_set_pa_pwr(nrf905_t *nrf, uint8_t pa_pwr);
int nrf905_set_rx_red_pwr(nrf905_t *nrf, bool rx_red_pwr);
int nrf905_set_rx_afw(nrf905_t *nrf, uint8_t rx_afw);
int nrf905_set_tx_afw(nrf905_t *nrf, uint8_t tx_afw);
int nrf905_set_afw(nrf905_t *nrf, uint8_t afw);
int nrf905_set_rx_pw(nrf905_t *nrf, uint8_t rx_pw);
int nrf905_set_tx_pw(nrf905_t *nrf, uint8_t tx_pw);
int nrf905_set_pw(nrf905_t *nrf, uint8_t pw);
int nrf905_set_rx_addr(nrf905_t *nrf, uint32_t rx_addr);
int nrf905_set_up_clk_freq(nrf905_t *nrf, uint8_t up_clk_freq);
int nrf905_set_up_clk_en(nrf905_t *nrf, bool up_clk_en);
int nrf905_set_xof(nrf905_t *nrf, uint8_t xof);
int nrf905_set_crc_en(nrf905_t *nrf, bool crc_en);
int nrf905_set_crc_mode(nrf905_t *nrf, uint8_t crc_mode);
///@}

///@{
/**
 * Configuration get functions
 *
 * These functions get a certain configuration parameter from the config cache.
 * Note: This functions only read the cached configuration! Use
 * nrf905_read_config() to read the current configuration from the device.
 */
uint8_t nrf905_get_pa_pwr(nrf905_t *nrf);
bool nrf905_get_rx_red_pwr(nrf905_t *nrf);
uint8_t nrf905_get_rx_afw(nrf905_t *nrf);
uint8_t nrf905_get_tx_afw(nrf905_t *nrf);
uint8_t nrf905_get_rx_pw(nrf905_t *nrf);
uint8_t nrf905_get_tx_pw(nrf905_t *nrf);
uint32_t nrf905_get_rx_addr(nrf905_t *nrf);
uint8_t nrf905_get_up_clk_freq(nrf905_t *nrf);
bool nrf905_get_up_clk_en(nrf905_t *nrf);
uint8_t nrf905_get_xof(nrf905_t *nrf);
bool nrf905_get_crc_en(nrf905_t *nrf);
uint8_t nrf905_get_crc_mode(nrf905_t *nrf);
///@}

/**
 * Set TX address register
 *
 * @param nrf	NRF905 object to initialize
 * @param addr	Address of target node
 */
int nrf905_write_tx_addr(nrf905_t *nrf, uint32_t addr);

/**
 * Send data
 *
 * @param nrf	NRF905 object to initialize
 * @param data	Data to send
 * @param len	Length of data. Should be <= TX payload width. If smaller then
 *
 * @returns	0 on success, -1 and set errno to EINVAL if len is greater then
 *		the TX payload width.
 */
int nrf905_send(nrf905_t *nrf, const void *data, size_t len);

/**
 * Send data to a specific TX address
 *
 * This function is just a combination of nrf905_write_tx_addr() and
 * nrf905_send().
 */
int nrf905_send_to(nrf905_t *nrf, uint32_t addr, const void *data, size_t len);

/**
 * Send data for a specific interval
 *
 * Send data using the auto retransmit function during a specific interval.
 *
 * @param nrf	NRF905 object to initialize
 * @param data	Data to send
 * @param len	Length of data. Should be <= TX payload width. If smaller then
 *		the TX payload width, buffer is padded with 0 bytes.
 * @param duration	Interval length during which to send the data.
 *
 * @returns	0 on success, -1 and set errno to EINVAL if len is greater then
 *		the TX payload width.
 */
int nrf905_send_for(nrf905_t *nrf, const void *data, size_t len,
		const struct timespec *duration);

/**
 * Send data to a specific TX address for a given interval
 *
 * This function is just a combination of nrf905_write_tx_addr() and
 * nrf905_send_for().
 */
int nrf905_send_to_for(nrf905_t *nrf, uint32_t addr, const void *data,
		size_t len, const struct timespec *duration);

/**
 * Enable receiver
 *
 * Enables the receiver. After calling this frames to the configured RX address
 * will be received. If a frame is received it must be obtained using one of
 * the nrf905_receive*() functions. During the periode between receiving the
 * frame and it being fetched by the software, no new frames can be received.
 *
 * @param nrf	NRF905 object to initialize
 */
int nrf905_recv_enable(nrf905_t *nrf);

/**
 * Disable receiver
 *
 * @param nrf	NRF905 object to initialize
 */
int nrf905_recv_disable(nrf905_t *nrf);

/**
 * Receive data
 *
 * Fetch received frame from receive buffer. If no frame is available block
 * until a frame is received. Will automatically enable/disable receiver if not
 * already enabled.
 *
 * @param nrf	NRF905 object to initialize
 * @param data	Buffer to return data in
 * @param len	Length of data buffer. If buffer is smaller then RX payload
 *		width, the received data is silently truncated
 */
int nrf905_recv(nrf905_t *nrf, void *data, size_t len);

/**
 * Receive data non-blocking
 *
 * Fetch received frame from receive buffer. Return directly if receive buffer
 * is empty. Requires the receiver to be enabled to be useful.
 *
 * @param nrf	NRF905 object to initialize
 * @param data	Buffer to return data in
 * @param len	Length of data buffer. If buffer is smaller then RX payload
 *		width, the received data is silently truncated
 *
 * @returns	0 on success, -1 and set errno to EWOULDBLOCK if no frame has
 *		been received.
 */
int nrf905_recv_nb(nrf905_t *nrf, void *data, size_t len);

/**
 * Receive data with timeout
 *
 * Fetch received frame from receive buffer. If no frame is available block
 * until a frame is received or the timeout expires. Will automatically
 * enable/disable receiver if not already enabled.
 *
 * @param nrf	NRF905 object to initialize
 * @param data	Buffer to return data in
 * @param len	Length of data buffer. If buffer is smaller then RX payload
 *		width, the received data is silently truncated
 * @param to	Timeout after which to return if no frame is received
 *
 * @returns	0 on success, -1 and set errno to ETIMEDOUT if timeout expired.
 */
int nrf905_recv_to(nrf905_t *nrf, void *data, size_t len,
			const struct timespec *to);

#ifdef __cplusplus
}
#endif

#endif // __NRF905_H__
