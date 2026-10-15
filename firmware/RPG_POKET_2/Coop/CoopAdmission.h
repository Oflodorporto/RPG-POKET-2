#pragma once
#include "CoopTraining.h"
namespace coopplatform {
namespace admission {
// Deliberately distinct from both the frozen PvP v5 packet and coop engine wire.
constexpr uint32_t kMagic=0x414F4F43; // "COOA" little endian.
constexpr uint8_t kVersion=2,kMode=0x54;
constexpr size_t kHeaderBytes=26,kProfileBytes=27,kMaxPacket=96;
enum class Type:uint8_t {Probe,Beacon,Join,Start,Ready,Decline,Busy,Bye,Waiting,Expired};
constexpr size_t kTypeCount=size_t(Type::Expired)+1;
struct Header {Type type=Type::Probe;uint64_t session=0,join=0;uint16_t payload=0;};
struct Packet {uint16_t size=0;uint8_t bytes[kMaxPacket]{};};
inline coop::Writer begin(Packet& p,Type type,uint64_t session,uint64_t join){
 p={};coop::Writer w{p.bytes,kMaxPacket};w.u32(kMagic);w.u8(kVersion);w.u8(kMode);w.u8(uint8_t(type));w.u8(0);
 w.u64(session);w.u64(join);w.u16(0);return w;
}
inline bool finish(Packet& p,coop::Writer& w){
 if(!w.ok||w.n<kHeaderBytes||w.n+4>kMaxPacket)return false;
 size_t payload=w.n-kHeaderBytes;p.bytes[24]=uint8_t(payload);p.bytes[25]=uint8_t(payload>>8);
 w.u32(coop::digest(p.bytes,w.n));p.size=uint16_t(w.n);return w.ok;
}
inline bool decode(const uint8_t* bytes,size_t size,Header& h){
 if(!bytes||size<kHeaderBytes+4||size>kMaxPacket)return false;
 coop::Reader r{bytes,size};if(r.u32()!=kMagic||r.u8()!=kVersion||r.u8()!=kMode)return false;
 uint8_t type=r.u8();if(type>=kTypeCount||r.u8()!=0)return false;
 h.type=Type(type);h.session=r.u64();h.join=r.u64();h.payload=r.u16();
 if(size_t(h.payload)+kHeaderBytes+4!=size)return false;
 size_t expected=0;
 switch(h.type){
  case Type::Beacon:expected=kProfileBytes+1;break;
  case Type::Join:expected=kProfileBytes;break;
  case Type::Start:expected=kProfileBytes+4;break;
  case Type::Ready:case Type::Waiting:expected=4;break;
  default:break;
 }
 if(h.payload!=expected)return false;
 if(h.type==Type::Probe){if(h.session||h.join)return false;}
 else if(!h.session||(h.type==Type::Beacon?h.join!=0:h.join==0))return false;
 coop::Reader tail{bytes+size-4,4};return tail.u32()==coop::digest(bytes,size-4);
}
inline bool sameProfile(const coop::Actor& a,const coop::Actor& b){
 uint8_t one[kProfileBytes]{},two[kProfileBytes]{};coop::Writer x{one,sizeof(one)},y{two,sizeof(two)};
 coop::writeActor(x,a);coop::writeActor(y,b);return x.ok&&y.ok&&std::memcmp(one,two,sizeof(one))==0;
}
} // namespace admission
} // namespace coopplatform
