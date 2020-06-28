/*
 * cgrf.h
 *
 * Created: 22/06/2020 09:18:29
 *  Author: Chris
 */ 

#include <stdint.h>

#ifndef CGRF_H_
#define CGRF_H_

typedef enum
{
	data_rate_1_mbps,
	data_rate_2_mbps,
} air_data_rate_t;

typedef enum
{
	power_minus_18dbm,
	power_minus_12dbm,
	power_minus_6dbm,
	power_0dbm,
} rf_output_power_t;

typedef enum
{
	crc_none,
	crc_1_byte,
	crc_2_bytes,
} crc_encoding_t;

typedef enum
{
	no_acknowledgment,
	auto_acknowledgment,
} auto_ack_t;

typedef enum
{
	static_length,
	dynamic_length,
} payload_length_t;

typedef enum
{
	success,
	failed,
	failed_retry_in_progress,
} acknowledgment_t;

// initialise the nRF24L01+ module ports.
void cgrf_init();

// set the channel.
void cgrf_set_channel(uint8_t const channel);

// set the air data rate.
void cgrf_set_data_rate(air_data_rate_t const data_rate);

// set the RF output power.
void cgrf_set_output_power(rf_output_power_t const output_power);

// set the cyclic encoding scheme.
void cgrf_set_crc_encoding(crc_encoding_t const crc);

// set the auto acknowledgment.
void cgrf_set_acknowledgment(auto_ack_t const ack);

// set the payload length.
void cgrf_set_length(payload_length_t const length, uint8_t const size);

// set the transmit destination address.
void cgrf_set_tx_address(uint8_t address[5]);

// setup as a transmitter and power up.
void cgrf_start_as_transmitter();

// setup as a receiver and power up.
void cgrf_start_as_reciever();

// power up the transmitter/receiver and return the status
uint8_t cgrf_power_up();

// power down the transmitter/receiver and return the status
uint8_t cgrf_power_down();

// send data.
acknowledgment_t cgrf_transmit_data(uint8_t const * const data, uint8_t const size);
acknowledgment_t cgrf_retransmit();

uint8_t cgrf_data_ready();
uint8_t cgrf_get_payload(uint8_t * data, uint8_t const size);

// check status for auto acknowledgment.
acknowledgment_t cgrf_check_acknowledgment();

#endif /* CGRF_H_ */