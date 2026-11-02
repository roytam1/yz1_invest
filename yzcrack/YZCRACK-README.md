# YZ1 (YamazakiZipper) password cracker

Standalone research tools. **Not part of the XacRett project** — they live in
the workarea root as untracked files and are deliberately kept out of
`src-0.49.17/`.

## Why cracking is cheap

YZ1's "encryption" is a 4-byte repeating XOR keystream with **no KDF, no salt
and no iteration count**. The format adds zero work factor, so the attack cost
is exactly the password's own entropy.

## The two-stage oracle

1. **`hdr[7]`, the 8-bit check byte**

   ```
   check = ( len(pw) + sum( pw[k] ^ k for k in range(len(pw)) ) ) & 0xFF
   ```

   A few ALU ops per character, so it filters millions of candidates/second.
   But 8 bits means roughly **one candidate in 256 matches by chance** — a check
   hit is *not* an answer.

2. **In-process confirmation**

   Every check hit is confirmed by actually decoding the archive's metadata
   block and testing the exact 32-bit invariant from the YZ1 header:

   ```
   metaSize + sum(member sizes) == total
   ```

   That has no false positives. Spawning the real `yzdec.exe` per hit costs
   ~9 ms on Windows and dominated everything; decoding in-process is
   microseconds instead (measured: 1742 s → 72 s for 916 M candidates, before
   adding cores).

## Files

| file | what it is |
|---|---|
| `yzcrack.c` / `yzcrack.exe` | the real cracker — in-process validation, no external tools |
| `shard.py` | fans the search across cores (shards on the first character) |
| `yzcrack.py` | readable Python reference; confirms hits by shelling out to `yzdec.exe` |
| `yzbf.c` / `yzbf.exe` | early check-byte-only scanner (kept for comparison) |
| `test_confirm.py` | self-test for the confirmer |
| `build_yzcrack.bat` / `build_yzbf.bat` | MSVC 6.0 builds |
| `words.txt` | sample wordlist |

## Build

```bat
build_yzcrack.bat      :: needs C:\Program Files (x86)\Microsoft Visual Studio\VC98
```

## Usage

Crack a single archive, one process:

```bat
yzcrack.exe lib.yz1 3 1 6
```

Crack across all cores (recommended):

```bat
python shard.py lib.yz1 3 1 6 16
```

Arguments: `ARCHIVE CHARSET MINLEN MAXLEN [FIRSTCHAR]`

| charset | contents |
|---|---|
| 0 | `a-z` |
| 1 | `A-Z` |
| 2 | `0-9` |
| 3 | alphanumeric (62) |
| 4 | printable ASCII (95) |
| 5 | alphanumeric + punctuation |

`FIRSTCHAR` shards the search on one alphabet index — that partitions the
space exactly, so N workers cover it with no overlap. `shard.py` does this
for you.

Output:

```
PASSWORD FOUND: gfyzi
  keystream: 09 15 F4 B5
```

## Important: the derivation is not injective

Different passwords can produce the **same check byte and the same
keystream**. Verified against the official unpacker: `yzdec -t -pgfyzi
yzenc.yz1` really does extract `yzenc.txt` correctly, even though the real
password was `yzenc`. So a "found" password is any input that derives the
right key — it need not be the original.

## Fixed: per-candidate memory leak

An earlier build leaked ~12.5 KB on **every** candidate that passed the 8-bit
check byte. The Huffman tables were `malloc`ed per candidate, and the
rejection path

```c
if (metaSize + sum != total) return -1;   /* returned without freeing */
```

skipped the `free`s — and that is the overwhelmingly common rejection, i.e.
practically every check-byte hit. Measured growth was ~890 KB/s, so a long
length 7/8 search accumulated on the order of 16 GB and died.

Measured before/after over the same 40 s of `lib.yz1`, charset 3:

| build | working set after 40 s |
|---|---|
| leaky | 4.6 MB → **39.6 MB** (rising throughout) |
| fixed | **3.2 MB → 3.2 MB** (flat) |

The fix removes the heap from the hot path entirely: `HuffTable` now holds
fixed-size arrays and the tables are `static`, so there is nothing to leak and
no allocator traffic. A leak on that path is now structurally impossible rather
than merely absent. It is also marginally faster (11.39 s vs 11.72 s on the
same search).

## Self-test

```bat
python test_confirm.py
```

Confirms the oracle returns True for a known password and False for wrong
ones, including a decoy that passes the 8-bit check byte.