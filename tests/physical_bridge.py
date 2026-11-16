"""Execute the generated Wii PPC hooks, including native-return ABI paths."""
from pathlib import Path
import struct,re,math
from unicorn import Uc,UC_ARCH_PPC,UC_MODE_32,UC_MODE_BIG_ENDIAN,UC_HOOK_CODE
from unicorn.ppc_const import *
root=Path(__file__).resolve().parents[1]
header=(root/'src/contact_bridge_data.h').read_text()
offsets={k:int(v) for k,v in re.findall(r'unsigned (\w+)Offset=(\d+)',header)}
base=0x80002000;stop=0x80400000;player=0x81010000;projectile=0x81020000;matrix=0x81030000;stack=0x81100000
def new():
 u=Uc(UC_ARCH_PPC,UC_MODE_32|UC_MODE_BIG_ENDIAN);u.mem_map(0x80000000,0x2000000)
 u.mem_write(base,(root/'assets/converted/contact_bridge.bin').read_bytes())
 u.reg_write(UC_PPC_REG_MSR,0x2000);u.reg_write(UC_PPC_REG_1,stack);u.reg_write(UC_PPC_REG_LR,stop)
 return u
def word(u,a,n):u.mem_write(a,struct.pack('>I',n&0xffffffff))
def floats(u,a,v):u.mem_write(a,struct.pack('>'+str(len(v))+'f',*v))
def readf(u,a,n):return struct.unpack('>'+str(n)+'f',u.mem_read(a,n*4))
def reg(i):return globals()['UC_PPC_REG_'+str(i)]
def close(a,b,tol=1e-3):assert all(abs(x-y)<tol for x,y in zip(a,b)),(a,b)
for owner,enabled,velocity_in in [(player,1,[1,2,3]),(player,0,[1,2,3]),(player+0x2120,1,[1,2,3]),(player,1,[0,0,0])]:
 u=new();ray=base+offsets['physical_ray'];word(u,0x80624204,player);word(u,ray,enabled)
 floats(u,ray+4,[4,5,6]);floats(u,ray+16,[0,.6,.8]);floats(u,0x81040000,[90,91,92]);floats(u,0x81040010,velocity_in)
 word(u,projectile+0x18,matrix);floats(u,matrix+0x30,[90,91,92]);floats(u,projectile+0x128,velocity_in)
 u.reg_write(reg(3),owner);u.reg_write(reg(5),0x81040000);u.reg_write(reg(6),0x81040010);u.reg_write(reg(8),0x81050000)
 for i in range(6):word(u,stack+8+i*4,0x12340000+i)
 native_calls=[]
 def native(uc,address,size,data):
  if address!=0x801ea2c4:return
  sp=uc.reg_read(reg(1));native_calls.append(True)
  for i in range(6):assert bytes(uc.mem_read(sp+8+i*4,4))==struct.pack('>I',0x12340000+i)
  assert uc.reg_read(reg(8))==(0 if owner==player and enabled else 0x81050000)
  uc.reg_write(reg(3),projectile);uc.reg_write(UC_PPC_REG_PC,uc.reg_read(UC_PPC_REG_LR))
 u.hook_add(UC_HOOK_CODE,native);u.emu_start(base+offsets['physical_projectile'],stop,count=10000)
 assert len(native_calls)==1 and u.reg_read(reg(1))==stack and u.reg_read(reg(3))==projectile
 if owner==player and enabled:
  close(readf(u,matrix+0x30,3),[4,5,6]);velocity=readf(u,projectile+0x128,3)
  speed=math.sqrt(sum(x*x for x in velocity));assert abs(speed-math.sqrt(sum(x*x for x in velocity_in)))<.01
  if speed:close([x/speed for x in velocity],[0,.6,.8])
  else:close(velocity,[0,0,0])
 else:close(readf(u,matrix+0x30,3),[90,91,92]);close(readf(u,projectile+0x128,3),velocity_in)
u=new();cam=0x81060000;state=base+offsets['physical_camera'];word(u,state,1);floats(u,state+4,[1.6])
word(u,0x80624a08,cam);word(u,0x80624204,player);word(u,player+0x18,matrix)
floats(u,matrix+0x30,[4,5,6]);floats(u,cam+0x9c,[10,11,12]);floats(u,cam+0x80,[11,11,12]);u.reg_write(reg(3),cam)
u.emu_start(base+offsets['physical_camera_update'],0x80160b44,count=1000)
close(readf(u,cam+0x9c,3),[4,5,7.6]);close(readf(u,cam+0x80,3),[5,5,7.6]);assert u.reg_read(reg(1))==stack
for flag,value in ((0x806eab14,1),(0x806ecbd8,0x01000000)):
 word(u,flag,value);floats(u,cam+0x9c,[10,11,12]);floats(u,cam+0x80,[11,11,12]);u.reg_write(reg(3),cam)
 u.emu_start(base+offsets['physical_camera_update'],0x80160b44,count=1000)
 close(readf(u,cam+0x9c,3),[10,11,12]);close(readf(u,cam+0x80,3),[11,11,12]);word(u,flag,0)
for owner in (player,player+0x2120):
 u.reg_write(reg(3),owner);u.reg_write(reg(1),stack);u.reg_write(UC_PPC_REG_LR,stop)
 end=stop if owner==player else 0x801684a4
 u.emu_start(base+offsets['physical_player_visibility'],end,count=1000)
 assert u.reg_read(reg(3))==(0 if owner==player else owner)
for distance in (34.,100.,121.):
 u=new();state=base+offsets['physical_visibility'];word(u,state,1);floats(u,state+4,[0,0,0])
 floats(u,0x81070000,[distance,0,0]);u.reg_write(reg(3),0x806247b0);u.reg_write(reg(4),0x81070000)
 u.emu_start(base+offsets['physical_frustum'],stop if distance<=120 else 0x8014d7b0,count=1000)
 if distance<=120:assert u.reg_read(reg(3))==1
 else:assert u.reg_read(reg(3))==0x806247b0
print('PASS: PPC projectile ABI and final ray, NPC isolation, current-root camera, player-only hiding, 120m omnidirectional visibility')
