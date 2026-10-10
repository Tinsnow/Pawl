#define main tune_main
#include "tune2.c"
#undef main
// re-evaluate candidate constant sets (read from stdin as: R0 R1 R2 R3 Q0 Q1 Q2 Q3) at high precision
int main(int argc,char**argv){
  int N=atoi(argv[1]); P p; p.o2=1;p.o3=1;p.o4=2;
  while(scanf("%d %d %d %d %d %d %d %d",&p.R[0],&p.R[1],&p.R[2],&p.R[3],&p.Q[0],&p.Q[1],&p.Q[2],&p.Q[3])==8){
    double d[9]; sm_state=20261010; double s=score(&p,N,d);
    printf("%8.2f | ",s); pr(&p); printf(" | fwd1 %.1f %.1f %.2f | inv1 %.1f %.1f %.2f | fwd2 %.1f %.1f %.2f\n",d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7],d[8]); fflush(stdout);
  }
}
