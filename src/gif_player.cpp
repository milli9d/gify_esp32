#include <stdio.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <AnimatedGIF.h>

#include <gif_player.hpp>

#include <gifs/this_is_fine.h>

namespace gify {

/**
 * @brief Task function to play GIF animations on a TFT display.
 *
 * This function is designed to run as a FreeRTOS task. It initializes an
 * AnimatedGIF object, processes GIF frames, and renders them on a TFT display
 * at a controlled frame rate. The function uses a static framebuffer to store
 * the GIF data and ensures proper synchronization with the display hardware.
 *
 * @param args Pointer to the gif_player instance. This is used to access the
 *             associated TFT display and other resources. If the pointer is
 *             null, the task will terminate.
 *
 * The function performs the following steps:
 * - Initializes the AnimatedGIF object and framebuffer.
 * - Opens a GIF file from memory and retrieves its metadata.
 * - Plays the GIF frame by frame, rendering each frame on the TFT display.
 * - Ensures the playback adheres to the calculated or predefined frame rate.
 * - Handles errors such as null pointers or frames taking too long to render.
 *
 * Notes:
 * - The function assumes the GIF data is stored in a global variable `gif_1`.
 * - The frame rate can be controlled using the `FRAME_RATE` macro. If not
 *   defined, the average frame delay is calculated from the GIF metadata.
 * - The function uses FreeRTOS APIs such as `vTaskDelete` and `vTaskDelay`.
 *
 * Example usage:
 * This function is intended to be used as a FreeRTOS task:
 * @code
 * xTaskCreate(gif_player::_run, "GIF Player Task", stackSize, &player, priority, NULL);
 * @endcode
 */
void gif_player::_run(void* args)
{
    static AnimatedGIF gif;
    static uint8_t ucFrameBuffer[(TFT_W * TFT_H) + ((TFT_W * TFT_H) * BPP_COOKED)];

    // Cast the argument to the correct type
    gif_player* player = static_cast<gif_player*>(args);
    if (player == nullptr) {
        printf("Error: pointer to base class is NULL\n");
        vTaskDelete(NULL); // Delete the task if player is null
        return;
    }

    Adafruit_ST7789& tft = *player->_tft.dev;

    while (1) {
        gif.begin(GIF_PALETTE_RGB565_BE); // Choose the "OLED" version of 1-bpp
                                          // output (vertical bytes, LSB on top)

        // For 1-bit output, we can pass NULL for the GIFDraw method pointer; it
        // won't be needed
        if (gif.open((uint8_t*)gif_1, sizeof(gif_1), NULL)) {
            // For 1-bit output, the whole frame (8-bit + 16-bit) is written
            // into the buffer we set below
            gif.setFrameBuf(ucFrameBuffer);
            // We want the library to generate ready-made (COOKED) pixels
            gif.setDrawType(GIF_DRAW_COOKED);
            printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
            GIFINFO info = {};
            gif.getInfo(&info);
            printf("Number of frames = %d\n"
                   "Duration [ms] = %d\n"
                   "Max Delay [ms] = %d]\n"
                   "Min Delay [ms] = %d]\n",
                   info.iFrameCount, info.iDuration, info.iMaxDelay, info.iMinDelay);

#ifndef FRAME_RATE
            uint64_t avg_frame_delay_us = ((info.iMaxDelay + info.iMinDelay) / 2u) * 1000u;
#else
            uint64_t avg_frame_delay_us = 1000000u / FRAME_RATE;
#endif

            int count = 0;
            while (gif.playFrame(false, NULL)) { // live dangerously; run unthrottled :)
                uint64_t start_time_us = micros();
                uint16_t* current_frame = (uint16_t*)(ucFrameBuffer + (TFT_W * TFT_H));
                tft.startWrite();
                tft.setAddrWindow(0, 0, TFT_W, TFT_H);
                tft.writePixels(current_frame, (TFT_W * TFT_H), false, true);
                tft.endWrite();
                count++;

#ifdef BENCHMARK
                // count average frames per second
                uint64_t elapsed_time_us = micros() - start_time_us;
                if (elapsed_time_us > 0) {
                    uint64_t fps = 1000000u / elapsed_time_us;
                    printf("Frame %d: %d us, %lu fps\n", count, elapsed_time_us, fps);
                }
                continue;
#endif

                // make things run at proper frame rate
                if (micros() - start_time_us > avg_frame_delay_us) {
                    printf("Frame %d took too long to draw: %d us\n", count, micros() - start_time_us);
                    continue;
                }
                while (micros() - start_time_us < avg_frame_delay_us) {
                    vTaskDelay(1u / portTICK_PERIOD_MS);
                }
            }
            gif.close();
        }
    }
}

/**
 * @brief Initializes the TFT display using the High Speed SPI bus.
 *
 * This function sets up the SPI bus, initializes the ST7789 TFT display,
 * and configures its parameters such as resolution and SPI speed. It also
 * sets the backlight pin to output mode and adjusts the brightness.
 *
 * @return int32_t Returns 0 on successful initialization, or -1 if the
 *         TFT device creation fails.
 *
 * @note The function assumes that the necessary pin definitions (e.g.,
 *       HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS, TFT_CS, TFT_DC, TFT_RST,
 *       TFT_BK_LT) and constants (e.g., TFT_W, TFT_H, SPI_SPEED) are
 *       defined elsewhere in the code.
 */
int32_t gif_player::_tft_init()
{
    /* initialize High Speed SPI bus */
    _tft.spi = new SPIClass(HSPI);
    _tft.spi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);

    /* initialize the ST7789 TFT */
    _tft.dev = new Adafruit_ST7789(_tft.spi, TFT_CS, TFT_DC, TFT_RST);
    if (_tft.dev == nullptr) {
        printf("Error: failed to create TFT device\n");
        return -1;
    }

    _tft.dev->init(TFT_W, TFT_H);
    _tft.dev->setSPISpeed(SPI_SPEED);
    // _tft.dev->setRotation(2);

    pinMode(TFT_BK_LT, OUTPUT);
    analogWrite(TFT_BK_LT, 100);

    uint16_t time = millis();
    _tft.dev->fillScreen(ST77XX_BLACK);

    return 0;
}

gif_player::gif_player()
{
    // Constructor implementation
    int rc = _tft_init();
    if (rc != 0) {
        printf("Error: failed to initialize TFT\n");
        return;
    }

    // Create the task for running the GIF player
    // The task will run on core 0
    xTaskCreatePinnedToCore(_run,         /* pvTaskCode */
                            "GIF_PLAYER", /* pcName */
                            2048u,        /* usStackDepth */
                            this,         /* pvParameters */
                            1u,           /* uxPriority */
                            &_tid,        /* pxCreatedTask */
                            0u);          /* xCoreID */
}

/**
 * @brief Destructor for the gif_player class.
 *
 * This destructor is responsible for cleaning up resources used by the gif_player object.
 * It ensures proper deallocation of dynamically allocated memory and tasks.
 *
 * - Deletes the `_tft.dev` object if it is not null and sets it to null.
 * - Deletes the `_tft.spi` object if it is not null and sets it to null.
 * - Deletes the FreeRTOS task associated with `_tid` if it is not null and sets it to null.
 * - Prints a message indicating that the gif_player object has been destroyed.
 *
 * Ensure that no other parts of the program attempt to access these resources after
 * the destructor has been called.
 */
gif_player::~gif_player()
{
    // Destructor implementation
    if (_tft.dev != nullptr) {
        delete _tft.dev;
        _tft.dev = nullptr;
    }
    if (_tft.spi != nullptr) {
        delete _tft.spi;
        _tft.spi = nullptr;
    }
    if (_tid != nullptr) {
        vTaskDelete(_tid);
        _tid = nullptr;
    }
    printf("gif_player destroyed\n");
    // Free any other resources if needed
}

} // namespace gify
