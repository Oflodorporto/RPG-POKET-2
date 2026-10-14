#pragma once
// PvP-only presentation. Existing PvE art/poses are reused without executing
// gameplay, consuming RNG, allocating a framebuffer or reading the SD card.
namespace clubView {
inline CombatResult feedback(const ClubVisual& event){
 CombatResult result;result.hasHeroAction=event.action!=club::Action::Surrender;
 result.heroAction=event.action==club::Action::Offensive?rpg::Action::Offensive:event.action==club::Action::Defensive?rpg::Action::Defensive:rpg::Action::Attack;
 result.kind=event.action==club::Action::Defensive?CombatFeedback::Guard:event.damage?CombatFeedback::Damage:CombatFeedback::NoDamage;
 result.amount=event.damage;result.recoveredHp=event.healing;return result;
}
inline Effect effect(unsigned cls,club::Action action){
 if(action==club::Action::Surrender)return Effect::None;
 if(action==club::Action::Defensive)return Effect::Shield;
 if(action==club::Action::Offensive)return cls==0?Effect::Lightning:cls==3?Effect::Rage:Effect::Slash;
 return cls==0?Effect::Projectile:cls==2?Effect::Thrust:Effect::Slash;
}
// Mirror around the two authored sprite pivots (8 and 124), not the screen's
// center. All action primitives stay between the identity labels and the HUD.
template<class C>struct EffectCanvas {
 CombatCanvas<C> clip;bool mirrored;
 EffectCanvas(C& c,bool mirror):clip{c,38,158,0,0},mirrored(mirror){}
 void fillRect(int x,int y,int w,int h,uint16_t color){clip.fillRect(mirrored?244-x-w:x,y-39,w,h,color);}
 void drawRect(int x,int y,int w,int h,uint16_t color){if(w<=0||h<=0)return;fillRect(x,y,w,1,color);fillRect(x,y+h-1,w,1,color);fillRect(x,y,1,h,color);fillRect(x+w-1,y,1,h,color);}
};
template<class C>void hud(C& c,const club::Fighter& fighter,int x){
 c.fillRect(x,160,106,38,UI_INK);c.drawRect(x,160,106,38,0x3186);char text[24];
 snprintf(text,sizeof(text),"HP %u/%u",fighter.hp,fighter.maxHP);panelLabel(c,x+3,161,100,text,UI_WHITE);panelBar(c,x+5,171,96,fighter.hp,fighter.maxHP,UI_GREEN);
 snprintf(text,sizeof(text),"MP %u/%u",fighter.mp,fighter.maxMP);panelLabel(c,x+3,175,100,text,UI_BLUE);c.fillRect(x+5,185,96,2,0x18c6);c.fillRect(x+5,185,fighter.maxMP?96*unsigned(fighter.mp)/fighter.maxMP:0,2,UI_BLUE);
 if(fighter.guard)snprintf(text,sizeof(text),"Guarda %u%%",fighter.guard);else snprintf(text,sizeof(text),"Guarda --");panelLabel(c,x+3,188,100,text,fighter.guard?UI_GOLD:UI_MUTED);
}
template<class C>void button(C& c,const club::Duel& duel,unsigned index){
 const int x=14+int(index%2)*110,y=220+int(index/2)*50;const auto action=club::Action(index);bool available=duel.canPlay(action);
 const char* names[]={"Ataque","Ofensiva","Defesa","Desistir"};c.fillRect(x,y,102,40,UI_PANEL);c.drawRect(x,y,102,40,available?UI_GOLD:UI_MUTED);c.drawRect(x+2,y+2,98,36,0x3186);
 panelLabel(c,x+3,y+4,96,names[index],available?UI_WHITE:UI_MUTED,2);char detail[24];
 if(index==3)snprintf(detail,sizeof(detail),duel.surrenderRequested?"Confirmando...":"Encerrar duelo");
 else snprintf(detail,sizeof(detail),duel.fighters[duel.local].mp<duel.cost(action)?"%u MP / sem mana":"%u MP",duel.cost(action));
 panelLabel(c,x+3,y+26,96,detail,index!=3&&duel.fighters[duel.local].mp<duel.cost(action)?UI_RED:available?UI_GOLD:UI_MUTED);
}
}
template<class C>void drawClubBattle(C& c,const rpg::Game& game){
 const auto& duel=arena.duel;const auto& event=arena.visual;const auto& peer=arena.session.opponent;
 const auto& me=duel.fighters[duel.local];const auto& foe=duel.fighters[1-duel.local];
 c.fillRect(10,5,220,22,UI_INK);panelLabel(c,12,8,216,"ARENA DA GUILDA",UI_GOLD,2);
 char label[32];snprintf(label,sizeof(label),"VOCE / %s",raceName(game.race));c.fillRect(10,28,106,10,UI_INK);panelLabel(c,12,28,102,label,UI_GOLD);
 snprintf(label,sizeof(label),"RIVAL / %s",raceName(peer.race));c.fillRect(124,28,106,10,UI_INK);panelLabel(c,126,28,102,label,UI_WHITE);
 auto opponent=rpg::create(foe.classId,1);opponent.race=peer.race;opponent.shirt=peer.shirt;opponent.trousers=peer.trousers;
 const bool active=event.active();const bool localActor=event.actor==duel.local;const unsigned progress=active?unsigned(event.elapsedMs)*1000/ClubVisual::duration:1000;
 const auto result=clubView::feedback(event);const auto pose=active?heroPose(duel.fighters[event.actor].classId,result,progress,event.elapsedMs):HeroPose{};
 const int recoil=active?combatRecoil(result,progress):0;
 CombatCanvas<C> sprites{c,38,158,0,0};
 personalSprite(sprites,game,8+(active&&localActor?pose.x:-recoil),38+(active&&localActor?pose.y:0),active&&localActor?pose.heroFrame:0);
 personalSprite(sprites,opponent,124+(active&&!localActor?-pose.x:recoil),38+(active&&!localActor?pose.y:0),active&&!localActor?pose.heroFrame:0,true);
 if(active){
  // The adapter is local to this render; it cannot leak a PvP action into PvE.
  ViewState effect;effect.effect=clubView::effect(duel.fighters[event.actor].classId,event.action);effect.effectProgress=progress;effect.effectElapsedMs=event.elapsedMs;effect.effectFrame=progress*8/1000;effect.combatResult=result;
  clubView::EffectCanvas<C> canvas(c,!localActor);heroActionEffect(canvas,localActor?game:opponent,effect,false,182,141);
  if(progress>=491&&progress<900&&event.damage){int x=localActor?126:12;snprintf(label,sizeof(label),"-%u HP",event.damage);c.fillRect(x,42,102,20,UI_INK);panelLabel(c,x+2,43,98,label,UI_RED,2);}
  if(progress>=200&&progress<900&&event.action==club::Action::Defensive){int x=localActor?12:126;snprintf(label,sizeof(label),event.healing?"+%u HP":"GUARDA",event.healing);c.fillRect(x,42,102,20,UI_INK);panelLabel(c,x+2,43,98,label,event.healing?UI_GREEN:UI_GOLD,event.healing?2:1);}
 }
 clubView::hud(c,me,10);clubView::hud(c,foe,124);
 c.fillRect(10,200,220,19,UI_INK);
 const char* status=duel.failed?"Conexao perdida":duel.surrenderRequested?"Confirmando desistencia":duel.finished?"Confirmando resultado":!duel.ready?"Preparando duelo":duel.waiting?"Confirmando acao":duel.turn==duel.local?"Seu turno":"Turno adversario";
 panelLabel(c,12,200,216,status,UI_GOLD);if(*arena.notice)panelLabel(c,12,210,216,arena.notice,UI_RED);
 for(unsigned i=0;i<4;++i)clubView::button(c,duel,i);
}
