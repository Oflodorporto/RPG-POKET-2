#pragma once
#include "CoopModel.h"
namespace coop {
constexpr uint32_t kMagic=0x43504F43; // Separate namespace from ClubProtocol v5.
constexpr uint8_t kWireVersion=1,kMode=0x43;
constexpr size_t kMaxPacket=240,kStateBytes=185,kHeaderBytes=22;
struct Packet {uint16_t size=0;uint8_t bytes[kMaxPacket]{};};
enum class Type:uint8_t { Command, Event, Ack, SnapshotRequest, Snapshot, Heartbeat, Reject };
struct Header {Type type=Type::Heartbeat;uint8_t sender=0;uint64_t nonce=0;uint32_t sequence=0;uint16_t payload=0;};
struct Writer {uint8_t*p;size_t cap,n=0;bool ok=true;void u8(uint8_t v){if(n<cap)p[n++]=v;else ok=false;}void u16(uint16_t v){u8(uint8_t(v));u8(uint8_t(v>>8));}void u32(uint32_t v){u16(uint16_t(v));u16(uint16_t(v>>16));}void u64(uint64_t v){u32(uint32_t(v));u32(uint32_t(v>>32));}};
struct Reader {const uint8_t*p;size_t cap,n=0;bool ok=true;uint8_t u8(){if(n<cap)return p[n++];ok=false;return 0;}uint16_t u16(){uint16_t a=u8();return uint16_t(a|uint16_t(u8())<<8);}uint32_t u32(){uint32_t a=u16();return a|uint32_t(u16())<<16;}uint64_t u64(){uint64_t a=u32();return a|uint64_t(u32())<<32;}};
inline uint32_t digest(const uint8_t*p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i){h^=p[i];h*=16777619u;}return h;}
inline void writeActor(Writer&w,const Actor&a){w.u8(a.id);w.u8(a.owner);w.u8(uint8_t(a.kind));w.u8(a.species);w.u8(a.cls);w.u8(a.level);w.u8(a.race);w.u8(a.shirt);w.u8(a.trousers);w.u16(a.hp);w.u16(a.maxhp);w.u16(a.mp);w.u16(a.maxmp);w.u16(a.attack);w.u16(a.defense);w.u8(a.guard);w.u8(a.criticalChance);w.u8(a.attacks);w.u8(a.fatiguePenalty);w.u8(a.life);w.u8(a.mana);}
inline Actor readActor(Reader&r){Actor a{};a.id=r.u8();a.owner=r.u8();a.kind=Kind(r.u8());a.species=r.u8();a.cls=r.u8();a.level=r.u8();a.race=r.u8();a.shirt=r.u8();a.trousers=r.u8();a.hp=r.u16();a.maxhp=r.u16();a.mp=r.u16();a.maxmp=r.u16();a.attack=r.u16();a.defense=r.u16();a.guard=r.u8();a.criticalChance=r.u8();a.attacks=r.u8();a.fatiguePenalty=r.u8();a.life=r.u8();a.mana=r.u8();return a;}
inline void writeState(Writer&w,const State&s){w.u8(s.rules);w.u8(uint8_t(s.phase));w.u8(s.wave);w.u16(s.round);w.u8(s.turn);w.u8(s.players);w.u32(s.revision);w.u32(s.rng);for(uint32_t n:s.accepted)w.u32(n);for(const Actor&a:s.actors)writeActor(w,a);}
inline State readState(Reader&r){State s{};s.rules=r.u8();s.phase=Phase(r.u8());s.wave=r.u8();s.round=r.u16();s.turn=r.u8();s.players=r.u8();s.revision=r.u32();s.rng=r.u32();for(uint32_t&n:s.accepted)n=r.u32();for(Actor&a:s.actors)a=readActor(r);return s;}
inline uint32_t stateHash(const State&s){uint8_t bytes[kStateBytes]{};Writer w{bytes,sizeof(bytes)};writeState(w,s);return w.ok&&w.n==sizeof(bytes)?digest(bytes,w.n):0;}
inline Writer begin(Packet&p,Type type,uint8_t sender,uint64_t nonce,uint32_t seq){p={};Writer w{p.bytes,kMaxPacket};w.u32(kMagic);w.u8(kWireVersion);w.u8(kMode);w.u8(uint8_t(type));w.u8(sender);w.u64(nonce);w.u32(seq);w.u16(0);return w;}
inline bool finish(Packet&p,Writer&w){if(!w.ok||w.n+4>kMaxPacket)return false;size_t payload=w.n-kHeaderBytes;p.bytes[20]=uint8_t(payload);p.bytes[21]=uint8_t(payload>>8);uint32_t h=digest(p.bytes,w.n);w.u32(h);p.size=uint16_t(w.n);return w.ok;}
inline bool decode(const Packet&p,Header&h){if(p.size<kHeaderBytes+4||p.size>kMaxPacket)return false;Reader r{p.bytes,p.size};if(r.u32()!=kMagic||r.u8()!=kWireVersion||r.u8()!=kMode)return false;h.type=Type(r.u8());h.sender=r.u8();h.nonce=r.u64();h.sequence=r.u32();h.payload=r.u16();if(uint8_t(h.type)>uint8_t(Type::Reject)||h.sender>=kPlayers||!h.nonce||size_t(h.payload)+kHeaderBytes+4!=p.size)return false;Reader tail{p.bytes+p.size-4,4};return tail.u32()==digest(p.bytes,p.size-4);}
inline Reader payloadReader(const Packet&p,const Header&h){return Reader{p.bytes+kHeaderBytes,h.payload};}
inline void writeCommand(Writer&w,const Command&c){w.u32(c.revision);w.u8(c.actor);w.u8(c.target);w.u8(uint8_t(c.action));}
inline Command readCommand(Reader&r,uint32_t seq){Command c{};c.sequence=seq;c.revision=r.u32();c.actor=r.u8();c.target=r.u8();c.action=Action(r.u8());return c;}
inline void writeEffect(Writer&w,const Effect&e){w.u32(e.beforeHash);w.u32(e.commandSequence);w.u8(e.peer);w.u8(e.actor);w.u8(e.target);w.u8(uint8_t(e.action));w.u16(e.amount);w.u8(e.critical?1:0);w.u8(e.fromWave);}
inline Effect readEffect(Reader&r){Effect e{};e.beforeHash=r.u32();e.commandSequence=r.u32();e.peer=r.u8();e.actor=r.u8();e.target=r.u8();e.action=Action(r.u8());e.amount=r.u16();uint8_t critical=r.u8();if(critical>1)r.ok=false;e.critical=critical!=0;e.fromWave=r.u8();return e;}
static_assert(kHeaderBytes+16+kStateBytes+4<=kMaxPacket,"Event exceeds conservative prototype payload budget");
template<typename T,size_t N> struct Queue {T data[N]{};size_t head=0,count=0;uint32_t overflow=0;bool push(const T&x){if(count==N){++overflow;return false;}data[(head+count)%N]=x;++count;return true;}bool pop(T&x){if(!count)return false;x=data[head];head=(head+1)%N;--count;return true;}void clear(){head=count=0;}};
}
