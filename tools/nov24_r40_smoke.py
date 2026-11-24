import json,time
from pathlib import Path
from recoveryplus_review_client import call
root=Path(__file__).resolve().parents[1]
def pulse(i,v=1):
 call('touch',i,v);time.sleep(.15);call('touch',i,0);time.sleep(.2)
rows=[]
try:
 call('touch',0,1);pulse(13)
 for i in range(6):
  pulse(2,-.9);pulse(3,.9)
  rows.append({'tab':call('host',0x12c074,1)[0],'row':call('host',0x12c078,1)[0]})
 pulse(13);pulse(14);pulse(14)
 states=[]
 for value in [True,False]:
  call('control',{'unpause':True,'cheat':{'name':'NeverWanted','value':value}});time.sleep(2)
  states.append(call('status'))
 a=call('read',0x806eb740,1)[0];time.sleep(.5);b=call('read',0x806eb740,1)[0]
 result={'menus':rows,'states':states,'guest_progress':a!=b,'final':call('status')}
 (root/'evidence/nov24-stability/r40-smoke.json').write_text(json.dumps(result,indent=2))
 print('r40 menu/controls/cheat smoke: guest advancing=',a!=b,'textures=',result['final'].get('imguiTextures'),flush=True)
finally:
 call('touch',0,0)
