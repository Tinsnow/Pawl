// Structural tests on the round-reduced permutation.
//   struct rot  N              rotational pairs: how far does P(rot(x)) stay close to rot(P(x))?
//   struct cube r d T where    higher-order differential: XOR of outputs over a d-dim cube of inputs
//                              where = lowb (low bits of b0, i.e. a counter), lowa, high, rand
#include "perm.h"
#include <omp.h>

static uint64_t rotw(uint64_t x,int k){ return (x<<k)|(x>>(64-k)); }

int main(int argc,char**argv){
  const char *mode=argv[1];
  if(!strcmp(mode,"rot")){
    long N=atol(argv[2]);
    for(int rc=0;rc<2;rc++){ USE_RC=rc;
      printf("%s round constants: mean Hamming distance between P(rot1(x)) and rot1(P(x)) (256 = unrelated)\n   ", rc?"WITH":"WITHOUT");
      for(int r=1;r<=8;r++){ xseed(31+r); double s=0; long exact=0;
        for(long n=0;n<N;n++){ uint64_t x[8],y[8]; for(int i=0;i<8;i++){ x[i]=xnext(); y[i]=rotw(x[i],1);} perm_f(x,r); perm_f(y,r);
          int h=0; for(int i=0;i<8;i++) h+=__builtin_popcountll(y[i]^rotw(x[i],1)); s+=h; if(!h) exact++; }
        printf("r%d: %.1f%s  ", r, s/N, exact? " (exact pairs seen)":""); }
      printf("\n"); }
    USE_RC=1;
  } else if(!strcmp(mode,"cube")){
    int r=atoi(argv[2]), d=atoi(argv[3]), T=atoi(argv[4]); const char *where=argv[5];
    int pos[64]; xseed(4242+d);
    // number of output bits whose cube sum was zero in every trial, and overall fraction of ones
    int zero_all[512]; for(int j=0;j<512;j++) zero_all[j]=1; long ones=0;
    for(int t=0;t<T;t++){
      if(!strcmp(where,"lowb")) for(int k=0;k<d;k++) pos[k]=256+k;            // low bits of b0 (the counter word)
      else if(!strcmp(where,"lowa")) for(int k=0;k<d;k++) pos[k]=k;           // low bits of a0
      else if(!strcmp(where,"high")) for(int k=0;k<d;k++) pos[k]=256+63-k;    // high bits of b0
      else if(!strcmp(where,"spread")) for(int k=0;k<d;k++) pos[k]=(k%8)*64 + (k/8); // lowest bits of every word
      else { for(int k=0;k<d;k++){ again: pos[k]=xnext()%512; for(int q=0;q<k;q++) if(pos[q]==pos[k]) goto again; } }
      uint64_t base[8]; for(int i=0;i<8;i++) base[i]=xnext();
      for(int k=0;k<d;k++) base[pos[k]>>6]&=~(1ULL<<(pos[k]&63));
      uint64_t acc[8]={0};
      #pragma omp parallel
      { uint64_t loc[8]={0};
        #pragma omp for schedule(static)
        for(long v=0;v<(1L<<d);v++){ uint64_t x[8]; memcpy(x,base,64);
          for(int k=0;k<d;k++) if((v>>k)&1) x[pos[k]>>6]|=1ULL<<(pos[k]&63);
          perm_f(x,r); for(int i=0;i<8;i++) loc[i]^=x[i]; }
        #pragma omp critical
        for(int i=0;i<8;i++) acc[i]^=loc[i]; }
      for(int j=0;j<512;j++){ int bit=(acc[j>>6]>>(j&63))&1; if(bit){ zero_all[j]=0; ones++; } }
    }
    int nz=0; for(int j=0;j<512;j++) nz+=zero_all[j];
    double frac=(double)ones/(512.0*T), z=(ones-256.0*T)/sqrt(128.0*T);
    printf("cube r=%d dim=%2d %-6s trials=%d: output bits with zero sum in ALL trials = %3d/512 (ideal %.2f), fraction of ones = %.4f (z=%+.1f)\n", r,d,where,T,nz,512.0/pow(2,T),frac,z);
  }
  return 0;
}
