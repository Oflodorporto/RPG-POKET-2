#define RPG_ENABLE_COOP_TRAINING 1
#include "../firmware/RPG_POKET_2/View.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main session_suite_main
#include "coop_training_session.cpp"
#undef main
#pragma GCC diagnostic pop
#include "../firmware/RPG_POKET_2/Coop/CoopInput.h"
using coopview::Control;
static Control sample(Input& input,Session& session,bool valid,bool down,int x,int y,uint32_t now){auto p=toPortrait(x,y);input.capture(session,valid,down,p.x,p.y,now);return input.dispatch(session,now);}
static Control click(Input& input,Session& session,int x,int y,uint32_t& now){
 CHECK(sample(input,session,true,false,0,0,now)==Control::None);now+=31;CHECK(sample(input,session,true,false,0,0,now)==Control::None);
 CHECK(sample(input,session,true,true,x,y,now)==Control::None);now+=8;CHECK(sample(input,session,true,false,0,0,now)==Control::None);now+=31;return sample(input,session,true,false,0,0,now);
}
static void controls(){
 Rig r;Input a,b;
 CHECK(sample(a,r.s[0],true,true,50,195,r.now)==Control::None);r.now+=40;CHECK(sample(a,r.s[0],true,false,0,0,r.now)==Control::None);r.now+=31;CHECK(sample(a,r.s[0],true,false,0,0,r.now)==Control::None);CHECK(r.s[0].state()==SessionState::Browsing);
 CHECK(click(a,r.s[0],50,195,r.now)==Control::Host);CHECK(r.s[0].host(r.now));r.flush();CHECK(r.s[1].peer(0));
 CHECK(click(b,r.s[1],200,60,r.now)==Control::Join0);CHECK(r.s[1].join(0,r.now));r.flush();CHECK(r.s[0].state()==SessionState::AwaitingAccept);
 CHECK(click(a,r.s[0],50,195,r.now)==Control::Accept);CHECK(r.s[0].accept(r.now));r.flush();CHECK(r.s[0].state()==SessionState::Playing);
 const auto before=packed(r.game[0]);CHECK(click(a,r.s[0],8,210,r.now)==Control::None);CHECK(r.s[0].training().ui.mode==coopui::Mode::Target);
 auto hit=r.s[0].training().ui.targetRect(r.s[0].training().endpoint,coop::firstEnemy(r.s[0].training().endpoint.state));CHECK(click(a,r.s[0],hit.x+3,hit.y+3,r.now)==Control::None);r.advance(1);CHECK(r.s[0].training().endpoint.state.revision==1);
 CHECK(click(a,r.s[0],280,10,r.now)==Control::Exit);r.s[0].leave(r.now);r.flush();CHECK(r.s[1].state()==SessionState::Ended&&r.s[1].endReason()==SessionEnd::PeerLeft);CHECK(packed(r.game[0])==before);
}
static void interrupted(){
 Rig r;r.discover();Input input;auto& s=r.s[1];
 sample(input,s,true,false,0,0,r.now);r.now+=31;sample(input,s,true,false,0,0,r.now);sample(input,s,true,true,200,60,r.now);
 // Same row, same visible tag, new full 64-bit room token: no old join gesture.
 auto peer=*s.peer(0);auto changed=admissionFrame(peer.mac,admission::Type::Beacon,peer.sessionToken+(uint64_t(1)<<16),0,&peer.profile);r.inject(1,changed);
 sample(input,s,true,false,0,0,r.now);r.now+=31;CHECK(sample(input,s,true,false,0,0,r.now)==Control::None);CHECK(s.state()==SessionState::Browsing);
 // Invalid touch and held continuation cannot trigger Host/Join/Exit.
 sample(input,s,true,true,280,10,r.now);sample(input,s,false,true,280,10,r.now);sample(input,s,true,true,280,10,r.now);sample(input,s,true,false,0,0,r.now);r.now+=31;CHECK(sample(input,s,true,false,0,0,r.now)==Control::None);
 // Drag across controls and back remains cancelled.
 sample(input,s,true,true,50,195,r.now);sample(input,s,true,true,280,10,r.now);sample(input,s,true,true,50,195,r.now);sample(input,s,true,false,0,0,r.now);r.now+=31;CHECK(sample(input,s,true,false,0,0,r.now)==Control::None);
 // Overflow clears all queued presses/releases, no action during capture.
 for(unsigned i=0;i<20;++i)input.capture(s,true,i%2,100,100,r.now+i*31);CHECK(s.state()==SessionState::Browsing&&input.dropped==4);CHECK(input.dispatch(s,r.now+700)==Control::None);
 CHECK(click(input,s,280,10,r.now)==Control::Exit);
}
static void pureView(){
 static uint8_t nativeArt[ART_BYTES]{};artMemory=nativeArt;makeFallback();
 Rig r;r.link();auto before=packed(r.game[0]);coopSession=r.s[0];
 struct Canvas {uint16_t pixels[240*320]{};int x=0,y=0,size=1;uint16_t color=0;
 void drawPixel(int a,int b,uint16_t c){if(a>=0&&a<240&&b>=0&&b<320)pixels[b*240+a]=c;}
 void fillRect(int a,int b,int w,int h,uint16_t c){for(int j=0;j<h;++j)for(int i=0;i<w;++i)drawPixel(a+i,b+j,c);}
 void drawRect(int a,int b,int w,int h,uint16_t c){fillRect(a,b,w,1,c);fillRect(a,b+h-1,w,1,c);fillRect(a,b,1,h,c);fillRect(a+w-1,b,1,h,c);}
 void fillScreen(uint16_t c){fillRect(0,0,240,320,c);}void setTextWrap(bool){}void setTextSize(int s){size=s;}void setTextColor(uint16_t c){color=c;}void setCursor(int a,int b){x=a;y=b;}
 void print(const char* value){for(;*value;++value){unsigned ch=uint8_t(*value);for(unsigned a=0;a<5;++a)for(unsigned b=0;b<8;++b)if(font[ch*5+a]&(1<<b))fillRect(x+a*size,y+b*size,size,size,color);x+=6*size;}}
 void draw16bitRGBBitmap(int a,int b,uint16_t* data,int w,int h){for(int j=0;j<h;++j)for(int i=0;i<w;++i)drawPixel(a+i,b+j,data[j*w+i]);}
 };static Canvas canvas;ViewState view;view.page=Page::CoopTraining;menu.art=ArtStatus::Missing;auto hash=coop::stateHash(coopSession.training().endpoint.state);
 for(unsigned i=0;i<20;++i)render(canvas,r.game[0],view,i);CHECK(packed(r.game[0])==before&&coop::stateHash(coopSession.training().endpoint.state)==hash);
 auto mapped=toPortrait(266,2);CHECK(canvas.pixels[mapped.y*240+mapped.x]!=0);view.page=Page::Guild;render(canvas,r.game[0],view,0);CHECK(packed(r.game[0])==before); // actual View dispatch restores portrait
}
int main(){controls();interrupted();pureView();CHECK(!allocations);std::printf("PASS: %u checks; real lobby controls, target/exit, stale row/release/overflow, actual View landscape/portrait dispatch; zero C++ allocations\n",checks);}
