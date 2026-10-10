// Rotation-constant tuning for the chosen round shape (AND term added in, rows swap).
// Objective: maximise the WORST-CASE diffusion over
//   - all 512 single-bit input differences, forward and inverse, and
//   - all 256 "cancelling" two-bit differences (a[i] bit k, b[i] bit k), forward,
// measured as mean output Hamming distance after 3, 4 and 5 rounds.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
typedef struct { int o2,o3,o4,R[4],Q[4]; } P;
static inline uint64_t rotl(uint64_t x,int r){ return (x<<r)|(x>>(64-r)); }
static inline uint64_t rotr(uint64_t x,int r){ return (x>>r)|(x<<(64-r)); }
static uint64_t sm_state=1;
static inline uint64_t sm64(void){ uint64_t z=(sm_state+=0x9E3779B97F4A7C15ULL); z=(z^(z>>30))*0xBF58476D1CE4E5B9ULL; z=(z^(z>>27))*0x94D049BB133111EBULL; return z^(z>>31);}
static inline uint64_t RC(int r){ return 0x9E3779B97F4A7C15ULL*(uint64_t)(r+1); }
static inline void rf(uint64_t *s,const P*p,int r){ uint64_t *a=s,*b=s+4,t[4]; a[0]^=RC(r);
  for(int i=0;i<4;i++) a[i]+=b[i];
  for(int i=0;i<4;i++) t[i]=rotl(b[i],p->R[i])^a[(i+p->o2)&3];
  for(int i=0;i<4;i++) b[i]=t[i];
  for(int i=0;i<4;i++) t[i]=rotl(a[i],p->Q[i])+(b[(i+p->o3)&3]&b[(i+p->o4)&3]);
  for(int i=0;i<4;i++){ a[i]=b[i]; b[i]=t[i]; } }
static inline void ri(uint64_t *s,const P*p,int r){ uint64_t *a=s,*b=s+4,t[4];
  for(int i=0;i<4;i++){ t[i]=a[i]; a[i]=b[i]; b[i]=t[i]; }
  for(int i=0;i<4;i++) t[i]=rotr(a[i]-(b[(i+p->o3)&3]&b[(i+p->o4)&3]),p->Q[i]);
  for(int i=0;i<4;i++) a[i]=t[i];
  for(int i=0;i<4;i++) t[i]=rotr(b[i]^a[(i+p->o2)&3],p->R[i]);
  for(int i=0;i<4;i++) b[i]=t[i];
  for(int i=0;i<4;i++) a[i]-=b[i];
  a[0]^=RC(r); }
#define MAXR 5
// d[0..2]: min HW fwd single at r=3,4,5 ; d[3..5]: inverse single ; d[6..8]: fwd cancelling pairs
static double score(const P*p,int N,double *d){
  static double h[3][MAXR][512]; memset(h,0,sizeof h);
  for(int n=0;n<N;n++){ uint64_t x[8]; for(int i=0;i<8;i++) x[i]=sm64();
    for(int bit=0;bit<512;bit++){ uint64_t u[8],v[8];
      memcpy(u,x,64); memcpy(v,x,64); v[bit>>6]^=1ULL<<(bit&63);
      for(int r=0;r<MAXR;r++){ rf(u,p,r); rf(v,p,r); if(r>=2){ int c=0; for(int i=0;i<8;i++) c+=__builtin_popcountll(u[i]^v[i]); h[0][r][bit]+=c; } }
      memcpy(u,x,64); memcpy(v,x,64); v[bit>>6]^=1ULL<<(bit&63);
      for(int r=0;r<MAXR;r++){ ri(u,p,MAXR-1-r); ri(v,p,MAXR-1-r); if(r>=2){ int c=0; for(int i=0;i<8;i++) c+=__builtin_popcountll(u[i]^v[i]); h[1][r][bit]+=c; } }
      if(bit<256){ memcpy(u,x,64); memcpy(v,x,64); v[bit>>6]^=1ULL<<(bit&63); v[4+(bit>>6)]^=1ULL<<(bit&63);
        for(int r=0;r<MAXR;r++){ rf(u,p,r); rf(v,p,r); if(r>=2){ int c=0; for(int i=0;i<8;i++) c+=__builtin_popcountll(u[i]^v[i]); h[2][r][bit]+=c; } } }
    } }
  for(int k=0;k<3;k++) for(int r=2;r<MAXR;r++){ double m=1e9; int lim=(k==2)?256:512; for(int b=0;b<lim;b++){ double v=h[k][r][b]/N; if(v<m)m=v; } d[k*3+(r-2)]=m; }
  // worst case across the three difference classes at rounds 4 and 5 (+ small weight on round 3)
  double w4=fmin(d[1],fmin(d[4],d[7])), w5=fmin(d[2],fmin(d[5],d[8])), w3=fmin(d[0],fmin(d[3],d[6]));
  return w4 + 2.0*w5 + 0.5*w3;
}
static void pr(const P*p){ printf("o=(%d,%d,%d) R={%d,%d,%d,%d} Q={%d,%d,%d,%d}",p->o2,p->o3,p->o4,p->R[0],p->R[1],p->R[2],p->R[3],p->Q[0],p->Q[1],p->Q[2],p->Q[3]); }
int main(int argc,char**argv){
  P p; p.o2=atoi(argv[1]); p.o3=atoi(argv[2]); p.o4=atoi(argv[3]); int starts=atoi(argv[4]); sm_state=strtoull(argv[5],0,0);
  int stale_lim=argc>6?atoi(argv[6]):100;
  for(int s=0;s<starts;s++){
    for(int i=0;i<4;i++){ p.R[i]=1+sm64()%63; p.Q[i]=1+sm64()%63; }
    double d[9]; double best=score(&p,24,d); int stale=0;
    while(stale<stale_lim){ P q=p; int k=sm64()%8; int v=1+sm64()%63; if(k<4) q.R[k]=v; else q.Q[k-4]=v;
      double sc=score(&q,24,d); if(sc>best+1.0){ best=sc; p=q; stale=0; } else stale++; }
    uint64_t keep=sm_state; sm_state=424242; double fin=score(&p,192,d); sm_state=keep;
    printf("%8.2f | ",fin); pr(&p); printf(" | fwd1 %.0f %.0f %.1f | inv1 %.0f %.0f %.1f | fwd2 %.0f %.0f %.1f\n",d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7],d[8]); fflush(stdout);
  }
}
