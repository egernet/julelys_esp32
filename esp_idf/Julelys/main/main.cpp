#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <esp_system.h>
#include <esp_log.h>
#include <esp_console.h>

#include <linenoise/linenoise.h>
#include <argtable3/argtable3.h>
#include <esp_vfs_dev.h>
#include <esp_vfs_fat.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include <esp_heap_caps.h>

#include "driver/uart_vfs.h"
#include <driver/uart.h>

#include "driver/spi_slave.h"
#include "driver/gpio.h"

#include "led_controller.h"
#include "settings_controller.h"

#include "cmd_system.h"

#include "foundation.h"

#include "rain_sequence.h"

#define PROMPT_STR "julelys"

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

static const char TAG[] = "Julelys";
const char *payload = "Julelys v2";

LedController *ledController = nullptr;
SettingsController *settingsController = nullptr;

#define RCV_HOST    SPI2_HOST
#define SPI_BUFFER_LEN 4
#define SPI_CS_PIN     0
#define SPI_CLK_PIN    1
#define SPI_MOSI_PIN   2
#define SPI_MISO_PIN   3

spi_device_handle_t spi_dev;

void spi_slave_task(void* arg) {
    const uint8_t expected_sequence[SPI_BUFFER_LEN] = {0x55, 0xAA, 0xFF, 0x00};
    uint8_t recv_buf[SPI_BUFFER_LEN];
    uint8_t send_buf[SPI_BUFFER_LEN];

    while (true) {
        memset(recv_buf, 0, SPI_BUFFER_LEN);
        memset(send_buf, 0, SPI_BUFFER_LEN);

        // Forbered svar
        memcpy(send_buf, expected_sequence, 4); // Svar med f.eks. ASCII "OKAY"

        spi_slave_transaction_t trans = {};
        trans.length = SPI_BUFFER_LEN * 8;
        trans.tx_buffer = send_buf;
        trans.rx_buffer = recv_buf;

        // Blokerer indtil master sender
        ESP_ERROR_CHECK(spi_slave_transmit(RCV_HOST, &trans, portMAX_DELAY));

        // Log modtagne data
        ESP_LOGI(TAG, "Modtog: %02X %02X %02X %02X", recv_buf[0], recv_buf[1], recv_buf[2], recv_buf[3]);

        // Kontroller om sekvens matcher
        if (memcmp(recv_buf, expected_sequence, SPI_BUFFER_LEN) == 0) {
            ESP_LOGI(TAG, "Gyldig kommando modtaget. Svarede: %s", send_buf);
        } else {
            ESP_LOGW(TAG, "Ugyldig sekvens. Svarede alligevel.");
        }
    }
}

static esp_err_t init_spi_master_async() {
    spi_bus_config_t buscfg = {
        .mosi_io_num = SPI_MOSI_PIN,
        .miso_io_num = SPI_MISO_PIN,
        .sclk_io_num = SPI_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SPI_BUFFER_LEN
    };

    spi_slave_interface_config_t slvcfg = {
        .spics_io_num = SPI_CS_PIN,
        .flags = 0,
        .queue_size = 1,
        .mode = 0,
        .post_setup_cb = NULL,
        .post_trans_cb = NULL
    };

    ESP_ERROR_CHECK(spi_slave_initialize(RCV_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI-slave initialiseret");

    xTaskCreate(spi_slave_task, "spi_slave_task", 4096, NULL, 5, NULL);
    return ESP_OK;
}

static void initialize_filesystem(void) {
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 4,
        .allocation_unit_size = 4096
    };

    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
    }
}

static void initialize_nvs(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static void initialize_console(void) {
    fflush(stdout);
    fsync(fileno(stdout));
    setvbuf(stdin, NULL, _IONBF, 0);

    uart_vfs_dev_port_set_rx_line_endings((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    uart_vfs_dev_port_set_tx_line_endings((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    const uart_config_t uart_config = {
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
#if SOC_UART_SUPPORT_REF_TICK
        .source_clk = UART_SCLK_REF_TICK,
#elif SOC_UART_SUPPORT_XTAL_CLK
        .source_clk = UART_SCLK_XTAL,
#endif
    };
    ESP_ERROR_CHECK(uart_driver_install((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));
    uart_vfs_dev_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    esp_console_config_t console_config = {
        .max_cmdline_length = 256,
        .max_cmdline_args = 8,
#if CONFIG_LOG_COLORS
        .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));
    linenoiseSetMultiLine(1);
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);
    linenoiseHistorySetMaxLen(100);
    linenoiseSetMaxLineLen(console_config.max_cmdline_length);
    linenoiseAllowEmpty(false);
    linenoiseHistoryLoad(HISTORY_PATH);
}

void setup() {
    settingsController = new SettingsController();
    ledController = new LedController(GPIO_NUM_7, 8, 55);
}

void startupTasks() {
    ledController->startupLoopTask();
    startupRainTask();
}

extern "C" void app_main(void) {
    setup();

    initialize_nvs();
    initialize_filesystem();
    initialize_console();

    esp_console_register_help_command();
    register_system();

    startupTasks();
    init_spi_master_async();

#if CONFIG_LOG_COLORS
    const char* prompt = LOG_COLOR_I PROMPT_STR "> " LOG_RESET_COLOR;
#else
    const char* prompt = PROMPT_STR "> ";
#endif

    printf("\n"
           "ESP-IDF Console\n"
           "Type 'help' for a list of commands.\n"
           "Use UP/DOWN to browse history, TAB to autocomplete.\n");

    if (linenoiseProbe()) {
        printf("Terminal does not support escape sequences.\n");
        linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
        prompt = PROMPT_STR "> ";
#endif
    }

    while (true) {
        char* line = linenoise(prompt);
        if (line == NULL) break;

        if (strlen(line) > 0) {
            linenoiseHistoryAdd(line);
            linenoiseHistorySave(HISTORY_PATH);
        }

        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err != ESP_OK) {
            printf("Error: %s\n", esp_err_to_name(err));
        }

        linenoiseFree(line);
    }

    ESP_LOGE(TAG, "Console ended");
    esp_console_deinit();
}