/*
 * cgrf.c
 *
 * Created: 22/06/2020 09:18:41
 *  Author: Chris
 */ 
#include "cgrf.h"
#include "nrf24l01.h"
#include <string.h>

#ifndef F_CPU				// if F_CPU was not defined in Project -> Properties
#define F_CPU 1000000UL		// define it now as 8 MHz unsigned long
#endif

#include <util/delay.h>

// Config bits
#define CONFIG_ENABLE_CRC	0x08
#define CONFIG_CRC_1BYTE	0x00
#define CONFIG_CRC_2BYTES	0x04
#define CONFIG_PWR_UP		0x02
#define CONFIG_PWR_DOWN		0x00
#define CONFIG_PRIM_PRX		0x01
#define CONFIG_PRIM_PTX		0x00

// status bits.
#define STATUS_RX_DR		0x40
#define STATUS_TX_DS		0x20
#define STATUS_MAX_RT		0x10
#define STATUS_TX_FIFO_FULL	0x01

// RF setup bits
#define RF_DR_1MBPS			0x00
#define RF_DR_2MBPS			0x08
#define RF_DR_250KBPS		0x20

#define RF_PWR_MINUS_18DBM  0x00
#define RF_PWR_MINUS_12DBM	0x02
#define RF_PWR_MINUS_6DBM	0x04
#define RF_PWR_0DBM			0x06

// enable enhanced ShockBurst auto acknowledgment bits for data pipes
#define ENAA_P5				0x20
#define ENAA_P4				0x10
#define ENAA_P3				0x08
#define ENAA_P2				0x04
#define ENAA_P1				0x02
#define ENAA_P0				0x01

// enable RX addresses bits for data pipes
#define ERX_P5				0x20
#define ERX_P4				0x10
#define ERX_P3				0x08
#define ERX_P2				0x04
#define ERX_P1				0x02
#define ERX_P0				0x01

// Address width bits
#define AW_3BYTES			0x01
#define AW_4BYTES			0x02
#define AW_5BYTES			0x03

// automatic retransmission bits.
#define ARD_WAIT_250US		0x00
#define ARD_WAIT_500US		0x10
#define ARD_WAIT_750US		0x20
#define ARD_WAIT_1000US		0x30
#define ARD_WAIT_1500US		0x50
#define ARD_WAIT_4000US		0xF0

// Enable dynamic payload length for data pipes.
#define DPL_P5				0x20
#define DPL_P4				0x10
#define DPL_P3				0x08
#define DPL_P2				0x04
#define DPL_P1				0x02
#define DPL_P0				0x01

#define FEATURE_EN_DPL		0x04
#define FEATURE_EN_ACK_PAY	0x02
#define FEATURE_EN_DYN_ACK	0x01

typedef enum
{
	transmitter,
	reciever,
} mode_t;

typedef enum
{
	on,
	off,
} power_t;

static crc_encoding_t m_crc_encoding = crc_1_byte;
static power_t m_power = off;
static mode_t m_mode = transmitter;
static uint8_t m_channel = 100;
static air_data_rate_t m_data_rate = data_rate_2_mbps;
static rf_output_power_t m_output_power = power_0dbm;
static auto_ack_t m_auto_ack = no_acknowledgment;
static payload_length_t m_payload_length = dynamic_length;
static uint8_t m_payload_size = 0;

static uint8_t m_tx_address[5] = {0x01, 0x02, 0x03, 0x04, 0x01};
static uint8_t m_pipe0_address[5] = {0x01, 0x02, 0x03, 0x04, 0x01};
static uint8_t m_pipe1_address[5] = {0x99, 0x98, 0x97, 0x96, 0x01};

// function declarations.
uint8_t set_auto_ack();
uint8_t set_dynamic_payload();
uint8_t set_features();
uint8_t set_payload1_size();
uint8_t set_config();
uint8_t set_channel();
uint8_t set_rf_setup();
uint8_t set_tx_address();
uint8_t set_pipe0_address();
uint8_t set_pipe1_address();

// initialise the nRF24L01+ module ports.
void cgrf_init()
{
	nrf24_configure_ports();
}

// set the channel.
void cgrf_set_channel(uint8_t const channel)
{
	if (channel <= 127)
	{
		if (m_channel != channel)
		{
			m_channel = channel;
			set_channel();
		}
	}
}

// set the air data rate.
void cgrf_set_data_rate(air_data_rate_t const data_rate)
{
	if (m_data_rate != data_rate)
	{
		m_data_rate = data_rate;
		set_rf_setup();
	}	
}

// set the RF output power.
void cgrf_set_output_power(rf_output_power_t const output_power)
{
	if (m_output_power != output_power)
	{
		m_output_power = output_power;
		set_rf_setup();
	}
}

// set the cyclic encoding scheme.
void cgrf_set_crc_encoding(crc_encoding_t const crc)
{
	if (m_crc_encoding != crc)
	{
		m_crc_encoding = crc;

		if (m_power == on)
			set_config();
	}
}

// set the auto acknowledgment.
void cgrf_set_acknowledgment(auto_ack_t const ack)
{
	if (m_auto_ack != ack)
	{
		m_auto_ack = ack;
		set_auto_ack();
		set_features();
	}	
}

// set the payload length.
void cgrf_set_length(payload_length_t const length, uint8_t const size)
{
	if (m_payload_length == dynamic_length)
	{
		if (m_payload_length != length)
		{
			m_payload_length = length;
			m_payload_size = 0;
			
			set_dynamic_payload();
			set_features();
			set_payload1_size();
		}
	}	
	else
	{
		if (m_payload_length != length)
		{
			m_payload_length = length;	
			set_dynamic_payload();
			set_features();
		}

		if (m_payload_size != size)
		{
			m_payload_size = size;
			set_payload1_size();
		}
	}
}

// set the transmit destination address.
void cgrf_set_tx_address(uint8_t address[5])
{
	if (memcmp(address, m_tx_address, 5) != 0)
	{
		memcpy(address, m_tx_address, 5);
		set_tx_address();
		set_pipe0_address();
	}
}

// setup as a transmitter and power up.
void cgrf_start_as_transmitter()
{
	// reserved automatic acknowledgment pipe only allows 0x00.
	nrf24_set_rx_pw_p0(0x00);

	// set number of bytes to zero for unused data pipes.
	nrf24_set_rx_pw_p2(0x00);
	nrf24_set_rx_pw_p3(0x00);
	nrf24_set_rx_pw_p4(0x00);
	nrf24_set_rx_pw_p5(0x00);
	
	// enable auto acknowledgment (enhanced ShockBurst).
	set_auto_ack();

	// enable RX addresses for data pipes 0 and 1
	nrf24_set_en_rxaddr(ERX_P0 | ERX_P1);

	// auto retransmit delay wait 1000 us
	// auto retransmit count, up to 15 (0x0F) retransmits on fail.
	nrf24_set_setup_retr(ARD_WAIT_500US | 0x0F);
	
	// set address width to 5 bytes.
	nrf24_set_setup_aw(AW_5BYTES);

	set_dynamic_payload();
	set_features();

	// number of bytes in RX payload for data pipe.
	set_payload1_size();
	
	set_channel();
	set_rf_setup();

	// set the addresses.
	set_tx_address();
	set_pipe0_address();
	set_pipe1_address();
	
	// flush the buffers.
	nrf24_flush_rx();
	nrf24_flush_tx();

	// clear the status bits by setting them to 1.
	nrf24_set_status(STATUS_RX_DR | STATUS_TX_DS | STATUS_MAX_RT);

	m_mode = transmitter;
	cgrf_power_up();
}

// setup as a receiver and power up.
void cgrf_start_as_reciever()
{
	// reserved automatic acknowledgment pipe only allows 0x00.
	nrf24_set_rx_pw_p0(0x00);

	// set number of bytes to zero for unused data pipes.
	nrf24_set_rx_pw_p2(0x00);
	nrf24_set_rx_pw_p3(0x00);
	nrf24_set_rx_pw_p4(0x00);
	nrf24_set_rx_pw_p5(0x00);
	
	// enable auto acknowledgment (enhanced ShockBurst).
	set_auto_ack();

	// enable RX addresses for data pipes 0 and 1
	nrf24_set_en_rxaddr(ERX_P0 | ERX_P1);

	// auto retransmit delay wait 1000 us
	// auto retransmit count, up to 15 (0x0F) retransmits on fail.
	nrf24_set_setup_retr(ARD_WAIT_500US | 0x0F);
	
	// set address width to 5 bytes.
	nrf24_set_setup_aw(AW_5BYTES);

	set_dynamic_payload();
	set_features();

	// number of bytes in RX payload for data pipe.
	set_payload1_size();
	
	set_channel();
	set_rf_setup();

	// set the addresses.
	m_pipe1_address[0] = 0x01;
	m_pipe1_address[1] = 0x02;
	m_pipe1_address[2] = 0x03;
	m_pipe1_address[3] = 0x04;
	m_pipe1_address[4] = 0x01;

	set_pipe1_address();

	// flush the buffers.
	nrf24_flush_rx();
	nrf24_flush_tx();

	// clear the status bits by setting them to 1.
	nrf24_set_status(STATUS_RX_DR | STATUS_TX_DS | STATUS_MAX_RT);

	m_mode = reciever;
	cgrf_power_up();
}

// power up the transmitter/receiver.
// returns the status.
uint8_t cgrf_power_up()
{
	if (!m_power == on)
	{
		m_power = on;
		return set_config();
	}

	return 0;
}

// power down the transmitter/receiver.
// returns the status.
uint8_t cgrf_power_down()
{
	if (m_power == on)
	{
		m_power = off;
		return set_config();
	}

	return 0;
}

// send data.
acknowledgment_t cgrf_transmit_data(uint8_t const * const data, uint8_t const size)
{
	nrf24_transmit_data(standby_II_fast_start, data, size);
	
	acknowledgment_t ack = cgrf_check_acknowledgment();
	
	// Note: write one to clear the bit.
	// clear transmitted and clear number of retries bits.
	nrf24_set_status(STATUS_TX_DS | STATUS_MAX_RT);
	
	return ack;
}

acknowledgment_t cgrf_retransmit()
{
	nrf24_retransmit(standby_II_fast_start);

	acknowledgment_t ack = cgrf_check_acknowledgment();
	
	// Note: write one to clear the bit.
	// clear transmitted and clear number of retries bits.
	nrf24_set_status(STATUS_TX_DS | STATUS_MAX_RT);
	
	return ack;
}

uint8_t cgrf_get_payload(uint8_t * data, uint8_t const size)
{
	uint8_t plsize = 0;
	nrf24_get_payload_size(&plsize);

	if (plsize != 0)
	{
		if (plsize <= size)
			nrf24_get_payload(data, plsize);
		else
			nrf24_get_payload(data, size);
	}

	uint8_t status = 0;
	nrf24_get_status(&status);

	if (status & STATUS_RX_DR)
	{
		// Note: write one to clear the bit.
		nrf24_set_status(STATUS_RX_DR);
	}
	
	return status;
}


acknowledgment_t cgrf_check_acknowledgment()
{
	uint8_t status = 0;
	nrf24_get_status(&status);

	// auto acknowledgment received.
	if (status & STATUS_TX_DS)
		return success;

	if (status & STATUS_MAX_RT)
		return failed;

	return failed_retry_in_progress;
}

// private functions...
//
uint8_t set_auto_ack()
{
	uint8_t cmd = 0x00;
	
	if (m_auto_ack == auto_acknowledgment)
		cmd |= ENAA_P0 | ENAA_P1 | ENAA_P2 | ENAA_P3 | ENAA_P4 | ENAA_P5;

	// enable auto acknowledgment (enhanced ShockBurst) for all data pipes.
	return nrf24_set_en_aa(cmd);
}


uint8_t set_dynamic_payload()
{
	uint8_t cmd = 0x00;

	if (m_payload_length == dynamic_length)
	{
		// Enable dynamically sized packets on the 2 RX pipes we use, 0 and 1.
		// RX pipe address 1 is used to for normal packets from radios that send us data.
		// RX pipe address 0 is used to for auto-acknowledgment packets from radios we transmit to.
		cmd |= DPL_P0 | DPL_P1;
	}

	return nrf24_set_dynpd(cmd);
}

uint8_t set_features()
{
	uint8_t cmd = 0x00;
	
	if (m_payload_length == dynamic_length)
	{
		cmd |= FEATURE_EN_DPL;
	}
	
	if (m_auto_ack == auto_acknowledgment)
	{
		cmd |= FEATURE_EN_ACK_PAY;
	}
	
	if (m_payload_length == dynamic_length && m_auto_ack == auto_acknowledgment)
	{
		cmd |= FEATURE_EN_DYN_ACK;
	}
	
	// Enable dynamically sized payloads, ACK payloads, and TX support with or without an ACK request.
	return nrf24_set_feature(cmd);
}

uint8_t set_payload1_size()
{
	// number of bytes in RX payload for data pipe.
	return nrf24_set_rx_pw_p1(m_payload_size);
}

uint8_t set_config()
{
	uint8_t cmd = 0x00;

	// CRC
	if (m_crc_encoding == crc_1_byte)
		cmd	|= CONFIG_ENABLE_CRC | CONFIG_CRC_1BYTE;

	else if (m_crc_encoding == crc_2_bytes)
		cmd	|= CONFIG_ENABLE_CRC | CONFIG_CRC_2BYTES;

	// power		
	if (m_power == on)
		cmd |= CONFIG_PWR_UP;
	else
		cmd |= CONFIG_PWR_DOWN;
		
	// mode
	if (m_mode == transmitter)
		cmd |= CONFIG_PRIM_PTX;

	else if (m_mode == reciever)
		cmd |= CONFIG_PRIM_PRX;
	
	uint8_t status = nrf24_set_config(cmd);
	
	if (m_mode == reciever)
	{
		if (m_power == on)
			nrf24_set_ce_high();
		else
			nrf24_set_ce_low();
	}
	
	return status;
}

uint8_t set_channel()
{
	// frequency, 7 least significant bits
	return nrf24_set_rf_ch(m_channel);
}

uint8_t set_rf_setup()
{
	uint8_t cmd = 0x00;

	// air data rate.	
	if (m_data_rate == data_rate_1_mbps)
		cmd |= RF_DR_1MBPS;
		
	else if (m_data_rate == data_rate_2_mbps)
		cmd |= RF_DR_2MBPS;
	
	// RF output power.
	if (m_output_power == power_minus_18dbm)
		cmd |= RF_PWR_MINUS_18DBM;

	else if (m_output_power == power_minus_12dbm)
		cmd |= RF_PWR_MINUS_12DBM;

	else if (m_output_power == power_minus_6dbm)
		cmd |= RF_PWR_MINUS_6DBM;

	else if (m_output_power == power_0dbm)
		cmd |= RF_PWR_0DBM;
	
	return nrf24_set_rf_setup(cmd);
}

uint8_t set_tx_address()
{
	return nrf24_set_tx_address(m_tx_address);
}

uint8_t set_pipe0_address()
{
	return nrf24_set_rx_address_pipe0(m_pipe0_address);
}

uint8_t set_pipe1_address()
{
	return nrf24_set_rx_address_pipe1(m_pipe1_address);
}

