/*
 * Rad Pro
 * FNIRSI GC-01 specifics
 *
 * (C) 2022-2024 Gissio
 *
 * License: MIT
 */

#if defined(GC01)

#include "stm32.h"

#include "../display.h"
#include "../events.h"
#include "../flash.h"
#include "../keyboard.h"
#include "../settings.h"
#include "../system.h"
#include "../tube.h"

#include "device.h"

#include "mcu-renderer-st7789.h"

// System

void initSystem(void)
{
    // Set stack pointer to fix bootloader madness
    __set_MSP(*((uint32_t *)FIRMWARE_BASE));

    // Enable HSE
    set_bits(RCC->CR,
             RCC_CR_HSEON);
    wait_until_bits_set(RCC->CR,
                        RCC_CR_HSERDY);

    // Set 2 wait states for flash
    modify_bits(FLASH->ACR,
                FLASH_ACR_LATENCY_Msk,
                FLASH_ACR_LATENCY_2WS);

    // Configure AHB, APB1, APB2, ADC, PLL
    RCC->CFGR =
        RCC_CFGR_SW_HSI |        // Select HSI as system clock
        RCC_CFGR_HPRE_DIV1 |     // Set AHB clock: 72 MHz / 1 = 72 MHz
        RCC_CFGR_PPRE1_DIV2 |    // Set APB1 clock: 72 MHz / 2 = 36 MHz
        RCC_CFGR_PPRE2_DIV1 |    // Set APB2 clock: 72 MHz / 1 = 72 MHz
        RCC_CFGR_ADCPRE_DIV8 |   // Set ADC clock: 72 MHz / 8 = 9 MHz
        RCC_CFGR_PLLSRC_HSE |    // Set PLL source: HSE
        RCC_CFGR_PLLXTPRE_HSE |  // Set PLL HSE predivision factor: /1
        RCC_CFGR_PLLMULL9 |      // Set PLL multiplier: 9x
        RCC_CFGR_USBPRE_DIV1_5 | // Set USB prescaler: 1.5x
        RCC_CFGR_MCO_NOCLOCK;    // Disable MCO

    // Enable PLL
    set_bits(RCC->CR,
             RCC_CR_PLLON);
    wait_until_bits_set(RCC->CR,
                        RCC_CR_PLLRDY);

    // Set PLL as system clock
    modify_bits(RCC->CFGR,
                RCC_CFGR_SW_Msk,
                RCC_CFGR_SW_PLL);
    wait_until_bits_value(RCC->CFGR,
                          RCC_CFGR_SWS_Msk,
                          RCC_CFGR_SWS_PLL);

    // Set vector table
    NVIC_DisableAllIRQs();
    SCB->VTOR = FIRMWARE_BASE;

    // Disable JTAG
    rcc_enable_afio();
#if !defined(GC01_USART)
    modify_bits(AFIO->MAPR,
                AFIO_MAPR_SWJ_CFG_Msk,
                AFIO_MAPR_SWJ_CFG_JTAGDISABLE);
#else
    modify_bits(AFIO->MAPR,
                AFIO_MAPR_SWJ_CFG_Msk,
                AFIO_MAPR_SWJ_CFG_JTAGDISABLE |
                    AFIO_MAPR_USART1_REMAP);
#endif

    // Enable GPIOA, GPIOB, GPIOC
    set_bits(RCC->APB2ENR,
             RCC_APB2ENR_IOPAEN |
                 RCC_APB2ENR_IOPBEN |
                 RCC_APB2ENR_IOPCEN);
}

// Communications

#if defined(CH32)
const char *const commId = "FNIRSI GC-01 (CH32F103C8);" FIRMWARE_NAME " " FIRMWARE_VERSION;
#elif defined(APM32)
const char *const commId = "FNIRSI GC-01 (APM32F103CB);" FIRMWARE_NAME " " FIRMWARE_VERSION;
#endif

// Keyboard

void initKeyboardController(void)
{
    // GPIO
    gpio_setup(KEY_LEFT_PORT,
               KEY_LEFT_PIN,
               GPIO_MODE_INPUT_PULLUP);
    gpio_setup(KEY_RIGHT_PORT,
               KEY_RIGHT_PIN,
               GPIO_MODE_INPUT_PULLUP);
    gpio_setup(KEY_UP_PORT,
               KEY_UP_PIN,
               GPIO_MODE_INPUT_PULLUP);
    gpio_setup(KEY_DOWN_PORT,
               KEY_DOWN_PIN,
               GPIO_MODE_INPUT_PULLUP);
    gpio_setup(KEY_OK_PORT,
               KEY_OK_PIN,
               GPIO_MODE_INPUT_PULLUP);
}

void getKeyboardState(bool *isKeyDown)
{
    isKeyDown[KEY_LEFT] = !gpio_get(KEY_LEFT_PORT, KEY_LEFT_PIN);
    isKeyDown[KEY_RIGHT] = !gpio_get(KEY_RIGHT_PORT, KEY_RIGHT_PIN);
    isKeyDown[KEY_UP] = !gpio_get(KEY_UP_PORT, KEY_UP_PIN);
    isKeyDown[KEY_DOWN] = !gpio_get(KEY_DOWN_PORT, KEY_DOWN_PIN);
    isKeyDown[KEY_OK] = !gpio_get(KEY_OK_PORT, KEY_OK_PIN);
}

// Display

extern mr_t mr;

bool displayOn;

static uint8_t displayTextbuffer[86 * 86];

static const uint8_t displayCL24CG104540BInitSequence[] = {
    MR_SEND_COMMAND(MR_ST7789_RAMCTRL),
    MR_SEND_DATA(0x00),
    MR_SEND_DATA(0xe0),
    MR_SEND_COMMAND(MR_ST7789_PORCTRL),
    MR_SEND_DATA(0x0c),
    MR_SEND_DATA(0x0c),
    MR_SEND_DATA(0x00),
    MR_SEND_DATA(0x33),
    MR_SEND_DATA(0x33),
    MR_SEND_COMMAND(MR_ST7789_GCTRL),
    MR_SEND_DATA(0x35), // VGLS=-10.43 V, VGHS=13.26 V
    MR_SEND_COMMAND(MR_ST7789_VCOMS),
    MR_SEND_DATA(0x28), // VCOM=1.1 V
    MR_SEND_COMMAND(MR_ST7789_VRHS),
    MR_SEND_DATA(0x0b), // VRH=4.1 V
    MR_SEND_COMMAND(MR_ST7789_FRCTRL2),
    MR_SEND_DATA(0x0f), // 60 Hz
    MR_SEND_COMMAND(MR_ST7789_PWCTRL1),
    MR_SEND_DATA(0xa4), // AVDD=6.8 V, AVCL=-4.8 V, VDDS=2.3 V
    MR_SEND_DATA(0xa1),
    MR_SEND_COMMAND(MR_ST7789_PVGAMCTRL),
    MR_SEND_DATA(0xd0),
    MR_SEND_DATA(0x01),
    MR_SEND_DATA(0x08),
    MR_SEND_DATA(0x0f),
    MR_SEND_DATA(0x11),
    MR_SEND_DATA(0x2a),
    MR_SEND_DATA(0x36),
    MR_SEND_DATA(0x55),
    MR_SEND_DATA(0x44),
    MR_SEND_DATA(0x3a),
    MR_SEND_DATA(0x0b),
    MR_SEND_DATA(0x06),
    MR_SEND_DATA(0x11),
    MR_SEND_DATA(0x20),
    MR_SEND_COMMAND(MR_ST7789_NVGAMCTRL),
    MR_SEND_DATA(0xd0),
    MR_SEND_DATA(0x02),
    MR_SEND_DATA(0x07),
    MR_SEND_DATA(0x0a),
    MR_SEND_DATA(0x0b),
    MR_SEND_DATA(0x18),
    MR_SEND_DATA(0x34),
    MR_SEND_DATA(0x43),
    MR_SEND_DATA(0x4a),
    MR_SEND_DATA(0x2b),
    MR_SEND_DATA(0x1b),
    MR_SEND_DATA(0x1c),
    MR_SEND_DATA(0x22),
    MR_SEND_DATA(0x1f),
    MR_SEND_COMMAND(MR_ST7789_PWCTRL2),
    MR_SEND_DATA(0x93), // SBCLK DIV 3, BCLK DIV 6
    MR_SEND_COMMAND(MR_ST7789_EQCTRL),
    MR_SEND_DATA(0x11), // SEQ=6.8 µs
    MR_SEND_DATA(0x11), // SPRET=6.8 µs
    MR_SEND_DATA(0x08), // GEQ=3.2 µs
    MR_END(),
};

static const uint8_t displayENHTV0240B026InitSequence[] = {
    MR_SEND_COMMAND(MR_ST7789_RAMCTRL),
    MR_SEND_DATA(0x00),
    MR_SEND_DATA(0xe0),
    MR_SEND_COMMAND(MR_ST7789_PORCTRL),
    MR_SEND_DATA(0x0c),
    MR_SEND_DATA(0x0c),
    MR_SEND_DATA(0x00),
    MR_SEND_DATA(0x33),
    MR_SEND_DATA(0x33),
    MR_SEND_COMMAND(MR_ST7789_GCTRL),
    MR_SEND_DATA(0x35), // VGLS=-10.43 V, VGHS=13.26 V
    MR_SEND_COMMAND(MR_ST7789_VCOMS),
    MR_SEND_DATA(0x36), // VCOM=1.45 V
    MR_SEND_COMMAND(MR_ST7789_VRHS),
    MR_SEND_DATA(0x12), // VRH=4.45 V
    MR_SEND_COMMAND(MR_ST7789_FRCTRL2),
    MR_SEND_DATA(0x0f), // 60 Hz
    MR_SEND_COMMAND(MR_ST7789_PWCTRL1),
    MR_SEND_DATA(0xa4), // AVDD=6.8 V, AVCL=-4.8 V, VDDS=2.3 V
    MR_SEND_DATA(0xa1),
    MR_SEND_COMMAND(MR_ST7789_PVGAMCTRL),
    MR_SEND_DATA(0xd0),
    MR_SEND_DATA(0x00),
    MR_SEND_DATA(0x05),
    MR_SEND_DATA(0x0e),
    MR_SEND_DATA(0x15),
    MR_SEND_DATA(0x0d),
    MR_SEND_DATA(0x37),
    MR_SEND_DATA(0x43),
    MR_SEND_DATA(0x47),
    MR_SEND_DATA(0x09),
    MR_SEND_DATA(0x15),
    MR_SEND_DATA(0x12),
    MR_SEND_DATA(0x16),
    MR_SEND_DATA(0x19),
    MR_SEND_COMMAND(MR_ST7789_NVGAMCTRL),
    MR_SEND_DATA(0xd0),
    MR_SEND_DATA(0x00),
    MR_SEND_DATA(0x05),
    MR_SEND_DATA(0x0d),
    MR_SEND_DATA(0x0c),
    MR_SEND_DATA(0x06),
    MR_SEND_DATA(0x2d),
    MR_SEND_DATA(0x44),
    MR_SEND_DATA(0x40),
    MR_SEND_DATA(0x0e),
    MR_SEND_DATA(0x1c),
    MR_SEND_DATA(0x18),
    MR_SEND_DATA(0x16),
    MR_SEND_DATA(0x19),
    MR_SEND_COMMAND(MR_ST7789_PWCTRL2),
    MR_SEND_DATA(0x93), // SBCLK DIV 3, BCLK DIV 6
    MR_SEND_COMMAND(MR_ST7789_EQCTRL),
    MR_SEND_DATA(0x11), // SEQ=6.8 µs
    MR_SEND_DATA(0x11), // SPRET=6.8 µs
    MR_SEND_DATA(0x08), // GEQ=3.2 µs
    MR_END(),
};

static const uint8_t displayGMT02405V20InitSequence[] = {
    MR_SEND_COMMAND(MR_ST7789_RAMCTRL),
    MR_SEND_DATA(0x00),
    MR_SEND_DATA(0xe0),
    MR_SEND_COMMAND(MR_ST7789_PORCTRL),
    MR_SEND_DATA(0x05),
    MR_SEND_DATA(0x05),
    MR_SEND_DATA(0x00),
    MR_SEND_DATA(0x33),
    MR_SEND_DATA(0x33),
    MR_SEND_COMMAND(MR_ST7789_GCTRL),
    MR_SEND_DATA(0x05), // VGLS=-10.43 V, VGHS=12.2 V
    MR_SEND_COMMAND(MR_ST7789_VCOMS),
    MR_SEND_DATA(0x39), // VCOM=1.525 V
    MR_SEND_COMMAND(MR_ST7789_VRHS),
    MR_SEND_DATA(0x0f), // VRH=4.3 V
    MR_SEND_COMMAND(MR_ST7789_FRCTRL2),
    MR_SEND_DATA(0x01), // 111 Hz
    MR_SEND_COMMAND(MR_ST7789_PWCTRL1),
    MR_SEND_DATA(0xa4), // AVDD=6.8 V, AVCL=-4.8 V, VDDS=2.3 V
    MR_SEND_DATA(0xa1),
    MR_SEND_COMMAND(MR_ST7789_PVGAMCTRL),
    MR_SEND_DATA(0xd0),
    MR_SEND_DATA(0x05),
    MR_SEND_DATA(0x09),
    MR_SEND_DATA(0x09),
    MR_SEND_DATA(0x08),
    MR_SEND_DATA(0x14),
    MR_SEND_DATA(0x28),
    MR_SEND_DATA(0x33),
    MR_SEND_DATA(0x3f),
    MR_SEND_DATA(0x07),
    MR_SEND_DATA(0x13),
    MR_SEND_DATA(0x14),
    MR_SEND_DATA(0x28),
    MR_SEND_DATA(0x30),
    MR_SEND_COMMAND(MR_ST7789_NVGAMCTRL),
    MR_SEND_DATA(0xd0),
    MR_SEND_DATA(0x05),
    MR_SEND_DATA(0x09),
    MR_SEND_DATA(0x09),
    MR_SEND_DATA(0x08),
    MR_SEND_DATA(0x03),
    MR_SEND_DATA(0x24),
    MR_SEND_DATA(0x32),
    MR_SEND_DATA(0x32),
    MR_SEND_DATA(0x3B),
    MR_SEND_DATA(0x14),
    MR_SEND_DATA(0x13),
    MR_SEND_DATA(0x28),
    MR_SEND_DATA(0x2f),
    MR_SEND_COMMAND(MR_ST7789_PWCTRL2),
    MR_SEND_DATA(0x03), // SBCLK DIV 2, BCLK DIV 6
    MR_SEND_COMMAND(MR_ST7789_EQCTRL),
    MR_SEND_DATA(0x09), // SEQ=3.6 µs
    MR_SEND_DATA(0x09), // SPRET=3.6 µs
    MR_SEND_DATA(0x08), // GEQ=3.2 µs
    MR_END(),
};

#if defined(GC01_DISPLAY_SPI)
static const uint8_t displayIPSInitSequence[] = {
    MR_SEND_COMMAND(MR_ST7789_INVON), // Inverse for IPS displays

    MR_END(),
};
#endif

void updateDisplayPanel(void)
{
    switch (settings.displayPanel)
    {
    case 0:
        mr_send_sequence(&mr,
                         displayCL24CG104540BInitSequence);

        break;

    case 1:
        mr_send_sequence(&mr,
                         displayENHTV0240B026InitSequence);

        break;

    case 2:
        mr_send_sequence(&mr,
                         displayGMT02405V20InitSequence);

        break;
    }

#if defined(GC01_DISPLAY_SPI)
    mr_send_sequence(&mr,
                     displayIPSInitSequence);
#endif
}

static void onDisplaySleep(uint32_t value)
{
    sleep(value);
}

static void onDisplaySetReset(bool value)
{
    gpio_modify(DISPLAY_RESX_PORT,
                DISPLAY_RESX_PIN,
                !value);
}

#if defined(GC01_DISPLAY_SPI)

static void onDisplaySetCommand(bool value)
{
    if (value)
    {
        // Trigger CS before command
        gpio_set(DISPLAY_CSX_PORT, DISPLAY_CSX_PIN);
        gpio_clear(DISPLAY_CSX_PORT, DISPLAY_CSX_PIN);
    }

    gpio_modify(DISPLAY_DCX_PORT,
                DISPLAY_DCX_PIN,
                !value);
}

static void onDisplaySend(uint16_t value)
{
    spi_send(SPI1, value);
}

static void onDisplaySend16(uint16_t value)
{
    spi_send(SPI1, (value >> 8) & 0xff);
    spi_send(SPI1, (value >> 0) & 0xff);
}

#else

static void onDisplaySetCommand(bool value)
{
    gpio_modify(DISPLAY_DCX_PORT,
                DISPLAY_DCX_PIN,
                !value);
}

static void onDisplaySend(uint16_t value)
{
    DISPLAY_WRX_PORT->BRR = get_bitvalue(DISPLAY_WRX_PIN);
    DISPLAY_DATA_PORT->ODR = value;
    DISPLAY_WRX_PORT->BSRR = get_bitvalue(DISPLAY_WRX_PIN);
}

#endif

void initDisplayController(void)
{
#if defined(GC01_DISPLAY_SPI)
    // GPIO
    gpio_set(DISPLAY_RESX_PORT,
             DISPLAY_RESX_PIN);
    gpio_set(DISPLAY_CSX_PORT,
             DISPLAY_CSX_PIN);

    gpio_setup(DISPLAY_RESX_PORT,
               DISPLAY_RESX_PIN,
               GPIO_MODE_OUTPUT_50MHZ_PUSHPULL);
    gpio_setup(DISPLAY_CSX_PORT,
               DISPLAY_CSX_PIN,
               GPIO_MODE_OUTPUT_50MHZ_PUSHPULL);
    gpio_setup(DISPLAY_DCX_PORT,
               DISPLAY_DCX_PIN,
               GPIO_MODE_OUTPUT_50MHZ_PUSHPULL);
    gpio_setup(DISPLAY_SCL_PORT,
               DISPLAY_SCL_PIN,
               GPIO_MODE_OUTPUT_50MHZ_AF_PUSHPULL);
    gpio_setup(DISPLAY_SDA_PORT,
               DISPLAY_SDA_PIN,
               GPIO_MODE_OUTPUT_50MHZ_AF_PUSHPULL);

    set_bits(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);

    // SPI
    SPI1->CR1 = SPI_CR1_CPHA |
                SPI_CR1_CPOL |
                SPI_CR1_MSTR |
                SPI_CR1_SSI |
                SPI_CR1_SSM;
    set_bits(SPI1->CR1,
             SPI_CR1_SPE);
#else
    // GPIO
    gpio_set(DISPLAY_RESX_PORT,
             DISPLAY_RESX_PIN);
    gpio_set(DISPLAY_RDX_PORT,
             DISPLAY_RDX_PIN);
    gpio_set(DISPLAY_WRX_PORT,
             DISPLAY_WRX_PIN);

    gpio_setup(DISPLAY_RESX_PORT,
               DISPLAY_RESX_PIN,
               GPIO_MODE_OUTPUT_50MHZ_PUSHPULL);
    gpio_setup(DISPLAY_RDX_PORT,
               DISPLAY_RDX_PIN,
               GPIO_MODE_OUTPUT_50MHZ_PUSHPULL);
    gpio_setup(DISPLAY_WRX_PORT,
               DISPLAY_WRX_PIN,
               GPIO_MODE_OUTPUT_50MHZ_PUSHPULL);
    gpio_setup(DISPLAY_DCX_PORT,
               DISPLAY_DCX_PIN,
               GPIO_MODE_OUTPUT_50MHZ_PUSHPULL);
    gpio_setup(DISPLAY_CSX_PORT,
               DISPLAY_CSX_PIN,
               GPIO_MODE_OUTPUT_50MHZ_PUSHPULL);

    DISPLAY_DATA_PORT->CRL =
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 0) |
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 4) |
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 8) |
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 12) |
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 16) |
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 20) |
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 24) |
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 28);
    DISPLAY_DATA_PORT->CRH =
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 0) |
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 4) |
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 8) |
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 12) |
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 16) |
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 20) |
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 24) |
        (GPIO_MODE_OUTPUT_50MHZ_PUSHPULL << 28);
#endif

    // mcu-renderer
#if defined(GC01_DISPLAY_SPI)
    mr_st7789_init(&mr,
                   240,
                   320,
                   MR_DISPLAY_ROTATION_270,
                   displayTextbuffer,
                   sizeof(displayTextbuffer),
                   onDisplaySleep,
                   onDisplaySetReset,
                   onDisplaySetCommand,
                   onDisplaySend,
                   onDisplaySend16);
#else
    mr_st7789_init(&mr,
                   240,
                   320,
                   MR_DISPLAY_ROTATION_270,
                   displayTextbuffer,
                   sizeof(displayTextbuffer),
                   onDisplaySleep,
                   onDisplaySetReset,
                   onDisplaySetCommand,
                   onDisplaySend,
                   onDisplaySend);
#endif

    updateDisplayPanel();
}

void setDisplayOn(bool value)
{
    displayOn = value;

    mr_st7789_set_display(&mr, value);
    mr_st7789_set_sleep(&mr, !value);
}

bool isDisplayOn(void)
{
    return displayOn;
}

void refreshDisplay(void)
{
}

#endif
