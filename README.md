# Julelys ESP32

Smart Christmas light system with 440 addressable SK6812 RGBW LEDs in an 8x55 matrix.

## Related Repositories

- **PCB:** [julelys_pcb_v2](https://github.com/egernet/julelys_pcb_v2/tree/feature/v3)
- **Manager:** [julelys_manager](https://github.com/egernet/julelys_manager)

## Hardware

- **MCU:** ESP32-C3 with custom HAT PCB v2
- **LEDs:** SK6812 RGBW strips (8 rows x 55 LEDs = 440 total)
- **Interface:** SPI slave receives frames from manager

### GPIO Pinout

| GPIO | Function |
|------|----------|
| 7 | LED data output (RMT) |
| 4, 5, 8 | LED row multiplexer (3-bit) |
| 10 | SPI CS |
| 3 | SPI CLK |
| 6 | SPI MOSI |
| 2 | SPI MISO |

## Build & Flash

Requires ESP-IDF v5.4.1+

```bash
cd esp_idf/Julelys
idf.py build
idf.py flash monitor
```

## Project Structure

```
esp_idf/Julelys/
├── main/
│   ├── main.cpp           # Entry point, SPI slave, console
│   └── rain_sequence.cpp  # Rainbow idle animation
├── components/
│   ├── led_controller/    # LED matrix control (double buffering)
│   ├── settings_controller/ # NVS persistence
│   ├── cmd_system/        # CLI commands
│   └── foundation/        # String helpers
└── managed_components/    # led_strip (espressif)
```

## Architecture

### Double Buffering
The LED controller uses double buffering to prevent tearing:
- **Back buffer:** SPI task writes new frames here
- **Front buffer:** LED task reads from here for display
- **Swap:** Atomic buffer swap with mutex protection

### Data Flow
1. SPI master (manager) sends LED frame (1760 bytes = 8x55x4 RGBW)
2. Frame is written to back buffer
3. `swapBuffers()` swaps front/back atomically
4. LED task refreshes display via RMT with GPIO multiplexing

### Idle Animation
After 5 seconds of inactivity, a rainbow animation starts automatically.

## SPI Frame Format

- **Size:** 1760 bytes per frame
- **Format:** 4 bytes per pixel (R, G, B, W)
- **Layout:** Row-major (row 0 col 0-54, row 1 col 0-54, ... row 7 col 0-54)

## Console Commands

Available via UART (115200 baud):

| Command | Description |
|---------|-------------|
| `version` | Chip info and SDK version |
| `restart` | Software reset |
| `free` | Free heap memory |
| `heap` | Minimum free heap |
| `tasks` | List FreeRTOS tasks |
