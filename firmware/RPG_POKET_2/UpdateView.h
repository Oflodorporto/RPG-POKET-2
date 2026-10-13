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
// Four compact rows also fit the idle screen, so the last failure remains
// retrievable after Back/Test connection without SD/NVS writes or new controls.
template<class C>void updateWriteDetail(C& c,int y,const updater::WriteFailure& f){
 char line[64];
 snprintf(line,sizeof(line),"Ultima SD %s %u%% E%d",updater::fileLabel(f.file),std::min(100u,f.progress),f.error);updateLines(c,y,line,UI_RED);
 snprintf(line,sizeof(line),"Pos: %lu/%lu B",(unsigned long)f.offset,(unsigned long)f.total);updateLines(c,y+11,line);
 snprintf(line,sizeof(line),"Gravou %lu de %lu B",(unsigned long)f.written,(unsigned long)f.requested);updateLines(c,y+22,line);
 if(f.freeKnown)snprintf(line,sizeof(line),"Livre antes: %llu KiB",(unsigned long long)(f.initialFree/1024));else snprintf(line,sizeof(line),"Livre antes: nao medido");updateLines(c,y+33,line,UI_MUTED);
}
template<class C>void updateVerifyDetail(C& c,int y,const storageDiag::Failure& f){
 char line[64];snprintf(line,sizeof(line),"%s: %s",f.stage==storageDiag::Stage::Boot?"Ao iniciar":"Conferencia",storageDiag::fileName(f.file));updateLines(c,y,line,UI_RED);
 snprintf(line,sizeof(line),"%s / cod %d",storageDiag::reason(f.issue),f.code);updateLines(c,y+11,line,UI_GOLD);
 if(storageDiag::hexValues(f.issue)){snprintf(line,sizeof(line),"Esperado: %08lx",(unsigned long)f.expected);updateLines(c,y+22,line);snprintf(line,sizeof(line),"Obtido: %08lx",(unsigned long)f.actual);}
 else {snprintf(line,sizeof(line),"Pos %lu / esperado %lu",(unsigned long)f.offset,(unsigned long)f.expected);updateLines(c,y+22,line);snprintf(line,sizeof(line),"Obtido: %lu",(unsigned long)f.actual);}
 updateLines(c,y+33,line,UI_MUTED);
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
  scenicPanel(c,10,113,220,153);updateLines(c,122,updateInfo.state==updater::State::Downloading&&updateInfo.file==updater::DownloadFile::Firmware?"Baixando firmware...":updateInfo.state==updater::State::Downloading&&updateInfo.file==updater::DownloadFile::Art?"Baixando artes...":updater::stage(updateInfo.state),UI_GOLD);
  snprintf(value,sizeof(value),"%u%%",std::min(100u,updateInfo.progress));panelLabel(c,12,151,216,value,UI_GREEN,3);
  c.fillRect(17,191,206,20,UI_INK);c.drawRect(16,190,208,22,0x8bcb);
  c.fillRect(19,193,202*std::min(100u,updateInfo.progress)/100,16,UI_GREEN);
  updateLines(c,221,updateInfo.message,updateInfo.state==updater::State::Error?UI_RED:UI_WHITE);
  panelLabel(c,12,250,216,"Nao retire o cartao ou a energia.",UI_GOLD);
  return;
 }
 bool install=updateInfo.state==updater::State::Available||updateInfo.canResume;
 const bool showVerify=updateInfo.lastVerify.valid()&&(updateInfo.state==updater::State::Idle||!strncmp(updateInfo.message,"Falha conferir SD",sizeof("Falha conferir SD")-1));
 const bool showWrite=updateInfo.lastWrite.valid&&(updateInfo.state==updater::State::Idle||!strncmp(updateInfo.message,"SD: gravacao falhou",sizeof("SD: gravacao falhou")-1));
 scenicPanel(c,10,113,220,install?96:53);
 if(showVerify){
  if(install){panelLabel(c,12,119,216,"ULTIMA CONFERENCIA (RAM)",UI_GOLD);updateVerifyDetail(c,132,updateInfo.lastVerify);}
  else updateVerifyDetail(c,119,updateInfo.lastVerify);
 }else if(showWrite){
  if(install){panelLabel(c,12,119,216,"ULTIMA FALHA SD (RAM)",UI_GOLD);updateWriteDetail(c,132,updateInfo.lastWrite);}
  else updateWriteDetail(c,119,updateInfo.lastWrite);
 }else{
  updateLines(c,119,*updateInfo.version?updateInfo.version:menu.connected?"Conectado / pronto para verificar":"Conecte o Wi-Fi primeiro",UI_GOLD);
  updateLines(c,142,updateInfo.message,updateInfo.state==updater::State::Error?UI_RED:UI_WHITE);
 }
 if(install){
  if(showVerify){panelLabel(c,12,179,216,"Cod 0: API sem detalhe extra",UI_MUTED);panelLabel(c,12,197,216,"RAM ate desligar; causa em analise",UI_GOLD);}
  else if(showWrite){panelLabel(c,12,179,216,updateInfo.lastWrite.append?"Modo append; reinicio solicitado":"Modo novo; reinicio solicitado",UI_MUTED);panelLabel(c,12,197,216,"RAM ate desligar; E0=sem detalhe",UI_GOLD);}
  else {panelLabel(c,12,171,216,updateInfo.artOnly?"Baixar e reparar as artes?":"Instalar a nova versao e artes?");panelLabel(c,12,190,216,"Instalacao exige cartao SD.",UI_GOLD);}
  button(14,218,212,"Testar conexao");button(14,272,102,"Cancelar");button(124,272,102,updateInfo.canResume?"Continuar":"Instalar");
 }else{
  button(14,173,212,"Testar conexao");button(14,218,212,"Verificar versao");button(14,272,212,"Voltar");
 }
}
