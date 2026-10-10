"""
Pawl version 1 -- readable Python reference.

Written straight from SPEC.md, independently of the C code, so that the two
can be checked against each other.  Slow; use it to understand the algorithm
or to verify another implementation, not to generate a lot of data.

EXPERIMENTAL: a new design with no independent cryptanalysis.  Do not use it
to protect real secrets.
"""

M = (1 << 64) - 1
PHI = 0x9E3779B97F4A7C15
R = (18, 50, 58, 37)
Q = (34, 30, 54, 4)
ROUNDS = 24

TAG_GEN = int.from_bytes(b"Pawl1gen", "little")
TAG_KEY = int.from_bytes(b"Pawl1key", "little")
TAG_MIX = int.from_bytes(b"Pawl1mix", "little")
TAG_END = int.from_bytes(b"Pawl1end", "little")


def rotl(x, r):
    return ((x << r) | (x >> (64 - r))) & M


def rotr(x, r):
    return ((x >> r) | (x << (64 - r))) & M


def permute(state, rounds=ROUNDS):
    """The 512-bit permutation on a list of eight 64-bit words."""
    a, b = list(state[:4]), list(state[4:])
    for r in range(rounds):
        a[0] ^= ((r + 1) * PHI) & M
        a = [(a[i] + b[i]) & M for i in range(4)]
        b = [rotl(b[i], R[i]) ^ a[(i + 1) % 4] for i in range(4)]
        a = [(rotl(a[i], Q[i]) + (b[(i + 1) % 4] & b[(i + 2) % 4])) & M for i in range(4)]
        a, b = b, a
    return a + b


def permute_inverse(state, rounds=ROUNDS):
    """Inverse permutation (not used by the generator; proves it is a bijection)."""
    a, b = list(state[:4]), list(state[4:])
    for r in reversed(range(rounds)):
        a, b = b, a
        a = [rotr((a[i] - (b[(i + 1) % 4] & b[(i + 2) % 4])) & M, Q[i]) for i in range(4)]
        b = [rotr(b[i] ^ a[(i + 1) % 4], R[i]) for i in range(4)]
        a = [(a[i] - b[i]) & M for i in range(4)]
        a[0] ^= ((r + 1) * PHI) & M
    return a + b


class Pawl:
    def __init__(self, seed=b"", stream=0, rounds=ROUNDS):
        if isinstance(seed, int):
            seed = seed.to_bytes(8, "little")
        self.rounds = rounds
        self.key = [0, 0, 0, 0]
        self.ctr = 0                      # 128-bit block counter
        self.stream = stream & M
        self.buf = b""                    # unread output of the last step
        self._absorb(TAG_KEY, bytes(seed))

    def _absorb(self, tag, data):
        s = self.key + [len(data) & M, 0, self.stream, tag]
        for off in range(0, len(data), 32):
            block = data[off:off + 32].ljust(32, b"\0")
            for i in range(4):
                s[i] ^= int.from_bytes(block[8 * i:8 * i + 8], "little")
            s = permute(s, self.rounds)
        s[7] ^= TAG_END
        s = permute(s, self.rounds)
        self.key = s[:4]

    def reseed(self, data):
        self._absorb(TAG_MIX, bytes(data))
        self.buf = b""

    def _step(self):
        s = permute(self.key + [self.ctr & M, self.ctr >> 64, self.stream, TAG_GEN], self.rounds)
        out = [(s[4 + i] + self.key[i]) & M for i in range(4)]
        self.key = [s[i] ^ self.key[i] for i in range(4)]
        self.ctr = (self.ctr + 1) & ((1 << 128) - 1)
        return b"".join(w.to_bytes(8, "little") for w in out)

    def bytes(self, n):
        while len(self.buf) < n:
            self.buf += self._step()
        out, self.buf = self.buf[:n], self.buf[n:]
        return out

    def u64(self):
        return int.from_bytes(self.bytes(8), "little")

    def u32(self):
        return int.from_bytes(self.bytes(4), "little")

    def random(self):
        """Float in [0, 1) with 53 random bits."""
        return (self.u64() >> 11) / 9007199254740992.0

    def below(self, n):
        """Unbiased integer in [0, n)."""
        if n <= 1:
            return 0
        mask = (1 << (n - 1).bit_length()) - 1
        while True:
            v = self.u64() & mask
            if v < n:
                return v


if __name__ == "__main__":
    import sys
    g = Pawl(sys.argv[1].encode() if len(sys.argv) > 1 else b"")
    print(g.bytes(64).hex())
