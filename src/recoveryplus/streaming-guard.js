// Serialize streaming operations. The original engine can otherwise accept
// equip requests while the world loader owns the player and inventory.
function createRecoveryPlusStreamingGuard(rec){
 const state=Memory.alloc(16);state.writeByteArray(new Uint8Array(16));
 const deadline=Memory.alloc(8);deadline.writeDouble(0);
 const module=new CModule(`
 #include <gum/guminterceptor.h>
 typedef struct {long sec,nsec;} Time;
 extern unsigned char rec_base[];
 extern unsigned int guard_stats[];
 extern double equipment_deadline[];
 extern int clock_gettime(int,Time*);
 static double now(void){Time t;clock_gettime(1,&t);return (double)t.sec+(double)t.nsec*.000000001;}
 void equip_enter(GumInvocationContext*ic){
  int index=(int)(long)gum_invocation_context_get_nth_argument(ic,0);
  double t=now();int busy=rec_base[0x12c070]||*(int*)(rec_base+0x12b754)!=-1;
  busy=busy||!rec_base[0x12bf98]||rec_base[0x12bf9c]||rec_base[0x12c094];
  if(index<0||index>=92)return;
  if(busy||*(int*)(rec_base+0x12b734)!=-2||t<equipment_deadline[0]){
   /* requestWeapon validates its index before any array lookup. */
   gum_invocation_context_replace_nth_argument(ic,0,(void*)-1);
   guard_stats[0]++;return;
  }
  equipment_deadline[0]=t+.8;
  guard_stats[1]++;
 }
 void input_enter(GumInvocationContext*ic){
  if(!rec_base[0x12c070]&&*(int*)(rec_base+0x12b754)==-1)return;
  *(int*)(rec_base+0x12b738)=-1;
  guard_stats[2]++;
 }
 `,{rec_base:rec.base,guard_stats:state,equipment_deadline:deadline,clock_gettime:Module.getGlobalExportByName('clock_gettime')});
 const equip=Interceptor.attach(rec.base.add(0x5b6d4),{onEnter:module.equip_enter});
 const input=Interceptor.attach(rec.base.add(0x5f884),{onEnter:module.input_enter});
 return {module,state,deadline,report(){return {blockedEquip:state.readU32(),acceptedEquip:state.add(4).readU32(),travelFrames:state.add(8).readU32()};},dispose(){equip.detach();input.detach();}};
}
