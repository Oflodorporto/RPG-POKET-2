#pragma once
// Shared, SD-free parchment for narration and decision screens.
constexpr uint16_t STORY_PAPER=0xf6d4,STORY_INK=0x30e4,STORY_EDGE=0xa3c9;
inline bool storyPaperPage(Page p){switch(p){
case Page::Prologue:case Page::Dialogue:case Page::Journal:case Page::People:case Page::Continent:case Page::Campaign:case Page::CampaignTask:case Page::CampaignResult:case Page::ContributionResult:case Page::Epilogue:
case Page::IslandEntry:case Page::DungeonEntry:case Page::DungeonVictory:case Page::DungeonLoot:case Page::DungeonExit:case Page::CrystalBuy:case Page::Help:case Page::Result:case Page::Contract:case Page::QuestConfirm:case Page::GuildJoin:case Page::Letters:case Page::LetterRefuse:case Page::EventResult:case Page::EventMission:case Page::EventAbandon:case Page::SlotConfirm:case Page::DeleteSlot:case Page::ForgetWifi:return true;
default:return false;}}
inline uint16_t storyInk(uint16_t color){return color==UI_RED?0xa000:color==UI_GREEN?0x0320:color==UI_BLUE?0x21b0:color==UI_MUTED?0x6b08:color==UI_GOLD?0x72a4:STORY_INK;}
template<class C>bool drawStoryPaper(C& c,const ViewState& v,unsigned frame){
 unsigned age=frame-v.paperStartFrame,opening=v.paperAnimate?std::min(age,6u):6u;
 int h=24+int(opening)*37,top=134-h/2;
 c.fillRect(7,top+3,226,h,0x18c3);c.fillRect(10,top,220,h,STORY_PAPER);c.drawRect(10,top,220,h,STORY_EDGE);
 for(int y=top+12;y<top+h-10;y+=9)c.fillRect(14,y,212,1,0xee72);
 auto roll=[&](int y){c.fillRect(5,y,230,8,STORY_EDGE);c.fillRect(8,y+1,224,3,0xff18);c.drawRect(5,y,230,8,STORY_INK);};roll(top-3);roll(top+h-4);return opening==6;
}
template<class C>bool drawStoryBackdrop(C& c,const Backdrop& bg,const ViewState& v,unsigned frame){drawBackdrop(c,bg);return drawStoryPaper(c,v,frame);}
