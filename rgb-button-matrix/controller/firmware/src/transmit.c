#include "matrix.h"


/*

Data to receive :
- colors: 48 * 24 bits => 1152 bits
At 400kHz (i2c) => 2.88ms (if 1 row per cycle => 360us for a single row)
At 1mHz (i2c max) => 1.152ms (1 row/cycle => 144us)
At 6mHz (uart max) => 192us

Data to transmit :
- 10 button events => 10 * 8 = 80 bits
- 44 * 8 bits value = 352 bits
=> 432 bits
At 400kHz (i2c) => 1.08ms
At 1mHz (i2c max) => 432us
At 6mHz (uart max) => 72us

Idea: use DMA with UART

DMA mode can be enabled for transmission by setting DMAT bit in the USART_CR3
register. Data is loaded from a SRAM area configured using the DMA peripheral (refer to
Section 10: Direct memory access controller (DMA) on page 149) to the USART_TDR
register whenever the TXE bit is set. To map a DMA channel for USART transmission, use
the following procedure (x denotes the channel number):
1.Write the USART_TDR register address in the DMA control register to configure it as
the destination of the transfer. The data is moved to this address from memory after
each TXE event.
2.Write the memory address in the DMA control register to configure it as the source of
the transfer. The data is loaded into the USART_TDR register from this memory area
after each TXE event.
3.Configure the total number of bytes to be transferred to the DMA control register.
4.Configure the channel priority in the DMA register
5.Configure DMA interrupt generation after half/ full transfer as required by the
application.
6.Clear the TC flag in the USART_ISR register by setting the TCCF bit in the
USART_ICR register.
7.Activate the channel in the DMA register.
When the number of data transfers programmed in the DMA Controller is reached, the DMA
controller generates an interrupt on the DMA channel interrupt vector.
In transmission mode, once the DMA has written all the data to be transmitted (the TCIF flag
is set in the DMA_ISR register), the TC flag can be monitored to make sure that the USART
communication is complete. This is required to avoid corrupting the last transmission before
disabling the USART or entering Stop mode. Software must wait until TC=1. The TC flag
remains cleared during all data transfers and it is set by hardware at the end of transmission
of the last frame.

Reception using DMA
DMA mode can be enabled for reception by setting the DMAR bit in USART_CR3 register.
Data is loaded from the USART_RDR register to a SRAM area configured using the DMA
peripheral (refer to Section 10: Direct memory access controller (DMA) on page 149)
whenever a data byte is received. To map a DMA channel for USART reception, use the
following procedure:
1.Write the USART_RDR register address in the DMA control register to configure it as
the source of the transfer. The data is moved from this address to the memory after
each RXNE event.
2.Write the memory address in the DMA control register to configure it as the destination
of the transfer. The data is loaded from USART_RDR to this memory area after each
RXNE event.
3.Configure the total number of bytes to be transferred to the DMA control register.
4.Configure the channel priority in the DMA control register
5.Configure interrupt generation after half/ full transfer as required by the application.
6.Activate the channel in the DMA control register.
When the number of data transfers programmed in the DMA Controller is reached, the DMA
controller generates an interrupt on the DMA channel interrupt vector.
*/


/*

For transmit :
Launch DMA transfer in 2nd longest BAM period
When it's done disable USART (may be done without interrupt ?)

For reception :
Initialize a circular DMA for reception to a static buffer
Continuously receive color data
Update colors data during 2nd longest BAM period

*/
