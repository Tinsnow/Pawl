// Differential bias hunting on the round-reduced permutation.
//  dbias all  r N dir          : every 1-bit input difference, rank inputs by chi-square over the 512 output bits
//  dbias pairs r N dir         : every 2-bit input difference (quick Hamming-weight screen), list slowest
//  dbias list r N dir d1 d2 .. : specific differences ("63" or "63+319"), high precision
// dir: 0 forward, 1 inverse.  Output z = strongest single output-bit bias in sigmas, chi z = pooled.
#include "perm.h"
#include <omp.h>
static void apply(uint64_t *v,int r,int inv){ if(inv) perm_i(v,r); else perm_f(v,r); }
static int parse(const char *s, int *bits){ int n=0; char buf[256]; strncpy(buf,s,255); buf[255]=0; char *t=strtok(buf,"+"); while(t){ bits[n++]=atoi(t); t=strtok(0,"+"); } return n; }

int main(int argc,char**argv){
  const char *mode=argv[1]; int r=atoi(argv[2]); long N=atol(argv[3]); int inv=atoi(argv[4]);
  if(!strcmp(mode,"all")){
    static uint32_t cnt[512][512]; xseed(77+r+inv*100);
    for(long n=0;n<N;n++){
      uint64_t x[8],y[8]; for(int i=0;i<8;i++) x[i]=xnext(); memcpy(y,x,64); apply(y,r,inv);
      for(int bit=0;bit<512;bit++){ uint64_t v[8]; memcpy(v,x,64); v[bit>>6]^=1ULL<<(bit&63); apply(v,r,inv);
        uint32_t *c=cnt[bit]; for(int w=0;w<8;w++){ uint64_t d=v[w]^y[w]; while(d){ c[w*64+__builtin_ctzll(d)]++; d&=d-1; } } }
    }
    double chi[512], maxz=0; int mi=0,mj=0; double tot=0;
    for(int i=0;i<512;i++){ chi[i]=0; for(int j=0;j<512;j++){ double z=(cnt[i][j]-N/2.0)/sqrt(N/4.0); chi[i]+=z*z; if(fabs(z)>maxz){maxz=fabs(z);mi=i;mj=j;} } tot+=chi[i]; }
    printf("%s r=%d N=%ld: global chi z=%+.2f, max single-entry |z|=%.2f (in=%d,out=%d; ideal max ~%.1f)\n", inv?"INV":"FWD", r,N,(tot-262144)/sqrt(2*262144.0),maxz,mi,mj, sqrt(2*log(2*262144.0)));
    printf("  worst input bits (chi z over 512 outputs): ");
    for(int k=0;k<10;k++){ int b=0; for(int i=1;i<512;i++) if(chi[i]>chi[b]) b=i; printf("%d(%+.1f) ", b,(chi[b]-512)/sqrt(1024.0)); chi[b]=-1; }
    printf("\n");
  } else if(!strcmp(mode,"pairs")){
    // 2-bit differences: mean output Hamming distance, find slowest-diffusing pairs
    int np=512*511/2; float *hw=malloc(sizeof(float)*np); int *pa=malloc(sizeof(int)*np), *pb=malloc(sizeof(int)*np);
    int k=0; for(int i=0;i<512;i++) for(int j=i+1;j<512;j++){ pa[k]=i; pb[k]=j; k++; }
    uint64_t (*X)[8]=malloc(sizeof(uint64_t)*8*N), (*Y)[8]=malloc(sizeof(uint64_t)*8*N);
    xseed(5); for(long n=0;n<N;n++){ for(int i=0;i<8;i++) X[n][i]=xnext(); memcpy(Y[n],X[n],64); apply(Y[n],r,inv); }
    #pragma omp parallel for schedule(dynamic,512)
    for(int q=0;q<np;q++){ long s=0; for(long n=0;n<N;n++){ uint64_t v[8]; memcpy(v,X[n],64); v[pa[q]>>6]^=1ULL<<(pa[q]&63); v[pb[q]>>6]^=1ULL<<(pb[q]&63); apply(v,r,inv);
        for(int w=0;w<8;w++) s+=__builtin_popcountll(v[w]^Y[n][w]); } hw[q]=(float)s/N; }
    printf("%s r=%d: slowest 2-bit input differences (mean output Hamming distance, ideal 256):\n  ", inv?"INV":"FWD", r);
    for(int t=0;t<12;t++){ int b=0; for(int q=1;q<np;q++) if(hw[q]<hw[b]) b=q; printf("%d+%d(%.1f) ",pa[b],pb[b],hw[b]); hw[b]=1e9; }
    printf("\n");
  } else {
    int nd=argc-5;
    #pragma omp parallel for schedule(dynamic,1)
    for(int d=0;d<nd;d++){
      int bits[64]; int nb=parse(argv[5+d],bits);
      uint64_t st[4]; { uint64_t s=0xABCDEF+d*7919+r; for(int i=0;i<4;i++){ s+=0x9E3779B97F4A7C15ULL; uint64_t z=s; z=(z^(z>>30))*0xBF58476D1CE4E5B9ULL; z=(z^(z>>27))*0x94D049BB133111EBULL; st[i]=z^(z>>31);} }
      uint64_t cnt[512]; memset(cnt,0,sizeof cnt);
      // local xoshiro
      for(long n=0;n<N;n++){
        uint64_t x[8],v[8];
        for(int i=0;i<8;i++){ uint64_t rr=rotl(st[1]*5,7)*9, t=st[1]<<17; st[2]^=st[0]; st[3]^=st[1]; st[1]^=st[2]; st[0]^=st[3]; st[2]^=t; st[3]=rotl(st[3],45); x[i]=rr; }
        memcpy(v,x,64); for(int k=0;k<nb;k++) v[bits[k]>>6]^=1ULL<<(bits[k]&63);
        apply(x,r,inv); apply(v,r,inv);
        for(int w=0;w<8;w++){ uint64_t dd=v[w]^x[w]; while(dd){ cnt[w*64+__builtin_ctzll(dd)]++; dd&=dd-1; } }
      }
      double chi=0,maxz=0; int mj=0; for(int j=0;j<512;j++){ double z=(cnt[j]-N/2.0)/sqrt(N/4.0); chi+=z*z; if(fabs(z)>maxz){maxz=fabs(z);mj=j;} }
      #pragma omp critical
      printf("%s r=%d N=2^%.0f diff=%-12s chi z=%+7.2f  max|z|=%6.2f at out bit %3d (bias %.2e)   [ideal: chi z~N(0,1), max|z|~3.9]\n", inv?"INV":"FWD", r, log2((double)N), argv[5+d], (chi-512)/sqrt(1024.0), maxz, mj, maxz*0.5/sqrt((double)N));
    }
  }
  return 0;
}
