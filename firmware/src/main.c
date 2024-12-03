#include "stm32f1xx_hal.h"

#define LED_PIN                                GPIO_PIN_13
#define LED_GPIO_PORT                          GPIOC
#define LED_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOC_CLK_ENABLE()

void LED_Init();

int main(void) {
  HAL_Init();
  LED_Init();

  while (1)
  {
    HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
    HAL_Delay(1000);
  }
}

void LED_Init() {
  LED_GPIO_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.Pin = LED_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL; // PC13 n'a pas besoin de pull-up ou pull-down
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // Low speed suffisant pour une LED
  HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);
}

void SysTick_Handler(void) {
  HAL_IncTick();
}