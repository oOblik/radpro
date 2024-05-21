/*
 * Rad Pro
 * Bosean FS-600/FS-1000 specifics
 *
 * (C) 2022-2024 Gissio
 *
 * License: MIT
 */

#include "stm32-ext.h"

#define SYSCLK_FREQUENCY 48000000
#define AHB_FREQUENCY (SYSCLK_FREQUENCY / 2)
#define APB1_FREQUENCY (SYSCLK_FREQUENCY / 2)
#define APB2_FREQUENCY (SYSCLK_FREQUENCY / 2)
#define TIM_FREQUENCY (SYSCLK_FREQUENCY / 2)
#define LSE_FREQUENCY 32768
#define LSI_FREQUENCY 32000

#undef FLASH_SIZE
#define FLASH_SIZE 0x40000
#define FIRMWARE_BASE 0x08000000
#define FIRMWARE_SIZE 0xc000

#define PWR_EN_PORT GPIOB
#define PWR_EN_PIN 5
#define PWR_VCC_PORT GPIOC
#define PWR_VCC_PIN 6
#define PWR_BAT_CHANNEL ADC_VBAT_CHANNEL
#define PWR_BAT_SCALE_FACTOR 3.0F
#define PWR_EXTERNAL_PORT GPIOC
#define PWR_EXTERNAL_PIN 1
#define PWR_CHRG_PORT GPIOA
#define PWR_CHRG_PIN 8
#define PWR_CHRG_ACTIVE_LOW
#define PWR_STDBY_PORT GPIOC
#define PWR_STDBY_PIN 7
#define PWR_STDBY_ACTIVE_LOW

#define TUBE_HV_PORT GPIOB
#define TUBE_HV_PIN 7
#define TUBE_DET_PORT GPIOB
#define TUBE_DET_PIN 6
#define TUBE_DET_PULLUP
#define TUBE_DET_IRQ EXTI9_5_IRQn
#define TUBE_DET_IRQ_HANDLER EXTI9_5_IRQHandler
#define TUBE_DET_TIMER_MASTER TIM1
#define TUBE_DET_TIMER_SLAVE TIM15
#define TUBE_DET_TIMER_TRIGGER_CONNECTION 0

#define KEY_LEFT_PORT GPIOD
#define KEY_LEFT_PIN 2
#define KEY_OK_PORT GPIOC
#define KEY_OK_PIN 8
#define KEY_RIGHT_PORT GPIOC
#define KEY_RIGHT_PIN 12

#define BUZZ_PORT GPIOB
#define BUZZ_PIN 11

#define DISPLAY_POWER_PORT GPIOA
#define DISPLAY_POWER_PIN 12
#define DISPLAY_RESX_PORT GPIOB
#define DISPLAY_RESX_PIN 0
#define DISPLAY_CSX_PORT GPIOB
#define DISPLAY_CSX_PIN 1
#define DISPLAY_DCX_PORT GPIOB
#define DISPLAY_DCX_PIN 2
#define DISPLAY_WRX_PORT GPIOC
#define DISPLAY_WRX_PIN 4
#define DISPLAY_RDX_PORT GPIOC
#define DISPLAY_RDX_PIN 5
#define DISPLAY_D0_PORT GPIOA
#define DISPLAY_D0_PIN 0
#define DISPLAY_D1_PORT GPIOA
#define DISPLAY_D1_PIN 1
#define DISPLAY_D2_PORT GPIOA
#define DISPLAY_D2_PIN 2
#define DISPLAY_D3_PORT GPIOA
#define DISPLAY_D3_PIN 3
#define DISPLAY_D4_PORT GPIOA
#define DISPLAY_D4_PIN 4
#define DISPLAY_D5_PORT GPIOA
#define DISPLAY_D5_PIN 5
#define DISPLAY_D6_PORT GPIOA
#define DISPLAY_D6_PIN 6
#define DISPLAY_D7_PORT GPIOA
#define DISPLAY_D7_PIN 7
#define DISPLAY_BACKLIGHT_PORT GPIOB
#define DISPLAY_BACKLIGHT_PIN 10
#define DISPLAY_BACKLIGHT_AF GPIO_AF1
#define DISPLAY_BACKLIGHT_TIMER TIM2
#define DISPLAY_BACKLIGHT_TIMER_CHANNEL TIM_CH3
#define DISPLAY_BACKLIGHT_TIMER_FREQUENCY 1000
#define DISPLAY_BACKLIGHT_TIMER_PERIOD (TIM_FREQUENCY / DISPLAY_BACKLIGHT_TIMER_FREQUENCY)

#define PULSE_LED_PORT GPIOA
#define PULSE_LED_PIN 15
#define ALERT_LED_PORT GPIOB
#define ALERT_LED_PIN 3

#define VIBRATOR_PORT GPIOC
#define VIBRATOR_PIN 9

#define USART_INTERFACE USART1
#define USART_TX_PORT GPIOA
#define USART_TX_PIN 9
#define USART_TX_AF GPIO_AF7
#define USART_RX_PORT GPIOA
#define USART_RX_PIN 10
#define USART_RX_AF GPIO_AF7
#define USART_APB_FREQUENCY APB2_FREQUENCY
#define USART_IRQ USART1_IRQn
#define USART_IRQ_HANDLER USART1_IRQHandler
