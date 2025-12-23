# Julelys ESP32

Smart julelys-system med 440 addresserbare SK6812 RGBW LED'er i en 8x55 matrix.

## Hardware

- **MCU:** ESP32-C3 med custom HAT PCB v2
- **LED'er:** SK6812 RGBW strips (8 rækker × 55 LED'er)

### GPIO Pinout

| GPIO | Funktion |
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

Kræver ESP-IDF v5.4.1+

## Projektstruktur

```
esp_idf/Julelys/
├── main/
│   ├── main.cpp           # Entry point, SPI slave, konsol
│   └── rain_sequence.cpp  # Rainbow idle-animation
├── components/
│   ├── led_controller/    # LED matrix styring via RMT
│   ├── settings_controller/ # NVS persistens (WiFi creds)
│   ├── cmd_system/        # CLI kommandoer
│   └── foundation/        # String helpers
└── managed_components/    # led_strip (espressif)
```

## Arkitektur

### Dataflow
1. SPI master sender LED frame (1760 bytes = 8×55×4 RGBW)
2. `main.cpp` parser til 2D matrix
3. `LedController` refresher via RMT med GPIO multiplexing
4. Efter 5s inaktivitet → `rain_sequence` starter rainbow-animation

### FreeRTOS Tasks
- **ledSequenceTask** - LED refresh loop (30ms interval)
- **spi_slave_task** - Modtager frames fra master
- **rain_sequence_task** - Idle animation (10ms interval)

## Kodekonventioner

- C++ klasser for hovedkomponenter (LedController, SettingsController)
- C funktioner for system commands
- ESP-IDF logging: `ESP_LOGI`, `ESP_LOGE`, etc.
- FreeRTOS til task management

## Vigtige klasser

### LedController
```cpp
ledController.setPixel(row, col, RgbwColor{r, g, b, w});
ledController.refresh();  // Push til hardware
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
