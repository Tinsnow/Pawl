/* TestU01 driver:  tu01 small|crush|big  fwd|rev  [seed-text]
 *                  tu01 one N        fwd|rev  [seed-text]   run only BigCrush test N (1..106)
 * fwd feeds the generator's 32-bit outputs as they come; rev feeds them
 * bit-reversed (TestU01 is more sensitive to the high bits of each word).
 * "one" lets the 106 BigCrush tests be spread over several processes. */
#include "../pawl.h"
#include "unif01.h"
#include "bbattery.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pawl_t g;
static int rev = 0;

static unsigned int next32(void)
{
    uint32_t v = pawl_u32(&g);
    if (rev) {
        v = ((v >> 1) & 0x55555555u) | ((v & 0x55555555u) << 1);
        v = ((v >> 2) & 0x33333333u) | ((v & 0x33333333u) << 2);
        v = ((v >> 4) & 0x0F0F0F0Fu) | ((v & 0x0F0F0F0Fu) << 4);
        v = ((v >> 8) & 0x00FF00FFu) | ((v & 0x00FF00FFu) << 8);
        v = (v >> 16) | (v << 16);
    }
    return v;
}

int main(int argc, char **argv)
{
    const char *bat = argc > 1 ? argv[1] : "small";
    int one = !strcmp(bat, "one") ? atoi(argv[2]) : 0;
    const char *seed;
    char name[64];
    unif01_Gen *gen;
    if (one) { argv++; argc--; }
    seed = argc > 3 ? argv[3] : "testu01";
    rev = argc > 2 && !strcmp(argv[2], "rev");
    setvbuf(stdout, NULL, _IOLBF, 0);
    pawl_init(&g, seed, strlen(seed), 0);
    sprintf(name, "Pawl-%d %s", PAWL_ROUNDS, rev ? "bit-reversed" : "forward");
    gen = unif01_CreateExternGenBits(name, next32);
    if (one) {
        int rep[107] = {0};
        rep[one] = 1;
        bbattery_RepeatBigCrush(gen, rep);
    }
    else if (!strcmp(bat, "small")) bbattery_SmallCrush(gen);
    else if (!strcmp(bat, "crush")) bbattery_Crush(gen);
    else bbattery_BigCrush(gen);
    unif01_DeleteExternGenBits(gen);
    return 0;
}
