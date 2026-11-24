"""Load each native inventory item through the existing native grant API."""
import json,time,struct
from pathlib import Path
from recoveryplus_review_client import call
ROOT=Path(__file__).resolve().parents[1];out=ROOT/'evidence/nov24-stability/granted-items.json'
rows=[r for r in json.loads(out.read_text()) if 'actual' in r] if out.exists() else []
def h(at,n=4):return call('host',at,n,timeout=8)
def integer(at):return struct.unpack('<i',bytes(h(at)))[0]
def save():out.write_text(json.dumps(rows,indent=2),encoding='utf8')
ids=struct.unpack('<92i',bytes(h(0x3204c,368)))
try:
 for i in range(1,92):
  if any(r.get('index')==i for r in rows):continue
  call('touch',0,1,timeout=8)
  result=call('grantitem',i,timeout=8);row={'index':i,'id':ids[i],'grant':result};rows.append(row);save()
  if not result['accepted']:row['result']='native request busy/rejected';save();time.sleep(.8);continue
  mailbox=struct.unpack('<I',bytes(h(0x12c11c)))[0];start=time.monotonic();ack=False
  while time.monotonic()-start<4:
   if call('read',mailbox+0x30,1,timeout=8)[0]==result['serial']:ack=True;break
   time.sleep(.1)
  row['acknowledged']=ack
  if not ack:row['result']='grant timeout; stopped';save();break
  row['nativeResult']=call('read',mailbox+0x24,1,timeout=8)[0]
  if row['nativeResult']==0xffffffff:
   row['actual']=integer(0x12b710);row['equipped']=False;row['result']='native grant rejected';save();time.sleep(.85);continue
  time.sleep(.4);call('equip',i,timeout=8);start=time.monotonic()
  while time.monotonic()-start<5:
   time.sleep(.15)
   if integer(0x12b710)==ids[i] and integer(0x12b734)==-2:break
   if time.monotonic()-start>.4 and integer(0x12b734)==-2 and integer(0x12bc4c+i*4)<=0:break
  row['actual']=integer(0x12b710);row['equipped']=row['actual']==ids[i];row['result']='equipped' if row['equipped'] else 'not equipped'
  if row['equipped']:
   call('capture',f'nov24-stability/item-{i:02d}.png',timeout=8);row['capture']=f'item-{i:02d}.png'
  save();time.sleep(.85)
  if i%10==0:print('Native item',i,row['result'],flush=True)
except Exception as e:
 rows.append({'failure':str(e)});save();print('Item sweep stopped:',e,flush=True)
finally:
 try:call('touch',0,0,timeout=5)
 except Exception:pass
