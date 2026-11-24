"""Compile the real native callback and check its state transitions in ARM64."""
from pathlib import Path
import re,subprocess,struct
from elftools.elf.elffile import ELFFile
from unicorn import Uc,UC_ARCH_ARM64,UC_MODE_ARM
from unicorn.arm64_const import UC_ARM64_REG_X0,UC_ARM64_REG_LR,UC_ARM64_REG_SP,UC_ARM64_REG_PC
root=Path(__file__).resolve().parents[1];out=root/'build/streaming-guard-test';(out/'gum').mkdir(parents=True,exist_ok=True)
(out/'gum/guminterceptor.h').write_text('typedef struct {void *argument;} GumInvocationContext;\nvoid *gum_invocation_context_get_nth_argument(GumInvocationContext*,unsigned int);\nvoid gum_invocation_context_replace_nth_argument(GumInvocationContext*,unsigned int,void*);\n')
source=(root/'src/recoveryplus/streaming-guard.js').read_text();code=re.search(r'new CModule\(`([\s\S]*?)`,',source)[1]
code+='''
unsigned char rec_base[0x12c200];unsigned int guard_stats[4];double equipment_deadline[1];
void *gum_invocation_context_get_nth_argument(GumInvocationContext*c,unsigned int n){return c->argument;}
void gum_invocation_context_replace_nth_argument(GumInvocationContext*c,unsigned int n,void*v){c->argument=v;}
int clock_gettime(int c,Time*t){t->sec=10;t->nsec=0;return 0;}
static void setup(void){rec_base[0x12bf98]=1;*(int*)(rec_base+0x12b754)=-1;*(int*)(rec_base+0x12b734)=-2;equipment_deadline[0]=0;}
int run_test(int which){GumInvocationContext c={(void*)2};setup();
 if(which==1)rec_base[0x12c070]=1;
 if(which==2)*(int*)(rec_base+0x12b734)=303;
 if(which==3)equipment_deadline[0]=11;
 if(which==4)rec_base[0x12bf9c]=1;
 if(which==5)rec_base[0x12c094]=1;
 if(which==6)*(int*)(rec_base+0x12b754)=2;
 equip_enter(&c);return (int)(long)c.argument;}
int run_matrix(int unused){int from,to,phase;GumInvocationContext c;
 for(from=0;from<92;from++)for(to=0;to<92;to++){
  setup();rec_base[0x12c070]=0;rec_base[0x12bf9c]=0;rec_base[0x12c094]=0;
  c.argument=(void*)(long)from;equip_enter(&c);if((long)c.argument!=from)return 1;
  c.argument=(void*)(long)to;equip_enter(&c);if((long)c.argument!=-1)return 2;
  equipment_deadline[0]=0;c.argument=(void*)(long)to;equip_enter(&c);if((long)c.argument!=to)return 3;
  for(phase=0;phase<5;phase++){
   equipment_deadline[0]=0;rec_base[0x12c070]=phase==0;rec_base[0x12bf9c]=phase==1;
   rec_base[0x12c094]=phase==2;rec_base[0x12bf98]=phase!=3;*(int*)(rec_base+0x12b754)=phase==4?1:-1;
   c.argument=(void*)(long)to;equip_enter(&c);if((long)c.argument!=-1)return 4+phase;
  }
 }
 return 0;
}
'''
(out/'test.c').write_text(code)
clang=Path.home()/'AppData/Local/Android/Sdk/ndk/29.0.14206865/toolchains/llvm/prebuilt/windows-x86_64/bin/clang.exe'
(out/'layout.ld').write_text('ENTRY(run_test)\nSECTIONS { . = 0x100000; .text : { *(.text*) } .rodata : { *(.rodata*) } .data : { *(.data*) } .bss : { *(.bss*) *(COMMON) } /DISCARD/ : { *(.eh_frame*) *(.note*) } }')
subprocess.run([str(clang),'--target=aarch64-linux-android28','-O1','-fno-stack-protector','-fno-pic','-fno-pie','-no-pie','-nostdlib','-static','-Wl,-T,'+str(out/'layout.ld'),'-I',str(out),str(out/'test.c'),'-o',str(out/'test.elf')],check=True,capture_output=True)
with (out/'test.elf').open('rb') as f:
 elf=ELFFile(f);segments=[(s['p_vaddr'],s['p_memsz'],s.data()) for s in elf.iter_segments() if s['p_type']=='PT_LOAD'];symbols={s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()};fn=symbols['run_test']
for case in range(8):
 u=Uc(UC_ARCH_ARM64,UC_MODE_ARM);mapped=set()
 for addr,size,data in segments:
  for p in range(addr&~4095,(addr+size+4095)&~4095,4096):
   if p not in mapped:u.mem_map(p,4096);mapped.add(p)
  u.mem_write(addr,data)
 u.mem_map(0x10000000,0x10000);u.mem_map(0x20000000,4096);u.reg_write(UC_ARM64_REG_SP,0x1000fff0);u.reg_write(UC_ARM64_REG_LR,0x20000000);u.reg_write(UC_ARM64_REG_X0,case)
 u.emu_start(symbols['run_matrix'] if case==7 else fn,0x20000000,count=20000000 if case==7 else 1000);value=u.reg_read(UC_ARM64_REG_X0)&0xffffffff
 assert u.reg_read(UC_ARM64_REG_PC)==0x20000000,'execution budget exceeded'
 assert value==(0 if case==7 else 2 if case==0 else 0xffffffff),(case,value)
print('Passed: actual ARM64 callback accepts idle equip; rejects travel, pending equip, cooldown, cinema, class and queued travel')
print('Passed: 92x92 = 8464 ordered equipment pairs; cooldown and five transition/loading gates for every pair (guard logic only, not rendering/models)')
