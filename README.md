# STM32 SD Card Interface with SPI & FatFS

Complete SD card driver for STM32 microcontrollers using SPI and FatFS file system.

![Working Demo](demo.gif)

## 🎥 Video Tutorial

📺 **Watch the full tutorial:** To be added later

## ✨ Features

- ✅ Read/Write files to SD card
- ✅ Create, append, and delete files
- ✅ Directory listing
- ✅ Supports SDHC cards (2GB - 32GB)
- ✅ Easy to integrate - just 3 files!
- ✅ No complex coding required

## 🔧 Hardware Required

- STM32 Nucleo-144 (F767ZI) or similar
- SD card module
- MicroSD card (2-32GB, FAT32 formatted)
- Jumper wires

## 📌 Wiring Connections
```
SD Module → STM32 Nucleo
========================
CS   → PD14
SCK  → PA5 (SPI1_SCK)
MISO → PA6 (SPI1_MISO)
MOSI → PA7 (SPI1_MOSI)
VCC  → 3.3V
GND  → GND
```

⚠️ **Important:** Use 3.3V, NOT 5V!

## 🚀 Quick Start

### 1. STM32CubeMX Configuration

- Enable **SPI1** (Full-Duplex Master, Prescaler: 256)
- Enable **FATFS** (User-defined, `_USE_MKFS = 1`)
- Enable **USART3** (115200 baud for printf)
- Set **PD14** as GPIO_Output (High)
- Generate code

### 2. Add Driver Files

Copy these files to your project:
- `Sd_spi.c` → `Core/Src/`
- `Sd_spi.h` → `Core/Inc/`
- `user_diskio.c` → `FATFS/Target/` (replace existing)

### 3. Add Test Code to main.c
```c
/* USER CODE BEGIN Includes */
#include "Sd_spi.h"
/* USER CODE END Includes */

/* USER CODE BEGIN 2 */
sd_mount();
sd_write_file("test.txt", "Hello SD Card!");
sd_unmount();
/* USER CODE END 2 */
```

### 4. Build and Flash!

That's it! Your SD card is ready to use.

## 📖 API Functions
```c
// Mount/Unmount
int sd_mount(void);
void sd_unmount(void);

// File operations
int sd_write_file(const char *filename, const char *data);
int sd_read_file(const char *filename, char *buffer, uint32_t size, UINT *bytes_read);
int sd_append_file(const char *filename, const char *data);
void sd_list_files(void);

// Low-level operations
uint8_t sd_init(void);
uint8_t sd_read_block(uint8_t *buf, uint32_t sector);
uint8_t sd_write_block(const uint8_t *buf, uint32_t sector);
```

## 🐛 Troubleshooting

**SD Init Failed: CMD0**
- Check wiring (especially MISO/MOSI)
- Verify 3.3V power
- Try different SD card

**Mount Failed (error 13)**
- Format SD card as FAT32
- Enable `_USE_MKFS = 1` in CubeMX

**No printf output**
- Check USART3 enabled
- Verify serial terminal (115200 baud)

## 📄 License

MIT License - Feel free to use in your projects!

## 🤝 Contributing

Found a bug? Have a suggestion? Open an issue or pull request!

## 💬 Support

- 📺 YouTube: https://www.youtube.com/@hardware_coding3603
-              https://www.youtube.com/@hardware_programming2875
- 💬 Comments: Ask questions under the video
- ⭐ Star this repo if it helped you!

## 📊 Tested Boards

- ✅ STM32F767ZI Nucleo-144
- ✅ STM32F429ZI Nucleo-144
- ✅ STM32F446RE Nucleo-64

