#include <stdio.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <AnimatedGIF.h>

#include <gifs/this_is_fine.h>

#include <gif_player.hpp>

namespace gify {

TaskHandle_t TaskA, TaskB;

AnimatedGIF gif;
uint8_t ucFrameBuffer[(TFT_W * TFT_H) +
                      ((TFT_W * TFT_H) * BPP_COOKED)]; // holds current canvas as 8-bpp and 16-bit output after that

void gif_player::_run(void* args)
{
    // Cast the argument to the correct type
    gif_player* player = static_cast<gif_player*>(args);
    if (player == nullptr) {
        printf("Error: pointer to base class is NULL\n");
        vTaskDelete(NULL); // Delete the task if player is null
        return;
    }

    // Init ST7789 172x320
    SPIClass hspi(SPI);
    hspi.begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS); // SCLK, MISO, MOSI, SS

    Adafruit_ST7789 tft = Adafruit_ST7789(&hspi, TFT_CS, TFT_DC, TFT_RST);
    tft.init(TFT_W, TFT_H);
    tft.setSPISpeed(SPI_SPEED);
    // tft.setRotation(2);

    pinMode(TFT_BK_LT, OUTPUT);
    analogWrite(TFT_BK_LT, 100);

    uint16_t time = millis();
    tft.fillScreen(ST77XX_BLACK);

    while (1) {
        gif.begin(GIF_PALETTE_RGB565_BE); // Choose the "OLED" version of 1-bpp output (vertical bytes, LSB on top)

        // For 1-bit output, we can pass NULL for the GIFDraw method pointer; it won't be needed
        if (gif.open((uint8_t*)gif_1, sizeof(gif_1), NULL)) {
            // For 1-bit output, the whole frame (8-bit + 16-bit) is written into the buffer we set below
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

gif_player::gif_player()
{
    // Constructor implementation
    printf("gif_player constructor called\n");

    xTaskCreatePinnedToCore(_run,         /* pvTaskCode */
                            "GIF_PLAYER", /* pcName */
                            2048u,        /* usStackDepth */
                            this,         /* pvParameters */
                            0u,           /* uxPriority */
                            &TaskA,       /* pxCreatedTask */
                            0u);          /* xCoreID */
}

gif_player::~gif_player()
{
    // Destructor implementation
    printf("gif_player destructor called\n");
}

} // namespace gify
