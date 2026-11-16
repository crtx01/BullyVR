// Reuse the game's own untextured HUD quad pipeline for an explicit timing
// target. Input acceptance and class progression stay entirely native.
function createRecoveryPlusChemistryWindow(main,rec,mem) {
 const memorySlot=Memory.alloc(Process.pointerSize);memorySlot.writePointer(mem);
 const stats=Memory.alloc(16);stats.writeByteArray(new Uint8Array(16));
 const callState=Memory.alloc(256);callState.writeByteArray(new Uint8Array(256));
 const frameState=Memory.alloc(8);frameState.writeByteArray(new Uint8Array(8).fill(255));
 const module=new CModule(`
 #include <gum/guminterceptor.h>
 #include <string.h>
 typedef unsigned long U;
 extern unsigned char main_base[],rec_base[];
 extern unsigned int window_stats[];
 extern void *guest_memory;
 extern unsigned int guest_read(void*,unsigned int);
 extern void *native_format(void);
 typedef void (*Draw)(void*,void*,void*,void*,U,U,unsigned int,void*);
 typedef struct {void *a[8];unsigned char saved[64];unsigned char *begin;float lx,hx,ly,hy,z;int active;} Call;
 extern unsigned char window_call[];
 extern U window_frame[];
 #define frame window_frame[0]
 static float rf(unsigned int at){union{unsigned int u;float f;}v;v.u=guest_read(guest_memory,at);return v.f;}
 static void box(Call*c,float x,float y,float w,float h,unsigned int color){
  int i;float *v;unsigned char *p;Draw draw=*(Draw*)(rec_base+0x12bc00);
  for(i=0;i<4;i++){p=c->begin+i*16;v=(float*)(c->saved+i*16);
   *(float*)p=v[0]<(c->lx+c->hx)*.5f?x:x+w;
   *(float*)(p+4)=v[1]<(c->ly+c->hy)*.5f?y:y+h;
   *(unsigned int*)(p+12)=color;
  }
  draw(c->a[0],c->a[1],c->a[2],c->a[3],(U)c->a[4],(U)c->a[5],(unsigned int)(U)c->a[6],c->a[7]);
 }
 void enter(GumInvocationContext*ic){
  unsigned char *xf,*bp,*f,*end,*v;unsigned int p,step,row,stages,s;int i;float x,y,t,start,finish;
  Call*c=(Call*)window_call;c->active=0;
  if(!rec_base[0x12bf98]||!rec_base[0x12c094])return;
  if(frame==*(U*)(rec_base+0x12bf78))return;
  p=guest_read(guest_memory,0x806ec1e8);
  if(!((p>=0x80000000&&p<0x817fe000)||(p>=0x90000000&&p<0x93ffe000)))return;
  if(guest_read(guest_memory,p)!=0x8058b304)return;
  step=guest_read(guest_memory,p+0x118);if(step>85)return;
  row=p+0x120+step*28;if(guest_read(guest_memory,row)>20)return;
  t=rf(p+0x114);start=rf(row+4);finish=rf(row+8);
  if(!(t>=0&&start>=0&&finish>start&&finish<120&&t<finish))return;
  xf=*(unsigned char**)(main_base+0xdd02c0);if(!xf||!*(int*)(xf+0x4098))return;
  bp=*(unsigned char**)(main_base+0xdd0e28);if(!bp)return;
  stages=(*(unsigned int*)bp>>10)&15;
  for(s=0;s<=stages;s++)if(((*(unsigned int*)(bp+0xa0+(s>>1)*4)>>((s&1)*12))&64))return;
  f=native_format();if(!f||*(unsigned int*)(f+8)!=16)return;
  for(i=0;i<8;i++)c->a[i]=gum_invocation_context_get_nth_argument(ic,i);
  c->begin=*(unsigned char**)((unsigned char*)c->a[0]+0x10);end=*(unsigned char**)((unsigned char*)c->a[0]+8);
  if(!c->begin||end-c->begin!=64)return;
  memcpy(c->saved,c->begin,64);c->lx=c->ly=1e6f;c->hx=c->hy=-1e6f;
  for(i=0;i<4;i++){v=c->begin+i*16;x=*(float*)v;y=*(float*)(v+4);if(x<c->lx)c->lx=x;if(x>c->hx)c->hx=x;if(y<c->ly)c->ly=y;if(y>c->hy)c->hy=y;}
  if(!(c->hx>c->lx&&c->hy>c->ly))return;
  c->z=start-t; /* approach from the right; centered when input window opens */
  c->active=1;frame=*(U*)(rec_base+0x12bf78);window_stats[1]++;
 }
 void leave(GumInvocationContext*ic){
  Call*c=(Call*)window_call;float x;
  if(!c->active)return;
  box(c,-.023f,-.165f,.046f,.002f,0xffdedede);
  box(c,-.023f,-.119f,.046f,.002f,0xffdedede);
  box(c,-.023f,-.165f,.002f,.048f,0xffdedede);
  box(c,.021f,-.165f,.002f,.048f,0xffdedede);
  x=c->z*.11f;if(x>.28f)x=.28f;if(x<-.20f)x=-.20f;
  box(c,x-.009f,-.151f,.018f,.020f,c->z<=0?0xff55c87a:0xffe6dfce);
  memcpy(c->begin,c->saved,64);window_stats[0]++;
 }
 `,{main_base:main.base,rec_base:rec.base,window_stats:stats,window_call:callState,window_frame:frameState,guest_memory:memorySlot,
 guest_read:main.base.add(0x5bb354),native_format:main.base.add(0x87b11c)});
 const hook=Interceptor.attach(rec.base.add(0x6316c),{onEnter:module.enter,onLeave:module.leave});
 return {module,stats,memorySlot,callState,frameState,report(){return {draws:stats.readU32(),eligible:stats.add(4).readU32()};},dispose(){hook.detach();}};
}
