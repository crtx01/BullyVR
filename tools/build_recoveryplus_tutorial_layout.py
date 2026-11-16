import json,struct
from pathlib import Path
from keystone import Ks,KS_ARCH_PPC,KS_MODE_PPC32,KS_MODE_BIG_ENDIAN
lines=['blr','center:','stwu 1,-80(1)','stw 12,68(1)']
lines += [f'stfd {i},{8+i*8}(1)' for i in range(6)]
lines += ['lis 12,0x43a0','stw 12,64(1)','lfs 3,64(1)','lfs 4,-20208(13)','fmuls 3,3,4',
 'lfs 0,96(1)','lfs 1,104(1)','fadds 2,0,1','lis 12,0x3f00','stw 12,64(1)',
 'lfs 4,64(1)','fmuls 2,2,4','fsubs 3,3,2',
 'fadds 0,0,3','fadds 1,1,3','fadds 31,31,3','stfs 0,96(1)','stfs 1,104(1)']
lines += [f'lfd {i},{8+i*8}(1)' for i in range(6)]
lines += ['lwz 12,68(1)','addi 1,1,80','addi 3,1,16','.long 0','.long 0x48554443']
k=Ks(KS_ARCH_PPC,KS_MODE_PPC32|KS_MODE_BIG_ENDIAN);data=bytes(k.asm('\n'.join(lines))[0]);data+=bytes((-len(data))%8)
words=list(struct.unpack('>'+str(len(data)//4)+'I',data))
Path('build/recoveryplus-tutorial-layout.json').write_text(json.dumps({'words':words,'entry':4,'returnOffset':data.find(struct.pack('>I',0x48554443))-4,'site':0x80362078,'original':0x38610010}))
print('Native tutorial panel centering:',len(data),'bytes')
