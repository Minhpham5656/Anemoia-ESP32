/*
 * Anemoia-ESP32.ino (single-file edition)
 * Self-contained build for Arduino IDE (C++14)
 */

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <string>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <vector>
#include <FS.h>
#include <esp_timer.h>
#include <rom/ets_sys.h>
#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_bt_main.h"


// ===== BEGIN config.h =====
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
// ===== END config.h =====

// ===== BEGIN src/controller.h =====
#ifndef CONTROLLER_H
#define CONTROLLER_H

enum CONTROLLER
{
    A = (1 << 0), // A Button
    B = (1 << 1), // B Button
    Select = (1 << 2), // Select Button
    Start = (1 << 3), // Start Button
    Up = (1 << 4), // Up Button
    Down = (1 << 5), // Down Button
    Left = (1 << 6), // Left Button
    Right = (1 << 7)  // Right Button
};

void initController();
uint8_t controllerRead();
bool isDownPressed(CONTROLLER button);

uint8_t NESControllerRead();
uint8_t SNESControllerRead();
uint8_t PSXControllerRead();
uint8_t PSXTransferByte(uint8_t byte);

#endif
// ===== END src/controller.h =====

// ===== BEGIN src/core/mapper.h =====
#ifndef MAPPER_H
#define MAPPER_H



class Cartridge;
struct MapperVTable;

class Mapper
{
public: 
    enum ROM_TYPE
    {
        PRG_ROM,
        CHR_ROM
    };

    const MapperVTable* vtable = nullptr;
    void* state = nullptr;
};

struct MapperVTable
{
    bool (*cpuRead)(Mapper* mapper, uint16_t addr, uint8_t& data);
    bool (*cpuWrite)(Mapper* mapper, uint16_t addr, uint8_t data);
    bool (*ppuRead)(Mapper* mapper, uint16_t addr, uint8_t& data);
    bool (*ppuWrite)(Mapper* mapper, uint16_t addr, uint8_t data);
    uint8_t* (*ppuReadPtr)(Mapper* mapper, uint16_t addr);
    void (*scanline)(Mapper* mapper);
    void (*cycle)(Mapper* mapper, int cycles);
    void (*reset)(Mapper* mapper);
    void (*dumpState)(Mapper* mapper, File& state);
    void (*loadState)(Mapper* mapper, File& state);
};
IRAM_ATTR static void mapperNoScanline(Mapper*) {}
IRAM_ATTR static void mapperNoCycle(Mapper*, int) {}

struct Bank
{
    uint8_t bank_id;
    uint8_t* bank_ptr;
    uint32_t last_used;
    uint32_t size;
};

struct BankCache
{
    Bank* banks;
    uint8_t num_banks;
    uint32_t tick;
    Cartridge* cart;
};

void bankInit(BankCache* cache, Bank* banks, uint8_t num_banks, uint32_t bank_size, Cartridge* cart);
uint8_t* getBank(BankCache* cache, uint8_t bank_id, Mapper::ROM_TYPE rom);
uint8_t getBankIndex(BankCache* cache, uint8_t* ptr);
void invalidateCache(BankCache* cache);

#endif
// ===== END src/core/mapper.h =====

// ===== BEGIN src/core/mappers/mapper000.h =====
#ifndef MAPPER000_H
#define MAPPER000_H


struct Mapper000_state
{
    Cartridge* cart;
    uint8_t number_PRG_banks;
    uint8_t number_CHR_banks;
    uint8_t PRG_ROM[32*1024];
    uint8_t CHR_ROM[8*1024];
    uint8_t* CHR_bank;
    uint8_t* PRG_banks[2];
};

Mapper createMapper000(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart);

#endif
// ===== END src/core/mappers/mapper000.h =====

// ===== BEGIN src/core/mappers/mapper001.h =====
#ifndef MAPPER001_H
#define MAPPER001_H


#define MAPPER001_NUM_PRG_BANKS_16K 8
#define MAPPER001_NUM_CHR_BANKS_8K 1
#define MAPPER001_NUM_CHR_BANKS_4K 2

Mapper createMapper001(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart);

#endif
// ===== END src/core/mappers/mapper001.h =====

// ===== BEGIN src/core/mappers/mapper002.h =====
#ifndef MAPPER002_H
#define MAPPER002_H


#define MAPPER002_NUM_PRG_BANKS_16K 7
struct Mapper002_state
{
    Cartridge* cart;
    uint8_t number_PRG_banks;
    uint8_t number_CHR_banks;
    uint8_t* ptr_16K_PRG_banks[2];
    Bank prg_banks[MAPPER002_NUM_PRG_BANKS_16K];
    BankCache prg_cache;
    uint8_t PRG_bank[16*1024];
    uint8_t CHR_bank[8*1024];
};

Mapper createMapper002(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart);

#endif
// ===== END src/core/mappers/mapper002.h =====

// ===== BEGIN src/core/mappers/mapper003.h =====
#ifndef MAPPER003_H
#define MAPPER003_H


#define MAPPER003_NUM_CHR_BANKS_8K 16
struct Mapper003_state
{
    Cartridge* cart;
    uint8_t number_PRG_banks;
    uint8_t number_CHR_banks;

    uint8_t* ptr_CHR_bank_8K;
    Bank CHR_banks_8K[MAPPER003_NUM_CHR_BANKS_8K];
    BankCache CHR_cache_8K;
    uint8_t PRG_bank[32*1024];
};

Mapper createMapper003(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart);

#endif
// ===== END src/core/mappers/mapper003.h =====

// ===== BEGIN src/core/mappers/mapper004.h =====
#ifndef MAPPER004_H
#define MAPPER004_H


#define MAPPER004_NUM_PRG_BANKS_8K 18
#define MAPPER004_NUM_CHR_BANKS_1K 26

Mapper createMapper004(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart);

#endif
// ===== END src/core/mappers/mapper004.h =====

// ===== BEGIN src/core/mappers/mapper069.h =====
#ifndef MAPPER069_H
#define MAPPER069_H


#define MAPPER069_NUM_PRG_BANKS_8K 17
#define MAPPER069_NUM_CHR_BANKS_1K 26

Mapper createMapper069(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart);

#endif
// ===== END src/core/mappers/mapper069.h =====

// ===== BEGIN src/core/cartridge.h =====
#ifndef CARTRIDGE_H
#define CARTRIDGE_H



class Bus;
class Cartridge
{
public:
    Cartridge(const char* filename);
    ~Cartridge();

    enum MIRROR
    {
        HORIZONTAL,
        VERTICAL,
        ONESCREEN_LOW,
        ONESCREEN_HIGH,
        HARDWARE
    };

    bool cpuRead(uint16_t addr, uint8_t& data);
	bool cpuWrite(uint16_t addr, uint8_t data);
	bool ppuRead(uint16_t addr, uint8_t& data);
    uint8_t* ppuReadPtr(uint16_t addr);
	bool ppuWrite(uint16_t addr, uint8_t data);
    void ppuScanline();
    void cpuCycle(int cycles);
    void reset();
    
    void loadPRGBank(uint8_t* bank, uint16_t size, uint32_t offset);
    void loadCHRBank(uint8_t* bank, uint16_t size, uint32_t offset);
    void setMirrorMode(MIRROR mirror);
    Cartridge::MIRROR getMirrorMode();
    void connectBus(Bus* n) { bus = n; }
    void IRQ();

    void dumpState(File& state);
    void loadState(File& state);

    uint8_t hardware_mirror;
    uint8_t mirror = HORIZONTAL;
    uint32_t CRC32 = ~0U;

private:
	Bus* bus = nullptr;
    uint32_t prg_base;
    uint32_t chr_base;

    File rom;
    Mapper mapper;
    uint8_t mapper_ID = 0;
	uint8_t number_PRG_banks = 0;
	uint8_t number_CHR_banks = 0;

    uint32_t crc32(const void* buf, size_t size, uint32_t seed = ~0U);
};

#endif
// ===== END src/core/cartridge.h =====

// ===== BEGIN src/core/apu2A03.h =====
#ifndef APU2A03_H
#define APU2A03_H



class Bus;
class Cpu6502;
class Apu2A03
{
public:
    Apu2A03();
    ~Apu2A03();

public:
    void connectBus(Bus* n) { bus = n; }
    void connectCPU(Cpu6502* n) { cpu = n; }
    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);
    void setVolume(uint8_t vol);
    void clock();
    void reset();

    uint8_t DMC_sample_byte = 0;
	bool IRQ = false;
	uint16_t buffer_index = 0;
	uint8_t volume = 100;

private:
	Bus* bus = nullptr;
	Cpu6502* cpu = nullptr;
	uint32_t clock_counter = 0;
	uint32_t pulse_hz = 0;
	uint16_t prev_sample = 0;
	bool four_step_sequence_mode = true;

    // double pulse_out = 0.0;
	// double tnd_out = 0.0;
	// double pulse_table[31];
	// double tnd_table[203];

    // Duty sequences
    static constexpr uint8_t duty_sequences[4][8] =
    {
        { 0, 1, 0, 0, 0, 0, 0, 0},
        { 0, 1 ,1 ,0 ,0, 0, 0, 0},
        { 0, 1, 1, 1, 1, 0, 0, 0},
        { 1, 0, 0, 1, 1, 1, 1, 1}
    };

    // Length counter lookup table
    static constexpr uint8_t length_counter_lookup[32] =
    { 10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

    // Triangle 32-step sequence
    static constexpr uint8_t triangle_sequence[32] =
    { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

    // Noise channel period lookup table
    static constexpr uint16_t noise_period_lookup[16] =
    { 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068 };

    // DMC rate lookup table
    static constexpr uint16_t DMC_rate_lookup[16] =
    { 428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54 };

    // Sound channel components
	struct sequencerUnit
	{
		uint8_t duty_cycle = 0x00;
		uint8_t cycle_position = 0x00;
		uint16_t timer = 0x0000;
		uint16_t reload = 0x0000;
		uint8_t output = 0x00;
	};
	struct linear_counter
	{
		bool control = false;
		bool reload_flag = false;
		uint8_t counter = 0x00;
		uint8_t reload = 0x00;
	};
	struct envelopeUnit
	{
		bool start_flag = false;
		bool loop = false;
		bool constant_volume = true;
		uint8_t volume = 0x00;
		uint8_t timer = 0x00;
		uint8_t output = 0x00;
		uint8_t decay_level_counter = 0x00;
	};
	struct sweepUnit
	{
		bool enable = false;
		bool negate = false;
		bool reload_flag = false;
		bool mute = false;
		uint8_t pulse_channel_number = 0;
		uint8_t shift_count = 0x00;
		int16_t change = 0x0000;
		uint16_t timer = 0x0000;
		uint16_t reload = 0x0000;
		int16_t target_period = 0x0000;
	};
	struct length_counter
	{
		bool enable = false;
		bool halt = false;
		uint8_t timer = 0x00;
	};
	struct memoryReader
	{
		uint16_t address = 0x0000;
		int16_t remaining_bytes = 0;
	};
	struct outputUnit
	{
		uint8_t shift_register = 0x00;
		int16_t remaining_bits = 0;
		uint8_t output_level = 0;
		bool silence_flag = false;
	};

	// Sound Channels 
	struct pulseChannel
	{
		sequencerUnit seq;
		envelopeUnit env;
		sweepUnit sweep;
		length_counter len_counter;
	};
	struct triangleChannel
	{
		sequencerUnit seq;
		length_counter len_counter;
		linear_counter lin_counter;
	};
	struct noiseChannel
	{
		envelopeUnit env;
		length_counter len_counter;
		uint16_t timer = 0x0000;
		uint16_t reload = 0x0000;
		uint16_t shift_register = 0x01;
		uint8_t output = 0x00;
		bool mode = false;
	};
	struct DMCChannel
	{
		bool IRQ_flag = false;
		bool loop_flag = false;
		bool sample_buffer_empty = false;
		uint8_t sample_buffer = 0x00;
		uint16_t sample_address = 0x0000;
		uint16_t sample_length = 0x0000;
		uint16_t timer = 0x0000;
		uint16_t reload = 0x0000;
		outputUnit output_unit;
		memoryReader memory_reader;
	};

	bool interrupt_inhibit = false;
	// Channel 1 - Pulse 1
	pulseChannel pulse1;
	bool pulse1_enable = false;

	// Channel 2 - Pulse 2
	pulseChannel pulse2;
	bool pulse2_enable = false;

	// Channel 3 - Triangle
	triangleChannel triangle;
	bool triangle_enable = false;

	// Channel 4 - Noise
	noiseChannel noise;
	bool noise_enable = false;

    // Channel 5 - DMC
	DMCChannel DMC;
	bool DMC_enable = false;

	void generateSample();

	void pulseChannelClock(sequencerUnit& seq, bool enable);
	void triangleChannelClock(triangleChannel& triangle, bool enable);
	void noiseChannelClock(noiseChannel& noise, bool enable);
	void DMCChannelClock(DMCChannel& DMC, bool enable);
    
	void soundChannelEnvelopeClock(envelopeUnit& envelope);
	void soundChannelSweeperClock(pulseChannel& channel);
	void soundChannelLengthCounterClock(length_counter& len_counter);
	void linearCounterClock(linear_counter& lin_counter);

	void setDMCBuffer();
};

#endif
// ===== END src/core/apu2A03.h =====

// ===== BEGIN src/core/cpu6502.h =====
#ifndef CPU6502_H
#define CPU6502_H



#define GET_FLAG(f) ((status & (f)) != 0)
#define SET_FLAG(f, v) (status = (v) ? (status | (f)) : (status & ~(f)))
#define SET_ZN(v) (status = ((status & ~(Z | N)) | zn_table[(v)]))

class Bus;
class Cpu6502
{
public:
    Cpu6502();
    ~Cpu6502();

public:
    Apu2A03 apu;

    // Status Register Flags
    enum FLAGS
    {
        C = (1 << 0), // Carry Bit
        Z = (1 << 1), // Zero Bit
        I = (1 << 2), // Interrupt Bit
        D = (1 << 3), // Decimal Bit
        B = (1 << 4), // Break Bit
        U = (1 << 5), // Unused Bit
        V = (1 << 6), // Overflow Bit
        N = (1 << 7)  // Negative Bit
    };

    void apuWrite(uint16_t addr, uint8_t data);
    uint8_t apuRead(uint16_t addr);
    void clock(int i);
    void OAM_DMA(uint8_t page);
    void reset();

    void IRQ();
    void NMI();

    void dumpState(File& state);
    void loadState(File& state);

    void connectBus(Bus* n) { bus = n; }
    void connectCartridge(Cartridge* cartridge) { cart = cartridge; }

    // Registers 
    uint8_t A = 0x00; // Accumulator
    uint8_t X = 0x00; // X Index
    uint8_t Y = 0x00; // Y Index
    uint16_t PC = 0x0000; // Program Counter
    uint8_t SP = 0x00; // Stack Pointer
    uint8_t status = 0x00; // Status register

    uint8_t fetched = 0x00;
    uint16_t addr_abs = 0x0000;
    uint16_t addr_rel = 0x0000;
    uint8_t opcode = 0x00;
    uint16_t cycles = 0;
    uint16_t temp = 0x0000;

private: 
    Cartridge* cart = nullptr;
	Bus* bus = nullptr;
    bool addrmode_implied = false;
    uint8_t additional_cycle1 = 0;
    uint8_t additional_cycle2 = 0;
    uint8_t fetch();
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);
    void OAM_Write(uint8_t addr, uint8_t data);

	// Addressing Modes
	uint8_t ABS();	uint8_t IDX();
	uint8_t ABX();	uint8_t IDY();
	uint8_t ABY();	uint8_t REL();
	uint8_t IMM();	uint8_t ZPG();
	uint8_t IMP();	uint8_t ZPX();
	uint8_t IND();	uint8_t ZPY();		

	// Instructions
	uint8_t Instr_ADC(); uint8_t Instr_CLI(); uint8_t Instr_LDX(); uint8_t Instr_SED();
	uint8_t Instr_AND(); uint8_t Instr_CLV(); uint8_t Instr_LDY(); uint8_t Instr_SEI();
	uint8_t Instr_ASL(); uint8_t Instr_CMP(); uint8_t Instr_LSR(); uint8_t Instr_STA();
	uint8_t Instr_BCC(); uint8_t Instr_CPX(); uint8_t Instr_NOP(); uint8_t Instr_STX();
	uint8_t Instr_BCS(); uint8_t Instr_CPY(); uint8_t Instr_ORA(); uint8_t Instr_STY();
	uint8_t Instr_BEQ(); uint8_t Instr_DEC(); uint8_t Instr_PHA(); uint8_t Instr_TAX();
	uint8_t Instr_BIT(); uint8_t Instr_DEX(); uint8_t Instr_PHP(); uint8_t Instr_TAY();
	uint8_t Instr_BMI(); uint8_t Instr_DEY(); uint8_t Instr_PLA(); uint8_t Instr_TSX();
	uint8_t Instr_BNE(); uint8_t Instr_EOR(); uint8_t Instr_PLP(); uint8_t Instr_TXA();
	uint8_t Instr_BPL(); uint8_t Instr_INC(); uint8_t Instr_ROL(); uint8_t Instr_TXS();
	uint8_t Instr_BRK(); uint8_t Instr_INX(); uint8_t Instr_ROR(); uint8_t Instr_TYA();
	uint8_t Instr_BVC(); uint8_t Instr_INY(); uint8_t Instr_RTI(); uint8_t Instr_CLD();
	uint8_t Instr_BVS(); uint8_t Instr_JMP(); uint8_t Instr_RTS(); uint8_t Instr_LDA();
	uint8_t Instr_CLC(); uint8_t Instr_JSR(); uint8_t Instr_SBC(); uint8_t Instr_SEC();
	uint8_t Instr_XXX();

    // Instruction cycle count
    static const uint8_t instr_cycles[256];
    static const uint8_t zn_table[256];

    void DMC_DMA_Load();
    void DMC_DMA_Reload();

    uint16_t OAM_DMA_page = 0x00;
};

#endif
// ===== END src/core/cpu6502.h =====

// ===== BEGIN src/core/ppu2C02.h =====
#ifndef PPU2C02_H
#define PPU2C02_H



#define BUFFER_SIZE 256 + 8 + 8
#define SCANLINE_SIZE 256
#define SCANLINES_PER_BUFFER 10
#define TILES_PER_SCANLINE 32
#define PIXELS_PER_TILE 8

class Bus;
class Ppu2C02
{
public:
    Ppu2C02();
    ~Ppu2C02();

public:
    void ppuWrite(uint16_t addr, uint8_t data);
    uint8_t ppuRead(uint16_t addr);
    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);

    void renderScanline(uint16_t scanline);
    void fakeSpriteHit(uint16_t scanline);
    void setVBlank();
    void clearVBlank();
    void reset();

    void connectBus(Bus* n) { bus = n; }
    void connectCartridge(Cartridge* cartridge);
    void setMirror(Cartridge::MIRROR mirror);
    Cartridge::MIRROR getMirror();

    void dumpState(File& state);
    void loadState(File& state);

    enum Palette
    {
        NTSC565,
        PAL565,
        NTSC222,
        PAL222,
        PaletteCount
    };
    void setPalette(uint8_t palette);
private:
    Cartridge* cart = nullptr;
    Bus* bus = nullptr;

    void renderBackground();
    void renderSprites(uint16_t scanline);
    void transferScroll(uint16_t scanline);
    void incrementY();
    void finishScanline(uint16_t scanline);
    uint16_t scanline_buffer[BUFFER_SIZE];
    uint8_t scanline_metadata[BUFFER_SIZE];
    static uint16_t display_buffer[SCANLINE_SIZE * SCANLINES_PER_BUFFER];
    uint8_t nametable[2048];
    uint8_t* ptr_nametable[4];
    uint8_t palette_table[32];
    uint8_t scanline_counter = 0;

    // NTSC888 Palette in RGB565
    static constexpr uint16_t palette_NTSC565[8][64] = 
    {
        {
            0x630C, 0x00F2, 0x1835, 0x4013, 0x600D, 0x6804, 0x6020, 0x48E0,
            0x21A0, 0x0240, 0x0260, 0x0242, 0x01AB, 0x0000, 0x0000, 0x0000,
            0xAD55, 0x0A7B, 0x397F, 0x70BE, 0x9857, 0xB08C, 0xA920, 0x8A20,
            0x5320, 0x23E0, 0x0440, 0x0406, 0x0372, 0x0000, 0x0000, 0x0000,
            0xFFFF, 0x553F, 0x843F, 0xB37F, 0xDB1F, 0xFB18, 0xFBAD, 0xDC84,
            0xB560, 0x8640, 0x56A4, 0x3E8D, 0x3E19, 0x4A69, 0x0000, 0x0000,
            0xFFFF, 0xBF1F, 0xCEBF, 0xE65F, 0xF63F, 0xFE3D, 0xFE59, 0xF6B5,
            0xE6F3, 0xD753, 0xC775, 0xB778, 0xB75C, 0xBDD7, 0x0000, 0x0000,
        },
        {
            0x6228, 0x004E, 0x1810, 0x400F, 0x5809, 0x6801, 0x6000, 0x4880,
            0x2140, 0x01A0, 0x01C0, 0x0180, 0x00E7, 0x0000, 0x0000, 0x0000,
            0xAC10, 0x1195, 0x40B9, 0x7018, 0x9812, 0xA828, 0xA0C0, 0x89A0,
            0x5A80, 0x2320, 0x0340, 0x0303, 0x026D, 0x0000, 0x0000, 0x0000,
            0xFE7A, 0x5BFD, 0x831F, 0xB27F, 0xE21C, 0xF233, 0xF2C8, 0xDB80,
            0xB460, 0x8520, 0x5D60, 0x4549, 0x44D4, 0x51A5, 0x0000, 0x0000,
            0xFE7A, 0xC59A, 0xCD3D, 0xE4FD, 0xF4DB, 0xFCD8, 0xFCF4, 0xF550,
            0xE58E, 0xD5EE, 0xC610, 0xBE13, 0xBDF7, 0xBC71, 0x0000, 0x0000,
        },
        {
            0x3AE6, 0x00CE, 0x0010, 0x200E, 0x3808, 0x4800, 0x4020, 0x28E0,
            0x09A0, 0x0220, 0x0260, 0x0220, 0x0188, 0x0000, 0x0000, 0x0000,
            0x7D0D, 0x0235, 0x1959, 0x4897, 0x7050, 0x8086, 0x7920, 0x6220,
            0x3300, 0x0BC0, 0x0420, 0x03E2, 0x032D, 0x0000, 0x0000, 0x0000,
            0xC795, 0x34DB, 0x5BFF, 0x8B3F, 0xAAD9, 0xC2F0, 0xC366, 0xAC40,
            0x8540, 0x5600, 0x2E60, 0x1E48, 0x1DB2, 0x2A44, 0x0000, 0x0000,
            0xC795, 0x8E97, 0x9E39, 0xADF9, 0xBDD7, 0xC5D4, 0xC5F0, 0xBE4C,
            0xAEAA, 0x9EEA, 0x8F0C, 0x870F, 0x86F3, 0x856E, 0x0000, 0x0000,
        },
        {
            0x4225, 0x004C, 0x080E, 0x280D, 0x4007, 0x4800, 0x4000, 0x2880,
            0x0920, 0x01A0, 0x01C0, 0x0160, 0x00E7, 0x0000, 0x0000, 0x0000,
            0x840C, 0x0194, 0x20B7, 0x5015, 0x700F, 0x8025, 0x80C0, 0x61A0,
            0x3A60, 0x0B00, 0x0340, 0x0302, 0x026C, 0x0000, 0x0000, 0x0000,
            0xCE55, 0x3BFA, 0x631E, 0x925D, 0xB218, 0xCA2F, 0xC2A5, 0xB380,
            0x8C60, 0x5D00, 0x3540, 0x2527, 0x24B1, 0x31A3, 0x0000, 0x0000,
            0xCE55, 0x9576, 0xA518, 0xB4D8, 0xC4B7, 0xCCB3, 0xCCEF, 0xC52C,
            0xB58A, 0xA5CA, 0x95EC, 0x8DEF, 0x8DD3, 0x8C6E, 0x0000, 0x0000,
        },
        {
            0x4A4F, 0x0073, 0x1016, 0x3014, 0x500E, 0x5805, 0x4800, 0x3060,
            0x0900, 0x0180, 0x01C0, 0x0184, 0x012D, 0x0000, 0x0000, 0x0000,
            0x8C58, 0x01DD, 0x30FF, 0x603F, 0x8018, 0x900D, 0x88A2, 0x6960,
            0x3A40, 0x0B00, 0x0340, 0x0329, 0x02B4, 0x0001, 0x0000, 0x0000,
            0xD6BF, 0x443F, 0x6B5F, 0x9ABF, 0xC23F, 0xD25A, 0xD2CF, 0xB387,
            0x8C62, 0x6522, 0x3D88, 0x2591, 0x2D1C, 0x39AC, 0x0000, 0x0000,
            0xD6BF, 0x9DDF, 0xA57F, 0xBD3F, 0xCD1F, 0xD51F, 0xD53C, 0xCD78,
            0xBDD6, 0xAE36, 0x9E58, 0x965B, 0x8E3F, 0x94B9, 0x0000, 0x0000,
        },
        {
            0x49E9, 0x002E, 0x1011, 0x300F, 0x480A, 0x5003, 0x4800, 0x3040,
            0x08E0, 0x0140, 0x0160, 0x0141, 0x00C8, 0x0000, 0x0000, 0x0000,
            0x8BB1, 0x0157, 0x309A, 0x6019, 0x8012, 0x9009, 0x8860, 0x6940,
            0x3A20, 0x0AA0, 0x02E0, 0x02C5, 0x022F, 0x0000, 0x0000, 0x0000,
            0xD5FB, 0x439E, 0x6ADF, 0x9A1F, 0xB9DD, 0xD1F4, 0xCA6A, 0xB322,
            0x8C00, 0x64A0, 0x3CE3, 0x24EC, 0x2C76, 0x3947, 0x0000, 0x0000,
            0xD5FB, 0x9D1C, 0xACDE, 0xBC7E, 0xCC5C, 0xD459, 0xD495, 0xCCD2,
            0xBD30, 0xAD70, 0x9D91, 0x9595, 0x9579, 0x9413, 0x0000, 0x0000,
        },
        {
            0x3A29, 0x006F, 0x0011, 0x200F, 0x3809, 0x4001, 0x3800, 0x2060,
            0x0100, 0x0180, 0x01A0, 0x0182, 0x0109, 0x0000, 0x0000, 0x0000,
            0x7410, 0x01B7, 0x18DA, 0x4838, 0x6811, 0x7808, 0x70A0, 0x5960,
            0x2A40, 0x0300, 0x0340, 0x0325, 0x028F, 0x0000, 0x0000, 0x0000,
            0xB679, 0x2C1D, 0x533F, 0x827F, 0xA23B, 0xBA32, 0xBAA9, 0xA380,
            0x7C60, 0x4D00, 0x2D62, 0x154B, 0x14D5, 0x29A7, 0x0000, 0x0000,
            0xB679, 0x859B, 0x8D3C, 0xA4FC, 0xACDB, 0xBCD7, 0xBCF3, 0xB550,
            0xA5AE, 0x95EE, 0x8610, 0x7E13, 0x7DD7, 0x7C72, 0x0000, 0x0000,
        },
        {
            0x39E7, 0x002D, 0x000F, 0x200D, 0x4008, 0x4801, 0x4000, 0x2840,
            0x08E0, 0x0140, 0x0160, 0x0140, 0x00C8, 0x0000, 0x0000, 0x0000,
            0x73AE, 0x0155, 0x2098, 0x4816, 0x7010, 0x8006, 0x7880, 0x5940,
            0x3220, 0x02C0, 0x02E0, 0x02C3, 0x022D, 0x0000, 0x0000, 0x0000,
            0xBDF7, 0x339B, 0x5ADF, 0x8A1F, 0xA9D9, 0xB9F1, 0xBA67, 0xA320,
            0x7C00, 0x54A0, 0x2D01, 0x1CEA, 0x1C53, 0x2965, 0x0000, 0x0000,
            0xBDF7, 0x8518, 0x94BA, 0xA47A, 0xB459, 0xBC55, 0xBC91, 0xB4CE,
            0xA52C, 0x956C, 0x858E, 0x7D91, 0x7D55, 0x8410, 0x0000, 0x0000,
        },
    };

    static constexpr uint16_t palette_NTSC222[8][64] = 
    {
        {
            0x52AA, 0x0015, 0x0015, 0x5015, 0x500A, 0x5000, 0x5000, 0x5000,
            0x0000, 0x02A0, 0x02A0, 0x02A0, 0x000A, 0x0000, 0x0000, 0x0000,
            0xAD55, 0x02BF, 0x001F, 0x501F, 0xA815, 0xA80A, 0xA800, 0xAAA0,
            0x52A0, 0x02A0, 0x0540, 0x0540, 0x02B5, 0x0000, 0x0000, 0x0000,
            0xFFFF, 0x555F, 0xAD5F, 0xAABF, 0xFABF, 0xFABF, 0xFAAA, 0xFD40,
            0xAD40, 0xAFE0, 0x57E0, 0x07EA, 0x07FF, 0x52AA, 0x0000, 0x0000,
            0xFFFF, 0xAFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFF5,
            0xFFF5, 0xFFF5, 0xFFF5, 0xAFFF, 0xAFFF, 0xAD55, 0x0000, 0x0000,
        },
        {
            0x52AA, 0x000A, 0x0015, 0x500A, 0x500A, 0x5000, 0x5000, 0x5000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0xAD55, 0x0015, 0x501F, 0x501F, 0xA815, 0xA80A, 0xA800, 0xA800,
            0x52A0, 0x02A0, 0x02A0, 0x02A0, 0x02AA, 0x0000, 0x0000, 0x0000,
            0xFFFF, 0x52BF, 0xAABF, 0xAABF, 0xFABF, 0xFAB5, 0xFAAA, 0xFAA0,
            0xAD40, 0xAD40, 0x5540, 0x554A, 0x5555, 0x5000, 0x0000, 0x0000,
            0xFFFF, 0xFD5F, 0xFD5F, 0xFD5F, 0xFD5F, 0xFD5F, 0xFD55, 0xFD55,
            0xFD4A, 0xFD4A, 0xFFF5, 0xAFF5, 0xAD55, 0xAD55, 0x0000, 0x0000,
        },
        {
            0x02A0, 0x000A, 0x0015, 0x000A, 0x000A, 0x5000, 0x5000, 0x0000,
            0x0000, 0x02A0, 0x02A0, 0x02A0, 0x000A, 0x0000, 0x0000, 0x0000,
            0x554A, 0x02B5, 0x001F, 0x5015, 0x5015, 0xA800, 0x5000, 0x52A0,
            0x02A0, 0x02A0, 0x0540, 0x02A0, 0x02AA, 0x0000, 0x0000, 0x0000,
            0xFFF5, 0x055F, 0x52BF, 0xAABF, 0xAABF, 0xFAB5, 0xFAA0, 0xAD40,
            0xAD40, 0x57E0, 0x07E0, 0x07EA, 0x0555, 0x02A0, 0x0000, 0x0000,
            0xFFF5, 0xAFF5, 0xAFFF, 0xAD5F, 0xAD55, 0xFD55, 0xFD55, 0xAFEA,
            0xAFEA, 0xAFEA, 0xAFEA, 0xAFEA, 0xAFF5, 0xAD4A, 0x0000, 0x0000,
        },
        {
            0x52A0, 0x000A, 0x000A, 0x000A, 0x5000, 0x5000, 0x5000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0xAD4A, 0x0015, 0x0015, 0x5015, 0x500A, 0xA800, 0xA800, 0x5000,
            0x02A0, 0x02A0, 0x02A0, 0x02A0, 0x02AA, 0x0000, 0x0000, 0x0000,
            0xFFF5, 0x02BF, 0x52BF, 0xAABF, 0xAABF, 0xFAAA, 0xFAA0, 0xAAA0,
            0xAD40, 0x5540, 0x0540, 0x0540, 0x0555, 0x0000, 0x0000, 0x0000,
            0xFFF5, 0xAD55, 0xAD5F, 0xAD5F, 0xFD55, 0xFD55, 0xFD4A, 0xFD4A,
            0xAD4A, 0xAD4A, 0xAD4A, 0xAD4A, 0xAD55, 0xAD4A, 0x0000, 0x0000,
        },
        {
            0x52AA, 0x0015, 0x0015, 0x0015, 0x500A, 0x5000, 0x5000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x000A, 0x0000, 0x0000, 0x0000,
            0xAD5F, 0x001F, 0x001F, 0x501F, 0xA81F, 0xA80A, 0xA800, 0x5000,
            0x02A0, 0x02A0, 0x02A0, 0x02AA, 0x02B5, 0x0000, 0x0000, 0x0000,
            0xFFFF, 0x555F, 0x52BF, 0xAABF, 0xFABF, 0xFABF, 0xFAAA, 0xAAA0,
            0xAD40, 0x5540, 0x054A, 0x0555, 0x055F, 0x000A, 0x0000, 0x0000,
            0xFFFF, 0xAD5F, 0xAD5F, 0xAD5F, 0xFD5F, 0xFD5F, 0xFD5F, 0xFD5F,
            0xAD55, 0xAFF5, 0xAFFF, 0xAFFF, 0xAFFF, 0xAD5F, 0x0000, 0x0000,
        },
        {
            0x500A, 0x000A, 0x0015, 0x000A, 0x500A, 0x5000, 0x5000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x000A, 0x0000, 0x0000, 0x0000,
            0xAAB5, 0x0015, 0x001F, 0x501F, 0xA815, 0xA80A, 0xA800, 0x5000,
            0x02A0, 0x02A0, 0x02A0, 0x02A0, 0x02AA, 0x0000, 0x0000, 0x0000,
            0xFD5F, 0x52BF, 0x52BF, 0xAABF, 0xA81F, 0xF815, 0xFAAA, 0xAAA0,
            0xAD40, 0x5540, 0x0540, 0x054A, 0x0555, 0x0000, 0x0000, 0x0000,
            0xFD5F, 0xAD5F, 0xAD5F, 0xAD5F, 0xFD5F, 0xFD5F, 0xFD55, 0xFD55,
            0xAD55, 0xAD55, 0xAD55, 0xAD55, 0xAD5F, 0xAD55, 0x0000, 0x0000,
        },
        {
            0x02AA, 0x000A, 0x0015, 0x000A, 0x000A, 0x5000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x000A, 0x0000, 0x0000, 0x0000,
            0x5555, 0x0015, 0x001F, 0x501F, 0x5015, 0x500A, 0x5000, 0x5000,
            0x02A0, 0x02A0, 0x02A0, 0x02A0, 0x02AA, 0x0000, 0x0000, 0x0000,
            0xAFFF, 0x055F, 0x52BF, 0xAABF, 0xAABF, 0xAAB5, 0xAAAA, 0xAAA0,
            0x5540, 0x5540, 0x0540, 0x054A, 0x0555, 0x0000, 0x0000, 0x0000,
            0xAFFF, 0xAD5F, 0xAD5F, 0xAD5F, 0xAD5F, 0xAD55, 0xAD55, 0xAD55,
            0xAD4A, 0xAD4A, 0xAFF5, 0x57F5, 0x5555, 0x5555, 0x0000, 0x0000,
        },
        {
            0x0000, 0x000A, 0x000A, 0x000A, 0x500A, 0x5000, 0x5000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x000A, 0x0000, 0x0000, 0x0000,
            0x52AA, 0x0015, 0x001F, 0x5015, 0x5015, 0xA800, 0x5000, 0x5000,
            0x02A0, 0x02A0, 0x02A0, 0x02A0, 0x02AA, 0x0000, 0x0000, 0x0000,
            0xAD55, 0x02BF, 0x52BF, 0xAABF, 0xA81F, 0xA815, 0xAAA0, 0xAAA0,
            0x5540, 0x5540, 0x0540, 0x054A, 0x0555, 0x0000, 0x0000, 0x0000,
            0xAD55, 0xAD5F, 0xAD5F, 0xAD5F, 0xAD5F, 0xAD55, 0xAD55, 0xAD4A,
            0xAD4A, 0xAD4A, 0xAD4A, 0x5555, 0x5555, 0xAD55, 0x0000, 0x0000,
        },
    };

    static constexpr uint16_t palette_PAL565[8][64] = 
    {
        {
            0x630C, 0x010C, 0x088F, 0x280F, 0x400C, 0x5006, 0x5020, 0x40A0,
            0x2920, 0x09A0, 0x01E0, 0x01E0, 0x0186, 0x0000, 0x0000, 0x0000,
            0xAD55, 0x1295, 0x31D9, 0x5939, 0x78D5, 0x90CD, 0x9144, 0x79E0,
            0x5AA0, 0x3340, 0x13A0, 0x03A4, 0x032D, 0x0000, 0x0000, 0x0000,
            0xFFFF, 0x651F, 0x845F, 0xABBF, 0xCB5F, 0xE377, 0xE3CE, 0xCC86,
            0xAD42, 0x85E2, 0x6626, 0x4E2E, 0x4DB7, 0x4A69, 0x0000, 0x0000,
            0xFFFF, 0xC6FF, 0xD69F, 0xE65F, 0xEE3F, 0xF65C, 0xF679, 0xEEB6,
            0xE6F4, 0xD734, 0xC756, 0xBF59, 0xBF3C, 0xBDD7, 0x0000, 0x0000,
        },
        {
            0x6248, 0x0088, 0x080C, 0x200C, 0x4009, 0x4804, 0x5000, 0x4040,
            0x28C0, 0x0920, 0x0140, 0x0120, 0x00E3, 0x0000, 0x0000, 0x0000,
            0xA430, 0x11B0, 0x3114, 0x5094, 0x7851, 0x886A, 0x88C2, 0x7960,
            0x5A00, 0x3280, 0x12C0, 0x02A1, 0x0249, 0x0000, 0x0000, 0x0000,
            0xF699, 0x63F9, 0x835E, 0xA2BE, 0xCA7A, 0xDA93, 0xDAEA, 0xCB83,
            0xAC40, 0x84C0, 0x6503, 0x4D0A, 0x4CB2, 0x49A6, 0x0000, 0x0000,
            0xF699, 0xBD9A, 0xCD5B, 0xDD1B, 0xE4FA, 0xECF7, 0xED34, 0xE571,
            0xDDAF, 0xCDEF, 0xBE11, 0xBE14, 0xBDD7, 0xB492, 0x0000, 0x0000,
        },
        {
            0x42C7, 0x00E8, 0x004B, 0x100A, 0x2807, 0x3003, 0x3800, 0x2880,
            0x1900, 0x0180, 0x01C0, 0x01C0, 0x0163, 0x0000, 0x0000, 0x0000,
            0x7CEE, 0x024F, 0x1993, 0x38F3, 0x58AF, 0x68A8, 0x6900, 0x59C0,
            0x4280, 0x1B20, 0x0360, 0x0361, 0x02E9, 0x0000, 0x0000, 0x0000,
            0xCF77, 0x44B8, 0x5BFB, 0x835B, 0xA317, 0xB310, 0xB388, 0xA421,
            0x84E0, 0x6580, 0x45C1, 0x2DC8, 0x2D51, 0x3225, 0x0000, 0x0000,
            0xCF77, 0x9677, 0xA639, 0xADF9, 0xBDD7, 0xC5D4, 0xC5F1, 0xBE2E,
            0xAE6D, 0xA6AD, 0x96CE, 0x8ED1, 0x8EB4, 0x8D4F, 0x0000, 0x0000,
        },
        {
            0x4226, 0x0067, 0x000A, 0x100A, 0x2807, 0x3002, 0x3800, 0x2820,
            0x18A0, 0x0100, 0x0120, 0x0120, 0x00C2, 0x0000, 0x0000, 0x0000,
            0x840D, 0x01AE, 0x1911, 0x3891, 0x584E, 0x6847, 0x68A0, 0x5940,
            0x41E0, 0x2260, 0x02A0, 0x02A0, 0x0228, 0x0000, 0x0000, 0x0000,
            0xCE56, 0x43D6, 0x633A, 0x82BA, 0xA256, 0xB270, 0xB2C8, 0xA361,
            0x8400, 0x64A0, 0x44E1, 0x34C8, 0x346F, 0x3184, 0x0000, 0x0000,
            0xCE56, 0x9576, 0xA518, 0xB4F8, 0xBCD6, 0xC4D3, 0xC4F0, 0xBD2E,
            0xB56C, 0xA5AC, 0x95CE, 0x8DD0, 0x95B3, 0x8C6E, 0x0000, 0x0000,
        },
        {
            0x4A4D, 0x00CC, 0x004F, 0x180F, 0x300C, 0x3807, 0x3801, 0x2820,
            0x10A0, 0x0100, 0x0140, 0x0161, 0x0127, 0x0000, 0x0000, 0x0000,
            0x8C57, 0x0215, 0x2159, 0x48D9, 0x6075, 0x706D, 0x70C5, 0x6140,
            0x41E0, 0x2280, 0x02E0, 0x02E6, 0x028E, 0x0000, 0x0000, 0x0000,
            0xD6BF, 0x4C3F, 0x6B7F, 0x8AFF, 0xAA9F, 0xC298, 0xBAF0, 0xAB89,
            0x8C45, 0x64E5, 0x4D29, 0x3530, 0x3CD8, 0x39CA, 0x0000, 0x0000,
            0xD6BF, 0x9DDF, 0xAD7F, 0xBD3F, 0xC53F, 0xCD3E, 0xCD5B, 0xC598,
            0xBDD6, 0xAE16, 0x9E38, 0x9E3B, 0x9E1E, 0x94B8, 0x0000, 0x0000,
        },
        {
            0x49E9, 0x0069, 0x000C, 0x180C, 0x3009, 0x3805, 0x3800, 0x2800,
            0x1080, 0x00C0, 0x0100, 0x0100, 0x00C4, 0x0000, 0x0000, 0x0000,
            0x83B1, 0x0190, 0x20F4, 0x4074, 0x6031, 0x702A, 0x7083, 0x5900,
            0x41A0, 0x2220, 0x0260, 0x0262, 0x020A, 0x0000, 0x0000, 0x0000,
            0xD5FA, 0x4BBA, 0x62FE, 0x8A7E, 0xAA3A, 0xBA33, 0xBA8B, 0xAB25,
            0x8BC1, 0x6441, 0x4CA5, 0x348B, 0x3C33, 0x3167, 0x0000, 0x0000,
            0xD5FA, 0x9D1B, 0xA4DC, 0xB49C, 0xC47B, 0xCC98, 0xCCB5, 0xC4F2,
            0xB531, 0xAD70, 0x9D92, 0x9575, 0x9558, 0x9413, 0x0000, 0x0000,
        },
        {
            0x3A28, 0x0089, 0x000B, 0x100B, 0x2008, 0x3003, 0x3000, 0x2020,
            0x1080, 0x0100, 0x0140, 0x0140, 0x0104, 0x0000, 0x0000, 0x0000,
            0x7410, 0x01D0, 0x1133, 0x3893, 0x504F, 0x6049, 0x60A2, 0x5120,
            0x31E0, 0x1260, 0x02C0, 0x02A2, 0x026A, 0x0000, 0x0000, 0x0000,
            0xBE79, 0x3BF9, 0x5B5D, 0x7ABD, 0x9279, 0xAA72, 0xAACA, 0x9364,
            0x7C20, 0x54A0, 0x3CE4, 0x2CEB, 0x2C92, 0x29A6, 0x0000, 0x0000,
            0xBE79, 0x8D79, 0x953B, 0xA4FB, 0xACD9, 0xB4F7, 0xB513, 0xAD51,
            0xA58F, 0x95CF, 0x8DD1, 0x85D3, 0x85B7, 0x7C71, 0x0000, 0x0000,
        },
        {
            0x39E7, 0x0047, 0x000A, 0x100A, 0x2007, 0x3003, 0x3000, 0x2000,
            0x1060, 0x00C0, 0x0100, 0x0100, 0x00C3, 0x0000, 0x0000, 0x0000,
            0x73AE, 0x018E, 0x18F2, 0x3872, 0x502E, 0x6028, 0x6081, 0x5100,
            0x39A0, 0x1A20, 0x0260, 0x0261, 0x0208, 0x0000, 0x0000, 0x0000,
            0xBDF7, 0x3B97, 0x5AFB, 0x7A7B, 0x9A17, 0xAA31, 0xAA89, 0x9B03,
            0x7BC0, 0x5C40, 0x3C83, 0x2C89, 0x2C31, 0x2965, 0x0000, 0x0000,
            0xBDF7, 0x8D17, 0x94D9, 0xA499, 0xB477, 0xB475, 0xB492, 0xB4CF,
            0xA50E, 0x954E, 0x8D6F, 0x8572, 0x8555, 0x8410, 0x0000, 0x0000,
        },
    };

    static constexpr uint16_t palette_PAL222[8][64] = 
    {
        {
            0x52AA, 0x000A, 0x000A, 0x000A, 0x500A, 0x5000, 0x5000, 0x5000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0xAD55, 0x02B5, 0x001F, 0x501F, 0x5015, 0xA80A, 0xA800, 0x5000,
            0x52A0, 0x02A0, 0x02A0, 0x02A0, 0x02AA, 0x0000, 0x0000, 0x0000,
            0xFFFF, 0x555F, 0xAD5F, 0xAABF, 0xFABF, 0xFAB5, 0xFAAA, 0xFD40,
            0xAD40, 0xAD40, 0x57E0, 0x57EA, 0x5555, 0x52AA, 0x0000, 0x0000,
            0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFF5,
            0xFFF5, 0xFFF5, 0xFFF5, 0xAFFF, 0xAFFF, 0xAD55, 0x0000, 0x0000,
        },
        {
            0x52AA, 0x000A, 0x000A, 0x000A, 0x500A, 0x5000, 0x5000, 0x5000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0xAD55, 0x0015, 0x0015, 0x5015, 0x5015, 0xA80A, 0xA800, 0x5000,
            0x52A0, 0x02A0, 0x02A0, 0x02A0, 0x02AA, 0x0000, 0x0000, 0x0000,
            0xFFFF, 0x52BF, 0xAABF, 0xAABF, 0xFABF, 0xFAB5, 0xFAAA, 0xFAA0,
            0xAD40, 0xAD40, 0x5540, 0x554A, 0x5555, 0x5000, 0x0000, 0x0000,
            0xFFFF, 0xAD5F, 0xFD5F, 0xFD5F, 0xFD5F, 0xFD55, 0xFD55, 0xFD55,
            0xFD4A, 0xFD4A, 0xAFF5, 0xAFF5, 0xAD55, 0xAD55, 0x0000, 0x0000,
        },
        {
            0x52A0, 0x000A, 0x000A, 0x000A, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x554A, 0x02AA, 0x0015, 0x0015, 0x500A, 0x500A, 0x5000, 0x5000,
            0x52A0, 0x02A0, 0x02A0, 0x02A0, 0x02AA, 0x0000, 0x0000, 0x0000,
            0xFFF5, 0x555F, 0x52BF, 0xAABF, 0xAAB5, 0xAAB5, 0xAAAA, 0xAD40,
            0xAD40, 0x5540, 0x5540, 0x054A, 0x0555, 0x02A0, 0x0000, 0x0000,
            0xFFF5, 0xAFF5, 0xAFFF, 0xAD5F, 0xAD55, 0xFD55, 0xFD55, 0xAFEA,
            0xAFEA, 0xAFEA, 0xAFEA, 0xAFF5, 0xAFF5, 0xAD4A, 0x0000, 0x0000,
        },
        {
            0x52A0, 0x0000, 0x000A, 0x000A, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0xAD4A, 0x000A, 0x0015, 0x0015, 0x500A, 0x5000, 0x5000, 0x5000,
            0x5000, 0x02A0, 0x02A0, 0x02A0, 0x02AA, 0x0000, 0x0000, 0x0000,
            0xFFF5, 0x52B5, 0x52BF, 0xAABF, 0xAAB5, 0xAAB5, 0xAAAA, 0xAAA0,
            0xAD40, 0x5540, 0x5540, 0x054A, 0x054A, 0x0000, 0x0000, 0x0000,
            0xFFF5, 0xAD55, 0xAD5F, 0xAD5F, 0xAD55, 0xFD55, 0xFD55, 0xAD4A,
            0xAD4A, 0xAD4A, 0xAD4A, 0xAD55, 0xAD55, 0xAD4A, 0x0000, 0x0000,
        },
        {
            0x52AA, 0x000A, 0x000A, 0x000A, 0x000A, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0xAD55, 0x02B5, 0x001F, 0x501F, 0x5015, 0x500A, 0x5000, 0x5000,
            0x5000, 0x02A0, 0x02A0, 0x02A0, 0x02AA, 0x0000, 0x0000, 0x0000,
            0xFFFF, 0x555F, 0x52BF, 0xAABF, 0xAABF, 0xFABF, 0xAAB5, 0xAAAA,
            0xAD40, 0x5540, 0x554A, 0x0555, 0x055F, 0x000A, 0x0000, 0x0000,
            0xFFFF, 0xAD5F, 0xAD5F, 0xAD5F, 0xFD5F, 0xFD5F, 0xFD5F, 0xFD5F,
            0xAD55, 0xAFF5, 0xAFFF, 0xAFFF, 0xAFFF, 0xAD5F, 0x0000, 0x0000,
        },
        {
            0x500A, 0x000A, 0x000A, 0x000A, 0x000A, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0xAAB5, 0x0015, 0x0015, 0x5015, 0x5015, 0x500A, 0x5000, 0x5000,
            0x5000, 0x02A0, 0x02A0, 0x02A0, 0x02AA, 0x0000, 0x0000, 0x0000,
            0xFD5F, 0x52BF, 0x52BF, 0xAABF, 0xAABF, 0xAAB5, 0xAAAA, 0xAAA0,
            0xAAA0, 0x5540, 0x5540, 0x054A, 0x0555, 0x0000, 0x0000, 0x0000,
            0xFD5F, 0xAD5F, 0xAD5F, 0xAD5F, 0xFD5F, 0xFD5F, 0xFD55, 0xFD55,
            0xAD55, 0xAD55, 0xAD55, 0xAD55, 0xAD5F, 0xAD55, 0x0000, 0x0000,
        },
        {
            0x02AA, 0x000A, 0x000A, 0x000A, 0x000A, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x5555, 0x0015, 0x0015, 0x0015, 0x500A, 0x500A, 0x5000, 0x5000,
            0x0000, 0x02A0, 0x02A0, 0x02A0, 0x02AA, 0x0000, 0x0000, 0x0000,
            0xAFFF, 0x02BF, 0x52BF, 0x52BF, 0xAABF, 0xAAB5, 0xAAAA, 0xAAA0,
            0x5540, 0x5540, 0x0540, 0x054A, 0x0555, 0x0000, 0x0000, 0x0000,
            0xAFFF, 0xAD5F, 0xAD5F, 0xAD5F, 0xAD5F, 0xAD55, 0xAD55, 0xAD55,
            0xAD4A, 0xAD4A, 0xAD55, 0xAD55, 0xAD55, 0x5555, 0x0000, 0x0000,
        },
        {
            0x0000, 0x0000, 0x000A, 0x000A, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x52AA, 0x000A, 0x0015, 0x0015, 0x500A, 0x500A, 0x5000, 0x5000,
            0x0000, 0x02A0, 0x02A0, 0x02A0, 0x02AA, 0x0000, 0x0000, 0x0000,
            0xAD55, 0x02B5, 0x52BF, 0x52BF, 0xAAB5, 0xAAB5, 0xAAAA, 0xAAA0,
            0x52A0, 0x5540, 0x0540, 0x054A, 0x0555, 0x0000, 0x0000, 0x0000,
            0xAD55, 0xAD55, 0xAD5F, 0xAD5F, 0xAD55, 0xAD55, 0xAD55, 0xAD4A,
            0xAD4A, 0xAD4A, 0xAD4A, 0xAD55, 0xAD55, 0xAD55, 0x0000, 0x0000,
        },
    };

    const uint16_t (*nes_palette)[64] = palette_NTSC565;
    static constexpr uint8_t palette_mirror[32] =
    {
        0x00,0x01,0x02,0x03, 0x04,0x05,0x06,0x07,
        0x08,0x09,0x0A,0x0B, 0x0C,0x0D,0x0E,0x0F,
        0x00,0x11,0x12,0x13, 0x04,0x15,0x16,0x17,
        0x08,0x19,0x1A,0x1B, 0x0C,0x1D,0x1E,0x1F
    };

    // PPU Registers
    // PPUCTRL
    union
    {
        struct
        {
            uint8_t nametable_x : 1;
            uint8_t nametable_y : 1;
            uint8_t VRAM_addr_increment : 1;
            uint8_t sprite_table_addr : 1;
            uint8_t background_table_addr : 1;
            uint8_t sprite_size : 1;
            uint8_t PPU_master_select : 1;
            uint8_t Vblank_NMI : 1;
        };
        uint8_t reg = 0x00;
    } control;

    // PPUMASK
    union
    {
        struct
        {
            uint8_t grayscale : 1;
            uint8_t render_background_left : 1;
            uint8_t render_sprite_left : 1;
            uint8_t render_background : 1;
            uint8_t render_sprite : 1;
            uint8_t emphasize : 3;
        };
        uint8_t reg = 0x00;
    } mask;

    // PPUSTATUS
    union
    {
        struct
        {
            uint8_t unused : 5;
            uint8_t sprite_overflow : 1;
            uint8_t sprite_zero_hit : 1;
            uint8_t VBlank : 1;
        };
        uint8_t reg = 0x00;
    } status;

    // OAMADDR
    uint8_t OAMADDR = 0x00;
    // OAMDATA
    uint8_t OAMDATA = 0x00;

    // Internal register 
    typedef union
    {
        struct
        {
            uint16_t coarse_x : 5;
            uint16_t coarse_y : 5;
            uint16_t nametable_x : 1;
            uint16_t nametable_y : 1;
            uint16_t fine_y: 3;
            uint16_t unused : 1;
        };
        uint16_t reg = 0x00;
    } internal_register;

    // OAM
    typedef struct
    {
        uint8_t y;
        uint8_t index;
        uint8_t attribute;
        uint8_t x;
    } OAM;

    OAM sprite[64];
    internal_register v;
    internal_register t;
    uint8_t x;
    uint8_t w;
    uint8_t PPUDATA_buffer = 0x00;

    // Rendering
    uint16_t offset = 0x0000;
    uint8_t nametable_index = 0x00;
    uint16_t nametable_byte_base = 0x00;
    uint16_t attribute_byte_base = 0x00;
    uint8_t nametable_byte = 0x00;
    uint8_t attribute_byte = 0x00;
    uint8_t x_tile = 0x00;
    uint8_t y_tile = 0x00;
    uint8_t attribute_shift = 0x00;
    uint8_t attribute = 0x00;
    uint8_t tile_index = 0x00;
    uint8_t* ptr_tile = nullptr;
    uint8_t* ptr_attribute = nullptr;
    uint8_t* ptr_pattern_tile = nullptr;
    uint8_t* ptr_scanline_meta = nullptr;

    uint8_t sprite_count = 0;
public:
    uint8_t* ptr_sprite = (uint8_t*)sprite;
    uint16_t* ptr_buffer = scanline_buffer;
    static constexpr uint16_t* ptr_display = display_buffer;
};

#endif
// ===== END src/core/ppu2C02.h =====

// ===== BEGIN src/core/bus.h =====
#ifndef BUS_H
#define BUS_H


class Bus
{
public:
    Bus();
    ~Bus();

public:
    Cpu6502 cpu;
    Ppu2C02 ppu;
    Cartridge* cart;
    uint8_t RAM[2048];
    uint8_t controller = 0x00;

    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);
    void setPPUMirrorMode(Cartridge::MIRROR mirror);
    Cartridge::MIRROR getPPUMirrorMode();

    void insertCartridge(Cartridge* cartridge);
    void connectScreen(TFT_eSPI* screen);
    void reset();
    void clock();
    void IRQ();
    void NMI();
    void OAM_Write(uint8_t addr, uint8_t data);
    uint16_t ppu_scanline = 0;
    void renderImage(uint16_t scanline);

    void saveState();
    void loadState();

private:
    void cpuClock();
    TFT_eSPI* ptr_screen;
    uint8_t controller_state;
    uint8_t controller_strobe = 0x00;
    bool frame_latch = false;
};

#endif
// ===== END src/core/bus.h =====

// ===== BEGIN src/ui.h =====
#ifndef UI_H
#define UI_H



#define BG_COLOR 0x0015
#define BAR_COLOR 0xAD55
#define TEXT_COLOR 0xFFFF
#define TEXT2_COLOR 0xA800
#define SELECTED_TEXT_COLOR 0x57CA
#define SELECTED_BG_COLOR 0x0560

class UI
{
public:
    UI(TFT_eSPI* screen);
    ~UI();
    Cartridge* selectGame();
    void getNesFiles();
    void drawFileList();
    void drawWindowBox(int x, int y, int w, int h);
    void drawBars();
    void pauseMenu(Bus* nes);
    void settingsMenu(Bus* nes);
    void initializeSettings(Bus* nes);

    bool paused = false;

private:
    void drawText(const char* text, const int x, const int y);
    TFT_eSPI* screen = nullptr;
    int selected = 0;
    int prev_selected = 0;
    int scroll_offset = 0;
    int max_items = 0;
    static constexpr int ITEM_HEIGHT = 12;
    std::vector<std::string> files;

    typedef struct Settings
    {
        uint8_t volume = 100;
        uint8_t palette = 0;
    };
    Settings settings;
    void saveSettings(const Settings* s);
    void loadSettings(Settings* s);
};

#endif
// ===== END src/ui.h =====

// ===== BEGIN src/core/mapper.cpp =====

void bankInit(BankCache* cache, Bank* banks, uint8_t num_banks, uint32_t bank_size, Cartridge* cart)
{
    cache->banks = banks;
    cache->num_banks = num_banks;
    cache->tick = 0;
    cache->cart = cart;

    for (int i = 0; i < num_banks; i++)
    {
        cache->banks[i].bank_id = 0xFF;
        cache->banks[i].last_used = 0;
        cache->banks[i].size = bank_size;

        uint8_t* ptr = (uint8_t*)malloc(bank_size);
        #ifdef DEBUG
            if (!ptr) Serial.printf("%i KB for bank %d Allocation failed.\n", bank_size / 1024, i);
            else
            {
                Serial.printf("Allocated %i KB for bank %d, free heap: %u bytes\n",
                bank_size / 1024, i, heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
            }
        #endif
        cache->banks[i].bank_ptr = ptr;
    }
}

IRAM_ATTR uint8_t* getBank(BankCache* cache, uint8_t bank_id, Mapper::ROM_TYPE rom)
{
    cache->tick++;

    for (int i = 0, n = cache->num_banks; i < n; i++)
    {
        if (cache->banks[i].bank_id == bank_id)
        {
            cache->banks[i].last_used = cache->tick;
            return cache->banks[i].bank_ptr;
        }
    }

    int bank_index = 0;
    uint32_t min_use = cache->banks[0].last_used;
    for (int i = 1, n = cache->num_banks; i < n; i++)
    {
        if (cache->banks[i].last_used < min_use)
        {
            min_use = cache->banks[i].last_used;
            bank_index = i;
        }
    } 

    uint8_t* bank = cache->banks[bank_index].bank_ptr;
    uint32_t size = cache->banks[bank_index].size;
    if (rom == Mapper::ROM_TYPE::PRG_ROM) cache->cart->loadPRGBank(bank, size, bank_id * size);
    else if (rom == Mapper::ROM_TYPE::CHR_ROM) cache->cart->loadCHRBank(bank, size, bank_id * size);

    cache->banks[bank_index].bank_id = bank_id;
    cache->banks[bank_index].last_used = cache->tick;

    return bank;
}

uint8_t getBankIndex(BankCache* cache, uint8_t* ptr)
{
    for (int i = 0, banks = cache->num_banks; i < banks; i++)
    {
        if (cache->banks[i].bank_ptr == ptr)
            return cache->banks[i].bank_id;
    }

    return 0;
}

void invalidateCache(BankCache* cache)
{
    if (!cache || !cache->banks) return;

    cache->tick = 0;
    for (int i = 0, n = cache->num_banks; i < n; i++)
    {
        cache->banks[i].bank_id = 0xFF;
        cache->banks[i].last_used = 0;
    }
}
// ===== END src/core/mapper.cpp =====

// ===== BEGIN src/core/mappers/mapper000.cpp =====

IRAM_ATTR bool mapper000_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr < 0x8000) return false;

    Mapper000_state* state = (Mapper000_state*)mapper->state;
    uint16_t offset = addr & 0x3FFF;
    uint8_t bank_id = (addr >> 14) & 1;

    data = state->PRG_banks[bank_id][offset];
    return true;
}

IRAM_ATTR bool mapper000_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
	return false;
}

IRAM_ATTR bool mapper000_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr > 0x1FFF) return false;

    Mapper000_state* state = (Mapper000_state*)mapper->state;
    data = state->CHR_bank[addr];
    return true;
}

IRAM_ATTR bool mapper000_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr > 0x1FFF) return false;

    Mapper000_state* state = (Mapper000_state*)mapper->state;
    if (state->number_CHR_banks == 0)
    {
        // Treat as RAM
        state->CHR_bank[addr] = data;
        return true;
    }

	return false;
}

IRAM_ATTR uint8_t* mapper000_ppuReadPtr(Mapper* mapper, uint16_t addr)
{
    if (addr > 0x1FFF) return nullptr;
    
    Mapper000_state* state = (Mapper000_state*)mapper->state;
    return &state->CHR_bank[addr];
}

void mapper000_reset(Mapper* mapper)
{
    Mapper000_state* state = (Mapper000_state*)mapper->state;
    state->PRG_banks[0] = &state->PRG_ROM[0];
    state->PRG_banks[1] = (state->number_PRG_banks > 1) ? &state->PRG_ROM[16*1024] : &state->PRG_ROM[0];
    state->CHR_bank = &state->CHR_ROM[0];

    state->cart->loadPRGBank(state->PRG_banks[0], 16*1024, 0);
    if (state->number_PRG_banks > 1) state->cart->loadPRGBank(state->PRG_banks[1], 16*1024, 16*1024);
    state->cart->loadCHRBank(state->CHR_bank, 8*1024, 0);
}

void mapper000_dumpState(Mapper* mapper, File& state)
{
    Mapper000_state* s = (Mapper000_state*)mapper->state;

    if (s->number_CHR_banks == 0 && s->CHR_bank)
    {
        state.write(s->CHR_bank, 8*1024);
    }
}

void mapper000_loadState(Mapper* mapper, File& state)
{
    Mapper000_state* s = (Mapper000_state*)mapper->state;

    if (s->number_CHR_banks == 0 && s->CHR_bank)
    {
        state.read(s->CHR_bank, 8 * 1024);
    }
}

const MapperVTable mapper000_vtable = 
{
    mapper000_cpuRead,
    mapper000_cpuWrite,
    mapper000_ppuRead,
    mapper000_ppuWrite,
    mapper000_ppuReadPtr,
    mapperNoScanline,
    mapperNoCycle,
    mapper000_reset,
    mapper000_dumpState,
    mapper000_loadState,
};

Mapper createMapper000(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart)
{
    Mapper mapper;
    mapper.vtable = &mapper000_vtable; 

    Mapper000_state* state = new Mapper000_state;
    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;

    mapper.state = state;
    return mapper;
}
// ===== END src/core/mappers/mapper000.cpp =====

// ===== BEGIN src/core/mappers/mapper001.cpp =====

struct Mapper001_state
{
    Cartridge* cart;
    uint8_t* RAM;
    uint8_t* CHR_RAM;

    uint8_t number_PRG_banks;
    uint8_t number_CHR_banks;
    uint8_t* ptr_16K_PRG_banks[4];
    uint8_t* ptr_8K_CHR_bank;
    uint8_t* ptr_4K_CHR_banks[2];

    Bank PRG_banks_16K[MAPPER001_NUM_PRG_BANKS_16K];
    Bank CHR_banks_8K[MAPPER001_NUM_CHR_BANKS_8K];
    Bank CHR_banks_4K[MAPPER001_NUM_CHR_BANKS_4K];
    BankCache PRG_16K_cache;
    BankCache CHR_8K_cache;
    BankCache CHR_4K_cache;

    uint8_t load = 0x00; // Load Register
	uint8_t control = 0x1C; // Control register
    uint8_t load_writes = 0; // Keeps track of number of writes to load register
							 // 5 writes == move data to register

    uint8_t PRG_ROM_bank_mode = 0x03;
	uint8_t CHR_ROM_bank_mode = 0;
    uint8_t CHR_bank_0 = 0x00; // CHR Bank 0 Register
	uint8_t CHR_bank_1 = 0x00; // CHR Bank 1 Register
	uint8_t PRG_bank = 0x00; // PRG Bank Register

    static constexpr Cartridge::MIRROR mirror[4] = 
    {
        Cartridge::MIRROR::ONESCREEN_LOW,
        Cartridge::MIRROR::ONESCREEN_HIGH,
        Cartridge::MIRROR::VERTICAL,
        Cartridge::MIRROR::HORIZONTAL
    };
};
constexpr Cartridge::MIRROR Mapper001_state::mirror[4];

IRAM_ATTR bool Mapper001_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr < 0x6000) return false;

    Mapper001_state* state = (Mapper001_state*)mapper->state;
    if (addr < 0x8000)
	{
		data = state->RAM[addr & 0x1FFF];
		return true;
	}

    if (state->PRG_ROM_bank_mode < 2)
    {
        uint8_t bank = ((addr >> 14) & 0x01) + 2;
        data = state->ptr_16K_PRG_banks[bank][addr & 0x3FFF];
        return true;
    }
    
    data = state->ptr_16K_PRG_banks[(addr >> 14) & 1][addr & 0x3FFF];
    return true;
}

IRAM_ATTR bool Mapper001_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr < 0x6000) return false;

    Mapper001_state* state = (Mapper001_state*)mapper->state;
    if (addr < 0x8000)
	{
		state->RAM[addr & 0x1FFF] = data;
		return true;
	}
    
    // If bit 7 is set clear load shift register
    if (!(data & 0x80))
    {
        state->load = (state->load >> 1) | ((data & 0x01) << 4);
        state->load_writes++;

        if (state->load_writes == 5)
        {
            // Write data to register
            switch ((addr >> 13) & 0x03)
            {
            // Control Register
            case 0:
                state->control = state->load & 0x1F;
                state->CHR_ROM_bank_mode = (state->control >> 4) & 0x01;
                state->PRG_ROM_bank_mode = (state->control >> 2) & 0x03;

                // Set mirror mode
                state->cart->setMirrorMode(state->mirror[state->control & 0x03]);
                break;

            // CHR bank 0 Register
            case 1:
                state->CHR_bank_0 = state->load & 0x1F;

                if (state->CHR_ROM_bank_mode == 0)
                {
                    if (state->number_CHR_banks == 0) state->cart->loadCHRBank(state->ptr_8K_CHR_bank, 8*1024, (state->CHR_bank_0 & 0x1E) * 8*1024);
                    else state->ptr_8K_CHR_bank = getBank(&state->CHR_8K_cache, state->CHR_bank_0 & 0x1E, Mapper::ROM_TYPE::CHR_ROM);
                }
                else
                {
                    if (state->number_CHR_banks == 0) state->cart->loadCHRBank(state->ptr_4K_CHR_banks[0], 4*1024, state->CHR_bank_0 * 4*1024);
                    else state->ptr_4K_CHR_banks[0] = getBank(&state->CHR_4K_cache, state->CHR_bank_0, Mapper::ROM_TYPE::CHR_ROM);
                }
                break;

            // CHR bank 1 Register
            case 2:
                state->CHR_bank_1 = state->load & 0x1F;

                if (state->CHR_ROM_bank_mode == 1)
                {
                    if (state->number_CHR_banks == 0) state->cart->loadCHRBank(state->ptr_4K_CHR_banks[1], 4*1024, state->CHR_bank_1 * 4*1024);
                    state->ptr_4K_CHR_banks[1] = getBank(&state->CHR_4K_cache, state->CHR_bank_1, Mapper::ROM_TYPE::CHR_ROM);
                }
                break;

            // PRG bank Register 
            case 3:
                state->PRG_bank = state->load & 0x1F;

                switch (state->PRG_ROM_bank_mode)
                {
                case 0: 
                case 1: 
                    state->ptr_16K_PRG_banks[2] = getBank(&state->PRG_16K_cache, state->PRG_bank & 0x0E, Mapper::ROM_TYPE::PRG_ROM); 
                    state->ptr_16K_PRG_banks[3] = getBank(&state->PRG_16K_cache, (state->PRG_bank & 0x0E) + 1, Mapper::ROM_TYPE::PRG_ROM); 
                    break;
                case 2:
                    state->ptr_16K_PRG_banks[0] = getBank(&state->PRG_16K_cache, 0, Mapper::ROM_TYPE::PRG_ROM);
                    state->ptr_16K_PRG_banks[1] = getBank(&state->PRG_16K_cache, state->PRG_bank & 0x0F, Mapper::ROM_TYPE::PRG_ROM);
                    break;
                case 3:
                    state->ptr_16K_PRG_banks[0] = getBank(&state->PRG_16K_cache, state->PRG_bank & 0x0F, Mapper::ROM_TYPE::PRG_ROM);
                    state->ptr_16K_PRG_banks[1] = getBank(&state->PRG_16K_cache, state->number_PRG_banks - 1, Mapper::ROM_TYPE::PRG_ROM);
                    break;
                }
                break;
            }

            // Reset Load Register and counter
            state->load = 0x00;
            state->load_writes = 0;
        }
    }
    else
    {
        state->load = 0x00;	
        state->load_writes = 0;
        state->control |= 0x0C;
    }
    return true;
}

IRAM_ATTR bool Mapper001_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr > 0x1FFF) return false;

    Mapper001_state* state = (Mapper001_state*)mapper->state;
    if (state->CHR_ROM_bank_mode == 0)
    {
        data = state->ptr_8K_CHR_bank[addr & 0x1FFF];
    }
    else
    {
        data = state->ptr_4K_CHR_banks[(addr >> 12) & 1][addr & 0x0FFF];
    }
    return true;
}

IRAM_ATTR bool Mapper001_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr > 0x1FFF) return false;
	
    Mapper001_state* state = (Mapper001_state*)mapper->state;
    if (state->number_CHR_banks == 0)
    {
        // Treat as RAM
        state->CHR_RAM[addr & 0x1FFF] = data;
        return true;
    }

	return false;
}

IRAM_ATTR uint8_t* Mapper001_ppuReadPtr(Mapper* mapper, uint16_t addr)
{
    if (addr > 0x1FFF) return nullptr;

    Mapper001_state* state = (Mapper001_state*)mapper->state;
    if (state->CHR_ROM_bank_mode == 0)
    {
        return &state->ptr_8K_CHR_bank[addr & 0x1FFF];
    }
    else
    {
        return &state->ptr_4K_CHR_banks[(addr >> 12) & 1][addr & 0x0FFF];
    }
}

void Mapper001_reset(Mapper* mapper)
{
    Mapper001_state* state = (Mapper001_state*)mapper->state;
    memset(state->RAM, 0, 8 * 1024);

    if (state->number_CHR_banks == 0) 
    {
        // Point 4K banks into the same memory
        state->ptr_8K_CHR_bank = state->CHR_RAM;
        state->ptr_4K_CHR_banks[0] = state->CHR_RAM;
        state->ptr_4K_CHR_banks[1] = state->CHR_RAM + 0x1000;
    }
    else
    {
        state->ptr_8K_CHR_bank = getBank(&state->CHR_8K_cache, 0, Mapper::ROM_TYPE::CHR_ROM);
        state->ptr_4K_CHR_banks[0] = getBank(&state->CHR_4K_cache, 0, Mapper::ROM_TYPE::CHR_ROM);
        state->ptr_4K_CHR_banks[1] = getBank(&state->CHR_4K_cache, 0, Mapper::ROM_TYPE::CHR_ROM);
    }

    state->ptr_16K_PRG_banks[0] = getBank(&state->PRG_16K_cache, 0, Mapper::ROM_TYPE::PRG_ROM);
    state->ptr_16K_PRG_banks[1] = getBank(&state->PRG_16K_cache, state->number_PRG_banks - 1, Mapper::ROM_TYPE::PRG_ROM);
    state->ptr_16K_PRG_banks[2] = getBank(&state->PRG_16K_cache, 0, Mapper::ROM_TYPE::PRG_ROM);
    state->ptr_16K_PRG_banks[3] = getBank(&state->PRG_16K_cache, 1, Mapper::ROM_TYPE::PRG_ROM);

    state->load = 0x00;
	state->control = 0x1C;
    state->load_writes = 0;
    state->PRG_ROM_bank_mode = 0x03;
	state->CHR_ROM_bank_mode = 0;
    state->CHR_bank_0 = 0x00;
	state->CHR_bank_1 = 0x00;
	state->PRG_bank = 0x00;
    state->cart->setMirrorMode(Cartridge::MIRROR::HORIZONTAL);
}

void Mapper001_dumpState(Mapper* mapper, File& state)
{
    Mapper001_state* s = (Mapper001_state*)mapper->state;
    Cartridge::MIRROR mirror = s->cart->getMirrorMode();
    state.write((uint8_t*)&s->load, sizeof(s->load));
    state.write((uint8_t*)&s->control, sizeof(s->control));
    state.write((uint8_t*)&s->load_writes, sizeof(s->load_writes));
    state.write((uint8_t*)&s->PRG_ROM_bank_mode, sizeof(s->PRG_ROM_bank_mode));
    state.write((uint8_t*)&s->CHR_ROM_bank_mode, sizeof(s->CHR_ROM_bank_mode));
    state.write((uint8_t*)&s->CHR_bank_0, sizeof(s->CHR_bank_0));
    state.write((uint8_t*)&s->CHR_bank_1, sizeof(s->CHR_bank_1));
    state.write((uint8_t*)&s->PRG_bank, sizeof(s->PRG_bank));
    state.write((uint8_t*)&mirror, sizeof(mirror));
    state.write(s->RAM, 8*1024);

    uint8_t PRG_16K[4];
    uint8_t CHR_8K;
    uint8_t CHR_4K[2];
    for (int i = 0; i < 4; i++) PRG_16K[i] = getBankIndex(&s->PRG_16K_cache, s->ptr_16K_PRG_banks[i]);
    state.write(PRG_16K, sizeof(PRG_16K));
    if (s->number_CHR_banks == 0)
    {
        state.write(s->CHR_RAM, 8*1024);
    }
    else
    {
        CHR_8K = getBankIndex(&s->CHR_8K_cache, s->ptr_8K_CHR_bank);
        for (int i = 0; i < 2; i++) CHR_4K[i] = getBankIndex(&s->CHR_4K_cache, s->ptr_4K_CHR_banks[i]);
        
        state.write((uint8_t*)&CHR_8K, sizeof(CHR_8K));
        state.write(CHR_4K, sizeof(CHR_4K));
    }
}

void Mapper001_loadState(Mapper* mapper, File& state)
{
    Mapper001_state* s = (Mapper001_state*)mapper->state;
    Cartridge::MIRROR mirror;
    state.read((uint8_t*)&s->load, sizeof(s->load));
    state.read((uint8_t*)&s->control, sizeof(s->control));
    state.read((uint8_t*)&s->load_writes, sizeof(s->load_writes));
    state.read((uint8_t*)&s->PRG_ROM_bank_mode, sizeof(s->PRG_ROM_bank_mode));
    state.read((uint8_t*)&s->CHR_ROM_bank_mode, sizeof(s->CHR_ROM_bank_mode));
    state.read((uint8_t*)&s->CHR_bank_0, sizeof(s->CHR_bank_0));
    state.read((uint8_t*)&s->CHR_bank_1, sizeof(s->CHR_bank_1));
    state.read((uint8_t*)&s->PRG_bank, sizeof(s->PRG_bank));
    state.read((uint8_t*)&mirror, sizeof(mirror));
    state.read(s->RAM, 8*1024);
    s->cart->setMirrorMode(mirror);

    uint8_t PRG_16K[4];
    uint8_t CHR_8K;
    uint8_t CHR_4K[2];
    state.read(PRG_16K, sizeof(PRG_16K));
    invalidateCache(&s->PRG_16K_cache);
    for (int i = 0; i < 4; i++) s->ptr_16K_PRG_banks[i] = getBank(&s->PRG_16K_cache, PRG_16K[i], Mapper::ROM_TYPE::PRG_ROM);
    if (s->number_CHR_banks == 0)
    {
        state.read(s->CHR_RAM, 8*1024);
    }
    else
    {
        state.read((uint8_t*)&CHR_8K, sizeof(CHR_8K));
        state.read(CHR_4K, sizeof(CHR_4K));

        invalidateCache(&s->CHR_8K_cache);
        invalidateCache(&s->CHR_4K_cache);
        s->ptr_8K_CHR_bank = getBank(&s->CHR_8K_cache, CHR_8K, Mapper::ROM_TYPE::CHR_ROM);
        for (int i = 0; i < 2; i++) s->ptr_4K_CHR_banks[i] = getBank(&s->CHR_4K_cache, CHR_4K[i], Mapper::ROM_TYPE::CHR_ROM);
    }
}

const MapperVTable Mapper001_vtable = 
{
    Mapper001_cpuRead,
    Mapper001_cpuWrite,
    Mapper001_ppuRead,
    Mapper001_ppuWrite,
    Mapper001_ppuReadPtr,
    mapperNoScanline,
    mapperNoCycle,
    Mapper001_reset,
    Mapper001_dumpState,
    Mapper001_loadState,
};

Mapper createMapper001(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart)
{
    Mapper mapper;
    mapper.vtable = &Mapper001_vtable; 
    Mapper001_state* state = new Mapper001_state;
    
    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;

    bankInit(&state->PRG_16K_cache, state->PRG_banks_16K, MAPPER001_NUM_PRG_BANKS_16K, 16*1024, cart);
    state->RAM = (uint8_t*)malloc(8*1024);

    if (CHR_banks == 0) 
    {
        // Allocate one shared 8 KB RAM
        state->CHR_RAM = (uint8_t*)malloc(8 * 1024);
        memset(state->CHR_RAM, 0, 8 * 1024);
    }
    else
    {
        bankInit(&state->CHR_8K_cache, state->CHR_banks_8K, MAPPER001_NUM_CHR_BANKS_8K, 8*1024, cart);
        bankInit(&state->CHR_4K_cache, state->CHR_banks_4K, MAPPER001_NUM_CHR_BANKS_4K, 4*1024, cart);
    }

    mapper.state = state;
    return mapper;
}
// ===== END src/core/mappers/mapper001.cpp =====

// ===== BEGIN src/core/mappers/mapper002.cpp =====

IRAM_ATTR bool Mapper002_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr < 0x8000) return false;

    Mapper002_state* state = (Mapper002_state*)mapper->state;
    uint16_t offset = addr & 0x3FFF;
    uint8_t bank_id = (addr >> 14) & 1;
    data = state->ptr_16K_PRG_banks[bank_id][offset];
    return true;
}

IRAM_ATTR bool Mapper002_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr < 0x8000) return false;

    Mapper002_state* state = (Mapper002_state*)mapper->state;
    uint8_t bank = data & 0x0F;
    state->ptr_16K_PRG_banks[0] = getBank(&state->prg_cache, bank, Mapper::ROM_TYPE::PRG_ROM);
    return true;
}

IRAM_ATTR bool Mapper002_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr > 0x1FFF) return false;

    Mapper002_state* state = (Mapper002_state*)mapper->state;
    data = state->CHR_bank[addr];
    return true;
}

IRAM_ATTR bool Mapper002_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr > 0x1FFF) return false;
    
    Mapper002_state* state = (Mapper002_state*)mapper->state;
    if (state->number_CHR_banks == 0)
    {
        // Treat as RAM
        state->CHR_bank[addr] = data;
        return true;
    }

	return false;
}

IRAM_ATTR uint8_t* Mapper002_ppuReadPtr(Mapper* mapper, uint16_t addr)
{
    if (addr > 0x1FFF) return nullptr;

    Mapper002_state* state = (Mapper002_state*)mapper->state;
    return &state->CHR_bank[addr];
}

void Mapper002_reset(Mapper* mapper)
{
    Mapper002_state* state = (Mapper002_state*)mapper->state;
    state->ptr_16K_PRG_banks[0] = getBank(&state->prg_cache, 0, Mapper::ROM_TYPE::PRG_ROM);
    state->ptr_16K_PRG_banks[1] = state->PRG_bank;

    state->cart->loadPRGBank(state->ptr_16K_PRG_banks[1], 16*1024, 0x4000 * (state->number_PRG_banks - 1));
    state->cart->loadCHRBank(state->CHR_bank, 8*1024, 0);
}

void Mapper002_dumpState(Mapper* mapper, File& state)
{
    Mapper002_state* s = (Mapper002_state*)mapper->state;

    uint8_t PRG_16K;
    PRG_16K = getBankIndex(&s->prg_cache, s->ptr_16K_PRG_banks[0]);
    state.write((uint8_t*)&PRG_16K, sizeof(PRG_16K));
    if (s->number_CHR_banks == 0)
    {
        state.write(s->CHR_bank, sizeof(s->CHR_bank));
    }
}

void Mapper002_loadState(Mapper* mapper, File& state)
{
    Mapper002_state* s = (Mapper002_state*)mapper->state;

    uint8_t PRG_16K;
    state.read((uint8_t*)&PRG_16K, sizeof(PRG_16K));
    invalidateCache(&s->prg_cache);
    s->ptr_16K_PRG_banks[0] = getBank(&s->prg_cache, PRG_16K, Mapper::ROM_TYPE::PRG_ROM);
    if (s->number_CHR_banks == 0)
    {
        state.read(s->CHR_bank, sizeof(s->CHR_bank));
    }
}

const MapperVTable Mapper002_vtable = 
{
    Mapper002_cpuRead,
    Mapper002_cpuWrite,
    Mapper002_ppuRead,
    Mapper002_ppuWrite,
    Mapper002_ppuReadPtr,
    mapperNoScanline,
    mapperNoCycle,
    Mapper002_reset,
    Mapper002_dumpState,
    Mapper002_loadState,
};

Mapper createMapper002(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart)
{
    Mapper mapper;
    mapper.vtable = &Mapper002_vtable; 
    Mapper002_state* state = new Mapper002_state;
    bankInit(&state->prg_cache, state->prg_banks, MAPPER002_NUM_PRG_BANKS_16K, 16*1024, cart);

    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;

    mapper.state = state;
    return mapper;
}
// ===== END src/core/mappers/mapper002.cpp =====

// ===== BEGIN src/core/mappers/mapper003.cpp =====

IRAM_ATTR bool Mapper003_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr < 0x8000) return false;

    Mapper003_state* state = (Mapper003_state*)mapper->state;
    data = state->PRG_bank[addr & 0x7FFF];
    return true;
}

IRAM_ATTR bool Mapper003_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr < 0x8000) return false;

    Mapper003_state* state = (Mapper003_state*)mapper->state;
    uint8_t bank = data & 0x03;
    state->ptr_CHR_bank_8K = getBank(&state->CHR_cache_8K, bank, Mapper::ROM_TYPE::CHR_ROM);
    return true;
}

IRAM_ATTR bool Mapper003_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr > 0x1FFF) return false;

    Mapper003_state* state = (Mapper003_state*)mapper->state;
    data = state->ptr_CHR_bank_8K[addr];
    return true;
}

IRAM_ATTR bool Mapper003_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
	return false;
}

IRAM_ATTR uint8_t* Mapper003_ppuReadPtr(Mapper* mapper, uint16_t addr)
{
    if (addr > 0x1FFF) return nullptr;

    Mapper003_state* state = (Mapper003_state*)mapper->state;
    return &state->ptr_CHR_bank_8K[addr];
}

void Mapper003_reset(Mapper* mapper)
{
    Mapper003_state* state = (Mapper003_state*)mapper->state;

    state->ptr_CHR_bank_8K = getBank(&state->CHR_cache_8K, 0, Mapper::ROM_TYPE::CHR_ROM);
    state->cart->loadPRGBank(state->PRG_bank, 32*1024, 0);
}

void Mapper003_dumpState(Mapper* mapper, File& state)
{
    Mapper003_state* s = (Mapper003_state*)mapper->state;

    uint8_t CHR_bank = getBankIndex(&s->CHR_cache_8K, s->ptr_CHR_bank_8K);
    state.write((uint8_t*)&CHR_bank, sizeof(CHR_bank));
}

void Mapper003_loadState(Mapper* mapper, File& state)
{
    Mapper003_state* s = (Mapper003_state*)mapper->state;

    uint8_t CHR_bank;
    state.read((uint8_t*)&CHR_bank, sizeof(CHR_bank));
    invalidateCache(&s->CHR_cache_8K);
    s->ptr_CHR_bank_8K = getBank(&s->CHR_cache_8K, CHR_bank, Mapper::ROM_TYPE::PRG_ROM);
}

const MapperVTable Mapper003_vtable = 
{
    Mapper003_cpuRead,
    Mapper003_cpuWrite,
    Mapper003_ppuRead,
    Mapper003_ppuWrite,
    Mapper003_ppuReadPtr,
    mapperNoScanline,
    mapperNoCycle,
    Mapper003_reset,
    Mapper003_dumpState,
    Mapper003_loadState,
};

Mapper createMapper003(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart)
{
    Mapper mapper;
    mapper.vtable = &Mapper003_vtable; 
    Mapper003_state* state = new Mapper003_state;
    bankInit(&state->CHR_cache_8K, state->CHR_banks_8K, MAPPER003_NUM_CHR_BANKS_8K, 8*1024, cart);

    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;

    mapper.state = state;
    return mapper;
}
// ===== END src/core/mappers/mapper003.cpp =====

// ===== BEGIN src/core/mappers/mapper004.cpp =====

struct Mapper004_state
{
    Cartridge* cart;
    uint8_t number_PRG_banks;
    uint8_t number_CHR_banks;
    uint8_t* RAM;

	uint8_t bank_register[8];
    uint8_t* ptr_PRG_bank_8K[4];
    uint8_t* ptr_CHR_bank_1K[8];

    Bank PRG_banks_8K[MAPPER004_NUM_PRG_BANKS_8K];
    Bank CHR_banks_1K[MAPPER004_NUM_CHR_BANKS_1K];
    BankCache PRG_cache_8K;
    BankCache CHR_cache_1K;

	uint8_t bank_select = 0x00; // Bank select register
	uint8_t IRQ_latch = 0x00; // IRQ latch register
	uint8_t IRQ_counter = 0x00;
	bool IRQ_enable = false; // IRQ enable/disable register

    uint8_t PRG_ROM_bank_mode = 0;
	uint8_t CHR_ROM_bank_mode = 0;
	uint16_t PRG_mask = 0;
	uint16_t CHR_mask = 0;

	static constexpr Cartridge::MIRROR mirror[2] = 
    {
        Cartridge::MIRROR::VERTICAL,
        Cartridge::MIRROR::HORIZONTAL
    };
};
constexpr Cartridge::MIRROR Mapper004_state::mirror[2];

IRAM_ATTR bool Mapper004_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
	if (addr < 0x6000) return false;

    Mapper004_state* state = (Mapper004_state*)mapper->state;
    if (addr < 0x8000) 
	{
		data = state->RAM[addr & 0x1FFF];
		return true;
	}   

    uint8_t bank = (addr >> 13) & 0x03;
    data = state->ptr_PRG_bank_8K[bank][addr & 0x1FFF];
    return true;
}

IRAM_ATTR bool Mapper004_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr < 0x6000) return false;

    Mapper004_state* state = (Mapper004_state*)mapper->state;
    if (addr < 0x8000)
	{
		state->RAM[addr & 0x1FFF] = data;
		return true;
	}

	// Bank select (even address) | Bank data (odd address)
	uint8_t bank_register[10];
	switch (addr & 0xE001)
	{
    case 0x8000:
		state->bank_select = data & 0x07;
		state->PRG_ROM_bank_mode = (data >> 6) & 0x01;
		state->CHR_ROM_bank_mode = (data >> 7) & 0x01;
		break;

	case 0x8001:
		state->bank_register[state->bank_select] = data;

		bank_register[0] = (state->bank_register[0] & 0xFE) & state->CHR_mask;
		bank_register[1] = (state->bank_register[1] & 0xFE) & state->CHR_mask;
		bank_register[2] = state->bank_register[2] & state->CHR_mask;
		bank_register[3] = state->bank_register[3] & state->CHR_mask;
		bank_register[4] = state->bank_register[4] & state->CHR_mask;
		bank_register[5] = state->bank_register[5] & state->CHR_mask;
		bank_register[6] = state->bank_register[6] & state->PRG_mask;
		bank_register[7] = state->bank_register[7] & state->PRG_mask;
		bank_register[8] = ((state->bank_register[0] & 0xFE) + 1) & state->CHR_mask;
		bank_register[9] = ((state->bank_register[1] & 0xFE) + 1) & state->CHR_mask;
		if (state->CHR_ROM_bank_mode)
		{
			state->ptr_CHR_bank_1K[0] = getBank(&state->CHR_cache_1K, bank_register[2], Mapper::ROM_TYPE::CHR_ROM);
			state->ptr_CHR_bank_1K[1] = getBank(&state->CHR_cache_1K, bank_register[3], Mapper::ROM_TYPE::CHR_ROM);
			state->ptr_CHR_bank_1K[2] = getBank(&state->CHR_cache_1K, bank_register[4], Mapper::ROM_TYPE::CHR_ROM);
			state->ptr_CHR_bank_1K[3] = getBank(&state->CHR_cache_1K, bank_register[5], Mapper::ROM_TYPE::CHR_ROM);
			state->ptr_CHR_bank_1K[4] = getBank(&state->CHR_cache_1K, bank_register[0], Mapper::ROM_TYPE::CHR_ROM);
			state->ptr_CHR_bank_1K[5] = getBank(&state->CHR_cache_1K, bank_register[8], Mapper::ROM_TYPE::CHR_ROM);
			state->ptr_CHR_bank_1K[6] = getBank(&state->CHR_cache_1K, bank_register[1], Mapper::ROM_TYPE::CHR_ROM);
			state->ptr_CHR_bank_1K[7] = getBank(&state->CHR_cache_1K, bank_register[9], Mapper::ROM_TYPE::CHR_ROM);
		}
		else
		{
			state->ptr_CHR_bank_1K[0] = getBank(&state->CHR_cache_1K, bank_register[0], Mapper::ROM_TYPE::CHR_ROM);
			state->ptr_CHR_bank_1K[1] = getBank(&state->CHR_cache_1K, bank_register[8], Mapper::ROM_TYPE::CHR_ROM);
			state->ptr_CHR_bank_1K[2] = getBank(&state->CHR_cache_1K, bank_register[1], Mapper::ROM_TYPE::CHR_ROM);
			state->ptr_CHR_bank_1K[3] = getBank(&state->CHR_cache_1K, bank_register[9], Mapper::ROM_TYPE::CHR_ROM);
			state->ptr_CHR_bank_1K[4] = getBank(&state->CHR_cache_1K, bank_register[2], Mapper::ROM_TYPE::CHR_ROM);
			state->ptr_CHR_bank_1K[5] = getBank(&state->CHR_cache_1K, bank_register[3], Mapper::ROM_TYPE::CHR_ROM);
			state->ptr_CHR_bank_1K[6] = getBank(&state->CHR_cache_1K, bank_register[4], Mapper::ROM_TYPE::CHR_ROM);
			state->ptr_CHR_bank_1K[7] = getBank(&state->CHR_cache_1K, bank_register[5], Mapper::ROM_TYPE::CHR_ROM);
		}

		if (state->PRG_ROM_bank_mode)
		{
			state->ptr_PRG_bank_8K[0] = getBank(&state->PRG_cache_8K, (state->number_PRG_banks * 2) - 2, Mapper::ROM_TYPE::PRG_ROM);
			state->ptr_PRG_bank_8K[2] = getBank(&state->PRG_cache_8K, bank_register[6], Mapper::ROM_TYPE::PRG_ROM);
		}
		else
		{
			state->ptr_PRG_bank_8K[0] = getBank(&state->PRG_cache_8K, bank_register[6], Mapper::ROM_TYPE::PRG_ROM);
			state->ptr_PRG_bank_8K[2] = getBank(&state->PRG_cache_8K, (state->number_PRG_banks * 2) - 2, Mapper::ROM_TYPE::PRG_ROM);
		}
		state->ptr_PRG_bank_8K[1] = getBank(&state->PRG_cache_8K, bank_register[7], Mapper::ROM_TYPE::PRG_ROM);
		state->ptr_PRG_bank_8K[3] = getBank(&state->PRG_cache_8K, (state->number_PRG_banks * 2) - 1, Mapper::ROM_TYPE::PRG_ROM);
		break;

	// Mirroring (even address)
	case 0xA000:
        state->cart->setMirrorMode(state->mirror[data & 0x01]);
		break;

	// IRQ latch (even address) | IRQ reload (odd address)
	case 0xC000:
		state->IRQ_latch = data;
		break;
	
	case 0xC001:
		state->IRQ_counter = 0;
		break;

	// IRQ disable (even address) | IRQ enable (odd address)
	case 0xE000:
		state->IRQ_enable = false;
		break;

	case 0xE001:
		state->IRQ_enable = true;
		break;
	}

	return false;
}

IRAM_ATTR bool Mapper004_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr > 0x1FFF) return false;

    Mapper004_state* state = (Mapper004_state*)mapper->state;
    uint8_t bank = (addr >> 10) & 0x07;
	data = state->ptr_CHR_bank_1K[bank][addr & 0x03FF];
    return true;
}

IRAM_ATTR bool Mapper004_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
	return false;
}

IRAM_ATTR uint8_t* Mapper004_ppuReadPtr(Mapper* mapper, uint16_t addr)
{
	if (addr > 0x1FFF) return nullptr;

    Mapper004_state* state = (Mapper004_state*)mapper->state;
    uint8_t bank = (addr >> 10) & 0x07;
	return &state->ptr_CHR_bank_1K[bank][addr & 0x03FF];
}

IRAM_ATTR void Mapper004_scanline(Mapper* mapper)
{
    Mapper004_state* state = (Mapper004_state*)mapper->state;
	if (state->IRQ_counter == 0)
		state->IRQ_counter = state->IRQ_latch;
	else 
    {
        state->IRQ_counter--;
        if ((state->IRQ_counter == 0) && state->IRQ_enable) 
		    state->cart->IRQ();
    }
}

void Mapper004_reset(Mapper* mapper)
{
    Mapper004_state* state = (Mapper004_state*)mapper->state;
    memset(state->RAM, 0, 8 * 1024);
	state->ptr_PRG_bank_8K[0] = getBank(&state->PRG_cache_8K, 0, Mapper::ROM_TYPE::PRG_ROM);
	state->ptr_PRG_bank_8K[1] = getBank(&state->PRG_cache_8K, 0, Mapper::ROM_TYPE::PRG_ROM);
	state->ptr_PRG_bank_8K[2] = getBank(&state->PRG_cache_8K, (state->number_PRG_banks * 2) - 2, Mapper::ROM_TYPE::PRG_ROM);
	state->ptr_PRG_bank_8K[3] = getBank(&state->PRG_cache_8K, (state->number_PRG_banks * 2) - 1, Mapper::ROM_TYPE::PRG_ROM);

	state->ptr_CHR_bank_1K[0] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
	state->ptr_CHR_bank_1K[1] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
	state->ptr_CHR_bank_1K[2] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
	state->ptr_CHR_bank_1K[3] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
	state->ptr_CHR_bank_1K[4] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
	state->ptr_CHR_bank_1K[5] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
	state->ptr_CHR_bank_1K[6] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
	state->ptr_CHR_bank_1K[7] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);

	state->bank_select = 0x00;
	state->IRQ_latch = 0x00;
	state->IRQ_counter = 0x00;
	state->IRQ_enable = false;
    state->PRG_ROM_bank_mode = 0;
	state->CHR_ROM_bank_mode = 0;
	state->PRG_mask = (state->number_PRG_banks * 2) - 1;
	state->CHR_mask = (state->number_CHR_banks * 8) - 1;
    state->cart->setMirrorMode(Cartridge::MIRROR::HORIZONTAL);
}

void Mapper004_dumpState(Mapper* mapper, File& state)
{
    Mapper004_state* s = (Mapper004_state*)mapper->state;
	state.write(s->bank_register, sizeof(s->bank_register));
	state.write((uint8_t*)&s->bank_select, sizeof(s->bank_select));
	state.write((uint8_t*)&s->IRQ_latch, sizeof(s->IRQ_latch));
	state.write((uint8_t*)&s->IRQ_counter, sizeof(s->IRQ_counter));
	state.write((uint8_t*)&s->IRQ_enable, sizeof(s->IRQ_enable));
	state.write((uint8_t*)&s->PRG_ROM_bank_mode, sizeof(s->PRG_ROM_bank_mode));
	state.write((uint8_t*)&s->CHR_ROM_bank_mode, sizeof(s->CHR_ROM_bank_mode));

	Cartridge::MIRROR mirror = s->cart->getMirrorMode();
	state.write((uint8_t*)&mirror, sizeof(mirror));

	uint8_t PRG_bank_8K[4];
	uint8_t CHR_bank_1K[8];
	for (int i = 0; i < 4; i++) PRG_bank_8K[i] = getBankIndex(&s->PRG_cache_8K, s->ptr_PRG_bank_8K[i]);
	for (int i = 0; i < 8; i++) CHR_bank_1K[i] = getBankIndex(&s->CHR_cache_1K, s->ptr_CHR_bank_1K[i]);
	state.write(PRG_bank_8K, sizeof(PRG_bank_8K));
	state.write(CHR_bank_1K, sizeof(CHR_bank_1K));
	state.write(s->RAM, 8*1024);
}

void Mapper004_loadState(Mapper* mapper, File& state)
{
	Mapper004_state* s = (Mapper004_state*)mapper->state;
	state.read(s->bank_register, sizeof(s->bank_register));
	state.read((uint8_t*)&s->bank_select, sizeof(s->bank_select));
	state.read((uint8_t*)&s->IRQ_latch, sizeof(s->IRQ_latch));
	state.read((uint8_t*)&s->IRQ_counter, sizeof(s->IRQ_counter));
	state.read((uint8_t*)&s->IRQ_enable, sizeof(s->IRQ_enable));
	state.read((uint8_t*)&s->PRG_ROM_bank_mode, sizeof(s->PRG_ROM_bank_mode));
	state.read((uint8_t*)&s->CHR_ROM_bank_mode, sizeof(s->CHR_ROM_bank_mode));

	Cartridge::MIRROR mirror;
	state.read((uint8_t*)&mirror, sizeof(mirror));
	s->cart->setMirrorMode(mirror);

	uint8_t PRG_bank_8K[4];
	uint8_t CHR_bank_1K[8];
	state.read(PRG_bank_8K, sizeof(PRG_bank_8K));
	state.read(CHR_bank_1K, sizeof(CHR_bank_1K));

	invalidateCache(&s->PRG_cache_8K);
	invalidateCache(&s->CHR_cache_1K);
	for (int i = 0; i < 4; i++) s->ptr_PRG_bank_8K[i] = getBank(&s->PRG_cache_8K, PRG_bank_8K[i], Mapper::ROM_TYPE::PRG_ROM);
	for (int i = 0; i < 8; i++) s->ptr_CHR_bank_1K[i] = getBank(&s->CHR_cache_1K, CHR_bank_1K[i], Mapper::ROM_TYPE::CHR_ROM);

	state.read(s->RAM, 8*1024);
}	

const MapperVTable Mapper004_vtable = 
{
    Mapper004_cpuRead,
    Mapper004_cpuWrite,
    Mapper004_ppuRead,
    Mapper004_ppuWrite,
    Mapper004_ppuReadPtr,
	Mapper004_scanline,
    mapperNoCycle,
	Mapper004_reset,
	Mapper004_dumpState,
	Mapper004_loadState,
};

Mapper createMapper004(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart)
{
    Mapper mapper;
    mapper.vtable = &Mapper004_vtable; 
    Mapper004_state* state = new Mapper004_state;
    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;

    bankInit(&state->PRG_cache_8K, state->PRG_banks_8K, MAPPER004_NUM_PRG_BANKS_8K, 8*1024, cart);
    bankInit(&state->CHR_cache_1K, state->CHR_banks_1K, MAPPER004_NUM_CHR_BANKS_1K, 1*1024, cart);
    state->RAM = (uint8_t*)malloc(8*1024);

    mapper.state = state;
    return mapper;
}
// ===== END src/core/mappers/mapper004.cpp =====

// ===== BEGIN src/core/mappers/mapper069.cpp =====

struct Mapper069_state
{
    Cartridge* cart;
    uint8_t number_PRG_banks;
    uint8_t number_CHR_banks;
    uint8_t* RAM;

    uint8_t* ptr_PRG_bank_8K[5];
    uint8_t* ptr_CHR_bank_1K[8];

    Bank PRG_banks_8K[MAPPER069_NUM_PRG_BANKS_8K];
    Bank CHR_banks_1K[MAPPER069_NUM_CHR_BANKS_1K];
    BankCache PRG_cache_8K;
    BankCache CHR_cache_1K;

    uint8_t command_register = 0x00;
    uint8_t parameter_register = 0x00;

	uint16_t IRQ_counter = 0x0000;
    bool IRQ_counter_enable = false;
	bool IRQ_enable = false;
    bool PRG_RAM_select = false;
    bool PRG_RAM_enable = false;
    uint16_t PRG_mask = 0;
	uint16_t CHR_mask = 0;

	static constexpr Cartridge::MIRROR mirror[4] = 
    {
        Cartridge::MIRROR::VERTICAL,
        Cartridge::MIRROR::HORIZONTAL,
        Cartridge::MIRROR::ONESCREEN_LOW,
        Cartridge::MIRROR::ONESCREEN_HIGH
    };
};
constexpr Cartridge::MIRROR Mapper069_state::mirror[4];

IRAM_ATTR bool Mapper069_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
	if (addr < 0x6000) return false;

    Mapper069_state* state = (Mapper069_state*)mapper->state;
    if (addr < 0x8000) 
	{
        // PRG RAM
        if (state->PRG_RAM_select)
        {
            if (state->PRG_RAM_enable) data = state->RAM[addr & 0x1FFF];
            return true;
        }

        // PRG ROM
        data = state->ptr_PRG_bank_8K[0][addr & 0x1FFF];
        return true;
	}   

    uint8_t bank = (addr >> 13) & 0x03;
    data = state->ptr_PRG_bank_8K[bank + 1][addr & 0x1FFF];
    return true;
}

IRAM_ATTR bool Mapper069_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr < 0x6000) return false;

    Mapper069_state* state = (Mapper069_state*)mapper->state;
    if (addr < 0x8000)
	{
        if (state->PRG_RAM_select && state->PRG_RAM_enable)
		    state->RAM[addr & 0x1FFF] = data;
		return true;
	}

	// Command Register ($8000-$9FFF) | Parameter Register ($A000-$BFFF)
    uint16_t masked_addr = addr & 0xE000;
	if (masked_addr == 0x8000) 
        state->command_register = data & 0x0F;
    else if (masked_addr == 0xA000)
    {
        uint8_t command = state->command_register;
        switch (command)
        {
            case 0x00: case 0x01: case 0x02: case 0x03:
            case 0x04: case 0x05: case 0x06: case 0x07:
                state->ptr_CHR_bank_1K[command] = getBank(&state->CHR_cache_1K, data & state->CHR_mask, Mapper::ROM_TYPE::CHR_ROM);
                break;

            case 0x08:
                state->ptr_PRG_bank_8K[command & 0x03] = getBank(&state->PRG_cache_8K, (data & 0x3F) & state->PRG_mask, Mapper::ROM_TYPE::PRG_ROM);
                state->PRG_RAM_select = (data & 0x40) != 0;
                state->PRG_RAM_enable = (data & 0x80) != 0;
                break;

            case 0x09: case 0x0A: case 0x0B:
                state->ptr_PRG_bank_8K[command & 0x03] = getBank(&state->PRG_cache_8K, (data & 0x3F) & state->PRG_mask, Mapper::ROM_TYPE::PRG_ROM);
                break;

            case 0x0C:
                state->cart->setMirrorMode(state->mirror[data & 0x03]);
                break;

            case 0x0D:
                state->IRQ_enable = (data & 0x01) != 0;
                state->IRQ_counter_enable = (data & 0x80) != 0;
                break;
            case 0x0E:
                state->IRQ_counter = (state->IRQ_counter & 0xFF00) | data;
                break;
            case 0x0F:
                state->IRQ_counter = (state->IRQ_counter & 0x00FF) | (data << 8);
                break;
        }
    }
    return false;
}

IRAM_ATTR bool Mapper069_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr > 0x1FFF) return false;

    Mapper069_state* state = (Mapper069_state*)mapper->state;
    uint8_t bank = (addr >> 10) & 0x07;
	data = state->ptr_CHR_bank_1K[bank][addr & 0x03FF];
    return true;
}

IRAM_ATTR bool Mapper069_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
	return false;
}

IRAM_ATTR uint8_t* Mapper069_ppuReadPtr(Mapper* mapper, uint16_t addr)
{
	if (addr > 0x1FFF) return nullptr;

    Mapper069_state* state = (Mapper069_state*)mapper->state;
    uint8_t bank = (addr >> 10) & 0x07;
	return &state->ptr_CHR_bank_1K[bank][addr & 0x03FF];
}

IRAM_ATTR void Mapper069_cycle(Mapper* mapper, int cycles)
{
    Mapper069_state* state = (Mapper069_state*)mapper->state;
    if (!state->IRQ_counter_enable) return;

    // IRQ if IRQ counter underflows from 0x0000 -> 0xFFFF;
    uint16_t before = state->IRQ_counter;
    state->IRQ_counter -= cycles;
    if ((before < cycles) && state->IRQ_enable) 
        state->cart->IRQ();
}

void Mapper069_reset(Mapper* mapper)
{
    Mapper069_state* state = (Mapper069_state*)mapper->state;
    memset(state->RAM, 0, 8 * 1024);
	state->ptr_PRG_bank_8K[0] = getBank(&state->PRG_cache_8K, 0, Mapper::ROM_TYPE::PRG_ROM);
	state->ptr_PRG_bank_8K[1] = getBank(&state->PRG_cache_8K, 0, Mapper::ROM_TYPE::PRG_ROM);
	state->ptr_PRG_bank_8K[2] = getBank(&state->PRG_cache_8K, 0, Mapper::ROM_TYPE::PRG_ROM);
	state->ptr_PRG_bank_8K[3] = getBank(&state->PRG_cache_8K, 0, Mapper::ROM_TYPE::PRG_ROM);
    state->cart->loadPRGBank(state->ptr_PRG_bank_8K[4], 8*1024, ((state->number_PRG_banks * 2) - 1) * 8*1024);

	state->ptr_CHR_bank_1K[0] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
	state->ptr_CHR_bank_1K[1] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
	state->ptr_CHR_bank_1K[2] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
	state->ptr_CHR_bank_1K[3] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
	state->ptr_CHR_bank_1K[4] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
	state->ptr_CHR_bank_1K[5] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
	state->ptr_CHR_bank_1K[6] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
	state->ptr_CHR_bank_1K[7] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);

	state->command_register = 0x00;
    state->parameter_register = 0x00;
	state->IRQ_counter = 0x0000;
    state->IRQ_counter_enable = false;
	state->IRQ_enable = false;
    state->PRG_RAM_select = false;
    state->PRG_RAM_enable = false;
	state->PRG_mask = (state->number_PRG_banks * 2) - 1;
	state->CHR_mask = (state->number_CHR_banks * 8) - 1;
    state->cart->setMirrorMode(Cartridge::MIRROR::HORIZONTAL);
}

void Mapper069_dumpState(Mapper* mapper, File& state)
{
    Mapper069_state* s = (Mapper069_state*)mapper->state;
    state.write((uint8_t*)&s->command_register, sizeof(s->command_register));
    state.write((uint8_t*)&s->parameter_register, sizeof(s->parameter_register));
	state.write((uint8_t*)&s->IRQ_counter, sizeof(s->IRQ_counter));
    state.write((uint8_t*)&s->IRQ_counter_enable, sizeof(s->IRQ_counter_enable));
	state.write((uint8_t*)&s->IRQ_enable, sizeof(s->IRQ_enable));
    state.write((uint8_t*)&s->PRG_RAM_select, sizeof(s->PRG_RAM_select));
    state.write((uint8_t*)&s->PRG_RAM_enable, sizeof(s->PRG_RAM_enable));

	Cartridge::MIRROR mirror = s->cart->getMirrorMode();
	state.write((uint8_t*)&mirror, sizeof(mirror));

	uint8_t PRG_bank_8K[4];
	uint8_t CHR_bank_1K[8];
	for (int i = 0; i < 4; i++) PRG_bank_8K[i] = getBankIndex(&s->PRG_cache_8K, s->ptr_PRG_bank_8K[i]);
	for (int i = 0; i < 8; i++) CHR_bank_1K[i] = getBankIndex(&s->CHR_cache_1K, s->ptr_CHR_bank_1K[i]);
	state.write(PRG_bank_8K, sizeof(PRG_bank_8K));
	state.write(CHR_bank_1K, sizeof(CHR_bank_1K));
	state.write(s->RAM, 8*1024);
}

void Mapper069_loadState(Mapper* mapper, File& state)
{
	Mapper069_state* s = (Mapper069_state*)mapper->state;
    state.read((uint8_t*)&s->command_register, sizeof(s->command_register));
    state.read((uint8_t*)&s->parameter_register, sizeof(s->parameter_register));
	state.read((uint8_t*)&s->IRQ_counter, sizeof(s->IRQ_counter));
    state.read((uint8_t*)&s->IRQ_counter_enable, sizeof(s->IRQ_counter_enable));
	state.read((uint8_t*)&s->IRQ_enable, sizeof(s->IRQ_enable));
    state.read((uint8_t*)&s->PRG_RAM_select, sizeof(s->PRG_RAM_select));
    state.read((uint8_t*)&s->PRG_RAM_enable, sizeof(s->PRG_RAM_enable));

	Cartridge::MIRROR mirror;
	state.read((uint8_t*)&mirror, sizeof(mirror));
	s->cart->setMirrorMode(mirror);

	uint8_t PRG_bank_8K[4];
	uint8_t CHR_bank_1K[8];
	state.read(PRG_bank_8K, sizeof(PRG_bank_8K));
	state.read(CHR_bank_1K, sizeof(CHR_bank_1K));

	invalidateCache(&s->PRG_cache_8K);
	invalidateCache(&s->CHR_cache_1K);
	for (int i = 0; i < 4; i++) s->ptr_PRG_bank_8K[i] = getBank(&s->PRG_cache_8K, PRG_bank_8K[i], Mapper::ROM_TYPE::PRG_ROM);
	for (int i = 0; i < 8; i++) s->ptr_CHR_bank_1K[i] = getBank(&s->CHR_cache_1K, CHR_bank_1K[i], Mapper::ROM_TYPE::CHR_ROM);

	state.read(s->RAM, 8*1024);
}	

const MapperVTable Mapper069_vtable = 
{
    Mapper069_cpuRead,
    Mapper069_cpuWrite,
    Mapper069_ppuRead,
    Mapper069_ppuWrite,
    Mapper069_ppuReadPtr,
    mapperNoScanline,    
    Mapper069_cycle,
	Mapper069_reset,
	Mapper069_dumpState,
	Mapper069_loadState,
};

Mapper createMapper069(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart)
{
    Mapper mapper;
    mapper.vtable = &Mapper069_vtable; 
    Mapper069_state* state = new Mapper069_state;
    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;

    bankInit(&state->PRG_cache_8K, state->PRG_banks_8K, MAPPER069_NUM_PRG_BANKS_8K, 8*1024, cart);
    bankInit(&state->CHR_cache_1K, state->CHR_banks_1K, MAPPER069_NUM_CHR_BANKS_1K, 1*1024, cart);
    state->RAM = (uint8_t*)malloc(8*1024);
    state->ptr_PRG_bank_8K[4] = (uint8_t*)malloc(8*1024);

    mapper.state = state;
    return mapper;
}
// ===== END src/core/mappers/mapper069.cpp =====

// ===== BEGIN src/core/cartridge.cpp =====

Cartridge::Cartridge(const char* filename)
{
    struct cartridge_header
    {
        char name[4];
        uint8_t PRG_ROM_chunks;
        uint8_t CHR_ROM_chunks;
        uint8_t mapper1;
        uint8_t mapper2;
        uint8_t PRG_RAM_size;
        uint8_t tv_system;
        uint8_t tv_system2;
        char unused[5];
    } header;

    // Load Cartridge ROM
    rom = SD.open(filename, FILE_READ);
    if (!rom) return;

    rom.read((uint8_t*)&header, sizeof(cartridge_header));
    if (header.mapper1 & 0x04) rom.seek(rom.position() + 512);

	mapper_ID = (header.mapper2 & 0xF0) | header.mapper1 >> 4;
    hardware_mirror = (header.mapper1 & 0x01) ? VERTICAL : HORIZONTAL;

    // Check file format
    uint8_t file_type = 1;
    if ((header.mapper2 & 0x0C) == 0x08);
    switch (file_type)
    {
    case 1:
        number_PRG_banks = header.PRG_ROM_chunks;
        number_CHR_banks = header.CHR_ROM_chunks;
        break;
    case 2:
        number_PRG_banks = ((header.PRG_RAM_size & 0x07) << 8) | header.PRG_ROM_chunks;
        number_CHR_banks = ((header.PRG_RAM_size & 0x38) << 8) | header.CHR_ROM_chunks;
        break;
    }

    prg_base = sizeof(cartridge_header) + (header.mapper1 & 0x04 ? 512 : 0);
    chr_base = prg_base + (number_PRG_banks * 16384);
    switch (mapper_ID)
    {
        case 0: mapper = createMapper000(number_PRG_banks, number_CHR_banks, this); break;
        case 1: mapper = createMapper001(number_PRG_banks, number_CHR_banks, this); break;
        case 2: mapper = createMapper002(number_PRG_banks, number_CHR_banks, this); break;
        case 3: mapper = createMapper003(number_PRG_banks, number_CHR_banks, this); break;
        case 4: mapper = createMapper004(number_PRG_banks, number_CHR_banks, this); break;
        case 69: mapper = createMapper069(number_PRG_banks, number_CHR_banks, this); break;
    }

    // Calculate ROM CRC32
    uint8_t buf[2048];
    size_t len;
    rom.seek(prg_base);
    while ((len = rom.read(buf, sizeof(buf))) > 0)
    {
        CRC32 = crc32(buf, len, CRC32);
    }
    CRC32 ^= ~0U;
    Serial.printf("CRC32: %08X\n", CRC32);
}

Cartridge::~Cartridge()
{

}


IRAM_ATTR bool Cartridge::cpuRead(uint16_t addr, uint8_t& data)
{
	return mapper.vtable->cpuRead(&mapper, addr, data);
}

IRAM_ATTR bool Cartridge::cpuWrite(uint16_t addr, uint8_t data)
{
	return mapper.vtable->cpuWrite(&mapper, addr, data);
}

IRAM_ATTR bool Cartridge::ppuRead(uint16_t addr, uint8_t& data)
{
	return mapper.vtable->ppuRead(&mapper, addr, data);
}

IRAM_ATTR bool Cartridge::ppuWrite(uint16_t addr, uint8_t data)
{
	return mapper.vtable->ppuWrite(&mapper, addr, data);	
}

IRAM_ATTR uint8_t* Cartridge::ppuReadPtr(uint16_t addr)
{
	return mapper.vtable->ppuReadPtr(&mapper, addr);
}

void Cartridge::ppuScanline()
{
    mapper.vtable->scanline(&mapper);
}

void Cartridge::cpuCycle(int cycles)
{
    mapper.vtable->cycle(&mapper, cycles);
}

void Cartridge::reset()
{
    mapper.vtable->reset(&mapper);
}

IRAM_ATTR void Cartridge::loadPRGBank(uint8_t* bank, uint16_t size, uint32_t offset)
{
    rom.seek(prg_base + offset);
    rom.read(bank, size);
}

IRAM_ATTR void Cartridge::loadCHRBank(uint8_t* bank, uint16_t size, uint32_t offset)
{
    rom.seek(chr_base + offset);
    rom.read(bank, size);
}

IRAM_ATTR void Cartridge::setMirrorMode(MIRROR mirror)
{
    bus->setPPUMirrorMode(mirror);
}

Cartridge::MIRROR Cartridge::getMirrorMode()
{
    return bus->getPPUMirrorMode();
}

void Cartridge::IRQ()
{
    bus->IRQ();
}

void Cartridge::dumpState(File& state)
{
    mapper.vtable->dumpState(&mapper, state);
}

void Cartridge::loadState(File& state)
{
    mapper.vtable->loadState(&mapper, state);
}

uint32_t Cartridge::crc32(const void* buf, size_t size, uint32_t seed)
{
    static constexpr uint32_t crc32_table[256] = 
    {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };

    const uint8_t* p = (const uint8_t*)buf;
    uint32_t crc;

    crc = seed;
    while (size--)
        crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    return crc;
}
// ===== END src/core/cartridge.cpp =====

// ===== BEGIN src/core/apu2A03.cpp =====

constexpr uint8_t Apu2A03::duty_sequences[4][8];
constexpr uint8_t Apu2A03::length_counter_lookup[32];
constexpr uint8_t Apu2A03::triangle_sequence[32];
constexpr uint16_t Apu2A03::noise_period_lookup[16];
constexpr uint16_t Apu2A03::DMC_rate_lookup[16];

static constexpr uint32_t APU_SAMPLE_RATE = 44100;

Apu2A03::Apu2A03()
{
}

Apu2A03::~Apu2A03()
{

}

void Apu2A03::reset()
{
	pulse1_enable = false;
	pulse2_enable = false;
	triangle_enable = false;
	noise_enable = false;
	DMC_enable = false;
	IRQ = false;

	pulse1.len_counter.timer = 0;
	pulse2.len_counter.timer = 0;
	triangle.len_counter.timer = 0;
	noise.len_counter.timer = 0;

	DMC.output_unit.output_level = 0;
	DMC.output_unit.remaining_bits = 0;
	DMC.output_unit.shift_register = 0;
	DMC.memory_reader.address = 0;
	DMC.memory_reader.remaining_bytes = 0;
	DMC.timer = 0;
	DMC.sample_address = 0;
	DMC.sample_buffer = 0;
	DMC.sample_length = 0;
	DMC.output_unit.silence_flag = true;
}


IRAM_ATTR void Apu2A03::cpuWrite(uint16_t addr, uint8_t data)
{
    switch (addr)
	{
	case 0x4000:
		pulse1.seq.duty_cycle = ((data & 0xC0) >> 6);

		pulse1.env.loop = ((data >> 5) & 0x01);
		pulse1.len_counter.halt = ((data >> 5) & 0x01);
		pulse1.env.constant_volume = ((data >> 4) & 0x01);
		pulse1.env.volume = (data & 0x0F);
		break;

	case 0x4001:
		pulse1.sweep.enable = (data >> 7);
		pulse1.sweep.reload = ((data >> 4) & 0x07) + 1;
		pulse1.sweep.negate = ((data >> 3) & 0x01);
		pulse1.sweep.shift_count = data & 0x07;
		pulse1.sweep.reload_flag = true;
		break;

	case 0x4002:
		pulse1.seq.reload = (pulse1.seq.reload & 0xFF00) | data;
		break;

	case 0x4003:
		pulse1.seq.cycle_position = 0;
		pulse1.seq.reload = (pulse1.seq.reload & 0x00FF) | (uint16_t)((data & 0x07) << 8);
		pulse1.seq.timer = pulse1.seq.reload;
		pulse1.env.start_flag = true;	

		if (pulse1_enable) pulse1.len_counter.timer = length_counter_lookup[data >> 3] + 1;

		// Restart envelope
		pulse1.env.timer = pulse1.env.volume;
		pulse1.env.decay_level_counter = 15;
		break;

	case 0x4004:
		pulse2.seq.duty_cycle = ((data & 0xC0) >> 6);

		pulse2.env.loop = ((data >> 5) & 0x01);
		pulse2.len_counter.halt = pulse2.env.loop;
		pulse2.env.constant_volume = ((data >> 4) & 0x01);
		pulse2.env.volume = (data & 0x0F);
		break;

	case 0x4005:
		pulse2.sweep.enable = (data >> 7);
		pulse2.sweep.reload = ((data >> 4) & 0x07) + 1;
		pulse2.sweep.negate = ((data >> 3) & 0x01);
		pulse2.sweep.shift_count = data & 0x07;
		pulse2.sweep.reload_flag = true;
		break;

	case 0x4006:
		pulse2.seq.reload = (pulse2.seq.reload & 0xFF00) | data;
		break;

	case 0x4007:
		pulse2.seq.cycle_position = 0;
		pulse2.seq.reload = (pulse2.seq.reload & 0x00FF) | (uint16_t)((data & 0x07) << 8);
		pulse2.seq.timer = pulse2.seq.reload;
		pulse2.env.start_flag = true;

		if (pulse2_enable) pulse2.len_counter.timer = length_counter_lookup[data >> 3] + 1;

		// Restart envelope
		pulse2.env.timer = pulse2.env.volume;
		pulse2.env.decay_level_counter = 15;
		break;

	case 0x4008:
		triangle.lin_counter.reload = data & 0x7F;
		triangle.len_counter.halt = data >> 7;
		triangle.lin_counter.control = data >> 7;
		break;

	case 0x400A:
		triangle.seq.reload = (triangle.seq.reload & 0xFF00) | data;
		break;

	case 0x400B:
		triangle.seq.reload = ((triangle.seq.reload & 0x00FF) | (uint16_t)((data & 0x07)) << 8) + 1;
		triangle.seq.timer = triangle.seq.reload;

		if (triangle_enable) triangle.len_counter.timer = length_counter_lookup[data >> 3] + 1;
		triangle.lin_counter.reload_flag = true;
		break;

	case 0x400C:
		noise.len_counter.halt = (data >> 5) & 0x01;
		noise.env.constant_volume = (data >> 4) & 0x01;
		noise.env.volume = data & 0x0F;
		break;

	case 0x400E:
		noise.mode = data >> 7;
		noise.reload = noise_period_lookup[data & 0x0F] / 2;
		break;

	case 0x400F:
		noise.env.start_flag = true;

		if (noise_enable) noise.len_counter.timer = length_counter_lookup[data >> 3] + 1;
		break;

	case 0x4010:
		DMC.IRQ_flag = data >> 7;
		DMC.loop_flag = (data & 0x40) == 0x40;
		DMC.reload = (DMC_rate_lookup[data & 0x0F] / 2) - 1;
		DMC.timer = DMC.reload;
		break;

	case 0x4011:
		DMC.output_unit.output_level = data & 0x7F;
		break;

	case 0x4012:
		DMC.sample_address = 0xC000 | ((uint32_t)data << 6);
		DMC.memory_reader.address = DMC.sample_address;
		break;

	case 0x4013:
		DMC.sample_length = (data << 4) | 0x0001;
		DMC.memory_reader.remaining_bytes = DMC.sample_length;
		break;

	case 0x4015:
		IRQ = false;
		// Pulse 1 enable
		if (data & 0x01)
		{
			pulse1_enable = true;
		}
		else
		{
			pulse1_enable = false;
			pulse1.len_counter.timer = 0;
		}

		// Pulse 2 enable
		if ((data >> 1) & 0x01)
		{
			pulse2_enable = true;
		}
		else
		{
			pulse2_enable = false;
			pulse2.len_counter.timer = 0;
		}

		// Triangle enable
		if ((data >> 2) & 0x01)
		{
			triangle_enable = true;
		}
		else
		{
			triangle_enable = false;
			triangle.len_counter.timer = 0;
		}

		// Noise enable
		if ((data >> 3) & 0x01)
		{
			noise_enable = true;
		}
		else
		{
			noise_enable = false;
			noise.len_counter.timer = 0;
		}

		// DMC enable
		if ((data >> 4) & 0x01)
		{
			DMC_enable = true;
			if (DMC.sample_buffer_empty)
			{
				setDMCBuffer();
				cpu->cycles += 3;
			}
		}
		else
		{
			DMC_enable = false;
		}
		break;

	case 0x4017:
		four_step_sequence_mode = ((data >> 7) == 0) ? true : false;

		if (((data >> 6) & 0x01) == 1)
		{
			IRQ = false; 
			interrupt_inhibit = true;
		}
		else interrupt_inhibit = false;
		break;
	}
}

IRAM_ATTR uint8_t Apu2A03::cpuRead(uint16_t addr)
{
    uint8_t data = 0x00;
	if (addr == 0x4015)
	{
		IRQ = false;
	}
	return data;
}

void Apu2A03::setVolume(uint8_t vol)
{
	volume = vol;
}

IRAM_ATTR void Apu2A03::clock()
{
    // Clock all sound channels
    pulseChannelClock(pulse1.seq, pulse1_enable);
    pulseChannelClock(pulse2.seq, pulse2_enable);
    noiseChannelClock(noise, noise_enable);
    DMCChannelClock(DMC, DMC_enable);
	triangleChannelClock(triangle, triangle_enable);

    switch (clock_counter)
    {
    case 3728:
		soundChannelEnvelopeClock(pulse1.env);
		soundChannelEnvelopeClock(pulse2.env);
		soundChannelEnvelopeClock(noise.env);
		linearCounterClock(triangle.lin_counter);
        break;

    case 7456:
		soundChannelEnvelopeClock(pulse1.env);
		soundChannelEnvelopeClock(pulse2.env);
		soundChannelEnvelopeClock(noise.env);
		linearCounterClock(triangle.lin_counter);
		
		soundChannelSweeperClock(pulse1);
		soundChannelLengthCounterClock(pulse1.len_counter);

		soundChannelSweeperClock(pulse2);
		soundChannelLengthCounterClock(pulse2.len_counter);

		soundChannelLengthCounterClock(triangle.len_counter);
		soundChannelLengthCounterClock(noise.len_counter);
        break;

    case 11185:
		soundChannelEnvelopeClock(pulse1.env);
		soundChannelEnvelopeClock(pulse2.env);
		soundChannelEnvelopeClock(noise.env);
		linearCounterClock(triangle.lin_counter);
        break;

    case 14914:
        if (four_step_sequence_mode)
        {
            if (!interrupt_inhibit) IRQ = true;
			soundChannelEnvelopeClock(pulse1.env);
			soundChannelEnvelopeClock(pulse2.env);
			soundChannelEnvelopeClock(noise.env);
			linearCounterClock(triangle.lin_counter);
			
			soundChannelSweeperClock(pulse1);
			soundChannelLengthCounterClock(pulse1.len_counter);

			soundChannelSweeperClock(pulse2);
			soundChannelLengthCounterClock(pulse2.len_counter);

			soundChannelLengthCounterClock(triangle.len_counter);
			soundChannelLengthCounterClock(noise.len_counter);
            clock_counter = 0;
        }
        break;

    case 18640:
        if (!four_step_sequence_mode)
        {
			soundChannelEnvelopeClock(pulse1.env);
			soundChannelEnvelopeClock(pulse2.env);
			soundChannelEnvelopeClock(noise.env);
			linearCounterClock(triangle.lin_counter);
			
			soundChannelSweeperClock(pulse1);
			soundChannelLengthCounterClock(pulse1.len_counter);

			soundChannelSweeperClock(pulse2);
			soundChannelLengthCounterClock(pulse2.len_counter);

			soundChannelLengthCounterClock(triangle.len_counter);
			soundChannelLengthCounterClock(noise.len_counter);
            clock_counter = 0;
        }
        break;
    }

	// Put sound channels output into audio buffers
	// Generate sample every 20.29221088 clocks
	// (1.789773 MHz / 2) / 44100 Hz
	pulse_hz += APU_SAMPLE_RATE;
	if (pulse_hz > 894886)
	{
		// Mute sound channels if muted
		if (pulse1.sweep.mute || pulse1.seq.reload < 8 || pulse1.len_counter.timer == 0)
		{
			pulse1.seq.output = 0;
			pulse1.env.output = 0;
		}
		if (pulse2.sweep.mute || pulse2.seq.reload < 8 || pulse2.len_counter.timer == 0)
		{
			pulse2.seq.output = 0;
			pulse2.env.output = 0;
		}
		// Silencing the triangle channel when triangle.seq.reload < 2 is considered less accurate emulation,
		// but eliminates high frequencies and popping
		// if (!triangle_enable || triangle.len_counter.timer == 0 || triangle.seq.reload < 2) 
		// {
		// 	triangle.seq.output = 0;
		// 	triangle.env.output = 0;
		// }
		generateSample();
		pulse_hz -= 894886;
	}
	clock_counter++;
}

inline void Apu2A03::generateSample()
{
	uint16_t sample = 0;
	sample += pulse1.seq.output ? pulse1.env.output : 0;
	sample += pulse2.seq.output ? pulse2.env.output: 0;
	sample += triangle.seq.output;
	sample += DMC.output_unit.output_level;

	if (!(noise.shift_register & 0x01) && noise.len_counter.timer > 0)
		sample += noise.env.output;
		
	// Clip audio and apply a low-pass filter
	uint32_t temp = sample * volume;
	sample = (uint16_t)(((temp + 50) / 100));
	sample += prev_sample;
	sample >>= 1;
	sample &= 0xFF;
	prev_sample = sample;
}

inline void Apu2A03::pulseChannelClock(sequencerUnit& seq, bool enable)
{
	if (!enable) return;

	seq.timer--;
	if (seq.timer == 0xFFFF)
	{
		seq.timer = seq.reload;
		// Shift duty cycle with wrapping
		seq.output = duty_sequences[seq.duty_cycle][seq.cycle_position];
		seq.cycle_position = (seq.cycle_position + 1) & 7;
	}
}

inline void Apu2A03::triangleChannelClock(triangleChannel& triangle, bool enable)
{
	if (!enable) return; // Temp

	for (int i = 0; i < 2; i++)
	{
		triangle.seq.timer--;
		if (triangle.seq.timer == 0)
		{
			triangle.seq.timer = triangle.seq.reload;
			if (!(triangle.len_counter.timer > 0 && triangle.lin_counter.counter > 0))
				return;

			if (triangle.seq.reload >= 2)
			{
				triangle.seq.output = triangle_sequence[triangle.seq.duty_cycle];
				triangle.seq.duty_cycle = (triangle.seq.duty_cycle + 1) & 31;
			}
		}
	}
}

inline void Apu2A03::noiseChannelClock(noiseChannel& noise, bool enable)
{   
	if (!enable) return; // Temp

	noise.timer--;
	if (noise.timer == 0xFFFF)
	{
		noise.timer = noise.reload;
		uint8_t temp = noise.mode ? (noise.shift_register >> 6) & 0x01 : (noise.shift_register >> 1) & 0x01;
		noise.output = (noise.shift_register & 0x01) ^ (temp);
		noise.shift_register >>= 1;
		noise.shift_register |= noise.output << 14;
	}
}

inline void Apu2A03::DMCChannelClock(DMCChannel& DMC, bool enable)
{
	if (!enable) return;

	DMC.timer--;
	if (DMC.timer == 0xFFFF)
	{
		DMC.timer = DMC.reload + 1;
		if (DMC.output_unit.silence_flag == false)
		{
			if (DMC.output_unit.shift_register & 0x01)
			{
				if (DMC.output_unit.output_level <= 125)
					DMC.output_unit.output_level += 2;
			}
			else
			{
				if (DMC.output_unit.output_level >= 2)
					DMC.output_unit.output_level -= 2;
			}

			DMC.output_unit.shift_register >>= 1;
		}

		// Update Bits remaining counter
		DMC.output_unit.remaining_bits--;
		if (DMC.output_unit.remaining_bits <= 0)
		{
			DMC.output_unit.remaining_bits = 8;

			if (DMC.sample_buffer_empty)
			{
				DMC.output_unit.silence_flag = true;
			}
			else
			{
				DMC.output_unit.silence_flag = false;
				DMC.output_unit.shift_register = DMC.sample_buffer;
				DMC.sample_buffer_empty = true;
				setDMCBuffer();
				cpu->cycles += 4;
			}
		}
	}
}

inline void Apu2A03::soundChannelEnvelopeClock(envelopeUnit& envelope)
{
	if (envelope.start_flag)
	{
		envelope.start_flag = false;
		envelope.decay_level_counter = 15;
		envelope.timer = envelope.volume + 1;
	}
	else
	{
		envelope.timer--;
		if (envelope.timer == 0)
		{
			envelope.timer = envelope.volume + 1;
			if (envelope.decay_level_counter > 0) envelope.decay_level_counter--;
			else if (envelope.loop) envelope.decay_level_counter = 15;
		}
	}

	if (envelope.constant_volume) envelope.output = envelope.volume;
	else envelope.output = envelope.decay_level_counter;
}

inline void Apu2A03::soundChannelSweeperClock(pulseChannel& channel)
{
	// Calculate the target period
	channel.sweep.change = (channel.seq.reload >> channel.sweep.shift_count);
	// Negate change if negate flag is true
	// Pulse 1 adds one's complement = -c - 1
	// Pulse 2 adds two's complement = -c
	if (channel.sweep.negate && channel.sweep.pulse_channel_number == 1)
		channel.sweep.change = -channel.sweep.change - 1;
	else if (channel.sweep.negate && channel.sweep.pulse_channel_number == 2)
		channel.sweep.change = -channel.sweep.change;

	channel.sweep.target_period = channel.seq.reload + channel.sweep.change;
	if (channel.sweep.target_period < 0)
		channel.sweep.target_period = 0;

	// Check if channel should be muted
	if (channel.seq.reload < 8) channel.sweep.mute = true;
	else if (channel.sweep.target_period > 0x7FF) channel.sweep.mute = true;
	else channel.sweep.mute = false;

	channel.sweep.timer--;
	if (channel.sweep.enable && channel.sweep.timer == 0 && channel.sweep.shift_count != 0)
	{
		if (!channel.sweep.mute)
			channel.seq.reload = channel.sweep.target_period;
	}
	if (channel.sweep.timer == 0 || channel.sweep.reload_flag)
	{
		channel.sweep.timer = channel.sweep.reload;
		channel.sweep.reload_flag = false;
	}
}

inline void Apu2A03::soundChannelLengthCounterClock(length_counter& len_counter)
{
	if (!len_counter.halt && len_counter.timer > 0)
		len_counter.timer--;
}

inline void Apu2A03::linearCounterClock(linear_counter& lin_counter)
{
	if (lin_counter.reload_flag)
		lin_counter.counter = lin_counter.reload;
	else if (lin_counter.counter > 0)
			lin_counter.counter--;

	if (lin_counter.control == 0)
		lin_counter.reload_flag = false;
}

inline void Apu2A03::setDMCBuffer()
{
	uint8_t value = bus->cpuRead(DMC.memory_reader.address);
	if (DMC.memory_reader.remaining_bytes <= 0) return;

    DMC.sample_buffer = value;
    DMC.sample_buffer_empty = false;

    DMC.memory_reader.address++;
    if (DMC.memory_reader.address == 0x0000)
        DMC.memory_reader.address = 0x8000;

    DMC.memory_reader.remaining_bytes--;
    if (DMC.memory_reader.remaining_bytes == 0)
    {
        if (DMC.loop_flag)
        {
            // Restart sample
            DMC.memory_reader.address = DMC.sample_address;
            DMC.memory_reader.remaining_bytes = DMC.sample_length;
        }
        else if (DMC.IRQ_flag) IRQ = true;
    }
}
// ===== END src/core/apu2A03.cpp =====

// ===== BEGIN src/core/cpu6502.cpp =====

constexpr uint8_t Cpu6502::instr_cycles[256] = {
    7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
    2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
    2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7
};

constexpr uint8_t Cpu6502::zn_table[256] = {
    #define ENTRY(v) (((v) == 0 ? Cpu6502::Z : 0) | ((v) & Cpu6502::N))
    ENTRY(0x00), ENTRY(0x01), ENTRY(0x02), ENTRY(0x03), ENTRY(0x04), ENTRY(0x05), ENTRY(0x06), ENTRY(0x07), ENTRY(0x08), ENTRY(0x09), ENTRY(0x0A), ENTRY(0x0B), ENTRY(0x0C), ENTRY(0x0D), ENTRY(0x0E), ENTRY(0x0F),
    ENTRY(0x10), ENTRY(0x11), ENTRY(0x12), ENTRY(0x13), ENTRY(0x14), ENTRY(0x15), ENTRY(0x16), ENTRY(0x17), ENTRY(0x18), ENTRY(0x19), ENTRY(0x1A), ENTRY(0x1B), ENTRY(0x1C), ENTRY(0x1D), ENTRY(0x1E), ENTRY(0x1F),
    ENTRY(0x20), ENTRY(0x21), ENTRY(0x22), ENTRY(0x23), ENTRY(0x24), ENTRY(0x25), ENTRY(0x26), ENTRY(0x27), ENTRY(0x28), ENTRY(0x29), ENTRY(0x2A), ENTRY(0x2B), ENTRY(0x2C), ENTRY(0x2D), ENTRY(0x2E), ENTRY(0x2F),
    ENTRY(0x30), ENTRY(0x31), ENTRY(0x32), ENTRY(0x33), ENTRY(0x34), ENTRY(0x35), ENTRY(0x36), ENTRY(0x37), ENTRY(0x38), ENTRY(0x39), ENTRY(0x3A), ENTRY(0x3B), ENTRY(0x3C), ENTRY(0x3D), ENTRY(0x3E), ENTRY(0x3F),
    ENTRY(0x40), ENTRY(0x41), ENTRY(0x42), ENTRY(0x43), ENTRY(0x44), ENTRY(0x45), ENTRY(0x46), ENTRY(0x47), ENTRY(0x48), ENTRY(0x49), ENTRY(0x4A), ENTRY(0x4B), ENTRY(0x4C), ENTRY(0x4D), ENTRY(0x4E), ENTRY(0x4F),
    ENTRY(0x50), ENTRY(0x51), ENTRY(0x52), ENTRY(0x53), ENTRY(0x54), ENTRY(0x55), ENTRY(0x56), ENTRY(0x57), ENTRY(0x58), ENTRY(0x59), ENTRY(0x5A), ENTRY(0x5B), ENTRY(0x5C), ENTRY(0x5D), ENTRY(0x5E), ENTRY(0x5F),
    ENTRY(0x60), ENTRY(0x61), ENTRY(0x62), ENTRY(0x63), ENTRY(0x64), ENTRY(0x65), ENTRY(0x66), ENTRY(0x67), ENTRY(0x68), ENTRY(0x69), ENTRY(0x6A), ENTRY(0x6B), ENTRY(0x6C), ENTRY(0x6D), ENTRY(0x6E), ENTRY(0x6F),
    ENTRY(0x70), ENTRY(0x71), ENTRY(0x72), ENTRY(0x73), ENTRY(0x74), ENTRY(0x75), ENTRY(0x76), ENTRY(0x77), ENTRY(0x78), ENTRY(0x79), ENTRY(0x7A), ENTRY(0x7B), ENTRY(0x7C), ENTRY(0x7D), ENTRY(0x7E), ENTRY(0x7F),
    ENTRY(0x80), ENTRY(0x81), ENTRY(0x82), ENTRY(0x83), ENTRY(0x84), ENTRY(0x85), ENTRY(0x86), ENTRY(0x87), ENTRY(0x88), ENTRY(0x89), ENTRY(0x8A), ENTRY(0x8B), ENTRY(0x8C), ENTRY(0x8D), ENTRY(0x8E), ENTRY(0x8F),
    ENTRY(0x90), ENTRY(0x91), ENTRY(0x92), ENTRY(0x93), ENTRY(0x94), ENTRY(0x95), ENTRY(0x96), ENTRY(0x97), ENTRY(0x98), ENTRY(0x99), ENTRY(0x9A), ENTRY(0x9B), ENTRY(0x9C), ENTRY(0x9D), ENTRY(0x9E), ENTRY(0x9F),
    ENTRY(0xA0), ENTRY(0xA1), ENTRY(0xA2), ENTRY(0xA3), ENTRY(0xA4), ENTRY(0xA5), ENTRY(0xA6), ENTRY(0xA7), ENTRY(0xA8), ENTRY(0xA9), ENTRY(0xAA), ENTRY(0xAB), ENTRY(0xAC), ENTRY(0xAD), ENTRY(0xAE), ENTRY(0xAF),
    ENTRY(0xB0), ENTRY(0xB1), ENTRY(0xB2), ENTRY(0xB3), ENTRY(0xB4), ENTRY(0xB5), ENTRY(0xB6), ENTRY(0xB7), ENTRY(0xB8), ENTRY(0xB9), ENTRY(0xBA), ENTRY(0xBB), ENTRY(0xBC), ENTRY(0xBD), ENTRY(0xBE), ENTRY(0xBF),
    ENTRY(0xC0), ENTRY(0xC1), ENTRY(0xC2), ENTRY(0xC3), ENTRY(0xC4), ENTRY(0xC5), ENTRY(0xC6), ENTRY(0xC7), ENTRY(0xC8), ENTRY(0xC9), ENTRY(0xCA), ENTRY(0xCB), ENTRY(0xCC), ENTRY(0xCD), ENTRY(0xCE), ENTRY(0xCF),
    ENTRY(0xD0), ENTRY(0xD1), ENTRY(0xD2), ENTRY(0xD3), ENTRY(0xD4), ENTRY(0xD5), ENTRY(0xD6), ENTRY(0xD7), ENTRY(0xD8), ENTRY(0xD9), ENTRY(0xDA), ENTRY(0xDB), ENTRY(0xDC), ENTRY(0xDD), ENTRY(0xDE), ENTRY(0xDF),
    ENTRY(0xE0), ENTRY(0xE1), ENTRY(0xE2), ENTRY(0xE3), ENTRY(0xE4), ENTRY(0xE5), ENTRY(0xE6), ENTRY(0xE7), ENTRY(0xE8), ENTRY(0xE9), ENTRY(0xEA), ENTRY(0xEB), ENTRY(0xEC), ENTRY(0xED), ENTRY(0xEE), ENTRY(0xEF),
    ENTRY(0xF0), ENTRY(0xF1), ENTRY(0xF2), ENTRY(0xF3), ENTRY(0xF4), ENTRY(0xF5), ENTRY(0xF6), ENTRY(0xF7), ENTRY(0xF8), ENTRY(0xF9), ENTRY(0xFA), ENTRY(0xFB), ENTRY(0xFC), ENTRY(0xFD), ENTRY(0xFE), ENTRY(0xFF)
    #undef ENTRY
};

#define EXECUTE(addrmode, instruction) { additional_cycle1 = addrmode(); additional_cycle2 = instruction(); }

Cpu6502::Cpu6502()
{
    apu.connectCPU(this);
}

Cpu6502::~Cpu6502()
{
    
}   

inline uint8_t Cpu6502::read(uint16_t addr)
{
	return bus->cpuRead(addr);
}

inline void Cpu6502::write(uint16_t addr, uint8_t data)
{
	bus->cpuWrite(addr, data);
}

IRAM_ATTR void Cpu6502::OAM_DMA(uint8_t page)
{
    OAM_DMA_page = page << 8;
    for (int i = 0; i < 256; i++)
    {
        OAM_Write(i, read((OAM_DMA_page) | i));
    }

    cycles += 512;
}

IRAM_ATTR void Cpu6502::OAM_Write(uint8_t addr, uint8_t data)
{
    bus->OAM_Write(addr, data);
}

IRAM_ATTR void Cpu6502::clock(int i)
{
    for (int remaining_cycles = i; remaining_cycles > 0; remaining_cycles--)
    {
        if (cycles > 0) { cycles--; cart->cpuCycle(1); continue; }

        opcode = read(PC++);
        cycles = instr_cycles[opcode];
        additional_cycle1 = 0;
        additional_cycle2 = 0;
        switch (opcode)
        {
            case 0x00: EXECUTE(IMM, Instr_BRK); break;
            case 0x01: EXECUTE(IDX, Instr_ORA); break;
            case 0x02: EXECUTE(IMP, Instr_XXX); break;
            case 0x03: EXECUTE(IMP, Instr_XXX); break;
            case 0x04: EXECUTE(IMP, Instr_NOP); break;
            case 0x05: EXECUTE(ZPG, Instr_ORA); break;
            case 0x06: EXECUTE(ZPG, Instr_ASL); break;
            case 0x07: EXECUTE(IMP, Instr_XXX); break;
            case 0x08: EXECUTE(IMP, Instr_PHP); break;
            case 0x09: EXECUTE(IMM, Instr_ORA); break;
            case 0x0A: EXECUTE(IMP, Instr_ASL); break;
            case 0x0B: EXECUTE(IMP, Instr_XXX); break;
            case 0x0C: EXECUTE(IMP, Instr_NOP); break;
            case 0x0D: EXECUTE(ABS, Instr_ORA); break;
            case 0x0E: EXECUTE(ABS, Instr_ASL); break;
            case 0x0F: EXECUTE(IMP, Instr_XXX); break;

            case 0x10: EXECUTE(REL, Instr_BPL); break;
            case 0x11: EXECUTE(IDY, Instr_ORA); break;
            case 0x12: EXECUTE(IMP, Instr_XXX); break;
            case 0x13: EXECUTE(IMP, Instr_XXX); break;
            case 0x14: EXECUTE(IMP, Instr_NOP); break;
            case 0x15: EXECUTE(ZPX, Instr_ORA); break;
            case 0x16: EXECUTE(ZPX, Instr_ASL); break;
            case 0x17: EXECUTE(IMP, Instr_XXX); break;
            case 0x18: EXECUTE(IMP, Instr_CLC); break;
            case 0x19: EXECUTE(ABY, Instr_ORA); break;
            case 0x1A: EXECUTE(IMP, Instr_NOP); break;
            case 0x1B: EXECUTE(IMP, Instr_XXX); break;
            case 0x1C: EXECUTE(IMP, Instr_NOP); break;
            case 0x1D: EXECUTE(ABX, Instr_ORA); break;
            case 0x1E: EXECUTE(ABX, Instr_ASL); break;
            case 0x1F: EXECUTE(IMP, Instr_XXX); break;

            case 0x20: EXECUTE(ABS, Instr_JSR); break;
            case 0x21: EXECUTE(IDX, Instr_AND); break;
            case 0x22: EXECUTE(IMP, Instr_XXX); break;
            case 0x23: EXECUTE(IMP, Instr_XXX); break;
            case 0x24: EXECUTE(ZPG, Instr_BIT); break;
            case 0x25: EXECUTE(ZPG, Instr_AND); break;
            case 0x26: EXECUTE(ZPG, Instr_ROL); break;
            case 0x27: EXECUTE(IMP, Instr_XXX); break;
            case 0x28: EXECUTE(IMP, Instr_PLP); break;
            case 0x29: EXECUTE(IMM, Instr_AND); break;
            case 0x2A: EXECUTE(IMP, Instr_ROL); break;
            case 0x2B: EXECUTE(IMP, Instr_XXX); break;
            case 0x2C: EXECUTE(ABS, Instr_BIT); break;
            case 0x2D: EXECUTE(ABS, Instr_AND); break;
            case 0x2E: EXECUTE(ABS, Instr_ROL); break;
            case 0x2F: EXECUTE(IMP, Instr_XXX); break;

            case 0x30: EXECUTE(REL, Instr_BMI); break;
            case 0x31: EXECUTE(IDY, Instr_AND); break;
            case 0x32: EXECUTE(IMP, Instr_XXX); break;
            case 0x33: EXECUTE(IMP, Instr_XXX); break;
            case 0x34: EXECUTE(IMP, Instr_NOP); break;
            case 0x35: EXECUTE(ZPX, Instr_AND); break;
            case 0x36: EXECUTE(ZPX, Instr_ROL); break;
            case 0x37: EXECUTE(IMP, Instr_XXX); break;
            case 0x38: EXECUTE(IMP, Instr_SEC); break;
            case 0x39: EXECUTE(ABY, Instr_AND); break;
            case 0x3A: EXECUTE(IMP, Instr_NOP); break;
            case 0x3B: EXECUTE(IMP, Instr_XXX); break;
            case 0x3C: EXECUTE(IMP, Instr_NOP); break;
            case 0x3D: EXECUTE(ABX, Instr_AND); break;
            case 0x3E: EXECUTE(ABX, Instr_ROL); break;
            case 0x3F: EXECUTE(IMP, Instr_XXX); break;

            case 0x40: EXECUTE(IMP, Instr_RTI); break;
            case 0x41: EXECUTE(IDX, Instr_EOR); break;
            case 0x42: EXECUTE(IMP, Instr_XXX); break;
            case 0x43: EXECUTE(IMP, Instr_XXX); break;
            case 0x44: EXECUTE(IMP, Instr_NOP); break;
            case 0x45: EXECUTE(ZPG, Instr_EOR); break;
            case 0x46: EXECUTE(ZPG, Instr_LSR); break;
            case 0x47: EXECUTE(IMP, Instr_XXX); break;
            case 0x48: EXECUTE(IMP, Instr_PHA); break;
            case 0x49: EXECUTE(IMM, Instr_EOR); break;
            case 0x4A: EXECUTE(IMP, Instr_LSR); break;
            case 0x4B: EXECUTE(IMP, Instr_XXX); break;
            case 0x4C: EXECUTE(ABS, Instr_JMP); break;
            case 0x4D: EXECUTE(ABS, Instr_EOR); break;
            case 0x4E: EXECUTE(ABS, Instr_LSR); break;
            case 0x4F: EXECUTE(IMP, Instr_XXX); break;

            case 0x50: EXECUTE(REL, Instr_BVC); break;
            case 0x51: EXECUTE(IDY, Instr_EOR); break;
            case 0x52: EXECUTE(IMP, Instr_XXX); break;
            case 0x53: EXECUTE(IMP, Instr_XXX); break;
            case 0x54: EXECUTE(IMP, Instr_NOP); break;
            case 0x55: EXECUTE(ZPX, Instr_EOR); break;
            case 0x56: EXECUTE(ZPX, Instr_LSR); break;
            case 0x57: EXECUTE(IMP, Instr_XXX); break;
            case 0x58: EXECUTE(IMP, Instr_CLI); break;
            case 0x59: EXECUTE(ABY, Instr_EOR); break;
            case 0x5A: EXECUTE(IMP, Instr_NOP); break;
            case 0x5B: EXECUTE(IMP, Instr_XXX); break;
            case 0x5C: EXECUTE(IMP, Instr_NOP); break;
            case 0x5D: EXECUTE(ABX, Instr_EOR); break;
            case 0x5E: EXECUTE(ABX, Instr_LSR); break;
            case 0x5F: EXECUTE(IMP, Instr_XXX); break;

            case 0x60: EXECUTE(IMP, Instr_RTS); break;
            case 0x61: EXECUTE(IDX, Instr_ADC); break;
            case 0x62: EXECUTE(IMP, Instr_XXX); break;
            case 0x63: EXECUTE(IMP, Instr_XXX); break;
            case 0x64: EXECUTE(IMP, Instr_NOP); break;
            case 0x65: EXECUTE(ZPG, Instr_ADC); break;
            case 0x66: EXECUTE(ZPG, Instr_ROR); break;
            case 0x67: EXECUTE(IMP, Instr_XXX); break;
            case 0x68: EXECUTE(IMP, Instr_PLA); break;
            case 0x69: EXECUTE(IMM, Instr_ADC); break;
            case 0x6A: EXECUTE(IMP, Instr_ROR); break;
            case 0x6B: EXECUTE(IMP, Instr_XXX); break;
            case 0x6C: EXECUTE(IND, Instr_JMP); break;
            case 0x6D: EXECUTE(ABS, Instr_ADC); break;
            case 0x6E: EXECUTE(ABS, Instr_ROR); break;
            case 0x6F: EXECUTE(IMP, Instr_XXX); break;

            case 0x70: EXECUTE(REL, Instr_BVS); break;
            case 0x71: EXECUTE(IDY, Instr_ADC); break;
            case 0x72: EXECUTE(IMP, Instr_XXX); break;
            case 0x73: EXECUTE(IMP, Instr_XXX); break;
            case 0x74: EXECUTE(IMP, Instr_NOP); break;
            case 0x75: EXECUTE(ZPX, Instr_ADC); break;
            case 0x76: EXECUTE(ZPX, Instr_ROR); break;
            case 0x77: EXECUTE(IMP, Instr_XXX); break;
            case 0x78: EXECUTE(IMP, Instr_SEI); break;
            case 0x79: EXECUTE(ABY, Instr_ADC); break;
            case 0x7A: EXECUTE(IMP, Instr_NOP); break;
            case 0x7B: EXECUTE(IMP, Instr_XXX); break;
            case 0x7C: EXECUTE(IMP, Instr_NOP); break;
            case 0x7D: EXECUTE(ABX, Instr_ADC); break;
            case 0x7E: EXECUTE(ABX, Instr_ROR); break;
            case 0x7F: EXECUTE(IMP, Instr_XXX); break;

            case 0x80: EXECUTE(IMP, Instr_NOP); break;
            case 0x81: EXECUTE(IDX, Instr_STA); break;
            case 0x82: EXECUTE(IMP, Instr_NOP); break;
            case 0x83: EXECUTE(IMP, Instr_XXX); break;
            case 0x84: EXECUTE(ZPG, Instr_STY); break;
            case 0x85: EXECUTE(ZPG, Instr_STA); break;
            case 0x86: EXECUTE(ZPG, Instr_STX); break;
            case 0x87: EXECUTE(IMP, Instr_XXX); break;
            case 0x88: EXECUTE(IMP, Instr_DEY); break;
            case 0x89: EXECUTE(IMP, Instr_NOP); break;
            case 0x8A: EXECUTE(IMP, Instr_TXA); break;
            case 0x8B: EXECUTE(IMP, Instr_XXX); break;
            case 0x8C: EXECUTE(ABS, Instr_STY); break;
            case 0x8D: EXECUTE(ABS, Instr_STA); break;
            case 0x8E: EXECUTE(ABS, Instr_STX); break;
            case 0x8F: EXECUTE(IMP, Instr_XXX); break;

            case 0x90: EXECUTE(REL, Instr_BCC); break;
            case 0x91: EXECUTE(IDY, Instr_STA); break;
            case 0x92: EXECUTE(IMP, Instr_XXX); break;
            case 0x93: EXECUTE(IMP, Instr_XXX); break;
            case 0x94: EXECUTE(ZPX, Instr_STY); break;
            case 0x95: EXECUTE(ZPX, Instr_STA); break;
            case 0x96: EXECUTE(ZPY, Instr_STX); break;
            case 0x97: EXECUTE(IMP, Instr_XXX); break;
            case 0x98: EXECUTE(IMP, Instr_TYA); break;
            case 0x99: EXECUTE(ABY, Instr_STA); break;
            case 0x9A: EXECUTE(IMP, Instr_TXS); break;
            case 0x9B: EXECUTE(IMP, Instr_XXX); break;
            case 0x9C: EXECUTE(IMP, Instr_XXX); break;
            case 0x9D: EXECUTE(ABX, Instr_STA); break;
            case 0x9E: EXECUTE(IMP, Instr_XXX); break;
            case 0x9F: EXECUTE(IMP, Instr_XXX); break;

            case 0xA0: EXECUTE(IMM, Instr_LDY); break;
            case 0xA1: EXECUTE(IDX, Instr_LDA); break;
            case 0xA2: EXECUTE(IMM, Instr_LDX); break;
            case 0xA3: EXECUTE(IMP, Instr_XXX); break;
            case 0xA4: EXECUTE(ZPG, Instr_LDY); break;
            case 0xA5: EXECUTE(ZPG, Instr_LDA); break;
            case 0xA6: EXECUTE(ZPG, Instr_LDX); break;
            case 0xA7: EXECUTE(IMP, Instr_XXX); break;
            case 0xA8: EXECUTE(IMP, Instr_TAY); break;
            case 0xA9: EXECUTE(IMM, Instr_LDA); break;
            case 0xAA: EXECUTE(IMP, Instr_TAX); break;
            case 0xAB: EXECUTE(IMP, Instr_XXX); break;
            case 0xAC: EXECUTE(ABS, Instr_LDY); break;
            case 0xAD: EXECUTE(ABS, Instr_LDA); break;
            case 0xAE: EXECUTE(ABS, Instr_LDX); break;
            case 0xAF: EXECUTE(IMP, Instr_XXX); break;

            case 0xB0: EXECUTE(REL, Instr_BCS); break;
            case 0xB1: EXECUTE(IDY, Instr_LDA); break;
            case 0xB2: EXECUTE(IMP, Instr_XXX); break;
            case 0xB3: EXECUTE(IMP, Instr_XXX); break;
            case 0xB4: EXECUTE(ZPX, Instr_LDY); break;
            case 0xB5: EXECUTE(ZPX, Instr_LDA); break;
            case 0xB6: EXECUTE(ZPY, Instr_LDX); break;
            case 0xB7: EXECUTE(IMP, Instr_XXX); break;
            case 0xB8: EXECUTE(IMP, Instr_CLV); break;
            case 0xB9: EXECUTE(ABY, Instr_LDA); break;
            case 0xBA: EXECUTE(IMP, Instr_TSX); break;
            case 0xBB: EXECUTE(IMP, Instr_XXX); break;
            case 0xBC: EXECUTE(ABX, Instr_LDY); break;
            case 0xBD: EXECUTE(ABX, Instr_LDA); break;
            case 0xBE: EXECUTE(ABY, Instr_LDX); break;
            case 0xBF: EXECUTE(IMP, Instr_XXX); break;

            case 0xC0: EXECUTE(IMM, Instr_CPY); break;
            case 0xC1: EXECUTE(IDX, Instr_CMP); break;
            case 0xC2: EXECUTE(IMP, Instr_NOP); break;
            case 0xC3: EXECUTE(IMP, Instr_XXX); break;
            case 0xC4: EXECUTE(ZPG, Instr_CPY); break;
            case 0xC5: EXECUTE(ZPG, Instr_CMP); break;
            case 0xC6: EXECUTE(ZPG, Instr_DEC); break;
            case 0xC7: EXECUTE(IMP, Instr_XXX); break;
            case 0xC8: EXECUTE(IMP, Instr_INY); break;
            case 0xC9: EXECUTE(IMM, Instr_CMP); break;
            case 0xCA: EXECUTE(IMP, Instr_DEX); break;
            case 0xCB: EXECUTE(IMP, Instr_XXX); break;
            case 0xCC: EXECUTE(ABS, Instr_CPY); break;
            case 0xCD: EXECUTE(ABS, Instr_CMP); break;
            case 0xCE: EXECUTE(ABS, Instr_DEC); break;
            case 0xCF: EXECUTE(IMP, Instr_XXX); break;

            case 0xD0: EXECUTE(REL, Instr_BNE); break;
            case 0xD1: EXECUTE(IDY, Instr_CMP); break;
            case 0xD2: EXECUTE(IMP, Instr_XXX); break;
            case 0xD3: EXECUTE(IMP, Instr_XXX); break;
            case 0xD4: EXECUTE(IMP, Instr_NOP); break;
            case 0xD5: EXECUTE(ZPX, Instr_CMP); break;
            case 0xD6: EXECUTE(ZPX, Instr_DEC); break;
            case 0xD7: EXECUTE(IMP, Instr_XXX); break;
            case 0xD8: EXECUTE(IMP, Instr_CLD); break;
            case 0xD9: EXECUTE(ABY, Instr_CMP); break;
            case 0xDA: EXECUTE(IMP, Instr_NOP); break;
            case 0xDB: EXECUTE(IMP, Instr_XXX); break;
            case 0xDC: EXECUTE(IMP, Instr_NOP); break;
            case 0xDD: EXECUTE(ABX, Instr_CMP); break;
            case 0xDE: EXECUTE(ABX, Instr_DEC); break;
            case 0xDF: EXECUTE(IMP, Instr_XXX); break;

            case 0xE0: EXECUTE(IMM, Instr_CPX); break;
            case 0xE1: EXECUTE(IDX, Instr_SBC); break;
            case 0xE2: EXECUTE(IMP, Instr_NOP); break;
            case 0xE3: EXECUTE(IMP, Instr_XXX); break;
            case 0xE4: EXECUTE(ZPG, Instr_CPX); break;
            case 0xE5: EXECUTE(ZPG, Instr_SBC); break;
            case 0xE6: EXECUTE(ZPG, Instr_INC); break;
            case 0xE7: EXECUTE(IMP, Instr_XXX); break;
            case 0xE8: EXECUTE(IMP, Instr_INX); break;
            case 0xE9: EXECUTE(IMM, Instr_SBC); break;
            case 0xEA: EXECUTE(IMP, Instr_NOP); break;
            case 0xEB: EXECUTE(IMP, Instr_XXX); break;
            case 0xEC: EXECUTE(ABS, Instr_CPX); break;
            case 0xED: EXECUTE(ABS, Instr_SBC); break;
            case 0xEE: EXECUTE(ABS, Instr_INC); break;
            case 0xEF: EXECUTE(IMP, Instr_XXX); break;

            case 0xF0: EXECUTE(REL, Instr_BEQ); break;
            case 0xF1: EXECUTE(IDY, Instr_SBC); break;
            case 0xF2: EXECUTE(IMP, Instr_XXX); break;
            case 0xF3: EXECUTE(IMP, Instr_XXX); break;
            case 0xF4: EXECUTE(IMP, Instr_NOP); break;
            case 0xF5: EXECUTE(ZPX, Instr_SBC); break;
            case 0xF6: EXECUTE(ZPX, Instr_INC); break;
            case 0xF7: EXECUTE(IMP, Instr_XXX); break;
            case 0xF8: EXECUTE(IMP, Instr_SED); break;
            case 0xF9: EXECUTE(ABY, Instr_SBC); break;
            case 0xFA: EXECUTE(IMP, Instr_NOP); break;
            case 0xFB: EXECUTE(IMP, Instr_XXX); break;
            case 0xFC: EXECUTE(IMP, Instr_NOP); break;
            case 0xFD: EXECUTE(ABX, Instr_SBC); break;
            case 0xFE: EXECUTE(ABX, Instr_INC); break;
            case 0xFF: EXECUTE(IMP, Instr_XXX); break;
        }

        cycles += (additional_cycle1 & additional_cycle2);
        addrmode_implied = false;

        if (remaining_cycles >= cycles)
        {
            remaining_cycles -= (cycles - 1);
            cart->cpuCycle(cycles);
            cycles = 0;
            continue;
        }
        else
        {
            cycles -= remaining_cycles;
            cart->cpuCycle(remaining_cycles);
            return;
        }
    }
}

void Cpu6502::reset()
{
	addr_abs = 0xFFFC;
	uint8_t low_byte = read(addr_abs);
	uint8_t high_byte = read(addr_abs + 1);

	PC = (high_byte << 8) | low_byte;
	A = 0;
	X = 0;
	Y = 0;
	SP = 0xFD;
	status = 0x00 | U;

	addr_rel = 0x0000;
	addr_abs = 0x0000;
	fetched = 0x00;

	cycles = 8;
}

IRAM_ATTR void Cpu6502::apuWrite(uint16_t addr, uint8_t data)
{
    apu.cpuWrite(addr, data);
}

inline uint8_t Cpu6502::fetch()
{
	if (addrmode_implied == false)
		fetched = read(addr_abs);
	return fetched;
}

inline uint8_t Cpu6502::ABS()
{
	uint8_t low_byte = read(PC++);
	uint8_t high_byte = read(PC++);

	addr_abs = (high_byte << 8) | low_byte;
	return 0;
}

inline uint8_t Cpu6502::ABX()
{
	uint8_t low_byte = read(PC++);
	uint8_t high_byte = read(PC++);

	addr_abs = (high_byte << 8) | low_byte;
	addr_abs += X;

	if ((addr_abs & 0xFF00) != (high_byte << 8))
		return 1;
	else
		return 0;
}

inline uint8_t Cpu6502::ABY()
{
	uint8_t low_byte = read(PC++);
	uint8_t high_byte = read(PC++);

	addr_abs = (high_byte << 8) | low_byte;
	addr_abs += Y;

	if ((addr_abs & 0xFF00) != (high_byte << 8))
		return 1;
	else
		return 0;

}

inline uint8_t Cpu6502::IMM()
{
    addr_abs = PC++;
    return 0;
}

inline uint8_t Cpu6502::IMP()
{
    fetched = A;
    addrmode_implied = true;
    return 0;
}

inline uint8_t Cpu6502::IND()
{
	uint8_t low_byte = read(PC++);
	uint8_t high_byte = read(PC++);

	uint16_t ptr = (high_byte << 8) | low_byte;
	
	if (low_byte == 0xFF)
	{
		addr_abs = (read(ptr & 0xFF00) << 8) | read(ptr);
	} 
	else
	{
		addr_abs = (read(ptr + 1) << 8) | read(ptr);
	}
	return 0;
}

inline uint8_t Cpu6502::IDX()
{
	uint8_t temp = read(PC++);

	uint8_t low_byte = read((uint16_t)(temp + (uint16_t)X) & 0x00FF);
	uint8_t high_byte = read((uint16_t)(temp + (uint16_t)X + 1) & 0x00FF);

	addr_abs = (high_byte << 8) | low_byte;
	return 0;
}

inline uint8_t Cpu6502::IDY()
{
	uint8_t temp = read(PC++);

	uint8_t low_byte = read(temp & 0x00FF);
	uint8_t high_byte = read((temp + 1) & 0x00FF);

	addr_abs = (high_byte << 8) | low_byte;
	addr_abs += Y;

	if ((addr_abs & 0xFF00) != (high_byte << 8))
		return 1;
	else
		return 0;
}

inline uint8_t Cpu6502::REL()
{
	addr_rel = read(PC++);
	if (addr_rel & 0x80) addr_rel |= 0xFF00;
	return 0;
}

inline uint8_t Cpu6502::ZPG()
{
	addr_abs = read(PC++);
	return 0;
}

inline uint8_t Cpu6502::ZPX()
{
	addr_abs = read(PC++) + X;
	addr_abs &= 0x00FF;
	return 0;
}

inline uint8_t Cpu6502::ZPY()
{
	addr_abs = read(PC++) + Y;
	addr_abs &= 0x00FF;
	return 0;
}

inline uint8_t Cpu6502::Instr_LDA()
{
    fetch();
    A = fetched;
    SET_ZN(A);
    return 1;
}

inline uint8_t Cpu6502::Instr_LDX()
{
    fetch();
    X = fetched;
    SET_ZN(X);
    return 1;
}

inline uint8_t Cpu6502::Instr_LDY()
{
    fetch();
    Y = fetched;
    SET_ZN(Y);
    return 1;
}

inline uint8_t Cpu6502::Instr_STA()
{
    write(addr_abs, A);
    return 0;
}

inline uint8_t Cpu6502::Instr_STX()
{
    write(addr_abs, X);
    return 0;
}

inline uint8_t Cpu6502::Instr_STY()
{
    write(addr_abs, Y);
    return 0;
}

inline uint8_t Cpu6502::Instr_TAX()
{
    X = A;
    SET_ZN(X);
    return 0;
}

inline uint8_t Cpu6502::Instr_TAY()
{
    Y = A;
    SET_ZN(Y);
    return 0;
}

inline uint8_t Cpu6502::Instr_TSX()
{
    X = SP;
    SET_ZN(X);
    return 0;
}

inline uint8_t Cpu6502::Instr_TXA()
{
    A = X;
    SET_ZN(A);
    return 0;
}

inline uint8_t Cpu6502::Instr_TXS()
{
    SP = X;
    return 0;
}

inline uint8_t Cpu6502::Instr_TYA()
{
    A = Y;
    SET_ZN(A);
    return 0;
}

inline uint8_t Cpu6502::Instr_PHA()
{
    write(0x0100 + SP, A);
    SP--;
    return 0;
}

inline uint8_t Cpu6502::Instr_PHP()
{
    write(0x0100 + SP, status | B | U);
    status &= ~B;
    status |= U;
    SP--;
    return 0;
}

inline uint8_t Cpu6502::Instr_PLA()
{
    SP++;
    A = read(0x0100 + SP);
    SET_ZN(A);
    return 0;
}

inline uint8_t Cpu6502::Instr_PLP()
{
    SP++;
    status = read(0x0100 + SP);
    status &= ~B;
    status |= U;
    return 0;
}

inline uint8_t Cpu6502::Instr_DEC()
{
    fetch();
    temp = (fetched - 1) & 0x00FF;
    SET_ZN(temp);
    write(addr_abs, (uint8_t)temp);
    return 0;
}

inline uint8_t Cpu6502::Instr_DEX()
{
    X--;
    SET_ZN(X);
    return 0;
}

inline uint8_t Cpu6502::Instr_DEY()
{
    Y--;
    SET_ZN(Y);
    return 0;
}

inline uint8_t Cpu6502::Instr_INC()
{
    fetch();
    temp = (fetched + 1) & 0x00FF;
    SET_ZN(temp);
    write(addr_abs, (uint8_t)temp);
    return 0;
}

inline uint8_t Cpu6502::Instr_INX()
{
    X++;
    SET_ZN(X);
    return 0;
}

inline uint8_t Cpu6502::Instr_INY()
{
    Y++;
    SET_ZN(Y);
    return 0;
}

inline uint8_t Cpu6502::Instr_ADC()
{
    fetch();
    temp = (uint16_t)A + (uint16_t)fetched + (uint16_t)GET_FLAG(C);
    SET_FLAG(C, temp > 255);
    SET_FLAG(V, ((~((uint16_t)A ^ (uint16_t)fetched) & ((uint16_t)A ^ temp)) & 0x0080) != 0);
    A = temp & 0x00FF;
    SET_ZN(A);
    return 1;
}

inline uint8_t Cpu6502::Instr_SBC()
{
    fetch();
    uint16_t value = ((uint16_t)fetched) ^ 0x00FF;

    temp = (uint16_t)A + value + (uint16_t)GET_FLAG(C);
    SET_FLAG(C, temp > 255);
    SET_FLAG(V, ((temp ^ (uint16_t)A) & (temp ^ value) & 0x0080) != 0);
    A = temp & 0x00FF;
    SET_ZN(A);
    return 1;
}

inline uint8_t Cpu6502::Instr_AND()
{
    fetch();
    A = A & fetched;
    SET_ZN(A);
    return 1;
}

inline uint8_t Cpu6502::Instr_EOR()
{
    fetch();
    A = A ^ fetched;
    SET_ZN(A);
    return 1;
}

inline uint8_t Cpu6502::Instr_ORA()
{
    fetch();
    A = A | fetched;
    SET_ZN(A);
    return 1;
}

inline uint8_t Cpu6502::Instr_ASL()
{
    fetch();
    temp = (uint16_t)fetched << 1;
    SET_FLAG(C, (temp & 0xFF00) > 0);
    SET_ZN(temp & 0x00FF);
    if (addrmode_implied) A = temp & 0x00FF;
    else write(addr_abs, temp & 0x00FF);

    return 0;
}

inline uint8_t Cpu6502::Instr_LSR()
{
    fetch();
    SET_FLAG(C, (fetched & 0x0001) != 0);
    temp = fetched >> 1;
    SET_ZN(temp & 0x00FF);
    if (addrmode_implied) A = temp & 0x00FF;
    else write(addr_abs, temp & 0x00FF);

    return 0;
}

inline uint8_t Cpu6502::Instr_ROL()
{
    fetch();
    temp = (uint16_t)(fetched << 1) | GET_FLAG(C);
    SET_FLAG(C, (temp & 0xFF00) != 0);
    SET_ZN(temp & 0x00FF);
    if (addrmode_implied) A = temp & 0x00FF;
    else write(addr_abs, temp & 0x00FF);
    return 0;
}

inline uint8_t Cpu6502::Instr_ROR()
{
    fetch();
    temp = (uint16_t)(GET_FLAG(C) << 7) | (fetched >> 1);
    SET_FLAG(C, (fetched & 0x01) != 0);
    SET_ZN(temp & 0x00FF);
    if (addrmode_implied) A = temp & 0x00FF;
    else write(addr_abs, temp & 0x00FF);
    return 0;
}

inline uint8_t Cpu6502::Instr_CLC()
{
    status &= ~C;
    return 0;
}

inline uint8_t Cpu6502::Instr_CLD()
{
    status &= ~D;
    return 0;
}

inline uint8_t Cpu6502::Instr_CLI()
{
    status &= ~I;
    return 0;
}

inline uint8_t Cpu6502::Instr_CLV()
{
    status &= ~V;
    return 0;
}

inline uint8_t Cpu6502::Instr_SEC()
{
    status |= C;
    return 0;
}

inline uint8_t Cpu6502::Instr_SED()
{
    status |= D;
    return 0;
}

inline uint8_t Cpu6502::Instr_SEI()
{
    status |= I;
    return 0;
}

inline uint8_t Cpu6502::Instr_CMP()
{
    fetch();
    temp = (uint16_t)A - (uint16_t)fetched;
    SET_FLAG(C, A >= fetched);
    SET_ZN(temp & 0x00FF);
    return 1;
}

inline uint8_t Cpu6502::Instr_CPX()
{
    fetch();
    temp = (uint16_t)X - (uint16_t)fetched;
    SET_FLAG(C, X >= fetched);
    SET_ZN(temp & 0x00FF);
    return 0;
}

inline uint8_t Cpu6502::Instr_CPY()
{
    fetch();
    temp = (uint16_t)Y - (uint16_t)fetched;
    SET_FLAG(C, Y >= fetched);
    SET_ZN(temp & 0x00FF);
    return 0;
}

inline uint8_t Cpu6502::Instr_BCC()
{
    if (GET_FLAG(C) == 0)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00))
            cycles++;

        PC = addr_abs;
    }
    return 0;
}

inline uint8_t Cpu6502::Instr_BCS()
{
    if (GET_FLAG(C) == 1)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00))
            cycles++;

        PC = addr_abs;
    }
    return 0;
}

inline uint8_t Cpu6502::Instr_BEQ()
{
    if (GET_FLAG(Z) == 1)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00))
            cycles++;

        PC = addr_abs;
    }
    return 0;
}

inline uint8_t Cpu6502::Instr_BMI()
{
    if (GET_FLAG(N) == 1)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00))
            cycles++;

        PC = addr_abs;
    }
    return 0;
}

inline uint8_t Cpu6502::Instr_BNE()
{
    if (GET_FLAG(Z) == 0)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00))
            cycles++;

        PC = addr_abs;
    }
    return 0;
}

inline uint8_t Cpu6502::Instr_BPL()
{
    if (GET_FLAG(N) == 0)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00))
            cycles++;

        PC = addr_abs;
    }
    return 0;
}

inline uint8_t Cpu6502::Instr_BVC()
{
    if (GET_FLAG(V) == 0)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00))
            cycles++;

        PC = addr_abs;
    }
    return 0;
}

inline uint8_t Cpu6502::Instr_BVS()
{
    if (GET_FLAG(V) == 1)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00))
            cycles++;

        PC = addr_abs;
    }
    return 0;
}

inline uint8_t Cpu6502::Instr_JMP()
{
    PC = addr_abs;
    return 0;
}

inline uint8_t Cpu6502::Instr_JSR()
{
    PC--;
    write(0x0100 + SP, (PC >> 8) & 0x00FF);
    write(0x0100 + (uint8_t)(SP - 1), PC & 0x00FF);

    SP -= 2;
    PC = addr_abs;
    return 0;
}

inline uint8_t Cpu6502::Instr_RTS()
{
    PC = read(0x0100 | (uint8_t)(SP + 1));
    PC |= read(0x0100 | (uint8_t)(SP + 2)) << 8;

    SP += 2;
    PC++;
    return 0;
}

inline uint8_t Cpu6502::Instr_BRK()
{
    write(0x0100 |  SP, (PC >> 8) & 0x00FF);
    write(0x0100 | (uint8_t)(SP - 1), PC & 0x00FF);
    status |= (U | B);
    write(0x0100 | (uint8_t)(SP - 2), status);
    status &= ~B;
    status |= I;

    uint8_t low_byte = read(0xFFFE);
    uint8_t high_byte = read(0xFFFF);

    SP -= 3;
    PC = (high_byte << 8) | low_byte;
    return 0;
}

inline uint8_t Cpu6502::Instr_RTI()
{
    status = read(0x0100 | (uint8_t)(SP + 1));
    status &= ~B;
    status |= U;

    PC = (uint16_t)read(0x0100 | (uint8_t)(SP + 2));
    PC |= (uint16_t)read(0x0100 | (uint8_t)(SP + 3)) << 8;
    SP += 3;
    return 0;
}

inline uint8_t Cpu6502::Instr_BIT()
{
    fetch();
    temp = A & fetched;
    SET_FLAG(Z, temp == 0);
    SET_FLAG(N, fetched & 0x80);
    SET_FLAG(V, fetched & 0x40);
    return 0;
}

inline uint8_t Cpu6502::Instr_NOP()
{
    return 0;
}

inline uint8_t Cpu6502::Instr_XXX()
{
    return 0;
}

void Cpu6502::IRQ()
{
    if (GET_FLAG(I) == 0)
    {
        write(0x0100 | SP, (PC >> 8) & 0x00FF);
        write(0x0100 | (uint8_t)(SP - 1), PC & 0x00FF);

        write(0x0100 | (uint8_t)(SP - 2), status);

        status |= I;

        uint8_t low_byte = read(0xFFFE);
        uint8_t high_byte = read(0xFFFF);

        PC = (high_byte << 8) | low_byte;

        SP -= 3;
        cycles += 7;
    }
}

void Cpu6502::NMI()
{
    write(0x0100 | SP, (PC >> 8) & 0x00FF);
    write(0x0100 | (uint8_t)(SP - 1), PC & 0x00FF);

    write(0x0100 | (uint8_t)(SP - 2), status);

    status |= I;

    uint8_t low_byte = read(0xFFFA);
    uint8_t high_byte = read(0xFFFB);

    PC = (high_byte << 8) | low_byte;

    SP -= 3;
    cycles += 8;
}

void Cpu6502::dumpState(File& state)
{
    state.write((uint8_t*)&A, sizeof(A));
    state.write((uint8_t*)&X, sizeof(X));
    state.write((uint8_t*)&Y, sizeof(Y));
    state.write((uint8_t*)&PC, sizeof(PC));
    state.write((uint8_t*)&SP, sizeof(SP));
    state.write((uint8_t*)&status, sizeof(status));

    state.write((uint8_t*)&fetched, sizeof(fetched));
    state.write((uint8_t*)&addr_abs, sizeof(addr_abs));
    state.write((uint8_t*)&addr_rel, sizeof(addr_rel));
    state.write((uint8_t*)&opcode, sizeof(opcode));
    state.write((uint8_t*)&cycles, sizeof(cycles));
    state.write((uint8_t*)&temp, sizeof(temp));

    state.write((uint8_t*)&addrmode_implied, sizeof(addrmode_implied));
    state.write((uint8_t*)&additional_cycle1, sizeof(additional_cycle1));
    state.write((uint8_t*)&additional_cycle2, sizeof(additional_cycle2));
    state.write((uint8_t*)&OAM_DMA_page, sizeof(OAM_DMA_page));
}

void Cpu6502::loadState(File& state)
{
    state.read((uint8_t*)&A, sizeof(A));
    state.read((uint8_t*)&X, sizeof(X));
    state.read((uint8_t*)&Y, sizeof(Y));
    state.read((uint8_t*)&PC, sizeof(PC));
    state.read((uint8_t*)&SP, sizeof(SP));
    state.read((uint8_t*)&status, sizeof(status));

    state.read((uint8_t*)&fetched, sizeof(fetched));
    state.read((uint8_t*)&addr_abs, sizeof(addr_abs));
    state.read((uint8_t*)&addr_rel, sizeof(addr_rel));
    state.read((uint8_t*)&opcode, sizeof(opcode));
    state.read((uint8_t*)&cycles, sizeof(cycles));
    state.read((uint8_t*)&temp, sizeof(temp));

    state.read((uint8_t*)&addrmode_implied, sizeof(addrmode_implied));
    state.read((uint8_t*)&additional_cycle1, sizeof(additional_cycle1));
    state.read((uint8_t*)&additional_cycle2, sizeof(additional_cycle2));
    state.read((uint8_t*)&OAM_DMA_page, sizeof(OAM_DMA_page));
}
// ===== END src/core/cpu6502.cpp =====

// ===== BEGIN src/core/ppu2C02.cpp =====


#define READ_PALETTE(x) palette_table[((x) & 0x1F) ^ (((x) & 0x13) == 0x10 ? 0x10 : 0x00)]
DMA_ATTR uint16_t Ppu2C02::display_buffer[SCANLINE_SIZE * SCANLINES_PER_BUFFER];
constexpr uint8_t Ppu2C02::palette_mirror[32];

Ppu2C02::Ppu2C02()
{
    memset(nametable, 0, sizeof(nametable));
    memset(palette_table, 0, sizeof(palette_table));
    memset(scanline_buffer, 0, sizeof(scanline_buffer));
    memset(scanline_metadata, 0, sizeof(scanline_metadata));
    memset(display_buffer, 0, sizeof(display_buffer));
    memset(sprite, 0, sizeof(sprite));
}

Ppu2C02::~Ppu2C02()
{

}

inline void Ppu2C02::ppuWrite(uint16_t addr, uint8_t data)
{
    addr &= 0x3FFF;
    
    if (cart->ppuWrite(addr, data)) return;
    else if (addr >= 0x2000 && addr <= 0x3EFF)
    {
        ptr_nametable[(addr >> 10) & 3][addr & 0x03FF] = data;
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF)
    {
        addr = palette_mirror[addr & 0x001F];
        palette_table[addr] = data;
    }
}

inline uint8_t Ppu2C02::ppuRead(uint16_t addr)
{
    uint8_t data = 0x00;
    addr &= 0x3FFF;
    
    if (cart->ppuRead(addr, data)) return data;
    else if (addr >= 0x2000 && addr <= 0x3EFF)
    {
        data = ptr_nametable[(addr >> 10) & 3][addr & 0x03FF];
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF)
    {
        addr &= 0x001F;
        switch (addr)
        {
        case 0x0010: addr = 0x0000; break;
        case 0x0014: addr = 0x0004; break;
        case 0x0018: addr = 0x0008; break;
        case 0x001C: addr = 0x000C; break;
        }
        data = palette_table[addr] & (mask.grayscale ? 0x30 : 0x3F);
    }

    return data;
}

IRAM_ATTR void Ppu2C02::cpuWrite(uint16_t addr, uint8_t data)
{
    switch (addr)
    {
    case 0x2000: // PPUCTRL
        control.reg = data;
        t.nametable_x = control.nametable_x;
        t.nametable_y = control.nametable_y;
        break;
    case 0x2001: // PPUMASK
        mask.reg = data;
        break;
    case 0x2003: // OAMADDR
        OAMADDR = data;
        break;
    case 0x2004: // OAMDATA
        ptr_sprite[OAMADDR++] = data;
        break;
    case 0x2005: // PPUSCROLL
        if (w == 0)
        {
            x = data & 0x07;
            t.coarse_x = data >> 3;
        }
        else
        {
            t.fine_y = data & 0x07;
            t.coarse_y = data >> 3;
        }
        w = ~w;
        break;
    case 0x2006: // PPUADDR
        if (w == 0)
        {
            t.reg = (t.reg & 0x00FF) | (uint16_t)((data & 0x3F) << 8);
        }
        else
        {
            t.reg = (t.reg & 0xFF00) | data;
            v.reg = t.reg;
        }
        w = ~w;
        break;
    case 0x2007: // PPUDATA
        ppuWrite(v.reg, data);
        v.reg += (control.VRAM_addr_increment ? 32 : 1);
        break;
    }
}

IRAM_ATTR uint8_t Ppu2C02::cpuRead(uint16_t addr)
{
    uint8_t data = 0x00;
    switch (addr)
    {
    case 0x2002: // PPUSTATUS
        data = status.reg & 0xE0;
        status.VBlank = 0;
        w = 0;
        break;
    case 0x2004: // OAMDATA
        data = ptr_sprite[OAMADDR];
        break;
    case 0x2007: // PPUDATA
        data = PPUDATA_buffer;
        PPUDATA_buffer = ppuRead(v.reg);

        if (v.reg >= 0x3F00 && v.reg <= 0x3FFF) data = PPUDATA_buffer;
        v.reg += (control.VRAM_addr_increment ? 32 : 1);
        break;
    }

    return data;
}

IRAM_ATTR void Ppu2C02::setVBlank()
{
    status.VBlank = 1;
    if (control.Vblank_NMI) bus->NMI();
}

IRAM_ATTR void Ppu2C02::clearVBlank()
{
    status.VBlank = 0;
    status.sprite_zero_hit = 0;
    status.sprite_overflow = 0;
}

IRAM_ATTR void Ppu2C02::renderScanline(uint16_t scanline)
{
    transferScroll(scanline);
    renderBackground();
    renderSprites(scanline);
    incrementY();
    finishScanline(scanline);
}

inline void Ppu2C02::transferScroll(uint16_t scanline)
{
    if (!(mask.reg & (1 << 3) || mask.reg & (1 << 4))) return;
    v.reg = (scanline == 0) ? t.reg : v.reg = (v.reg & ~0x041F) | (t.reg & 0x041F);
}

inline void Ppu2C02::incrementY()
{
    if (!(mask.render_background || mask.render_sprite)) return;

    if (v.fine_y < 7)
    {
        v.fine_y++;
        return;
    }
    
    v.fine_y = 0;
    if (v.coarse_y == 29)
    {
        v.coarse_y = 0;
        v.nametable_y = ~v.nametable_y;
    }
    else if (v.coarse_y == 31) v.coarse_y = 0;
    else v.coarse_y++;
}

inline void Ppu2C02::renderBackground()
{   
    // Show transparency pixel if not rendering background
    if (!mask.render_background)
    {
        uint16_t bg_color = nes_palette[mask.emphasize][palette_table[0]];
        uint32_t color32 = ((uint32_t)bg_color << 16) | bg_color;
        uint32_t* buffer = (uint32_t*)scanline_buffer;
        for (int i = 0, size = (BUFFER_SIZE >> 1); i < size; i++) 
            buffer[i] = color32;

        memset(scanline_metadata, 0x80, BUFFER_SIZE);
        ptr_buffer = scanline_buffer + x;
        ptr_scanline_meta = scanline_metadata + x;
        return;
    }

    uint16_t bg_color = nes_palette[mask.emphasize][palette_table[0]];
    ptr_buffer = scanline_buffer;
    ptr_scanline_meta = scanline_metadata;
    x_tile = v.coarse_x;
    y_tile = v.coarse_y;
    offset = (control.background_table_addr ? 0x1000 : 0x0000) + v.fine_y;
    nametable_index = (v.reg >> 10) & 3;

    nametable_byte_base = v.reg & 0x03E0;
    ptr_tile = &ptr_nametable[nametable_index][nametable_byte_base + x_tile];

    attribute_byte_base = 0x03C0 + ((y_tile & 0x1C) << 1);
    ptr_attribute = &ptr_nametable[nametable_index][attribute_byte_base + (x_tile >> 2)];
    attribute_byte = *ptr_attribute++;
    attribute_shift = ((y_tile & 2) << 1) + (x_tile & 2);
    attribute = ((attribute_byte >> attribute_shift) & 3) << 2;

    static constexpr DRAM_ATTR uint8_t pixel_shift[8] = { 14, 6, 12, 4, 10, 2, 8, 0 }; // Shifts to get the bits of a pixel
    static constexpr DRAM_ATTR uint8_t pixel_metadata[4] = { 0x80, 0x00, 0x00, 0x00 };
    for (int tile = 0; tile < 33; tile++)
    {
        tile_index = *ptr_tile++;
        ptr_pattern_tile = cart->ppuReadPtr(offset + (tile_index << 4)); 

        // draw to framebuffer
        uint16_t pattern = ((ptr_pattern_tile[8] & 0xAA) << 8) | ((ptr_pattern_tile[8] & 0x55) << 1)
                    | ((ptr_pattern_tile[0] & 0xAA) << 7) | (ptr_pattern_tile[0] & 0x55);
        uint16_t tile_palette[4];
        tile_palette[0] = bg_color;
        for (int t = 1; t < 4; t++) tile_palette[t] = nes_palette[mask.emphasize][READ_PALETTE(attribute + t)];
        for (int i = 0; i < 8; i++)
        {   
            uint8_t pixel = (pattern >> pixel_shift[i]) & 3;
            *ptr_buffer++ = tile_palette[pixel];
            *ptr_scanline_meta++ = pixel_metadata[pixel]; // Store if pixel is transparent for sprite rendering
        }

        x_tile++;
        if ((x_tile & 1) == 0)
        {
            if ((x_tile & 3) == 0)
            {
                if (x_tile == 32)
                {
                    // switch nametable
                    x_tile = 0;
                    nametable_index ^= 1;

                    // recalculate pointer to tile and attribute data
                    ptr_tile = &ptr_nametable[nametable_index][nametable_byte_base];
                    ptr_attribute = &ptr_nametable[nametable_index][attribute_byte_base];
                }

                attribute_byte = *ptr_attribute++;
            }

            attribute_shift ^= 2;
            attribute = ((attribute_byte >> attribute_shift) & 0x03) << 2;
        }
    }
    ptr_buffer = scanline_buffer + x;
}

inline void Ppu2C02::renderSprites(uint16_t scanline)
{
    if (!mask.render_sprite) 
    {
        return;
    }

    OAM* ptr_sprite_OAM;
    uint8_t sprite_size;
    uint8_t sprite_count = 0;

    uint16_t bg_color = nes_palette[mask.emphasize][palette_table[0]];
    uint16_t tile_palette[4];
    tile_palette[0] = bg_color;

    ptr_sprite_OAM = sprite;
    offset = (control.sprite_table_addr ? 0x1000 : 0);

    sprite_size = (control.sprite_size ? 16 : 8);

    uint16_t* buffer_offset = scanline_buffer + x;
    uint8_t* metadata_offset = scanline_metadata + x;
    for (int i = 0; i < 64; i++, ptr_sprite_OAM++)
    {
        uint8_t sprite_x, sprite_y;
        sprite_y = ptr_sprite_OAM->y + 1;
        // Check if sprite is in scanline
        if ((sprite_y > scanline) || (sprite_y <= (scanline - sprite_size)) 
            || (sprite_y == 0) || (sprite_y >= 240))
            continue;

        int16_t y_offset;
        uint16_t tile_addr;

        sprite_x = ptr_sprite_OAM->x;
        tile_index = ptr_sprite_OAM->index;
        attribute_byte = ptr_sprite_OAM->attribute;
        attribute = ((attribute_byte & 0x03) << 2);
    
        ptr_buffer = buffer_offset + sprite_x;
        ptr_scanline_meta = metadata_offset + sprite_x;

        // If 8x16 sprite mode
        tile_addr = (control.sprite_size) ? ((tile_index & 0x01) << 12) | ((tile_index & 0xFE) << 4) : offset + (tile_index << 4);
        ptr_tile = cart->ppuReadPtr(tile_addr);

        y_offset = scanline - sprite_y;
        if (y_offset > 7) y_offset += 8;
        if (attribute_byte & 0x80) // If flip sprite vertically
        {
            y_offset -= (control.sprite_size) ? 23 : 7;
            ptr_tile -= y_offset;
        }
        else ptr_tile += y_offset;

        // Draw to buffer
        uint16_t pattern = ((ptr_tile[8] & 0xAA) << 8) | ((ptr_tile[8] & 0x55) << 1)
                    | ((ptr_tile[0] & 0xAA) << 7) | (ptr_tile[0] & 0x55);
        if (pattern)
        {
            uint8_t pixel[8];
            uint8_t palette_offset = 16 + attribute;
            for (int t = 1; t < 4; t++) tile_palette[t] = nes_palette[mask.emphasize][READ_PALETTE(palette_offset + t)];

            if (attribute_byte & 0x40) // If flip sprite horizontally
            {
                pixel[7] = (pattern >> 14) & 3;
                pixel[6] = (pattern >> 6) & 3;
                pixel[5] = (pattern >> 12) & 3;
                pixel[4] = (pattern >> 4) & 3;
                pixel[3] = (pattern >> 10) & 3;
                pixel[2] = (pattern >> 2) & 3;
                pixel[1] = (pattern >> 8) & 3;
                pixel[0] = pattern & 3;
            }
            else
            {
                pixel[0] = (pattern >> 14) & 3;
                pixel[1] = (pattern >> 6) & 3;
                pixel[2] = (pattern >> 12) & 3;
                pixel[3] = (pattern >> 4) & 3;
                pixel[4] = (pattern >> 10) & 3;
                pixel[5] = (pattern >> 2) & 3;
                pixel[6] = (pattern >> 8) & 3;
                pixel[7] = pattern & 3;
            }

            // Check for sprite 0 hit
            if (i == 0 && status.sprite_zero_hit == 0)
            {
                for (int j = 0; j < 8; j++)
                {
                    if (pixel[j] && ((ptr_scanline_meta[j] & 0x80) == 0))
                    {
                        status.sprite_zero_hit = true;
                        break;
                    }
                }
            }

            // Render sprite pixels on scanline buffer
            if (attribute_byte & 0x20) // Sprite Priorty : 1 - behind background | 0 - in front of background
            {
                for (int j = 0; j < 8; j++)
                {
                    if (pixel[j])
                    {
                        if (ptr_scanline_meta[j] & 0x80) ptr_buffer[j] = tile_palette[pixel[j]];
                        ptr_scanline_meta[j] |= 0x40;
                    }
                }
            }
            else
            {
                for (int j = 0; j < 8; j++)
                {
                    if (pixel[j] && ((ptr_scanline_meta[j] & 0x40) == 0))
                    {
                        ptr_buffer[j] = tile_palette[pixel[j]];;
                        ptr_scanline_meta[j] |= 0x40;
                    }
                }
            }
        }

        // If sprite overflow, break
        if (++sprite_count == 8) { status.sprite_overflow = true; break; }
    }

    ptr_buffer = buffer_offset;
}

void Ppu2C02::fakeSpriteHit(uint16_t scanline)
{
    if (mask.render_background || mask.render_sprite) cart->ppuScanline();
    if (!mask.render_sprite || status.sprite_zero_hit) return;

    uint8_t sprite_size;
    offset = (control.sprite_table_addr ? 0x1000 : 0);
    sprite_size = (control.sprite_size ? 16 : 8);

    uint8_t sprite_x, sprite_y;
    sprite_y = sprite[0].y + 1;
    // Check if sprite is in scanline
    if ((sprite_y > scanline) || (sprite_y <= (scanline - sprite_size)) 
        || (sprite_y == 0) || (sprite_y >= 240))
        return;

    int16_t y_offset;
    uint16_t tile_addr;

    sprite_x = sprite[0].x;
    tile_index = sprite[0].index;
    attribute_byte = sprite[0].attribute;

    tile_addr = (control.sprite_size) ? ((tile_index & 0x01) << 12) | ((tile_index & 0xFE) << 4) : offset + (tile_index << 4);
    ptr_tile = cart->ppuReadPtr(tile_addr);

    y_offset = scanline - sprite_y;
    if (y_offset > 7) y_offset += 8;

    if (attribute_byte & 0x80) // If flip sprite vertically
    {
        y_offset -= (control.sprite_size) ? 23 : 7;
        ptr_tile -= y_offset;
    }
    else ptr_tile += y_offset;

    // Draw to buffer
    uint16_t pattern = ((ptr_tile[8] & 0xAA) << 8) | ((ptr_tile[8] & 0x55) << 1)
                    | ((ptr_tile[0] & 0xAA) << 7) | (ptr_tile[0] & 0x55);
    if (pattern)
    {
        status.sprite_zero_hit = true;
        return;
        // uint8_t pixel[8];
        // if (attribute_byte & 0x40) // If flip sprite horizontally
        // {
        //     pixel[7] = (pattern >> 14) & 3;
        //     pixel[6] = (pattern >> 6) & 3;
        //     pixel[5] = (pattern >> 12) & 3;
        //     pixel[4] = (pattern >> 4) & 3;
        //     pixel[3] = (pattern >> 10) & 3;
        //     pixel[2] = (pattern >> 2) & 3;
        //     pixel[1] = (pattern >> 8) & 3;
        //     pixel[0] = pattern & 3;
        // }
        // else
        // {
        //     pixel[0] = (pattern >> 14) & 3;
        //     pixel[1] = (pattern >> 6) & 3;
        //     pixel[2] = (pattern >> 12) & 3;
        //     pixel[3] = (pattern >> 4) & 3;
        //     pixel[4] = (pattern >> 10) & 3;
        //     pixel[5] = (pattern >> 2) & 3;
        //     pixel[6] = (pattern >> 8) & 3;
        //     pixel[7] = pattern & 3;
        // }

        // for (int i = 0; i < 8; i++)
        // {
        //     if (pixel[i]) 
        //     {
        //         status.sprite_zero_hit = true;
        //         return;
        //     }
        // }
    }
}

inline void Ppu2C02::finishScanline(uint16_t scanline)
{
    if (mask.render_background || mask.render_sprite) 
        cart->ppuScanline();
    uint32_t* display = (uint32_t*)(ptr_display + (scanline_counter * SCANLINE_SIZE));
    uint32_t* buffer = (uint32_t*)ptr_buffer;
    for (int i = 0, size = (SCANLINE_SIZE >> 1); i < size; i++) 
        display[i] = buffer[i];
    scanline_counter++;
    if (scanline_counter >= SCANLINES_PER_BUFFER) 
    { 
        bus->renderImage(scanline - (SCANLINES_PER_BUFFER - 1));
        scanline_counter = 0;
    }
}

void Ppu2C02::reset()
{
    status.reg = 0x00;
	mask.reg = 0x00;
	control.reg = 0x00;
	t.reg = 0x00;
	v.reg = 0x00;
	x = 0x00;
    w = 0x00;
    OAMADDR = 0x00;
    OAMDATA = 0x00;
	PPUDATA_buffer = 0x00;
    nametable_byte = 0x00;
    attribute_byte = 0x00;
}

void Ppu2C02::connectCartridge(Cartridge* cartridge)
{
    cart = cartridge;
    setMirror((Cartridge::MIRROR)cart->hardware_mirror);
}

void Ppu2C02::setMirror(Cartridge::MIRROR mirror)
{
    switch (mirror)
    {
        case Cartridge::MIRROR::VERTICAL:
            ptr_nametable[0] = &nametable[0x0000];
            ptr_nametable[1] = &nametable[0x0400];
            ptr_nametable[2] = &nametable[0x0000];
            ptr_nametable[3] = &nametable[0x0400];
            break;

        case Cartridge::MIRROR::HORIZONTAL:
            ptr_nametable[0] = &nametable[0x0000];
            ptr_nametable[1] = &nametable[0x0000];
            ptr_nametable[2] = &nametable[0x0400];
            ptr_nametable[3] = &nametable[0x0400];
            break;

        case Cartridge::MIRROR::ONESCREEN_LOW:
            ptr_nametable[0] = ptr_nametable[1] = ptr_nametable[2] = ptr_nametable[3] = &nametable[0x0000];
            break;

        case Cartridge::MIRROR::ONESCREEN_HIGH:
            ptr_nametable[0] = ptr_nametable[1] = ptr_nametable[2] = ptr_nametable[3] = &nametable[0x0400];
            break;
    }
}

Cartridge::MIRROR Ppu2C02::getMirror()
{
    if (ptr_nametable[0] == &nametable[0x0000] && 
        ptr_nametable[1] == &nametable[0x0400] &&
        ptr_nametable[2] == &nametable[0x0000] &&
        ptr_nametable[3] == &nametable[0x0400])
        return Cartridge::MIRROR::VERTICAL;

    if (ptr_nametable[0] == &nametable[0x0000] && 
        ptr_nametable[1] == &nametable[0x0000] &&
        ptr_nametable[2] == &nametable[0x0400] &&
        ptr_nametable[3] == &nametable[0x0400])
        return Cartridge::MIRROR::HORIZONTAL;

    if (ptr_nametable[0] == &nametable[0x0000] && 
        ptr_nametable[1] == &nametable[0x0000] &&
        ptr_nametable[2] == &nametable[0x0000] &&
        ptr_nametable[3] == &nametable[0x0000])
        return Cartridge::MIRROR::ONESCREEN_LOW;

    if (ptr_nametable[0] == &nametable[0x0400] && 
        ptr_nametable[1] == &nametable[0x0400] &&
        ptr_nametable[2] == &nametable[0x0400] &&
        ptr_nametable[3] == &nametable[0x0400])
        return Cartridge::MIRROR::ONESCREEN_HIGH;

    return Cartridge::MIRROR::HORIZONTAL;
}

void Ppu2C02::setPalette(uint8_t palette)
{
    switch (palette)
    {
    case NTSC565:
        nes_palette = palette_NTSC565;
        break;

    case PAL565:
        nes_palette = palette_PAL565;
        break;

    case NTSC222:
        nes_palette = palette_NTSC222;
        break;
    
    case PAL222:
        nes_palette = palette_PAL222;
        break;
    }
}

void Ppu2C02::dumpState(File& state)
{
    state.write((uint8_t*)scanline_buffer, sizeof(scanline_buffer));
    state.write((uint8_t*)scanline_metadata, sizeof(scanline_metadata));
    state.write(nametable, sizeof(nametable));
    for (int i = 0; i < 4; i++)
    {
        uint8_t map = 0;
        if (ptr_nametable[i] == &nametable[0x0400]) 
            map = 1;

        state.write((uint8_t*)&map, sizeof(map));
    }
    state.write(palette_table, sizeof(palette_table));
    state.write((uint8_t*)&scanline_counter, sizeof(scanline_counter));

    state.write((uint8_t*)&control.reg, sizeof(control.reg));
    state.write((uint8_t*)&mask.reg, sizeof(mask.reg));
    state.write((uint8_t*)&status.reg, sizeof(status.reg));
    state.write((uint8_t*)&OAMADDR, sizeof(OAMADDR));
    state.write((uint8_t*)&OAMDATA, sizeof(OAMDATA));

    state.write((uint8_t*)sprite, sizeof(sprite));
    state.write((uint8_t*)&v.reg, sizeof(v.reg));
    state.write((uint8_t*)&t.reg, sizeof(t.reg));
    state.write((uint8_t*)&x, sizeof(x));
    state.write((uint8_t*)&w, sizeof(w));
    state.write((uint8_t*)&PPUDATA_buffer, sizeof(PPUDATA_buffer));

    state.write((uint8_t*)&offset, sizeof(offset));
    state.write((uint8_t*)&nametable_index, sizeof(nametable_index));
    state.write((uint8_t*)&nametable_byte_base, sizeof(nametable_byte_base));
    state.write((uint8_t*)&attribute_byte_base, sizeof(attribute_byte_base));
    state.write((uint8_t*)&nametable_byte, sizeof(nametable_byte));
    state.write((uint8_t*)&attribute_byte, sizeof(attribute_byte));
    state.write((uint8_t*)&x_tile, sizeof(x_tile));
    state.write((uint8_t*)&y_tile, sizeof(y_tile));
    state.write((uint8_t*)&attribute_shift, sizeof(attribute_shift));
    state.write((uint8_t*)&attribute, sizeof(attribute));
    state.write((uint8_t*)&tile_index, sizeof(tile_index));
    state.write((uint8_t*)&sprite_count, sizeof(sprite_count));
}

void Ppu2C02::loadState(File& state)
{
    state.read((uint8_t*)scanline_buffer, sizeof(scanline_buffer));
    state.read((uint8_t*)scanline_metadata, sizeof(scanline_metadata));
    state.read(nametable, sizeof(nametable));
    for (int i = 0; i < 4; i++)
    {
        uint8_t map = 0;
        state.read(&map, sizeof(map));
        ptr_nametable[i] = &nametable[(map == 0) ? 0x0000 : 0x0400];
    }
    state.read(palette_table, sizeof(palette_table));
    state.read((uint8_t*)&scanline_counter, sizeof(scanline_counter));

    state.read((uint8_t*)&control.reg, sizeof(control.reg));
    state.read((uint8_t*)&mask.reg, sizeof(mask.reg));
    state.read((uint8_t*)&status.reg, sizeof(status.reg));
    state.read((uint8_t*)&OAMADDR, sizeof(OAMADDR));
    state.read((uint8_t*)&OAMDATA, sizeof(OAMDATA));

    state.read((uint8_t*)sprite, sizeof(sprite));
    state.read((uint8_t*)&v.reg, sizeof(v.reg));
    state.read((uint8_t*)&t.reg, sizeof(t.reg));
    state.read((uint8_t*)&x, sizeof(x));
    state.read((uint8_t*)&w, sizeof(w));
    state.read((uint8_t*)&PPUDATA_buffer, sizeof(PPUDATA_buffer));

    state.read((uint8_t*)&offset, sizeof(offset));
    state.read((uint8_t*)&nametable_index, sizeof(nametable_index));
    state.read((uint8_t*)&nametable_byte_base, sizeof(nametable_byte_base));
    state.read((uint8_t*)&attribute_byte_base, sizeof(attribute_byte_base));
    state.read((uint8_t*)&nametable_byte, sizeof(nametable_byte));
    state.read((uint8_t*)&attribute_byte, sizeof(attribute_byte));
    state.read((uint8_t*)&x_tile, sizeof(x_tile));
    state.read((uint8_t*)&y_tile, sizeof(y_tile));
    state.read((uint8_t*)&attribute_shift, sizeof(attribute_shift));
    state.read((uint8_t*)&attribute, sizeof(attribute));
    state.read((uint8_t*)&tile_index, sizeof(tile_index));
    state.read((uint8_t*)&sprite_count, sizeof(sprite_count));
}
// ===== END src/core/ppu2C02.cpp =====

// ===== BEGIN src/core/bus.cpp =====

Bus::Bus()
{
    memset(RAM, 0, sizeof(RAM));
    cpu.connectBus(this);
    cpu.apu.connectBus(this);
    ppu.connectBus(this);
}

Bus::~Bus()
{
}

IRAM_ATTR void Bus::cpuWrite(uint16_t addr, uint8_t data)
{
    if (cart->cpuWrite(addr, data)) {}
    else if ((addr & 0xE000) == 0x0000)
    {
        RAM[addr & 0x07FF] = data;
    }
    else if ((addr & 0xE000) == 0x2000)
    {
        ppu.cpuWrite(addr, data);
    }
    else if ((addr & 0xF000) == 0x4000 && (addr <= 0x4013 || addr == 0x4015 || addr == 0x4017))
    {
        cpu.apuWrite(addr, data);
    }
    else if (addr == 0x4014)
    {
        cpu.OAM_DMA(data);
    }
    else if (addr == 0x4016)
    {
        controller_strobe = data & 1;
        if (controller_strobe)
        {
            controller_state = controller;
        }
    }
}

IRAM_ATTR uint8_t Bus::cpuRead(uint16_t addr)
{
    uint8_t data = 0x00;

    if (cart->cpuRead(addr, data)) {}
    else if ((addr & 0xE000) == 0x0000)
    {   
        data = RAM[addr & 0x07FF];
    }
    else if ((addr & 0xE000) == 0x2000)
    {
        data = ppu.cpuRead(addr);
    }
    else if (addr == 0x4016)
    {
        uint8_t value = controller_state & 1;
        if (!controller_strobe)
            controller_state >>= 1;
        data = value | 0x40;
    }
    return data;
}

void Bus::reset()
{
    ptr_screen->fillScreen(TFT_BLACK);
	for (auto& i : RAM) i = 0x00;
    cart->reset();
	cpu.reset();
    cpu.apu.reset();
	ppu.reset();
}

IRAM_ATTR void Bus::clock()
{
    // 1 frame == 341 dots * 261 scanlines
    // Visible scanlines 0-239
    
    // Rendering 3 scanlines at a time because 1 CPU clock == 3 PPU clocks
    // and there's only 341 ppu clocks (dots) in a scanline, which is not divisible by 3.
    // Using a counter/for loop with += 341 & -= 3 is too big of a performance hit.
    // 1 scanline == ~113.67 CPU clocks, so for every 3 scanlines, two scanlines will have an extra CPU clock
    if (!frame_latch)
    {
        for (ppu_scanline = 0; ppu_scanline < 240; ppu_scanline += 3)
        {
            cpu.clock(113);
            ppu.renderScanline(ppu_scanline);

            cpu.clock(114);
            ppu.renderScanline(ppu_scanline + 1);

            cpu.clock(114);
            ppu.renderScanline(ppu_scanline + 2);
        }
    }
    else
    {
        for (ppu_scanline = 0; ppu_scanline < 240; ppu_scanline += 3)
        {
            cpu.clock(113);
            ppu.fakeSpriteHit(ppu_scanline);

            cpu.clock(114);
            ppu.fakeSpriteHit(ppu_scanline + 1);

            cpu.clock(114);
            ppu.fakeSpriteHit(ppu_scanline + 2);
        }
    }

    // Setup for the next frame
    // Same reason as scanlines 0-239, 2/3 of scanlines will have an extra CPU clock. 
    // Scanline 240
    cpu.clock(113);

    // Scanline 241-261
    ppu.setVBlank();
    cpu.clock(2501);

    ppu.clearVBlank();
    cpu.clock(114);

#ifdef FRAMESKIP
    frame_latch = !frame_latch;
#endif
}

IRAM_ATTR void Bus::setPPUMirrorMode(Cartridge::MIRROR mirror)
{
    ppu.setMirror(mirror);
}

Cartridge::MIRROR Bus::getPPUMirrorMode()
{
    return ppu.getMirror();
}

IRAM_ATTR void Bus::OAM_Write(uint8_t addr, uint8_t data)
{
    ppu.ptr_sprite[addr] = data;
}

void Bus::insertCartridge(Cartridge* cartridge)
{
    cart = cartridge;
    cpu.connectCartridge(cartridge);
    ppu.connectCartridge(cartridge);
    cart->connectBus(this);
}

void Bus::connectScreen(TFT_eSPI* screen)
{
    ptr_screen = screen;
}

IRAM_ATTR void Bus::renderImage(uint16_t scanline)
{
    int16_t screen_w = ptr_screen->width();
    int16_t screen_h = ptr_screen->height();

    constexpr int16_t nes_w = 256;
    constexpr int16_t nes_h = 240;

    int16_t src_crop_x = (nes_w > screen_w) ? ((nes_w - screen_w) / 2) : 0;
    int16_t src_crop_y = (nes_h > screen_h) ? ((nes_h - screen_h) / 2) : 0;
    int16_t render_w = (screen_w < nes_w) ? screen_w : nes_w;

    int16_t src_start_y = scanline;
    int16_t src_end_y = scanline + SCANLINES_PER_BUFFER;
    int16_t visible_start = src_crop_y;
    int16_t visible_end = src_crop_y + ((screen_h < nes_h) ? screen_h : nes_h);

    if (src_end_y <= visible_start || src_start_y >= visible_end)
        return;

    int16_t copy_start_y = (src_start_y < visible_start) ? visible_start : src_start_y;
    int16_t copy_end_y = (src_end_y > visible_end) ? visible_end : src_end_y;
    int16_t rows = copy_end_y - copy_start_y;

    int16_t src_row_offset = copy_start_y - src_start_y;
    int16_t dest_y = copy_start_y - src_crop_y;
    int16_t dest_x = (screen_w > nes_w) ? ((screen_w - nes_w) / 2) : 0;

    static DMA_ATTR uint16_t render_buffer[SCANLINE_SIZE * SCANLINES_PER_BUFFER];
    for (int16_t row = 0; row < rows; row++)
    {
        uint16_t* src = (uint16_t*)ppu.ptr_display + ((src_row_offset + row) * nes_w) + src_crop_x;
        memcpy(render_buffer + (row * render_w), src, render_w * sizeof(uint16_t));
    }

    #ifndef TFT_PARALLEL
        ptr_screen->pushImageDMA(dest_x, dest_y, render_w, rows, render_buffer);
    #else
        ptr_screen->pushImage(dest_x, dest_y, render_w, rows, render_buffer);
    #endif
} 

IRAM_ATTR void Bus::IRQ()
{
    cpu.IRQ();
}

IRAM_ATTR void Bus::NMI()
{
    cpu.NMI();
}

void Bus::saveState()
{
    if (!SD.exists("/states")) SD.mkdir("/states");
    uint32_t CRC32 = cart->CRC32;

    char CRC32_str[9];
    sprintf(CRC32_str, "%08X", CRC32);

    char filename[32];
    sprintf(filename, "/states/%s.state", CRC32_str);

    File state = SD.open(filename, FILE_WRITE);
    if (!state) return;

    // Header for verification - ANEMOIA + CRC32
    state.print("ANEMOIA");
    state.write((const uint8_t*)CRC32_str, 8);

    // Dump state
    state.write(RAM, sizeof(RAM));
    cpu.dumpState(state);
    ppu.dumpState(state);
    cart->dumpState(state);

    state.close();
}

void Bus::loadState()
{
    uint32_t CRC32 = cart->CRC32;

    char CRC32_str[9];
    sprintf(CRC32_str, "%08X", CRC32);

    char filename[32];
    sprintf(filename, "/states/%s.state", CRC32_str);
    if (!SD.exists(filename)) return;

    File state = SD.open(filename, FILE_READ);
    if (!state) return;

    // Verify header
    char header[8];
    char CRC[9];
    state.read((uint8_t*)&header, 7);
    header[7] = '\0';
    state.read((uint8_t*)&CRC, 8);
    CRC[8] = '\0';

    if (strcmp(header, "ANEMOIA") != 0)
    {
        state.close();
        return;
    }
    if (strcmp(CRC, CRC32_str) != 0)
    {
        state.close();
        return;
    }
    
    // Load state
    state.read(RAM, sizeof(RAM));
    cpu.loadState(state);
    ppu.loadState(state);
    cart->loadState(state);

    state.close();
}
// ===== END src/core/bus.cpp =====

// ===== BEGIN src/controller.cpp =====

uint8_t controllerRead()
{
    uint8_t state = 0;

#if CONTROLLER_TYPE == 0
    if (digitalRead(A_BUTTON)      == LOW) state |= CONTROLLER::A;
    if (digitalRead(B_BUTTON)      == LOW) state |= CONTROLLER::B;
    if (digitalRead(SELECT_BUTTON) == LOW) state |= CONTROLLER::Select;
    if (digitalRead(START_BUTTON)  == LOW) state |= CONTROLLER::Start;
    if (digitalRead(UP_BUTTON)     == LOW) state |= CONTROLLER::Up;
    if (digitalRead(DOWN_BUTTON)   == LOW) state |= CONTROLLER::Down;
    if (digitalRead(LEFT_BUTTON)   == LOW) state |= CONTROLLER::Left;
    if (digitalRead(RIGHT_BUTTON)  == LOW) state |= CONTROLLER::Right;

#elif CONTROLLER_TYPE == 1
    state = NESControllerRead();

#elif CONTROLLER_TYPE == 2
    state = SNESControllerRead();

#elif CONTROLLER_TYPE == 3
    state = PSXControllerRead();

#else
    #error "No controller type selected"
#endif

    return state;
}

bool isDownPressed(CONTROLLER button)
{
    return (controllerRead() & button) != 0;
}

void initController()
{
#if CONTROLLER_TYPE == 0
    pinMode(A_BUTTON, INPUT_PULLUP);
    pinMode(B_BUTTON, INPUT_PULLUP);
    pinMode(LEFT_BUTTON, INPUT_PULLUP);
    pinMode(RIGHT_BUTTON, INPUT_PULLUP);
    pinMode(UP_BUTTON, INPUT_PULLUP);
    pinMode(DOWN_BUTTON, INPUT_PULLUP);
    pinMode(START_BUTTON, INPUT_PULLUP);
    pinMode(SELECT_BUTTON, INPUT_PULLUP);

#elif CONTROLLER_TYPE == 1
    pinMode(CONTROLLER_NES_CLK, OUTPUT);
    pinMode(CONTROLLER_NES_LATCH, OUTPUT);
    pinMode(CONTROLLER_NES_DATA, INPUT);

#elif CONTROLLER_TYPE == 2
    pinMode(CONTROLLER_SNES_CLK, OUTPUT);
    pinMode(CONTROLLER_SNES_LATCH, OUTPUT);
    pinMode(CONTROLLER_SNES_DATA, INPUT);

#elif CONTROLLER_TYPE == 3
    pinMode(CONTROLLER_PSX_DATA, INPUT_PULLUP);
    pinMode(CONTROLLER_PSX_COMMAND, OUTPUT);
    pinMode(CONTROLLER_PSX_ATTENTION, OUTPUT);
    pinMode(CONTROLLER_PSX_CLK, OUTPUT);

    digitalWrite(CONTROLLER_PSX_ATTENTION, HIGH);
    digitalWrite(CONTROLLER_PSX_CLK, HIGH);
    delayMicroseconds(10);

    // Dummy transfer bytes to clean internal controller state
    for (int i = 0; i < 2; i++)
    {
        digitalWrite(CONTROLLER_PSX_ATTENTION, LOW);
        delayMicroseconds(10);

        PSXTransferByte(0);
        delayMicroseconds(10);

        digitalWrite(CONTROLLER_PSX_ATTENTION, HIGH);
        delayMicroseconds(12);
    }

#else
    #error "No controller type selected"
#endif
}

inline uint8_t NESControllerRead()
{
    uint8_t state = 0x00;
    digitalWrite(CONTROLLER_NES_LATCH, HIGH);
    delayMicroseconds(12);
    digitalWrite(CONTROLLER_NES_LATCH, LOW);
    delayMicroseconds(6);

    for (int i = 0; i < 8; i++)
    {
        if (digitalRead(CONTROLLER_NES_DATA) == LOW) state |= (1 << i);
        digitalWrite(CONTROLLER_NES_CLK, LOW);
        delayMicroseconds(6);
        digitalWrite(CONTROLLER_NES_CLK, HIGH);
        delayMicroseconds(6);
    }

    return state;
}

inline uint8_t SNESControllerRead()
{
    // SNES bits
    // 0 - B
    // 1 - Y
    // 2 - Select
    // 3 - Start
    // 4 - Up
    // 5 - Down
    // 6 - Left
    // 7 - Right
    // 8 - A
    // 9 - X
    // 10 - L
    // 11 - R

    uint8_t state = 0x00;
    uint16_t snes_state = 0x0000;
    digitalWrite(CONTROLLER_SNES_LATCH, HIGH);
    delayMicroseconds(12);
    digitalWrite(CONTROLLER_SNES_LATCH, LOW);
    delayMicroseconds(6);

    for (int i = 0; i < 12; i++)
    {
        if (digitalRead(CONTROLLER_SNES_DATA) == LOW) snes_state |= (1 << i);
        digitalWrite(CONTROLLER_SNES_CLK, LOW);
        delayMicroseconds(6);
        digitalWrite(CONTROLLER_SNES_CLK, HIGH);
        delayMicroseconds(6);
    }

    // NES compatible bits
    state |= snes_state & 0xFF;

    // Map extra bits to A and B buttons
    if (snes_state & (1 << 8)) state |= CONTROLLER::A;
    if (snes_state & (1 << 9)) state |= CONTROLLER::B;
    if (snes_state & (1 << 10)) state |= CONTROLLER::B;
    if (snes_state & (1 << 11)) state |= CONTROLLER::A;

    return state;
}

inline uint8_t PSXControllerRead()
{
    /*
    Communication Protocol
    First three bytes - Header
    Following bytes - Digital Mode (2 bytes) / Analog Mode (18 bytes)

    First byte
    Command: 0x01 (indicates new packet)
    Data: 0xFF

    Second byte
    Command: Main command (poll or configure controller)
             Polling: 0x42
    Data: Device Mode
          Upper 4 bits: mode (4 = digital, 7 = analog, F = config)
          Lower 4 bits: how many 16 bit words follow the header

    Third byte
    Command : 0x00
    Data: 0x5A
    */

    // Button Mappings
    /*     
    Digital Mode
    0 - Select
    1 - L3
    2 - R3
    3 - Start
    4 - Up
    5 - Right
    6 - Down
    7 - Left
    8 - L2
    9 - R2
    10 - L1
    11 - R1
    12 - Triangle
    13 - O
    14 - X
    15 - Square

    Analog Mode
    - Analog sticks range 0x00 - 0xFF, 0x7F at rest
    - Pressure buttons range 0x00 - 0xFF, 0xFF is fully pressed
    0-15 - Same as digital mode
    Byte 2 - RX
    Byte 3 - RY
    Byte 4 - LX
    Byte 5 - LY
    Byte 6 - Right
    Byte 7 - Left
    Byte 8 - Up
    Byte 9 - Down
    Byte 10 - Triangle
    Byte 11 - O
    Byte 12 - X
    Byte 13 - Square
    Byte 14 - L1
    Byte 15 - R1
    Byte 16 - L2
    Byte 17 - R2
    */
    int b1, b2;
    uint8_t state = 0x00; 
    uint16_t psx_state = 0x0000;

    // Initiate transfer
    delayMicroseconds(2);
    digitalWrite(CONTROLLER_PSX_ATTENTION, LOW);

    PSXTransferByte(0x01);
	PSXTransferByte(0x42); 
	PSXTransferByte(0xFF); 
    b1 = PSXTransferByte(0xFF);
    b2 = PSXTransferByte(0xFF);

    psx_state = (b2 << 8) | b1;

    // Map PSX bits to NES bits
    constexpr uint16_t PSX_SELECT = (1 << 0);
    constexpr uint16_t PSX_START  = (1 << 3);
    constexpr uint16_t PSX_A_MASK =
        (1 << 11) | // R1
        (1 << 9)  | // R2
        (1 << 2)  | // R3
        (1 << 14) | // X
        (1 << 13);  // O
    constexpr uint16_t PSX_B_MASK =
        (1 << 10) | // L1
        (1 << 8)  | // L2
        (1 << 1)  | // L3
        (1 << 15) | // Square
        (1 << 12);  // Triangle
    constexpr uint16_t PSX_UP    = (1 << 4);
    constexpr uint16_t PSX_DOWN  = (1 << 6);
    constexpr uint16_t PSX_LEFT  = (1 << 7);
    constexpr uint16_t PSX_RIGHT = (1 << 5);

    if (psx_state & PSX_SELECT) state |= CONTROLLER::Select;
    if (psx_state & PSX_START) state |= CONTROLLER::Start;

    if (psx_state & PSX_A_MASK) state |= CONTROLLER::A;
    if (psx_state & PSX_B_MASK) state |= CONTROLLER::B;

    if (psx_state & PSX_UP) state |= CONTROLLER::Up;
    if (psx_state & PSX_DOWN) state |= CONTROLLER::Down;
    if (psx_state & PSX_LEFT) state |= CONTROLLER::Left;
    if (psx_state & PSX_RIGHT) state |= CONTROLLER::Right;

    // End transfer
    digitalWrite(CONTROLLER_PSX_ATTENTION, HIGH);   

    return state;
}

inline uint8_t PSXTransferByte(uint8_t byte)
{
    uint8_t temp = 0;
    for (int i = 0; i < 8; i++)
    {
        digitalWrite(CONTROLLER_PSX_COMMAND, (byte >> i) & 1);    

        digitalWrite(CONTROLLER_PSX_CLK, LOW);

        digitalWrite(CONTROLLER_PSX_CLK, HIGH);
        delayMicroseconds(10);
        if (digitalRead(CONTROLLER_PSX_DATA) == LOW) temp |= (1 << i);
    }

    return temp;
}
// ===== END src/controller.cpp =====

// ===== BEGIN src/ui.cpp =====

UI::UI(TFT_eSPI* screen)
{    
    this->screen = screen;
}

UI::~UI()
{
}

Cartridge* UI::selectGame()
{
    unsigned int last_input_time = 0;
    constexpr unsigned int delay = 250; 
    max_items = (screen->height() - 56) / ITEM_HEIGHT;

    drawWindowBox(2, 20, screen->width() - 4, screen->height() - 40);
    drawBars();
    getNesFiles();
    drawFileList();

    const int size = files.size();
    while (true)
    {
        unsigned int now = millis();

        if (now - last_input_time > delay)
        {
            if (isDownPressed(CONTROLLER::Up)) 
            {
                selected--;
                if (selected < 0)
                {
                    selected = (size - 1);
                    scroll_offset = selected - max_items + 1;
                }
                else if (selected < scroll_offset) scroll_offset = selected; 
                if (scroll_offset < 0) scroll_offset = 0;
                if (scroll_offset > size - 1) scroll_offset = size - 1;
                drawFileList();
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::Down)) 
            {
                selected++; 
                if (selected > (size - 1))
                {
                    selected = 0;
                    scroll_offset = selected;
                }
                else if (selected >= scroll_offset + max_items) scroll_offset = selected - max_items + 1;
                if (scroll_offset < 0) scroll_offset = 0;
                if (scroll_offset > size - 1) scroll_offset = size - 1;
                drawFileList();
                last_input_time = now;
            }
            
        }
        
        if (isDownPressed(CONTROLLER::A) && (selected >= 0 && selected < size))
        {
            Cartridge* cart;
            const char* game = ("/" + files[selected]).c_str();
            std::vector<std::string>().swap(files);
            cart = new Cartridge(game);
            return cart;
        }
    }
}

void UI::getNesFiles()
{
    File root = SD.open("/");
    while (true)
    {
        File file = root.openNextFile();
        if (!file) break;
        if (!file.isDirectory())
        {
            std::string filename = file.name();
            if (filename.rfind(".nes") == filename.size() - 4)
                files.push_back(filename);
        }

        file.close();
    }

    root.close();
}

void UI::drawFileList()
{
    if (prev_selected != selected) 
        screen->fillRect(10, 32, screen->width() - 20, screen->height() - 64, BG_COLOR);

    const int size = files.size();
    for (int i = 0; i < max_items; i++)
    {
        int item = i + scroll_offset;
        if (item >= size) break;

        std::string file = files[item];
        int maxWidth = screen->width() - 28;
        while (screen->textWidth(file.c_str()) > maxWidth)
        {
            file.pop_back();
        }
        if (file.size() < files[item].size())
        {
            file.replace(file.size()-3, 3, "...");
        }

        const char* filename = file.c_str();
        int y = i * ITEM_HEIGHT + 32;
        if (item == selected)
        {
            screen->setTextColor(SELECTED_TEXT_COLOR);
            screen->drawString(filename, 14, y, 1);
        }
        else
        {
            screen->setTextColor(TEXT_COLOR); 
            screen->drawString(filename, 14, y, 1);
        }
    }

    prev_selected = selected;
}

void UI::drawWindowBox(int x, int y, int w, int h) 
{
    screen->drawRect(x, y, w, h, TFT_WHITE);
    screen->drawRect(x+1, y, w-2, h, TFT_WHITE);

    screen->drawRect(x+4, y+3, w-8, h-7, TFT_WHITE);
    screen->drawRect(x+5, y+3, w-10, h-7, TFT_WHITE);

    const char* text1 = " ANEMOIA.CPP ";
    screen->setTextColor(TEXT_COLOR, BG_COLOR);
    screen->setCursor((screen->width() - screen->textWidth(text1)) / 2, 20);
    screen->print(text1);
}

void UI::drawBars() 
{
    // Top bar
    screen->fillRect(0, 0, screen->width(), 16, BAR_COLOR);
    screen->setTextColor(TFT_BLACK, BAR_COLOR);

    const char* text1 = "ANEMOIA-ESP32";
    screen->setCursor((screen->width() - screen->textWidth(text1)) / 2, 4);
    screen->print(text1);

    // Bottom bar
    screen->fillRect(0, screen->height() - 16, screen->width(), 16, BAR_COLOR);
    screen->setTextColor(TFT_BLACK, BAR_COLOR);

    int y = screen->height() - 12;
    int x = 4;

    screen->setTextColor(TEXT2_COLOR, BAR_COLOR);
    screen->setCursor(x, y);
    screen->print("Up/Down");

    screen->setTextColor(TFT_BLACK, BAR_COLOR);
    screen->print(" Move   ");

    screen->setTextColor(TEXT2_COLOR, BAR_COLOR);
    screen->print("A");

    screen->setTextColor(TFT_BLACK, BAR_COLOR);
    screen->print(" Select");
}

void UI::pauseMenu(Bus* nes)
{
    // Black magic stuff
    // Padding bytes for code alignment for better performance
    __attribute__((used, section(".text"), aligned(64)))
    static const uint8_t padding[128] = {0};

    paused = true;
    int prev_select = 0;
    int select = 0;

    screen->endWrite();

    // Draw bars with text
    drawBars();
    const char* text2 = "Pause";
    int text2_x = screen->width() - screen->textWidth(text2) - 12;
    screen->fillRect(text2_x - 4, 0, screen->textWidth(text2) + 8, 16, SELECTED_BG_COLOR);
    drawText(text2, text2_x, 4);

    constexpr int section_count[] = { 3, 2, 1 };
    constexpr const char* items[] = 
    { 
        "Resume", "Settings", "Reset", 
        "Quick Save State", "Quick Load State", 
        "Save and Quit" 
    };
    enum ItemSelect
    {
        Resume,
        Settings,
        Reset,
        QuickSaveState,
        QuickLoadState,
        SaveAndQuit
    };
    constexpr int items_y[] = { 28, 40, 52, 72, 84, 102 };
    constexpr int num_items = sizeof(items) / sizeof(items[0]);
    constexpr int num_sections = sizeof(section_count) / sizeof(section_count[0]);
    constexpr int item_height = 12;
    constexpr int text_height = 8;
    constexpr int text_padding = (item_height - text_height) / 2;

    // Draw pause window 
    constexpr int window_w = 124;
    constexpr int window_h = 104;
    int window_x = screen->width() - window_w;
    constexpr int window_y = 16;
    screen->fillRect(window_x, window_y, window_w, window_h, BAR_COLOR);

    // Draw section borders
    int section_y = window_y + 8;
    for (int s = 0; s < num_sections; s++)
    {
        int w = window_w - 16;
        int h = (section_count[s] * item_height) + 8;
        screen->drawRect(window_x + 8, section_y, w, h, TFT_BLACK);
        screen->drawRect(window_x + 9, section_y, w, h, TFT_BLACK);

        section_y += (h - 1);
    }

    // Draw items
    screen->fillRect(window_x + 10, items_y[0], window_w - 19, item_height, SELECTED_BG_COLOR);
    for (int i = 0; i < num_items; i++)
    {
        int y = items_y[i] + text_padding;
        drawText(items[i], window_x + 12, y);
    }

    constexpr int initial_delay = 500;
    int last_input_time = millis() + initial_delay;
    while (true)
    {
        constexpr int delay = 250; 
        int now = millis();
        if (now - last_input_time > delay)
        {
            if (isDownPressed(CONTROLLER::Up)) 
            {
                select--;
                if (select < 0) select = (num_items - 1);
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::Down)) 
            {
                select++; 
                if (select > (num_items - 1)) select = 0;
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::A)) 
            {
                switch (select)
                {
                case Resume:
                    screen->fillScreen(TFT_BLACK);
                    screen->startWrite();
                    paused = false;
                    return;

                case Settings:
                    settingsMenu(nes);

                    // Redraw pause menu
                    screen->fillRect(window_x, window_y, window_w, window_h, BAR_COLOR);
                    section_y = window_y + 8;
                    for (int s = 0; s < num_sections; s++)
                    {
                        int w = window_w - 16;
                        int h = (section_count[s] * item_height) + 8;
                        screen->drawRect(window_x + 8, section_y, w, h, TFT_BLACK);
                        screen->drawRect(window_x + 9, section_y, w, h, TFT_BLACK);

                        section_y += (h - 1);
                    }
                    screen->fillRect(window_x + 10, items_y[select], window_w - 19, item_height, SELECTED_BG_COLOR);
                    for (int i = 0; i < num_items; i++)
                    {
                        int y = items_y[i] + text_padding;
                        drawText(items[i], window_x + 12, y);
                    }
                    last_input_time = millis() + 500;
                    break;
            
                case Reset:
                    nes->reset();
                    screen->startWrite();
                    paused = false;
                    return;

                case QuickSaveState:
                    nes->saveState();
                    screen->fillScreen(TFT_BLACK);
                    screen->startWrite();
                    paused = false;
                    return;

                case QuickLoadState:
                    nes->loadState();
                    screen->fillScreen(TFT_BLACK);
                    screen->startWrite();
                    paused = false;
                    return;

                case SaveAndQuit:
                    ESP.restart();
                    return;
                }
            }
        }

        // Update Selection
        if (prev_select != select)
        {
            int y;
            // Redraw old selection
            screen->fillRect(window_x + 10, items_y[prev_select], window_w - 19, item_height, BAR_COLOR);
            y = items_y[prev_select] + text_padding;
            drawText(items[prev_select], window_x + 12, y);

            // Draw new selection
            screen->fillRect(window_x + 10, items_y[select], window_w - 19, item_height, SELECTED_BG_COLOR);
            y = items_y[select] + text_padding;
            drawText(items[select], window_x + 12, y);
        }

        prev_select = select;
    }
}

void UI::settingsMenu(Bus* nes)
{
    // Draw settings window 

    int prev_select = 0;
    int select = 0;

    constexpr int window_w = 124;
    constexpr int window_h = 104;
    int window_x = screen->width() - window_w;
    constexpr int window_y = 16;
    screen->fillRect(window_x, window_y, window_w, window_h, BAR_COLOR);

    const char* text2 = "Settings";
    int text2_x = screen->width() - screen->textWidth(text2) - 12;
    screen->fillRect(text2_x - 4, 0, screen->textWidth(text2) + 8, 16, SELECTED_BG_COLOR);
    drawText(text2, text2_x, 4);

    static char volume_text[15];
    static char palette_text[20];
    const char* palette_names[] =
    {
        "NTSC 565",
        "PAL 565",
        "NTSC 222",
        "PAL 222"
    };

    snprintf(volume_text, sizeof(volume_text), "Volume: %d%%", settings.volume);
    snprintf(palette_text, sizeof(palette_text), "Palette: %s", palette_names[settings.palette]);
    char* items[] = 
    { 
        volume_text,
        palette_text,
        "Save & Return"
    };
    enum ItemSelect
    {
        Volume,
        Palette,
        Back
    };
    constexpr int items_y[] = { 30, 42, 54 };
    constexpr int num_items = sizeof(items) / sizeof(items[0]);
    constexpr int item_height = 12;
    constexpr int text_height = 8;
    constexpr int text_padding = (item_height - text_height) / 2;

    screen->drawRect(window_x + 8, window_y + 8, window_w - 16, window_h - 16, TFT_BLACK);
    screen->drawRect(window_x + 9, window_y + 8, window_w - 16, window_h - 16, TFT_BLACK);
    
    screen->fillRect(window_x + 10, items_y[0], window_w - 19, item_height, SELECTED_BG_COLOR);
    for (int i = 0; i < num_items; i++)
    {
        int y = items_y[i] + text_padding;
        drawText(items[i], window_x + 12, y);
    }

    constexpr int initial_delay = 500;
    int last_input_time = millis() + initial_delay;
    while (true)
    {
        constexpr int delay = 250; 
        int now = millis();
        if (now - last_input_time > delay)
        {
            if (isDownPressed(CONTROLLER::Up)) 
            {
                select--;
                if (select < 0) select = (num_items - 1);
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::Down)) 
            {
                select++; 
                if (select > (num_items - 1)) select = 0;
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::Left)) 
            {
                switch (select)
                {
                case Volume:
                    if (settings.volume >= 5) settings.volume -= 5;
                    snprintf(volume_text, sizeof(volume_text), "Volume: %d%%", settings.volume);
                    screen->fillRect(window_x + 10, items_y[Volume], window_w - 19, item_height, SELECTED_BG_COLOR);
                    drawText(items[Volume], window_x + 12, items_y[Volume] + text_padding);
                    break;
                case Palette:
                    if (settings.palette == 0)
                        settings.palette = Ppu2C02::Palette::PaletteCount - 1;
                    else settings.palette--;
                    snprintf(palette_text, sizeof(palette_text), "Palette: %s", palette_names[settings.palette]);
                    screen->fillRect(window_x + 10, items_y[Palette], window_w - 19, item_height, SELECTED_BG_COLOR);
                    drawText(items[Palette], window_x + 12, items_y[Palette] + text_padding);
                    break;
                }
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::Right)) 
            {
                switch (select)
                {
                case Volume:
                    if (settings.volume <= 95) settings.volume += 5;
                    snprintf(volume_text, sizeof(volume_text), "Volume: %d%%", settings.volume);
                    screen->fillRect(window_x + 10, items_y[Volume], window_w - 19, item_height, SELECTED_BG_COLOR);
                    drawText(items[Volume], window_x + 12, items_y[Volume] + text_padding);
                    break;
                case Palette:
                    settings.palette = (settings.palette + 1) % Ppu2C02::Palette::PaletteCount;
                    snprintf(palette_text, sizeof(palette_text), "Palette: %s", palette_names[settings.palette]);
                    screen->fillRect(window_x + 10, items_y[Palette], window_w - 19, item_height, SELECTED_BG_COLOR);
                    drawText(items[Palette], window_x + 12, items_y[Palette] + text_padding);
                    break;
                }
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::A)) 
            {
                switch (select)
                {
                case Back:
                    nes->ppu.setPalette(settings.palette);
                    nes->cpu.apu.setVolume(settings.volume);
                    saveSettings(&settings);
                    return;
                }
            }
        }

        // Update Selection
        if (prev_select != select)
        {
            int y;
            // Redraw old selection
            screen->fillRect(window_x + 10, items_y[prev_select], window_w - 19, item_height, BAR_COLOR);
            y = items_y[prev_select] + text_padding;
            drawText(items[prev_select], window_x + 12, y);
    

            // Draw new selection
            screen->fillRect(window_x + 10, items_y[select], window_w - 19, item_height, SELECTED_BG_COLOR);
            y = items_y[select] + text_padding;
            drawText(items[select], window_x + 12, y);
        }

        prev_select = select;
    }
}

void UI::initializeSettings(Bus* nes)
{
    if (!SD.exists("/settings.bin"))
    {
        Settings temp;
        saveSettings(&temp);
    }
    loadSettings(&settings);
    
    nes->ppu.setPalette(settings.palette);
    nes->cpu.apu.setVolume(settings.volume);
}

void UI::saveSettings(const Settings* s)
{
    File f = SD.open("/settings.bin", FILE_WRITE);
    if (!f) return;
        
    f.seek(0);
    f.write((uint8_t*)s, sizeof(*s));
    f.close();
}

void UI::loadSettings(Settings* s)
{
    File f = SD.open("/settings.bin", FILE_READ);
    if (!f) return;
    if (f.size() != sizeof(Settings)) 
    {
        f.close();
        Settings temp;
        saveSettings(&temp);
        *s = temp;
    }

    f.read((uint8_t*)s, sizeof(*s));
    f.close();
}

void UI::drawText(const char* text, const int x, const int y)
{
    screen->setTextColor(TFT_BLACK);
    screen->setCursor(x, y);
    screen->print(text);
    screen->setCursor(x, y);
    screen->setTextColor(TEXT2_COLOR);
    screen->print(text[0]);
}
// ===== END src/ui.cpp =====

// ===== BEGIN Anemoia-ESP32.ino =====



#ifndef OPTIMIZATION_FLAGS
#error The optimization flags were not applied! Please refer to *Step 4* of the README how to build and upload section.
#endif

TFT_eSPI screen = TFT_eSPI();
SPIClass SD_SPI(HSPI);
UI ui(&screen);
Cartridge* cart;
void setup() 
{
    // Turn off Wifi and Bluetooth to reduce CPU overhead
    #ifdef DEBUG
        Serial.begin(115200);
    #endif
    
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();
    esp_wifi_deinit();
    btStop();
    esp_bt_controller_disable();
    esp_bt_mem_release(ESP_BT_MODE_BTDM);
    esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);

    // Initialize TFT screen
    screen.begin();
    screen.setRotation(SCREEN_ROTATION);
    #ifndef TFT_PARALLEL
        screen.initDMA();
    #endif
    screen.fillScreen(BG_COLOR);
    screen.startWrite();
    screen.setSwapBytes(SCREEN_SWAP_BYTES);

    // Initialize microsd card
    if(!initSD()) while (true);

    // Setup buttons
    initController();
}

void loop() 
{
    cart = ui.selectGame();
    emulate();
}

#ifdef DEBUG
    unsigned long last_frame_time = 0;
    unsigned long current_frame_time = 0;
    unsigned long total_frame_time = 0;
    unsigned long frame_count = 0;
#endif
IRAM_ATTR void emulate()
{
    Bus nes;
    nes.insertCartridge(cart);
    nes.connectScreen(&screen);
    nes.reset();
    ui.initializeSettings(&nes);

    TaskHandle_t apu_task_handle;
    xTaskCreatePinnedToCore(
    apuTask,
    "APU Task",
    768,
    &nes.cpu.apu,
    1,
    &apu_task_handle,
    0
    );

    TaskHandle_t polling_task_handle;
    xTaskCreatePinnedToCore(
    pollingTask,
    "Polling Task",
    768,
    &nes,
    1,
    &polling_task_handle,
    0
    );

    #ifdef DEBUG
        last_frame_time = esp_timer_get_time();
    #endif

    // Target frame time: 16639µs (60.098 FPS)
    #define FRAME_TIME 16639
    uint64_t next_frame = esp_timer_get_time();
    // Emulation Loop
    while (true) 
    {
        // Start + Select opens the pause menu
        if ((nes.controller & CONTROLLER::Start) && (nes.controller & CONTROLLER::Select)) 
        {
            if (!ui.paused)
            {
                ui.pauseMenu(&nes);
                next_frame = esp_timer_get_time() + FRAME_TIME;
                nes.controller = 0;
            }
        }

        // Generate one frame
        nes.clock();

        #ifdef DEBUG
            current_frame_time = esp_timer_get_time();
            total_frame_time += (current_frame_time - last_frame_time);
            frame_count++;

            if ((frame_count & 63) == 0)
            {
                float avg_fps = (1000000.0 * frame_count) / total_frame_time;
                Serial.printf("FPS: %.2f\n", avg_fps);
                total_frame_time = 0;
                frame_count = 0;
            }

            last_frame_time = current_frame_time;
        #endif

        // Frame limiting
        uint64_t now = esp_timer_get_time();
        if (now < next_frame) ets_delay_us(next_frame - now);
        next_frame += FRAME_TIME;
    }
    #undef FRAME_TIME
}

bool initSD() 
{
    Serial.println("Initializing SD...");
    SD_SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (!SD.begin(SD_CS_PIN, SD_SPI, SD_FREQ)) 
    {
        #ifdef DEBUG
            Serial.println("SD Card Mount Failed");
        #endif

        screen.setTextSize(2);
        const char* txt1 = "SD Init failed!";
        const char* txt2 = "Insert SD card or";
        const char* txt3 = "lower SD frequency";
        const char* txt4 = "in config.h";
        int w1 = screen.textWidth(txt1, 2);
        int w2 = screen.textWidth(txt2, 2);
        int w3 = screen.textWidth(txt3, 2);
        int w4 = screen.textWidth(txt4, 2);
        
        int screenWidth = screen.width();
        int screenHeight = screen.height();
        int x1 = (screenWidth - w1) / 2;
        int x2 = (screenWidth - w2) / 2;
        int x3 = (screenWidth - w3) / 2;
        int x4 = (screenWidth - w4) / 2;
        int textStartY = (screenHeight / 2) - 44;

        screen.setTextColor(TFT_BLACK);
        screen.drawString(txt1, x1, textStartY, 2);
        screen.drawString(txt2, x2, textStartY + 24, 2);
        screen.drawString(txt3, x3, textStartY + 48, 2);
        screen.drawString(txt4, x4, textStartY + 72, 2);
        return false;
    }

    return true;
}

void apuTask(void* param) 
{
    Apu2A03* apu = (Apu2A03*)param;

    while (true)
    {
        apu->clock();
    }
}

void pollingTask(void* param)
{
    Bus* nes = (Bus*)param;
    const TickType_t frameTicks = pdMS_TO_TICKS(1000 / 60);
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (true)
    {
        // Read button input
        nes->controller = 0;
        nes->controller = controllerRead();

        vTaskDelayUntil(&lastWakeTime, frameTicks);
    }
    
}
// ===== END Anemoia-ESP32.ino =====
