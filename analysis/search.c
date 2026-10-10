// First-pass design-space exploration for the 512-bit permutation (which column wiring,
// row swap or not, second rotation or not).  This pass XORs the AND term in; the final
// tuning in tune2.c adds it instead, which measured better.
// Family ("braid" round) on two rows a[4], b[4] of 64-bit words:
//   S1: a[i] += b[i]                                  (carry nonlinearity)
//   S2: b[i]  = rotl(b[i], R[i]) ^ a[(i+o2)&3]        (linear diffusion, may cross columns)
//   S3: a[i]  = rotl(a[i], Q[i]) ^ (b[(i+o3)&3] & b[(i+o4)&3])   (AND nonlinearity across columns)
//   S4: optionally swap rows
// We measure diffusion speed (forward and inverse) for random parameter choices.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct { int o2, o3, o4, swap, R[4], Q[4]; } P;

static inline uint64_t rotl(uint64_t x, int r){ r&=63; return r? (x<<r)|(x>>(64-r)) : x; }
static inline uint64_t rotr(uint64_t x, int r){ r&=63; return r? (x>>r)|(x<<(64-r)) : x; }

static uint64_t sm_state = 0x1234567;
static inline uint64_t sm64(void){ uint64_t z=(sm_state+=0x9E3779B97F4A7C15ULL); z=(z^(z>>30))*0xBF58476D1CE4E5B9ULL; z=(z^(z>>27))*0x94D049BB133111EBULL; return z^(z>>31);}

static inline void rnd_f(uint64_t *s, const P *p, uint64_t rc){
  uint64_t *a=s, *b=s+4, t[4];
  a[0]^=rc;
  for(int i=0;i<4;i++) a[i]+=b[i];
  for(int i=0;i<4;i++) t[i]=rotl(b[i],p->R[i])^a[(i+p->o2)&3];
  for(int i=0;i<4;i++) b[i]=t[i];
  for(int i=0;i<4;i++) t[i]=rotl(a[i],p->Q[i])^(b[(i+p->o3)&3]&b[(i+p->o4)&3]);
  if(p->swap){ for(int i=0;i<4;i++){ a[i]=b[i]; b[i]=t[i]; } }
  else       { for(int i=0;i<4;i++) a[i]=t[i]; }
}
static inline void rnd_i(uint64_t *s, const P *p, uint64_t rc){
  uint64_t *a=s, *b=s+4, t[4];
  if(p->swap){ for(int i=0;i<4;i++){ t[i]=a[i]; a[i]=b[i]; b[i]=t[i]; } }
  for(int i=0;i<4;i++) t[i]=rotr(a[i]^(b[(i+p->o3)&3]&b[(i+p->o4)&3]),p->Q[i]);
  for(int i=0;i<4;i++) a[i]=t[i];
  for(int i=0;i<4;i++) t[i]=rotr(b[i]^a[(i+p->o2)&3],p->R[i]);
  for(int i=0;i<4;i++) b[i]=t[i];
  for(int i=0;i<4;i++) a[i]-=b[i];
  a[0]^=rc;
}
static inline uint64_t RC(int r){ return 0x9E3779B97F4A7C15ULL*(uint64_t)(r+1); }

// mean / min (over input bits) Hamming distance after r=1..maxr rounds, both directions
static void quick(const P *p, int N, int maxr, double *minhw_f, double *minhw_i, double *avg_f, double *avg_i){
  static double hf[8][512], hi[8][512];
  memset(hf,0,sizeof hf); memset(hi,0,sizeof hi);
  for(int n=0;n<N;n++){
    uint64_t x[8]; for(int i=0;i<8;i++) x[i]=sm64();
    for(int bit=0;bit<512;bit++){
      uint64_t u[8],v[8];
      memcpy(u,x,64); memcpy(v,x,64); v[bit>>6]^=1ULL<<(bit&63);
      for(int r=0;r<maxr;r++){ rnd_f(u,p,RC(r)); rnd_f(v,p,RC(r));
        int h=0; for(int i=0;i<8;i++) h+=__builtin_popcountll(u[i]^v[i]); hf[r][bit]+=h; }
      memcpy(u,x,64); memcpy(v,x,64); v[bit>>6]^=1ULL<<(bit&63);
      for(int r=0;r<maxr;r++){ rnd_i(u,p,RC(maxr-1-r)); rnd_i(v,p,RC(maxr-1-r));
        int h=0; for(int i=0;i<8;i++) h+=__builtin_popcountll(u[i]^v[i]); hi[r][bit]+=h; }
    }
  }
  for(int r=0;r<maxr;r++){
    double mf=1e9, mi=1e9, af=0, ai=0;
    for(int b=0;b<512;b++){ double f=hf[r][b]/N, g=hi[r][b]/N; if(f<mf)mf=f; if(g<mi)mi=g; af+=f; ai+=g; }
    minhw_f[r]=mf; minhw_i[r]=mi; avg_f[r]=af/512; avg_i[r]=ai/512;
  }
}

static void pr(const P *p){ printf("o2=%d o3=%d o4=%d swap=%d R={%d,%d,%d,%d} Q={%d,%d,%d,%d}", p->o2,p->o3,p->o4,p->swap,p->R[0],p->R[1],p->R[2],p->R[3],p->Q[0],p->Q[1],p->Q[2],p->Q[3]); }

int main(int argc, char **argv){
  int tries = argc>1? atoi(argv[1]) : 100;
  int N = argc>2? atoi(argv[2]) : 16;
  int useQ = argc>3? atoi(argv[3]) : 1;
  int samecol = argc>4? atoi(argv[4]) : 0;   // 1: same rotation in every column
  sm_state = argc>5? strtoull(argv[5],0,0) : 42;
  int pairs[6][2]={{0,1},{0,2},{0,3},{1,2},{1,3},{2,3}};
  // per structure: best score over random rotations
  for(int swap=0;swap<2;swap++) for(int o2=0;o2<4;o2++) for(int pi=0;pi<6;pi++){
    P best; double bests=-1; double bm[4][8];
    for(int t=0;t<tries;t++){
      P p; p.o2=o2; p.o3=pairs[pi][0]; p.o4=pairs[pi][1]; p.swap=swap;
      for(int i=0;i<4;i++){ p.R[i]=1+sm64()%63; p.Q[i]=useQ? (int)(1+sm64()%63):0; }
      if(samecol) for(int i=1;i<4;i++){ p.R[i]=p.R[0]; p.Q[i]=p.Q[0]; }
      double mf[8],mi[8],af[8],ai[8];
      quick(&p,N,5,mf,mi,af,ai);
      // score: worst-case (min over bits, min over direction) diffusion after 3 and 4 rounds
      double s = fmin(mf[2],mi[2]) + fmin(mf[3],mi[3]) + 0.25*fmin(mf[1],mi[1]);
      if(s>bests){ bests=s; best=p; memcpy(bm[0],mf,sizeof mf); memcpy(bm[1],mi,sizeof mi); memcpy(bm[2],af,sizeof af); memcpy(bm[3],ai,sizeof ai); }
    }
    printf("%7.2f | ", bests); pr(&best);
    printf(" | minF r2..5: %.1f %.1f %.1f %.1f | minI: %.1f %.1f %.1f %.1f\n", bm[0][1],bm[0][2],bm[0][3],bm[0][4], bm[1][1],bm[1][2],bm[1][3],bm[1][4]);
    fflush(stdout);
  }
  return 0;
}
