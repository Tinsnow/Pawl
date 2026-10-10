"""Prints the same test vectors as test_vectors.c, from the Python reference."""
from pawl import Pawl, permute, permute_inverse

def words(ws):
    return " ".join("%016x" % w for w in ws)

for r in (1, 2, 3, 4, 24):
    print("permute zero r=%d: %s" % (r, words(permute([0] * 8, r))))
print("permute 0..7 r=24: %s" % words(permute(list(range(8)), 24)))

g = Pawl(b"")
print("seed empty, stream 0, 96 bytes: " + g.bytes(96).hex())

for i in range(1, 71):
    g = Pawl(bytes(range(i)))
    print("seed len %d: %s" % (i, g.bytes(16).hex()))

print("seed 'pawl', stream 1: " + Pawl(b"pawl", 1).bytes(48).hex())
print("seed 'pawl', stream 2^64-1: " + Pawl(b"pawl", 2**64 - 1).bytes(48).hex())

g = Pawl(42)
print("seed 42: 5 bytes: " + g.bytes(5).hex())
print("seed 42: then u64: %016x" % g.u64())
print("seed 42: then u32: %08x" % g.u32())
print("seed 42: then 77 bytes: " + g.bytes(77).hex())
print("seed 42: then u64: %016x" % g.u64())
for _ in range(5):
    print("seed 42: below(1000): %d" % g.below(1000))
print("seed 42: double: %.17g" % g.random())

g = Pawl(7)
g.bytes(40)
g.reseed(b"fresh entropy that is longer than one block, to cover two absorb blocks")
print("seed 7, 40 bytes, reseed, 48 bytes: " + g.bytes(48).hex())
g.reseed(b"")
print("then empty reseed, 32 bytes: " + g.bytes(32).hex())

g = Pawl(b"long")
g.bytes(1000000)
print("seed 'long', bytes 1000000..1000031: " + g.bytes(32).hex())

# not part of the C output: the permutation really is a bijection
import random, sys
rnd = random.Random(1)
for _ in range(200):
    x = [rnd.getrandbits(64) for _ in range(8)]
    assert permute_inverse(permute(x)) == x and permute(permute_inverse(x)) == x
print("inverse check: ok", file=sys.stderr)
