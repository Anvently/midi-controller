#include <matrix.h>

/*
Procedure
1.Select the BR[2:0] bits to define the serial clock baud rate (see SPI_CR1 register).
2.Select the CPOL and CPHA bits to define one of the four relationships between the
data transfer and the serial clock (see Figure 210).
3.Set the DFF bit to define 8- or 16-bit data frame format
4.Configure the LSBFIRST bit in the SPI_CR1 register to define the frame format
5.If the NSS pin is required in input mode, in hardware mode, connect the NSS pin to a
high-level signal during the complete byte transmit sequence. In software mode, set the
SSM and SSI bits in the SPI_CR1 register.
If the NSS pin is required in output mode, the SSOE bit only should be set.
6.The MSTR and SPE bits must be set (they remain set only if the NSS pin is connected
to a high-level signal).
In this configuration the MOSI pin is a data output and the MISO pin is a data input.
Transmit sequence
The transmit sequence begins when a byte is written in the Tx Buffer.
The data byte is parallel-loaded into the shift register (from the internal bus) during the first
bit transmission and then shifted out serially to the MOSI pin MSB first or LSB first
depending on the LSBFIRST bit in the SPI_CR1 register. The TXE flag is set on the transfer
of data from the Tx Buffer to the shift register and an interrupt is generated if the TXEIE bit in
the SPI_CR2 register is set.

*/

/// @brief Transmit 32bits of data to SPI1.
/// Receive 8 bits when transmitting the first 16 bytes
/// @param data 
/// @param size 
/// @return received data
uint8_t	spi_transmit_receive(uint32_t data) {
	uint8_t	button_reading;

	//Load 74HC165 input into parallel shift register
	RESET_PIN(GPIOA, LOAD_PIN);
	// __NOP();
	SET_PIN(GPIOA, LOAD_PIN);

	RESET_PIN(GPIOA, LATCH_PIN);
	SPI1->CR1 |= (SPI_CR1_MSTR | SPI_CR1_SPE); //Enable master mode and SPI
	SPI1->DR = (uint16_t)(data >> 16); //Write 16 first MSB
	while ((SPI1->SR & SPI_SR_TXE) == 0); //While tx_buffer is not empty
	while ((SPI1->SR & SPI_SR_RXNE) == 0); // While rx_buffer is not ready
	button_reading = (uint8_t)(SPI1->DR >> 8); //Read received data
	SPI1->DR = (uint16_t)(data & 0x0000FFFF); //Send 16 LSB
	// while ((SPI1->SR & SPI_SR_TXE) == 0); //While tx_buffer is not empty
	// SPI1->CR1 &= ~SPI_CR1_SPE;
	SET_PIN(GPIOA, LATCH_PIN);

	return (button_reading);
}

// static uint8_t SPI_TransmitReceive(uint32_t data) {
// 	uint8_t	button_reading;

// 	//Load 74HC165 input into parallel shift register
// 	RESET_PIN(GPIOA, LOAD_PIN);
// 	// __NOP();
// 	SET_PIN(GPIOA, LOAD_PIN);

// 	RESET_PIN(GPIOA, LATCH_PIN);
// 	// if (HAL_SPI_Transmit(hspi, data, 2, 1) != HAL_OK) {
// 	// 	Error_Handler();
// 	// }
// 	if (HAL_SPI_TransmitReceive(&hspi, (uint8_t*)&data + 3, &button_reading, 1, 1) != HAL_OK) {
// 		Error_Handler();
// 	}
// 	if (HAL_SPI_Transmit(&hspi, (uint8_t*)&data + 2, 1, 1) != HAL_OK) {
// 		Error_Handler();
// 	}
// 	if (HAL_SPI_Transmit(&hspi, (uint8_t*)&data + 1, 1, 1) != HAL_OK) {
// 		Error_Handler();
// 	}
// 	if (HAL_SPI_Transmit(&hspi, (uint8_t*)&data, 1, 1) != HAL_OK) {
// 		Error_Handler();
// 	}
// 	SET_PIN(GPIOA, LATCH_PIN);

// 	return (button_reading);
// }



// static void SPI_Transmit(uint32_t data) {

// 	RESET_PIN(GPIOA, LATCH_PIN);
// 	// if (HAL_SPI_Transmit(hspi, data, 2, 1) != HAL_OK) {
// 	// 	Error_Handler();
// 	// }
// 	if (HAL_SPI_Transmit(&hspi, (uint8_t*)&data + 3, 1, 1) != HAL_OK) {
// 		Error_Handler();
// 	}
// 	if (HAL_SPI_Transmit(&hspi, (uint8_t*)&data + 2, 1, 1) != HAL_OK) {
// 		Error_Handler();
// 	}
// 	if (HAL_SPI_Transmit(&hspi, (uint8_t*)&data + 1, 1, 1) != HAL_OK) {
// 		Error_Handler();
// 	}
// 	if (HAL_SPI_Transmit(&hspi, (uint8_t*)&data, 1, 1) != HAL_OK) {
// 		Error_Handler();
// 	}
// 	SET_PIN(GPIOA, LATCH_PIN);
// }