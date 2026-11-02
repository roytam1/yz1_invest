/* yzbf - fast check-byte search for YZ1 password archives.
 *
 * The YZ1 check byte is
 *     check = ( len(pw) + sum( pw[k] ^ k ) ) & 0xFF
 * which is a handful of ALU ops per character, so this runs at a few
 * hundred million candidates/second and the limiting factor becomes the
 * search space rather than the cipher.
 *
 * Only candidates whose check byte matches are printed; roughly 1 in 256
 * of them is a false positive, so the driver MUST confirm each hit with a
 * real test-extract (see yzcrack.py's Confirmer).
 *
 * Build (VC6):
 *   cl /nologo /O2 /MT yzbf.c
 * Usage:
 *   yzbf ARCHIVE.yz1 CHARSET MINLEN MAXLEN
 *   charset: 0=lower 1=upper 2=digits 3=alnum 4=ascii 5=alnum+sym
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char want;      /* target check byte */

static int BE32(const unsigned char* p)
{
    return ((int)p[0] << 24) | ((int)p[1] << 16) | ((int)p[2] << 8) | p[3];
}

/* Returns the archive offset of the yz01 image (0 for a plain archive). */
static long find_image(const unsigned char* d, long len)
{
    if (len >= 4 && !memcmp(d, "yz01", 4)) return 0;
    if (len < 24) return -1;
    {
        long n = BE32(d + len - 4);
        long off = len - (n + 4);
        if (off >= 0 && off + 4 <= len && !memcmp(d + off, "yz01", 4))
            return off;
    }
    return -1;
}

static unsigned char g_alpha[256];
static int g_nalpha;

static unsigned char g_pw[16];

static void emit(int len)
{
    int k;
    for (k = 0; k < len; k++) putchar(g_pw[k]);
    putchar('\n');
    fflush(stdout);
}

/* Recursive descent over lengths; `pos` fills the prefix. */
static void search(int pos, int len, unsigned acc)
{
    int i, v;
    unsigned char saved[16];

    memcpy(saved, g_pw + pos, (size_t)(len - pos));

    if (pos == len) {
        unsigned final = (acc + (unsigned)len) & 0xFFu;
        if (final == want) emit(len);
        return;
    }

    for (i = 0; i < g_nalpha; i++) {
        unsigned c = g_alpha[i];
        unsigned contrib = (c ^ (unsigned)pos) & 0xFFu;
        g_pw[pos] = (unsigned char)c;
        search(pos + 1, len, (acc + contrib) & 0xFFu);
    }

    memcpy(g_pw + pos, saved, (size_t)(len - pos));
}

int main(int argc, char** argv)
{
    static unsigned char buf[64 * 1024 * 1024];
    static const char* sets[] = {
        "abcdefghijklmnopqrstuvwxyz",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        "0123456789",
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
        " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~",
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
        "!@#$%^&*()-_=+[]{};:,.<>?/\\|~"
    };
    static const int nsets = (int)(sizeof(sets) / sizeof(sets[0]));
    FILE* f;
    long len;
    long off;
    int cs, lo, hi, L;

    if (argc < 5) {
        fprintf(stderr, "usage: yzbf ARCHIVE CHARSET(%d..%d) MINLEN MAXLEN\n",
                0, nsets - 1);
        return 2;
    }

    cs = atoi(argv[2]);
    lo = atoi(argv[3]);
    hi = atoi(argv[4]);
    if (cs < 0 || cs >= nsets) { fprintf(stderr, "bad charset\n"); return 2; }
    if (lo < 1 || hi > 15 || lo > hi) { fprintf(stderr, "bad lengths\n"); return 2; }

    f = fopen(argv[1], "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", argv[1]); return 1; }
    len = (long)fread(buf, 1, sizeof(buf), f);
    fclose(f);

    off = find_image(buf, len);
    if (off < 0) { fprintf(stderr, "not a YZ1 archive\n"); return 1; }

    if (!(buf[off + 6] & 0x04)) {
        fprintf(stderr, "archive is NOT password protected\n");
        return 1;
    }
    want = buf[off + 7];
    fprintf(stderr, "check byte = 0x%02X, members = %d\n", want, BE32(buf + off + 16));

    g_nalpha = (int)strlen(sets[cs]);
    memcpy(g_alpha, sets[cs], (size_t)g_nalpha);

    for (L = lo; L <= hi; L++) {
        long space = 1, k;
        for (k = 0; k < L; k++) space *= g_nalpha;
        fprintf(stderr, "length %d: %.4g candidates\n", L, (double)space);
        search(0, L, 0);
    }
    return 0;
}