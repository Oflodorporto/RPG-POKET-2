#pragma once
// New, code-native leather candidate (2026-10-26), not the missing earlier preview.
// Decoration only: no inventory state, hitboxes, timers, IO, heap or PSRAM.
namespace bagLeather {
constexpr uint16_t edge=0x20c1, shadow=0x1881, hide=0x59e5, light=0x8b48,
  seam=0xb4ab, brass=0xc58c, brassLight=0xe6b3, lining=0x28e2, strap=0x4143;
template<class C>void stitch(C& c,int x,int y,int w,int h){
 for(int dx=4;dx<w-4;dx+=6){c.fillRect(x+dx,y,3,1,seam);c.fillRect(x+dx,y+h-1,3,1,seam);}
 for(int dy=4;dy<h-4;dy+=6){c.fillRect(x,y+dy,1,3,seam);c.fillRect(x+w-1,y+dy,1,3,seam);}
}
template<class C>void stud(C& c,int x,int y){c.fillRect(x,y,3,3,edge);c.fillRect(x,y,2,2,brass);c.fillRect(x,y,1,1,brassLight);}
template<class C>void buckle(C& c,int x,int y){
 c.fillRect(x,y,16,21,shadow);c.drawRect(x,y,16,21,brass);c.drawRect(x+1,y+1,14,19,brassLight);
 c.fillRect(x+3,y+3,10,15,strap);c.fillRect(x+7,y+2,2,17,brass);c.fillRect(x+7,y+7,6,2,brassLight);
}
template<class C>void plate(C& c,int x,int y,int w,int h){
 c.fillRect(x,y,w,h,shadow);c.drawRect(x,y,w,h,light);c.fillRect(x+2,y+2,w-4,h-4,lining);
 c.fillRect(x+2,y+1,w-4,1,seam);c.fillRect(x+2,y+h-2,w-4,1,edge);
}
template<class C>void background(C& c){
 // Reuse the existing 38,432-byte flash texture at the outer silhouette.
 bagSkin::crop(c,0,0,240,320,0,0,240,320);
 c.fillRect(7,3,226,271,edge);c.fillRect(9,4,222,269,hide);
 // Sparse deterministic grain, computed directly without an image buffer.
 for(int y=5;y<273;++y)for(int x=10;x<230;++x){unsigned n=unsigned(x*17+y*29);if((n&63)==0)c.fillRect(x,y,1,1,light);else if((n&63)==17)c.fillRect(x,y,1,1,strap);}
 c.drawRect(9,4,222,269,light);stitch(c,11,6,218,265);
 // Side straps and two brass buckles stay outside all nine touch targets.
 for(int x:{15,215}){c.fillRect(x,34,10,168,shadow);c.fillRect(x+1,34,8,168,strap);c.fillRect(x+2,35,1,165,light);for(int y=101;y<190;y+=14)c.fillRect(x+4,y,2,2,shadow);stud(c,x+3,39);}
 buckle(c,12,62);buckle(c,212,62);plate(c,12,6,216,26);
 // Cross seam under the pocket grid separates the description compartment.
 c.fillRect(25,204,190,1,edge);c.fillRect(27,205,186,1,light);
}
template<class C>void pocket(C& c,const scenicUi::Rect& r){
 c.fillRect(r.x,r.y,r.w,r.h,shadow);c.fillRect(r.x+1,r.y+1,r.w-2,r.h-2,hide);
 c.drawRect(r.x+1,r.y+1,r.w-2,r.h-2,light);
 c.fillRect(r.x+3,r.y+3,r.w-6,r.h-6,lining);
 c.fillRect(r.x+3,r.y+3,r.w-6,2,hide);c.fillRect(r.x+3,r.y+r.h-5,r.w-6,2,edge);
 // A stitched gusset and corner rivets make each unchanged slot a pocket.
 for(int dy=8;dy<r.h-6;dy+=6){c.fillRect(r.x+2,r.y+dy,1,3,seam);c.fillRect(r.x+r.w-3,r.y+dy,1,3,seam);}
 stud(c,r.x+5,r.y+5);stud(c,r.x+r.w-8,r.y+5);
}
template<class C>void equipment(C& c,const scenicUi::Rect& r){
 plate(c,r.x,r.y,r.w,r.h);c.fillRect(r.x+3,r.y+3,r.w-6,5,hide);stitch(c,r.x+3,r.y+3,r.w-6,r.h-6);
 // Small fasteners, clear of the live equipment count.
 for(int x:{r.x+6,r.x+r.w-11}){c.fillRect(x,r.y+13,5,13,strap);c.drawRect(x,r.y+16,5,6,brass);}
}
template<class C>void button(C& c,const scenicUi::Rect& r){
 plate(c,r.x,r.y,r.w,r.h);c.fillRect(r.x+3,r.y+3,r.w-6,r.h-6,strap);stitch(c,r.x+3,r.y+3,r.w-6,r.h-6);
 c.fillRect(r.x+5,r.y+5,r.w-10,1,light);c.fillRect(r.x+5,r.y+r.h-6,r.w-10,1,shadow);
}
}
