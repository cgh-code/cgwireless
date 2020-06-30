/*
 * nrf24l01.h
 *
 * Created: 17-06-2020
 * Author:  Chris G Hough
 */ 

#include <stdint.h>

#ifndef NRF24L01_H_
#define NRF24L01_H_

#define NRF24_PORT_CE PORTC
#define NRF24_DDR_CE DDRC
#define NRF24_CE PC0

#define NRF24_PORT_CSN PORTC
#define NRF24_DDR_CSN DDRC
#define NRF24_CSN PC1

typedef enum
{
	standby_I_minimise_current,
	standby_II_fast_start,
} nrf24_mode_t;

// configure the nRF24L01+ ports
void nrf24_configure_ports();

// flush to transmitter buffer.
uint8_t nrf24_flush_tx();

// flush the receiver buffer.
uint8_t nrf24_flush_rx();

// get the config register value and return by pointer;
uint8_t nrf24_get_config(uint8_t * value);

// set the config register.
uint8_t nrf24_set_config(uint8_t const value);

// get the enhanced ShockBurst auto acknowledgment register value and return by pointer;
uint8_t nrf24_get_en_aa(uint8_t * value);

// set the enhanced ShockBurst auto acknowledgment register.
uint8_t nrf24_set_en_aa(uint8_t const value);

// get enabled RX addresses register value and return by pointer;
uint8_t nrf24_get_en_rxaddr(uint8_t * value);

// set enabled RX addresses register.
uint8_t nrf24_set_en_rxaddr(uint8_t const value);

// get setup of address widths register value and return by pointer;
uint8_t nrf24_get_setup_aw(uint8_t * value);

// set setup of address widths register.
uint8_t nrf24_set_setup_aw(uint8_t const value);

// get setup of automatic retransmission register value and return by pointer;
uint8_t nrf24_get_setup_retr(uint8_t * value);

// set setup of automatic retransmission register.
uint8_t nrf24_set_setup_retr(uint8_t const value);

// get radio frequency channel register value and return by pointer;
uint8_t nrf24_get_rf_ch(uint8_t * value);

// set radio frequency channel register.
uint8_t nrf24_set_rf_ch(uint8_t const value);

// get radio frequency setup register value and return by pointer;
uint8_t nrf24_get_rf_setup(uint8_t * value);

// set radio frequency setup register.
uint8_t nrf24_set_rf_setup(uint8_t const value);

// get status register value and return by pointer;
uint8_t nrf24_get_status(uint8_t * value);

// set status register.
uint8_t nrf24_set_status(uint8_t const value);

// get transmit observe register value and return by pointer;
uint8_t nrf24_get_observe_tx(uint8_t * value);

// set transmit observe register.
uint8_t nrf24_set_observe_tx(uint8_t const value);

// get rx payload in data pipe 0 register value and return by pointer;
uint8_t nrf24_get_rx_pw_p0(uint8_t * value);

// set rx payload in data pipe 0 register.
uint8_t nrf24_set_rx_pw_p0(uint8_t const value);

// get rx payload in data pipe 1 register value and return by pointer;
uint8_t nrf24_get_rx_pw_p1(uint8_t * value);

// set rx payload in data pipe 1 register.
uint8_t nrf24_set_rx_pw_p1(uint8_t const value);

// get rx payload in data pipe 2 register value and return by pointer;
uint8_t nrf24_get_rx_pw_p2(uint8_t * value);

// set rx payload in data pipe 2 register.
uint8_t nrf24_set_rx_pw_p2(uint8_t const value);

// get rx payload in data pipe 3 register value and return by pointer;
uint8_t nrf24_get_rx_pw_p3(uint8_t * value);

// set rx payload in data pipe 3 register.
uint8_t nrf24_set_rx_pw_p3(uint8_t const value);

// get rx payload in data pipe 4 register value and return by pointer;
uint8_t nrf24_get_rx_pw_p4(uint8_t * value);

// set rx payload in data pipe 4 register.
uint8_t nrf24_set_rx_pw_p4(uint8_t const value);

// get rx payload in data pipe 5 register value and return by pointer;
uint8_t nrf24_get_rx_pw_p5(uint8_t * value);

// set rx payload in data pipe 5 register.
uint8_t nrf24_set_rx_pw_p5(uint8_t const value);

// get FIFO status register value and return by pointer;
uint8_t nrf24_get_fifo_status(uint8_t * value);

// set FIFO status register.
uint8_t nrf24_set_fifo_status(uint8_t const value);

// get dynamic payload data register value and return by pointer;
uint8_t nrf24_get_dynpd(uint8_t * value);

// set dynamic payload data register.
uint8_t nrf24_set_dynpd(uint8_t const value);

// get feature register value and return by pointer;
uint8_t nrf24_get_feature(uint8_t * value);

// set feature register.
uint8_t nrf24_set_feature(uint8_t const value);

// get the tx address.
uint8_t nrf24_get_tx_address(uint8_t * ptr);

// set the tx address.
uint8_t nrf24_set_tx_address(uint8_t addr[5]);

// get the rx address data pipe 0.
uint8_t nrf24_get_rx_address_pipe0(uint8_t * ptr);

// set the rx address data pipe 0.
uint8_t nrf24_set_rx_address_pipe0(uint8_t addr[5]);

// get the rx address data pipe 1.
uint8_t nrf24_get_rx_address_pipe1(uint8_t * ptr);

// set the rx address data pipe 1.
uint8_t nrf24_set_rx_address_pipe1(uint8_t addr[5]);

// get the carrier detect.
uint8_t nrf24_get_cd(uint8_t * value);


// send data.
uint8_t nrf24_transmit_data(nrf24_mode_t const mode, uint8_t const * const data, uint8_t const size);

// resend data.
uint8_t nrf24_retransmit(nrf24_mode_t const mode);

// get the size of the received payload.
uint8_t nrf24_get_payload_size(uint8_t * size);

// get the payload.
uint8_t nrf24_get_payload(uint8_t * dataptr, uint8_t const size);

void nrf24_set_ce_low();
void nrf24_set_ce_high();

#endif /* NRF24L01_H_ */