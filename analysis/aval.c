// Full 512x512 avalanche matrix for r rounds (forward or inverse).
// For every single-bit input flip, estimate the probability that each output bit flips.
// Reports: entries that never/always flip, max |bias|, and a chi-square over all entries
// (ideal: each entry ~ Binomial(N,1/2)  =>  sum of z^2 ~ chi2 with 262144 dof).
#include "perm.h"
static uint32_t cnt[512][512];
int main(int argc,char**argv){
  int rmin=atoi(argv[1]), rmax=atoi(argv[2]); long N=atol(argv[3]); int inv=argc>4?atoi(argv[4]):0;
  if(argc>5) USE_RC=atoi(argv[5]);
  for(int r=rmin;r<=rmax;r++){
    memset(cnt,0,sizeof cnt); xseed(1000+r);
    for(long n=0;n<N;n++){
      uint64_t x[8],y[8]; for(int i=0;i<8;i++) x[i]=xnext();
      memcpy(y,x,64); if(inv) perm_i(y,r); else perm_f(y,r);
      for(int bit=0;bit<512;bit++){
        uint64_t v[8]; memcpy(v,x,64); v[bit>>6]^=1ULL<<(bit&63);
        if(inv) perm_i(v,r); else perm_f(v,r);
        uint32_t *c=cnt[bit];
        for(int w=0;w<8;w++){ uint64_t d=v[w]^y[w]; while(d){ int j=__builtin_ctzll(d); c[w*64+j]++; d&=d-1; } }
      }
    }
    long zeros=0, ones=0; double maxb=0, chi=0; int mi=0,mj=0;
    for(int i=0;i<512;i++) for(int j=0;j<512;j++){
      double p=(double)cnt[i][j]/N; if(cnt[i][j]==0) zeros++; if(cnt[i][j]==N) ones++;
      double b=fabs(p-0.5); if(b>maxb){maxb=b;mi=i;mj=j;}
      double z=(cnt[i][j]-N/2.0)/sqrt(N/4.0); chi+=z*z;
    }
    double dof=262144.0, zchi=(chi-dof)/sqrt(2*dof);
    // expected max |bias| for ideal: ~ sqrt(2 ln(2*262144)) * 0.5/sqrt(N)
    double expmax=sqrt(2*log(2*262144.0))*0.5/sqrt((double)N);
    printf("%s r=%2d N=%ld  never=%6ld always=%4ld  max|bias|=%.5f (ideal~%.5f) at in=%d out=%d  chi2 z=%+.1f\n", inv?"INV":"FWD", r, N, zeros, ones, maxb, expmax, mi, mj, zchi);
    fflush(stdout);
  }
}
