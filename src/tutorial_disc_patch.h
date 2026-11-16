// Applies the localized record patch before any disc is opened by emulation.
static void patchTutorialDisc(){
 constexpr off_t offset=2250692876;constexpr size_t size=862208;
 auto patch=readAsset("American-Quest.img");if(patch.size()!=size)return;
 const char* path=BULLY_FILES "/Bully.nkit.iso";
 FILE* file=fopen(path,"r+b");if(!file)return;
 char id[6]{};fread(id,1,6,file);if(memcmp(id,"RB7E54",6)){fclose(file);return;}
 if(fseeko(file,offset,SEEK_SET)){fclose(file);return;}
 std::vector<uint8_t> data(size);if(fread(data.data(),1,size,file)!=size){fclose(file);return;}
 uint64_t hash=14695981039346656037ULL;for(uint8_t byte:data)hash=(hash^byte)*1099511628211ULL;
 if(hash==11285889282924856103ULL){fclose(file);return;}
 if(hash!=15073658025615542930ULL){LOG("Tutorial record patch skipped: unknown text region");fclose(file);return;}
 if(!fseeko(file,offset,SEEK_SET)&&fwrite(patch.data(),1,size,file)==size){fflush(file);LOG("Validated Quest tutorial records applied: 51 strings");}
 fclose(file);
}
