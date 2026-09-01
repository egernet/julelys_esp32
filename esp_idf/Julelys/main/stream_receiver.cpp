#include "stream_receiver.h"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"

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

        /* One bit per row received for activeSeq. The frame is shown only once
         * every row has arrived. */
        uint8_t rowMask = 0;
        const uint8_t fullMask = (width >= 8) ? 0xFF : (uint8_t)((1u << width) - 1);

        uint32_t shown = 0, stale = 0, partial = 0;
        int64_t lastReport = esp_timer_get_time();

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

            /* UDP gives no ordering guarantee, so a delayed packet from an
             * older frame can arrive after a newer frame has started. Compare
             * sequence numbers as a signed difference - that sorts old from
             * new and handles the wrap at 65535 in one step. */
            int16_t age = (activeSeq < 0) ? 1 : (int16_t)(seq - (uint16_t)activeSeq);

            if (age < 0) {
                /* Older than what we are already building - it can only make
                 * the picture worse, so drop it. */
                stale++;
                continue;
            }

            if (age > 0) {
                if (rowMask != 0) {
                    /* A newer frame started before the previous one was whole.
                     * At 30 FPS the missing rows are 33ms from being resent, so
                     * discard the partial frame instead of showing a torn mix
                     * of two. */
                    partial++;
                }
                activeSeq = seq;
                rowMask = 0;
            }

            const uint8_t *pixels = rx_buffer + STREAM_HEADER_LEN;
            for (int col = 0; col < height; ++col) {
                const uint8_t *p = pixels + (col * 4);
                ledController->setPixel(row, col, RgbwColor(p[0], p[1], p[2], p[3]));
            }

            rowMask |= (uint8_t)(1u << row);

            if (rowMask == fullMask) {
                ledController->swapBuffers();
                rowMask = 0;
                shown++;
            }

            int64_t now = esp_timer_get_time();
            if (now - lastReport >= 5000000) {
                ESP_LOGI(TAG, "%lu fps, dropped: %lu stale, %lu partial, heap %u",
                         (unsigned long)(shown / 5), (unsigned long)stale,
                         (unsigned long)partial,
                         (unsigned)heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
                shown = stale = partial = 0;
                lastReport = now;
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
