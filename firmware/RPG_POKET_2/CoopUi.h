#pragma once
#include "CoopConfig.h"
#if RPG_ENABLE_COOP_TRAINING
#include <cstdint>
// Small out-of-line runtime bridge. Rendering/art remain in the original
// sketch translation unit; this facade needs no complete Session definition.
namespace rpg {struct Game;}
namespace coopplatform {class Session;class Input;}
namespace coopradio {class Device;}
extern coopplatform::Session coopSession;
extern coopplatform::Input coopInput;
extern coopradio::Device coopDevice;
extern bool coopClosing;
namespace coopruntime {
struct TickResult {bool activity=false,closed=false,restoreOk=true;};
bool active();
bool open(const rpg::Game& game,bool blocked,uint64_t entropy,uint32_t now);
void capture(bool valid,bool down,int x,int y,uint32_t now);
TickResult tick(uint32_t now);
bool close(); // Immediate, idempotent control cleanup; returns Wi-Fi restore OK.
}
#endif
