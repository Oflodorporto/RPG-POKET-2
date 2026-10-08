#pragma once
#include <stdint.h>
enum class Effect:uint8_t {None,Slash,Lightning,Shield,Rage,Projectile,Thrust,MagicDarts,FlameVolley,FireBurst,Radiant};
// Presentation only. Rewards and turns are saved before an effect begins.
struct CombatFx {
  Effect kind=Effect::None;bool onHero=false;uint32_t started=0;
  static constexpr uint32_t duration=480;
  uint32_t length()const{return kind==Effect::MagicDarts||kind==Effect::FlameVolley?1100:kind==Effect::FireBurst||kind==Effect::Radiant?1200:kind==Effect::Lightning?1200:kind==Effect::Rage?960:kind==Effect::Projectile?960:kind==Effect::Thrust?640:duration;}
  void start(Effect e,bool hero,uint32_t now){kind=e;onHero=hero;started=now;}
  bool active()const{return kind!=Effect::None;}
  bool expire(uint32_t now){if(active()&&uint32_t(now-started)>=length()){kind=Effect::None;return true;}return false;}
  unsigned frame(uint32_t now)const{return uint32_t(now-started)*8/length();}
};
