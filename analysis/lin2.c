// Linear-approximation search on the round-reduced permutation.
// Take a 2^n subcube of inputs (n free bit positions, the other 512-n bits fixed at random),
// and for an output mask m compute the full Walsh spectrum of x -> parity(P_r(x) & m).
// That gives the correlation of EVERY input mask on those n bits at once.  Any strong linear
// approximation of the full permutation (whatever its input mask) leaves energy in this spectrum.
// Output masks tried: all 512 single bits and all 512 pairs of adjacent bits (the shape ARX trails favour).
//   lin2 r n trials where      where = rand | low (lowest bits of each word)
#include "perm.h"
static void wht(int32_t *a, long n){ for(long h=1;h<n;h<<=1) for(long i=0;i<n;i+=h<<1) for(long j=i;j<i+h;j++){ int32_t x=a[j],y=a[j+h]; a[j]=x+y; a[j+h]=x-y; } }
int main(int argc,char**argv){
  int r=atoi(argv[1]), n=atoi(argv[2]), T=atoi(argv[3]); const char *where=argv[4]; long L=1L<<n;
  uint64_t (*Y)[8]=malloc(sizeof(uint64_t)*8*L); int32_t *s=malloc(sizeof(int32_t)*L);
  double gmax=0; int gout=0; long gmask=0; long big=0, total=0; xseed(606+r*13+n);
  for(int t=0;t<T;t++){
    int pos[32];
    for(int k=0;k<n;k++){ if(!strcmp(where,"low")) pos[k]=(k%8)*64+(k/8); else { again: pos[k]=xnext()%512; for(int q=0;q<k;q++) if(pos[q]==pos[k]) goto again; } }
    uint64_t base[8]; for(int i=0;i<8;i++) base[i]=xnext();
    for(int k=0;k<n;k++) base[pos[k]>>6]&=~(1ULL<<(pos[k]&63));
    for(long v=0;v<L;v++){ uint64_t x[8]; memcpy(x,base,64); for(int k=0;k<n;k++) if((v>>k)&1) x[pos[k]>>6]|=1ULL<<(pos[k]&63); perm_f(x,r); memcpy(Y[v],x,64); }
    for(int om=0;om<1024;om++){
      int j=om&511, w=j>>6, b=j&63; int pair=om>=512; int w2=((j+1)&511)>>6, b2=(j+1)&63; if(pair && w2!=w) continue; // adjacent bits inside one word
      for(long v=0;v<L;v++){ int bit=(Y[v][w]>>b)&1; if(pair) bit^=(Y[v][w2]>>b2)&1; s[v]=1-2*bit; }
      wht(s,L);
      for(long m=0;m<L;m++){ double c=fabs((double)s[m])/L; if(c>gmax){gmax=c;gout=om;gmask=m;} if(c*sqrt((double)L)>5.0) big++; total++; }
    }
  }
  double sigma=1.0/sqrt((double)L), expmax=sqrt(2*log(2.0*total))*sigma, expbig=total*5.733e-7;
  printf("linear r=%d: subcube 2^%d (%s) x %d trials: max |corr| = %.5f = 2^%.1f (random function would give ~%.5f); spectrum entries beyond 5 sigma: %ld (expected ~%.0f)  [worst: %s output bit %d, input mask %lx]\n",
     r,n,where,T,gmax,log2(gmax),expmax,big,expbig, gout>=512?"pair at":"single", gout&511, gmask);
  return 0;
}
