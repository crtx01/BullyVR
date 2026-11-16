from pathlib import Path
from keystone import Ks,KS_ARCH_PPC,KS_MODE_PPC32,KS_MODE_BIG_ENDIAN
import struct
lines=['stwu 1,-512(1)','mflr 0','stw 0,516(1)','stmw 2,128(1)','mfctr 0','stw 0,112(1)','mfcr 0','stw 0,116(1)']
for f in range(14):lines.append(f'stfd {f},{256+f*8}(1)')
lines+=['bl here','here:','mflr 30','addi 30,30,mailbox-here','lwz 3,40(30)','addi 3,3,1','stw 3,40(30)','lwz 31,0(30)','cmpwi 31,0','beq done','li 0,0','stw 0,0(30)','lwz 29,4(30)','lwz 0,0(29)','lwz 3,8(30)','cmpw 0,3','bne rejected','cmpwi 31,2','beq equip','cmpwi 31,3','beq equip','cmpwi 31,4','beq grantOnly','cmpwi 31,1','bne rejected','lwz 0,5468(29)','cmpwi 0,0','bne rejected',
'mr 3,29','li 4,0','lfs 1,12(30)','lis 12,0x8018','ori 12,12,0x199c','mtctr 12','bctrl','li 9,0','stw 9,8(1)','stw 9,12(1)','li 0,1','stw 0,16(1)','stw 9,20(1)','li 0,-1','stw 0,24(1)','stw 9,28(1)',
'mr 3,29','li 4,0','li 5,-1','li 6,0','li 7,0','li 8,0','li 10,0','lfs 1,12(30)','lfs 2,44(30)','lfs 3,16(30)','lfs 4,20(30)','lfs 5,24(30)','lwz 12,0(29)','lwz 12,68(12)','mtctr 12','bctrl','b accepted',
'equip:','lwz 0,28(30)','cmpwi 0,-1','beq equipcommit','cmpwi 31,3','beq grant','lwz 3,480(29)','lwz 4,28(30)','lis 12,0x8014','ori 12,12,0x3950','mtctr 12','bctrl','cmpwi 3,0','ble rejected','b loadweapon','grant:','lwz 3,480(29)','lwz 4,28(30)','li 5,5','li 6,0','li 7,0','lis 12,0x8014','ori 12,12,0x3de0','mtctr 12','bctrl','loadweapon:',
'lwz 3,28(30)','lis 12,0x8031','ori 12,12,0x358c','mtctr 12','bctrl','cmpwi 3,0','beq rejected','lis 12,0x8025','ori 12,12,0x1edc','mtctr 12','bctrl','equipcommit:','lwz 3,768(29)','lwz 0,28(30)','stw 0,32(3)','li 0,1','stw 0,5320(29)','b accepted',
'grantOnly:','lwz 3,28(30)','lis 12,0x8031','ori 12,12,0x358c','mtctr 12','bctrl','cmpwi 3,0','beq rejected','lis 12,0x8025','ori 12,12,0x1edc','mtctr 12','bctrl','lwz 3,480(29)','lwz 4,28(30)','li 5,5','li 6,0','li 7,0','lis 12,0x8014','ori 12,12,0x3de0','mtctr 12','bctrl','accepted:','li 0,1','b result','rejected:','li 0,-1','result:','stw 0,36(30)','lwz 0,32(30)','stw 0,48(30)','done:']
for f in range(14):lines.append(f'lfd {f},{256+f*8}(1)')
lines+=['lwz 0,112(1)','mtctr 0','lwz 0,116(1)','mtcrf 255,0','lmw 2,128(1)','lwz 0,516(1)','mtlr 0','addi 1,1,512','blr','mailbox:']
# Use .long for fixed data so assembling code does not require separately resolving labels.
lines+=['.long 0']*13+['.long 0x42565243']
source='\n'.join(lines)
source=source.replace('cmpwi 31,1\nbne rejected','cmpwi 31,5\nbeq facing\ncmpwi 31,1\nbne rejected',1)
source=source.replace('grantOnly:\n','facing:\nmr 3,29\nli 0,0\nstw 0,8(1)\naddi 4,1,8\nlfs 1,12(30)\nlis 12,0x803d\nori 12,12,0xccf8\nmtctr 12\nbctrl\nb accepted\ngrantOnly:\n',1)
# Grant inventory without streaming nineteen models when closing the menu.
# Only the selected item's equip path needs to load its render resources.
source=source.replace('grantOnly:\nlwz 3,28(30)\nlis 12,0x8031\nori 12,12,0x358c\nmtctr 12\nbctrl\ncmpwi 3,0\nbeq rejected\nlis 12,0x8025\nori 12,12,0x1edc\nmtctr 12\nbctrl\n','grantOnly:\n',1)
source=source.replace('cmpwi 31,5\nbeq facing','cmpwi 31,6\nbeq flushcode\ncmpwi 31,5\nbeq facing',1)
source=source.replace('cmpwi 31,6\nbeq flushcode','cmpwi 31,7\nbeq startmission\ncmpwi 31,6\nbeq flushcode',1)
source=source.replace('facing:\n','startmission:\nlis 3,0x8063\nori 3,3,0x90bc\nlwz 4,28(30)\nlis 12,0x8038\nori 12,12,0xb488\nmtctr 12\nbctrl\nlbz 0,4(3)\nori 0,0,0x40\nstb 0,4(3)\nlis 3,0x8063\nori 3,3,0x90bc\nli 4,1\nlis 12,0x8038\nori 12,12,0xd96c\nmtctr 12\nbctrl\nlis 3,0x8063\nori 3,3,0x90bc\nlwz 4,28(30)\nli 5,0\nlis 12,0x8038\nori 12,12,0xb124\nmtctr 12\nbctrl\nb accepted\nfacing:\n',1)
flush=['flushcode:','sync']
for site in (0x8038b124,0x8038b834,0x8038b8d0,0x801ea2c0,0x801a8944,0x8014d7ac,0x80160b40,0x801684a0):
 flush += [f'lis 3,{site>>16}',f'ori 3,3,{site&65535}','icbi 0,3']
flush += ['lis 3,0x8000','ori 3,3,0x1800','li 4,192','mtctr 4','flushloop:','icbi 0,3','addi 3,3,32','bdnz flushloop','sync','isync','b accepted']
source=source.replace('facing:\n','\n'.join(flush)+'\nfacing:\n',1)
gate_offset=bytes(Ks(KS_ARCH_PPC,KS_MODE_PPC32|KS_MODE_BIG_ENDIAN).asm(source)[0]).find(struct.pack('>I',0x42565243))-52
gates=[]
for index,original in enumerate([
 ['stwu 1,-64(1)','mflr 0','stw 0,68(1)','addi 11,1,64'],
 ['stwu 1,-16(1)','mflr 0','stw 0,20(1)','stw 31,12(1)'],
 ['stw 4,24(3)','li 7,1','stb 7,28(3)','li 4,0']]):
 gates += ['cmpwi 4,0',f'beq demo_allow_{index}','cmpwi 4,4',f'beq demo_allow_{index}','cmpwi 4,5',f'beq demo_allow_{index}','li 3,0','blr',f'demo_allow_{index}:',*original,'.long 0']
source=source.replace('mailbox:','\n'.join(gates)+'\nmailbox:',1)
from physical_ray_bridge import append_physical_hooks
source=append_physical_hooks(source)
Path('build/contact-bridge.s').write_text(source)
k=Ks(KS_ARCH_PPC,KS_MODE_PPC32|KS_MODE_BIG_ENDIAN);data=bytes(k.asm(source)[0]);data+=bytes((-len(data))%8)
mailbox=len(k.asm(source[:source.index('mailbox:')])[0]) if False else data.find(struct.pack('>I',0x42565243))-52
assert mailbox>0 and len(data)<3000
Path('assets/converted/contact_bridge.bin').write_bytes(data)
Path('src/contact_bridge_data.h').write_text('#pragma once\nstatic constexpr uint32_t contactBridgeWords[]={'+','.join(hex(x) for x in struct.unpack('>'+str(len(data)//4)+'I',data))+'};\nstatic constexpr unsigned contactMailboxOffset='+str(mailbox)+';\nstatic constexpr unsigned demoGateOffset='+str(gate_offset)+';\n')
print('bridge bytes',len(data),'mailbox',mailbox)
labels={}
# Resolve label addresses by assembling a branch from the beginning. This
# avoids assembling incomplete sources with unresolved forward references.
for name in ('physical_projectile','physical_target','physical_ray','physical_visibility','physical_frustum','physical_camera','physical_camera_update','physical_player_visibility'):
 probe=bytes(k.asm('b '+name+'\n'+source)[0]);labels[name]=(struct.unpack_from('>I',probe)[0]&0x03fffffc)-4
with Path('src/contact_bridge_data.h').open('a') as f:
 for name,off in labels.items():f.write('static constexpr unsigned '+name+'Offset='+str(off)+';\n')
