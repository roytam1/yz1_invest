#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yz1codec.h"

namespace yz1codec {

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

#define ALPHA_A 768
#define ALPHA_B 128
#define RING    256
#define MASK32  0xFFFFFFFFu

static void set_err(char * err, size_t err_size, const char * msg)
{
    if (err && err_size) { strncpy(err, msg, err_size - 1); err[err_size - 1] = 0; }
}

void derive_key(const char * password, unsigned char key[4])
{
    const u8 * b = (const u8 *)password;
    u32 n = (u32)strlen(password);
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
    {
        u32 v37 = (v101 + inc + v34 * v100) & MASK32;
        key[0] = (u8)(v37 >> 24); key[1] = (u8)(v37 >> 16);
        key[2] = (u8)(v37 >> 8);  key[3] = (u8)(v37 & 0xFF);
    }
}

unsigned char pw_chk(const char * password)
{
    const u8 * b = (const u8 *)password;
    u32 n = (u32)strlen(password), i, s = 0;
    for (i = 0; i < n; i++) s += (u32)(b[i] ^ (u8)i);
    return (u8)((s + n) & 0xFF);
}

bool parse_pub_password(const char * password, unsigned char key[4], unsigned char * chk)
{
    unsigned int b0, b1, b2, b3, b4;
    if (sscanf(password, "pub-%02x%02x-%02x%02x-%02x", &b0, &b1, &b2, &b3, &b4) != 5)
        return false;
    key[0] = (unsigned char)b0; key[1] = (unsigned char)b1;
    key[2] = (unsigned char)b2; key[3] = (unsigned char)b3;
    if (chk) *chk = (unsigned char)b4;
    return true;
}

void make_pub_password(const unsigned char key[4], unsigned char chk, char * out, size_t out_size)
{
    _snprintf(out, out_size, "pub-%02x%02x-%02x%02x-%02x",
              key[0], key[1], key[2], key[3], chk);
    out[out_size - 1] = 0;
}

bool read_header(const unsigned char * data, size_t size, Header * hdr)
{
    if (!data || size < 20 || memcmp(data, "yz01", 4) != 0) return false;
    hdr->flags = data[6];
    hdr->chk = data[7];
    hdr->all_size  = ((unsigned long)data[8] << 24) | ((unsigned long)data[9] << 16) |
                     ((unsigned long)data[10] << 8) | (unsigned long)data[11];
    hdr->head_size = ((unsigned long)data[12] << 24) | ((unsigned long)data[13] << 16) |
                     ((unsigned long)data[14] << 8) | (unsigned long)data[15];
    hdr->file_count= ((unsigned long)data[16] << 24) | ((unsigned long)data[17] << 16) |
                     ((unsigned long)data[18] << 8) | (unsigned long)data[19];
    return true;
}

/* ---------------- bit reader ---------------- */
struct BitRd { const u8 * d; u32 len, pos; unsigned __int64 acc; int n; };

static bool br_read(BitRd * b, int k, int * out)
{
    while (b->n < k) {
        if (b->pos >= b->len) return false;
        b->acc = (b->acc << 8) | b->d[b->pos++];
        b->n += 8;
    }
    b->n -= k;
    *out = (int)((b->acc >> b->n) & ((1u << k) - 1));
    return true;
}

/* ---------------- Huffman code builder ---------------- */
struct Node { u32 value; u16 symbol; u16 next; };
struct ValSym { u32 v; int sym; };

static int node_cmp(const void * pa, const void * pb)
{
    const Node * a = (const Node *)pa, * b = (const Node *)pb;
    if (a->value != b->value) return a->value < b->value ? -1 : 1;
    {
        u32 da = (u32)a->symbol | ((u32)a->next << 16);
        u32 db = (u32)b->symbol | ((u32)b->next << 16);
        return da < db ? -1 : (da > db ? 1 : 0);
    }
}

static int node_cmp_desc(const void * pa, const void * pb)
{
    const Node * a = (const Node *)pa, * b = (const Node *)pb;
    if (a->value != b->value) return a->value > b->value ? -1 : 1;
    {
        u32 da = (u32)a->symbol | ((u32)a->next << 16);
        u32 db = (u32)b->symbol | ((u32)b->next << 16);
        return da > db ? -1 : (da < db ? 1 : 0);
    }
}

static int valsym_cmp(const void * x, const void * y)
{
    u32 a = ((const ValSym *)x)->v, b = ((const ValSym *)y)->v;
    return a < b ? -1 : (a > b ? 1 : 0);
}

static void build_codes(const u16 * in_lengths, int N, u32 * vals, u16 * clens)
{
    Node * nodes = (Node *)malloc(sizeof(Node) * (size_t)N);
    int i, s;
    if (!nodes) return;
    for (s = 0; s < N; s++) {
        nodes[s].value = in_lengths[s]; nodes[s].symbol = (u16)s;
        nodes[s].next = 0xFFFF; vals[s] = 0; clens[s] = 0;
    }
    qsort(nodes, (size_t)N, sizeof(Node), node_cmp);
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
        {
            u16 sym = nodes[i + 1].symbol;
            clens[sym]++;
            vals[sym] = (vals[sym] >> 1) | 0x80000000u;
        }
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

static void huff_lens(const u32 * freq, int N, u16 * lens)
{
    Node * nodes = (Node *)malloc(sizeof(Node) * (size_t)N);
    int i;
    if (!nodes) return;
    for (i = 0; i < N; i++) {
        nodes[i].value = freq[i]; nodes[i].symbol = (u16)i;
        nodes[i].next = 0xFFFF; lens[i] = 0;
    }
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

static bool huff_build(const u32 * freq, int N, u32 * vals, u16 * lens)
{
    int i;
    u32 mx = 0;
    huff_lens(freq, N, lens);
    for (i = 0; i < N; i++) if (lens[i] > mx) mx = lens[i];
    if (mx >= 32) return false;
    build_codes(lens, N, vals, lens);
    return true;
}

/* ---------------- symbol decoder ---------------- */
struct SymDecoder { ValSym * a[32]; int cnt[32]; };

static bool sd_init(SymDecoder * sd, const u32 * vals, const u16 * lens, int N)
{
    int i, j; int off[32];
    memset(sd, 0, sizeof(*sd));
    for (i = 0; i < N; i++) {
        u16 L = lens[i];
        if (L == 0) continue;
        if (L > 31) return false;
        sd->cnt[L]++;
    }
    for (j = 1; j <= 31; j++) {
        sd->a[j] = (ValSym *)malloc(sizeof(ValSym) * (sd->cnt[j] ? (size_t)sd->cnt[j] : 1));
        if (!sd->a[j]) return false;
        off[j] = 0;
    }
    for (i = 0; i < N; i++) {
        u16 L = lens[i];
        if (!L) continue;
        sd->a[L][off[L]].v = vals[i] >> (32 - L);
        sd->a[L][off[L]].sym = i;
        off[L]++;
    }
    for (j = 1; j <= 31; j++) qsort(sd->a[j], (size_t)sd->cnt[j], sizeof(ValSym), valsym_cmp);
    return true;
}

static void sd_free(SymDecoder * sd)
{
    int j;
    for (j = 1; j <= 31; j++) { free(sd->a[j]); sd->a[j] = 0; }
}

static bool sd_read(BitRd * br, const SymDecoder * sd, int * sym)
{
    u32 code = 0; int nb = 1, bit;
    for (;;) {
        if (!br_read(br, 1, &bit)) return false;
        code = (code << 1) | (u32)bit;
        if (nb > 31) return false;
        {
            const ValSym * g = sd->a[nb]; int n = sd->cnt[nb];
            int lo = 0, hi = n - 1;
            while (lo <= hi) {
                int mid = (lo + hi) >> 1;
                u32 v = g[mid].v;
                if (v == code) { *sym = g[mid].sym; return true; }
                if (v < code) lo = mid + 1; else hi = mid - 1;
            }
        }
        nb++;
    }
}

/* ---------------- LZ decode ---------------- */
struct LzIn {
    u16 A[RING], B[RING];
    u32 * T1; u32 * T2;
    u8 * out; u32 outpos, h;
};

static void lz_in_reset(LzIn * st, u8 * out)
{
    memset(st->A, 0, sizeof(st->A));
    memset(st->B, 0, sizeof(st->B));
    memset(st->T1, 0, sizeof(u32) * RING * RING);
    memset(st->T2, 0, sizeof(u32) * RING * RING);
    st->out = out; st->outpos = 0; st->h = 0;
}

static bool lz_decompress(BitRd * br, const SymDecoder * sA, const SymDecoder * sB,
                          u32 need, LzIn * st, u32 cap)
{
    u32 limit = st->outpos + need;
    if (need > cap - st->outpos) return false;
    while (st->outpos < limit) {
        int sym;
        u32 src = 0, ml = 1;
        if (!sd_read(br, sA, &sym)) return false;
        if (sym >= 512) {
            st->out[st->outpos++] = (u8)(sym - 512);
        } else {
            u8 c;
            u16 a0;
            u32 slot;
            if (st->h >= st->outpos) return false;
            c = st->out[st->h];
            a0 = st->A[c];
            slot = ((u32)(sym & 0xFF) + a0) & 0xFF;
            if (sym >= 256) {
                ml = st->T2[c * RING + slot];
                src = st->T1[c * RING + slot];
                {
                    u32 v11 = st->T1[c * RING + a0];
                    if (v11) {
                        st->T1[c * RING + slot] = v11;
                        st->T2[c * RING + slot] = st->T2[c * RING + a0];
                    }
                }
            } else {
                int lc;
                if (!sd_read(br, sB, &lc)) return false;
                if (lc == 127) {
                    int e, v;
                    if (!br_read(br, 5, &e)) return false;
                    if (!br_read(br, e + 1, &v)) return false;
                    lc = v;
                }
                ml = (u32)(lc + 2);
                src = st->T1[c * RING + slot];
            }
            if (!ml || src >= st->outpos) return false;
            if (ml > cap - st->outpos) return false;
            {
                u32 i;
                for (i = 0; i < ml; i++) st->out[st->outpos + i] = st->out[src + i];
                st->outpos += ml;
            }
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
    return true;
}

static bool read_code_tables(BitRd * br, SymDecoder * sA, SymDecoder * sB,
                             char * err, size_t err_size)
{
    u32 valsA[ALPHA_A], valsB[ALPHA_B];
    u16 lensA[ALPHA_A], lensB[ALPHA_B];
    int i;

    for (i = 0; i < ALPHA_A; i++) {
        int v;
        if (!br_read(br, 5, &v)) { set_err(err, err_size, "truncated length table"); return false; }
        lensA[i] = (u16)v;
    }
    for (i = 0; i < ALPHA_B; i++) {
        int v;
        if (!br_read(br, 5, &v)) { set_err(err, err_size, "truncated length table"); return false; }
        lensB[i] = (u16)v;
    }
    build_codes(lensA, ALPHA_A, valsA, lensA);
    build_codes(lensB, ALPHA_B, valsB, lensB);
    if (!sd_init(sA, valsA, lensA, ALPHA_A) || !sd_init(sB, valsB, lensB, ALPHA_B)) {
        set_err(err, err_size, "out of memory");
        sd_free(sA); sd_free(sB);
        return false;
    }
    return true;
}

bool decompress_header(const unsigned char * body, size_t body_size,
                       size_t head_size,
                       unsigned char * out, char * err, size_t err_size)
{
    BitRd br;
    SymDecoder sA, sB;
    LzIn st;
    bool ok = false;

    memset(&sA, 0, sizeof(sA));
    memset(&sB, 0, sizeof(sB));
    st.T1 = 0; st.T2 = 0;

    br.d = body; br.len = (u32)body_size; br.pos = 0; br.acc = 0; br.n = 0;
    if (!read_code_tables(&br, &sA, &sB, err, err_size)) return false;
    st.T1 = (u32 *)calloc(RING * RING, sizeof(u32));
    st.T2 = (u32 *)calloc(RING * RING, sizeof(u32));
    if (!st.T1 || !st.T2) {
        set_err(err, err_size, "out of memory");
        goto done;
    }
    lz_in_reset(&st, out);
    if (!lz_decompress(&br, &sA, &sB, (u32)head_size, &st, (u32)head_size)) {
        set_err(err, err_size, "corrupted header stream");
        goto done;
    }
    ok = true;

done:
    free(st.T1); free(st.T2);
    sd_free(&sA); sd_free(&sB);
    return ok;
}

bool decompress(const unsigned char * body, size_t body_size,
                size_t head_size, size_t total_size,
                unsigned char * out, char * err, size_t err_size)
{
    BitRd br;
    SymDecoder sA, sB;
    LzIn st;
    bool ok = false;

    memset(&sA, 0, sizeof(sA));
    memset(&sB, 0, sizeof(sB));
    st.T1 = 0; st.T2 = 0;

    if (total_size < head_size) { set_err(err, err_size, "bad sizes"); return false; }

    br.d = body; br.len = (u32)body_size; br.pos = 0; br.acc = 0; br.n = 0;
    if (!read_code_tables(&br, &sA, &sB, err, err_size)) return false;
    st.T1 = (u32 *)calloc(RING * RING, sizeof(u32));
    st.T2 = (u32 *)calloc(RING * RING, sizeof(u32));
    if (!st.T1 || !st.T2) {
        set_err(err, err_size, "out of memory");
        goto done;
    }

    lz_in_reset(&st, out);
    if (!lz_decompress(&br, &sA, &sB, (u32)head_size, &st, (u32)head_size)) {
        set_err(err, err_size, "corrupted header stream");
        goto done;
    }
    lz_in_reset(&st, out + head_size);
    if (!lz_decompress(&br, &sA, &sB, (u32)(total_size - head_size), &st,
                       (u32)(total_size - head_size))) {
        set_err(err, err_size, "corrupted data stream");
        goto done;
    }
    ok = true;

done:
    free(st.T1); free(st.T2);
    sd_free(&sA); sd_free(&sB);
    return ok;
}

/* ---------------- bit writer ---------------- */
struct BW { u8 * buf; u32 cap, nbits; };

static bool bw_new(BW * b)
{
    b->cap = 1u << 16;
    b->buf = (u8 *)calloc(b->cap, 1);
    b->nbits = 0;
    return b->buf != 0;
}

static bool bw_bit(BW * b, int bit)
{
    u32 pos = b->nbits++;
    if ((pos >> 3) + 1 > b->cap) {
        u8 * nb;
        u32 ncap = b->cap;
        while (ncap < (pos >> 3) + 1) ncap <<= 1;
        nb = (u8 *)realloc(b->buf, ncap);
        if (!nb) return false;
        memset(nb + b->cap, 0, ncap - b->cap);
        b->buf = nb; b->cap = ncap;
    }
    if (bit) b->buf[pos >> 3] |= (u8)(0x80 >> (pos & 7));
    return true;
}

static bool bw_bits(BW * b, u32 v, int n)
{
    int k;
    for (k = n - 1; k >= 0; k--) if (!bw_bit(b, (v >> k) & 1)) return false;
    return true;
}

/* ---------------- LZ encode ---------------- */
struct Tok { u16 asy; u32 mclass; u8 ismatch; };

struct LZ {
    const u8 * data;
    u32 p, h;
    u16 A[RING], B[RING];
    u16 * heads; u16 * nxt; u16 * prv;
    u32 * T1v; u16 * T2v;
};

static void lz_reset(LZ * s)
{
    memset(s->A, 0, sizeof(s->A));
    memset(s->B, 0, sizeof(s->B));
    memset(s->heads, 0xFF, sizeof(u16) * RING * RING);
    memset(s->nxt, 0xFF, sizeof(u16) * RING * RING);
    memset(s->prv, 0xFF, sizeof(u16) * RING * RING);
    memset(s->T1v, 0, sizeof(u32) * RING * RING);
    memset(s->T2v, 0, sizeof(u16) * RING * RING);
    s->h = 0;
}

static void chain_unlink(LZ * s, u32 row, u16 a)
{
    u16 n = s->nxt[row | a], v = s->prv[row | a];
    if (n != 0xFFFF) s->prv[row | n] = v;
    if (v == 0xFFFF) {
        u32 off = s->T1v[row | a];
        if (off) s->heads[((row >> 8) << 8) | s->data[off]] = n;
    } else {
        s->nxt[row | v] = n;
    }
}

static void chain_link(LZ * s, u32 row, u16 a)
{
    u32 c = row >> 8;
    u16 b = s->heads[(c << 8) | s->data[s->T1v[row | a]]];
    if (b != 0xFFFF) s->prv[row | b] = a;
    s->nxt[row | a] = b;
    s->prv[row | a] = 0xFFFF;
    s->heads[(c << 8) | s->data[s->T1v[row | a]]] = a;
}

static bool lz_pass(LZ * s, const u8 * data, u32 len, u32 * freqA, u32 * freqB,
                    Tok ** toks, u32 * ntok, u32 * cap)
{
    u32 p = 0;
    s->data = data;
    lz_reset(s);
    if (len == 0) return true;
    for (;;) {
        u32 ml = 0, best = 0, chunklen;
        Tok * tk;
        if (*ntok == *cap) {
            Tok * nt;
            *cap = *cap * 2 + 256;
            nt = (Tok *)realloc(*toks, sizeof(Tok) * (size_t)*cap);
            if (!nt) return false;
            *toks = nt;
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
                        int good = 1;
                        for (k = ml; k > 0; ) {
                            k--;
                            if (data[src + k] != data[p + k]) { good = 0; break; }
                        }
                        if (good) {
                            j = ml + 1;
                            while (j < rem && data[src + j] == data[p + j]) j++;
                            best = i; ml = j;
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
                    tk->mclass = 0;
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
                tk->mclass = 0;
                freqA[tk->asy]++;
                chunklen = ml;
                p += 1;
            }
        } else {
            tk->asy = (u16)(512 + data[p]);
            tk->ismatch = 0;
            tk->mclass = 0;
            freqA[tk->asy]++;
            chunklen = 0;
            p += 1;
        }

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
        if (p >= len) break;
    }
    return true;
}

bool compress(const unsigned char * table, size_t table_size,
              const unsigned long * file_sizes, size_t file_count,
              const unsigned char * payload, size_t payload_size,
              unsigned char ** body, size_t * body_size,
              char * err, size_t err_size)
{
    u32 freqA[ALPHA_A], freqB[ALPHA_B];
    u32 valsA[ALPHA_A], valsB[ALPHA_B];
    u16 lensA[ALPHA_A], lensB[ALPHA_B];
    Tok * t1 = 0, * t2 = 0;
    u32 nt1 = 0, nt2 = 0, cap1 = 0, cap2 = 0;
    LZ * lz = 0;
    BW bw;
    bool ok = false;
    u32 i;
    size_t fi, off;

    *body = 0; *body_size = 0;
    memset(freqA, 0, sizeof(freqA));
    memset(freqB, 0, sizeof(freqB));
    memset(&bw, 0, sizeof(bw));

    lz = (LZ *)calloc(1, sizeof(LZ));
    if (!lz) { set_err(err, err_size, "out of memory"); return false; }
    lz->heads = (u16 *)malloc(sizeof(u16) * RING * RING);
    lz->nxt   = (u16 *)malloc(sizeof(u16) * RING * RING);
    lz->prv   = (u16 *)malloc(sizeof(u16) * RING * RING);
    lz->T1v   = (u32 *)malloc(sizeof(u32) * RING * RING);
    lz->T2v   = (u16 *)malloc(sizeof(u16) * RING * RING);
    if (!lz->heads || !lz->nxt || !lz->prv || !lz->T1v || !lz->T2v) {
        set_err(err, err_size, "out of memory");
        goto done;
    }

    if (!lz_pass(lz, table, (u32)table_size, freqA, freqB, &t1, &nt1, &cap1)) {
        set_err(err, err_size, "compress pass 1 failed");
        goto done;
    }
    off = 0;
    if (file_count) {
        if (!file_sizes) { set_err(err, err_size, "bad file table"); goto done; }
        for (fi = 0; fi < file_count; fi++) {
            u32 sz = (u32)file_sizes[fi];
            if (off > payload_size || (size_t)sz > payload_size - off) {
                set_err(err, err_size, "bad file table (size exceeds data)");
                goto done;
            }
            if (!lz_pass(lz, payload + off, sz, freqA, freqB, &t2, &nt2, &cap2)) {
                set_err(err, err_size, "compress pass 2 failed");
                goto done;
            }
            off += (size_t)sz;
        }
    } else {
        if (!lz_pass(lz, payload, (u32)payload_size, freqA, freqB, &t2, &nt2, &cap2)) {
            set_err(err, err_size, "compress pass 2 failed");
            goto done;
        }
    }
    if (!huff_build(freqA, ALPHA_A, valsA, lensA) ||
        !huff_build(freqB, ALPHA_B, valsB, lensB)) {
        set_err(err, err_size, "code length overflow");
        goto done;
    }
    if (!bw_new(&bw)) { set_err(err, err_size, "out of memory"); goto done; }

    for (i = 0; i < ALPHA_A; i++) if (!bw_bits(&bw, lensA[i], 5)) goto oom;
    for (i = 0; i < ALPHA_B; i++) if (!bw_bits(&bw, lensB[i], 5)) goto oom;
    {
        int pass;
        for (pass = 0; pass < 2; pass++) {
            Tok * tv = pass ? t2 : t1;
            u32 nv = pass ? nt2 : nt1;
            u32 j;
            for (j = 0; j < nv; j++) {
                u16 sym = tv[j].asy;
                u32 L = lensA[sym];
                if (!bw_bits(&bw, L ? (valsA[sym] >> (32 - L)) : 0, (int)L)) goto oom;
                if (tv[j].ismatch) {
                    u32 cls = tv[j].mclass;
                    if (cls < 127) {
                        u32 LB = lensB[cls];
                        if (!bw_bits(&bw, LB ? (valsB[cls] >> (32 - LB)) : 0, (int)LB)) goto oom;
                    } else {
                        u32 LB = lensB[127];
                        u32 e = 31, x = cls;
                        if (!bw_bits(&bw, LB ? (valsB[127] >> (32 - LB)) : 0, (int)LB)) goto oom;
                        while (!(x & 0x80000000u)) { x <<= 1; e--; }
                        if (!bw_bits(&bw, e, 5)) goto oom;
                        if (!bw_bits(&bw, cls, (int)e + 1)) goto oom;
                    }
                }
            }
        }
    }

    *body_size = (bw.nbits + 7) >> 3;
    *body = bw.buf;
    bw.buf = 0;
    ok = true;
    goto done;

oom:
    set_err(err, err_size, "out of memory");

done:
    if (bw.buf) free(bw.buf);
    free(t1); free(t2);
    if (lz) {
        free(lz->heads); free(lz->nxt); free(lz->prv);
        free(lz->T1v); free(lz->T2v);
        free(lz);
    }
    return ok;
}

}
