// Additive review for the extracted Recovery+ r22 binary only.
// Does not replace the camera, aim, projection, draw-distance or chemistry hooks.
function createRecoveryPlusReview(main,rec,read,mem){
 const expected=rec.getExportByName('BullyRecovery_TestControl').sub(rec.base).toInt32();
 if(expected!==0x5acb0)throw Error('Unsupported Recovery+ binary');
 const root='/data/user/0/org.dolphinemu.dolphinemu.quest.bully.recover2/files/';
 const request=new NativeFunction(rec.base.add(0x739bc),'uchar',['uint32','uint32','float','float','float','float','int']);
 const reg=new NativeFunction(main.getExportByName('Java_org_dolphinemu_dolphinemu_features_input_model_InputOverrider_registerWii'),'void',['pointer','pointer','int']);
 const set=new NativeFunction(main.getExportByName('Java_org_dolphinemu_dolphinemu_features_input_model_InputOverrider_setControlState'),'void',['pointer','pointer','int','int','double']);
 const clear=new NativeFunction(main.getExportByName('Java_org_dolphinemu_dolphinemu_features_input_model_InputOverrider_clearControlState'),'void',['pointer','pointer','int','int']);
 reg(ptr(0),ptr(0),0);
 const valid=(p,bytes=4)=>Number.isInteger(p)&&bytes>=4&&((p>=0x80000000&&p+bytes<=0x81800000)||(p>=0x90000000&&p+bytes<=0x94000000))&&(p&3)===0;
 let raw={lt:false,y:false,x:false,menu:false},errors='',frames=0,overrides=false,lastOverride='';
 let fists=null,fistRequests=0,fistRetries=0,fistCompleted=0,fistTimeouts=0,lastFistResult=null;
 const nativeIds=Array.from({length:92},(_,i)=>rec.base.add(0x3204c+i*4).readS32());
 let armedIntent=false;
 function releaseControls(){
  if(!overrides)return;
  for(const id of [20,21,31,32])clear(ptr(0),ptr(0),0,id);
  overrides=false;lastOverride='';
 }
 function gameplay(){return !!rec.base.add(0x12bf98).readU8()&&!rec.base.add(0x12bf9c).readU8()&&!rec.base.add(0x12c094).readU8()&&!rec.base.add(0x12c070).readU8()&&rec.base.add(0x12b754).readS32()===-1;}
 function fistState(){
  const player=read(mem,0x80624204);if(!valid(player,0x16bc))return null;
  const held=read(mem,player+0x1ec),inventory=read(mem,player+0x300);
  if((held!==0&&!valid(held,0x114))||!valid(inventory,0x24))return null;
  const packed=held===0?0xffff:read(mem,held+0x110),id=(packed<<16)>>16;
  return {player,id,cached:read(mem,player+0x16b8)|0,desired:read(mem,inventory+0x20)|0};
 }
 const equip=Interceptor.attach(rec.base.add(0x5b6d4),{onEnter(args){
  const index=args[0].toInt32();
  if(index===0&&gameplay()){
   fists={started:Date.now(),sent:0,serial:0,stable:0};fistRequests++;
   // A queued weapon must not re-equip after the fists request completes.
   rec.base.add(0x12b738).writeS32(-1);
  }else if(index>0&&index<92){fists=null;armedIntent=true;}
 }});
 const early=Interceptor.attach(rec.base.add(0x5f884),{onEnter(){try{
  const p=this.context.x19;
  raw={lt:!!p.add(0x44).readU8()||p.add(0x48).readFloat()>.25,y:!!p.add(0x42).readU8(),x:!!p.add(0x41).readU8(),menu:!!p.add(0x43).readU8()};
  const item=rec.base.add(0x12b710).readS32(),pending=rec.base.add(0x12b734).readS32();
  armedIntent=item!==-1||(pending!==-2&&pending!==-1);
  // A held focus button must reach the native selector continuously.
  if(gameplay()){rec.base.add(0x12c08c).writeU8(0);rec.base.add(0x12c088).writeU8(0);}
  errors='';
 }catch(e){errors=String(e);}}});
 const late=Interceptor.attach(rec.base.add(0x60b68),{onEnter(){try{
  frames++;
  const p=this.context.x19;
  const ui=rec.base.add(0x12bc2c).readU8()||rec.base.add(0x12bc3c).readU8();
  if(!gameplay()||ui){
   releaseControls();
   // A review-layer unequip request must never resume in a different scene.
   if(!gameplay())fists=null;
   return;
  }
  // Override the Wii actions, not the OpenXR poses or head/controller basis.
  // Native C = focus + selector, Z = grab; holding is continuous, not a pulse.
  const signature=[raw.lt,raw.y,raw.x||raw.menu].join(':');
  if(!overrides||signature!==lastOverride){
   set(ptr(0),ptr(0),0,31,raw.lt?1:0);
   set(ptr(0),ptr(0),0,32,raw.y?1:0);
   set(ptr(0),ptr(0),0,20,(raw.x||raw.menu)?1:0);
   set(ptr(0),ptr(0),0,21,0);overrides=true;lastOverride=signature;
  }
  const item=rec.base.add(0x12b710).readS32(),index=rec.base.add(0x12c090).readS32();
  const ranged=item!==-1&&item!==437&&index>=0&&index<92&&!rec.base.add(0x321bc+index).readU8();
  if(ranged||(armedIntent&&item===-1)){
   p.add(0x212).writeU8(0);p.add(0x213).writeU8(0);
   p.add(0x1f8).writeU8(0);p.add(0x1f9).writeU8(0);
  }
  if(armedIntent&&item===-1){p.add(0xcc).writeU8(0);p.add(0xd0).writeFloat(0);}
  // The original early trampoline aliases Menu onto Y. Restore the actual Y
  // in the output snapshot for downstream helpers after native menu handling.
  p.add(0x42).writeU8(raw.y?1:0);
 }catch(e){errors=String(e);}}});
  const cpu=Interceptor.attach(main.base.add(0x53e4dc),{onEnter(){try{
  if(!gameplay()){fists=null;return;}
  if(!fists)return;
  const now=Date.now();
  if(now-fists.started>6000){lastFistResult={result:'timeout'};fistTimeouts++;fists=null;return;}
  const state=fistState();if(!state)return;
  const mailbox=rec.base.add(0x12c11c).readU32();if(!valid(mailbox,0x34))return;
  rec.base.add(0x12b738).writeS32(-1);
  // Always dispatch an explicit native unequip, even if actualWeapon already
  // reads -1 while a previous equip command is still in flight.
  if((!fists.sent||now-fists.sent>450)&&read(mem,mailbox)===0){
   if(request(2,state.player,0,0,0,0,-1)){
    fists.sent=now;fists.serial=rec.base.add(0x12c120).readU32();fistRetries++;
   }
  }
  const ack=fists.serial&&read(mem,mailbox+0x30)===fists.serial;
  const clear=state.id===-1&&state.cached===-1&&(state.desired===0||state.desired===-1);
  if(ack&&clear){
   if(++fists.stable>=3){rec.base.add(0x12b734).writeS32(-2);lastFistResult={result:'confirmed',...state};fistCompleted++;fists=null;}
  }else fists.stable=0;
 }catch(e){errors=String(e);}}});
 const timer=setInterval(()=>{
  try{File.writeAllText(root+'recovery-review-status.json',JSON.stringify(report()));}catch(e){}
 },500);
 function report(){return {build:'recoveryplus-review-r23',scope:'controls and fists only; original aim/culling preserved',frames,raw,overrides,errors,fists:!!fists,fistRequests,fistRetries,fistCompleted,fistTimeouts,lastFistResult};}
 return {report,dispose(){clearInterval(timer);releaseControls();for(const hook of [equip,early,late,cpu])hook.detach();}};
}
