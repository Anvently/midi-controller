#include <matrix.h>

extern TIM_HandleTypeDef	hTIM14;
extern const uint32_t		BAM_PERIODS[COLOR_RESOLUTION];

void LED_Init() {
	__HAL_RCC_GPIOC_CLK_ENABLE();
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = LED_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);
	HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
}


void Timer2_Init(void) {
	__HAL_RCC_TIM14_CLK_ENABLE();

	hTIM14.Instance = TIM14;
	hTIM14.Init.Prescaler = (SystemCoreClock / 1000000) - 1; // Prescaler pour µs
	hTIM14.Init.CounterMode = TIM_COUNTERMODE_UP;
	hTIM14.Init.Period = BAM_PERIODS[COLOR_RESOLUTION - 1]; // Période pour 1 ms
	hTIM14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	hTIM14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

	if (HAL_TIM_Base_Init(&hTIM14) != HAL_OK) {
		Error_Handler();
	}

	HAL_TIM_Base_Start_IT(&hTIM14); // Activer le timer en mode interruption
}


void SPI_GPIO_Init(void) {
	__HAL_RCC_GPIOA_CLK_ENABLE();  // Activer l'horloge GPIO

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	// SCK -> PA5, MISO -> PA6, MOSI -> PA7
	GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;  // Mode alternatif push-pull
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LATCH_PIN | LOAD_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// SPI 2 Pins for ADC
	GPIO_InitStruct.Pin = ADC_SCK_PIN | ADC_NSS_PIN | ADC_MISO_PIN | ADC_MOSI_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP,
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	//Multiplexer pins for ADC
	GPIO_InitStruct.Pin = ADC_MUL0_PIN | ADC_MUL1_PIN | ADC_MUL2_PIN | ADC_MUL3_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	// Disable both pin latch by default
	HAL_GPIO_WritePin(GPIOA, LATCH_PIN, 1);
	HAL_GPIO_WritePin(GPIOA, LOAD_PIN, 1);
	GPIOB->BSRR = (0b1111 << 8) << 16; //Set multiplexer to 0
}

void SPI1_Init(void) {
	__HAL_RCC_SPI1_CLK_ENABLE();  // Activer l'horloge SPI1

	SPI1->CR1 = SPI_CR1_BR_0;  //Baud rate F_Pclk / 2 => 48Mhz / 2 => 24Mhz
	// SPI1->CR1 |= SPI_CR1_DFF;
	SPI1->CR1 |= (SPI_CR1_SSM | SPI_CR1_SSI);
	SPI1->CR1 |= SPI_CR1_MSTR;
	SPI1->CR1 |= SPI_CR1_SPE;
	SPI1->CR2 = (0b1111 << SPI_CR2_DS_Pos); // 16 bits data size
	// SPI1->CR2 = 0;
}

void SPI2_Init(void) {
	__HAL_RCC_SPI2_CLK_ENABLE();  // Activer l'horloge SPI1

	SPI2->CR1 = (0b11 << SPI_CR1_BR_Pos); //Baud rate F_Pclk / 16 => 48Mhz / 16 => 3Mhz
	// SPI2->CR1 |= SPI_CR1_DFF;
	SPI2->CR1 |= (SPI_CR1_SSM | SPI_CR1_SSI); //Software slave management + Slave select High
	SPI2->CR1 |= SPI_CR1_MSTR; //Master mode
	SPI2->CR1 |= SPI_CR1_SPE; //Enable
	SPI2->CR2 = (0b1011 << SPI_CR2_DS_Pos); // 12 bits serial data
	// SPI2->CR2 = 0;
}

void	USART1_init(void) {
	USART1->CR1 = USART_CR1_OVER8;
	USART1->BRR = 0X10;
	USART1->CR2 = USART_CR2_MSBFIRST;
	USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
	// USART1->CR1 |= USART_CR1_TCIE | USART_CR1_RXNEIE;
	USART1->CR3 = USART_CR3_DMAR;
}

void	init_usart_dma(void) {
	DMA1_Channel3->CCR = DMA_CCR_PL_1 //Priority high
						| DMA_CCR_MINC // increment on
						| DMA_CCR_CIRC; // Circular buffer
	DMA1_Channel3->CNDTR = sizeof(colors);
	DMA1_Channel3->CPAR = &USART1->RDR;
	DMA1_Channel1->CMAR = &colors[0];
	DMA1_Channel1->CCR = DMA_CCR_EN;
}


void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	// 1. Configurer l'oscillateur principal
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE; // Utiliser HSE
	RCC_OscInitStruct.HSEState = RCC_HSE_ON; // Activer HSE
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON; // Activer PLL
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE; // Source PLL = HSE
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9; // Multiplier par 9 (8 MHz * 9 = 72 MHz)
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		// Gestion d'erreur
		while (1);
	}

	// 2. Configurer les prescalers pour les bus AHB, APB1 et APB2
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
								RCC_CLOCKTYPE_PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; // Source SYSCLK = PLL
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1; // AHB = /1
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2; // APB1 = /2 (max 36 MHz)
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
		// Gestion d'erreur
		while (1);
	}
}