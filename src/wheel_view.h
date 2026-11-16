// The eight-slot wheel uses the original Bully gold, ink and HUD artwork.
namespace wheelStyle {
constexpr ImU32 ink=IM_COL32(15,20,22,238),gold=IM_COL32(222,177,74,255),paper=IM_COL32(244,236,213,255),muted=IM_COL32(135,140,141,255),error=IM_COL32(255,150,120,255);
}
static void centeredText(ImDrawList* dl,ImVec2 at,float size,ImU32 color,const char* text,float wrap=0){
 auto* font=dl->_Data->Font;auto extent=font->CalcTextSizeA(size,10000.f,wrap,text);
 dl->AddText(font,size,{at.x-extent.x*.5f,at.y-extent.y*.5f},color,text,nullptr,wrap);
}
static void drawWheel(ImDrawList* dl,void* ctx,const Snapshot&){
 if(!wheelOpen)return;
 auto* raw=(uint8_t*)ctx;float w=*(float*)(raw+8),h=*(float*)(raw+12),unit=std::min(w,h);
 ImVec2 center(w*.5f,h*.48f);float radius=unit*.24f,cell=radius*.24f,font=std::clamp(unit*.020f,18.f,42.f);
 auto page=wheel::pageAt(wheelPage);int selected=std::clamp(wheelHover.load(),0,weaponCount-1);
 dl->AddCircleFilled(center,radius*1.34f,wheelStyle::ink,64);
 dl->AddCircle(center,radius*1.34f,wheelStyle::gold,64,2);
 char header[96];snprintf(header,sizeof(header),"%s  %d/%d",wheel::groupNames[page.group],page.number+1,page.pages);
 centeredText(dl,{center.x,center.y-radius*1.46f},font,wheelStyle::paper,header);
 for(int slot=0;slot<page.count;slot++){
  int k=page.items[slot],stock=weaponStock[k];bool owned=stock>0,ready=wheel::canEquip(k,stock,weaponAmmo[k]),hover=k==selected,equipped=equipmentKnown&&weaponIds[k]==actualWeapon;
  float angle=slot*6.283185307f/page.count;ImVec2 p(center.x+std::sin(angle)*radius,center.y-std::cos(angle)*radius);
  dl->AddCircleFilled(p,cell,hover?IM_COL32(86,65,30,255):IM_COL32(29,35,37,255),32);
  if(hover||equipped)dl->AddCircle(p,cell,hover?wheelStyle::gold:wheelStyle::paper,32,hover?4.f:2.f);
  auto* icon=*weaponIcons[k]?texture(weaponIcons[k]):nullptr;
  if(icon)dl->AddImage(ImTextureRef((ImTextureID)(uintptr_t)icon),{p.x-cell*.70f,p.y-cell*.80f},{p.x+cell*.70f,p.y+cell*.60f},{0,0},{1,1},ready?IM_COL32_WHITE:IM_COL32(100,100,100,95));
  else centeredText(dl,{p.x,p.y-cell*.12f},font*.65f,owned?wheelStyle::paper:wheelStyle::muted,weaponNames[k],cell*1.75f);
  char count[32];if(!owned)snprintf(count,sizeof(count),"SEM ITEM");else if(!ready)snprintf(count,sizeof(count),"SEM MUNICAO");else if(k==0)snprintf(count,sizeof(count),"PUNHOS");else if(wheel::ammoId(k)>0)snprintf(count,sizeof(count),"%d municoes",weaponAmmo[k].load());else snprintf(count,sizeof(count),"x%d",stock);
  centeredText(dl,{p.x,p.y+cell*.78f},font*.54f,owned?wheelStyle::gold:wheelStyle::muted,count);
 }
 centeredText(dl,{center.x,center.y-font},font,wheelStyle::paper,weaponNames[selected],radius*1.32f);
 const char* status=pendingWeapon!=-2?"Equipando...":weaponStock[selected]<=0?"Voce ainda nao possui este item":!wheel::canEquip(selected,weaponStock[selected],weaponAmmo[selected])?"Sem municao":now()<equipFailedUntil?"Nao foi possivel equipar":actualWeapon==weaponIds[selected]?"Equipado":"A: equipar";
 centeredText(dl,{center.x,center.y+font*.9f},font*.62f,now()<equipFailedUntil?wheelStyle::error:wheelStyle::gold,status,radius*1.25f);
 char current[100];snprintf(current,sizeof(current),"Na mao: %s",equipmentKnown?weaponNames[std::clamp(weaponCurrent.load(),0,weaponCount-1)]:"verificando inventario");
 centeredText(dl,{center.x,center.y+radius*1.45f},font*.72f,wheelStyle::paper,current);
 centeredText(dl,{center.x,center.y+radius*1.60f},font*.65f,wheelStyle::paper,"AD: escolher   A: equipar   R3: fechar");
 centeredText(dl,{center.x,center.y+radius*1.73f},font*.65f,wheelStyle::paper,"X: pagina anterior   B: proxima pagina");
}
