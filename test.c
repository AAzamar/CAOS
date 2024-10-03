
// Código para PRNG
#include <stdio.h>
#include "pico/stdlib.h"

// Definición de parámetros
#define G 10.0f
#define P 28.0f
#define B (8.0f / 3.0f)
#define H 0.02f    // Paso de Integración
#define N_SOL (256*2)-1  // Número de soluciones
#define COMPARADOR 25.8821f
#define FR 16      // Factor para orden de nbit

float x[N_SOL], y[N_SOL], z[N_SOL];
int bitstream[N_SOL];
int nbit[N_SOL];

int main() {
    stdio_init_all();
    while (true) {
        x[0] = 3.0f;
        y[0] = 7.0f;
        z[0] = 1.0f;
        for (int t = 0; t < N_SOL; t++) {
            // Método numérico de Euler
            x[t + 1] = x[t] + H * (G * (y[t] - x[t]));
            y[t + 1] = y[t] + H * (x[t] * (P - z[t]) - y[t]);
            z[t + 1] = z[t] + H * (x[t] * y[t] - B * z[t]);
            // Generación del bitstream según la comparación con z(t)
            if (z[t] > COMPARADOR) {
                bitstream[t] = 1;
            } else {
                bitstream[t] = 0;
            }
        }
        // Procesamiento para llenar nbit
        int a = 0, ii = 0;
        for (int i = 0; i < N_SOL / FR; i++) {
            for (int p = 0; p < FR; p++) {
                nbit[((N_SOL / FR) * p) - a] = bitstream[ii];
                ii++;
            }
            a++;
        }
        
        printf("bitstream: ");
        for (int i = 0; i < N_SOL; i++) {
            printf("%d", nbit[i]);
        }
        printf("\n");
        sleep_ms(3000);
    }
}



/*
// Código para medir la taza de bytes por segundo
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/timer.h"

#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define BUFFER_SIZE 10
volatile uint8_t buffer[BUFFER_SIZE];
volatile uint32_t byte_count = 0;  // Contador de bytes recibidos

int dma_chan;
bool print_byte_count(repeating_timer_t *rt) {
    printf("Bytes recibidos: %u\n", byte_count);
    byte_count = 0;
    return true;
}
// Interrupción de UART para capturar los datos entrantes y transferirlos al buffer
void on_uart_rx() {
    while (uart_is_readable(UART_ID)) {
        uint8_t byte = uart_getc(UART_ID);
        buffer[byte_count % BUFFER_SIZE] = byte;
        byte_count++;
    }
}
int main() {
    stdio_init_all();
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_ID, true);
    irq_set_exclusive_handler(UART0_IRQ, on_uart_rx);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);
    repeating_timer_t timer;
    add_repeating_timer_ms(1000, print_byte_count, NULL, &timer);
    while (1) {
        tight_loop_contents();
    }
}
*/



/*
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#define CAPTURE_CHANNEL 0
#define CAPTURE_DEPTH 252 // 252  0x01 0x0A bytes, corresponde a 126 muestras del ADC 4bytes 0x00 0x12 lenght
uint8_t capture_buf[CAPTURE_DEPTH];
#define UART_ID uart0
#define BAUD_RATE 921600
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define LED_PIN 25
uint16_t destination = 0x0490;
uint8_t api_header[] = {0x7E,0x01,0x0A,0x10,0x00,0x00,0x13,0xA2,0x00,0x42,0x3D,0x8B,0xC1,0xFF,0xFE,0x03,0x00};
size_t header_length = sizeof(api_header);
uint32_t accumulator = 0x00000000;
uint dma_chan;

void dma_capture_and_accumulate() {
    adc_gpio_init(26 + CAPTURE_CHANNEL);
    adc_init();
    adc_select_input(CAPTURE_CHANNEL);
    adc_fifo_setup(true, true, 1, true, false);  // FCS.EN y FCS.DREQ_EN habilitados, umbral en 1
    adc_set_clkdiv(0); // Configurar para la máxima velocidad
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8); // Transferencia de 8 bits (1 byte)
    channel_config_set_read_increment(&cfg, false);          // No incrementar al leer el FIFO
    channel_config_set_write_increment(&cfg, true);          // Incrementar al escribir en el buffer
    channel_config_set_dreq(&cfg, DREQ_ADC);                 // Sincronizar con el ADC
    channel_config_set_sniff_enable(&cfg, true);             // Habilitar el sniffer
    dma_sniffer_set_data_accumulator(accumulator);                     // Iniciar acumulador en 0
    dma_sniffer_enable(dma_chan, 0xf, true); // Habilitar suma en 32 bits  DMA_SNIFF_CTRL_CALC_VALUE_CRC32R
    dma_channel_configure(
        dma_chan,              
        &cfg,                  
        capture_buf,           
        &adc_hw->fifo,         
        CAPTURE_DEPTH,         
        true                   
    );
    adc_run(true);
    dma_channel_wait_for_finish_blocking(dma_chan); 
    adc_run(false);                                
    adc_fifo_drain();                              
}

uint8_t calculate_checksum(uint32_t accumulator, uint16_t destination) {
    uint32_t total_sum = accumulator + destination;
    uint8_t lower_8_bits = total_sum & 0xFF;
    return 0xFF - lower_8_bits;
}

void print_data(const uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

// Averiguar si con el DMA se pueden poner los datos de dabecera, data y checksum
// en el registro FIFO del UART para su rápida impresión.
void send_api_frame(uint8_t *data, size_t data_length, uint8_t checksum) {
    size_t total_length = header_length + data_length + 1;
    uint8_t api_frame[total_length];
    for (size_t i = 0; i < header_length; ++i) {
        api_frame[i] = api_header[i];
    }
    for (size_t i = 0; i < data_length; ++i) {
        api_frame[header_length + i] = data[i];
    }
    api_frame[total_length - 1] = checksum;
    gpio_put(LED_PIN, 0);
    uart_write_blocking(UART_ID, api_frame, total_length);  // Envía el frame
    uart_write_blocking(UART_ID, api_frame, total_length);
    uart_write_blocking(UART_ID, api_frame, total_length);
    uart_write_blocking(UART_ID, api_frame, total_length);
    uart_write_blocking(UART_ID, api_frame, total_length);
    uart_write_blocking(UART_ID, api_frame, total_length);
    uart_write_blocking(UART_ID, api_frame, total_length);
    uart_write_blocking(UART_ID, api_frame, total_length);
    uart_write_blocking(UART_ID, api_frame, total_length);
    uart_write_blocking(UART_ID, api_frame, total_length);
    uart_write_blocking(UART_ID, api_frame, total_length);
    uart_write_blocking(UART_ID, api_frame, total_length);
    uart_write_blocking(UART_ID, api_frame, total_length);
    uart_write_blocking(UART_ID, api_frame, total_length);
    uart_write_blocking(UART_ID, api_frame, total_length);
    //printf("api_frame: ");
    //print_data(api_frame, total_length);
    gpio_put(LED_PIN, 1);
}

int main() {
    stdio_init_all();
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    dma_chan = dma_claim_unused_channel(true);

    while (1) {
        dma_capture_and_accumulate();
        accumulator = dma_sniffer_get_data_accumulator();
        uint8_t checksum = calculate_checksum(accumulator, destination);
        //printf("Accumulator: %08X\n", accumulator);
        //printf("Checksum: %02X\n", checksum);
        dma_sniffer_set_data_accumulator(0x00000000);
        accumulator = dma_sniffer_get_data_accumulator();
        //printf("Accumulator empty: %08X\n", accumulator);
        send_api_frame(capture_buf, CAPTURE_DEPTH, checksum);
    }
}
*/





/*
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define UART_ID uart0
#define BAUD_RATE 9600
#define UART_TX_PIN 0
#define UART_RX_PIN 1

int main() {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uint8_t api_frame[] = {
        0x7E, 0x00, 0x1B, 0x10, 0x01, 0x00, 0x13, 0xA2, 0x00, 0x42, 0x3D, 0x8B, 0xC1, 0xFF, 0xFE, 0x02,
        0x00, 0x68, 0x6F, 0x6C, 0x61, 0x20, 0x6D, 0x75, 0x6E, 0x64, 0x6F, 0x20, 0x3A, 0x29, 0x05
    };
    size_t frame_length = sizeof(api_frame);
    
    while (true) {
        uart_write_blocking(UART_ID, api_frame, frame_length);
        sleep_ms(2000);
    }
}
*/