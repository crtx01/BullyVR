"""Execute the emitted gate branches, checking every native mission index."""
from pathlib import Path
import re,struct
root=Path(__file__).resolve().parents[1]
header=(root/'src/contact_bridge_data.h').read_text()
start=int(re.search(r'demoGateOffset=(\d+)',header)[1])
data=(root/'assets/converted/contact_bridge.bin').read_bytes()
def allowed(words,index):
 pc=0;equal=False
 for _ in range(16):
  instruction=words[pc//4]
  if instruction>>26==11:
   value=instruction&65535;value=value-65536 if value&32768 else value
   equal=index==value;pc+=4
  elif instruction>>16==0x4182:
   delta=instruction&0xfffc;delta=delta-65536 if delta&32768 else delta
   pc+=delta if equal else 4
  elif instruction==0x38600000:pc+=4
  elif instruction==0x4e800020:return False
  else:
   assert pc==32,hex(instruction)
   return True # Execution reached the unmodified native prologue.
 raise AssertionError('gate did not terminate')
for gate in range(3):
 words=list(struct.unpack_from('>13I',data,start+gate*52))
 for i in range(-1,501):assert allowed(words,i)==(i in (0,4,5)),(gate,i)
 words[2]=words[4]=0x2c040000
 for i in range(-1,501):assert allowed(words,i)==(i==0),(gate,i,'completed')
print('PASS: all three binary gates; 500 mission/class indices; free roam; intro; tutorial; completion boundary')
