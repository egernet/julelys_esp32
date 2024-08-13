#include "tcp_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "oled_controller.h"
#include "led_controller.h"
#include "rain_sequence.h"

#define PORT 2412

extern const char *server_ip;

static const char TAG[] = "Jylelys.TCP";

extern const char *payload;
extern LedController *ledController;
extern OLEDController *oledController;

uint64_t timestamp = 0;

uint64_t convertBytesToLong(const char *bytes) {
    if (bytes == NULL) {
        return 0;
    }

    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value = (value << 8) | (uint8_t)bytes[i];
    }

    return value;
}

void get_tick_count() {
    uint64_t ticks = (uint64_t) xTaskGetTickCount();
    ESP_LOGI(TAG, "Ticks since start: %lld", (timestamp + ticks));
}

void parseData(char rx_buffer[], int len) {
    for(int i=0; i<len; i+=6) {
        int row = rx_buffer[i+1];
        int col = rx_buffer[i];
        int red = rx_buffer[i+2];
        int green = rx_buffer[i+3];
        int blue = rx_buffer[i+4];
        int white = rx_buffer[i+5];

        // RgbwColor color(red, green, blue, white);
        ESP_LOGI(TAG, "len: %d, index: %d [row: %d, col: %d], (R: %d, G: %d, B: %d, W: %d)", len, i, row, col, red, green, blue, white);
        // ledController->setPixel(row, col, color);
    }

    // ledController->imageHaveChange = true;
    // do {
    //     vTaskDelay(10 / portTICK_PERIOD_MS);
    // } while(ledController->isReading);
                
    //ledController->refresh();
                // vTaskDelay(updateInterval / portTICK_PERIOD_MS);
                // ledController->imageHaveChange = true;
                // do {
                //     vTaskDelay(updateInterval / portTICK_PERIOD_MS);
                // } while(ledController->isReading);

                //char str[] = "Dette er en IP-adresse: 192.168.1.1. Det er en ugyldig IP: 256.256.256.256 Og en anden gyldig IP: 10.0.0.1";
}

void parseDataTimestamp(char data[]) {
    timestamp = convertBytesToLong(data);
    get_tick_count();
}

void parseDataFrame(char data[], size_t size) {
    //parseData(data, size);

    for (size_t i = 0; i < size; i++) {
        printf("%d ", (unsigned char)data[i]);
    }

     printf("\n");

    get_tick_count();
}

void tcp_client_task(void *pvParameters) {
    char rx_buffer[1032];
    char tx_buffer[128];
    int addr_family = 0;
    int ip_protocol = 0;
    
    char *address = (char *)malloc(strlen(server_ip) + 1);
    strcpy(address, server_ip);

    oledController->setHostIPAddress(address);

    while (1) {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(address);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", address, PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            close(sock);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "Successfully connected");

        stopRainTask();

        get_tick_count();

        // Sending data
        snprintf(tx_buffer, sizeof(tx_buffer), payload);
        int err_send = send(sock, tx_buffer, strlen(tx_buffer), 0);
        if (err_send < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        }

        get_tick_count();

        while (1) {
            int length = recv(sock, rx_buffer, sizeof(rx_buffer), 0);
            if (length < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            } else {
                unsigned char size = rx_buffer[1];
                if (size > length - 2) {
                    size = length - 2;
                }

                char destination[size];
                memcpy(destination, rx_buffer + 2, size);
                destination[size] = '\0';

                switch (rx_buffer[0])
                {
                case 128:
                    parseDataTimestamp(destination);
                    break;
                default:
                    parseDataFrame(destination, size);
                    break;
                }
                //rx_buffer[len] = 0; // Null-terminate the received data
                // ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
            }

        }
        
        startupRainTask();

        close(sock);
        ESP_LOGI(TAG, "Socket closed, restarting after 2 seconds...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void startupSteamTask() {
    ESP_LOGI(TAG, "Startup the TCP Client.");

    xTaskCreate(tcp_client_task, "tcp_client_task", 4096, NULL, 5, NULL);
}