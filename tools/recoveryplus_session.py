"""Inspect/control the extracted Recovery+ without replacing its native engine."""
import os,json,subprocess,time,sys
from pathlib import Path
import frida
ROOT=Path(__file__).resolve().parents[1]
PACKAGE=os.environ.get('BULLY_REVIEW_PACKAGE','org.dolphinemu.dolphinemu.quest.bully.recover2')
SERIAL=os.environ.get('ANDROID_SERIAL','')
BRIDGE=r'''
const main=Process.getModuleByName('libmain.so'),rec=Process.getModuleByName('libbully_recovery.so');
const mem=new NativeFunction(main.base.add(0x6b3a84),'pointer',['pointer'])(main.base.add(0xdec9c8));
const rd=new NativeFunction(main.base.add(0x5bb354),'uint32',['pointer','uint32']);
const wr=new NativeFunction(main.base.add(0x5bb688),'void',['pointer','uint32','uint32']);
const tc=new NativeFunction(rec.getExportByName('BullyRecovery_TestControl'),'void',['int','float']);
const eq=new NativeFunction(rec.base.add(0x5b6d4),'void',['int']);
const diagnostic=new NativeFunction(rec.getExportByName('BullyRecovery_Diagnostic'),'void',['int']);
const root='/data/user/0/org.dolphinemu.dolphinemu.quest.bully.recover2/files/';
let skyRouting=null;
function checkGuestRange(a,n){const end=a+n*4;if(!Number.isInteger(a)||(a&3)||!((a>=0x80000000&&end<=0x81800000)||(a>=0x90000000&&end<=0x94000000)))throw Error('Guest access outside physical RAM');}
function saveCheckpoint(name){
 if(!/^[a-zA-Z0-9_.-]+\.sav$/.test(name))throw Error('Invalid checkpoint');
 const jni=(table,index,ret,args)=>new NativeFunction(table.add(index*8).readPointer(),ret,args);
 const out=Memory.alloc(8),count=Memory.alloc(4);out.writePointer(ptr(0));count.writeS32(0);
 const getVMs=new NativeFunction(Module.getGlobalExportByName('JNI_GetCreatedJavaVMs'),'int',['pointer','int','pointer']);
 if(getVMs(out,1,count)!==0||count.readS32()<1)throw Error('VM unavailable');
 const vm=out.readPointer(),table=vm.readPointer(),envOut=Memory.alloc(8);let attached=false;
 let rc=jni(table,6,'int',['pointer','pointer','int'])(vm,envOut,0x10006);
 if(rc===-2){rc=jni(table,4,'int',['pointer','pointer','pointer'])(vm,envOut,ptr(0));attached=true;}
 if(rc!==0)throw Error('Cannot attach JNI');
 const env=envOut.readPointer(),vtable=env.readPointer();
 const text=jni(vtable,167,'pointer',['pointer','pointer'])(env,Memory.allocUtf8String(root+'StateSaves/'+name));
 try{new NativeFunction(main.getExportByName('Java_org_dolphinemu_dolphinemu_NativeLibrary_SaveStateAs'),'void',['pointer','pointer','pointer'])(env,ptr(0),text);}
 finally{jni(vtable,23,'void',['pointer','pointer'])(env,text);if(attached)jni(table,5,'int',['pointer'])(vm);}
 return name;
}
rpc.exports={
 cpu(){const state=new NativeFunction(main.base.add(0x6b3af4),'pointer',['pointer'])(main.base.add(0xdec9c8));return {pc:state.readU32(),gpr:Array.from({length:32},(_,i)=>state.add(32+i*4).readU32())};},
 sky(enable){if(enable&&!skyRouting)skyRouting=createRecoveryPlusSkyRouting(main,rec);if(enable===false&&skyRouting){skyRouting.dispose();skyRouting=null;}return skyRouting?skyRouting.report():null;},
 setting(name,value){
  const keys={radarX:0x1741600,radarY:0x1741604,missionX:0x1741608,missionY:0x174160c,instructionsX:0x1741610,instructionsY:0x1741614};
  if(!(name in keys))throw Error('Unknown HUD setting');const at=main.base.add(keys[name]),before=at.readFloat();
  if(value!==null&&value!==undefined){if(!Number.isFinite(value)||Math.abs(value)>3)throw Error('HUD setting range');at.writeFloat(value);}
  return {before,value:at.readFloat()};
 },
 read(a,n){if(n>4096||n<1)throw Error('Read limit');checkGuestRange(a,n);return Array.from({length:n},(_,i)=>rd(mem,a+i*4));},
 write(a,v){checkGuestRange(a,1);wr(mem,v>>>0,a);},
 host(a,n){return Array.from(new Uint8Array(rec.base.add(a).readByteArray(n)));},
 touch(i,v){tc(i,v);},equip(i){eq(i);},effect(i){diagnostic(i);},
 grantitem(index){
  if(!Number.isInteger(index)||index<1||index>=92)throw Error('Equipment index');
  if(!rec.base.add(0x12bf98).readU8()||rec.base.add(0x12bf9c).readU8()||rec.base.add(0x12c094).readU8()||rec.base.add(0x12bc2c).readU8()||rec.base.add(0x12bc3c).readU8()||rec.base.add(0x12c070).readU8()||rec.base.add(0x12b754).readS32()!==-1)throw Error('Wait for free gameplay');
  const ped=rd(mem,0x80624204);checkGuestRange(ped,0x16bc/4);
  const request=new NativeFunction(rec.base.add(0x739bc),'uchar',['uint32','uint32','float','float','float','float','int']);
  const id=rec.base.add(0x3204c+index*4).readS32();
  return {id,accepted:!!request(4,ped,0,0,0,0,id),serial:rec.base.add(0x12c120).readU32()};
 },
 button(i,v){
  const reg=new NativeFunction(main.getExportByName('Java_org_dolphinemu_dolphinemu_features_input_model_InputOverrider_registerWii'),'void',['pointer','pointer','int']);
  const set=new NativeFunction(main.getExportByName('Java_org_dolphinemu_dolphinemu_features_input_model_InputOverrider_setControlState'),'void',['pointer','pointer','int','int','double']);
  reg(ptr(0),ptr(0),0);set(ptr(0),ptr(0),0,i,v);
 },
 save(name){return saveCheckpoint(name);},
 velocity(hand,x,y,z){
  if(hand!==0&&hand!==1)throw Error('Hand index');
  const lock=new NativeFunction(Module.getGlobalExportByName('pthread_mutex_lock'),'int',['pointer']);
  const unlock=new NativeFunction(Module.getGlobalExportByName('pthread_mutex_unlock'),'int',['pointer']);
  const mutex=rec.base.add(0x12b9a0);lock(mutex);
  try{const v=rec.base.add(0x12b9d0+hand*136+92);v.writeU8(1);[x,y,z].forEach((n,i)=>v.add(4+i*4).writeFloat(n));}
  finally{unlock(mutex);}
 },
 control(v){v.sequence=Date.now();File.writeAllText(root+'recovery-control.json',JSON.stringify(v));return v.sequence;},
 status(){return JSON.parse(File.readAllText(root+'recovery-fixes-status.json'));},
 addresses(){return {main:main.base.toString(),rec:rec.base.toString()};}
};
'''
class PlusSession:
 def __init__(self,patch=False):
  if not SERIAL:raise RuntimeError('Set ANDROID_SERIAL to the connected Quest shown by adb devices.')
  subprocess.run(['adb','-s',SERIAL,'forward','tcp:27044','tcp:27044'],check=True,capture_output=True)
  pid=int(subprocess.check_output(['adb','-s',SERIAL,'shell','pidof',PACKAGE],text=True).strip())
  self.device=frida.get_device_manager().add_remote_device('127.0.0.1:27044')
  self.session=self.device.attach(pid);source=(ROOT/'src/recoveryplus/sky-routing.js').read_text(encoding='utf8')+'\n'+BRIDGE.replace('org.dolphinemu.dolphinemu.quest.bully.recover2',PACKAGE)
  if patch:
   patchsource=(ROOT/'src/recoveryplus/review.js').read_text(encoding='utf8')
   source=patchsource+'\n'+source.replace('rpc.exports={','const review=createRecoveryPlusReview(main,rec,rd,mem);\nrpc.exports={review(){return review.report();},dispose(){review.dispose();},',1)
  self.script=self.session.create_script(source)
  self.script.on('message',lambda m,d:print('BRIDGE',m,file=sys.stderr) if m.get('type')=='error' else None)
  self.script.load();self.api=self.script.exports_sync
 def close(self):self.session.detach()
 def load(self,name):
  import re
  if not re.fullmatch(r'[a-zA-Z0-9_.-]+\.sav',name):raise ValueError('Invalid checkpoint name')
  path=f'/data/user/0/{PACKAGE}/files/'
  # Use Android's attach-agent thread, not the runtime JavaScript timer.
  subprocess.run(['adb','-s',SERIAL,'shell','am','attach-agent',PACKAGE,path+'review-state-agent.so=load:'+path+'StateSaves/'+name],check=True,capture_output=True,text=True,timeout=30)
  return {'checkpoint':name,'requested_via':'Android attach-agent; verify gameplay after load'}
 def save(self,name):
  import re
  if not re.fullmatch(r'[a-zA-Z0-9_.-]+\.sav',name):raise ValueError('Invalid checkpoint name')
  path=f'/data/user/0/{PACKAGE}/files/'
  subprocess.check_output(['adb','-s',SERIAL,'shell','am','attach-agent',PACKAGE,path+'review-state-agent.so=save:'+path+'StateSaves/'+name],text=True)
  previous=0
  for _ in range(60):
   time.sleep(.2)
   result=subprocess.run(['adb','-s',SERIAL,'shell','run-as',PACKAGE,'stat','-c','%s','files/StateSaves/'+name],capture_output=True,text=True)
   try:size=int(result.stdout.strip())
   except ValueError:continue
   if size>1024*1024 and size==previous:return {'checkpoint':name,'bytes':size,'verified':True}
   previous=size
  raise RuntimeError('Checkpoint was requested but a completed file was not confirmed')
 def capture(self,name):
  subprocess.run(['adb','-s',SERIAL,'shell','run-as',PACKAGE,'rm','-f','files/recovery-frame.rgba'],check=True,capture_output=True)
  self.api.effect(90);p=ROOT/'evidence'/name;p.parent.mkdir(parents=True,exist_ok=True)
  import struct
  from PIL import Image
  for _ in range(30):
   time.sleep(.15)
   b=subprocess.check_output(['adb','-s',SERIAL,'exec-out','run-as',PACKAGE,'cat','files/recovery-frame.rgba'])
   if len(b)>8:
    w,h=struct.unpack_from('<2I',b)
    if len(b)==8+w*h*4:Image.frombytes('RGBA',(w,h),b[8:]).transpose(Image.Transpose.FLIP_TOP_BOTTOM).save(p);return p
  raise RuntimeError('No eye frame')
if __name__=='__main__':
 s=PlusSession()
 try:print(json.dumps(s.api.status(),indent=2))
 finally:s.close()
