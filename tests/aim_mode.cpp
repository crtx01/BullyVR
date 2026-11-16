#include "../src/aim_mode.h"
#include <cassert>
#include <cstdio>
int main(){
 AimMode mode;
 assert(!mode.step(false,0,true).manual);
 auto state=mode.step(true,1,true);assert(state.manual&&state.enter&&!state.exit);
 state=mode.step(true,1.2,true);assert(state.manual&&!state.enter);
 state=mode.step(false,2,true);assert(state.manual&&!state.exit);
 state=mode.step(false,2.29,true);assert(state.manual&&!state.exit);
 state=mode.step(false,2.31,true);assert(state.manual&&state.exit);
 assert(!mode.step(false,2.44,true).manual);
 mode.step(true,3,true);mode.step(false,3.2,true);
 state=mode.step(true,3.3,true);assert(state.manual&&!state.enter&&!state.exit);
 assert(!mode.step(true,4,false).manual);
 puts("PASS: enter, trigger-release firing grace, delayed exit, retap without toggle, menu cancellation");
}
