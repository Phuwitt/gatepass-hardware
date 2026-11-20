#include <initializer_list>

#include <cassert>
#include <cstdint>
#include "../include/AccessPolicy.h"
int main(){
 for(int mask=0;mask<32;++mask){
  const bool a=mask&1,s=mask&2,l=mask&4,f=mask&8,repeated=mask&16;
  assert(permitPulse(a,s,l,f,repeated,3000)==(a&&s&&l&&f&&!repeated));
 }
 for(uint32_t ms:{0u,99u,5001u,0xffffffffu})assert(!permitPulse(true,true,true,true,false,ms));
 assert(elapsed(25u,0xfffffff0u,40u));
 assert(!elapsed(20u,0xfffffff0u,40u));
}
