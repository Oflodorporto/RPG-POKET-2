#pragma once
// Local candidate only. Published builds remain unchanged unless qualification
// explicitly enables this compile flag. Never enable through a saved setting.
#ifndef RPG_ENABLE_COOP_TRAINING
#define RPG_ENABLE_COOP_TRAINING 0
#endif
#if RPG_ENABLE_COOP_TRAINING != 0 && RPG_ENABLE_COOP_TRAINING != 1
#error "RPG_ENABLE_COOP_TRAINING must be 0 or 1"
#endif
