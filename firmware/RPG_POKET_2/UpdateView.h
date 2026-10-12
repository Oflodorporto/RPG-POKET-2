#pragma once
// Firmware-owned text and geometry. This screen never reads the SD art buffer.
// Each dynamic string in updater::Info is <=39 bytes, so two 36-char rows suffice.
template<class C>void updateLines(C& c,int y,const char* text,uint16_t color=UI_WHITE){
 unsigned row=0;const char* p=text;
 while(*p&&row<2){unsigned count=std::min<size_t>(36,strlen(p));
  if(p[count]&&count==36){unsigned split=count;while(split&&p[split]!=' ')--split;if(row==0&&split&&strlen(p+split+1)<=36)count=split;}
  char line[37];memcpy(line,p,count);line[count]=0;panelLabel(c,12,y+row*11,216,line,color);
  p+=count;while(*p==' ')++p;++row;
 }
}
template<class C>void drawUpdate(C& c){
 c.setTextWrap(false);c.fillRect(0,0,240,320,UI_INK);
 // Small code-native frame keeps the same layout with missing/invalid SD artwork.
 c.drawRect(4,4,232,312,0x8bcb);scenicHeading(c,"ATUALIZACAO");
 scenicPanel(c,10,50,220,61);
 panelLabel(c,12,55,216,"INSTALADO / GITHUB + WI-FI",UI_GOLD);
 updateLines(c,68,FW_VERSION);updateLines(c,91,FW_PACKAGE,UI_MUTED);
 auto button=[&](int x,int y,int w,const char* title){scenicPanel(c,x,y,w,40);panelLabel(c,x+4,y+15,w-8,title,UI_WHITE);};
 char value[12];
 if(updateInfo.busy){
  scenicPanel(c,10,113,220,153);updateLines(c,122,updater::stage(updateInfo.state),UI_GOLD);
  snprintf(value,sizeof(value),"%u%%",std::min(100u,updateInfo.progress));panelLabel(c,12,151,216,value,UI_GREEN,3);
  c.fillRect(17,191,206,20,UI_INK);c.drawRect(16,190,208,22,0x8bcb);
  c.fillRect(19,193,202*std::min(100u,updateInfo.progress)/100,16,UI_GREEN);
  updateLines(c,221,updateInfo.message,updateInfo.state==updater::State::Error?UI_RED:UI_WHITE);
  panelLabel(c,12,250,216,"Nao retire o cartao ou a energia.",UI_GOLD);
  return;
 }
 bool install=updateInfo.state==updater::State::Available||updateInfo.canResume;
 scenicPanel(c,10,113,220,install?96:53);
 updateLines(c,119,*updateInfo.version?updateInfo.version:menu.connected?"Conectado / pronto para verificar":"Conecte o Wi-Fi primeiro",UI_GOLD);
 updateLines(c,142,updateInfo.message,updateInfo.state==updater::State::Error?UI_RED:UI_WHITE);
 if(install){
  panelLabel(c,12,171,216,updateInfo.artOnly?"Baixar e reparar as artes?":"Instalar a nova versao e artes?");
  panelLabel(c,12,190,216,"Instalacao exige cartao SD.",UI_GOLD);
  button(14,218,212,"Testar conexao");button(14,272,102,"Cancelar");button(124,272,102,updateInfo.canResume?"Continuar":"Instalar");
 }else{
  button(14,173,212,"Testar conexao");button(14,218,212,"Verificar versao");button(14,272,212,"Voltar");
 }
}
