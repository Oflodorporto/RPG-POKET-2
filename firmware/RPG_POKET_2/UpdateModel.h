#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ReleaseVersion.h"
#include "SaveVersion.h"
namespace updater {
enum class State:uint8_t {Idle,Checking,Current,Available,Downloading,Verifying,Installing,Cleaning,Restarting,Error,Testing};
struct Info {State state=State::Idle;unsigned progress=0;bool busy=false,artOnly=false,canResume=false,tested=false;uint8_t netTrials=0,netSuccess=0,netDrop=0;int minDbm=-100,maxDbm=-100;uint32_t avgMs=0;char version[40]="",message[39]="";};
struct Package {char url[240]="",sha[65]="";uint32_t bytes=0;};
struct Plan {char version[40]="";uint32_t build=0,artCrc=0,assets=0,saveFormat=0,writesSaveFormat=0;bool readsCurrentSaves=false;Package firmware,art;};
inline bool hashValid(const char* s){if(strlen(s)!=64)return false;for(unsigned i=0;i<64;++i)if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')))return false;return true;}
inline bool versionValid(const char* s){size_t n=strlen(s);if(!n||n>=40)return false;for(size_t i=0;i<n;++i)if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='z')||s[i]=='.'||s[i]=='-'))return false;return true;}
inline bool releaseUrl(const char* s){const char* prefix="https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v";if(strncmp(s,prefix,strlen(prefix)))return false;for(const char* p=s+strlen(prefix);*p;++p)if(!((*p>='0'&&*p<='9')||(*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||*p=='.'||*p=='-'||*p=='/'||*p=='_'))return false;return true;}
inline bool valid(const Plan& p,uint32_t partitionBytes,uint32_t psramBytes){return versionValid(p.version)&&p.build>=FW_BUILD&&p.saveFormat==7&&p.writesSaveFormat>=rpg::CURRENT_SAVE_FORMAT&&p.writesSaveFormat<=65535&&p.readsCurrentSaves&&p.assets>0&&p.assets<2048&&p.firmware.bytes>0&&p.firmware.bytes<=partitionBytes&&p.art.bytes>16&&uint64_t(p.art.bytes)+153600<psramBytes&&hashValid(p.firmware.sha)&&hashValid(p.art.sha)&&releaseUrl(p.firmware.url)&&releaseUrl(p.art.url);}
inline void artPath(char* out,size_t n,uint32_t crc){snprintf(out,n,"/RPGPOKET/artes_%08lx.pak",(unsigned long)crc);}
inline const char* stage(State s){switch(s){case State::Testing:return "Testando conexao HTTPS...";case State::Checking:return "Verificando repositorio...";case State::Downloading:return "Baixando arquivos...";case State::Verifying:return "Conferindo os arquivos...";case State::Installing:return "Instalando o jogo...";case State::Cleaning:return "Limpando temporarios...";case State::Restarting:return "Reiniciando...";default:return "";}}
}
inline updater::Info updateInfo;
