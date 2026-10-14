#include "NvsPersistence.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <type_traits>
#include <vector>
using namespace rpg;

using Bytes=std::vector<uint8_t>;
enum class Type {Blob,U8,U32,String};
struct Value {
  Type type=Type::Blob;Bytes bytes;
  bool operator==(const Value& other)const{return type==other.type&&bytes==other.bytes;}
};
using Namespace=std::map<std::string,Value>;
using Image=std::map<std::string,Namespace>;
struct Call {std::string operation,name,key;};
enum class Fault {None,Io,LostAck,NoEffect,ShortRead,Missing};
struct PowerLoss {};

// Unlike a map-only Store mock, this fake implements SDK handles, typed keys,
// size probes, pending writes, explicit commit, failed/lost acknowledgements and
// close discarding uncommitted changes. Tests run the exact production adapter.
struct FakeNvsDisk {
  struct Session {std::string name;bool writable=false;Namespace pending;std::vector<std::string> erased;};
  Image image;
  std::map<unsigned,Session> handles;
  unsigned nextHandle=1;
  std::vector<Call> calls;
  std::string faultOperation,faultNamespace,faultKey;
  Fault fault=Fault::None;
  int faultAt=-1,cut=-1,boundary=0;
  void checkpoint(){if(boundary++==cut)throw PowerLoss{};}
  Fault begin(const char* operation,const std::string& name,const std::string& key=""){
    checkpoint();calls.push_back({operation,name,key});
    const bool selected=faultAt>=0?int(calls.size())-1==faultAt:
      operation==faultOperation&&(faultNamespace.empty()||name==faultNamespace)&&
      (faultKey.empty()||key==faultKey);
    return selected?fault:Fault::None;
  }
  NvsStatus end(NvsStatus result){checkpoint();return result;}
  void disarm(){fault=Fault::None;faultAt=cut=-1;boundary=0;faultOperation.clear();faultNamespace.clear();faultKey.clear();calls.clear();handles.clear();}
  void fail(const char* op,const char* ns,const char* key,Fault mode){
    disarm();faultOperation=op;faultNamespace=ns;faultKey=key;fault=mode;
  }
};

struct FakeNvsDriver {
  using Handle=unsigned;
  FakeNvsDisk* disk=nullptr;
  explicit FakeNvsDriver(FakeNvsDisk& value):disk(&value){}
  NvsStatus open(const char* name,bool write,Handle& handle){
    const auto fault=disk->begin(write?"openWrite":"openRead",name);
    if(fault==Fault::Io)return disk->end(NvsStatus::Error);
    if(!disk->image.count(name)){
      if(!write)return disk->end(NvsStatus::Missing);
      disk->image[name]={};
    }
    handle=disk->nextHandle++;disk->handles[handle]={name,write,{}, {}};
    return disk->end(NvsStatus::Ok);
  }
  void close(Handle handle){disk->handles.erase(handle);}
  const Value* find(Handle handle,const char* key){
    auto& session=disk->handles.at(handle);
    auto pending=session.pending.find(key);if(pending!=session.pending.end())return &pending->second;
    for(const auto& erased:session.erased)if(erased==key)return nullptr;
    const auto& space=disk->image.at(session.name);auto value=space.find(key);
    return value==space.end()?nullptr:&value->second;
  }
  NvsStatus getBlob(Handle handle,const char* key,void* out,size_t& length){
    const auto fault=disk->begin(out?"readBlob":"probeBlob",disk->handles.at(handle).name,key);
    if(fault==Fault::Io)return disk->end(NvsStatus::Error);
    if(fault==Fault::Missing)return disk->end(NvsStatus::Missing);
    const auto* value=find(handle,key);
    if(!value)return disk->end(NvsStatus::Missing);
    if(value->type!=Type::Blob)return disk->end(NvsStatus::TypeMismatch);
    if(!out){length=value->bytes.size();return disk->end(NvsStatus::Ok);}
    if(length<value->bytes.size()){length=value->bytes.size();return disk->end(NvsStatus::Error);}
    length=value->bytes.size();if(fault==Fault::ShortRead&&length)--length;
    if(length)memcpy(out,value->bytes.data(),length);
    return disk->end(fault==Fault::LostAck?NvsStatus::Error:NvsStatus::Ok);
  }
  NvsStatus getU8(Handle handle,const char* key,uint8_t& value){
    const auto fault=disk->begin("readU8",disk->handles.at(handle).name,key);
    if(fault==Fault::Io)return disk->end(NvsStatus::Error);
    const auto* found=find(handle,key);
    if(!found)return disk->end(NvsStatus::Missing);
    if(found->type!=Type::U8)return disk->end(NvsStatus::TypeMismatch);
    assert(found->bytes.size()==1);value=found->bytes[0];return disk->end(NvsStatus::Ok);
  }
  NvsStatus setBlob(Handle handle,const char* key,const void* bytes,size_t length){
    auto& session=disk->handles.at(handle);assert(session.writable);
    const auto fault=disk->begin("setBlob",session.name,key);
    if(fault==Fault::Io)return disk->end(NvsStatus::Error);
    if(fault!=Fault::NoEffect){const auto* begin=static_cast<const uint8_t*>(bytes);session.pending[key]={Type::Blob,Bytes(begin,begin+length)};}
    return disk->end(fault==Fault::LostAck?NvsStatus::Error:NvsStatus::Ok);
  }
  NvsStatus setU8(Handle handle,const char* key,uint8_t value){
    auto& session=disk->handles.at(handle);assert(session.writable);
    const auto fault=disk->begin("setU8",session.name,key);
    if(fault==Fault::Io)return disk->end(NvsStatus::Error);
    if(fault!=Fault::NoEffect)session.pending[key]={Type::U8,{value}};
    return disk->end(fault==Fault::LostAck?NvsStatus::Error:NvsStatus::Ok);
  }
  NvsStatus eraseKey(Handle handle,const char* key){
    auto& session=disk->handles.at(handle);assert(session.writable);
    const auto fault=disk->begin("erase",session.name,key);
    if(fault==Fault::Io)return disk->end(NvsStatus::Error);
    if(!find(handle,key))return disk->end(NvsStatus::Missing);
    if(fault!=Fault::NoEffect)session.erased.push_back(key);
    return disk->end(fault==Fault::LostAck?NvsStatus::Error:NvsStatus::Ok);
  }
  NvsStatus commit(Handle handle){
    auto& session=disk->handles.at(handle);assert(session.writable);
    const auto fault=disk->begin("commit",session.name);
    if(fault==Fault::Io)return disk->end(NvsStatus::Error);
    if(fault!=Fault::NoEffect){
      auto& space=disk->image[session.name];
      for(const auto& erased:session.erased)space.erase(erased);
      for(const auto& value:session.pending)space[value.first]=value.second;
    }
    session.pending.clear();session.erased.clear();
    return disk->end(fault==Fault::LostAck?NvsStatus::Error:NvsStatus::Ok);
  }
};
