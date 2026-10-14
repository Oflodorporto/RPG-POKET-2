#include "CoopRadioPort.h"
#include "../../firmware/RPG_POKET_2/ClubProtocol.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <new>
#include <array>
using namespace coopplatform;
static unsigned checks=0;static unsigned allocations=0;
#define CHECK(x) do {++checks;assert(x);} while(0)
void* operator new(std::size_t n){++allocations;if(void*p=std::malloc(n))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void*p)noexcept{std::free(p);}void operator delete[](void*p)noexcept{std::free(p);}
void operator delete(void*p,std::size_t)noexcept{std::free(p);}void operator delete[](void*p,std::size_t)noexcept{std::free(p);}
constexpr Mac mac[3]={{{2,0,0,0,0,1}},{{2,0,0,0,0,2}},{{2,0,0,0,0,3}}};
rpg::Game hero(uint8_t cls,unsigned level){auto g=rpg::create(cls,123+cls);while(g.p.level<level){g.p.xp=rpg::xpNeeded(g);rpg::levelUp(g);}g.p.hp=g.p.maxhp;g.p.mp=g.p.maxmp;CHECK(rpg::persistentValid(g));return g;}
std::array<uint8_t,160> packed(const rpg::Game& g){std::array<uint8_t,160> b{};rpg::encodePersistent(g,17,0,b.data());return b;}
struct Rig;
static Rig* current=nullptr;static unsigned sending=0;
struct Rig {
 rpg::Game games[2]={hero(0,5),hero(2,7)};Training t[2];RadioPort port[2];uint32_t now=100;unsigned losses=0,duplicated=0,packetNo=0;bool chaos=false;
 Rig(){current=this;for(unsigned i=0;i<2;++i){CHECK(t[i].start({mac[i],mac[1-i],87,uint8_t(i)},games[0],games[1],123,now)==StartError::None);CHECK(port[i].initializeQueue());CHECK(port[i].attach(t[i]));}}
 void drain(){for(unsigned n=0;n<4;++n){for(sending=0;sending<2;++sending)port[sending].flush();for(auto& p:port)p.poll(now);}for(auto& s:t)s.dispatchSamples(now);}
 void sample(unsigned side,bool valid,bool down,Point p){t[side].captureSample(valid,down,p.x,p.y,now);t[side].dispatchSamples(now);}
 void release(unsigned side){sample(side,true,false,{0,0});now+=31;sample(side,true,false,{0,0});}
 void click(unsigned side,int x,int y){release(side);sample(side,true,true,toPortrait(x,y));now+=8;release(side);}
 void attack(unsigned side){click(side,8,210);CHECK(t[side].ui.mode==coopui::Mode::Target);auto rect=t[side].ui.targetRect(t[side].endpoint,coop::firstEnemy(t[side].endpoint.state));click(side,rect.x+1,rect.y+1);}
};
void sink(const uint8_t* destination,const uint8_t* bytes,size_t size){
 CHECK(current&&std::memcmp(destination,mac[1-sending].bytes,6)==0);CHECK(size<=240);
 ++current->packetNo;if(current->chaos&&current->packetNo%7==0){++current->losses;return;}
 esp_now_recv_info_t info{mac[sending].bytes};auto& p=current->port[1-sending];p.enqueue(&info,bytes,int(size));
 if(current->chaos&&current->packetNo%5==0){p.enqueue(&info,bytes,int(size));++current->duplicated;}
}
void mapping(){
 struct Canvas {uint16_t before=0x1234,pixels[240*320]{},after=0x5678;void drawPixel(int x,int y,uint16_t c){CHECK(x>=0&&x<240&&y>=0&&y<320);pixels[y*240+x]=c;}void fillRect(int x,int y,int w,int h,uint16_t c){for(int j=0;j<h;++j)for(int i=0;i<w;++i)drawPixel(x+i,y+j,c);}};
 static Canvas p;LandscapeCanvas<Canvas> c(p);bool visited[240*320]{};
 for(int y=0;y<240;++y)for(int x=0;x<320;++x){auto q=toPortrait(x,y),r=toLandscape(q.x,q.y);CHECK(r.x==x&&r.y==y);CHECK(!visited[q.y*240+q.x]);visited[q.y*240+q.x]=true;c.drawPixel(x,y,1);}
 CHECK(toPortrait(-1,0).x==-1&&toPortrait(320,0).x==-1&&toLandscape(240,0).x==-1&&toLandscape(0,320).x==-1);
 c.fillRect(-3,-4,8,9,2);c.fillRect(319,239,INT32_MAX,INT32_MAX,3);c.fillRect(INT32_MIN,0,INT32_MAX,5,4);c.drawPixel(INT32_MAX,0,5);
 uint16_t tile[6]={11,12,13,14,15,16};c.draw16bitRGBBitmap(10,20,tile,3,2);for(unsigned y=0;y<2;++y)for(unsigned x=0;x<3;++x){auto q=toPortrait(10+x,20+y);CHECK(p.pixels[q.y*240+q.x]==tile[y*3+x]);}
 CHECK(p.before==0x1234&&p.after==0x5678);
}
void profiles(){
 auto h=hero(0,5),g=hero(2,7);CHECK(trainingEligible(h));auto copy=h;copy.dungeonSeed=123;copy.dungeonRun=7;CHECK(rpg::persistentValid(copy));CHECK(trainingEligible(copy));
 copy=h;copy.guildMember=true;copy.clubStage=3;copy.clubSession=12;CHECK(rpg::persistentValid(copy));CHECK(trainingEligible(copy));
 for(unsigned which=0;which<19;++which){copy=h;switch(which){case 0:copy.p.cls=8;break;case 1:copy.p.hp=0;break;case 2:copy.p.mp=copy.p.maxmp+1;break;case 3:copy.attributes[0]=30;break;case 4:copy.equipped[0]=1;break;case 5:copy.phase=rpg::Phase::Hero;break;case 6:copy.guard=1;break;case 7:copy.rageTurns=1;break;case 8:copy.sacredTurns=1;break;case 9:copy.turnedTurns=1;break;case 10:copy.surgePending=true;break;case 11:copy.surgeTurnUsed=true;break;case 12:copy.tripStage=1;break;case 13:copy.campStage=1;break;case 14:copy.dungeonFlags=1;break;case 15:copy.campaignStage=1;break;case 16:copy.discovery=1;break;case 17:copy.eventStage=2;break;case 18:copy.clubStage=1;break;}CHECK(!trainingEligible(copy));Training t;CHECK(t.start({mac[0],mac[1],3,0},copy,g,1,0)==StartError::Profile);CHECK(!t.active());}
 Training t;CHECK(t.start({mac[0],mac[0],3,0},h,g,1,0)==StartError::Binding);CHECK(t.start({mac[0],mac[1],0,0},h,g,1,0)==StartError::Binding);
}
void touch(){
 Rig r;r.drain();auto& t=r.t[0];CHECK(t.endpoint.canPlay());const auto before=coop::stateHash(t.endpoint.state);
 r.sample(0,true,true,toPortrait(8,210));r.release(0);CHECK(t.ui.mode==coopui::Mode::Action); // held on entry
 r.click(0,8,210);CHECK(t.ui.mode==coopui::Mode::Target);
 r.sample(0,true,true,toPortrait(190,50));r.sample(0,false,true,toPortrait(190,50));r.sample(0,true,true,toPortrait(190,50));r.release(0);CHECK(coop::stateHash(t.endpoint.state)==before); // invalid contact cancelled
 r.click(0,250,210);CHECK(t.ui.mode==coopui::Mode::Action&&t.ui.cancelled==1);CHECK(coop::stateHash(t.endpoint.state)==before);
 r.click(0,8,210);r.sample(0,true,true,toPortrait(190,50));r.sample(0,true,true,toPortrait(160,30));r.sample(0,true,true,toPortrait(190,50));r.release(0);CHECK(coop::stateHash(t.endpoint.state)==before);
 // Capture during render cannot submit. Overflow invalidates the whole gesture.
 r.sample(0,true,true,toPortrait(190,50));for(unsigned i=0;i<18;++i)t.captureSample(true,false,0,0,r.now+i*31);CHECK(coop::stateHash(t.endpoint.state)==before);t.dispatchSamples(r.now+600);CHECK(coop::stateHash(t.endpoint.state)==before&&t.touchDrops==2);
 r.sample(0,true,true,toPortrait(190,50));r.release(0);CHECK(coop::stateHash(t.endpoint.state)==before); // cannot reuse held overflow contact
 r.release(0);r.click(0,190,50);CHECK(t.endpoint.state.revision==1);r.drain();CHECK(r.t[1].endpoint.state.revision==1);
 // Queued guest touch from old revision cannot execute on a new state.
 auto& guest=r.t[1];guest.captureSample(true,false,0,0,r.now);guest.endpoint.state.revision++;guest.dispatchSamples(r.now);CHECK(guest.staleTouches==1);
 // Release debounce handles uint32 wrap.
 ContactSampler sampler;coopui::TouchUi ui;auto& e=t.endpoint;sampler.sample(ui,e,true,false,0,0,UINT32_MAX-10);sampler.sample(ui,e,true,false,0,0,21);CHECK(!ui.contactDown());
}
coop::Packet heartbeat(uint8_t sender,uint64_t nonce){coop::Packet p{};auto w=coop::begin(p,coop::Type::Heartbeat,sender,nonce,0);w.u32(0);CHECK(coop::finish(p,w));return p;}
void staleInput(){
 Rig r;r.drain();auto& t=r.t[0];r.release(0);auto point=toPortrait(8,210);
 t.captureSample(true,true,point.x,point.y,r.now);t.endpoint.state.actors[0].hp--;t.dispatchSamples(r.now);CHECK(t.staleTouches==1&&!t.ui.contactDown());
 t.endpoint.paused=true;t.captureSample(true,true,point.x,point.y,r.now);t.endpoint.paused=false;t.dispatchSamples(r.now);CHECK(t.staleTouches==2&&!t.ui.contactDown());
 // Capture before disconnect, then restore the very same state before dispatch.
 r.release(0);t.captureSample(true,true,point.x,point.y,r.now);t.endpoint.paused=true;t.tick(r.now);t.endpoint.paused=false;t.tick(r.now);t.dispatchSamples(r.now);CHECK(t.staleTouches==3&&!t.ui.contactDown());
}
void radio(){
 Rig r;r.drain();auto& t=r.t[0];auto& port=r.port[0];auto b=heartbeat(1,87);esp_now_recv_info_t info{mac[1].bytes};const auto rev=t.endpoint.state.revision;
 // Radio callback is copy-only, even with valid matching packets.
 for(unsigned i=0;i<10;++i)port.enqueue(&info,b.bytes,b.size);CHECK(port.queueDrops==2&&t.endpoint.state.revision==rev);port.poll(r.now);
 auto malformed=b;malformed.bytes[0]^=1;port.enqueue(&info,malformed.bytes,malformed.size);
 auto wrongNonce=heartbeat(1,88);port.enqueue(&info,wrongNonce.bytes,wrongNonce.size);
 auto wrongPeer=heartbeat(0,87);port.enqueue(&info,wrongPeer.bytes,wrongPeer.size);
 info.src_addr=mac[2].bytes;port.enqueue(&info,b.bytes,b.size);
 club::Packet pvp{};pvp.magic=club::MAGIC;pvp.version=club::VERSION;info.src_addr=mac[1].bytes;port.enqueue(&info,reinterpret_cast<uint8_t*>(&pvp),sizeof(pvp));
 port.enqueue(nullptr,b.bytes,b.size);port.enqueue(&info,nullptr,b.size);port.enqueue(&info,b.bytes,-1);port.enqueue(&info,b.bytes,241);port.enqueue(&info,b.bytes,25);
 port.poll(r.now);CHECK(port.rejected==5);CHECK(t.endpoint.state.revision==rev);
 port.enqueue(&info,b.bytes,b.size);port.detach();CHECK(port.attach(r.t[1]));port.poll(r.now);CHECK(port.stale==1);port.detach();CHECK(port.attach(t));
 radioFail=true;t.endpoint.tick(r.now+1000);port.flush();CHECK(port.sendErrors>0);radioFail=false;
 port.detach();port.enqueue(&info,b.bytes,b.size);port.poll(r.now);CHECK(t.endpoint.state.revision==rev);
}
void restart(){
 Rig r;r.drain();auto& t=r.t[0];auto& p=r.port[0];auto packet=heartbeat(1,87);esp_now_recv_info_t info{mac[1].bytes};p.enqueue(&info,packet.bytes,packet.size);
 const auto generation=t.generationId();t.stop();CHECK(!p.attached());CHECK(t.start({mac[0],mac[1],87,0},r.games[0],r.games[1],123,r.now)==StartError::None);CHECK(t.generationId()!=generation&&!p.attached());auto count=t.endpoint.outgoing.count;p.flush();CHECK(t.endpoint.outgoing.count==count);p.poll(r.now);CHECK(p.stale==1&&t.endpoint.paused);CHECK(!p.attach(t));p.detach();CHECK(p.attach(t)&&p.attached());
 // Frozen foreign protocol version cannot be admitted even if magic is correct.
 packet=heartbeat(1,87);packet.bytes[4]=2;p.enqueue(&info,packet.bytes,packet.size);p.poll(r.now);CHECK(p.rejected==1);
}
uint32_t finish(bool chaos){
 Rig r;auto a=packed(r.games[0]),b=packed(r.games[1]);uint32_t rng[2]={r.games[0].randomState,r.games[1].randomState};r.chaos=chaos;r.drain();
 for(unsigned step=0;step<3000;++step){r.now+=50;r.drain();for(unsigned i=0;i<2;++i)if(r.t[i].endpoint.canPlay()){r.attack(i);r.drain();}if(r.t[0].endpoint.receipt.valid&&r.t[1].endpoint.receipt.valid&&!r.t[0].endpoint.awaitingAck)break;}
 CHECK(r.t[0].endpoint.receipt.valid&&r.t[1].endpoint.receipt.valid);CHECK(coop::stateHash(r.t[0].endpoint.state)==coop::stateHash(r.t[1].endpoint.state));auto hash=coop::stateHash(r.t[0].endpoint.state);
 CHECK(r.t[0].endpoint.state.wave==2&&r.t[0].endpoint.state.phase==coop::Phase::Won);CHECK(packed(r.games[0])==a&&packed(r.games[1])==b&&r.games[0].randomState==rng[0]&&r.games[1].randomState==rng[1]);
 if(chaos)CHECK(r.losses&&r.duplicated);for(unsigned i=0;i<2;++i){r.port[i].detach();r.t[i].stop();CHECK(!r.t[i].active()&&!r.t[i].endpoint.receipt.valid);}CHECK(packed(r.games[0])==a&&packed(r.games[1])==b);return hash;
}
void disconnect(){
 Rig r;r.drain();auto a=packed(r.games[0]),b=packed(r.games[1]);r.attack(0); // drop event/ACK path
 r.port[1].detach();r.now+=3000;r.port[0].poll(r.now);CHECK(r.t[0].endpoint.paused);auto state=coop::stateHash(r.t[0].endpoint.state);r.now+=3000;r.port[0].poll(r.now);CHECK(coop::stateHash(r.t[0].endpoint.state)==state&&!r.t[0].endpoint.receipt.valid);
 CHECK(r.port[1].attach(r.t[1]));r.drain();CHECK(!r.t[0].endpoint.paused&&coop::stateHash(r.t[0].endpoint.state)==coop::stateHash(r.t[1].endpoint.state));
 for(unsigned i=0;i<2;++i){r.port[i].detach();r.t[i].stop();}CHECK(packed(r.games[0])==a&&packed(r.games[1])==b);
}
int main(){radioSink=sink;mapping();profiles();touch();staleInput();radio();restart();const auto stable=finish(false),lossy=finish(true);CHECK(stable==lossy);disconnect();CHECK(allocations==0);printf("PASS: %u checks; zero C++ heap allocations; Training=%zu RadioPort=%zu ContactSampler=%zu; final hash=%u\n",checks,sizeof(Training),sizeof(RadioPort),sizeof(ContactSampler),stable);}
