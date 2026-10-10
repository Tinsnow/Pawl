// Extend a known difference backwards by k rounds.
// Given a difference D (as bit list) that is wanted AFTER round k, sample random states and
// tabulate which input differences produce it; print the most frequent ones with their probability.
//   backdiff N top k D1 D2 ...        (N samples; needs 64*N bytes of memory)
#include "perm.h"
typedef struct { uint64_t d[8]; } D512;
static int cmp(const void*a,const void*b){ return memcmp(a,b,64); }
int main(int argc,char**argv){
  long N=atol(argv[1]); int top=atoi(argv[2]); int kr=atoi(argv[3]);
  for(int ai=4;ai<argc;ai++){
    int bits[64],nb=0; char buf[256]; strncpy(buf,argv[ai],255); buf[255]=0; for(char*t=strtok(buf,"+");t;t=strtok(0,"+")) bits[nb++]=atoi(t);
    D512 *v=malloc(sizeof(D512)*N); xseed(1234+ai);
    for(long n=0;n<N;n++){ uint64_t x[8],y[8]; for(int i=0;i<8;i++) x[i]=xnext(); memcpy(y,x,64);
      for(int q=0;q<kr;q++) rnd_f(y,q); for(int k=0;k<nb;k++) y[bits[k]>>6]^=1ULL<<(bits[k]&63); for(int q=kr-1;q>=0;q--) rnd_i(y,q);
      for(int i=0;i<8;i++) v[n].d[i]=x[i]^y[i]; }
    qsort(v,N,sizeof(D512),cmp);
    // run-length count, keep the top few
    long bestc[16]={0}; D512 bestd[16];
    for(long i=0;i<N;){ long j=i; while(j<N && !memcmp(&v[i],&v[j],64)) j++; long c=j-i;
      for(int t=0;t<top;t++) if(c>bestc[t]){ for(int u=top-1;u>t;u--){bestc[u]=bestc[u-1];bestd[u]=bestd[u-1];} bestc[t]=c; bestd[t]=v[i]; break; }
      i=j; }
    printf("target difference after round %d: %s\n",kr,argv[ai]);
    for(int t=0;t<top;t++){ if(!bestc[t]) break; printf("  p=2^%.2f  input difference: ",log2((double)bestc[t]/N)); int first=1;
      for(int b=0;b<512;b++) if((bestd[t].d[b>>6]>>(b&63))&1){ printf("%s%d",first?"":"+",b); first=0; } printf("\n"); }
    free(v);
  }
}
