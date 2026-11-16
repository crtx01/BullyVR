// Shared Bully palette for the travel panel and controller help.
namespace wheelStyle {
constexpr ImU32 ink=IM_COL32(16,16,18,244),gold=IM_COL32(82,214,242,255),paper=IM_COL32(209,224,230,255),muted=IM_COL32(145,174,187,255),error=IM_COL32(255,150,120,255);
constexpr ImU32 selection=IM_COL32(111,36,109,255),selectedText=IM_COL32(255,242,150,255);
}
static ImFont* menuFont(ImDrawList* dl){
 auto* font=dl->_Data->Font;
 if(font&&font->ContainerAtlas&&font->ContainerAtlas->Fonts.Size>0)return font->ContainerAtlas->Fonts[0];
 return font;
}
static void menuText(ImDrawList* dl,ImVec2 at,float size,ImU32 color,const char* text){dl->AddText(menuFont(dl),size,at,color,text);}
static void centeredText(ImDrawList* dl,ImVec2 at,float size,ImU32 color,const char* text,float wrap=0){
 auto* font=dl->_Data->Font;auto extent=font->CalcTextSizeA(size,10000.f,wrap,text);
 dl->AddText(font,size,{at.x-extent.x*.5f,at.y-extent.y*.5f},color,text,nullptr,wrap);
}
