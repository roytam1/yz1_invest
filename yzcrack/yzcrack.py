#!/usr/bin/env python3
"""
yzcrack - YZ1 (YamazakiZipper) archive password cracker.

Standalone research tool. NOT part of the XacRett project and deliberately
kept out of that repository.

Why this is even feasible
-------------------------
The YZ1 "encryption" is a 4-byte repeating XOR keystream.  The only thing
standing between an attacker and the plaintext is the password itself --
the format adds NO work factor, no KDF stretching, no salt, no iteration
count.  So the attack cost is exactly the password's own entropy, and a
dictionary attack is instant.

How a candidate is checked
--------------------------
hdr[7] holds an 8-bit check byte:

    check = ( len(pw) + sum( pw[k] ^ k for k in range(len(pw)) ) ) & 0xFF

That is O(len) of a couple of ALU ops, so candidates are validated in
microseconds with no decompression at all.

THE CAVEAT THAT MATTERS: 8 bits means that roughly 1 candidate in 256
matches by pure chance.  A check-byte hit is therefore NOT proof.  Every
hit is confirmed with a real test-extract before it is reported.

Usage
-----
  yzcrack.py ARCHIVE.yz1 --dict words.txt
  yzcrack.py ARCHIVE.yz1 --brute 1 6
  yzcrack.py ARCHIVE.yz1 --info
  yzcrack.py ARCHIVE.yz1 --dict w.txt --brute 1 4 --charset lower
"""

import argparse
import os
import re
import shutil
import string
import subprocess
import sys
import tempfile
import time

# ---------------------------------------------------------------- derivation

def check_byte(pw_bytes):
    """The YZ1 header check byte for a candidate password."""
    v = len(pw_bytes) & 0xFF
    for k, c in enumerate(pw_bytes):
        v = (v + (c ^ k)) & 0xFF
    return v


def key_from_password(pw_bytes):
    """The 4-byte repeating keystream (for decryption / confirmation)."""
    v33 = len(pw_bytes) & 0xFF
    v34 = v99 = v98 = i = v33
    for k, cu in enumerate(pw_bytes):
        c = cu
        v33 = (v33 + (c ^ k)) & 0xFF
        v36 = k ^ c
        v99 = (v99 + v36) & 0xFFFFFFFF
        v98 = (v98 - v36) & 0xFFFFFFFF
        v34 = (v34 * v36) & 0xFFFFFFFF
        i = (i ^ v36) & 0xFFFFFFFF
    v37 = (i + v34 + v99 * v98) & 0xFFFFFFFF
    return bytes([(v37 >> 24) & 0xFF, (v37 >> 16) & 0xFF,
                  (v37 >> 8) & 0xFF, v37 & 0xFF])


# ------------------------------------------------------------------- archive

class Yz1(object):
    def __init__(self, path):
        self.path = path
        with open(path, 'rb') as f:
            self.data = f.read()
        self.base = 0
        if self.data[:4] != b'yz01':
            # SFX: the image lives near EOF behind a 4-byte size trailer
            n = int.from_bytes(self.data[-4:], 'big')
            off = len(self.data) - (n + 4)
            if off >= 0 and self.data[off:off + 4] == b'yz01':
                self.base = off
        self.header = self.data[self.base:self.base + 20]
        self.version = self.header[4:6]
        self.flag = self.header[6]
        self.want = self.header[7]
        self.encrypted = bool(self.flag & 0x04)
        self.count = int.from_bytes(self.header[16:20], 'big')

    def info(self):
        return {
            'path': self.path,
            'size': len(self.data),
            'sfx': self.base != 0,
            'version': self.version.decode('latin-1'),
            'flag': self.flag,
            'encrypted': self.encrypted,
            'check': self.want,
            'members': self.count,
        }


# --------------------------------------------------------------- confirmor

TESTFILE_RE = re.compile(r'TestFile\((\d+)/(\d+)\):\s*(.*?)\s*\(([-0-9,]+) byte\)')


class Confirmer(object):
    """
    Turns a check-byte hit into a real answer by running the official
    yzdec.exe in test mode.

    yzdec exits 0 even for a WRONG password (it happily prints a garbage
    name and a negative size), so the exit code is useless.  We parse the
    TestFile lines instead and require every one to have a non-empty name
    and a positive size.
    """

    def __init__(self, yzdec, timeout=60):
        self.yzdec = yzdec
        self.timeout = timeout
        self.calls = 0

    def available(self):
        return bool(self.yzdec) and os.path.isfile(self.yzdec)

    def confirm(self, archive, pw):
        if not self.available():
            return None                     # unknown
        self.calls += 1
        args = [self.yzdec, '-t', '-p' + pw.decode('latin-1'), archive]
        try:
            r = subprocess.run(args, capture_output=True, text=True,
                               timeout=self.timeout)
        except Exception:
            return False
        out = r.stdout or ''
        hits = TESTFILE_RE.findall(out)
        if not hits:
            return False                    # nothing parsed -> treat as bad
        for _i, _n, name, size in hits:
            if not name.strip():
                return False
            try:
                if int(size.replace(',', '')) <= 0:
                    return False
            except ValueError:
                return False
        return True


# ---------------------------------------------------------------- candidates

CHARSETS = {
    'lower': string.ascii_lowercase,
    'upper': string.ascii_uppercase,
    'digits': string.digits,
    'ascii': ''.join(chr(c) for c in range(0x20, 0x7F)),
    'alnum': string.ascii_letters + string.digits,
    'alnum8': string.ascii_letters + string.digits + "!@#$%^&*()-_=+[]{};:,.<>?/",
    'anybyte': ''.join(chr(c) for c in range(0x20, 0x100)),
}


def expand_word(w, variants):
    """Yield a word plus optional simple mutations."""
    yield w
    if variants:
        yield w.lower()
        yield w.upper()
        yield w.capitalize()
        for d in '0123456789':
            yield w + d
            for c in CHARSETS['lower']:
                yield c + w
        for a in ('!', '@', '#', '$', '123', '1234'):
            yield w + a


def brute(alphabet, lo, hi, prefix=b'', progress=None):
    """Yield every byte string of length lo..hi over `alphabet`."""
    n = len(alphabet)
    for length in range(lo, hi + 1):
        idx = [0] * length
        while True:
            yield prefix + bytes(alphabet[i] for i in idx)
            p = length - 1
            while p >= 0:
                idx[p] += 1
                if idx[p] < n:
                    break
                idx[p] = 0
                p -= 1
            if p < 0:
                break
            if progress:
                progress(length)


# --------------------------------------------------------------------- main

def main():
    ap = argparse.ArgumentParser(
        description='YZ1 archive password cracker (standalone research tool)')
    ap.add_argument('archive')
    ap.add_argument('--info', action='store_true', help='print header info')
    ap.add_argument('--dict', metavar='FILE', help='dictionary / wordlist')
    ap.add_argument('--brute', nargs=2, type=int, metavar=('LO', 'HI'),
                    help='exhaustive search over length LO..HI')
    ap.add_argument('--charset', default='ascii', choices=sorted(CHARSETS))
    ap.add_argument('--no-variants', action='store_true',
                    help='do not try case/digit mutations of dictionary words')
    ap.add_argument('--yzdec', default=None,
                    help='path to yzdec.exe (for confirming hits)')
    ap.add_argument('--max-hits', type=int, default=20,
                    help='stop after this many confirmed-looking hits')
    ap.add_argument('--quiet', action='store_true')
    args = ap.parse_args()

    arc = Yz1(args.archive)
    if args.info or (not args.dict and not args.brute):
        i = arc.info()
        for k in ('size', 'sfx', 'version', 'flag', 'encrypted', 'check',
                  'members'):
            print('%-10s %s' % (k, i[k]))
        if not arc.encrypted:
            print('\nThis archive is NOT password protected.')
            return 0
        if not args.dict and not args.brute:
            print('\nTarget check byte: 0x%02X  (8 bits -> expect ~1 false '
                  'positive per 256 candidates)' % arc.want)
            print('Every hit is confirmed with a real test-extract.')
        return 0

    if not arc.encrypted:
        print('Not password protected; nothing to crack.')
        return 0

    yzdec = args.yzdec
    if yzdec is None:
        guess = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                             'yzdec.exe')
        yzdec = guess if os.path.isfile(guess) else None
    conf = Confirmer(yzdec)
    if not conf.available():
        print('WARNING: yzdec.exe not found - check-byte hits cannot be '
              'confirmed and ~1 in 256 will be FALSE POSITIVES.', file=sys.stderr)

    def report(cand, confirmed):
        tag = 'CONFIRMED' if confirmed else ('unverified' if confirmed is None
                                              else 'REJECTED')
        print('\n*** %s: %s  (check 0x%02X)' %
              (tag, ascii(cand), check_byte(cand)))
        print('    keystream: %s' % key_from_password(cand).hex(' '))

    t0 = time.time()
    tested = 0
    hits = 0

    def try_one(cand):
        nonlocal tested, hits
        tested += 1
        if check_byte(cand) != arc.want:
            return False
        hits += 1
        ok = conf.confirm(args.archive, cand)
        if ok or (ok is None and hits <= args.max_hits):
            report(cand, ok)
            return True
        return False

    # ---- dictionary
    if args.dict:
        words = []
        with open(args.dict, 'rb') as f:
            for line in f:
                line = line.strip()
                if line:
                    words.append(line)
        total = 0
        for w in words:
            for cand in expand_word(w.decode('latin-1'),
                                    not args.no_variants):
                total += 1
                if try_one(cand.encode('latin-1')):
                    break
        dt = time.time() - t0
        print('dictionary: %d candidates in %.1fs (%.0f/s), %d check hits'
              % (total, dt, total / dt if dt else 0, hits))
        if hits:
            return 0

    # ---- brute force
    if args.brute:
        alpha = CHARSETS[args.charset].encode('latin-1')
        lo, hi = args.brute
        space = sum(len(alpha) ** L for L in range(lo, hi + 1))
        print('brute force: charset %s (%d chars), lengths %d..%d'
              % (args.charset, len(alpha), lo, hi))
        print('            search space ~ %.3g candidates' % space)
        if space > 1e10:
            print('            WARNING: this is a large search; expect hours '
                  'to days.', file=sys.stderr)
        t0 = time.time()
        for cand in brute(alpha, lo, hi):
            if try_one(cand):
                break
            if not args.quiet and tested % 2000000 == 0:
                dt = time.time() - t0
                print('  ... %d tested (%.0f/s)' % (tested, tested / dt))
        dt = time.time() - t0
        print('brute force: %d candidates in %.1fs (%.0f/s), %d check hits'
              % (tested, dt, tested / dt if dt else 0, hits))
        if not conf.available():
            print('\n(no yzdec.exe: any hit above is a CHECK-BYTE MATCH ONLY '
                  '- 1 in 256 is expected to be wrong)')
    return 0


if __name__ == '__main__':
    sys.exit(main())