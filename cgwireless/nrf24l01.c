/*
 * nrf24l01.c
 *
 * Created: 17-06-2020
 * Author:  Chris G Hough
 */ 

#include "nrf24l01.h"

#ifndef F_CPU				// if F_CPU was not defined in Project -> Properties
#define F_CPU 1000000UL		// define it now as 8 MHz unsigned long
#endif

#include <util/delay.h>
#include <avr/io.h>

#define NRF24_PORT_SCK PORTB
#define NRF24_DDR_SCK DDRB
#define NRF24_SCK PB5

#define NRF24_PORT_MISO PORTB
#define NRF24_DDR_MISO DDRB
#define NRF24_PIN_MISO PINB
#define NRF24_MISO PB4

#define NRF24_PORT_MOSI PORTB
#define NRF24_DDR_MOSI DDRB
#define NRF24_MOSI PB3


// Register map table addresses
// ----------------------------
// configuration register address.
// enhanced ShockBurst address.
// enabled RX addresses.
// setup of address widths.
// setup of automatic retransmission
// RF channel
// RF setup register
// status register
//   (In parallel to the SPI command word applied on the MOSI pin,
//   the status register is shifted serially out on the MISO pin)
// transmit observe register

// Register map table
#define RMAP_CONFIG		 0x00
#define RMAP_EN_AA       0x01
#define RMAP_EN_RXADDR   0x02
#define RMAP_SETUP_AW    0x03
#define RMAP_SETUP_RETR  0x04
#define RMAP_RF_CH       0x05
#define RMAP_RF_SETUP    0x06
#define RMAP_STATUS		 0x07
#define RMAP_OBSERVE_TX  0x08
#define RMAP_CD          0x09
#define RMAP_RX_ADDR_P0  0x0A
#define RMAP_RX_ADDR_P1  0x0B
#define RMAP_RX_ADDR_P2  0x0C
#define RMAP_RX_ADDR_P3  0x0D
#define RMAP_RX_ADDR_P4  0x0E
#define RMAP_RX_ADDR_P5  0x0F
#define RMAP_TX_ADDR     0x10
#define RMAP_RX_PW_P0    0x11
#define RMAP_RX_PW_P1    0x12
#define RMAP_RX_PW_P2    0x13
#define RMAP_RX_PW_P3    0x14
#define RMAP_RX_PW_P4    0x15
#define RMAP_RX_PW_P5    0x16
#define RMAP_FIFO_STATUS 0x17
#define RMAP_DYNPD       0x1C
#define RMAP_FEATURE     0x1D

/* Instruction Mnemonics */
#define R_REGISTER    0x00
#define W_REGISTER    0x20
#define REGISTER_MASK 0x1F
#define ACTIVATE      0x50
#define R_RX_PL_WID   0x60
#define R_RX_PAYLOAD  0x61
#define W_TX_PAYLOAD  0xA0
#define W_ACK_PAYLOAD 0xA8
#define FLUSH_TX      0xE1
#define FLUSH_RX      0xE2
#define REUSE_TX_PL   0xE3
#define RF24_NOP      0xFF

// function declarations
uint8_t write_register_value(uint8_t const reg_map_addr, uint8_t const data);
uint8_t write_register_bytes(uint8_t const reg_map_addr, uint8_t const * const data, uint8_t const size);
uint8_t read_register_bytes(uint8_t const reg_map_addr, uint8_t * dataptr, uint8_t const size);

// SPI function declaration.
uint8_t spi_out_command(uint8_t const cmd);
void spi_out_data_value(uint8_t const data);
void spi_out_data_bytes(uint8_t const * const data, uint8_t const size);
uint8_t spi_in_data_value();
void spi_in_data_bytes(uint8_t * dataptr, uint8_t size);

// public interface to configure the nRF24L01+
void nrf24_configure_ports()
{
	// setup port pins for output.
	NRF24_DDR_CE |= (1 << NRF24_CE);
	NRF24_DDR_CSN |= (1 << NRF24_CSN);
	NRF24_DDR_SCK |= (1 << NRF24_SCK);
	NRF24_DDR_MOSI |= (1 << NRF24_MOSI);

	// Set port pins as input.
	NRF24_DDR_MISO &= ~(1 << NRF24_MISO);

	// Set CE low.
	NRF24_PORT_CE &= ~(1 << NRF24_CE);

	// Ensure CSN is high.
	NRF24_PORT_CSN |= (1 << NRF24_CSN);
}

// flush to transmitter buffer.
uint8_t nrf24_flush_tx()
{
	// every command must be started by a high to low transition on CSN.
	// set CSN low to begin command.
	NRF24_PORT_CSN &= ~(1 << NRF24_CSN);

	uint8_t status = spi_out_command(FLUSH_TX);
	
	// Set CSN high to end command.
	NRF24_PORT_CSN |= (1 << NRF24_CSN);
	
	return status;
}

// flush the receiver buffer.
uint8_t nrf24_flush_rx()
{
	// every command must be started by a high to low transition on CSN.
	// set CSN low to begin command.
	NRF24_PORT_CSN &= ~(1 << NRF24_CSN);

	uint8_t status = spi_out_command(FLUSH_RX);
	
	// Set CSN high to end command.
	NRF24_PORT_CSN |= (1 << NRF24_CSN);
	
	return status;
}

// get the config register value and return by pointer;
uint8_t nrf24_get_config(uint8_t * value)
{
	return read_register_bytes(RMAP_CONFIG, value, 1);
}

// set the config register.
uint8_t nrf24_set_config(uint8_t const value)
{
	return write_register_value(RMAP_CONFIG, value);
}

// get the enhanced ShockBurst auto acknowledgment register value and return by pointer;
uint8_t nrf24_get_en_aa(uint8_t * value)
{
	return read_register_bytes(RMAP_EN_AA, value, 1);
}

// set the enhanced ShockBurst auto acknowledgment register.
uint8_t nrf24_set_en_aa(uint8_t const value)
{
	return write_register_value(RMAP_EN_AA, value);
}

// get enabled RX addresses register value and return by pointer;
uint8_t nrf24_get_en_rxaddr(uint8_t * value)
{
	return read_register_bytes(RMAP_EN_RXADDR, value, 1);
}

// set enabled RX addresses register.
uint8_t nrf24_set_en_rxaddr(uint8_t const value)
{
	return write_register_value(RMAP_EN_RXADDR, value);
}

// get setup of address widths register value and return by pointer;
uint8_t nrf24_get_setup_aw(uint8_t * value)
{
	return read_register_bytes(RMAP_SETUP_AW, value, 1);
}

// set setup of address widths register.
uint8_t nrf24_set_setup_aw(uint8_t const value)
{
	return write_register_value(RMAP_SETUP_AW, value);
}

// get setup of automatic retransmission register value and return by pointer;
uint8_t nrf24_get_setup_retr(uint8_t * value)
{
	return read_register_bytes(RMAP_SETUP_RETR, value, 1);
}

// set setup of automatic retransmission register.
uint8_t nrf24_set_setup_retr(uint8_t const value)
{
	return write_register_value(RMAP_SETUP_RETR, value);
}

// get radio frequency channel register value and return by pointer;
uint8_t nrf24_get_rf_ch(uint8_t * value)
{
	return read_register_bytes(RMAP_RF_CH, value, 1);
}

// set radio frequency channel register.
uint8_t nrf24_set_rf_ch(uint8_t const value)
{
	return write_register_value(RMAP_RF_CH, value);
}

// get radio frequency setup register value and return by pointer;
uint8_t nrf24_get_rf_setup(uint8_t * value)
{
	return read_register_bytes(RMAP_RF_SETUP, value, 1);
}

// set radio frequency setup register.
uint8_t nrf24_set_rf_setup(uint8_t const value)
{
	return write_register_value(RMAP_RF_SETUP, value);
}

// get status register value and return by pointer;
uint8_t nrf24_get_status(uint8_t * value)
{
	return read_register_bytes(RMAP_STATUS, value, 1);
}

// set status register.
uint8_t nrf24_set_status(uint8_t const value)
{
	return write_register_value(RMAP_STATUS, value);
}

// get transmit observe register value and return by pointer;
uint8_t nrf24_get_observe_tx(uint8_t * value)
{
	return read_register_bytes(RMAP_OBSERVE_TX, value, 1);
}

// set transmit observe register.
uint8_t nrf24_set_observe_tx(uint8_t const value)
{
	return write_register_value(RMAP_OBSERVE_TX, value);
}

// get rx payload in data pipe 0 register value and return by pointer;
uint8_t nrf24_get_rx_pw_p0(uint8_t * value)
{
	return read_register_bytes(RMAP_RX_PW_P0, value, 1);
}

// set rx payload in data pipe 0 register.
uint8_t nrf24_set_rx_pw_p0(uint8_t const value)
{
	return write_register_value(RMAP_RX_PW_P0, value);
}

// get rx payload in data pipe 1 register value and return by pointer;
uint8_t nrf24_get_rx_pw_p1(uint8_t * value)
{
	return read_register_bytes(RMAP_RX_PW_P1, value, 1);
}

// set rx payload in data pipe 1 register.
uint8_t nrf24_set_rx_pw_p1(uint8_t const value)
{
	return write_register_value(RMAP_RX_PW_P1, value);
}

// get rx payload in data pipe 2 register value and return by pointer;
uint8_t nrf24_get_rx_pw_p2(uint8_t * value)
{
	return read_register_bytes(RMAP_RX_PW_P2, value, 1);
}

// set rx payload in data pipe 2 register.
uint8_t nrf24_set_rx_pw_p2(uint8_t const value)
{
	return write_register_value(RMAP_RX_PW_P2, value);
}

// get rx payload in data pipe 3 register value and return by pointer;
uint8_t nrf24_get_rx_pw_p3(uint8_t * value)
{
	return read_register_bytes(RMAP_RX_PW_P3, value, 1);
}

// set rx payload in data pipe 3 register.
uint8_t nrf24_set_rx_pw_p3(uint8_t const value)
{
	return write_register_value(RMAP_RX_PW_P3, value);
}

// get rx payload in data pipe 4 register value and return by pointer;
uint8_t nrf24_get_rx_pw_p4(uint8_t * value)
{
	return read_register_bytes(RMAP_RX_PW_P4, value, 1);
}

// set rx payload in data pipe 4 register.
uint8_t nrf24_set_rx_pw_p4(uint8_t const value)
{
	return write_register_value(RMAP_RX_PW_P4, value);
}

// get rx payload in data pipe 5 register value and return by pointer;
uint8_t nrf24_get_rx_pw_p5(uint8_t * value)
{
	return read_register_bytes(RMAP_RX_PW_P5, value, 1);
}

// set rx payload in data pipe 5 register.
uint8_t nrf24_set_rx_pw_p5(uint8_t const value)
{
	return write_register_value(RMAP_RX_PW_P5, value);
}

// get FIFO status register value and return by pointer;
uint8_t nrf24_get_fifo_status(uint8_t * value)
{
	return read_register_bytes(RMAP_FIFO_STATUS, value, 1);
}

// set FIFO status register.
uint8_t nrf24_set_fifo_status(uint8_t const value)
{
	return write_register_value(RMAP_FIFO_STATUS, value);
}

// get dynamic payload data register value and return by pointer;
uint8_t nrf24_get_dynpd(uint8_t * value)
{
	return read_register_bytes(RMAP_DYNPD, value, 1);
}

// set dynamic payload data register.
uint8_t nrf24_set_dynpd(uint8_t const value)
{
	return write_register_value(RMAP_DYNPD, value);
}

// get feature register value and return by pointer;
uint8_t nrf24_get_feature(uint8_t * value)
{
	return read_register_bytes(RMAP_FEATURE, value, 1);
}

// set feature register.
uint8_t nrf24_set_feature(uint8_t const value)
{
	return write_register_value(RMAP_FEATURE, value);
}

// get the tx address.
uint8_t nrf24_get_tx_address(uint8_t * ptr)
{
	return read_register_bytes(RMAP_TX_ADDR, ptr, 5);
}

// set the tx address.
uint8_t nrf24_set_tx_address(uint8_t addr[5])
{
	return write_register_bytes(RMAP_TX_ADDR, addr, 5);
}

// get the rx address data pipe 0.
uint8_t nrf24_get_rx_address_pipe0(uint8_t * ptr)
{
	return read_register_bytes(RMAP_RX_ADDR_P0, ptr, 5);
}

// set the rx address data pipe 0.
uint8_t nrf24_set_rx_address_pipe0(uint8_t addr[5])
{
	return write_register_bytes(RMAP_RX_ADDR_P0, addr, 5);
}

// get the rx address data pipe 1.
uint8_t nrf24_get_rx_address_pipe1(uint8_t * ptr)
{
	return read_register_bytes(RMAP_RX_ADDR_P1, ptr, 5);
}

// set the rx address data pipe 1.
uint8_t nrf24_set_rx_address_pipe1(uint8_t addr[5])
{
	return write_register_bytes(RMAP_RX_ADDR_P1, addr, 5);
}

// get the carrier detect.
uint8_t nrf24_get_cd(uint8_t * value)
{
	return read_register_bytes(RMAP_CD, value, 1);
}


// send data.
uint8_t nrf24_transmit_data(nrf24_mode_t const mode, uint8_t const * const data, uint8_t const size)
{
	// every command must be started by a high to low transition on CSN.
	// set CSN low to begin command.
	NRF24_PORT_CSN &= ~(1 << NRF24_CSN);

	// write payload command.
	uint8_t status = spi_out_command(W_TX_PAYLOAD);
	
	// now send data, size is 1 to 32 bytes
	spi_out_data_bytes(data, size);

	// Set CSN high to end command.
	NRF24_PORT_CSN |= (1 << NRF24_CSN);

	// high value represents Standby-II mode.
	if (NRF24_PORT_CE & (1 << NRF24_CE))
	{
		// set CE low
		NRF24_PORT_CE &= ~(1 << NRF24_CE);
	}

	// pulse CE high to transmit.
	NRF24_PORT_CE |= (1 << NRF24_CE);

	// return to standby_I mode after transmission.
	if (mode == standby_I_minimise_current)
	{
		// set CE low
		NRF24_PORT_CE &= ~(1 << NRF24_CE);
	}
	
	return status;
}

// send data.
uint8_t nrf24_retransmit(nrf24_mode_t const mode)
{
	// every command must be started by a high to low transition on CSN.
	// set CSN low to begin command.
	NRF24_PORT_CSN &= ~(1 << NRF24_CSN);

	// write payload command.
	uint8_t status = spi_out_command(W_TX_PAYLOAD);

	// Set CSN high to end command.
	NRF24_PORT_CSN |= (1 << NRF24_CSN);

	// high value represents Standby-II mode.
	if (NRF24_PORT_CE & (1 << NRF24_CE))
	{
		// set CE low
		NRF24_PORT_CE &= ~(1 << NRF24_CE);
	}

	// pulse CE high to transmit.
	NRF24_PORT_CE |= (1 << NRF24_CE);

	// return to standby_I mode after transmission.
	if (mode == standby_I_minimise_current)
	{
		// set CE low
		NRF24_PORT_CE &= ~(1 << NRF24_CE);
	}
	
	return status;
}

// get the size of the received payload.
uint8_t nrf24_get_payload_size(uint8_t * size)
{
	uint8_t cmd = R_RX_PL_WID;

	// every command must be started by a high to low transition on CSN.
	// set CSN low to begin command.
	NRF24_PORT_CSN &= ~(1 << NRF24_CSN);

	uint8_t status = spi_out_command(cmd);
	spi_in_data_bytes(size, 1);
	
	// Set CSN high to end command.
	NRF24_PORT_CSN |= (1 << NRF24_CSN);
	
	return status;
}

// get the payload.
uint8_t nrf24_get_payload(uint8_t * dataptr, uint8_t const size)
{
	uint8_t cmd = R_RX_PAYLOAD;

	// every command must be started by a high to low transition on CSN.
	// set CSN low to begin command.
	NRF24_PORT_CSN &= ~(1 << NRF24_CSN);

	uint8_t status = spi_out_command(cmd);
	spi_in_data_bytes(dataptr, size);
		
	// Set CSN high to end command.
	NRF24_PORT_CSN |= (1 << NRF24_CSN);
		
	return status;
}

void nrf24_set_ce_low()
{
	// Set CE low.
	NRF24_PORT_CE &= ~(1 << NRF24_CE);
}

void nrf24_set_ce_high()
{
	// Set CE high.
	NRF24_PORT_CE |= (1 << NRF24_CE);	
}


// write to given register map with given value.
uint8_t write_register_value(uint8_t const reg_map_addr, uint8_t const data)
{
	// 001AAAAA (where AAAAA is register map address)
	uint8_t cmd = W_REGISTER | (REGISTER_MASK & reg_map_addr);

	// every command must be started by a high to low transition on CSN.
	// set CSN low to begin command.
	NRF24_PORT_CSN &= ~(1 << NRF24_CSN);

	uint8_t status = spi_out_command(cmd);
	spi_out_data_value(data);
	
	// Set CSN high to end command.
	NRF24_PORT_CSN |= (1 << NRF24_CSN);
	
	return status;
}

// write to given register map with given value.
uint8_t write_register_bytes(uint8_t const reg_map_addr, uint8_t const * const data, uint8_t const size)
{
	// 001AAAAA (where AAAAA is register map address)
	uint8_t cmd = W_REGISTER | (REGISTER_MASK & reg_map_addr);

	// every command must be started by a high to low transition on CSN.
	// set CSN low to begin command.
	NRF24_PORT_CSN &= ~(1 << NRF24_CSN);

	uint8_t status = spi_out_command(cmd);
	spi_out_data_bytes(data, size);
	
	// Set CSN high to end command.
	NRF24_PORT_CSN |= (1 << NRF24_CSN);
	
	return status;
}

// read data for given register map.
uint8_t read_register_bytes(uint8_t const reg_map_addr, uint8_t * dataptr, uint8_t const size)
{
	// 001AAAAA (where AAAAA is register map address)
	uint8_t cmd = R_REGISTER | (REGISTER_MASK & reg_map_addr);

	// every command must be started by a high to low transition on CSN.
	// set CSN low to begin command.
	NRF24_PORT_CSN &= ~(1 << NRF24_CSN);

	uint8_t status = spi_out_command(cmd);
	spi_in_data_bytes(dataptr, size);
	
	// Set CSN high to end command.
	NRF24_PORT_CSN |= (1 << NRF24_CSN);
	
	return status;
}


// send SPI command and return the status byte.
uint8_t spi_out_command(uint8_t const cmd)
{
	// The STATUS register is serially shifted out on the MISO pin simultaneously
	// to the SPI command word shifting to the MOSI pin.

	uint8_t status = 0x00;

	// start with clock set low
	NRF24_PORT_SCK &= ~(1 << NRF24_SCK);
	
	// i = bit index, most significant bit to least significant bit.
	//
	for (uint8_t i = 8; i-- != 0;)
	{
		// send command bit via MOSI.
		if (cmd & (1 << i))
		{
			// command bit is high, set MOSI high
			NRF24_PORT_MOSI |= (1 << NRF24_MOSI);
		}
		else
		{
			// command bit is low, set MOSI low
			NRF24_PORT_MOSI &= ~(1 << NRF24_MOSI);
		}

		
		// Set clock high to receive status bit via MISO.
		NRF24_PORT_SCK |= (1 << NRF24_SCK);

		// copy to status.
		if (NRF24_PIN_MISO & (1 << NRF24_MISO))
		{	
			status |= (1 << i);
		}

		// Set clock low
		NRF24_PORT_SCK &= ~(1 << NRF24_SCK);
	}

	return status;
}

// send a byte of data via SPI.
void spi_out_data_value(uint8_t const data)
{
	// start with clock set low
	NRF24_PORT_SCK &= ~(1 << NRF24_SCK);

	// i = bit index, most significant bit to least significant bit.
	//
	for (uint8_t i = 8; i-- != 0;)
	{
		// send command bit via MOSI.
		if (data & (1 << i))
		{
			// data bit is high, set MOSI high
			NRF24_PORT_MOSI |= (1 << NRF24_MOSI);
		}
		else
		{
			// data bit is low, set MOSI low
			NRF24_PORT_MOSI &= ~(1 << NRF24_MOSI);
		}

		// Set clock high to receive status bit via MISO.
		NRF24_PORT_SCK |= (1 << NRF24_SCK);
		
		// Set clock low
		NRF24_PORT_SCK &= ~(1 << NRF24_SCK);
	}
}

// send multiple bytes of data via SPI.
void spi_out_data_bytes(uint8_t const * const data, uint8_t const size)
{
	for (uint8_t i = 0; i != size; i++)
	{
		spi_out_data_value(data[i]);
	}
}

uint8_t spi_in_data_value()
{
	uint8_t data = 0x00;

	// start with clock set low
	NRF24_PORT_SCK &= ~(1 << NRF24_SCK);
	
	// i = bit index, most significant bit to least significant bit.
	//
	for (uint8_t i = 8; i-- != 0;)
	{
		// Set clock high to receive status bit via MISO.
		NRF24_PORT_SCK |= (1 << NRF24_SCK);

		// copy to status.
		if (NRF24_PIN_MISO & (1 << NRF24_MISO))
		{
			data |= (1 << i);
		}

		// Set clock low
		NRF24_PORT_SCK &= ~(1 << NRF24_SCK);
	}

	return data;	
}

void spi_in_data_bytes(uint8_t * dataptr, uint8_t size)
{
	uint8_t * ptr = dataptr;
	
	for (uint8_t i = 0; i != size; i++)
	{
		*ptr = spi_in_data_value();
		ptr++;
		_delay_us(20);
	}
}

