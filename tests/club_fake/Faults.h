#pragma once
namespace clubFake {
inline int openFail=0,closeFail=0;
inline int initCalls=0,deinitCalls=0,unregisterCalls=0,reconnectCalls=0;
inline bool radio=false,connected=false,persistent=true,autoReconnect=true;
inline int mode=0,channel=1,storage=1;
inline void reset(){openFail=closeFail=initCalls=deinitCalls=unregisterCalls=reconnectCalls=0;radio=connected=false;persistent=autoReconnect=true;mode=0;channel=storage=1;}
}
