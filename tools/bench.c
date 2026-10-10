#define _POSIX_C_SOURCE 199309L   /* clock_gettime */
/* Throughput comparison, single thread, same compiler and flags for everything.
 * Each generator fills the same 64 KiB buffer repeatedly for about a second.
 *   pawl          this generator (24 rounds, key replaced every 32 bytes)
 *   chacha20      plain portable C ChaCha20 (RFC 8439 block function, no SIMD intrinsics)
 *   xoshiro256++  fast statistical generator (not unpredictable)
 *   mt19937-64    Mersenne Twister (not unpredictable)
 */
#include "../pawl.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BUF 65536
static unsigned char buf[BUF];
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec + t.tv_nsec*1e-9; }

/* ---- ChaCha20 (portable reference) ---- */
#define R32(x,n) (((x)<<(n))|((x)>>(32-(n))))
#define QR(a,b,c,d) a+=b; d^=a; d=R32(d,16); c+=d; b^=c; b=R32(b,12); a+=b; d^=a; d=R32(d,8); c+=d; b^=c; b=R32(b,7);
static uint32_t cc[16];
static void chacha_init(const unsigned char key[32], uint32_t counter, const unsigned char nonce[12]){
    int i; cc[0]=0x61707865; cc[1]=0x3320646e; cc[2]=0x79622d32; cc[3]=0x6b206574;
    for(i=0;i<8;i++) cc[4+i]=(uint32_t)key[4*i]|(uint32_t)key[4*i+1]<<8|(uint32_t)key[4*i+2]<<16|(uint32_t)key[4*i+3]<<24;
    cc[12]=counter;
    for(i=0;i<3;i++) cc[13+i]=(uint32_t)nonce[4*i]|(uint32_t)nonce[4*i+1]<<8|(uint32_t)nonce[4*i+2]<<16|(uint32_t)nonce[4*i+3]<<24;
}
static void chacha_block(unsigned char out[64]){
    uint32_t x[16]; int i;
    memcpy(x,cc,sizeof x);
    for(i=0;i<10;i++){ QR(x[0],x[4],x[8],x[12]) QR(x[1],x[5],x[9],x[13]) QR(x[2],x[6],x[10],x[14]) QR(x[3],x[7],x[11],x[15])
                       QR(x[0],x[5],x[10],x[15]) QR(x[1],x[6],x[11],x[12]) QR(x[2],x[7],x[8],x[13]) QR(x[3],x[4],x[9],x[14]) }
    for(i=0;i<16;i++){ uint32_t v=x[i]+cc[i]; out[4*i]=(unsigned char)v; out[4*i+1]=(unsigned char)(v>>8); out[4*i+2]=(unsigned char)(v>>16); out[4*i+3]=(unsigned char)(v>>24); }
    cc[12]++;
}
static void chacha_fill(unsigned char *o, size_t n){ size_t i; for(i=0;i<n;i+=64) chacha_block(o+i); }

/* ---- xoshiro256++ ---- */
static uint64_t xs[4]={1,2,3,4};
#define R64(x,n) (((x)<<(n))|((x)>>(64-(n))))
static inline uint64_t xoshiro(void){ uint64_t r=R64(xs[0]+xs[3],23)+xs[0], t=xs[1]<<17; xs[2]^=xs[0]; xs[3]^=xs[1]; xs[1]^=xs[2]; xs[0]^=xs[3]; xs[2]^=t; xs[3]=R64(xs[3],45); return r; }
static void xoshiro_fill(unsigned char *o, size_t n){ size_t i; for(i=0;i<n;i+=8){ uint64_t v=xoshiro(); memcpy(o+i,&v,8);} }

/* ---- MT19937-64 ---- */
#define NN 312
#define MM 156
static uint64_t mt[NN]; static int mti=NN+1;
static void mt_seed(uint64_t s){ mt[0]=s; for(mti=1;mti<NN;mti++) mt[mti]=6364136223846793005ULL*(mt[mti-1]^(mt[mti-1]>>62))+(uint64_t)mti; }
static uint64_t mt_next(void){ static const uint64_t mag[2]={0,0xB5026F5AA96619E9ULL}; uint64_t x; int i;
    if(mti>=NN){ for(i=0;i<NN-MM;i++){ x=(mt[i]&0xFFFFFFFF80000000ULL)|(mt[i+1]&0x7FFFFFFFULL); mt[i]=mt[i+MM]^(x>>1)^mag[x&1]; }
        for(;i<NN-1;i++){ x=(mt[i]&0xFFFFFFFF80000000ULL)|(mt[i+1]&0x7FFFFFFFULL); mt[i]=mt[i+(MM-NN)]^(x>>1)^mag[x&1]; }
        x=(mt[NN-1]&0xFFFFFFFF80000000ULL)|(mt[0]&0x7FFFFFFFULL); mt[NN-1]=mt[MM-1]^(x>>1)^mag[x&1]; mti=0; }
    x=mt[mti++]; x^=(x>>29)&0x5555555555555555ULL; x^=(x<<17)&0x71D67FFFEDA60000ULL; x^=(x<<37)&0xFFF7EEE000000000ULL; x^=x>>43; return x; }
static void mt_fill(unsigned char *o, size_t n){ size_t i; for(i=0;i<n;i+=8){ uint64_t v=mt_next(); memcpy(o+i,&v,8);} }

static pawl_t g;
static void pawl_fill_(unsigned char *o, size_t n){ pawl_fill(&g,o,n); }

static void bench(const char *name, void (*fill)(unsigned char*,size_t)){
    double best=0; int rep;
    for(rep=0;rep<5;rep++){ double t0=now(), t; long n=0; unsigned char acc=0;
        do { fill(buf,BUF); acc^=buf[n&(BUF-1)]; n++; t=now()-t0; } while(t<0.4);
        if(n*(double)BUF/t>best) best=n*(double)BUF/t;
        (void)acc; }
    printf("%-14s %7.0f MB/s   %5.2f ns per 64-bit value\n", name, best/1e6, 8e9/best);
}

int main(int argc, char **argv){
    unsigned char key[32], nonce[12]={0,0,0,9,0,0,0,0x4a,0,0,0,0}, blk[64]; int i;
    for(i=0;i<32;i++) key[i]=(unsigned char)i;
    chacha_init(key,1,nonce); chacha_block(blk);
    (void)argv;
    if(argc>1){ for(i=0;i<64;i++) printf("%02x",blk[i]); printf("\n"); return 0; }  /* any argument: print the RFC 8439 2.3.2 block */
    pawl_init(&g,"bench",5,0); mt_seed(5489);
    bench("pawl", pawl_fill_);
    bench("chacha20", chacha_fill);
    bench("xoshiro256++", xoshiro_fill);
    bench("mt19937-64", mt_fill);
    return 0;
}
