#include "../src/charged_punch.h"
#include <cassert>
#include <cstdio>
int main(){
 ChargedPunch detector;bool tracked[2]{true,true};float f[2]{.3f,.3f};
 assert(!detector.step(f,tracked,1,true).attack);
 f[1]=.55f;assert(!detector.step(f,tracked,1.05,true).attack); // ordinary forward punch
 f[1]=-.2f;assert(detector.step(f,tracked,1.1,true).attack);
 assert(detector.step(f,tracked,1.2,true).suppress[1]);
 f[1]=.25f;auto hit=detector.step(f,tracked,1.3,true);assert(hit.attack&&hit.punch[1]&&!hit.punch[0]);
 assert(detector.step(f,tracked,1.35,true).punch[1]);
 assert(!detector.step(f,tracked,1.5,true).attack);
 f[1]=-.3f;assert(!detector.step(f,tracked,1.55,true).attack); // cooldown
 detector.step(f,tracked,1.6,false);assert(!detector.step(f,tracked,1.65,true).attack);
 tracked[1]=false;assert(!detector.step(f,tracked,1.7,true).attack);
 // A tracking discontinuity cannot turn a jumped hand position into a hit.
 tracked[1]=true;f[1]=.6f;assert(!detector.step(f,tracked,2,true).punch[1]);
 puts("PASS: quick punch unchanged, wind-up heavy, bounded pulse, cooldown and tracking reset");
}
