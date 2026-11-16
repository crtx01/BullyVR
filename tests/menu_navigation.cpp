#include <cassert>
#include <limits>
#include <cstdio>
#include "../src/menu_navigation.h"
int main(){
 MenuNavigation nav;
 assert(nav.step(-.8f,1)==1);assert(nav.step(-.8f,1.1)==0);
 assert(nav.step(-.8f,1.4)==1);assert(nav.step(-.8f,1.5)==0);
 assert(nav.step(-.8f,1.57)==1);assert(nav.step(.8f,1.58)==-1);
 assert(nav.step(.2f,1.6)==0);assert(nav.step(.8f,1.7)==-1);
 assert(nav.step(std::numeric_limits<float>::quiet_NaN(),2)==0);
 assert(nav.step(-1,2.1)==1);nav.reset();assert(nav.step(-1,2.12)==1);
 puts("PASS: travel navigation repeat, neutral tolerance, reversal, invalid input and reopen");
}
