"""Builds one table of all BigCrush p-values from the two workers' logs.
Tests 1-43 come from the sequential run (testu01_bigcrush_tests_001_043.txt), tests 44-106 from
one-test-per-process runs (bigcrush_tests/), which let the battery use both CPU cores."""
import re, glob
def pvals(block):
    out = []
    for line in block.split('\n'):
        if 'p-value' in line:
            m = re.search(r':\s*([0-9.eE+-]+|eps\d?)\s*(\*+)?\s*$', line)
            if m: out.append(m.group(1))
    return out
def name(block):
    m = re.findall(r'\n([a-z]+_[A-Za-z0-9]+) test:', '\n' + block)
    return m[-1] if m else '?'
rows = []
t = open('testu01_bigcrush_tests_001_043.txt').read()
ends = [m.end() for m in re.finditer(r'CPU time used[^\n]*\n', t)]
start = 0
for i, e in enumerate(ends[:43]):
    blk = t[start:e]; rows.append((i + 1, name(blk), pvals(blk), 'sequential run')); start = e
for f in sorted(glob.glob('bigcrush_tests/test_*.txt')):
    n = int(re.search(r'test_(\d+)', f).group(1)); s = open(f).read()
    body = s.split('========= Summary results')[0]
    rows.append((n, name(body), pvals(body), 'single-test run'))
rows.sort()
allp = []
print('BigCrush (TestU01 1.2.3) on Pawl-24, 32-bit outputs in generation order')
print('%4s  %-28s %-16s p-values' % ('test', 'function', 'source'))
for n, nm, pv, src in rows:
    flag = ''
    for p in pv:
        try:
            x = float(p); allp.append(x)
            if x < 0.001 or x > 0.999: flag = '   <-- outside [0.001, 0.999]'
        except ValueError:
            flag = '   <-- FAIL'
    print('%4d  %-28s %-16s %s%s' % (n, nm, src, ' '.join(pv), flag))
print()
print('tests: %d   p-values printed: %d   outside [0.001, 0.999]: %d   smallest %.4f   largest %.4f' %
      (len(rows), len(allp), sum(1 for x in allp if x < 0.001 or x > 0.999), min(allp), max(allp)))
