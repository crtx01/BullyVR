#include "../src/hud_panel.h"
#include <cassert>
#include <cstdio>
int main(){
 HudQuadSample quad[]={{-2,-1,0xff202020},{2,-1,0xff202020},{2,1,0xff202020},{-2,1,0xff202020}};
 assert(isDarkHudPanel(quad,4,.5,1)); // opaque charcoal was missed before
 for(auto& v:quad)v.color=0x50000000;
 assert(isDarkHudPanel(quad,4,.5,1));
 HudQuadSample tutorial[]={{-1.1f,-.2f,0x40101010},{-.4f,-.2f,0x40101010},{-.4f,.1f,0x40101010},{-1.1f,.1f,0x40101010}};
 assert(isDarkHudPanel(tutorial,4,1,1)); // actual short equipment tutorial panel
 quad[0].color=0xffffffff;assert(!isDarkHudPanel(quad,4,.5,1));
 for(auto& v:quad)v.color=0x00202020;
 assert(!isDarkHudPanel(quad,4,.5,1));
 for(auto& v:quad){v.color=0xff202020;v.x*=.05f;}
 assert(!isDarkHudPanel(quad,4,.5,1)); // small dark icons survive
 assert(!isDarkHudPanel(quad,3,.5,1));
 puts("PASS: opaque and translucent dark panels, preserving text/colored graphics and small icons");
}
