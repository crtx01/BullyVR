import json,struct
from unicorn import Uc,UC_ARCH_PPC,UC_MODE_32,UC_MODE_BIG_ENDIAN,UC_HOOK_CODE
from unicorn.ppc_const import UC_PPC_REG_0,UC_PPC_REG_1,UC_PPC_REG_3,UC_PPC_REG_4,UC_PPC_REG_5,UC_PPC_REG_LR,UC_PPC_REG_MSR

j=json.load(open('build/recoveryplus-mission-skip.json'));blob=struct.pack('>'+str(len(j['words']))+'I',*j['words'])
for actual,requested,state,expected in [(8,8,4,1),(8,9,4,-1),(-1,-1,4,-1),(8,8,8,-1)]:
 u=Uc(UC_ARCH_PPC,UC_MODE_32|UC_MODE_BIG_ENDIAN)
 for at,size in [(0x80010000,0x1000),(0x8038c000,0x1000),(0x80639000,0x1000),(0x81000000,0x2000)]:u.mem_map(at,size)
 u.mem_write(0x80010000,blob);mail=0x80010000+j['mailboxOffset']
 u.mem_write(mail,struct.pack('>iii',1,requested,0));u.mem_write(0x80639578,struct.pack('>iii',actual,1,state))
 u.reg_write(UC_PPC_REG_1,0x81001000);u.reg_write(UC_PPC_REG_LR,0x80010800)
 u.reg_write(UC_PPC_REG_MSR,0x2000)
 called=[]
 def hook(uc,at,size,data):
  if at==0x8038c430:
   called.append(tuple(uc.reg_read(r) for r in [UC_PPC_REG_3,UC_PPC_REG_4,UC_PPC_REG_5]));uc.reg_write(UC_PPC_REG_0,0x12345678);uc.reg_write(UC_PPC_REG_LR,uc.reg_read(UC_PPC_REG_LR));uc.reg_write(1,0) if False else None
   from unicorn.ppc_const import UC_PPC_REG_PC
   uc.reg_write(UC_PPC_REG_PC,uc.reg_read(UC_PPC_REG_LR))
 u.hook_add(UC_HOOK_CODE,hook)
 u.emu_start(0x80010000,0x80010800,count=1000)
 result=struct.unpack('>i',u.mem_read(mail+8,4))[0]
 assert result==expected,(actual,requested,state,result)
 assert called==([(0x80639578,0,0)] if expected==1 else [])
 assert u.reg_read(UC_PPC_REG_1)==0x81001000
print('Passed: native completion args, stale mission rejection, inactive state, stack restoration')
