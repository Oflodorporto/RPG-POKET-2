#pragma once
#include "ClubProtocol.h"
#include "ClubDuel.h"
struct ClubUi {club::Session session;club::Duel duel;bool opened=false,begun=false;int choice=0,result=0;uint32_t finishedAt=0,redraw=0;const char* notice="";};
inline ClubUi arena;
