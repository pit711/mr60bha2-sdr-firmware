#include "radar_link.h"
#include "sdkconfig.h"

#if CONFIG_ESP_SDR_RADAR_ENABLED
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "esp_rom_crc.h"
#include "burst_serial.h"

#define RADAR_UART UART_NUM_1
#define RADAR_RX_BUFFER 16384
#define RADAR_READ_MAX 4096

static bool ready;
static uint32_t total_rx, total_tx;
static uint8_t chunk[RADAR_READ_MAX];

void radar_link_init(void) {
    const uart_config_t config = {
        .baud_rate = CONFIG_ESP_SDR_RADAR_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    if (uart_param_config(RADAR_UART, &config) != ESP_OK) return;
    if (uart_set_pin(RADAR_UART, CONFIG_ESP_SDR_RADAR_TX_PIN, CONFIG_ESP_SDR_RADAR_RX_PIN,
                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) return;
    /* Interrupt-driven RX buffer keeps radar bytes while I/Q captures run. */
    if (uart_driver_install(RADAR_UART, RADAR_RX_BUFFER, 0, 0, NULL, 0) != ESP_OK) return;
    uart_flush_input(RADAR_UART);
    ready = true;
}

static void reply(const char *s) { (void)burst_serial_send(s, strlen(s)); }

static int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool radar_link_command(const char *line) {
    if (!strcmp(line, "RADARINFO?")) {
        char h[96];
        size_t pending = 0;
        if (ready) uart_get_buffered_data_len(RADAR_UART, &pending);
        snprintf(h, sizeof(h), "RADARINFO %u %u %u %u %" PRIu32 " %" PRIu32 "\n", ready ? 1u : 0u,
                 (unsigned)CONFIG_ESP_SDR_RADAR_BAUD, (unsigned)pending, (unsigned)RADAR_RX_BUFFER, total_rx, total_tx);
        reply(h);
        return true;
    }
    if (!strcmp(line, "RADAR?")) {
        /* Reply: "RADAR <n> <crc32>\n" followed by n raw radar bytes. */
        int n = 0;
        if (ready) {
            n = uart_read_bytes(RADAR_UART, chunk, sizeof(chunk), 0);
            if (n < 0) n = 0;
        }
        uint32_t crc = esp_rom_crc32_le(0, chunk, (uint32_t)n);
        char h[48];
        snprintf(h, sizeof(h), "RADAR %d %08" PRIx32 "\n", n, crc);
        reply(h);
        if (n > 0) (void)burst_serial_send(chunk, (size_t)n);
        total_rx += (uint32_t)n;
        return true;
    }
    if (!strncmp(line, "RADARTX ", 8)) {
        const char *p = line + 8;
        size_t len = strlen(p);
        if (!ready || len == 0 || len % 2 || len / 2 > 60) {
            reply("ERR radar_args\n");
            return true;
        }
        uint8_t out[60];
        for (size_t j = 0; j < len / 2; ++j) {
            int hi = hex_value(p[2 * j]), lo = hex_value(p[2 * j + 1]);
            if (hi < 0 || lo < 0) {
                reply("ERR radar_args\n");
                return true;
            }
            out[j] = (uint8_t)(hi << 4 | lo);
        }
        uart_write_bytes(RADAR_UART, out, len / 2);
        total_tx += (uint32_t)(len / 2);
        reply("OK\n");
        return true;
    }
    return false;
}

#else
void radar_link_init(void) {}
bool radar_link_command(const char *line) { (void)line; return false; }
#endif
