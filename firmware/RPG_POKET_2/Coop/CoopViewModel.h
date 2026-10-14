#pragma once
// Pure state-to-controls/view model. No font, artwork, decoder or pixel canvas.
#include "CoopSession.h"
namespace coopview {
using coopui::Rect;
inline constexpr Rect kExit={266,2,50,18},kHost={16,181,136,38},kAccept=kHost,kDecline={168,181,136,38};
inline constexpr Rect kJoin[4]={{168,50,136,27},{168,80,136,27},{168,110,136,27},{168,140,136,27}};
inline constexpr Rect kArena={3,42,314,106},kEvent={4,150,312,14};
inline constexpr Rect kStats[2]={{4,166,154,31},{162,166,154,31}};
enum class Screen:uint8_t {Lobby,Hosting,Searching,JoinReady,ConfirmPeer,Connecting,Playing,Ended,Unavailable};
enum class EndReason:uint8_t {None,Won,Lost,Left,PeerLeft,LinkLost,Protocol,Ineligible,JoinTimeout};
enum class Control:uint8_t {None,Exit,Host,Join0,Join1,Join2,Join3,Accept,Decline};
enum class LobbyNotice:uint8_t {None,Declined,Busy,PeerExpired,SendFailed};
struct PeerView {bool present=false,available=true;coop::Actor profile{};uint16_t tag=0;};
struct ViewModel {
 Screen screen=Screen::Lobby;EndReason end=EndReason::None;LobbyNotice notice=LobbyNotice::None;
 coop::Actor localProfile{},peerProfile{};PeerView peers[4]{};
 bool hasLocal=false,hasPeer=false,artReady=true,allowHost=true,allowJoin=true,allowAccept=false;
 uint8_t local=0;uint32_t roomCode=0;
};
// Pure presentation adapter: state inspection only, never tick/expire/discover here.
inline ViewModel model(const coopplatform::Session& session,bool artReady){
 ViewModel v;v.artReady=artReady;v.localProfile=session.localProfile();v.hasLocal=v.localProfile.kind==coop::Kind::Player;v.local=v.localProfile.owner;
 v.roomCode=uint32_t(session.sessionToken());const auto* pending=session.pendingPeer();if(pending){v.peerProfile=pending->profile;v.hasPeer=true;}
 for(size_t i=0;i<coopplatform::Session::kMaxPeers;++i)if(const auto* p=session.peer(i)){v.peers[i].present=true;v.peers[i].available=p->available;v.peers[i].profile=p->profile;v.peers[i].tag=uint16_t(p->sessionToken);}
 using S=coopplatform::SessionState;switch(session.state()){
  case S::Closed:v.screen=Screen::Unavailable;break;case S::Browsing:v.screen=Screen::Lobby;break;
  case S::Hosting:v.screen=Screen::Hosting;break;case S::AwaitingAccept:v.screen=Screen::ConfirmPeer;break;
  case S::Joining:case S::Starting:v.screen=Screen::Connecting;break;case S::Playing:v.screen=Screen::Playing;break;case S::Ended:v.screen=Screen::Ended;break;
 }
 v.allowHost=v.allowJoin=session.state()==S::Browsing;v.allowAccept=session.state()==S::AwaitingAccept&&v.hasPeer;
 using E=coopplatform::SessionEnd;switch(session.endReason()){
  case E::None:v.end=EndReason::None;break;case E::Left:v.end=EndReason::Left;break;case E::PeerLeft:v.end=EndReason::PeerLeft;break;
  case E::JoinTimeout:v.end=EndReason::JoinTimeout;break;case E::LinkLost:v.end=EndReason::LinkLost;break;
  case E::Won:v.end=EndReason::Won;break;case E::Lost:v.end=EndReason::Lost;break;case E::Profile:v.end=EndReason::Ineligible;break;case E::Protocol:v.end=EndReason::Protocol;break;
 }
 using N=coopplatform::SessionNotice;switch(session.notice()){
  case N::None:break;case N::Declined:v.notice=LobbyNotice::Declined;break;case N::Busy:v.notice=LobbyNotice::Busy;break;
  case N::PeerExpired:v.notice=LobbyNotice::PeerExpired;break;case N::SendFailed:v.notice=LobbyNotice::SendFailed;break;
 }
 return v;
}
inline Control hitTest(const ViewModel& v,int32_t x,int32_t y){
 if(x<0||x>=320||y<0||y>=240)return Control::None;
 if(kExit.contains(x,y))return Control::Exit;
 if(v.screen==Screen::Lobby||v.screen==Screen::JoinReady||v.screen==Screen::Searching){
  if(v.allowHost&&kHost.contains(x,y))return Control::Host;
  if(v.allowJoin)for(uint8_t i=0;i<4;++i)if(v.peers[i].present&&v.peers[i].available&&kJoin[i].contains(x,y))return Control(uint8_t(Control::Join0)+i);
 }
 if(v.screen==Screen::ConfirmPeer){
  if(v.allowAccept&&v.hasPeer&&kAccept.contains(x,y))return Control::Accept;
  if(kDecline.contains(x,y))return Control::Decline;
 }
 return Control::None;
}
}
