/**
 * @file gif_player.hpp
 * @brief Declaration of the gif_player class for handling GIF playback functionality.
 *
 * This file contains the definition of the gif_player class, which provides
 * an interface for managing and playing GIF animations.
 */
#include <stdio.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <AnimatedGIF.h>

namespace gify {

// #define ESP32C6
#define ESP32S3

#ifdef ESP32S3
#define TFT_RST   39 // Or set to -1 and connect to Arduino RESET pin
#define TFT_SCLK  40 // Clock out
#define TFT_DC    41
#define TFT_CS    42
#define TFT_MOSI  45 // Data out
#define TFT_BK_LT 48
#else
// #define TFT_RST   21 // Or set to -1 and connect to Arduino RESET pin
// #define TFT_SCLK  7  // Clock out
// #define TFT_DC    15
// #define TFT_CS    14
// #define TFT_MOSI  6 // Data out
// #define TFT_BK_LT 22
#endif

#define HSPI_MISO  5
#define HSPI_MOSI  TFT_MOSI
#define HSPI_SCLK  TFT_SCLK
#define HSPI_SS    TFT_CS

#define SPI_SPEED  100u * 1000u * 1000u
#define FRAME_RATE 18u

#define TFT_W      172
#define TFT_H      320
#define BPP_COOKED 2u

// #define BENCHMARK

/**
 * @class gif_player
 * @brief A class for managing and playing GIF animations.
 *
 * The gif_player class provides functionality to load, control, and display
 * GIF animations. It serves as a core component for handling GIF playback
 * within the gify namespace.
 */
class gif_player
{
  private:
    // Private members for internal use.
    TaskHandle_t _tid;

    struct {
        Adafruit_ST7789* dev;
        SPIClass* spi;
    } _tft;

    static void _run(void* args);
    int32_t _tft_init();

  public:
    /**
     * @brief Constructs a new gif_player object.
     *
     * Initializes the gif_player instance and prepares it for use.
     */
    gif_player();

    /**
     * @brief Destroys the gif_player object.
     *
     * Cleans up resources used by the gif_player instance.
     */
    ~gif_player();
};

} // namespace gify
