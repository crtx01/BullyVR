"""Build a separate, bounded Gecko callback using the native mission finish API."""
import json, struct
from pathlib import Path
from keystone import Ks, KS_ARCH_PPC, KS_MODE_PPC32, KS_MODE_BIG_ENDIAN

lines=['stwu 1,-512(1)','mflr 0','stw 0,516(1)','stmw 2,128(1)',
       'mfctr 0','stw 0,112(1)','mfcr 0','stw 0,116(1)']
lines += [f'stfd {f},{256+f*8}(1)' for f in range(14)]
lines += ['bl here','here:','mflr 30','addi 30,30,mailbox-here',
 'lwz 0,0(30)','cmpwi 0,1','bne done','li 0,0','stw 0,0(30)',
 'lis 3,0x8063','ori 3,3,0x9578','lwz 0,0(3)','lwz 4,4(30)',
 'cmpw 0,4','bne reject','cmpwi 0,0','blt reject',
 'lwz 0,8(3)','cmpwi 0,4','bne reject',
 'li 4,0','li 5,0','li 6,0','lis 12,0x8038','ori 12,12,0xc430',
 'mtctr 12','bctrl','li 0,1','stw 0,8(30)','b done',
 'reject:','li 0,-1','stw 0,8(30)','done:']
lines += [f'lfd {f},{256+f*8}(1)' for f in range(14)]
lines += ['lwz 0,112(1)','mtctr 0','lwz 0,116(1)','mtcrf 255,0',
 'lmw 2,128(1)','lwz 0,516(1)','mtlr 0','addi 1,1,512','blr',
 'mailbox:','.long 0','.long -1','.long 0','.long 0x534b4950']
k=Ks(KS_ARCH_PPC,KS_MODE_PPC32|KS_MODE_BIG_ENDIAN)
data=bytes(k.asm('\n'.join(lines))[0]);data+=bytes((-len(data))%8)
offset=data.find(struct.pack('>I',0x534b4950))-12
out=Path('build/recoveryplus-mission-skip.json')
out.write_text(json.dumps({'words':list(struct.unpack('>'+str(len(data)//4)+'I',data)),'mailboxOffset':offset}))
print('Mission skip callback:',len(data),'bytes; mailbox',offset)
