// #include "main_conf.h"
#include "midi.h"

// #include "stm32f4xx.h"
// #include "stm32f4xx_hal.h"

// midi subsystem is:
// 2uart + i2c + usb_device + control_panel

//this is hardware glue file
//do not forget to put midi daemons inside control rate routine (midi_update())

MidiInUartContextT u1rx_ram;
MidiOutUartContextT u1tx_ram;
MidiOutUartContextT u3tx_ram;

MidiOutPortContextT u1tx_port;
MidiOutPortContextT u3tx_port;
MidiOutPortContextT i2c1tx_port;
MidiOutPortContextT usbtx_port;

MidiInPortT u1midi_in = {
    "MIDI IN",
    MIDI_TYPE_UART,
    MIDI_CN_UART1,
    &u1rx_ram
};

void u1_start_send()
{
    USART1->CR1 |= USART_CR1_TXEIE;
}
void u1_stop_send()
{
    USART1->CR1 &= ~USART_CR1_TXEIE;
}
uint8_t u1_is_busy()
{
    return (USART1->CR1 & USART_CR1_TXEIE);
}
void u1_sendbyte(uint8_t b)
{
    USART1->DR = b;
}

MidiOutUartApiT u1_api = {
    u1_start_send,
    u1_stop_send,
    u1_is_busy,
    u1_sendbyte
};

void u3_start_send()
{
    USART3->CR1 |= USART_CR1_TXEIE;
}
void u3_stop_send()
{
    USART3->CR1 &= ~USART_CR1_TXEIE;
}
uint8_t u3_is_busy()
{
    return (USART3->CR1 & USART_CR1_TXEIE);
}
void u3_sendbyte(uint8_t b)
{
    USART3->DR = b;
}

MidiOutUartApiT u3_api = {
    u3_start_send,
    u3_stop_send,
    u3_is_busy,
    u3_sendbyte
};

MidiOutPortT u1midi_out = {
    "MIDI OUT 1",
    MIDI_TYPE_UART, //type
    MIDI_CN_UART1,
    &u1tx_port,
    &u1tx_ram,
    &u1_api
};

MidiOutPortT u3midi_out = {
    "MIDI OUT 2",
    MIDI_TYPE_UART, //type
    MIDI_CN_UART2,
    &u3tx_port,
    &u3tx_ram,
    &u3_api
};

MidiOutPortT usbmidi_out = {
    "MIDI OUT USB",
    MIDI_TYPE_USB,
    MIDI_CN_USBDEVICE1,
    &usbtx_port,
    0,
    0
};

MidiOutPortT panelmidi_out = {
    "EXT PANEL",
    MIDI_TYPE_IIC,
    MIDI_CN_CONTROLPANEL,
    &i2c1tx_port,
    0,
    0
};

void midi_start()
{
    GPIO_InitTypeDef GPIO_InitStruct;
    // PA9  - UART1_TX midi1
    // PA10 - UART1_RX midi1
    // PD8  - UART3_TX midi2
    // PB6  - I2C1 SCL panel
    // PB7  - I2C1 SDA panel
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    //over16 mode
    USART1->BRR = 0x00000A80;
    USART1->CR1 |= USART_CR1_UE | USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE;
    USART1->CR3 |= USART_CR3_EIE;

    USART3->BRR = 0x00000540;
    USART3->CR1 |= (USART_CR1_UE | USART_CR1_TE);
    USART3->CR3 |= USART_CR3_EIE;

    // TODO: add i2c panel IO

    // init data structures

    //init global sink input
    midiInit();

    //init connections
    midiInUartInit(&u1midi_in);
    midiOutUartInit(&u1midi_out);
    midiOutUartInit(&u3midi_out);
    midiPortInit(&usbmidi_out);
    midiPortInit(&panelmidi_out);

    // hw start
    NVIC_SetPriority(USART1_IRQn, IRQ_PRIORITY_UARTMIDI);
    NVIC_EnableIRQ(USART1_IRQn);

    NVIC_SetPriority(USART3_IRQn, IRQ_PRIORITY_UARTMIDI);
    NVIC_EnableIRQ(USART3_IRQn);
}

void USART1_IRQHandler(void)
{
    if (USART1->SR & USART_SR_RXNE) {
        midiInUartByteReceiveCallback(USART1->DR, &u1midi_in);
    }
    if (USART1->SR & USART_SR_TXE) {
        midiOutUartTranmissionCompleteCallback(&u1midi_out);
    }
}

void USART3_IRQHandler(void)
{
    // no RX, only TX
    if (USART3->SR & USART_SR_TXE) {
        midiOutUartTranmissionCompleteCallback(&u3midi_out);
    }
}

void midi_update()
{
    midiInUartTap(&u1midi_in);
    midiOutUartTap(&u1midi_out);
    midiOutUartTap(&u3midi_out);
    // i2c panel - probably in SR interrupt with main panel
}
