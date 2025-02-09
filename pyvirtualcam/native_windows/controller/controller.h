#pragma once

#include <stdint.h>

bool virtual_output_start(int width, int height, double fps, int delay, int channel);
void virtual_output_stop();
void virtual_video(uint8_t **data, int channel);
bool virtual_output_is_running();
