#pragma once
// Package 12 enables experimental training in production Arduino builds.
// Standalone legacy host harnesses stay off; integrated host gates explicitly
// compile flag 1 and link the actual runtime. No saved setting changes the flag.
#ifndef RPG_ENABLE_COOP_TRAINING
#ifdef ARDUINO
#define RPG_ENABLE_COOP_TRAINING 1
#else
#define RPG_ENABLE_COOP_TRAINING 0
#endif
#endif
#if RPG_ENABLE_COOP_TRAINING != 0 && RPG_ENABLE_COOP_TRAINING != 1
#error "RPG_ENABLE_COOP_TRAINING must be 0 or 1"
#endif
