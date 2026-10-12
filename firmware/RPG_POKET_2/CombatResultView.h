#pragma once
template<class C>void combatResultLabel(C& c,const ViewState& v,bool dungeon){
 unsigned p=combatProgress(v.effectProgress,v.effectFrame);
 const auto& r=v.combatResult;
 unsigned elapsed=v.effectElapsedMs==UINT32_MAX?v.effectFrame*1100/8:v.effectElapsedMs;
 bool beforeImpact=v.effect==Effect::MagicDarts?elapsed<540:p<491;
 if(v.effect==Effect::None||r.kind==CombatFeedback::None||beforeImpact||p>=1000)return;
 char label[24];uint16_t color=UI_GOLD;
 switch(r.kind){
 case CombatFeedback::Damage:snprintf(label,sizeof(label),r.critical?"CRIT -%u":"-%u",r.amount);color=v.effectOnHero?UI_RED:r.critical?UI_WHITE:UI_GOLD;break;
 case CombatFeedback::Heal:snprintf(label,sizeof(label),"+%u HP",r.amount);color=UI_GREEN;break;
 case CombatFeedback::Miss:snprintf(label,sizeof(label),"ESQUIVA");color=UI_MUTED;break;
 case CombatFeedback::NoDamage:snprintf(label,sizeof(label),"SEM DANO");color=UI_MUTED;break;
 case CombatFeedback::Skip:snprintf(label,sizeof(label),"SEM ACAO");color=UI_MUTED;break;
 case CombatFeedback::Guard:snprintf(label,sizeof(label),"DEFESA");color=UI_BLUE;break;
 case CombatFeedback::Buff:snprintf(label,sizeof(label),v.effect==Effect::Rage?"FURIA":"PREPARO");break;
 default:return;
 }
 int w=int(strlen(label))*6,cx=dungeon?(v.effectOnHero?180:120):v.effectOnHero?55:182;
 int x=std::max(3,std::min(237-w,cx-w/2)),y=(dungeon?(v.effectOnHero?140:65):104)-int((p>491?p-491:0)*14/509);
 c.fillRect(x-2,y-2,w+4,12,UI_INK);c.setTextSize(1);c.setTextColor(color);c.setCursor(x,y);c.print(label);
}
