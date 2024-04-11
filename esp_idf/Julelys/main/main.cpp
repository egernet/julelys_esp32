#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_system.h"
#include "esp_log.h"

#include "sdkconfig.h"

#include "esp_log.h"
#include "esp_console.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"

#include <esp_heap_caps.h>

#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "js_string.h"

#include "led_controller.h"
#include "js_controller.h"
#include "settings_controller.h"
#include "cmd_wifi.h"

#include "iot_button.h"
#include "button_gpio.h"

static const char TAG[] = "Jylelys";

#define PROMPT_STR "julelys"

LedController *ledController;
SettingsController *settingsController;

#define BUTTON_PIN GPIO_NUM_9
#define LONG_PRESS_DELAY 10000

void button_single_click_cb(void *arg,void *usr_data) {
    ESP_LOGI(TAG, "BUTTON_SINGLE_CLICK");
}

static void button_long_press_start_cb(void *arg,void *usr_data) {
    settingsController->reset();
    esp_restart();
}

void configure_button() {
    button_config_t gpio_btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
        .gpio_button_config = {
            .gpio_num = 9,
            .active_level = 0,
        },
    };
    
    button_handle_t gpio_btn = iot_button_create(&gpio_btn_cfg);
    if(NULL == gpio_btn) {
        ESP_LOGE(TAG, "Button create failed");
        return;
    }

    iot_button_register_cb(gpio_btn, BUTTON_SINGLE_CLICK, button_single_click_cb, NULL);
    iot_button_register_cb(gpio_btn, BUTTON_LONG_PRESS_START, button_long_press_start_cb, NULL);
}

void free_memory(void *pvParameter) {
    while (1) {
        uint32_t freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        printf("Free mem:%ld\n", freeHeapBytes);

        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
    vTaskDelete( NULL ); 
}

void led_sequence_task(void *pvParameter) {
    gpio_reset_pin(GPIO_NUM_8);
    gpio_reset_pin(GPIO_NUM_7);
    gpio_reset_pin(GPIO_NUM_6);
    gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_7, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_6, GPIO_MODE_OUTPUT);

    while (1) {
        gpio_set_level(GPIO_NUM_6, 0);
        gpio_set_level(GPIO_NUM_8, 1);     
        vTaskDelay(300 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_8, 0);
        gpio_set_level(GPIO_NUM_7, 1);
        vTaskDelay(300 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_7, 0);
        gpio_set_level(GPIO_NUM_6, 1);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
    vTaskDelete( NULL );  
}

void led_setting_sequence_task(void *pvParameter) {
    gpio_reset_pin(GPIO_NUM_8);
    gpio_reset_pin(GPIO_NUM_7);
    gpio_reset_pin(GPIO_NUM_6);
    gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_7, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_6, GPIO_MODE_OUTPUT);

    gpio_set_level(GPIO_NUM_8, 0);
    gpio_set_level(GPIO_NUM_7, 0);
    gpio_set_level(GPIO_NUM_6, 0);

    while (1) {
        gpio_set_level(GPIO_NUM_8, 1);     
        vTaskDelay(300 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_8, 0);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
    vTaskDelete( NULL );  
}

void js_sequence_task(void *pvParameter) {
    printf("Start JS demo\n");

    JSController *jsController = new JSController(ledController);

    jsController->runCode(js_content);

    delete jsController;

    vTaskDelete( NULL );  
}

static void initialize_console(void)
{
    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
            .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
#if SOC_UART_SUPPORT_REF_TICK
        .source_clk = UART_SCLK_REF_TICK,
#elif SOC_UART_SUPPORT_XTAL_CLK
        .source_clk = UART_SCLK_XTAL,
#endif
    };
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM,
            256, 0, 0, NULL, 0) );
    ESP_ERROR_CHECK( uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
            .max_cmdline_length = 256,
            .max_cmdline_args = 8,
#if CONFIG_LOG_COLORS
            .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);

    /* Set command maximum length */
    linenoiseSetMaxLineLen(console_config.max_cmdline_length);

    /* Don't return empty lines */
    linenoiseAllowEmpty(false);

#if CONFIG_STORE_HISTORY
    /* Load command history from filesystem */
    linenoiseHistoryLoad(HISTORY_PATH);
#endif
}

void startupTasks() {
    configure_button();

    settingsController = new SettingsController();
    ledController = new LedController(10, 1, 55);

    xTaskCreate(
    &led_sequence_task,
    "led_sequence_task",
    2048,
    NULL,
    5,
    NULL);

    xTaskCreate(
    &free_memory,
    "free_memory",
    2048,
    NULL,
    1,
    NULL);

    xTaskCreate(
    &js_sequence_task,
    "js_sequence_task",
    16384,
    NULL,
    5,
    NULL);
}

extern "C" {
    void app_main();
}

void app_main(void)
{
    startupTasks();

    // initialize_nvs();

#if CONFIG_STORE_HISTORY
    initialize_filesystem();
    ESP_LOGI(TAG, "Command history enabled");
#else
    ESP_LOGI(TAG, "Command history disabled");
#endif

    initialize_console();

    /* Register commands */
    esp_console_register_help_command();
    // register_system();
    register_wifi();
    // register_nvs();

    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    const char* prompt = LOG_COLOR_I PROMPT_STR "> " LOG_RESET_COLOR;

    printf("\n"
           "This is an example of ESP-IDF console component.\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n"
           "Press Enter or Ctrl+C will terminate the console environment.\n");

    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status) { /* zero indicates success */
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
        /* Since the terminal doesn't support escape sequences,
         * don't use color codes in the prompt.
         */
        prompt = PROMPT_STR "> ";
#endif //CONFIG_LOG_COLORS
    }

    /* Main loop */
    while(true) {
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        char* line = linenoise(prompt);
        if (line == NULL) { /* Break on EOF or error */
            break;
        }
        /* Add the command to the history if not empty*/
        if (strlen(line) > 0) {
            linenoiseHistoryAdd(line);
#if CONFIG_STORE_HISTORY
            /* Save command history to filesystem */
            linenoiseHistorySave(HISTORY_PATH);
#endif
        }

        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
        } else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }

    ESP_LOGE(TAG, "Error or end-of-input, terminating console");
    esp_console_deinit();
}
/*
void app_main()
{   
    settingsController = new SettingsController();

    int number = settingsController->loadConfig();

    if (number == 0 ) {
        printf("Vi venter på data");

        xTaskCreate(
        &led_setting_sequence_task,
        "led_setting_sequence_task",
        2048,
        NULL,
        5,
        NULL);

        initialize_console();
        console_task();

        //settingsController->saveConfig(512);

        //vTaskDelay(15000 / portTICK_PERIOD_MS);

        //esp_restart();
    } else {
        configure_button();

        printf("Setting data: %d", number);
        ledController = new LedController(10, 1, 55);

        xTaskCreate(
        &led_sequence_task,
        "led_sequence_task",
        2048,
        NULL,
        5,
        NULL);

        xTaskCreate(
        &free_memory,
        "free_memory",
        2048,
        NULL,
        1,
        NULL);

        xTaskCreate(
        &js_sequence_task,
        "js_sequence_task",
        16384,
        NULL,
        5,
        NULL);
    }   
}

*/