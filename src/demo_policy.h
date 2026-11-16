// RB7E54 mission-entry gates. Three native entry paths share mission indices:
// 0 is the engine's free-roam dummy, 4 intro, 5 the combat tutorial.
static std::atomic<bool> demoComplete{false},demoGateReady{false};
static void updateDemoPolicy(){
#if BULLY_DEMO
 constexpr uint32_t bank=0x806390bc;
 uint32_t records=gameRead(bank+0x4b8),stats=gameRead(bank+0x690);
 if(!gameAddress(records)||!gameAddress(stats)||gameRead(bank+0x74c)!=500)return;
 // Reject an unexpected disc layout rather than applying indices blindly.
 uint32_t path=gameRead(records+5*0x78+0x1c);
 if(!gameAddress(path&~3u))return;
 char name[40]{};
 for(int i=0;i<39;i++)name[i]=char(gameRead((path+i)&~3u)>>(24-((path+i)&3)*8));
 if(strcmp(name,"chap1/1_02.lua"))return;
 if(!installContactBridge())return;
 uint32_t payload=contactMailbox-contactMailboxOffset;
 // Exploration preview: no campaign or class entry, including the intro.
 bool complete=true;demoComplete=false;
 constexpr uint32_t sites[]={0x8038b124,0x8038b834,0x8038b8d0};
 constexpr uint32_t originals[][4]={{0x9421ffc0,0x7c0802a6,0x90010044,0x39610040},
  {0x9421fff0,0x7c0802a6,0x90010014,0x93e1000c},
  {0x90830018,0x38e00001,0x98e3001c,0x38800000}};
 bool ready=true;static bool cachePending=false;
 for(int i=0;i<3;i++){
  uint32_t gate=payload+demoGateOffset+i*52;
  auto branch=[](uint32_t from,uint32_t to){return 0x48000000u|((to-from)&0x03fffffcu);};
  uint32_t jump=branch(sites[i],gate),current=gameRead(sites[i]);
  if(current!=jump){
   bool matches=true;for(int j=1;j<4;j++)matches&=gameRead(sites[i]+j*4)==originals[i][j];
   int32_t displacement=int32_t(current<<6)>>6;uint32_t prior=sites[i]+displacement;
   bool previousOwn=(current&0xfc000003u)==0x48000000u&&prior>=0x80001800u&&prior<0x80003000u;
   matches&=current==originals[i][0]||previousOwn;
   // A state load may retain an earlier copy of our gate in the same low RAM.
   if(!matches){ready=false;continue;}
  }
  uint32_t first=complete?0x2c040000u:0x2c040004u,second=complete?0x2c040000u:0x2c040005u;
  if(gameRead(gate+8)!=first||gameRead(gate+16)!=second||gameRead(gate+48)!=branch(gate+48,sites[i]+16)||current!=jump){
   gameWrite(gate+8,first);gameWrite(gate+16,second);
   gameWrite(gate+48,branch(gate+48,sites[i]+16));gameWrite(sites[i],jump);cachePending=true;
  }
 }
 if(cachePending&&nativeRequest(6,gameRead(0x80624204),0,{}))cachePending=false;
 demoGateReady=ready&&!cachePending;
#endif
}
