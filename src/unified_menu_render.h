static void drawTravel(ImDrawList* dl,void* ctx){
 if(!travelOpen)return;
 float w=*(float*)((uint8_t*)ctx+8),h=*(float*)((uint8_t*)ctx+12),u=std::min(w,h);
 int tab=vrMenuTab,row=vrMenuRow;
 float font=std::clamp(u*(tab==1?.014f:.021f),18.f,40.f),cx=w*.5f,top=h*.23f;
 float panelWidth=u*(tab==1?.54f:.80f),panelCenter=cx-(tab==1?u*.16f:0.f);
 float left=panelCenter-panelWidth*.5f,right=panelCenter+panelWidth*.5f,bottom=top+font*16.f;
 dl->AddRectFilled({left,top},{right,bottom},wheelStyle::ink,5.f);
 dl->AddRect({left,top},{right,bottom},IM_COL32(55,58,65,255),5.f);
 const char* names[]={"VR","HUD",tr("TELEPORTE","TRAVEL"),tr("ARMAS","WEAPONS"),"CHEATS",tr("CODIGOS","CODES")};
 char title[120];unsigned time=nativeClock.load();
 snprintf(title,sizeof(title),"BULLY VR  /  %s",names[tab]);
 menuText(dl,{left+font*.5f,top+font*.4f},font,wheelStyle::gold,title);
 char clock[16];snprintf(clock,sizeof(clock),"%02u:%02u",time/60,time%60);menuText(dl,{right-font*4.5f,top+font*.4f},font,wheelStyle::gold,clock);
 for(int i=0;i<6;i++){
  float x=left+font*.4f+(right-left-font*.8f)*i/6;
  if(i==tab)dl->AddRectFilled({x-4,top+font*1.8f},{x+(right-left)/6-8,top+font*3.f},wheelStyle::selection,3);
  menuText(dl,{x,top+font*1.95f},font*.72f,i==tab?wheelStyle::selectedText:wheelStyle::muted,names[i]);
 }
 dl->AddLine({left+font*.4f,top+font*3.3f},{right-font*.4f,top+font*3.3f},IM_COL32(55,58,65,255));
 int count=menuRows(tab),first=std::clamp(row-3,0,std::max(0,count-7));
 for(int line=0;line<7&&first+line<count;line++){
  int i=first+line;char label[180]{};bool enabled=true;
  if(tab==0){
   if(i==0)snprintf(label,sizeof(label),tr("Giro: %.0f graus/s","Turn: %.0f degrees/s"),vrTurnSpeed.load());
   if(i==1)snprintf(label,sizeof(label),tr("Suavidade: %.2f s","Smoothing: %.2f s"),vrTurnSmoothing.load());
   if(i==2)snprintf(label,sizeof(label),tr("Altura dos olhos: %.2f m","Eye height: %.2f m"),playerEyeHeight.load());
   if(i==3)snprintf(label,sizeof(label),"%s",tr("Idioma: Portugues","Language: English"));
   if(i==4)snprintf(label,sizeof(label),"%s",tr("Ver controles","Show controls"));
  }else if(tab==1){
   const char* labels[]={tr("Mapa horizontal","Map horizontal"),tr("Mapa vertical","Map vertical"),tr("Missoes horizontal","Missions horizontal"),tr("Missoes vertical","Missions vertical"),tr("Instrucoes horizontal","Instructions horizontal"),tr("Instrucoes vertical","Instructions vertical"),tr("Escala do HUD","HUD scale"),tr("Centralizar tudo (A)","Center all (A)")};
   constexpr uintptr_t offsets[]={0x1741600,0x1741604,0x1741608,0x174160c,0x1741610,0x1741614,0xde4f04};
   if(i<7)snprintf(label,sizeof(label),"%s: %.2f",labels[i],mem<float>(offsets[i]));else snprintf(label,sizeof(label),"%s",labels[i]);
  }else if(tab==2)snprintf(label,sizeof(label),"%s",vrLanguage&&travelDestinations[i].english?travelDestinations[i].english:travelDestinations[i].name);
  else if(tab==3){enabled=wheel::canEquip(i,weaponStock[i],weaponAmmo[i]);snprintf(label,sizeof(label),"%s%s  [%d]",actualWeapon==weaponIds[i]?"> ":"",weaponNames[i],weaponStock[i].load());}
  else if(tab==4){const char* labels[]={tr("Liberar todas as armas","Unlock all weapons"),tr("Equipar punhos","Equip fists"),tr("Ver controles","Show controls")};snprintf(label,sizeof(label),"%s",labels[i]);}
  else {const auto& code=vrCodes[i];snprintf(label,sizeof(label),"[%s] %s",mem<uint8_t>(code.offset)?"ON":"OFF",tr(code.pt,code.en));}
  float y=top+font*(4.1f+line*1.3f);
  if(i==row)dl->AddRectFilled({left+12,y-font*.15f},{right-12,y+font*1.15f},wheelStyle::selection,5);
  ImU32 color=!enabled?wheelStyle::muted:i==row?wheelStyle::selectedText:wheelStyle::paper;
  const char* divider=(tab==0||tab==1)?strrchr(label,':'):nullptr;
  if(divider){char key[160];snprintf(key,sizeof(key),"%.*s",int(divider-label),label);menuText(dl,{left+font*.6f,y},font*.86f,color,key);menuText(dl,{right-font*7.f,y},font*.86f,i==row?wheelStyle::selectedText:wheelStyle::gold,divider+1);}
  else menuText(dl,{left+font*.6f,y},font*.86f,color,label);
 }
 if(tab==1){
  // Small live layout preview beside the HUD controls. Its highlight follows
  // the selected row, and offsets move the corresponding marker in the mock view.
  float pw=u*.36f,ph=u*.225f,px=cx+u*.305f,py=top+u*.015f;
  ImVec2 a{px-pw*.5f,py},b{px+pw*.5f,py+ph};
  dl->AddRectFilled(a,b,IM_COL32(20,29,34,238),8.f);dl->AddRect(a,b,IM_COL32(105,119,122,255),8.f,0,2.f);
  dl->AddRectFilled({a.x+8,a.y+8},{b.x-8,b.y+ph*.19f},IM_COL32(48,65,66,255),4.f);
  menuText(dl,{a.x+pw*.04f,a.y+ph*.055f},font*.52f,wheelStyle::muted,"BULLWORTH ACADEMY  /  PREVIA");
  float mx=b.x-pw*.13f+std::clamp(mem<float>(0x1741600),-3.f,3.f)*pw*.035f;
  float my=a.y+ph*.35f-std::clamp(mem<float>(0x1741604),-3.f,3.f)*ph*.04f;
  float missionX=px+std::clamp(mem<float>(0x1741608),-3.f,3.f)*pw*.035f;
  float missionY=a.y+ph*.31f-std::clamp(mem<float>(0x174160c),-3.f,3.f)*ph*.04f;
  float instructionX=px+std::clamp(mem<float>(0x1741610),-3.f,3.f)*pw*.035f;
  float instructionY=a.y+ph*.72f-std::clamp(mem<float>(0x1741614),-3.f,3.f)*ph*.04f;
  bool selMap=row<2,selMission=row>=2&&row<4,selInstructions=row>=4&&row<6,selAll=row>=6;
  ImU32 mapColor=(selMap||selAll)?wheelStyle::gold:IM_COL32(95,179,153,255);
  dl->AddCircleFilled({mx,my},pw*.055f,mapColor);dl->AddCircle({mx,my},pw*.06f,(selMap||selAll)?wheelStyle::selection:IM_COL32(210,220,216,255),0,3.f);
  dl->AddText(menuFont(dl),font*.42f,{mx-pw*.042f,my+pw*.064f},wheelStyle::paper,"MAPA");
  ImU32 missionColor=(selMission||selAll)?wheelStyle::gold:wheelStyle::paper;
  dl->AddRectFilled({missionX-pw*.25f,missionY},{missionX+pw*.25f,missionY+ph*.13f},IM_COL32(8,12,15,190),4.f);
  if(selMission||selAll)dl->AddRect({missionX-pw*.25f,missionY},{missionX+pw*.25f,missionY+ph*.13f},wheelStyle::selection,4.f,0,3.f);
  menuText(dl,{missionX-pw*.22f,missionY+ph*.025f},font*.48f,missionColor,"MISSAO  /  Encontre o diretor");
  ImU32 instructionColor=(selInstructions||selAll)?wheelStyle::gold:wheelStyle::paper;
  dl->AddRectFilled({instructionX-pw*.32f,instructionY},{instructionX+pw*.32f,instructionY+ph*.14f},IM_COL32(8,12,15,190),4.f);
  if(selInstructions||selAll)dl->AddRect({instructionX-pw*.32f,instructionY},{instructionX+pw*.32f,instructionY+ph*.14f},wheelStyle::selection,4.f,0,3.f);
  menuText(dl,{instructionX-pw*.29f,instructionY+ph*.03f},font*.46f,instructionColor,"INSTRUCAO  /  Fale com o diretor");
  menuText(dl,{px-pw*.46f,b.y+font*.25f},font*.48f,wheelStyle::muted,selMap?"Mapa no canto superior direito":selMission?"Missao centralizada acima":selInstructions?"Instrucao centralizada":selAll?"Ajustes e preview centralizados":"Selecione uma linha do HUD");
 }
 char counter[80];snprintf(counter,sizeof(counter),"%d / %d",row+1,count);
 menuText(dl,{left+font*.5f,top+font*13.6f},font*.72f,wheelStyle::muted,counter);
 menuText(dl,{left+font*.5f,top+font*14.5f},font*.65f,wheelStyle::muted,tr("AD: ABAS | AE: SELECIONAR / AJUSTAR | A: CONFIRMAR","RIGHT: TABS | LEFT: SELECT / ADJUST | A: CONFIRM"));
 menuText(dl,{left+font*.5f,top+font*15.3f},font*.65f,wheelStyle::muted,tr("L3 / B: FECHAR | R3: CONTROLES","L3 / B: CLOSE | R3: CONTROLS"));
 if(travelInProgress)menuText(dl,{left, bottom+font*.3f},font*.8f,wheelStyle::gold,tr("Carregando destino...","Loading destination..."));
 else if(arsenalProgress>=0)menuText(dl,{left,bottom+font*.3f},font*.8f,wheelStyle::gold,tr("Adicionando inventario...","Adding inventory..."));
}
