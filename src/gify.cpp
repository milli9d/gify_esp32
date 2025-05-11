#include <thread>
#include <gif_player.hpp>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/**
 * @brief Main functions for the application.
 *
 * This function initializes a `gif_player` instance and then suspends
 * the main task to prevent further execution. The suspension ensures
 * that the main task does not consume CPU resources unnecessarily.
 * 
 * Note: The `vTaskSuspend(NULL)` call is used to suspend the current
 * task, which in this case is the main task.
 */
void setup(void) {
  /* initialize GIF player task */
  gify::gif_player gif_player;

  /* suspend the main task */
  vTaskSuspend(NULL); // Suspend the main task
}

void loop() {
  // Nothing to do here
}
