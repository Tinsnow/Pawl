// The Pawl permutation, written for analysis (any round count, forward and inverse).
// VARIANT 1 is Pawl. VARIANT 0 XORs the AND term in instead of adding it (rejected: slower diffusion).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef VARIANT
#define VARIANT 1
#endif
static int ROT_R[4] = {18, 50, 58, 37};
static int ROT_Q[4] = {34, 30, 54, 4};
static inline uint64_t rotl(uint64_t x, int r){ return (x<<r)|(x>>(64-r)); }
static inline uint64_t rotr(uint64_t x, int r){ return (x>>r)|(x<<(64-r)); }
static inline uint64_t RC(int r){ return 0x9E3779B97F4A7C15ULL*(uint64_t)(r+1); }
static int USE_RC = 1;

static inline void rnd_f(uint64_t *s, int r){
  uint64_t *a=s, *b=s+4, t[4];
  if(USE_RC) a[0]^=RC(r);
  for(int i=0;i<4;i++) a[i]+=b[i];
  for(int i=0;i<4;i++) t[i]=rotl(b[i],ROT_R[i])^a[(i+1)&3];
  for(int i=0;i<4;i++) b[i]=t[i];
#if VARIANT==0
  for(int i=0;i<4;i++) t[i]=rotl(a[i],ROT_Q[i])^(b[(i+1)&3]&b[(i+2)&3]);
#else
  for(int i=0;i<4;i++) t[i]=rotl(a[i],ROT_Q[i])+(b[(i+1)&3]&b[(i+2)&3]);
#endif
  for(int i=0;i<4;i++){ a[i]=b[i]; b[i]=t[i]; }
}
static inline void rnd_i(uint64_t *s, int r){
  uint64_t *a=s, *b=s+4, t[4];
  for(int i=0;i<4;i++){ t[i]=a[i]; a[i]=b[i]; b[i]=t[i]; }
#if VARIANT==0
  for(int i=0;i<4;i++) t[i]=rotr(a[i]^(b[(i+1)&3]&b[(i+2)&3]),ROT_Q[i]);
#else
  for(int i=0;i<4;i++) t[i]=rotr(a[i]-(b[(i+1)&3]&b[(i+2)&3]),ROT_Q[i]);
#endif
  for(int i=0;i<4;i++) a[i]=t[i];
  for(int i=0;i<4;i++) t[i]=rotr(b[i]^a[(i+1)&3],ROT_R[i]);
  for(int i=0;i<4;i++) b[i]=t[i];
  for(int i=0;i<4;i++) a[i]-=b[i];
  if(USE_RC) a[0]^=RC(r);
}
static inline void perm_f(uint64_t *s, int rounds){ for(int r=0;r<rounds;r++) rnd_f(s,r); }
static inline void perm_i(uint64_t *s, int rounds){ for(int r=rounds-1;r>=0;r--) rnd_i(s,r); }

// xoshiro256** for test inputs (independent of the design under test)
static uint64_t xs[4]={0x243F6A8885A308D3ULL,0x13198A2E03707344ULL,0xA4093822299F31D0ULL,0x082EFA98EC4E6C89ULL};
static inline uint64_t xnext(void){ uint64_t r=rotl(xs[1]*5,7)*9, t=xs[1]<<17; xs[2]^=xs[0]; xs[3]^=xs[1]; xs[1]^=xs[2]; xs[0]^=xs[3]; xs[2]^=t; xs[3]=rotl(xs[3],45); return r; }
static void xseed(uint64_t s){ for(int i=0;i<4;i++){ s+=0x9E3779B97F4A7C15ULL; uint64_t z=s; z=(z^(z>>30))*0xBF58476D1CE4E5B9ULL; z=(z^(z>>27))*0x94D049BB133111EBULL; xs[i]=z^(z>>31);} }
