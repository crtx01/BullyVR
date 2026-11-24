"""Keep the additive Recovery+ patch attached while local checks send commands."""
import json,time,sys
from pathlib import Path
from recoveryplus_session import PlusSession,ROOT
command=ROOT/'build/recoveryplus-command.json';response=ROOT/'build/recoveryplus-response.json'
s=PlusSession(patch='--no-patch' not in sys.argv);last=None
try:last=json.loads(command.read_text(encoding='utf8')).get('sequence')
except (OSError,ValueError):pass
print('Recovery+ session attached; original native engine retained',flush=True)
try:
 while True:
  time.sleep(.04)
  try:c=json.loads(command.read_text(encoding='utf8'))
  except (OSError,ValueError):continue
  seq=c.get('sequence')
  if seq==last:continue
  last=seq;method=c['method'];result={'sequence':seq}
  try:
   if method=='stop':result['result']='stopped'
   elif method=='save':result['result']=s.save(*c.get('args',[]))
   elif method=='load':result['result']=s.load(*c.get('args',[]))
   elif method=='control' and c.get('args') and 'loadState' in c['args'][0]:
    values=dict(c['args'][0]);checkpoint=values.pop('loadState')
    s.api.control(values)
    result['result']=s.load(checkpoint)
   elif method=='capture':result['result']=str(s.capture(*c.get('args',[])))
   else:result['result']=getattr(s.api,method)(*c.get('args',[]))
  except Exception as e:result['error']=str(e)
  temp=response.with_suffix('.tmp');temp.write_text(json.dumps(result),encoding='utf8')
  for attempt in range(30):
   try:temp.replace(response);break
   except PermissionError:
    if attempt==29:raise
    time.sleep(.01)
  if method=='stop':break
finally:
 try:
  s.api.touch(0,0)
  if '--no-patch' not in sys.argv:s.api.dispose()
 finally:s.close()
