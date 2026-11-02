# YZ1 archive format

Format used by YAMAZAKI-Zip 1.05.2 and the reimplementation in this
repository. This document describes the **decoder-visible** format first,
then the encoder quirks needed for byte-identical output.

All integer fields are big-endian. Bit fields are read most-significant bit
first. There is no CRC, checksum, signature, or central directory.

## 1. Physical layout

A normal archive is:

```text
offset 0                   20                         end
+--------------------------+---------------------------+
| 20-byte header           | compressed bit stream     |
+--------------------------+---------------------------+
```

A self-extracting (SFX) file is:

```text
+----------------------+--------------------------+----------------+
| executable bytes     | YZ1 archive              | 4-byte length  |
+----------------------+--------------------------+----------------+
                        ^ archive starts here      ^ archive length
```

The last four bytes contain the archive length as a 32-bit big-endian
integer. The archive starts at `file_size - 4 - length`; the decoder checks
that the bytes there begin with `yz01`. The length covers the archive only,
not the appended length field.

The uncompressed content represented by an archive is:

```text
[ metadata table, Bt bytes ][ file 0 ][ file 1 ] ... [ file C-1 ]
```

`Atot` in the header is the total uncompressed length:

```text
Atot = Bt + sum(file_size[i])
```

The table is itself compressed as the first LZ stream. The file payload is
the second and subsequent LZ streams.

## 2. Header

The header is 20 bytes and is always plaintext, even in an encrypted archive.

| Offset | Size | Field | Meaning |
|---:|---:|---|---|
| 0 | 4 | magic | `yz01` (ASCII) |
| 4 | 1 | version major | `0x30` (`'0'`) in 1.05.2 |
| 5 | 1 | version minor | `0x35` (`'5'`) in 1.05.2 |
| 6 | 1 | flags | `0x30` (`'0'`) plain, `0x34` (`'4'`) encrypted |
| 7 | 1 | password check | `0x30` plain; otherwise `chk(password)` |
| 8 | 4 | `Atot` | total uncompressed size, table + payload |
| 12 | 4 | `Bt` | uncompressed metadata-table size |
| 16 | 4 | `Cnt` | number of files |

Notes:

- Writers emit `yz0105` in the first six bytes. Readers in this repository
  validate only the first four bytes (`yz01`), matching the original's
  four-byte comparison. The version bytes are informational.
- Encryption is indicated by bit `0x04` of the flags byte. Other bits are
  ignored. The 1.05.2 writer emits `0x34` for encrypted and `0x30` for plain.
- The password check byte is an 8-bit filter, not a MAC. See section 3.
- `Cnt == 0` is valid. The empty archive still contains the Huffman length
  tables (see section 6) and is normally 580 bytes total.

### Table consistency

A decoder should require:

```text
Bt <= Atot
Cnt * 8 <= Bt
sum(file_size[i]) <= Atot - Bt
```

The reference encoder writes exact equality for the last condition. A
decoder may tolerate extra uncompressed payload after the last file; it
stops after the declared files. The archive body may also have extra trailing
bytes after the bitstream; they are not needed.

## 3. Password handling and body encryption

Encrypted archives XOR the body bytes (all bytes from archive offset 20 to
the end of the archive) with a four-byte key repeated with period 4:

```text
body[i] ^= key[(i - 20) & 3]
```

The header, including the check byte, is not encrypted.

### 3.1 Key derivation

The password is a NUL-terminated byte string. Let `n = strlen(password)`.
The following is 32-bit unsigned arithmetic with wraparound:

```c
uint32_t v100 = n, v101 = n, v34 = n, inc = n;
for (uint32_t i = 0; i < n; i++) {
    int8_t   ch  = (int8_t)password[i];       /* signed char */
    uint32_t v36 = (uint32_t)((int32_t)ch ^ (int32_t)i);
    v34  += v36;
    v100 -= v36;
    v101 ^= v36;
    inc  *= v36;
}
uint32_t v37 = v101 + inc + v34 * v100;
key[0] = (uint8_t)(v37 >> 24);
key[1] = (uint8_t)(v37 >> 16);
key[2] = (uint8_t)(v37 >> 8);
key[3] = (uint8_t)(v37);
```

The signed-char cast matters for non-ASCII passwords.

### 3.2 Check byte

```c
uint32_t s = 0;
for (uint32_t i = 0; i < n; i++)
    s += (uint8_t)password[i] ^ (uint8_t)i;
uint8_t chk = (uint8_t)(s + n);
```

The plaintext header has `header[7] == 0x30`. An encrypted header must have
`header[7] == chk` for the supplied password. Because this is only eight
bits, it has many collisions; successful header validation is not proof that
the decoded data is correct.

### 3.3 Public token

The SDK and command-line tools also accept a public token of the form:

```text
pub-AABB-CCDD-EE
```

where `AA BB CC DD` are the four key bytes in hexadecimal and `EE` is the
check byte. For example:

```text
pub-0102-0304-05   -> key = 01 02 03 04, chk = 0x05
```

A public token bypasses the password derivation; the embedded check is
compared directly. This token is an interoperability convenience, not an
additional security mechanism.

### 3.4 Security warning

There is no salt, KDF, or iteration count. The same password always yields the
same key, and different passwords can yield the same key and check byte. This
is format-compatible obfuscation only.

## 4. Metadata table

The table is the uncompressed output of the first LZ stream and is exactly
`Bt` bytes long.

```text
+----------------------+----------------------+---------------------------+
| file sizes           | timestamps           | file names                |
| Cnt * 4 bytes (BE)   | Cnt * 4 bytes (BE)   | Cnt NUL-terminated names  |
+----------------------+----------------------+---------------------------+
```

Layout in detail:

```text
offset 0:                 file_size[0..Cnt-1]   4 bytes each, big-endian
offset 4*Cnt:             timestamp[0..Cnt-1]   4 bytes each, big-endian
offset 8*Cnt:             name[0], '\0',
                          name[1], '\0',
                          ...
```

- `file_size[i]` is the uncompressed size of member `i`.
- `timestamp[i]` is a 32-bit Unix `time_t` value (seconds since 1970), or
  zero for "do not restore a timestamp". It is not a DOS date/time pair.
- `name[i]` is a raw byte string with `\` path separators. No encoding
  conversion is performed. A leading separator is not used. The reference
  stores the top-level input component plus any relative subdirectories, for
  example `samples\sub\file.txt`.
- Names are stored in table order together with their sizes and timestamps.
- There are no directory entries. Empty directories cannot be represented and
  disappear when an archive is created.
- `Bt` must equal `8*Cnt + sum(strlen(name[i]) + 1)` for an exact table; a
  decoder should reject a name that runs past `Bt` and may warn if the names
  do not consume the whole table.

### Empty archive example

For `Cnt = 0`:

```text
Atot = 0
Bt   = 0
body = only the Huffman length tables (560 bytes)
total file size = 20 + 560 = 580 bytes
```

The 560 bytes are `(768 + 128) * 5 / 8 = 560`.

## 5. Bit stream and Huffman tables

The compressed stream begins at archive offset 20. It is a single MSB-first
bit stream:

```text
[ 768 five-bit lengths for alphabet A ]
[ 128 five-bit lengths for alphabet B ]
[ table-pass LZ tokens ]
[ payload-pass LZ tokens, one group per batch ]
```

There is **no byte or word alignment** between the length tables, the table
pass and the payload passes. Only the end of the body is rounded up to a whole
byte by the writer. The decoder stops according to the declared sizes and may
ignore the remaining padding bits.

### 5.1 Length fields

- Alphabet A has 768 symbols; alphabet B has 128 symbols.
- Each length is a 5-bit value in the range 0 through 31.
- Length 0 means the symbol is unused. The reference encoder normally assigns
  a code to every symbol, even one with zero frequency.
- The two length tables are read before any LZ token.

### 5.2 Code assignment

The format stores only code lengths. The code values are assigned with the
following reference algorithm. This is **not** the common "start at zero and
increment" canonical assignment; use the algorithm exactly.

```c
struct Node {
    uint32_t value;   /* code length for build_codes */
    uint16_t symbol;
    uint16_t next;    /* 0xffff = end of chain */
};

/* in_len[N] are the transmitted lengths.
   val[N] receives left-aligned 32-bit codes.
   code_len[N] receives the actual lengths. */
static void build_codes(const uint16_t in_len[N], int N,
                        uint32_t val[N], uint16_t code_len[N])
{
    Node node[N];
    for (int s = 0; s < N; s++) {
        node[s].value  = in_len[s];
        node[s].symbol = (uint16_t)s;
        node[s].next   = 0xffff;
        val[s] = 0;
        code_len[s] = 0;
    }

    /* Ascending by value; ties by (symbol | (next << 16)). */
    qsort(node, N, sizeof(Node), compare_value_symbol_next_ascending);

    for (int i = N - 2; i >= 0; i--) {
        /* First chain: increment length, shift code right. */
        int c = i;
        for (;;) {
            int s = node[c].symbol;
            code_len[s]++;
            val[s] >>= 1;
            if (node[c].next == 0xffff) break;
            c = node[c].next;
        }

        /* Second chain: increment length, shift in a high bit. */
        {
            int s = node[i + 1].symbol;
            code_len[s]++;
            val[s] = (val[s] >> 1) | 0x80000000u;
        }
        for (int c2 = node[i + 1].next; c2 != 0xffff; c2 = node[c2].next) {
            int s = node[c2].symbol;
            code_len[s]++;
            val[s] = (val[s] >> 1) | 0x80000000u;
        }

        /* Append the second chain below the first. */
        int tail = i;
        while (node[tail].next != 0xffff) tail = node[tail].next;
        node[tail].next = (uint16_t)(i + 1);

        /* Merge the values and reinsert node i, ascending. */
        node[i].value -= 1;
        uint32_t sv = node[i].value;
        uint32_t packed = (uint32_t)node[i].symbol |
                          ((uint32_t)node[i].next << 16);
        int at = i - 1;
        while (at >= 0 && node[at].value >= sv) {
            node[at + 1] = node[at];
            at--;
        }
        node[at + 1].value  = sv;
        node[at + 1].symbol = (uint16_t)(packed & 0xffff);
        node[at + 1].next   = (uint16_t)(packed >> 16);
    }
}
```

For a valid transmitted table, `code_len[s]` equals `in_len[s]`. A symbol of
length `L` is written/read using the top `L` bits of `val[s]`:

```text
code = val[s] >> (32 - L)
```

The bit writer emits those `L` bits MSB first. The decoder reads bits
MSB-first and tests the accumulated code against all codes of that length.

## 6. LZ token stream

Both the metadata table and the file payload use the same token alphabet and
the same Huffman tables. The token stream is not self-delimiting: the decoder
knows how many output bytes each pass must produce.

### 6.1 Alphabet A

| Symbol | Meaning |
|---:|---|
| `0 .. 255` | Match with a normal length code |
| `256 .. 511` | "Family" match; the length is inherited from `T2` |
| `512 .. 767` | Literal byte `symbol - 512` |

For a match, `delta = symbol & 0xff` selects a slot after adding the current
ring position `A[c]`.

### 6.2 Alphabet B

Alphabet B encodes match lengths:

| Code | Meaning |
|---:|---|
| `0 .. 126` | Match length `code + 2` |
| `127` | Escape: read a 5-bit exponent `e`, then `e+1` raw bits `x`; length = `x + 2` |

A normal match has length at least 2. The escape permits lengths beyond 128.
The writer uses `x = length - 2`, `e` = the index of the highest set bit of
`x` (0 through 31), and writes `e` as five bits followed by `x` in `e+1`
bits.

### 6.3 Decoder state

```text
uint8_t  out[capacity];          /* batch output buffer */
uint32_t pos;                    /* output position */
uint32_t h;                      /* last position processed by the ring */
uint16_t A[256];                 /* per-byte ring cursor */
uint16_t B[256];                 /* per-byte high-water mark */
uint32_t T1[256][256];           /* 1-based source positions; 0 = empty */
uint32_t T2[256][256];           /* remembered match lengths */
```

At the start of each independent stream:

```c
memset(A, 0, sizeof(A));
memset(B, 0, sizeof(B));
memset(T1, 0, sizeof(T1));
memset(T2, 0, sizeof(T2));
pos = 0;
h = 0;
```

### 6.4 Decoding one token

`read_A()` and `read_B()` read one Huffman symbol. `out` is the current
batch buffer.

```c
sym = read_A();

if (sym >= 512) {
    /* Literal */
    out[pos++] = (uint8_t)(sym - 512);
    ml = 1;
} else {
    if (h >= pos) error("no history byte");

    c    = out[h];
    a0   = A[c];
    slot = ((sym & 0xff) + a0) & 0xff;

    if (sym >= 256) {
        /* Family match: length and source come from the slot. */
        ml  = T2[c][slot];
        src = T1[c][slot];

        /* Reuse the current ring entry when it is populated. */
        if (T1[c][a0] != 0) {
            T1[c][slot] = T1[c][a0];
            T2[c][slot] = T2[c][a0];
        }
    } else {
        /* Normal match: read an explicit length. */
        lc = read_B();
        if (lc == 127) {
            e  = read_bits(5);
            x  = read_bits(e + 1);
            lc = x;
        }
        ml  = lc + 2;
        src = T1[c][slot];
    }

    if (ml == 0 || src >= pos)
        error("bad match");
    if (ml > cap - pos)
        error("match exceeds buffer");

    /* Copy forward; source and destination may overlap. */
    for (i = 0; i < ml; i++)
        out[pos + i] = out[src + i];
    pos += ml;
}

/* Ring update after every token, literal or match. */
if (pos - 1 > h) {
    c = out[h];
    a = A[c];

    T1[c][a] = h + 1;    /* 0 means empty, so store 1-based */
    T2[c][a] = ml;

    v = a + 1;
    if (B[c] < v) B[c] = v;
    A[c] = v % 256;
    h = pos - 1;
}
```

Important details:

- `T1` values are one-based positions; zero means "no entry".
- `T2` stores the length of the token that created the entry. For a literal,
  `ml` is 1.
- A match may read bytes that were written earlier in the same copy loop. This
  is ordinary overlapping LZ copying, so copy forward, not with `memmove`.
- The first token must be a literal, because a match requires `h < pos` and
  `pos` starts at 0.
- The family match does not read alphabet B at all.
- The ring update happens after the token and uses the total token length
  (`ml`), including literals.

### 6.5 Table pass

Decode exactly `Bt` bytes into a fresh buffer with a fresh LZ state. Those
bytes are the metadata table. Parse the file sizes, timestamps and names from
it before decoding any payload.

### 6.6 Payload batching (critical)

The original does not reset the LZ state at every file boundary. It uses the
largest file size as a working buffer and groups consecutive files into a
batch. A member may even be written by a match that began in an earlier
member of the same batch.

Let:

```text
C     = file count
s[i]  = uncompressed size of file i
M     = max(s[i]), or 0 when C == 0
```

The decoder groups files as follows:

```text
i = 0
while i < C:
    start = i
    batch_len = 0
    while i < C and batch_len + s[i] <= M:
        batch_len += s[i]
        i++
    reset LZ state
    decode batch_len bytes into a buffer of capacity M
    for each file j in [start, i):
        its data is the next s[j] bytes of that buffer
```

The same grouping is used by the encoder:

```text
append files to the current batch while batch_len + s[i] <= M;
when the next file does not fit, compress the current batch and reset.
```

Consequences:

- If `M` is large enough to hold several files, those files share one LZ
  state and cross-file matches are legal.
- A decoder must decode the **whole batch** before slicing it into files. It
  must not stop the token decoder at a member boundary; a match can overshoot
  a member boundary within the batch.
- The buffer capacity is `M`, not `Atot`, which is why extraction memory is
  proportional to the largest file rather than the whole archive.
- A batch can contain zero-length files. They add zero bytes and do not cause
  a reset.
- A file larger than `M` cannot occur when `M` is the maximum; a decoder
  should still validate this.
- For `C == 0`, `M == 0` and there is no payload.

Example with sizes `4, 6, 10` (`M = 10`):

```text
batch 1: files 0 and 1 (4 + 6 = 10); reset
batch 2: file 2 (10); reset
```

A match at offset 3 of batch 1 can cover bytes 3 through 9, crossing from
file 0 into file 1. This is valid.

Example with sizes `7, 14, 21, 28, 35` (`M = 35`):

```text
batch 1: 7 + 14 = 21                 (files 0, 1)
batch 2: 21                         (file 2; adding 28 would exceed 35)
batch 3: 28                         (file 3)
batch 4: 35                         (file 4)
```

### 6.7 Validation limits

A robust decoder should enforce at least:

- `Bt <= Atot` and `Cnt * 8 <= Bt`;
- table names are NUL-terminated within `Bt`;
- sum of file sizes does not exceed `Atot - Bt`;
- code lengths are at most 31;
- the bit reader never runs past the available body;
- `src < pos`, `ml != 0`, and `ml <= cap - pos` before every match;
- `h < pos` before reading a match's history byte;
- a batch output never exceeds the buffer capacity and produces exactly the
  requested batch length.

The reference tools use 32-bit sizes. This format description does not
define a 64-bit extension.

## 7. Encoder details for byte-identical output

The decoder-visible format above is sufficient to read archives. Reproducing
the reference encoder's exact bytes additionally requires the following
quirks.

### 7.1 Table bytes

The table is built as:

```text
file_size[0..C-1]  (4 bytes each, big-endian)
timestamp[0..C-1]  (4 bytes each, big-endian)
name[0] '\0' name[1] '\0' ...
```

No padding or alignment is inserted.

### 7.2 Huffman frequencies

Both passes share one pair of Huffman tables. Frequency counts are accumulated
over:

1. all tokens in the table pass; and
2. all tokens in every payload batch.

Then the reference writes code lengths to the stream. The code lengths are
produced by its deterministic Huffman merge, not by an arbitrary valid tree:

- Start with one node per symbol: `value = frequency`, `symbol = index`,
  `next = 0xffff`.
- Sort descending by `value`; ties use the packed key
  `(symbol | (next << 16))` descending.
- Merge from the end: merge node `i` with node `i+1`, increment the code
  length of every symbol in both chains, link the second chain after the
  first, add the values, and reinsert node `i` scanning left while the
  previous value is less than or equal to the new value.
- `build_codes` then assigns the actual code values as shown in section 5.2.
- If the maximum resulting length is at least 32, the encoder aborts with a
  code-length-overflow error.

This is why the reference's output is not reproducible by a generic Huffman
library even when the file contents match.

### 7.3 Match selection

The token search is a greedy hash-chain search over the batch. It uses the
previous output byte's value and the current byte to select a chain, searches
for the longest match, and emits one of:

- a literal;
- a normal match with an explicit B length; or
- a family match when the remembered length at the selected slot equals the
  newly found length.

The exact chain maintenance, tie handling and length-family decision are in
`src/yzenc.c` (`lz_pass` and the `chain_*` helpers). For byte-identical output,
use that implementation rather than only the decoder pseudocode above.

### 7.4 Batch boundaries

The encoder creates the same batches as section 6.6 using the largest file
size. It does not start a new LZ stream at every file boundary. This is the
main reason small/similar multi-file archives can differ in size if a
per-file encoder is used.

### 7.5 Empty files

An empty file contributes zero bytes to a batch and is still listed in the
table. A batch containing only empty files emits no payload tokens. A file
count of zero is valid and produces the 560-byte length-table-only body.

## 8. File naming and timestamps

The archive stores member names, not source paths. The reference behavior is:

- For a single file argument, store its last path component.
- For a directory argument, store the directory's last component followed by
  each relative subpath, separated by backslashes.
- Replace the output archive's filename extension with `.yz1`.
- Preserve raw name bytes; no code-page conversion is performed.
- Use a Japanese DBCS lead-byte heuristic when scanning for `\` in some
  command-line paths: a byte whose high nibble is `0x8`, `0x9` or `0xE` is
  treated as the first byte of a two-byte character.
- Restore the 32-bit timestamp with `_utime` when it is nonzero. A zero
  timestamp means "do not set a timestamp".

The format has no explicit directory entries, so empty directories are not
preserved.

## 9. Reference implementation map

| Concern | Reference source |
|---|---|
| Header read/write | `src/yzenc.c`, `src/yzdec.c`, `yz32lib/yz1codec.cpp` |
| Password and public token | `derive_key`, `pw_chk`, `parse_pub*` |
| Huffman length generation | `huff_lens` in `src/yzenc.c` |
| Code assignment | `build_codes` |
| LZ token encode | `lz_pass` in `src/yzenc.c` |
| LZ token decode | `lz_decompress` in `src/yzdec.c` and `yz32lib/yz1codec.cpp` |
| Streaming decode | `decode_one` and `yzFileDecode::DecodeFile` |
| SFX trailer | `has_appended_archive` / `yzFileEncode` |

For a readable implementation, see `src/yzdec.c` for the CLI decoder and
`yz32lib/yz1codec.cpp` for the reusable codec.
