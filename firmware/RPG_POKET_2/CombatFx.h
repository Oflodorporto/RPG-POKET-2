#pragma once
#include <stdint.h>
enum class Effect:uint8_t {None,Slash,Lightning,Shield,Rage,Projectile,Thrust,MagicDarts,FlameVolley,FireBurst,Radiant,DivineSlash};
enum class CombatFeedback:uint8_t {None,Damage,Miss,NoDamage,Heal,Guard,Buff,Skip};
struct CombatResult {
  CombatFeedback kind=CombatFeedback::None;uint16_t amount=0;bool critical=false,enemyAction=false;
  bool injury()const{return kind==CombatFeedback::Damage&&amount>0;}
  bool noHit()const{return kind==CombatFeedback::Miss||kind==CombatFeedback::NoDamage||kind==CombatFeedback::Skip;}
  bool attack()const{return kind==CombatFeedback::Damage||kind==CombatFeedback::Miss||kind==CombatFeedback::NoDamage;}
};
// Presentation only. Rewards and turns are saved before an effect begins.
struct CombatFx {
  Effect kind=Effect::None;bool onHero=false;uint32_t started=0;CombatResult result;
  static constexpr uint32_t duration=480;
  uint32_t length()const{return kind==Effect::DivineSlash?1050:kind==Effect::MagicDarts||kind==Effect::FlameVolley?1100:kind==Effect::FireBurst||kind==Effect::Radiant?1200:kind==Effect::Lightning?1200:kind==Effect::Rage?960:kind==Effect::Projectile?960:kind==Effect::Thrust?640:duration;}
  void start(Effect e,bool hero,uint32_t now,CombatResult resolved={}){kind=e;onHero=hero;started=now;result=resolved;}
  bool active()const{return kind!=Effect::None;}
  bool expire(uint32_t now){if(active()&&uint32_t(now-started)>=length()){kind=Effect::None;return true;}return false;}
  unsigned frame(uint32_t now)const{return uint32_t(now-started)*8/length();}
  unsigned progress(uint32_t now)const{return uint32_t(now-started)>=length()?1000:uint32_t(now-started)*1000/length();}
};
