#pragma once
#include "wifi_radio_fake/Arduino.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wswitch"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "SceneCanvas.h"
#pragma GCC diagnostic pop
#include "../firmware/RPG_POKET_2/TouchGate.h"
rpg::Game game;ViewState view;TouchGate gate;CombatFx combatFx;
bool dirty=true,pendingTouch=false;uint32_t lastActivity=0;

inline unsigned updaterTicks=0;
inline void tickUpdater(){++updaterTicks;}
#include "../firmware/RPG_POKET_2/WifiRadioTestRuntime.h"
inline void resetRuntime(){using namespace wifi_radio_ui;if(device.active())assert(device.close());device.~Device();new (&device) wifi_radio_test::Device;fake::reset();fake::sendObserver=nullptr;WiFi.persistent(true);receiver=nullptr;senderCallback=nullptr;view=ViewState{};view.page=Page::NetworkTest;menu=MenuState{};updateInfo=updater::Info{};arena.opened=false;combatFx=CombatFx{};reportOpen=restorePending=false;exportNotice[0]=0;exportFingerprint=0;manualLevel=wifi_radio_test::Level::N1;manualDistance=wifi_radio_test::Distance::TwoPalms;previousState=wifi_radio_test::State::Closed;dirty=true;}
inline void tapAction(wifi_radio_ui::Action action,unsigned value=0){const auto snapshot=wifi_radio_ui::screen;for(unsigned i=0;i<snapshot.buttonCount;++i){const auto& b=snapshot.buttons[i];if(b.action==action&&b.value==value&&b.enabled){wifiRadioTestTap(b.x+b.w/2,b.y+b.h/2);return;}}assert(!"Action missing or disabled");}
