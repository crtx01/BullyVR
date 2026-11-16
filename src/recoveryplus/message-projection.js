// Native tutorial rectangles are centered in guest UI coordinates. Do not
// recenter each subsequent text/icon batch again from its stale last-line box.
function createRecoveryPlusMessageProjection(rec){
 const stats=Memory.alloc(4);stats.writeU32(0);
 const module=new CModule(`
 #include <gum/guminterceptor.h>
 extern unsigned char rec_base[];
 extern unsigned int message_stats[];
 extern void *tls(void*);
 void enter(GumInvocationContext*ic){
  if(!rec_base[0x12bf98]||rec_base[0x12bf9c])return;
  if(!*(unsigned char*)tls(rec_base+0x12b868))return;
  if(*(int*)tls(rec_base+0x12b888)<2)return;
  *(float*)tls(rec_base+0x12b8a8)=0;
  message_stats[0]++;
 }
 `,{rec_base:rec.base,message_stats:stats,tls:rec.base.add(0x11b0a0)});
 const hook=Interceptor.attach(rec.base.add(0x62dfc),{onEnter:module.enter});
 return {module,stats,report(){return {adjustments:stats.readU32()};},dispose(){hook.detach();}};
}
