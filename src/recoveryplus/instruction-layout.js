// Center the complete instruction group using previous-frame bounds, instead
// of recentering every line/icon independently from the last rendered string.
function createRecoveryPlusInstructionLayout(main,rec) {
 const state=Memory.alloc(16);state.writeByteArray(new Uint8Array(16));
 const bounds=Memory.alloc(32);bounds.writeByteArray(new Uint8Array(32));bounds.add(8).writeFloat(1e6);bounds.add(12).writeFloat(-1e6);
 const module=new CModule(`
 #include <gum/guminterceptor.h>
 extern unsigned char main_base[],rec_base[];
 extern unsigned int layout_stats[];
 extern void *tls(void*);
 extern void *format(void);
 extern unsigned char bounds_state[];
 #define frame (*(unsigned long long*)(bounds_state))
 #define lo (*(float*)(bounds_state+8))
 #define hi (*(float*)(bounds_state+12))
 #define center (*(float*)(bounds_state+16))
 #define have (*(int*)(bounds_state+20))
 #define batch (*(void**)(bounds_state+24))
 typedef struct {float *constants;} Call;
 void draw_enter(GumInvocationContext *ic){batch=gum_invocation_context_get_nth_argument(ic,0);}
 void draw_leave(GumInvocationContext *ic){batch=0;}
 void geometry_enter(GumInvocationContext *ic){
  Call *c=gum_invocation_context_get_listener_invocation_data(ic,sizeof(Call));
  c->constants=gum_invocation_context_get_nth_argument(ic,0);
 }
 void geometry_leave(GumInvocationContext *ic){
  unsigned long long now;unsigned int stride;unsigned char *f,*begin,*end,*v,*xf;float *c,*projection,x,a,b,old;
  Call *call=gum_invocation_context_get_listener_invocation_data(ic,sizeof(Call));
  if(!rec_base[0x12bf98]||rec_base[0x12bf9c]||!batch)return;
  if(!*(unsigned char*)tls(rec_base+0x12b868)||*(int*)tls(rec_base+0x12b888)<2)return;
  now=*(unsigned long long*)(rec_base+0x12bf78);
  if(now!=frame){
   if(hi>=lo){center=(lo+hi)*.5f;have=1;}
   else have=0;
   lo=1e6f;hi=-1e6f;frame=now;
  }
  xf=*(unsigned char**)(main_base+0xdd02c0);if(!xf)return;
  projection=(float*)(xf+0x4080);f=format();if(!f)return;
  stride=*(unsigned int*)(f+8);if(stride<12||stride>188)return;
  begin=*(unsigned char**)((unsigned char*)batch+0x10);end=*(unsigned char**)((unsigned char*)batch+8);
  if(!begin||end<=begin||end-begin>stride*1024)return;
  a=1e6f;b=-1e6f;
  for(v=begin;v+stride<=end;v+=stride){x=*(float*)v*projection[0]+projection[1];if(x>-2&&x<2){if(x<a)a=x;if(x>b)b=x;}}
  if(b>=a&&b-a<3){if(a<lo)lo=a;if(b>hi)hi=b;}
  if(!have)return;
  c=call->constants;old=*(float*)tls(rec_base+0x12b8a8);
  c[0x32]+=(old-center)*c[0x2c];c[0x3a]+=(old-center)*c[0x2c];
  ((unsigned char*)c)[0x130]=1;layout_stats[0]++;
 }
 `,{main_base:main.base,rec_base:rec.base,layout_stats:state,bounds_state:bounds,tls:rec.base.add(0x11b0a0),format:main.base.add(0x87b11c)});
 const draw=Interceptor.attach(rec.base.add(0x6316c),{onEnter:module.draw_enter,onLeave:module.draw_leave});
 const geometry=Interceptor.attach(rec.base.add(0x62dfc),{onEnter:module.geometry_enter,onLeave:module.geometry_leave});
 return {module,state,bounds,report(){return {adjustments:state.readU32(),center:bounds.add(16).readFloat(),have:bounds.add(20).readS32()};},dispose(){geometry.detach();draw.detach();}};
}
