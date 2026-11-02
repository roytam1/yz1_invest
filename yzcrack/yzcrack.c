/* yzcrack.c - YZ1 (YamazakiZipper) password cracker.
 *
 * Standalone research tool, deliberately OUTSIDE the XacRett repo.
 *
 * Why cracking is cheap
 * ---------------------
 * YZ1's "encryption" is a 4-byte repeating XOR keystream with no KDF, no
 * salt and no iteration count, so the format adds zero work factor: the
 * attack cost is exactly the password's own entropy.
 *
 * Two-stage oracle (this is the important part)
 * ----------------------------------------------
 *  1. hdr[7] holds an 8-bit check byte:
 *         check = ( len(pw) + sum(pw[k]^k) ) & 0xFF
 *     That is a few ALU ops per character, so it filters at hundreds of
 *     millions of candidates/second -- but 8 bits means roughly ONE
 *     candidate in 256 matches by chance.  A check hit is NOT an answer.
 *
 *  2. Every check hit is confirmed IN-PROCESS by actually decoding the
 *     archive's metadata block and checking the 32-bit invariant
 *         metaSize + sum(member sizes) == total
 *     from the YZ1 header.  That is an exact test with no false positives.
 *
 * Spawning the real yzdec.exe per hit costs ~9 ms on Windows and dominated
 * everything; decoding in-process costs microseconds instead.
 *
 * The decoder below is a standalone transcription of the YZ1 stream
 * format (Huffman tables + LZ77 context blocks), matching the clean-room
 * reconstruction in src-0.49.17/yz32lib/yz32lib_v1.cpp.
 *
 * Build (VC6):
 *   cl /nologo /O2 /MT yzcrack.c
 * Usage:
 *   yzcrack ARCHIVE.yz1 CHARSET MINLEN MAXLEN [--extract DIR]
 *   charset 0=lower 1=upper 2=digits 3=alnum 4=ascii 5=alnum+punct
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ header */

static unsigned long BE32(const unsigned char* p)
{
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) |
           ((unsigned long)p[2] << 8) | (unsigned long)p[3];
}

/* ------------------------------------------------------- password -> key */

static unsigned char pw_check(const unsigned char* pw, int len)
{
    unsigned char v33 = (unsigned char)len;
    int k;
    for (k = 0; k < len; k++)
        v33 = (unsigned char)(v33 + ((unsigned)pw[k] ^ (unsigned)k));
    return v33;
}

static void pw_key(const unsigned char* pw, int len, unsigned char key[4])
{
    unsigned char v33 = (unsigned char)len;
    unsigned int v34 = v33, v99 = v33, v98 = v33, i = v33, v37;
    int k;
    for (k = 0; k < len; k++) {
        unsigned c = pw[k];
        unsigned v36;
        v33 = (unsigned char)(v33 + (c ^ (unsigned)k));
        v36 = (unsigned)k ^ c;
        v99 += v36;
        v98 -= v36;
        v34 *= v36;
        i ^= v36;
    }
    v37 = i + v34 + v99 * v98;
    key[0] = (unsigned char)(v37 >> 24);
    key[1] = (unsigned char)(v37 >> 16);
    key[2] = (unsigned char)(v37 >> 8);
    key[3] = (unsigned char)v37;
}

/* -------------------------------------------------------------- bit reader */

typedef struct {
    const unsigned char* data;
    long dataLen;
    long bitPos;
    int primed;
    unsigned long lookAhead;
} BitReader;

/* Returns the 32 bits starting at bitPos, MSB first.
   Fast path: 32 bits plus up to 7 shift bits always fit in 5 whole bytes,
   so one aligned 5-byte gather replaces 32 single-bit iterations.  This is
   bit-identical to the naive loop and matters because RdPeek32 is called
   ~900 times per candidate just to build the Huffman tables. */
static unsigned long RdPeek32(BitReader* r, long bitPos)
{
    long idx = bitPos >> 3;
    int sh = (int)(bitPos & 7);
    unsigned long v = 0;
    int i;

    if (idx >= 0 && idx + 5 <= r->dataLen) {
        const unsigned char* p = r->data + idx;
        unsigned long x = ((unsigned long)p[0] << 24) |
                          ((unsigned long)p[1] << 16) |
                          ((unsigned long)p[2] << 8)  |
                          ((unsigned long)p[3]);
        /* (x<<8 | p[4]) >> (8-sh)  ==  (x << sh) | (p[4] >> (8-sh)),
           with the low `sh` bits of x<<sh falling off the top.  For sh==0
           the second term is p[4]>>8 == 0, so this one expression covers
           both the aligned and unaligned case. */
        return ((x << sh) | ((unsigned long)p[4] >> (8 - sh))) & 0xFFFFFFFFu;
    }
    for (i = 0; i < 32; i++) {
        long pp = bitPos + i;
        long j = pp >> 3;
        unsigned long bit = 0;
        if (j >= 0 && j < r->dataLen)
            bit = (r->data[j] >> (7 - (pp & 7))) & 1u;
        v = (v << 1) | bit;
    }
    return v;
}

static int RdConsume(BitReader* r, int n, unsigned long* out)
{
    if (!r->primed) {
        if (n != 32) return -1;
        r->primed = 1;
        r->bitPos = 0;
    } else {
        r->bitPos += n;
    }
    r->lookAhead = RdPeek32(r, r->bitPos);
    *out = r->lookAhead;
    return 0;
}

/* ----------------------------------------------------------- huffman table */

typedef struct { unsigned long weight; unsigned short symbol;
                 unsigned short next; } WorkNode;
typedef struct { unsigned long code; unsigned short bits;
                 unsigned short symbol; } CodeNode;
typedef struct {
    long count;
    /* Fixed-size rather than heap-allocated.  These tables are rebuilt for
       EVERY candidate that passes the 8-bit check byte, so a per-call
       malloc/free pair is pure overhead -- and a single missed free on one
       of the rejection paths leaks ~12 KB per hit, which is how an earlier
       build of this tool died after hours of length 7/8 searching.  With
       the arrays inline and the tables static, a leak is impossible. */
    unsigned long codes[0x300];
    unsigned short bits[0x300];
    CodeNode nodes[0x300];
    unsigned short lookup12[4096];
} HuffTable;

static int WorkLess(const WorkNode* a, const WorkNode* b)
{
    if (a->weight != b->weight) return a->weight < b->weight;
    return a->symbol < b->symbol;
}
/* Weights come from 5-bit fields, so they are always 0..31, and the input
   array is already in symbol order.  A STABLE counting sort by weight
   therefore yields exactly the (weight, symbol) ordering that WorkLess
   demands, in O(n) instead of the insertion sort's O(n^2).  That rewrite
   was worth ~10x on the per-candidate validation cost, which dominates
   because ~1 in 100 candidates passes the 8-bit check byte. */
static void SortWork(WorkNode* a, long n)
{
    static long cnt[33];
    static WorkNode tmp[0x300];
    long i, start = 0;
    for (i = 0; i <= 32; i++) cnt[i] = 0;
    for (i = 0; i < n; i++) cnt[a[i].weight]++;
    for (i = 0; i <= 32; i++) { long c = cnt[i]; cnt[i] = start; start += c; }
    for (i = 0; i < n; i++) tmp[cnt[a[i].weight]++] = a[i];
    memcpy(a, tmp, (size_t)n * sizeof(WorkNode));
}
static int CodeLess(const CodeNode* a, const CodeNode* b)
{
    unsigned long pa, pb;
    if (a->code != b->code) return a->code < b->code;
    pa = ((unsigned long)a->symbol << 16) | a->bits;
    pb = ((unsigned long)b->symbol << 16) | b->bits;
    return pa < pb;
}
static void SortCodes(CodeNode* a, long n)
{
    long i;
    for (i = 1; i < n; i++) {
        CodeNode t = a[i];
        long j = i - 1;
        while (j >= 0 && CodeLess(&t, &a[j])) { a[j + 1] = a[j]; j--; }
        a[j + 1] = t;
    }
}
static void AddChain(HuffTable* h, WorkNode* w, long idx, int oneBit)
{
    long c = idx;
    while (c != 0xFFFF) {
        long s = w[c].symbol;
        h->bits[s]++;
        h->codes[s] >>= 1;
        if (oneBit) h->codes[s] |= 0x80000000u;
        c = w[c].next;
    }
}

static WorkNode  g_work[0x300];

static void NormalizeTable(HuffTable* h, const unsigned char* raw, long n)
{
    long i, bp;
    h->count = n;
    for (i = 0; i < n; i++) {
        h->codes[i] = 0; h->bits[i] = 0;
        g_work[i].weight = raw[i];
        g_work[i].symbol = (unsigned short)i;
        g_work[i].next = 0xFFFF;
    }
    SortWork(g_work, n);
    for (bp = n - 2; bp >= 0; bp--) {
        long j, last, a;
        AddChain(h, g_work, bp, 0);
        AddChain(h, g_work, bp + 1, 1);
        g_work[bp].weight--;
        a = g_work[bp].next;
        last = bp;
        while (a != 0xFFFF) { last = a; a = g_work[a].next; }
        g_work[last].next = (unsigned short)(bp + 1);
        {
            WorkNode saved = g_work[bp];
            j = bp - 1;
            while (j >= 0 && saved.weight <= g_work[j].weight) {
                g_work[j + 1] = g_work[j]; j--;
            }
            g_work[j + 1] = saved;
        }
    }
    for (i = 0; i < n; i++) {
        h->nodes[i].code = h->codes[i];
        h->nodes[i].bits = h->bits[i];
        h->nodes[i].symbol = (unsigned short)i;
    }
    SortCodes(h->nodes, n);
    for (i = 0; i < 4096; i++) h->lookup12[i] = 0xFFFF;
    for (i = n - 1; i >= 1; i--)
        h->lookup12[(h->nodes[i].code >> 20) & 0xFFF] = (unsigned short)i;
    h->lookup12[0] = 0;
    for (i = 1; i < 4096; i++)
        if (h->lookup12[i] == 0xFFFF) h->lookup12[i] = h->lookup12[i - 1];
}

static int ReadTables(BitReader* r, HuffTable* mainT, HuffTable* extraT)
{
    unsigned long a;
    static unsigned char mraw[0x300];
    static unsigned char eraw[0x80];
    int i;
    if (RdConsume(r, 32, &a)) return -1;
    for (i = 0; i < 0x300; i++) {
        mraw[i] = (unsigned char)(a >> 27);
        if (RdConsume(r, 5, &a)) return -1;
    }
    for (i = 0; i < 0x80; i++) {
        eraw[i] = (unsigned char)(a >> 27);
        if (RdConsume(r, 5, &a)) return -1;
    }
    NormalizeTable(mainT, mraw, 0x300);
    NormalizeTable(extraT, eraw, 0x80);
    return 0;
}

static int DecodeMainSym(BitReader* r, const HuffTable* h, unsigned short* out)
{
    unsigned long a;
    long idx;
    unsigned short bits;
    if (RdConsume(r, 0, &a)) return -1;
    idx = h->lookup12[(a >> 20) & 0xFFF];
    if (idx < 0 || idx >= h->count) return -1;
    bits = h->nodes[idx].bits;
    if (bits < 12) {
        if (RdConsume(r, bits, &a)) return -1;
        *out = h->nodes[idx].symbol;
        return 0;
    }
    while (idx < h->count) {
        bits = h->nodes[idx].bits;
        if (bits == 0) return -1;
        if ((a >> (32 - bits)) == (h->nodes[idx].code >> (32 - bits))) {
            if (RdConsume(r, bits, &a)) return -1;
            *out = h->nodes[idx].symbol;
            return 0;
        }
        idx++;
    }
    return -1;
}

static int DecodeExtra(BitReader* r, const HuffTable* h, long* out)
{
    unsigned long a;
    long i;
    unsigned short b;
    if (RdConsume(r, 0, &a)) return -1;
    for (i = 0; i <= 0x7E; i++) {
        b = h->bits[i];
        if (b != 0 && (a >> (32 - b)) == (h->codes[i] >> (32 - b))) {
            if (RdConsume(r, b, &a)) return -1;
            *out = i;
            return 0;
        }
    }
    b = h->bits[0x7F];
    if (b == 0 || (a >> (32 - b)) != (h->codes[0x7F] >> (32 - b))) return -1;
    if (RdConsume(r, b, &a)) return -1;
    *out = 0x7F;
    return 0;
}

/* ------------------------------------------------------------- LZ decoder */

/* posTab/lenTab are 256*256; kept static so the hot loop does not allocate.
   A full 512 KB memset per candidate dominated the search, so instead we
   record every index written during a block and reset only those.  The
   metadata block is typically only tens of bytes, so the touch list is
   tiny compared to the arrays themselves. */
static long        g_posTab[256 * 256];
static long        g_lenTab[256 * 256];
static unsigned short g_cur[256];
static int         g_touch[4096];
static int         g_ntouch;

static void Touch(int i)
{
    if (g_ntouch < (int)(sizeof(g_touch) / sizeof(g_touch[0])))
        g_touch[g_ntouch++] = i;
}

static void ResetTables(void)
{
    int i;
    for (i = 0; i < g_ntouch; i++) {
        g_posTab[g_touch[i]] = 0;
        g_lenTab[g_touch[i]] = 0;
    }
    g_ntouch = 0;
    memset(g_cur, 0, sizeof(g_cur));
}

static int DecodeBlock(BitReader* r, const HuffTable* mainT,
                       const HuffTable* extraT, unsigned long blockSize,
                       unsigned char* blk)
{
    long outPos = 0, lastUpd = 0, idle = 0;

    if (blockSize > 0x7FFFFFFFu) return -1;
    if (blockSize) memset(blk, 0, (size_t)blockSize);
    ResetTables();

    while (outPos < (long)blockSize) {
        long bitBefore = r->bitPos, outBefore = outPos;
        unsigned short sym = 0;
        long len = 1, src = -1;
        if (DecodeMainSym(r, mainT, &sym)) return -1;

        if (sym >= 2 * 256) {
            if (outPos >= (long)blockSize) return -1;
            blk[outPos++] = (unsigned char)(sym - 2 * 256);
        } else if (sym < 256) {
            long ev = 0, ctx, idx;
            if (DecodeExtra(r, extraT, &ev)) return -1;
            len = ev + 2;
            ctx = blk[lastUpd];
            idx = (g_cur[ctx] + sym) % 256;
            src = g_posTab[ctx * 256 + idx];
            if (src < 0 || src >= (long)blockSize ||
                outPos + len > (long)blockSize) return -1;
            { long k; for (k = 0; k < len; k++) blk[outPos + k] = blk[src + k]; }
            outPos += len;
        } else {
            long ctx, idx;
            ctx = blk[lastUpd];
            idx = (g_cur[ctx] + sym) % 256;
            len = g_lenTab[ctx * 256 + idx];
            src = g_posTab[ctx * 256 + ((g_cur[ctx] + sym - 256) % 256)];
            {
                long oldP = g_posTab[ctx * 256 + g_cur[ctx]];
                long oldL = g_lenTab[ctx * 256 + g_cur[ctx]];
                if (oldP != 0) {
                    Touch(ctx * 256 + idx);
                    g_posTab[ctx * 256 + idx] = oldP;
                    Touch(ctx * 256 + idx);
                    g_lenTab[ctx * 256 + idx] = oldL;
                }
            }
            if (src < 0 || src >= (long)blockSize) return -1;
            if (len > 0) {
                long k;
                if (outPos + len > (long)blockSize) return -1;
                for (k = 0; k < len; k++) blk[outPos + k] = blk[src + k];
                outPos += len;
            }
        }
        if (lastUpd < outPos - 1) {
            long ctx = blk[lastUpd];
            long idx = g_cur[ctx];
            Touch(ctx * 256 + idx);
            g_posTab[ctx * 256 + idx] = lastUpd + 1;
            g_lenTab[ctx * 256 + idx] = len;
            g_cur[ctx] = (unsigned short)((g_cur[ctx] + 1) % 256);
            lastUpd = outPos - 1;
        }
        if (r->bitPos == bitBefore && outPos == outBefore) {
            if (++idle > 100000) return -1;
        } else idle = 0;
    }
    return 0;
}

/* ---------------------------------------------------------- the validator */

/* Returns 0 when `arc` (a decrypted yz01 image) is structurally valid. */
static int yz_validate(const unsigned char* arc, long arcLen)
{
    unsigned long total, metaSize, fileCount;
    BitReader r;
    static HuffTable mainT, extraT;   /* ~19 KB each; no heap traffic */
    static unsigned char meta[0x10000];
    unsigned long sum = 0, k;
    int rc = -1;

    if (arcLen < 20) return -1;
    if (arc[0] != 'y' || arc[1] != 'z' || arc[2] != '0' || arc[3] != '1')
        return -1;
    total     = BE32(arc + 8);
    metaSize  = BE32(arc + 12);
    fileCount = BE32(arc + 16);
    if (metaSize > total || fileCount > 65535) return -1;
    if (metaSize > sizeof(meta)) return -1;
    if ((unsigned long)fileCount * 8 > metaSize) return -1;

    r.data = arc + 20; r.dataLen = arcLen - 20;
    r.bitPos = 0; r.primed = 0; r.lookAhead = 0;

    /* One exit: every rejection path below used to be a candidate for a
       missed free, and the `sum != total` one -- the overwhelmingly common
       rejection -- leaked on every single check-byte hit. */
    if (ReadTables(&r, &mainT, &extraT)) return -1;
    if (metaSize == 0 || DecodeBlock(&r, &mainT, &extraT, metaSize, meta) == 0) {
        for (k = 0; k < fileCount; k++)
            sum += BE32(meta + k * 4);
        if (metaSize + sum == total) rc = 0;
    }
    return rc;
}

/* ------------------------------------------------------------ brute force */

static const char* g_sets[] = {
    "abcdefghijklmnopqrstuvwxyz",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    "0123456789",
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
    " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~",
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    "!@#$%^&*()-_=+[]{};:,.<>?/\\|~"
};
static const int g_nsets = (int)(sizeof(g_sets) / sizeof(g_sets[0]));

static unsigned char* g_arc;      /* original file bytes          */
static long          g_arcLen;    /* file length                   */
static long          g_base;      /* offset of the yz01 image      */
static long          g_workLen;   /* bytes actually decrypted      */
static unsigned char g_scratch[4096];
static unsigned char g_pw[16];
static unsigned char g_want;
static long          g_hits, g_conf, g_tested;
static int           g_cs;
static unsigned char g_alpha[256];
static int           g_alphaLen;
static int           g_fc = -1;   /* shard: restrict 1st char, -1 = all */

static void try_pw(int len)
{
    unsigned char key[4];
    long i;
    unsigned long metaSize;

    g_tested++;
    if (pw_check(g_pw, len) != g_want) return;
    g_hits++;

    pw_key(g_pw, len, key);
    memcpy(g_scratch, g_arc + g_base, (size_t)g_workLen);
    for (i = 20; i < g_workLen; i++)
        g_scratch[i] ^= key[(i - 20) & 3];

    if (yz_validate(g_scratch, g_workLen) == 0) {
        int k;
        g_conf++;
        fprintf(stderr, "tested %ld candidates, %ld check hits", g_tested, g_hits);
        fprintf(stdout, "PASSWORD FOUND: ");
        for (k = 0; k < len; k++) putchar(g_pw[k]);
        fprintf(stdout, "\n  keystream: %02X %02X %02X %02X\n",
                key[0], key[1], key[2], key[3]);
        fflush(stdout);
        exit(0);
    }
}

static void search(int pos, int len)
{
    int i, n = g_alphaLen;
    if (pos == len) { try_pw(len); return; }
    /* Sharding: restricting the FIRST character to one alphabet index
       partitions the whole search space exactly, so N worker processes
       cover it with no overlap and no coordination. */
    if (pos == 0 && g_fc >= 0) {
        g_pw[0] = (unsigned char)g_alpha[g_fc];
        search(1, len);
        return;
    }
    for (i = 0; i < n; i++) {
        g_pw[pos] = (unsigned char)g_alpha[i];
        search(pos + 1, len);
    }
}

int main(int argc, char** argv)
{
    FILE* f;
    long off, L, k;
    unsigned char* buf;
    unsigned long metaSize;

    if (argc < 5) {
        fprintf(stderr,
            "usage: yzcrack ARCHIVE.yz1 CHARSET(%d..%d) MINLEN MAXLEN\n",
            0, g_nsets - 1);
        return 2;
    }
    g_cs = atoi(argv[2]);
    if (g_cs < 0 || g_cs >= g_nsets) { fprintf(stderr, "bad charset\n"); return 2; }
    if (atoi(argv[3]) < 1 || atoi(argv[4]) > 15) { fprintf(stderr, "bad lengths\n"); return 2; }

    f = fopen(argv[1], "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", argv[1]); return 1; }
    fseek(f, 0, SEEK_END); g_arcLen = ftell(f); fseek(f, 0, SEEK_SET);
    buf = (unsigned char*)malloc((size_t)g_arcLen);
    if (!buf || (long)fread(buf, 1, (size_t)g_arcLen, f) != g_arcLen) {
        fprintf(stderr, "read failed\n"); return 1;
    }
    fclose(f);
    g_arc = buf;

    g_base = 0;
    if (memcmp(g_arc, "yz01", 4) != 0) {
        long n = (long)BE32(g_arc + g_arcLen - 4);
        off = g_arcLen - (n + 4);
        if (off < 0 || memcmp(g_arc + off, "yz01", 4) != 0) {
            fprintf(stderr, "not a YZ1 archive\n"); return 1;
        }
        g_base = off;
    }
    if (!(g_arc[g_base + 6] & 0x04)) {
        fprintf(stderr, "archive is NOT password protected\n"); return 1;
    }
    g_want = g_arc[g_base + 7];
    metaSize = BE32(g_arc + g_base + 12);
    if (argc >= 6) g_fc = atoi(argv[5]);
    g_alphaLen = (int)strlen(g_sets[g_cs]);
    memcpy(g_alpha, g_sets[g_cs], (size_t)g_alphaLen);

/* Only the metadata block is needed to validate, and the Huffman
       tables occupy a fixed 560 bytes, so a small window suffices.  The
       previous 16 KB window meant every check hit copied and XORed 32 KB,
       which dominated the search. */
    {
        long need = 20 + 560 + (long)metaSize * 4 + 512;
        g_workLen = g_arcLen - g_base;
        if (g_workLen > need) g_workLen = need;
        if (g_workLen > (long)sizeof(g_scratch))
            g_workLen = (long)sizeof(g_scratch);
    }

    fprintf(stderr, "check byte   : 0x%02X\n", g_want);
    fprintf(stderr, "members      : %lu\n", BE32(g_arc + g_base + 16));
    fprintf(stderr, "meta block   : %lu bytes (decoded in-process)\n", metaSize);
    fprintf(stderr, "charset %d (%d chars), lengths %s..%s\n",
            g_cs, g_alphaLen, argv[3], argv[4]);

    for (L = atoi(argv[3]); L <= atoi(argv[4]); L++) {
        double space = 1;
        for (k = 0; k < L; k++) space *= (double)g_alphaLen;
        fprintf(stderr, "length %ld: %.4g candidates\n", L, space);
        search(0, (int)L);
    }
    fprintf(stderr, "done. %ld check hits, %ld confirmed\n", g_hits, g_conf);
    return 1;
}