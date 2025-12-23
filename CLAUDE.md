# Julelys ESP32

Smart Christmas light system with 440 addressable SK6812 RGBW LEDs in an 8x55 matrix.

## Related Repositories

- **PCB:** [julelys_pcb_v2](https://github.com/egernet/julelys_pcb_v2/tree/feature/v3)
- **Manager:** [julelys_manager](https://github.com/egernet/julelys_manager)

## Hardware

- **MCU:** ESP32-C3 with custom HAT PCB v2
- **LEDs:** SK6812 RGBW strips (8 rows x 55 LEDs)

### GPIO Pinout

| GPIO | Function |
|------|----------|
| 7 | LED data output (RMT) |
| 4, 5, 8 | LED row multiplexer (3-bit channel select) |
| 10 | SPI CS |
| 3 | SPI CLK |
| 6 | SPI MOSI |
| 2 | SPI MISO |

## Build & Flash

```bash
cd esp_idf/Julelys
idf.py build
idf.py flash monitor
```

Requires ESP-IDF v5.4.1+

## Project Structure

```
esp_idf/Julelys/
├── main/
│   ├── main.cpp           # Entry point, SPI slave, console
│   └── rain_sequence.cpp  # Rainbow idle animation
├── components/
│   ├── led_controller/    # LED matrix control (double buffering)
│   ├── settings_controller/ # NVS persistence (WiFi creds)
│   ├── cmd_system/        # CLI commands
│   └── foundation/        # String helpers
└── managed_components/    # led_strip (espressif)
```

## Architecture

### Double Buffering
The LED controller uses double buffering to prevent tearing:

```
SPI Task                    LED Task
    │                           │
    ▼                           ▼
┌─────────┐   swap()     ┌─────────┐
│  Back   │◄────────────►│  Front  │
│ Buffer  │              │ Buffer  │
└─────────┘              └─────────┘
```

- **Back buffer:** SPI task writes new frames here
- **Front buffer:** LED task reads from here for display
- **Swap:** Atomic buffer swap with mutex protection

### Data Flow
1. SPI master (manager) sends LED frame (1760 bytes = 8x55x4 RGBW)
2. Frame is written to back buffer via `setPixel()`
3. `swapBuffers()` swaps front/back atomically with mutex
4. LED task refreshes display via RMT with GPIO multiplexing

### FreeRTOS Tasks
- **ledSequenceTask** - LED refresh loop (1ms yield)
- **spi_slave_task** - Receives frames from manager
- **rain_sequence_task** - Idle animation (10ms interval)

## Code Conventions

- C++ classes for main components (LedController, SettingsController)
- C functions for system commands
- ESP-IDF logging: `ESP_LOGI`, `ESP_LOGE`, etc.
- FreeRTOS for task management

## Key Classes

### LedController
```cpp
// Write to back buffer
ledController.setPixel(row, col, RgbwColor{r, g, b, w});

// Swap buffers atomically (call after complete frame)
ledController.swapBuffers();

// Clear display (sets all to red)
ledController.clean();
```

### SettingsController
```cpp
settings.getWIFISSID();
settings.setWIFIPassword("secret");
```

## SPI Frame Format

- 1760 bytes per frame
- Byte order: R, G, B, W per pixel
- Layout: row 0 col 0-54, row 1 col 0-54, ... row 7 col 0-54
