/* yzdec.c - .yz1 unpacker (YAMAZAKI-Zip v1.05.2), port of validated yzdec_py.py.
 * Build (MSVC6): cl /O2 /W2 yzdec.c /Feyzdec.exe
 * Usage: yzdec [-d<outdir>] [-p<password>] [-y] [-t] [-j] <archive.yz1> ...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <sys/utime.h>

typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32;
typedef int i32;

#define ALPHA_A 768
#define ALPHA_B 128
#define RING    256
#define MASK32  0xFFFFFFFFu

static void die(const char *m) { fprintf(stderr, "yzdec: %s\n", m); exit(1); }
static void die2(const char *m, const char *a) { fprintf(stderr, "yzdec: %s%s\n", m, a); exit(1); }

/* ---------------- password key derivation (sub_4015EA) ---------------- */
static void derive_key(const char *pw, u8 key[4]) {
    const u8 *b = (const u8 *)pw; u32 n = (u32)strlen(pw);
    u32 v100 = n, v101 = n, v34 = n, inc = n;
    u32 idx;
    for (idx = 0; idx < n; idx++) {
        u32 ch = (u32)(int)(signed char)b[idx], v36;
        v36 = (ch ^ idx) & MASK32;
        v34 = (v34 + v36) & MASK32;
        v100 = (v100 - v36) & MASK32;
        v101 ^= (v36 & MASK32);
        inc = (inc * v36) & MASK32;
    }
    { u32 v37 = (v101 + inc + v34 * v100) & MASK32;
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

/* ---------------- bit reader (MSB first) ---------------- */
typedef struct { const u8 *d; u32 len, pos; unsigned __int64 acc; int n; } BitRd;
static int br_read(BitRd *b, int k) {
    while (b->n < k) {
        if (b->pos >= b->len) die("bit reader overrun");
        b->acc = (b->acc << 8) | b->d[b->pos++];
        b->n += 8;
    }
    b->n -= k;
    return (int)((b->acc >> b->n) & ((1u << k) - 1));
}

/* ---------------- Huffman code builder (yzdec sub_404974) ---------------- */
typedef struct { u32 value; u16 symbol; u16 next; } Node;
typedef struct TagV { u32 v; int sym; } V;
static int node_cmp(const void *pa, const void *pb) {
    const Node *a = (const Node *)pa, *b = (const Node *)pb;
    if (a->value != b->value) return a->value < b->value ? -1 : 1;
    { u32 da = (u32)a->symbol | ((u32)a->next << 16);
      u32 db = (u32)b->symbol | ((u32)b->next << 16);
      return da < db ? -1 : (da > db ? 1 : 0); }
}
static int v_cmp(const void *x, const void *y) {
    u32 a = ((const V *)x)->v, b = ((const V *)y)->v;
    return a < b ? -1 : (a > b ? 1 : 0);
}
static void build_codes(const u16 *in_lengths, int N, u32 *vals, u16 *clens) {
    Node *nodes = (Node *)malloc(sizeof(Node) * (size_t)N);
    int i, s;
    for (s = 0; s < N; s++) { nodes[s].value = in_lengths[s]; nodes[s].symbol = (u16)s; nodes[s].next = 0xFFFF; vals[s] = 0; clens[s] = 0; }
    qsort(nodes, (size_t)N, sizeof(Node), node_cmp);
    for (i = N - 2; i >= 0; i--) {
        u16 c = (u16)i;
        u32 sv; u32 sd; i32 ecx;
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

/* ---------------- symbol decoder: sorted arrays per code length ---------------- */
typedef struct {
    V *a[32]; int cnt[32];
} SymDecoder;

static void sd_init(SymDecoder *sd, const u32 *vals, const u16 *lens, int N) {
    int i, j; int off[32];
    memset(sd, 0, sizeof(*sd));
    for (i = 0; i < N; i++) {
        u16 L = lens[i];
        if (L == 0) continue;
        if (L > 31) die("code len > 31");
        sd->cnt[L]++;
    }
    for (j = 1; j <= 31; j++) {
        sd->a[j] = (V *)malloc(sizeof(V) * (sd->cnt[j] ? (size_t)sd->cnt[j] : 1));
        if (!sd->a[j]) die("oom groups");
        off[j] = 0;
    }
    for (i = 0; i < N; i++) {
        u16 L = lens[i];
        if (!L) continue;
        sd->a[L][off[L]].v = vals[i] >> (32 - L);
        sd->a[L][off[L]].sym = i;
        off[L]++;
    }
    for (j = 1; j <= 31; j++) qsort(sd->a[j], (size_t)sd->cnt[j], sizeof(V), v_cmp);
}

static void sd_free(SymDecoder *sd) { int j; for (j = 1; j <= 31; j++) free(sd->a[j]); }

static int sd_read(BitRd *br, const SymDecoder *sd) {
    u32 code = 0; int nb = 1;
    for (;;) {
        code = (code << 1) | (u32)br_read(br, 1);
        if (nb > 31) die("symbol overrun");
        { const V *g = sd->a[nb]; int n = sd->cnt[nb];
          int lo = 0, hi = n - 1;
          while (lo <= hi) {
              int mid = (lo + hi) >> 1;
              u32 v = g[mid].v;
              if (v == code) return g[mid].sym;
              if (v < code) lo = mid + 1; else hi = mid - 1;
          } }
        nb++;
    }
}

/* ---------------- LZ decode (yzdec sub_40465D) ---------------- */
typedef struct {
    u16 A[RING], B[RING];
    u32 *T1, *T2;              /* [RING*RING] */
    u8 *out; u32 outpos, h;
} LzState;

static void lz_reset(LzState *st, u8 *out) {
    memset(st->A, 0, sizeof(st->A));
    memset(st->B, 0, sizeof(st->B));
    memset(st->T1, 0, sizeof(u32) * RING * RING);
    memset(st->T2, 0, sizeof(u32) * RING * RING);
    st->out = out; st->outpos = 0; st->h = 0;
}

static int lz_decompress(BitRd *br, const SymDecoder *sA, const SymDecoder *sB, u32 need, LzState *st, u32 cap) {
    u32 limit = st->outpos + need;
    int dbg = getenv("YZDECDBG") != 0;
    if (need > cap - st->outpos) return 0;
    while (st->outpos < limit) {
        u32 p_start = st->outpos;
        int sym = sd_read(br, sA);
        int is_lit; u32 src = 0; u32 ml = 1;
        if (sym >= 512) {
            st->out[st->outpos++] = (u8)(sym - 512);
            is_lit = 1;
        } else {
            u8 c;
            u16 a0;
            u32 slot;
            if (st->h >= st->outpos) return 0;
            c = st->out[st->h];
            a0 = st->A[c];
            slot = ((u32)(sym & 0xFF) + a0) & 0xFF;
            is_lit = 0;
            if (sym >= 256) {
                ml = st->T2[c * RING + slot];
                src = st->T1[c * RING + slot];
                { u32 v11 = st->T1[c * RING + a0];
                  if (v11) { st->T1[c * RING + slot] = v11; st->T2[c * RING + slot] = st->T2[c * RING + a0]; } }
            } else {
                int lc = sd_read(br, sB);
                if (lc == 127) {
                    int e = br_read(br, 5);
                    lc = br_read(br, e + 1);
                }
                ml = (u32)(lc + 2);
                src = st->T1[c * RING + slot];
            }
            if (!ml || src >= st->outpos) return 0;
            if (ml > cap - st->outpos) return 0;
            { u32 i; for (i = 0; i < ml; i++) st->out[st->outpos + i] = st->out[src + i];
              st->outpos += ml; }
        }
        if (dbg) {
            if (is_lit) fprintf(stderr, "DEC p=%u LIT b=0x%02X\n", p_start, (unsigned)(sym & 0xFF));
            else fprintf(stderr, "DEC p=%u %s sym=%d len=%u src=%u\n", p_start, sym >= 256 ? "FAM" : "MAT", sym, ml, src);
        }
        if (st->outpos - 1 > st->h) {
            u8 c = st->out[st->h];
            u16 a = st->A[c], v22;
            u32 idx = (u32)c * RING + a;
            st->T1[idx] = st->h + 1;
            st->T2[idx] = ml;
            v22 = (u16)(a + 1);
            if (st->B[c] < v22) st->B[c] = v22;
            st->A[c] = v22 % RING;
            st->h = st->outpos - 1;
        }
    }
    return 1;
}

/* ---------------- fs helpers ---------------- */
static int is_dbcs_lead(unsigned char c) {
    return (c & 0xF0) == 0x80 || (c & 0xF0) == 0x90 || (c & 0xF0) == 0xE0;
}

static void make_parents(const char *path) {
    char tmp[1300]; char *q;
    if (strlen(path) >= sizeof(tmp)) die2("path too long: ", path);
    _snprintf(tmp, sizeof(tmp) - 1, "%s", path);
    tmp[sizeof(tmp) - 1] = 0;
    for (q = tmp; *q; ) {
        if (is_dbcs_lead((unsigned char)*q) && q[1]) { q += 2; continue; }
        if (*q == '/') *q = '\\';
        q++;
    }
    for (q = tmp + 1; *q; ) {
        if (is_dbcs_lead((unsigned char)*q) && q[1]) { q += 2; continue; }
        if (*q == '\\') {
            char c = *q; *q = 0;
            if (tmp[0] && tmp[strlen(tmp) - 1] != ':') _mkdir(tmp);
            *q = c;
        }
        q++;
    }
}

static void write_file(const char *path, const u8 *data, size_t size, u32 ts) {
    FILE *f = fopen(path, "wb");
    struct _utimbuf ut;
    if (!f) die2("cannot create: ", path);
    if (size && fwrite(data, 1, size, f) != size) die2("write failed: ", path);
    fclose(f);
    if (ts) {
        ut.actime = (time_t)ts; ut.modtime = (time_t)ts;
        if (_utime(path, &ut) != 0)
            printf("Timestamp of a file(%s) could not be changed. But continue.\n", path);
    }
}

static u32 ge32(const u8 *p) {
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3];
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

static int exists(const char *path) {
    struct _stat st;
    return _stat(path, &st) == 0;
}

/* EOF means yes, as in the 1.05.2 tools */
static int ask_yn(const char *question, int assume_yes) {
    char buf[80];
    if (assume_yes) { printf("All YES.\n"); return 1; }
    printf("%s [y/n]:", question);
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return 1;
    return !(buf[0] == 'n' || buf[0] == 'N');
}

static int compare_file(const char *path, const u8 *data, u32 size) {
    FILE *f = fopen(path, "rb");
    u8 buf[4096];
    u32 off = 0;
    if (!f) return 0;
    while (off < size) {
        u32 n = size - off;
        if (n > sizeof(buf)) n = (u32)sizeof(buf);
        if (fread(buf, 1, n, f) != n) { fclose(f); return 0; }
        if (memcmp(buf, data + off, n) != 0) { fclose(f); return 0; }
        off += n;
    }
    if (fgetc(f) != EOF) { fclose(f); return 0; }
    fclose(f);
    return 1;
}

/* SFX: yzdec.exe + archive + big-endian archive size */
static int has_appended_archive(const char *exe) {
    FILE *f = fopen(exe, "rb");
    long sz;
    u8 t[4];
    u32 tot;
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    if (sz < 28) { fclose(f); return 0; }
    fseek(f, -4, SEEK_END);
    if (fread(t, 1, 4, f) != 4) { fclose(f); return 0; }
    tot = ge32(t);
    if (!tot || (long)tot + 4 > sz) { fclose(f); return 0; }
    fseek(f, sz - 4 - (long)tot, SEEK_SET);
    if (fread(t, 1, 4, f) != 4) { fclose(f); return 0; }
    fclose(f);
    return memcmp(t, "yz01", 4) == 0;
}

static int decode_one(const char *arc, const char *outdir, const char *passwd,
                      int test_mode, int assume_yes, const char *dumpf) {
    int i;
    FILE *f; long fs; u8 *raw; u8 *data; u32 datasz;
    u32 Atot, Bt, Cnt, payload_size; int encrypted; u8 xk[4];
    char pwb[256];
    BitRd br; u32 valsA[ALPHA_A], valsB[ALPHA_B]; u16 lensA[ALPHA_A], lensB[ALPHA_B];
    SymDecoder sA, sB; LzState st;
    u8 *table, *filebuf; u32 batch_off, max_size;
    struct { u32 size; u32 ts; const char *name; } *ents;
    unsigned __int64 sum;
    clock_t t0 = clock();

    f = fopen(arc, "rb");
    if (!f) die2("cannot open archive: ", arc);
    fseek(f, 0, SEEK_END); fs = ftell(f); rewind(f);
    raw = (u8 *)malloc(fs ? (size_t)fs : 1);
    if (!raw) die("oom archive");
    if (fread(raw, 1, (size_t)fs, f) != (size_t)fs) die("short archive");
    fclose(f);
    printf("InFile: %s (%s Bytes)\n", arc, fmt_num((u32)fs));

    data = raw; datasz = (u32)fs;
    if (datasz >= 24 && memcmp(data, "yz01", 4) != 0) {
        u32 tot = ge32(data + datasz - 4);
        if (tot && tot + 4 <= datasz) { data += datasz - 4 - tot; datasz = tot; }
    }
    if (datasz < 20 || memcmp(data, "yz01", 4) != 0) die("bad archive (bad magic)");
    Atot = ge32(data + 8); Bt = ge32(data + 12); Cnt = ge32(data + 16);
    if (Bt > Atot) die("bad archive (header size > total size)");
    if ((unsigned __int64)Cnt * 8u > (unsigned __int64)Bt) die("bad archive (file table too small)");
    payload_size = Atot - Bt;
    encrypted = (data[6] & 0x04) != 0;
    if (encrypted) {
        u32 q;
        if (!passwd || !passwd[0]) {
            size_t l;
            printf("InputPassword(1):\n");
            if (!fgets(pwb, sizeof(pwb), stdin)) die("password was not input");
            l = strlen(pwb);
            while (l && (pwb[l - 1] == '\n' || pwb[l - 1] == '\r')) pwb[--l] = 0;
            if (!l) die("password was not input");
            passwd = pwb;
        }
        derive_key(passwd, xk);
        if (data[7] != pw_chk(passwd)) die("Password is not match !");
        for (q = 20; q < datasz; q++) data[q] ^= xk[(q - 20) & 3];
    }
    table = (u8 *)malloc(Bt ? Bt : 1);
    st.T1 = (u32 *)calloc(RING * RING, sizeof(u32));
    st.T2 = (u32 *)calloc(RING * RING, sizeof(u32));
    if (!table || !st.T1 || !st.T2) die("oom tables");

    br.d = data + 20; br.len = datasz - 20; br.pos = 0; br.acc = 0; br.n = 0;
    for (i = 0; i < ALPHA_A; i++) lensA[i] = (u16)br_read(&br, 5);
    for (i = 0; i < ALPHA_B; i++) lensB[i] = (u16)br_read(&br, 5);
    if (dumpf) {
        FILE *tf = fopen(dumpf, "wb");
        if (tf) {
            u32 p_;
            u16 v;
            for (i = 0; i < ALPHA_A; i++) { v = (u16)lensA[i]; fwrite(&v, 2, 1, tf); }
            for (i = 0; i < ALPHA_B; i++) { v = (u16)lensB[i]; fwrite(&v, 2, 1, tf); }
            p_ = br.pos * 8 - br.n; p_ = 0; (void)p_;
            fwrite(&p_, 4, 1, tf);
            fclose(tf);
        }
    }
    build_codes(lensA, ALPHA_A, valsA, lensA);
    build_codes(lensB, ALPHA_B, valsB, lensB);
    sd_init(&sA, valsA, lensA, ALPHA_A);
    sd_init(&sB, valsB, lensB, ALPHA_B);

    lz_reset(&st, table);
    if (!lz_decompress(&br, &sA, &sB, Bt, &st, Bt)) die("corrupt header stream");

    ents = malloc(sizeof(*ents) * (size_t)(Cnt ? Cnt : 1));
    if (!ents) die("oom file table");
    {
        u32 np = 8 * Cnt;
        for (i = 0; i < (int)Cnt; i++) {
            u32 l = 0;
            ents[i].size = ge32(table + 4 * i);
            ents[i].ts = ge32(table + 4 * Cnt + 4 * i);
            if (np >= Bt) die("bad file table (name offset past end)");
            while (np + l < Bt && table[np + l]) l++;
            if (np + l >= Bt) die("bad file table (unterminated name)");
            ents[i].name = (const char *)(table + np);
            np += l + 1;
        }
        if (np != Bt) fprintf(stderr, "yzdec: warning: table size mismatch (%d vs %d)\n", (int)np, (int)Bt);
    }
    sum = 0;
    for (i = 0; i < (int)Cnt; i++) sum += ents[i].size;
    if (sum > (unsigned __int64)payload_size) die("bad file table (sizes exceed data)");

    max_size = 0;
    for (i = 0; i < (int)Cnt; i++) if (ents[i].size > max_size) max_size = ents[i].size;
    filebuf = (u8 *)malloc(max_size ? max_size : 1);
    if (!filebuf) die("oom file buffer");

    printf("%s: %s (%d files, %s Bytes)\n", test_mode ? "TestDir" : "OutDir", outdir, (int)Cnt, fmt_num(payload_size));
    batch_off = 0;
    for (i = 0; i < (int)Cnt; i++) {
        u32 file_start;
        if (batch_off == 0 || (unsigned __int64)batch_off + ents[i].size > max_size) {
            lz_reset(&st, filebuf);
            batch_off = 0;
        }
        file_start = batch_off;
        if (ents[i].size) {
            if (!lz_decompress(&br, &sA, &sB, ents[i].size, &st, max_size)) die("corrupt data stream");
        }
        batch_off += ents[i].size;

        if (test_mode) {
            printf("TestFile(%d/%d): %s (%s Bytes)\n", i + 1, (int)Cnt, ents[i].name, fmt_num(ents[i].size));
            continue;
        }

        {
            char path[1200];
            int r = _snprintf(path, sizeof(path), "%s\\%s", outdir, ents[i].name);
            if (r < 0 || r >= (int)sizeof(path)) die2("output path too long: ", ents[i].name);
            {
                char *s;
                for (s = path; *s; ) {
                    if (is_dbcs_lead((unsigned char)*s) && s[1]) { s += 2; continue; }
                    if (*s == '/') *s = '\\';
                    s++;
                }
            }
            make_parents(path);
            if (exists(path)) {
                char q[2048];
                _snprintf(q, sizeof(q), "File(%s) has already existed. Do you want to write over it?", ents[i].name);
                q[sizeof(q) - 1] = 0;
                if (!ask_yn(q, assume_yes)) {
                    if (ask_yn("Do you compare previous file with compressed file?", assume_yes)) {
                        printf("CompFile(%d/%d): %s (%s Bytes)\n", i + 1, (int)Cnt, ents[i].name, fmt_num(ents[i].size));
                        if (compare_file(path, filebuf + file_start, ents[i].size))
                            printf("Two files are entirely match.\n");
                        else
                            printf("Contents of previous file and compressed file is not match.\n");
                    } else {
                        printf("File(%s) is not output.\n", ents[i].name);
                    }
                    continue;
                }
            }
            write_file(path, filebuf + file_start, ents[i].size, ents[i].ts);
            printf("OutFile(%d/%d): %s (%s Bytes)\n", i + 1, (int)Cnt, ents[i].name, fmt_num(ents[i].size));
        }
    }
    if (test_mode) {
        printf("Extraction TEST has done.\n has no any errors. (Tested in: %.2f seconds)\n",
               (double)(clock() - t0) / (double)CLOCKS_PER_SEC);
    } else {
        printf("Extraction Process is done successfully. (Processed in: %.2f seconds)\n",
               (double)(clock() - t0) / (double)CLOCKS_PER_SEC);
    }
    free(ents); free(table); free(filebuf); free(st.T1); free(st.T2); free(raw);
    sd_free(&sA); sd_free(&sB);
    return 0;
}

static void usage(void) {
    fprintf(stderr,
        "usage: yzdec [-d[outdir]] [-p[password]] [-y] [-t] [-j] <archive.yz1> ...\n");
}

int main(int argc, char **argv) {
    const char *outdir = ".";
    const char *passwd = NULL;
    const char *dumpf = NULL;
    const char **arcs;
    char dirbuf[1024];
    int narcs = 0, test_mode = 0, assume_yes = 0, i, rc, failed = 0;

    arcs = (const char **)malloc(sizeof(*arcs) * (size_t)(argc > 0 ? argc : 1));
    if (!arcs) die("oom args");
    printf("yzdec : YAMAZAKI-Zip compatible decoder (1.05.2)\n");

    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-p", 2) == 0) passwd = argv[i] + 2;
        else if (strncmp(argv[i], "-d", 2) == 0) {
            outdir = argv[i] + 2;
            if (!outdir[0]) {
                if (!read_line("InputDirName:\nPlease choose a folder you want to put extracted file(s) into.\n?:", dirbuf, sizeof(dirbuf)) || !dirbuf[0])
                    outdir = ".";
                else
                    outdir = dirbuf;
            }
        }
        else if (strncmp(argv[i], "-x", 2) == 0) dumpf = argv[i] + 2;
        else if (strcmp(argv[i], "-t") == 0) test_mode = 1;
        else if (strcmp(argv[i], "-y") == 0) assume_yes = 1;
        else if (strcmp(argv[i], "-j") == 0) { /* English-only build: no-op */ }
        else if (argv[i][0] == '-') { printf("Illegal Option!! : %s\n", argv[i]); usage(); return 1; }
        else arcs[narcs++] = argv[i];
    }

    /* an SFX copy extracts the archive appended to itself, then pauses */
    if (has_appended_archive(argv[0])) {
        rc = decode_one(argv[0], outdir, passwd, test_mode, assume_yes, dumpf);
        if (rc != 0) failed = 1;
        printf("hit [enter] key\n");
        fflush(stdout);
        getchar();
        return failed;
    }

    if (narcs == 0) { usage(); return 1; }
    for (i = 0; i < narcs; i++) {
        rc = decode_one(arcs[i], outdir, passwd, test_mode, assume_yes, dumpf);
        if (rc != 0) failed = 1;
    }
    return failed;
}

