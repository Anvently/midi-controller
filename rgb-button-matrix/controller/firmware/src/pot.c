#include <matrix.h>

/*

Channel 0: 16 pot (R0A -> R3B)
Channel 1: 16 pot (R3C -> R6C)
Channel 2: 12 pot (R6D -> R8E)

Bam sanning with 6 bits = 122Hz
If 16 pot read at each routine, f = 122/3 ~= 40Hz.

SPI Speed = 3Mhz
Sampling rate = 170 ksps/s
24 clock cycle at 3Mhz => 8us
10us * 16 = 160us minimum to read 16 potentiometer
*/

#define NBR_POT 44

int	pot_readings[NBR_POT] = {0};

/// @brief Select MUL
/// @param  
void	read_adc(void) {
	static int	channel = 0;
	int			val;
	
	SPI2->CR1 |= (SPI_CR1_MSTR | SPI_CR1_SPE); // Enable SPI MASTER
	for (uint8_t i = 0; i < 16; i++) {
		ADC_MUL_SELECT(i); // Multiplexer select
		SPI2->CR1 &= ~SPI_CR1_SSI; //Select slave
		SPI2->DR = (0b11 << 3) | channel; // Write 12 bits : start + single-ended + channel
		while ((SPI2->SR & SPI_SR_TXE) == 0); //While tx_buffer is not empty
		SPI2->DR = 0; // Send dummy data
		while ((SPI2->SR & SPI_SR_TXE) == 0); //While tx_buffer is not empty
		while ((SPI2->SR & SPI_SR_RXNE) == 0); // While rx_buffer is not ready
		val = SPI2->DR & (0b1111111111);
		SPI2->CR1 |= SPI_CR1_SSI; //Disable slave
		if (channel * 16 + i < NBR_POT)
			pot_readings[channel * 16 + i] = val;
	}
	channel = (channel + 1) % 3;
}