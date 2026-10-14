#include "ClubDuel.h"
#include <vector>
#include <cstdio>
#include <cassert>
struct M{unsigned source;uint16_t seq;uint8_t action;bool ack;int8_t result=-2;uint32_t digest=0;};
struct N{
 club::Duel d[2];std::vector<M> q;struct C{N* n;unsigned who;}c[2]{{this,0},{this,1}};
 static void send(void* v,uint16_t seq,uint8_t action,bool ack){auto* c=(C*)v;c->n->q.push_back({c->who,seq,action,ack});}
 static void result(void* v,uint16_t seq,int8_t winner,uint32_t digest){auto* c=(C*)v;c->n->q.push_back({c->who,seq,0,false,winner,digest});}
 void recv(M m,uint32_t now){if(m.result==-2)d[1-m.source].receive(m.seq,m.action,m.ack,now);else d[1-m.source].receiveResult(m.seq,m.result,m.digest,now);}
};
uint32_t seed=0x7418ab;unsigned rnd(){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
int main(){unsigned steps=0;for(unsigned k=0;k<1024;++k){N n;unsigned cls[2]={rnd()%4,rnd()%4},lvl[2]={1+rnd()%99,1+rnd()%99};uint32_t now=k%2?UINT32_MAX-10000:0,id=1+rnd();for(unsigned a=0;a<2;++a){n.d[a].begin(id,!a,cls[a],lvl[a],cls[1-a],lvl[1-a],N::send,&n.c[a],now);n.d[a].setResultSender(N::result);}
 for(unsigned step=0;step<240;++step){now+=rnd()%201;for(unsigned a=0;a<2;++a){if(rnd()%3==0)n.d[a].play(club::Action(rnd()%5),now);n.d[a].tick(now);}
 unsigned budget=rnd()%8;while(!n.q.empty()&&budget--){unsigned i=rnd()%n.q.size();M m=n.q[i];n.q.erase(n.q.begin()+i);printf("m %u %u %u %u %d %u\n",m.source,m.seq,m.action,m.ack,m.result,m.digest);if(rnd()%100<15)continue;n.recv(m,now);if(rnd()%100<12)n.recv(m,now);}
 for(auto& d:n.d){printf("s %u %u %u %u %u %u %u %u %u %u %u %u %d %u %u",d.stateDigest(),d.revision,d.local,d.turn,d.lastActor,unsigned(d.lastAction),d.active,d.ready,d.waiting,d.finished,d.failed,d.surrenderRequested,d.winner,d.terminalConfirmed,d.lastDamage);for(auto& f:d.fighters)printf(" %u %u %u %u %u %u %u %u",f.hp,f.maxHP,f.mp,f.maxMP,f.attack,f.defense,f.classId,f.guard);puts("");}++steps;
 }}fprintf(stderr,"PASS: %u randomized paired state transitions\n",steps);}
