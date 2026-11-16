static std::atomic<int> vrMenuTab{0},vrMenuRow{0};
struct MenuCode{const char* pt;const char* en;uintptr_t offset;};
static constexpr MenuCode vrCodes[]={
 {"Vida infinita","Infinite health",0x1741618},
 {"Sem procurado","Never wanted",0x1741619},
 {"Dinheiro infinito","Infinite money",0x174161a},
 {"Municao infinita","Infinite ammunition",0x1a46820},
 {"Lojas gratuitas","Free shops",0x1a46821},
 {"Congelar horario","Freeze time",0x174161b},
 {"Ingressos infinitos","Infinite tickets",0x174161c},
 {"Entrada no parque","Carnival admission",0x1741620}};
static constexpr int codeCount=sizeof(vrCodes)/sizeof(*vrCodes);
static int menuRows(int tab){return tab==0?5:tab==1?8:tab==2?travelCount:tab==3?weaponCount:tab==4?3:codeCount;}
static bool unifiedMenuInput(Snapshot& s){
 auto& l=s.c[0];auto& r=s.c[1];
 static bool oldL3=false,oldR3=false,oldA=false,oldB=false;
 static MenuNavigation navigation,tabs,adjust;
 bool key=l.stick,accept=r.primary,back=r.secondary,r3=r.stick;
 if(key&&!oldL3&&!travelInProgress){travelOpen=!travelOpen;helpOpen=false;mem<uint8_t>(0x170b4ef)=0;navigation.reset();tabs.reset();adjust.reset();}
 if(r3&&!oldR3){helpOpen=!helpOpen;travelOpen=false;}
 oldL3=key;oldR3=r3;l.stick=r.stick=false;settingsOpen=false;wheelOpen=false;
 bool consumed=travelOpen||helpOpen;
 if(travelOpen){
  mem<uint8_t>(0x170b4ef)=0;freeAim=false;
  int tab=vrMenuTab,row=vrMenuRow;
  int changeTab=-tabs.step(r.x,now());
  if(changeTab){tab=(tab+changeTab+6)%6;row=0;vrMenuTab=tab;}
  int next=navigation.step(std::abs(l.y)>std::abs(r.y)?l.y:r.y,now());
  row=(row+next+menuRows(tab))%menuRows(tab);vrMenuRow=row;
  int change=-adjust.step(l.x,now());bool clicked=accept&&!oldA;
  if(tab==0){
   if(change){
    if(row==0)vrTurnSpeed=std::clamp(vrTurnSpeed.load()+change*5.f,15.f,120.f);
    if(row==1)vrTurnSmoothing=std::clamp(vrTurnSmoothing.load()+change*.02f,.03f,.4f);
    if(row==2)playerEyeHeight=std::clamp(playerEyeHeight.load()+change*.02f,1.2f,2.05f);
    if(row==3)vrLanguage=1-vrLanguage.load();
    saveVrSettings();
   }
   if(clicked&&row==3){vrLanguage=1-vrLanguage.load();saveVrSettings();}
   if(clicked&&row==4){helpOpen=true;travelOpen=false;}
  }else if(tab==1){
   constexpr uintptr_t offsets[]={0x1741600,0x1741604,0x1741608,0x174160c,0x1741610,0x1741614,0xde4f04};
   if(change&&row<7){float& value=mem<float>(offsets[row]);value=std::clamp(value+change*.05f,row==6?.4f:-3.f,row==6?2.f:3.f);saveVrSettings();}
   if(clicked&&row==7){resetMessageOffsets();mem<float>(0x1741600)=mem<float>(0x1741604)=0.f;saveVrSettings();}
  }else if(clicked&&tab==2&&!travelInProgress&&travelRequest==-1)travelRequest=row;
  else if(clicked&&tab==3)requestWeapon(row);
  else if(clicked&&tab==4){if(row==0)grantArsenal=true;if(row==1)requestWeapon(0);if(row==2){helpOpen=true;travelOpen=false;}}
  else if(clicked&&tab==5){const auto& code=vrCodes[row];mem<uint8_t>(code.offset)=!mem<uint8_t>(code.offset);mem<uint8_t>(0x1a4680c)=1;}
  if(back&&!oldB&&!travelInProgress)travelOpen=false;
 }else if(helpOpen&&back&&!oldB)helpOpen=false;
 oldA=accept;oldB=back;
 if(consumed){
  l.x=l.y=r.x=r.y=0;l.primary=l.secondary=r.primary=r.secondary=l.trigger=r.trigger=l.squeeze=r.squeeze=false;
  l.tv=r.tv=l.sv=r.sv=l.sf=r.sf=0;
 }
 return consumed;
}
