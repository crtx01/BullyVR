// A visible approach cue driven by the original activity clock/window.
// Never changes class results, timing windows or button input.
function createRecoveryPlusChemistryTiming(main,rec,mem) {
 const stats=Memory.alloc(64);stats.writeByteArray(new Uint8Array(64));
 const memorySlot=Memory.alloc(Process.pointerSize);memorySlot.writePointer(mem);
 const module=new CModule(`
 #include <gum/guminterceptor.h>
 typedef struct {float x,y;} V2;
 extern unsigned char rec_base[];
 extern unsigned int timing_stats[];
 extern void *guest_memory;
 extern unsigned int guest_read(void*,unsigned int);
 extern void rect(void*,const V2*,const V2*,unsigned int,float,int);
 extern void text(void*,void*,float,const V2*,unsigned int,const char*,const char*,float,const void*);
 static const char *labels[]={"X","Y","A","B","X","Y","X","Y","X","L2","L2","L2","X","A","B","A","A+R2","B","A","B","A"};
 static int valid(unsigned int p){return (p>=0x80000000&&p<0x817fe000)||(p>=0x90000000&&p<0x93ffe000);}
 static float rf(unsigned int at){union{unsigned int u;float f;}v;v.u=guest_read(guest_memory,at);return v.f;}
 static void box(void *d,float x,float y,float w,float h,unsigned int c){V2 a={x,y},b={x+w,y+h};rect(d,&a,&b,c,0,0);}
 void render_enter(GumInvocationContext *ic){
  unsigned int p,step,row,action;float t,start,end,w,h,x,y,s,font;void *d;V2 pos;
  unsigned char *sp=(unsigned char*)ic->cpu_context->sp;
  timing_stats[2]++;
  timing_stats[3]=rec_base[0x12bf98];
  if(!rec_base[0x12bf98])return;
  p=guest_read(guest_memory,0x806ec1e8);timing_stats[4]=p;if(!valid(p))return;
  timing_stats[5]=guest_read(guest_memory,p);
  if(guest_read(guest_memory,p)!=0x8058b304)return;
  step=guest_read(guest_memory,p+0x118);timing_stats[6]=step;if(step>85)return;
  row=p+0x120+step*28;action=guest_read(guest_memory,row);if(action>20)return;
  t=rf(p+0x114);start=rf(row+4);end=rf(row+8);
  if(!(start>=0&&end>start&&end<120&&t>=0&&t<=end))return;
  d=sp+0x120;w=*(float*)(sp+0x70);h=*(float*)(sp+0x74);timing_stats[7]=(unsigned int)w;timing_stats[8]=(unsigned int)h;if(w<100||h<100)return;
  y=h*.70f;s=w*.035f;font=s*.55f;
  if(t<start&&start>.001f)x=w*(.72f-.20f*t/start);
  else x=w*(.52f-.04f*(t-start)/(end-start));
  box(d,w*.28f,y+s*.47f,w*.44f,s*.06f,0x885b5149);
  box(d,w*.48f,y-s*.18f,w*.04f,s*1.36f,0x553da766);
  box(d,w*.48f,y-s*.18f,w*.04f,2,0xffd8e8dc);
  box(d,w*.48f,y+s*1.18f,w*.04f,2,0xffd8e8dc);
  box(d,w*.48f,y-s*.18f,2,s*1.36f,0xffd8e8dc);
  box(d,w*.52f-2,y-s*.18f,2,s*1.36f,0xffd8e8dc);
  box(d,x-s*.5f,y,s,s,t>=start?0xff61ae57:0xffdddace);
  pos.x=x-font*(action==16?1.2f:.30f);pos.y=y+(s-font)*.5f;
  text(d,0,font,&pos,0xff181b1d,labels[action],0,0,0);
  timing_stats[0]++;timing_stats[1]=action;
 }
 `,{rec_base:rec.base,timing_stats:stats,guest_memory:memorySlot,
 guest_read:main.base.add(0x5bb354),
 rect:rec.base.add(0xa6128),text:rec.base.add(0xa73c8)});
 // guest_memory is a pointer-valued C external, so give it stable pointer storage.
 const at=main.base.add(0x809170),instruction=Instruction.parse(at);
 if(instruction.mnemonic!=='ldr'||instruction.opStr!=='w8, [x25, #4]')throw Error('Unsupported activity overlay site');
 const hook=Interceptor.attach(at,{onEnter:module.render_enter});
 return {module,stats,memorySlot,report(){return {draws:stats.readU32(),action:stats.add(4).readU32(),probe:Array.from({length:7},(_,i)=>stats.add(8+i*4).readU32())};},dispose(){hook.detach();}};
}
