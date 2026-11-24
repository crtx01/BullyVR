import json,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def call(method,*args,timeout=25):
 seq=time.time_ns();p=ROOT/'build/recoveryplus-command.json';tmp=p.with_suffix('.tmp');tmp.write_text(json.dumps(dict(sequence=seq,method=method,args=args)),encoding='utf8')
 for attempt in range(30):
  try:tmp.replace(p);break
  except PermissionError:
   if attempt==29:raise
   time.sleep(.01)
 started=time.monotonic()
 while time.monotonic()-started<timeout:
  time.sleep(.04)
  try:r=json.loads((ROOT/'build/recoveryplus-response.json').read_text(encoding='utf8'))
  except (OSError,ValueError):continue
  if r.get('sequence')!=seq:continue
  if 'error' in r:raise RuntimeError(r['error'])
  return r.get('result')
 raise TimeoutError(method+' did not return')
if __name__=='__main__':print(json.dumps(call('review'),indent=2))
