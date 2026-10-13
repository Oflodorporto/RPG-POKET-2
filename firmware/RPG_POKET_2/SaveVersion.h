#pragma once
#include <stdint.h>
namespace rpg {
constexpr unsigned CURRENT_SAVE_FORMAT=29;
// Authorized one-time game reset. Never tie this epoch to a build number.
constexpr uint32_t SAVE_RESET_EPOCH=1;
}
