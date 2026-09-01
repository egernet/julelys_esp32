#include "stream_receiver.h"

#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "led_controller.h"
#include "rain_sequence.h"

static const char TAG[] = "Julelys.Stream";

extern LedController *ledController;

/* Defined in main.cpp - restarts the 5 second idle animation timer. */
extern void reset_inactivity_timer();

static void stream_task(void *pvParameters) {
    const int width = ledController->matrixWidth;    // rows / LED strings
    const int height = ledController->matrixHeight;  // LEDs per string

    const size_t rowBytes = (size_t)height * 4;
    const size_t packetLen = STREAM_HEADER_LEN + rowBytes;

    uint8_t *rx_buffer = (uint8_t *)malloc(packetLen);
    if (rx_buffer == NULL) {
        ESP_LOGE(TAG, "Out of memory for receive buffer");
        vTaskDelete(NULL);
        return;
    }

    while (true) {
        struct sockaddr_in bind_addr = {};
        bind_addr.sin_family = AF_INET;
        bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        bind_addr.sin_port = htons(STREAM_PORT);

        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            continue;
        }

        if (bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
            ESP_LOGE(TAG, "Unable to bind port %d: errno %d", STREAM_PORT, errno);
            close(sock);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "Listening for frames on UDP port %d", STREAM_PORT);

        /* Sequence number of the frame currently being filled into the back
         * buffer, or -1 when no frame is in flight. */
        int32_t activeSeq = -1;

        while (true) {
            struct sockaddr_storage source_addr;
            socklen_t socklen = sizeof(source_addr);

            int len = recvfrom(sock, rx_buffer, packetLen, 0,
                               (struct sockaddr *)&source_addr, &socklen);
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }

            if ((size_t)len != packetLen) {
                ESP_LOGW(TAG, "Ignoring %d byte packet, expected %u", len, (unsigned)packetLen);
                continue;
            }

            uint16_t seq = ((uint16_t)rx_buffer[0] << 8) | rx_buffer[1];
            uint8_t row = rx_buffer[2];
            uint8_t flags = rx_buffer[3];

            if (row >= width) {
                ESP_LOGW(TAG, "Ignoring packet for row %d, matrix has %d rows", row, width);
                continue;
            }

            reset_inactivity_timer();
            stopRainTask();

            /* A new sequence number means the previous frame never got its
             * end-of-frame packet. Show what we have rather than blending the
             * two frames together in the back buffer. */
            if (activeSeq >= 0 && seq != (uint16_t)activeSeq) {
                ledController->swapBuffers();
            }
            activeSeq = seq;

            const uint8_t *pixels = rx_buffer + STREAM_HEADER_LEN;
            for (int col = 0; col < height; ++col) {
                const uint8_t *p = pixels + (col * 4);
                ledController->setPixel(row, col, RgbwColor(p[0], p[1], p[2], p[3]));
            }

            if (flags & STREAM_FLAG_END_OF_FRAME) {
                ledController->swapBuffers();
                activeSeq = -1;
            }
        }

        close(sock);
        ESP_LOGW(TAG, "Socket closed, reopening in 2 seconds");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    free(rx_buffer);
    vTaskDelete(NULL);
}

void startupStreamTask() {
    xTaskCreate(stream_task, "stream_task", 4096, NULL, 5, NULL);
}
