#include "perm.h"
void pawl_permute(uint64_t s[8], int rounds);
int main(void){ int bad=0; for(int t=0;t<2000;t++){ uint64_t x[8],y[8]; for(int i=0;i<8;i++) x[i]=y[i]=xnext(); int r=1+t%24; perm_f(x,r); pawl_permute(y,r); if(memcmp(x,y,64)) bad++; uint64_t z[8]; memcpy(z,x,64); perm_i(z,r); perm_f(z,r); if(memcmp(z,x,64)) bad++; }
  printf("analysis code vs library permutation, 2000 random states, rounds 1..24: %s\n", bad?"MISMATCH":"identical"); return bad; }
