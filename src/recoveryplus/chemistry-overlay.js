// Explicit timing target composed on the active game framebuffer. Native
// activity time and input windows remain unchanged.
function createRecoveryPlusChemistryOverlay(main,rec,mem) {
 const slot=Memory.alloc(Process.pointerSize);slot.writePointer(mem);
 const state=Memory.alloc(8);state.writeByteArray(new Uint8Array(8));
 const symbols={main_base:main.base,rec_base:rec.base,guest_memory:slot,overlay_stats:state,guest_read:main.base.add(0x5bb354)};
 for(const name of ['glGetIntegerv','glGetFloatv','glGetBooleanv','glIsEnabled','glEnable','glDisable','glScissor','glClearColor','glColorMask','glClear'])symbols[name]=Module.getGlobalExportByName(name);
 const module=new CModule(`
 #include <gum/guminterceptor.h>
 extern unsigned char main_base[],rec_base[];
 extern void *guest_memory;
 extern unsigned int overlay_stats[];
 extern unsigned int guest_read(void*,unsigned int);
 extern void glGetIntegerv(unsigned int,int*);extern void glGetFloatv(unsigned int,float*);extern void glGetBooleanv(unsigned int,unsigned char*);
 extern unsigned char glIsEnabled(unsigned int);extern void glEnable(unsigned int);extern void glDisable(unsigned int);
 extern void glScissor(int,int,int,int);extern void glClearColor(float,float,float,float);extern void glColorMask(unsigned char,unsigned char,unsigned char,unsigned char);extern void glClear(unsigned int);
 static float rf(unsigned int at){union{unsigned int u;float f;}v;v.u=guest_read(guest_memory,at);return v.f;}
 static void box(int x,int y,int w,int h,float r,float g,float b){glScissor(x,y,w,h);glClearColor(r,g,b,1);glClear(0x4000);}
 static void glyph(int x,int y,int pixel,unsigned int bits){int i;for(i=0;i<15;i++)if(bits&(1u<<(14-i)))box(x+(i%3)*pixel,y+(4-i/3)*pixel,pixel,pixel,.08f,.09f,.10f);}
 void leave(GumInvocationContext*ic){
  unsigned int p,step,row,action,bits;unsigned char *xf,mask[4],enabled;int view[4],clip[4],x,y,size,line,moving,pixel;float color[4],t,start,end,k;
  if(!rec_base[0x12bf98]||!rec_base[0x12c094])return;
  p=guest_read(guest_memory,0x806ec1e8);if(!((p>=0x80000000&&p<0x817fe000)||(p>=0x90000000&&p<0x93ffe000)))return;
  if(guest_read(guest_memory,p)!=0x8058b304)return;
  step=guest_read(guest_memory,p+0x118);if(step>85)return;
  row=p+0x120+step*28;action=guest_read(guest_memory,row);if(action>20)return;
  t=rf(p+0x114);start=rf(row+4);end=rf(row+8);if(!(t>0&&t<end&&end>start&&end<120))return;
  xf=*(unsigned char**)(main_base+0xdd02c0);if(!xf||!*(int*)(xf+0x4098))return;
  glGetIntegerv(0x0ba2,view);if(view[2]<100||view[3]<100)return;
  glGetIntegerv(0x0c10,clip);glGetFloatv(0x0c22,color);glGetBooleanv(0x0c23,mask);enabled=glIsEnabled(0x0c11);
  glEnable(0x0c11);glColorMask(1,1,1,1);
  x=view[0]+view[2]/2;y=view[1]+view[3]*.28f;size=view[3]*.045f;if(size<12)size=12;line=size/12;if(line<1)line=1;
  box(x-size,y-size,size*2,line,.9f,.86f,.7f);box(x-size,y+size-line,size*2,line,.9f,.86f,.7f);
  box(x-size,y-size,line,size*2,.9f,.86f,.7f);box(x+size-line,y-size,line,size*2,.9f,.86f,.7f);
  if(t<start){k=(start-t)/(.5f+start);if(k>1)k=1;moving=x+size+(int)(k*view[2]*.25f);}
  else moving=x+size-(int)((t-start)/(end-start)*size*2);
  box(moving-size/3,y-size/2,size*2/3,size,t>=start?.35f:.95f,t>=start?.9f:.9f,t>=start?.45f:.75f);
  pixel=size/9;if(pixel<1)pixel=1;
  /* 3x5 glyphs: X, Y, A, B. The original prompt remains visible too. */
  bits=0x5aad;
  if(action==1||action==5||action==7)bits=0x5a92;
  else if(action==2||action==13||action==15||action==16||action==18||action==20)bits=0x2bed;
  else if(action==3||action==14||action==17||action==19)bits=0x6bae;
  if(action>=9&&action<=11){glyph(moving-4*pixel,y-2*pixel,pixel,0x4927);glyph(moving,y-2*pixel,pixel,0x73e7);}
  else glyph(moving-pixel,y-2*pixel,pixel,bits);
  glClearColor(color[0],color[1],color[2],color[3]);glColorMask(mask[0],mask[1],mask[2],mask[3]);glScissor(clip[0],clip[1],clip[2],clip[3]);if(!enabled)glDisable(0x0c11);
  overlay_stats[0]++;overlay_stats[1]=view[2];
 }
 `,symbols);
 const hook=Interceptor.attach(rec.base.add(0x64d88),{onLeave:module.leave});
 return {module,slot,state,report(){return {draws:state.readU32(),viewport:state.add(4).readU32()};},dispose(){hook.detach();}};
}
