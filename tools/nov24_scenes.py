"""Real single-eye scene captures and measured travel/camera observations."""
import json,time,math,struct
from pathlib import Path
from recoveryplus_review_client import call
root=Path(__file__).resolve().parents[1];rows=[]
def touch(i,v):call('touch',i,v)
def setup():
 touch(0,1);touch(15,0);touch(16,-5)
 for i,v in [(17,-.22),(18,1.25),(19,-.5),(20,.22),(21,1.25),(22,-.5),(23,-5),(24,0),(25,-5),(26,0)]:touch(i,v)
def capture(name):
 s=call('status');p=call('capture','nov24-stability/scene-'+name+'.png')
 rows.append({'name':name,'path':p,'position':s.get('position'),'drawDistance':s.get('drawDistance'),'textures':s.get('imguiTextures'),'scope':'visual sample only; not proof of every LOD or NPC'})
 (root/'evidence/nov24-stability/scenes.json').write_text(json.dumps(rows,indent=2),encoding='utf8')
setup();call('equip',2);time.sleep(1);capture('school-front-trees')
for yaw in [-80,80]:
 touch(15,yaw);touch(24,yaw);time.sleep(.7);capture('school-turn-'+str(yaw))
touch(15,0);touch(24,0)
for page,index,name in [('town',1,'town-street'),('carnival',1,'carnival'),('school',3,'main-school-building')]:
 call('control',{'unpause':True,'travelPage':page});time.sleep(.4)
 call('control',{'unpause':True,'travelPage':page,'travel':index});time.sleep(5)
 setup();capture(name)
call('effect',40);time.sleep(1);capture('controls-panel');call('effect',41)
call('touch',0,0)
print(f'{len(rows)} real scene samples recorded',flush=True)
