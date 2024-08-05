#include "stm8s.h"

// Pin definitions
#define CS_PIN     GPIO_PIN_3   // PC3 as CS
#define SCK_PIN    GPIO_PIN_5   // PB5 as SCK
#define MOSI_PIN   GPIO_PIN_4   // PD4 as MOSI
#define MISO_PIN   GPIO_PIN_6   // PC6 as MISO
#define AUDIO_OUT  GPIO_PIN_6   // PD6 as audio output

// Port definitions
#define CS_PORT    GPIOC
#define SCK_PORT   GPIOB
#define MOSI_PORT  GPIOD
#define MISO_PORT  GPIOC
#define AUDIO_PORT GPIOD

#define BUFFER_SIZE 256
uint8_t audioBuffer[BUFFER_SIZE];
uint32_t currentAddress = 0;

void SPI_Init(void);
uint8_t SPI_Transfer(uint8_t data);
void Read_Audio_Data(uint32_t address, uint8_t *buffer, uint16_t length);
void Play_Audio(void);
void delay_ms(uint16_t ms);

int main(void) {
    // Initialize GPIO pins
    GPIO_Init(CS_PORT, CS_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(SCK_PORT, SCK_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(MOSI_PORT, MOSI_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(MISO_PORT, MISO_PIN, GPIO_MODE_IN_FL_NO_IT);
    GPIO_Init(AUDIO_PORT, AUDIO_OUT, GPIO_MODE_OUT_PP_LOW_FAST);

    // Initialize SPI
    SPI_Init();

    // Play audio forever
    while (1) {
        Play_Audio();
    }
}

void SPI_Init(void) {
    // Initialize SPI with settings
    SPI_DeInit();
    SPI_Init(SPI_FIRSTBIT_MSB, SPI_BAUDRATEPRESCALER_4, SPI_MODE_MASTER, 
             SPI_CLOCKPOLARITY_LOW, SPI_CLOCKPHASE_1EDGE, 
             SPI_DATADIRECTION_2LINES_FULLDUPLEX, SPI_NSS_SOFT, 0x07);
    SPI_Cmd(ENABLE);
}

uint8_t SPI_Transfer(uint8_t data) {
    // Send data via SPI and receive response
    SPI_SendData(data);
    while (SPI_GetFlagStatus(SPI_FLAG_TXE) == RESET);
    while (SPI_GetFlagStatus(SPI_FLAG_RXNE) == RESET);
    return SPI_ReceiveData();
}

void Read_Audio_Data(uint32_t address, uint8_t *buffer, uint16_t length) {
    GPIO_WriteLow(CS_PORT, CS_PIN);
    SPI_Transfer(0x03);  // Read command
    SPI_Transfer((address >> 16) & 0xFF);
    SPI_Transfer((address >> 8) & 0xFF);
    SPI_Transfer(address & 0xFF);
    for (uint16_t i = 0; i < length; i++) {
        buffer[i] = SPI_Transfer(0x00);
    }
    GPIO_WriteHigh(CS_PORT, CS_PIN);
}

void Play_Audio(void) {
    Read_Audio_Data(currentAddress, audioBuffer, BUFFER_SIZE);
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
        // Output audio data to PAM8403
        DAC_SetChannel1Data(DAC_ALIGN_8B_R, audioBuffer[i]);
        delay_ms(1);  // Adjust this delay for audio timing
    }
    currentAddress += BUFFER_SIZE;
    if (currentAddress >= 0x800000) {
        currentAddress = 0;
    }
}

void delay_ms(uint16_t ms) {
    // Simple delay function
    while (ms--) {
        for (uint16_t i = 0; i < 1600; i++) {
            __asm__("nop");
        }
    }
}
