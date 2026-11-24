"""Bounded real-device input sweep; preserve every observation, including failures."""
import json,time,struct
from pathlib import Path
from recoveryplus_review_client import call
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'evidence/nov24-stability/live-sweep.json'
report={'build':'pending','menus':[],'equipment':[],'complete':False}
def save():OUT.write_text(json.dumps(report,indent=2),encoding='utf8')
def touch(i,v):call('touch',i,v,timeout=8)
def pulse(i,v=1):
 touch(i,v);time.sleep(.12);touch(i,0);time.sleep(.15)
def status():return call('status',timeout=8)
def host(offset,n=4):return call('host',offset,n,timeout=8)
def int_at(offset):return struct.unpack('<i',bytes(host(offset)))[0]
try:
 call('touch',0,1);call('control',{'unpause':True});time.sleep(1)
 report['build']=status().get('build');save()
 for cycle in range(3):
  pulse(13)
  for tab in range(6):
   for _ in range(4):pulse(2,-.9)
   report['menus'].append({'cycle':cycle,'tab':int_at(0x12c074),'row':int_at(0x12c078),'open':host(0x12bc2c,1)[0]})
   save();pulse(3,.9)
  pulse(13);pulse(14);pulse(14)
  print('Menu cycle',cycle+1,'completed',flush=True)
 report['afterMenus']=status();save()
 first=call('read',0x806eb740,1)[0];time.sleep(.5)
 if call('read',0x806eb740,1)[0]==first:raise RuntimeError('Guest clock stopped after menus; equipment checks withheld')
 call('effect',50,timeout=8);time.sleep(5)
 ids=struct.unpack('<92i',bytes(host(0x3204c,368)))
 for i,expected in enumerate(ids):
  touch(0,1);call('equip',i,timeout=8);started=time.monotonic()
  while time.monotonic()-started<2.5:
   time.sleep(.15)
   if int_at(0x12b734)==-2:break
  actual=int_at(0x12b710)
  row={'index':i,'expected':expected,'actual':actual,'pending':int_at(0x12b734),'equipped':actual==expected,'seconds':round(time.monotonic()-started,2),'use_test':'not equipped'}
  if row['equipped'] and expected not in (-1,437):
   touch(10,1);time.sleep(.35);touch(10,0);time.sleep(.25);row['use_test']='trigger press/release sent; projectile/animation not visually confirmed'
  report['equipment'].append(row);save()
  time.sleep(.85)
  if i%10==0:print('Equipment index',i,'actual',actual,flush=True)
 report['final']=status();report['complete']=True;save()
except Exception as e:
 report['failure']=str(e);save();print('Sweep stopped:',e,flush=True)
finally:
 for i in [2,3,9,10,13,14,0]:
  try:touch(i,0)
  except Exception:pass
