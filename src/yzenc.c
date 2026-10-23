/* yzenc.c - YZ1 v1.05.2 packer (plain mode), shaped after yzenc.exe sub_4014D0/sub_4021C0.
 * Default: all inputs go into one archive. -m: one archive per input (original behavior).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <io.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

#define ALPHA_A 768
#define ALPHA_B 128
#define RING    256

static void die(const char *m) { fprintf(stderr, "yzenc: %s\n", m); exit(1); }

/* ---------------- password key derivation (sub_4015EA) ---------------- */
static void derive_key(const char *pw, u8 key[4]) {
    const u8 *b = (const u8 *)pw; u32 n = (u32)strlen(pw);
    u32 v100 = n, v101 = n, v34 = n, inc = n;
    u32 idx;
    for (idx = 0; idx < n; idx++) {
        u32 ch = (u32)(int)(signed char)b[idx], v36;
        v36 = (ch ^ idx);
        v34 = (v34 + v36);
        v100 = (v100 - v36);
        v101 ^= v36;
        inc = (inc * v36);
    }
    { u32 v37 = v101 + inc + v34 * v100;
      key[0] = (u8)(v37 >> 24); key[1] = (u8)(v37 >> 16);
      key[2] = (u8)(v37 >> 8);  key[3] = (u8)(v37 & 0xFF); }
}

/* header chk byte: (sum of (pw[i] ^ i) + len) & 0xFF */
static u8 pw_chk(const char *pw) {
    const u8 *b = (const u8 *)pw;
    u32 n = (u32)strlen(pw), i, s = 0;
    for (i = 0; i < n; i++) s += (u32)(b[i] ^ (u8)i);
    return (u8)((s + n) & 0xFF);
}

/* "pub-xxxx-xxxx-xxxx" carries the 4 key bytes plus the chk byte */
static int parse_pub(const char *pw, u8 key[4], u8 *chk) {
    unsigned int b0, b1, b2, b3, b4;
    if (sscanf(pw, "pub-%02x%02x-%02x%02x-%02x", &b0, &b1, &b2, &b3, &b4) != 5) return 0;
    key[0] = (u8)b0; key[1] = (u8)b1; key[2] = (u8)b2; key[3] = (u8)b3;
    *chk = (u8)b4;
    return 1;
}

static int read_line(const char *prompt, char *buf, size_t n) {
    size_t l;
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, n, stdin)) return 0;
    l = strlen(buf);
    while (l && (buf[l - 1] == '\n' || buf[l - 1] == '\r')) buf[--l] = 0;
    return 1;
}

static const char *fmt_num(u32 v) {
    static char out[16];
    char tmp[16];
    int n = 0, i = 0;
    do { tmp[n++] = (char)('0' + (int)(v % 10)); v /= 10; } while (v);
    while (n) {
        out[i++] = tmp[--n];
        if (n && (n % 3) == 0) out[i++] = ',';
    }
    out[i] = 0;
    return out;
}

/* ---------------- bit writer (MSB first) ---------------- */
typedef struct { u8 *buf; u32 cap, nbits; } BW;

static void bw_new(BW *b) {
    b->cap = 1u << 16;
    b->buf = (u8 *)calloc(b->cap, 1);
    b->nbits = 0;
    if (!b->buf) die("oom bw");
}
static void bw_grow(BW *b, u32 need) {
    u8 *nb;
    if (need <= b->cap) return;
    while (b->cap < need) b->cap <<= 1;
    nb = (u8 *)realloc(b->buf, b->cap);
    if (!nb) die("oom bw");
    b->buf = nb;
}
static void bw_bit(BW *b, int bit) {
    u32 pos = b->nbits++;
    if ((pos >> 3) + 1 > b->cap) bw_grow(b, (pos >> 3) + 1);
    if (bit) b->buf[pos >> 3] |= (u8)(0x80 >> (pos & 7));
}
static void bw_bits(BW *b, u32 v, int n) {
    int k;
    for (k = n - 1; k >= 0; k--) bw_bit(b, (v >> k) & 1);
}
static u32 bw_bytes(BW *b) { return (b->nbits + 7) >> 3; }

/* ---------------- Huffman builder (yzenc sub_4021C0) ---------------- */
typedef struct { u32 value; u16 symbol; u16 next; } Node;

static int node_cmp_asc(const void *pa, const void *pb) {
    const Node *a = (const Node *)pa, *b = (const Node *)pb;
    u32 da, db;
    if (a->value != b->value) return a->value < b->value ? -1 : 1;
    da = (u32)a->symbol | ((u32)a->next << 16);
    db = (u32)b->symbol | ((u32)b->next << 16);
    return da < db ? -1 : (da > db ? 1 : 0);
}

/* pass-A qsort uses PtFuncCompare: -1 iff b < a  ->  descending */
static int node_cmp_desc(const void *pa, const void *pb) {
    const Node *a = (const Node *)pa, *b = (const Node *)pb;
    u32 da, db;
    if (a->value != b->value) return a->value > b->value ? -1 : 1;
    da = (u32)a->symbol | ((u32)a->next << 16);
    db = (u32)b->symbol | ((u32)b->next << 16);
    return da > db ? -1 : (da < db ? 1 : 0);
}

/* pass A: freqs -> code lengths (insert shifts past equal: stops on >) */
static void huff_lens(const u32 *freq, int N, u16 *lens) {
    Node *nodes = (Node *)malloc(sizeof(Node) * (size_t)N);
    int i;
    if (!nodes) die("oom nodes");
    for (i = 0; i < N; i++) { nodes[i].value = freq[i]; nodes[i].symbol = (u16)i; nodes[i].next = 0xFFFF; lens[i] = 0; }
    qsort(nodes, (size_t)N, sizeof(Node), node_cmp_desc);
    for (i = N - 2; i >= 0; i--) {
        u16 c = (u16)i, sym;
        u32 sv; u32 sd; int ecx;
        for (;;) {
            sym = nodes[c].symbol;
            lens[sym]++;
            if (nodes[c].next == 0xFFFF) break;
            c = nodes[c].next;
        }
        sym = nodes[i + 1].symbol;
        lens[sym]++;
        c = nodes[i + 1].next;
        while (c != 0xFFFF) {
            sym = nodes[c].symbol;
            lens[sym]++;
            c = nodes[c].next;
        }
        c = (u16)i;
        while (nodes[c].next != 0xFFFF) c = nodes[c].next;
        nodes[c].next = (u16)(i + 1);
        nodes[i].value += nodes[i + 1].value;
        sv = nodes[i].value;
        sd = (u32)nodes[i].symbol | ((u32)nodes[i].next << 16);
        ecx = i - 1;
        while (ecx >= 0 && nodes[ecx].value <= sv) { nodes[ecx + 1] = nodes[ecx]; ecx--; }
        nodes[ecx + 1].value = sv;
        nodes[ecx + 1].symbol = (u16)(sd & 0xFFFF);
        nodes[ecx + 1].next = (u16)(sd >> 16);
    }
    free(nodes);
}

/* pass B: lengths -> left-aligned code values (identical to decoder builder) */
static void build_codes(const u16 *in_lengths, int N, u32 *vals, u16 *clens) {
    Node *nodes = (Node *)malloc(sizeof(Node) * (size_t)N);
    int i;
    if (!nodes) die("oom nodes");
    for (i = 0; i < N; i++) { nodes[i].value = in_lengths[i]; nodes[i].symbol = (u16)i; nodes[i].next = 0xFFFF; vals[i] = 0; clens[i] = 0; }
    qsort(nodes, (size_t)N, sizeof(Node), node_cmp_asc);
    for (i = N - 2; i >= 0; i--) {
        u16 c = (u16)i;
        u32 sv; u32 sd; int ecx;
        for (;;) {
            u16 sym = nodes[c].symbol;
            clens[sym]++;
            vals[sym] >>= 1;
            if (nodes[c].next == 0xFFFF) break;
            c = nodes[c].next;
        }
        { u16 sym = nodes[i + 1].symbol;
          clens[sym]++;
          vals[sym] = (vals[sym] >> 1) | 0x80000000u; }
        c = nodes[i + 1].next;
        while (c != 0xFFFF) {
            u16 sym = nodes[c].symbol;
            clens[sym]++;
            vals[sym] = (vals[sym] >> 1) | 0x80000000u;
            c = nodes[c].next;
        }
        c = (u16)i;
        while (nodes[c].next != 0xFFFF) c = nodes[c].next;
        nodes[c].next = (u16)(i + 1);
        nodes[i].value -= 1;
        sv = nodes[i].value;
        sd = (u32)nodes[i].symbol | ((u32)nodes[i].next << 16);
        ecx = i - 1;
        while (ecx >= 0 && nodes[ecx].value >= sv) { nodes[ecx + 1] = nodes[ecx]; ecx--; }
        nodes[ecx + 1].value = sv;
        nodes[ecx + 1].symbol = (u16)(sd & 0xFFFF);
        nodes[ecx + 1].next = (u16)(sd >> 16);
    }
    free(nodes);
}

static void huff_build(const u32 *freq, int N, u32 *vals, u16 *lens) {
    int i;
    u32 mx = 0;
    huff_lens(freq, N, lens);
    for (i = 0; i < N; i++) if (lens[i] > mx) mx = lens[i];
    if (mx >= 32) die("code length overflow");
    build_codes(lens, N, vals, lens);
}

/* ---------------- LZ pass (yzenc sub_4014D0) ---------------- */
typedef struct {
    u16 asy;      /* A-stream symbol: literal 512+b, match 0..255, family 256..511 */
    u32 mclass;   /* match class (len-2), valid when asy < 256 */
    u8  ismatch;
} Tok;

typedef struct {
    const u8 *data;
    u32 p, h;
    u16 A[RING], B[RING];
    u16 heads[RING * RING];
    u16 nxt[RING * RING];
    u16 prv[RING * RING];
    u32 T1v[RING * RING];   /* pass-local chunk-start offsets; 0 = empty */
    u16 T2v[RING * RING];
} LZ;

static void lz_reset(LZ *s) {
    memset(s->A, 0, sizeof(s->A));
    memset(s->B, 0, sizeof(s->B));
    memset(s->heads, 0xFF, sizeof(s->heads));
    memset(s->nxt, 0xFF, sizeof(s->nxt));
    memset(s->prv, 0xFF, sizeof(s->prv));
    memset(s->T1v, 0, sizeof(s->T1v));
    memset(s->T2v, 0, sizeof(s->T2v));
    s->h = 0;
}

static void chain_unlink(LZ *s, u32 row, u16 a) {
    u16 n = s->nxt[row | a], v = s->prv[row | a];
    if (n != 0xFFFF) s->prv[row | n] = v;
    if (v == 0xFFFF) {
        u32 off = s->T1v[row | a];
        if (off) s->heads[((row >> 8) << 8) | s->data[off]] = n;
    } else {
        s->nxt[row | v] = n;
    }
}

static void chain_link(LZ *s, u32 row, u16 a) {
    u32 c = row >> 8;
    u16 b = s->heads[(c << 8) | s->data[s->T1v[row | a]]];
    if (b != 0xFFFF) s->prv[row | b] = a;
    s->nxt[row | a] = b;
    s->prv[row | a] = 0xFFFF;
    s->heads[(c << 8) | s->data[s->T1v[row | a]]] = a;
}

static void lz_pass(LZ *s, const u8 *data, u32 len, u32 *freqA, u32 *freqB,
                    Tok **toks, u32 *ntok, u32 *cap) {
    u32 p = 0;
    s->data = data;
    lz_reset(s);
    if (len == 0) return;
    for (;;) {
        u32 ml = 0, best = 0, chunklen, p_start = p, srcdbg = 0;
        Tok *tk;
        if (*ntok == *cap) {
            *cap = *cap * 2 + 256;
            *toks = (Tok *)realloc(*toks, sizeof(Tok) * (size_t)*cap);
            if (!*toks) die("oom toks");
        }
        tk = &(*toks)[(*ntok)++];

        if (p >= 1) {
            u32 q = data[p - 1];
            u32 row = q << 8;
            u32 a0 = s->A[q];
            if (s->B[q] != 0) {
                u32 i = s->heads[(q << 8) | data[p]];
                while (!(i & 0x8000)) {
                    u32 src = s->T1v[row | i];
                    if (data[src + ml] == data[p + ml]) {
                        u32 k, j, rem = len - p;
                        int ok = 1;
                        for (k = ml; k > 0; ) {
                            k--;
                            if (data[src + k] != data[p + k]) { ok = 0; break; }
                        }
                        if (ok) {
                            j = ml + 1;
                            while (j < rem && data[src + j] == data[p + j]) j++;
                            best = i; ml = j; srcdbg = src;
                            if (j >= rem) break;
                        }
                    }
                    i = s->nxt[row | i];
                }
            }
        if (ml >= 2) {
            u32 delta = (RING + best - a0) % RING;
            int fam = (s->T2v[row | best] == (u16)ml);
            if (fam) {
                    tk->asy = (u16)(256 + delta);
                    tk->ismatch = 0;
                    freqA[tk->asy]++;
                    if (s->T1v[row | a0]) {
                        chain_unlink(s, row, (u16)best);
                        s->T1v[row | best] = s->T1v[row | a0];
                        s->T2v[row | best] = s->T2v[row | a0];
                        chain_link(s, row, (u16)best);
                    }
                } else {
                    tk->asy = (u16)delta;
                    tk->ismatch = 1;
                    tk->mclass = ml - 2;
                    freqA[tk->asy]++;
                    freqB[ml - 2 >= 127 ? 127 : ml - 2]++;
                }
                chunklen = ml;
                p += ml;
            } else {
                tk->asy = (u16)(512 + data[p]);
                tk->ismatch = 0;
                freqA[tk->asy]++;
                chunklen = ml;   /* 0 (no match) or 1 (len-1 candidate) */
                p += 1;
            }
        } else {
            tk->asy = (u16)(512 + data[p]);
            tk->ismatch = 0;
            freqA[tk->asy]++;
            chunklen = 0;
            p += 1;
        }

        /* ring update: if (p - 1 > h) */
        if (p - 1 > s->h) {
            u32 cc = data[s->h];
            u32 rr = cc << 8;
            u16 a = s->A[cc];
            chain_unlink(s, rr, a);
            s->T1v[rr | a] = s->h + 1;
            s->T2v[rr | a] = (u16)chunklen;
            chain_link(s, rr, a);
            s->A[cc] = (u16)(a + 1);
            if (s->B[cc] < s->A[cc]) s->B[cc] = s->A[cc];
            s->A[cc] = (u16)(s->A[cc] % RING);
            s->h = p - 1;
        }
        if (getenv("YZENCDBG")) {
            u32 asy = tk->asy;
            if (asy >= 512) fprintf(stderr, "ENC p=%u LIT b=0x%02X ml=%u\n", p_start, asy & 0xFF, ml);
            else if (ml >= 2) fprintf(stderr, "ENC p=%u %s sym=%u class=%u len=%u src=%u\n", p_start, asy >= 256 ? "FAM" : "MAT", asy, tk->mclass, ml, srcdbg);
            else fprintf(stderr, "ENC p=%u LIT b=0x%02X ml=%u\n", p_start, asy & 0xFF, ml);
        }
        if (p >= len) break;
    }
}

/* ---------------- input collection ---------------- */
typedef struct { char fs[1024]; char path[1024]; u32 size; u32 ts; } Ent;

static Ent *g_ents; static u32 g_nent, g_cap;

static void add_ent(const char *fspath, const char *disp, u32 size, u32 ts) {
    if (g_nent == g_cap) {
        g_cap = g_cap * 2 + 32;
        g_ents = (Ent *)realloc(g_ents, sizeof(Ent) * (size_t)g_cap);
        if (!g_ents) die("oom ents");
    }
    if (strlen(fspath) >= sizeof(g_ents[0].fs)) { fprintf(stderr, "yzenc: path too long: %s\n", fspath); exit(1); }
    if (strlen(disp) >= sizeof(g_ents[0].path)) { fprintf(stderr, "yzenc: name too long: %s\n", disp); exit(1); }
    strcpy(g_ents[g_nent].fs, fspath);
    strcpy(g_ents[g_nent].path, disp);
    g_ents[g_nent].size = size;
    g_ents[g_nent].ts = ts;
    g_nent++;
}

static void add_dir2(const char *fspath, const char *prefix);   /* forward */

static void add_file2(const char *fspath, const char *disp) {
    struct _stat st;
    if (_stat(fspath, &st) != 0) { fprintf(stderr, "yzenc: cannot stat %s\n", fspath); exit(1); }
    add_ent(fspath, disp, (u32)st.st_size, (u32)st.st_mtime);
}

static void add_dir2(const char *fspath, const char *prefix) {
    char sub[1032];
    long h;
    struct _finddata_t fd;
    sprintf(sub, "%s\\*", fspath);
    h = _findfirst(sub, &fd);
    if (h == -1) { fprintf(stderr, "yzenc: cannot list dir %s\n", fspath); exit(1); }
    do {
        if (strcmp(fd.name, ".") == 0 || strcmp(fd.name, "..") == 0) continue;
        {
            char fp[1024], dp[1024];
            sprintf(fp, "%s\\%s", fspath, fd.name);
            sprintf(dp, "%s\\%s", prefix, fd.name);
            if (fd.attrib & _A_SUBDIR) add_dir2(fp, dp);
            else add_file2(fp, dp);
        }
    } while (_findnext(h, &fd) == 0);
    _findclose(h);
}

/* reference strips only backslash-separated directories from the stored name */
static int is_dbcs_lead(unsigned char c) {
    return (c & 0xF0) == 0x80 || (c & 0xF0) == 0x90 || (c & 0xF0) == 0xE0;
}

static const char *arg_top(const char *arg) {
    const char *p = arg, *top = arg;
    while (*p) {
        if (is_dbcs_lead((unsigned char)*p) && p[1]) { p += 2; continue; }
        if (*p == '\\') top = p + 1;
        p++;
    }
    return top;
}

static void add_any(const char *arg) {
    struct _stat st;
    const char *top = arg_top(arg);
    if (_stat(arg, &st) != 0) { fprintf(stderr, "yzenc: cannot stat %s\n", arg); exit(1); }
    if (st.st_mode & _S_IFDIR) add_dir2(arg, top);
    else add_file2(arg, top);
}

/* ---------------- misc ---------------- */
static void pe32(u8 *d, u32 v) { d[0] = (u8)(v >> 24); d[1] = (u8)(v >> 16); d[2] = (u8)(v >> 8); d[3] = (u8)v; }

static u8 *read_full(const char *path, u32 size) {
    FILE *f = fopen(path, "rb");
    u8 *b;
    if (!f) { fprintf(stderr, "yzenc: cannot open %s\n", path); exit(1); }
    b = (u8 *)malloc(size ? size : 1);
    if (!b) die("oom");
    if (size && fread(b, 1, size, f) != size) { fprintf(stderr, "yzenc: short read %s\n", path); exit(1); }
    fclose(f);
    return b;
}

static int has_yz1_ext(const char *p) {
    size_t n = strlen(p);
    return n >= 4 && p[n - 4] == '.'
        && (p[n - 3] == 'y' || p[n - 3] == 'Y')
        && (p[n - 2] == 'z' || p[n - 2] == 'Z')
        && p[n - 1] == '1';
}

static int same_file(const char *a, const char *b) {
    for (;;) {
        int ca = (unsigned char)*a++, cb = (unsigned char)*b++;
        if (ca == '/') ca = '\\';
        if (cb == '/') cb = '\\';
        ca = tolower(ca); cb = tolower(cb);
        if (ca != cb) return 0;
        if (!ca) return 1;
    }
}

static int exists(const char *p) {
    struct _stat st;
    return _stat(p, &st) == 0;
}

/* EOF means yes, as in the 1.05.2 tools */
static int ask_yes(const char *question, int assume_yes) {
    char buf[80];
    if (assume_yes) { printf("All YES.\n"); return 1; }
    printf("%s [y/n]:", question);
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return 1;
    return !(buf[0] == 'n' || buf[0] == 'N');
}

static int ask_overwrite(const char *outname, int assume_yes) {
    char q[1200];
    _snprintf(q, sizeof(q), "File(%s) has already existed. Do you write over this file?", outname);
    q[sizeof(q) - 1] = 0;
    return ask_yes(q, assume_yes);
}

/* output name: reference uses (outdir\) + arg after last backslash, ext -> .yz1 */
static void make_outname(const char *arg, const char *outdir, char *out, size_t outsz) {
    char top[1024];
    const char *t = arg_top(arg);
    size_t k, bl, dot;
    const char *last = NULL;

    strcpy(top, t);
    for (k = 0; top[k]; ) {
        if (is_dbcs_lead((unsigned char)top[k]) && top[k + 1]) { k += 2; continue; }
        if (top[k] == '/') top[k] = '\\';
        k++;
    }
    bl = strlen(top);
    for (k = 0; k < bl; ) {
        if (is_dbcs_lead((unsigned char)top[k]) && k + 1 < bl) { k += 2; continue; }
        if (top[k] == '.') last = top + k;
        k++;
    }
    dot = last ? (size_t)(last - top) : bl;
    if (outdir) _snprintf(out, outsz, "%s\\%.*s.yz1", outdir, (int)dot, top);
    else        _snprintf(out, outsz, "%.*s.yz1", (int)dot, top);
    out[outsz - 1] = 0;
}

static int pack(const char *const *args, int nargs, const char *outdir,
                const u8 key[4], int enc, u8 chk, int assume_yes)
{
    char outname[1040];
    u32 Cnt, i;
    u32 Bt, Atot, off, nbyte;
    u8 *tabledata;
    u32 nt1 = 0, nt2 = 0, cap1 = 0, cap2 = 0;
    Tok *t1 = NULL, *t2 = NULL;
    u32 freqA[ALPHA_A], freqB[ALPHA_B];
    u32 valsA[ALPHA_A], valsB[ALPHA_B];
    u16 lensA[ALPHA_A], lensB[ALPHA_B];
    static LZ lz;
    BW bw;
    FILE *f;
    u8 hd[20];
    int k;
    clock_t t0;
    u32 source_bytes, out_bytes;
    char q[1200];

    t0 = clock();

    for (k = 0; k < nargs; k++) {
        if (has_yz1_ext(args[k])) {
            printf("Message(2039):\nFile(%s) which file-extension is '.yz1' cannot deal with.\nPlease change extension.\n", args[k]);
            return -1;
        }
    }

    g_nent = 0;
    for (k = 0; k < nargs; k++) add_any(args[k]);
    if (g_nent == 0) {
        int any_dir = 0;
        for (k = 0; k < nargs; k++) {
            struct _stat st;
            if (_stat(args[k], &st) == 0 && (st.st_mode & _S_IFDIR)) any_dir = 1;
        }
        if (!any_dir) { fprintf(stderr, "yzenc: no input files\n"); return 1; }
    }

    make_outname(args[0], outdir, outname, sizeof(outname));
    if (nargs == 1) {
        struct _stat st;
        if (_stat(args[0], &st) == 0 && (st.st_mode & _S_IFDIR)) {
            u32 total = 0;
            for (i = 0; i < g_nent; i++) total += g_ents[i].size;
            printf("InDir: %s (%d files, %s Bytes)\n", arg_top(args[0]), (int)g_nent, fmt_num(total));
        }
    }
    for (i = 0; i < g_nent; i++) {
        if (same_file(outname, g_ents[i].fs)) {
            printf("Message(2040):\nCannot write over previous file(%s)\nPlease change a folder into which put compressed file.\n", outname);
            return 1;
        }
    }

    Cnt = g_nent;
    Bt = 8 * Cnt;
    for (i = 0; i < Cnt; i++) Bt += (u32)strlen(g_ents[i].path) + 1;
    Atot = Bt;
    for (i = 0; i < Cnt; i++) Atot += g_ents[i].size;

    /* table data: [sizes x C][timestamps x C][names, NUL separated] */
    tabledata = (u8 *)malloc(Bt ? Bt : 1);
    if (!tabledata) die("oom table");
    off = 0;
    for (i = 0; i < Cnt; i++) { pe32(tabledata + off, g_ents[i].size); off += 4; }
    for (i = 0; i < Cnt; i++) { pe32(tabledata + off, g_ents[i].ts); off += 4; }
    for (i = 0; i < Cnt; i++) {
        u32 l = (u32)strlen(g_ents[i].path) + 1;
        memcpy(tabledata + off, g_ents[i].path, l);
        off += l;
    }
    if (off != Bt) die("table size internal error");

    /* two passes: the table as one buffer, then payload in max-file-size
       batches.  The reference resets the LZ state whenever the next file
       would not fit in its max-file-size buffer. */
    memset(freqA, 0, sizeof(freqA));
    memset(freqB, 0, sizeof(freqB));
    lz_pass(&lz, tabledata, Bt, freqA, freqB, &t1, &nt1, &cap1);
    {
        u32 max_size = 0, batch_len = 0;
        u8 *batch;
        for (i = 0; i < Cnt; i++) if (g_ents[i].size > max_size) max_size = g_ents[i].size;
        batch = (u8 *)malloc(max_size ? max_size : 1);
        if (!batch) die("oom payload");
        for (i = 0; i < Cnt; i++) {
            u8 *fb;
            printf("InFile(%d/%d): %s (%s Bytes)\n", (int)i + 1, (int)Cnt, g_ents[i].path, fmt_num(g_ents[i].size));
            if (batch_len + g_ents[i].size > max_size) {
                lz_pass(&lz, batch, batch_len, freqA, freqB, &t2, &nt2, &cap2);
                batch_len = 0;
            }
            fb = read_full(g_ents[i].fs, g_ents[i].size);
            memcpy(batch + batch_len, fb, g_ents[i].size);
            batch_len += g_ents[i].size;
            free(fb);
        }
        if (batch_len) lz_pass(&lz, batch, batch_len, freqA, freqB, &t2, &nt2, &cap2);
        free(batch);
    }

    /* build Huffman tables once for both passes */
    huff_build(freqA, ALPHA_A, valsA, lensA);
    huff_build(freqB, ALPHA_B, valsB, lensB);

    /* emit stream: lensA, lensB, pass1 tokens, pass2 tokens */
    bw_new(&bw);
    for (i = 0; i < ALPHA_A; i++) bw_bits(&bw, lensA[i], 5);
    for (i = 0; i < ALPHA_B; i++) bw_bits(&bw, lensB[i], 5);
    {
        u32 pass;
        for (pass = 0; pass < 2; pass++) {
            Tok *tv = pass ? t2 : t1;
            u32 nv = pass ? nt2 : nt1;
            u32 j;
            for (j = 0; j < nv; j++) {
                u16 sym = tv[j].asy;
                u32 L = lensA[sym];
                bw_bits(&bw, L ? (valsA[sym] >> (32 - L)) : 0, L);   /* L >= 1 for used syms */
                if (tv[j].ismatch) {
                    u32 cls = tv[j].mclass;
                    if (cls < 127) {
                        u32 LB = lensB[cls];
                        bw_bits(&bw, valsB[cls] >> (32 - LB), LB);
                    } else {
                        u32 LB = lensB[127];
                        u32 e = 31, x = cls;
                        bw_bits(&bw, valsB[127] >> (32 - LB), LB);
                        while (!(x & 0x80000000u)) { x <<= 1; e--; }
                        bw_bits(&bw, e, 5);
                        bw_bits(&bw, cls, (int)e + 1);
                    }
                }
            }
        }
    }

    nbyte = bw_bytes(&bw);
    if (exists(outname) && !ask_overwrite(outname, assume_yes)) {
        printf("Information(1042):\nProcess can not continue. Thus stopped. without output file(%s)\n", outname);
        free(t1); free(t2); free(tabledata); free(bw.buf);
        return 0;
    }

    source_bytes = Atot - Bt;
    out_bytes = 20 + nbyte;
    if (out_bytes > source_bytes) {
        printf("Output File : %s (Cannot compress : %.1f%%, %s Bytes)\n",
               outname, source_bytes ? (double)out_bytes * 100.0 / (double)source_bytes : 0.0, fmt_num(out_bytes));
        _snprintf(q, sizeof(q),
                  "Could not compress because data is too few or too complex.\na filesize of compressed file(%s) is large than previous file. Do you output file?",
                  outname);
        q[sizeof(q) - 1] = 0;
        if (!ask_yes(q, assume_yes)) {
            printf("Information: File(%s) is not output.\n", outname);
            free(t1); free(t2); free(tabledata); free(bw.buf);
            return 0;
        }
    }

    /* header + write */
    memcpy(hd, "yz0105", 6);
    if (enc) {
        u32 k;
        for (k = 0; k < nbyte; k++) bw.buf[k] ^= key[k & 3];
        hd[6] = 0x34;
        hd[7] = chk;
    } else {
        hd[6] = 0x30;
        hd[7] = 0x30;
    }
    pe32(hd + 8, Atot);
    pe32(hd + 12, Bt);
    pe32(hd + 16, Cnt);
    f = fopen(outname, "wb");
    if (!f) {
        fprintf(stderr, "yzenc: cannot write %s\n", outname);
        free(t1); free(t2); free(tabledata); free(bw.buf);
        return 1;
    }
    fwrite(hd, 1, 20, f);
    fwrite(bw.buf, 1, nbyte, f);
    fclose(f);
    printf("OutFile: %s (%s Bytes)\n", outname, fmt_num(20 + nbyte));
    printf("Compression process is done successfully. (Processed in: %.2f seconds)\n",
           (double)(clock() - t0) / (double)CLOCKS_PER_SEC);
    free(t1); free(t2); free(tabledata); free(bw.buf);
    return 0;
}

static void usage(void) {
    fprintf(stderr,
        "usage: yzenc [-d[outdir]] [-p[password]] [-y] [-j] [-m] <file|dir> ...\n"
        "  -m  one archive per input (default: one archive for all inputs)\n"
        "  -j  accepted for compatibility (English-only messages)\n");
}

int main(int argc, char **argv) {
    const char *outdir = NULL;
    const char *passwd = NULL;
    const char **args;
    char pwbuf[256], pwb2[256], dirbuf[1024];
    u8 key[4];
    u8 chk = 0;
    int enc = 0;
    int nargs = 0, multi = 0, assume_yes = 0, i, rc, failed = 0;

    args = (const char **)malloc(sizeof(*args) * (size_t)(argc > 0 ? argc : 1));
    if (!args) die("oom args");
    printf("yzenc : YAMAZAKI-Zip compatible encoder (1.05.2)\n");

    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-d", 2) == 0) {
            outdir = argv[i] + 2;
            if (!outdir[0]) {
                if (!read_line("InputDirName:\nPlease choose a folder you want to put compressed file(s) into.\n?:", dirbuf, sizeof(dirbuf)) || !dirbuf[0])
                    outdir = NULL;
                else
                    outdir = dirbuf;
            }
        }
        else if (strncmp(argv[i], "-p", 2) == 0) passwd = argv[i] + 2;
        else if (strcmp(argv[i], "-y") == 0) assume_yes = 1;
        else if (strcmp(argv[i], "-m") == 0) multi = 1;
        else if (strcmp(argv[i], "-j") == 0) { /* English-only build: no-op */ }
        else if (argv[i][0] == '-') { printf("Illegal Option!! : %s\n", argv[i]); usage(); return 1; }
        else args[nargs++] = argv[i];
    }
    if (nargs == 0) { usage(); return 1; }

    if (passwd) enc = 1;
    if (passwd && !passwd[0]) {
        for (;;) {
            if (!read_line("InputPassword(1):\n", pwbuf, sizeof(pwbuf)) || !pwbuf[0]) {
                fprintf(stderr, "yzenc: password was not input\n");
                return 1;
            }
            if (!read_line("Please input again to confirm:\n", pwb2, sizeof(pwb2))) {
                fprintf(stderr, "yzenc: password was not input\n");
                return 1;
            }
            if (strcmp(pwbuf, pwb2) == 0) break;
            printf("Password incorrect. Please retry again.\n");
        }
        passwd = pwbuf;
    }
    if (enc && !parse_pub(passwd, key, &chk)) {
        derive_key(passwd, key);
        chk = pw_chk(passwd);
    }

    if (multi) {
        for (i = 0; i < nargs; i++) {
            rc = pack(&args[i], 1, outdir, key, enc, chk, assume_yes);
            if (rc != 0) failed = 1;
        }
        return failed;
    }
    return pack(args, nargs, outdir, key, enc, chk, assume_yes) == 0 ? 0 : 1;
}
