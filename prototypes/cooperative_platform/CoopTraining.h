#pragma once
#include "CoopPortraitBridge.h"
#include "../../firmware/RPG_POKET_2/Persistence.h"
#include <cstring>
namespace coopplatform {
struct Mac {uint8_t bytes[6]{};};
inline bool sameMac(const Mac&a,const Mac&b){return std::memcmp(a.bytes,b.bytes,6)==0;}
inline bool usableMac(const Mac&m){uint8_t any=0;for(uint8_t b:m.bytes)any|=b;return any&&!(m.bytes[0]&1);}
struct Binding {Mac own{},remote{};uint64_t nonce=0;uint8_t local=0;};
inline bool validBinding(const Binding& b){return b.local<coop::kPlayers&&b.nonce&&usableMac(b.own)&&usableMac(b.remote)&&!sameMac(b.own,b.remote);}
// Conservative training-only gate. Durable spent abilities remain untouched;
// active combat buffs/pending actions are not silently imported or discarded.
inline bool trainingEligible(const rpg::Game& g){
 return rpg::persistentValid(g)&&g.phase==rpg::Phase::Home&&g.p.hp&&
 !g.tripStage&&!g.campStage&&!g.dungeonFlags&&!g.dungeonGenerator&&
 rpg::dailyStateEmpty(g.daily)&&!g.campaignStage&&!g.discovery&&g.eventStage!=2&&
 g.clubStage!=1&&g.clubStage!=2&&!g.guard&&!g.sacredTurns&&!g.turnedTurns&&
 !g.rageTurns&&!g.surgePending&&!g.surgeTurnUsed&&!g.chestLock&&!g.chestPick&&!g.chestTrap;
}
enum class StartError:uint8_t {None,Active,Binding,Profile,Limit};
class Training {
 bool running=false;Binding bound{};uint32_t sessionSerial=0;
 struct Sample {bool valid=false,down=false,playable=false;int32_t x=0,y=0;uint32_t now=0,revision=0,hash=0;uint64_t inputGeneration=0;};
 coop::Queue<Sample,16> samples{};bool sampleOverflow=false;
 uint64_t inputGeneration=0;uint32_t inputHash=0;uint8_t inputFlags=0;
 void syncInput(uint32_t now){
  const uint32_t hash=coop::stateHash(endpoint.state);
  const uint8_t flags=uint8_t(endpoint.hasState)|(uint8_t(endpoint.paused)<<1)|(uint8_t(endpoint.awaitingAck)<<2)|(uint8_t(endpoint.pending)<<3);
  if(hash!=inputHash||flags!=inputFlags){++inputGeneration;inputHash=hash;inputFlags=flags;touch.cancel(ui,endpoint,now);}
  ui.sync(endpoint);
 }
public:
 uint32_t touchDrops=0,staleTouches=0;
 coop::Endpoint endpoint{};coopui::TouchUi ui{};ContactSampler touch{};
 // Both const profiles are pre-admitted harness inputs, NOT a radio handshake.
 // No pointer to either campaign Game is retained, and no commit/save API exists.
 StartError start(const Binding& binding,const rpg::Game& host,const rpg::Game& guest,uint32_t seed,uint32_t now){
  if(running)return StartError::Active;
  if(sessionSerial==UINT32_MAX)return StartError::Limit;
  if(!validBinding(binding))return StartError::Binding;
  if(!trainingEligible(host)||!trainingEligible(guest))return StartError::Profile;
  coop::State initial{};if(!coop::initialize(initial,host,guest,seed)||!coop::validState(initial))return StartError::Profile;
  ++sessionSerial;inputGeneration=0;inputHash=0;inputFlags=0;bound=binding;ui={};touch.reset();samples.clear();sampleOverflow=false;running=true;
  if(!binding.local)endpoint.hostStart(initial,binding.nonce,now);else endpoint.clientStart(binding.nonce,now);
  syncInput(now);return StartError::None;
 }
 bool active()const{return running;}
 uint32_t generationId()const{return sessionSerial;}
 const Binding& binding()const{return bound;}
 void stop(){running=false;bound={};endpoint={};ui={};touch.reset();samples.clear();sampleOverflow=false;}
 void tick(uint32_t now){if(running){endpoint.tick(now);syncInput(now);}}
 // Safe from same-thread FrameBuffer checkpoints: captures only, never resolves.
 // Not an ISR API. Never call dispatchSamples from inside a renderer/checkpoint.
 void captureSample(bool valid,bool down,int32_t px,int32_t py,uint32_t now){
  if(!running)return;
  if(sampleOverflow){++touchDrops;return;}
  if(!samples.push({valid,down,ui.playable(endpoint),px,py,now,endpoint.state.revision,coop::stateHash(endpoint.state),inputGeneration})){
   ++touchDrops;sampleOverflow=true;samples.clear();
  }
 }
 void dispatchSamples(uint32_t now){
  if(!running)return;
  if(sampleOverflow){touch.cancel(ui,endpoint,now);sampleOverflow=false;samples.clear();return;}
  Sample s{};while(samples.pop(s)){
   if(s.inputGeneration!=inputGeneration||!s.playable||!ui.playable(endpoint)||s.revision!=endpoint.state.revision||s.hash!=coop::stateHash(endpoint.state)){
    ++staleTouches;touch.cancel(ui,endpoint,now);continue;
   }
   touch.sample(ui,endpoint,s.valid,s.down,s.x,s.y,s.now);syncInput(now);
  }
 }
 bool receive(const Mac& source,const coop::Packet& packet,uint32_t now){
  if(!running||!sameMac(source,bound.remote))return false;
  coop::Header h{};if(!coop::decode(packet,h)||h.sender!=1-bound.local||h.nonce!=bound.nonce)return false;
  endpoint.receive(h.sender,packet,now);syncInput(now);return true;
 }
};
}
