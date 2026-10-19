# Julelys ESP32

Smart Christmas light system with addressable LED strings in a matrix: 8x55 SK6812 RGBW
by default, 4x100 WS2812B RGB for the 2026 season. See "LED configuration" below.

## Related Repositories

- **PCB:** [julelys_pcb_v2](https://github.com/egernet/julelys_pcb_v2/tree/feature/v3)
- **Manager:** [julelys_manager](https://github.com/egernet/julelys_manager)

## Hardware

- **MCU:** ESP32-C3 with custom HAT PCB v2
- **LEDs:** SK6812 RGBW strips (8 rows x 55 LEDs), or WS2812B RGB fairy strings (4 x 100)

### GPIO Pinout

| GPIO | Function |
|------|----------|
| 7 | LED data output (RMT) |
| 4, 5, 8 | LED row multiplexer (3-bit channel select) |
| 10 | SPI CS |
| 3 | SPI CLK |
| 6 | SPI MOSI |
| 2 | SPI MISO |

## LED configuration

Chip type and matrix size are Kconfig options in `idf.py menuconfig` under
**Julelys LED** (`components/led_controller/Kconfig`). `sdkconfig` is gitignored,
so set them again on every new checkout.

| Option | Default | 2026 season |
|------|------|------|
| `CONFIG_JULELYS_LED_TYPE_*` | `SK6812` | `WS2812B` |
| `CONFIG_JULELYS_LED_COLOR_ORDER_*` (WS2812B only) | `GRB` | `GRB` (not yet checked with `Test Color`) |
| `CONFIG_JULELYS_NUMBER_OF_STRINGS` | 8 | 4 |
| `CONFIG_JULELYS_LEDS_PER_STRING` | 55 | 100 |

- Frames always arrive as RGBW. On WS2812B the white channel is added to R, G
  and B (clamped at 255), so the manager and the sequences are unchanged.
- The manager must use the same size: `--matrix-width 4 --matrix-height 100`.
  A mismatch shows as `Ignoring N byte packet, expected M` in the log.
- With fewer than 8 strings, only mux channels 0 and up are refreshed, so the
  strings go in the first JST outputs (J4 = CH1, J5, J6, J7).
- If red and green are swapped on a WS2812B string, switch the color order to RGB.

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
1. Manager sends a frame over SPI (strings x LEDs x 4 bytes RGBW, 1760 for 8x55) or UDP stream
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

- strings x LEDs x 4 bytes per frame (1760 for the default 8x55)
- Byte order: R, G, B, W per pixel
- Layout: row 0 col 0..N-1, row 1 col 0..N-1, ... one row per string
