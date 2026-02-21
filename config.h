#ifndef CONFIG_H
#define CONFIG_H

// Controller Configuration
// 0 = CONTROLLER_GPIO, 1 = CONTROLLER_NES, 
// 2 = CONTROLLER_SNES, 3 = CONTROLLER_PSX
#define CONTROLLER_TYPE 0

// MicroSD card configuration
#define SD_FREQ 25000000

// Screen Configuration
#define SCREEN_ROTATION 1 // ST7735 160x80 landscape
#define SCREEN_SWAP_BYTES true // Set to false if colors appear wrong
// #define TFT_PARALLEL // Uncomment this line if using parallel communication instead of SPI communication

// ESP32-S3 SPI pins (edit to match your PCB wiring)
// Shared SPI bus between TFT + SD is recommended.
#define SD_MOSI_PIN 11
#define SD_MISO_PIN 13
#define SD_SCLK_PIN 12
#define SD_CS_PIN 4

// Button pins (GPIO button -> GND, active LOW with internal pull-up)
#define A_BUTTON 1
#define B_BUTTON 2
#define LEFT_BUTTON 5
#define RIGHT_BUTTON 6
#define UP_BUTTON 7
#define DOWN_BUTTON 8
#define START_BUTTON 16
#define SELECT_BUTTON 17

// NES controller pins
#define CONTROLLER_NES_CLK 5
#define CONTROLLER_NES_LATCH 19
#define CONTROLLER_NES_DATA 21

// SNES controller pins
#define CONTROLLER_SNES_CLK 5
#define CONTROLLER_SNES_LATCH 19
#define CONTROLLER_SNES_DATA 21

// PS1/PS2 controller pins
#define CONTROLLER_PSX_DATA 5
#define CONTROLLER_PSX_COMMAND 19
#define CONTROLLER_PSX_ATTENTION 21
#define CONTROLLER_PSX_CLK 22

#define FRAMESKIP
// #define DEBUG // Uncomment this line if you want debug prints from serial

#endif
