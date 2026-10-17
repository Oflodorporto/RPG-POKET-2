#pragma once
#include "WifiRadioRuntimeHarness.h"
#include <deque>
#include <vector>
struct RuntimeNetwork {
 struct Packet{bool fromDevice;std::vector<uint8_t> bytes;};
 wifi_radio_test::Flow peer;std::deque<Packet> queue;bool dropBaseline=false;int8_t peerPower=80;uint8_t peerMac[6]={2,0,0,0,0,2};
 static inline RuntimeNetwork* active=nullptr;
 RuntimeNetwork(){active=this;fake::sendObserver=sendDevice;wifi_radio_test::Hooks h;h.context=this;h.nowMs=[](void*){return millis();};h.send=sendPeer;h.applyPower=[](void*p,int8_t raw){auto& n=*static_cast<RuntimeNetwork*>(p);n.peerPower=raw;return wifi_radio_test::PowerResult{raw,raw,true,0};};h.restorePower=[](void*p){auto& n=*static_cast<RuntimeNetwork*>(p);n.peerPower=80;return wifi_radio_test::PowerResult{80,80,true,0};};peer.setHooks(h);peer.setRadioConfig(6,7,1);assert(peer.begin(peerMac,222,{80,80,true,0},80,millis()));}
 ~RuntimeNetwork(){fake::sendObserver=nullptr;active=nullptr;}
 static int sendDevice(const uint8_t*,const uint8_t*b,size_t n){assert(active);active->queue.push_back({true,std::vector<uint8_t>(b,b+n)});return 0;}
 static int32_t sendPeer(void*p,const uint8_t*,const uint8_t*b,size_t n){static_cast<RuntimeNetwork*>(p)->queue.push_back({false,std::vector<uint8_t>(b,b+n)});return 0;}
 void step(){fake::timeMs+=10;wifiRadioTestTick(millis());peer.tick(millis());unsigned fence=0;while(!queue.empty()){assert(++fence<1000);auto p=queue.front();queue.pop_front();if(dropBaseline&&(fake::power==80||peerPower==80))continue;if(p.fromDevice)peer.receive(wifi_radio_ui::device.localMac(),p.bytes.data(),p.bytes.size(),-48,true,millis());else{wifi_pkt_rx_ctrl_t ctrl;ctrl.rssi=-44;esp_now_recv_info_t info;info.src_addr=peerMac;info.rx_ctrl=&ctrl;auto callback=receiver.load();if(callback)callback(&info,p.bytes.data(),int(p.bytes.size()));}}}
 void advance(unsigned ms){for(unsigned i=0;i<ms;i+=10)step();}
 void pair(){advance(30);assert(wifi_radio_ui::device.flow().peerCount()==1);tapAction(wifi_radio_ui::Action::Invite);advance(30);assert(peer.state()==wifi_radio_test::State::IncomingInvite);assert(peer.acceptInvite(millis()));advance(30);assert(wifi_radio_ui::device.flow().state()==wifi_radio_test::State::DistancePause);}
 void report(size_t count){unsigned bound=0;while(wifi_radio_ui::device.flow().reportCount()<count&&bound++<4000)step();assert(wifi_radio_ui::device.flow().reportCount()>=count);}
};
