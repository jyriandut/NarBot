#pragma once

// clangd compatibility shim for ESP-IDF's xtensa/config/core.h, which includes
// "../hal.h". The Xtensa compiler resolves this through its SDK layout, but
// Apple clangd does not. Keep this out of firmware builds; it is only referenced
// from .clangd.
#include "/Users/jyria/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/include/xtensa/include/xtensa/hal.h"
