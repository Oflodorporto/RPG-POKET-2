#include "../firmware/RPG_POKET_2/DialogueStory.h"
#include "../firmware/RPG_POKET_2/Save.h"
#include <cassert>
#include <cstring>
#include <cstdio>
int main(){unsigned conversations=0,pages=0;for(unsigned city=0;city<4;++city)for(unsigned person=0;person<3;++person)for(unsigned progress=0;progress<6;++progress){auto g=rpg::create(0,77);g.city=city;g.tutorial=true;if(progress>=1){g.ruinsWins=3;g.guardianDefeated=true;}if(progress>=2)g.dungeonClears=1;if(progress>=3)g.campaignFlags=progress==3?1:progress==4?3:7;uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,1,before);const char* text=story::conversation(g,person);assert(strlen(text)>140&&strstr(text," "));unsigned rows=story::wrapStory(text,18,[](unsigned,const char* line){assert(strlen(line)<=18);});assert(story::conversationPages(text)==(rows+6)/7&&rows<=42);pages+=story::conversationPages(text);++conversations;rpg::encode(g,1,after);assert(!memcmp(before,after,sizeof(before)));assert(!strstr(text,"atualizacao")&&!strstr(text,"proxima etapa"));}
 auto g=rpg::create(0,77);g.city=3;assert(!strstr(story::conversation(g,0),"Vigilia"));g.campaignFlags=7;assert(strstr(story::conversation(g,0),"Vigilia"));assert(strstr(story::conversation(g,2),"Anwen"));g.city=2;g.campaignFlags=0;assert(strstr(story::conversation(g,0),"Iria"));g.dungeonClears=1;assert(strstr(story::conversation(g,0),"provas"));g.campaignFlags=3;assert(strstr(story::conversation(g,0),"refugiados"));
 printf("PASS: %u continuous NPC conversations/%u pages;18-column large font,7rows,progress coherence,no update placeholders,read-only\n",conversations,pages);
}
