/* compare build_codes vs Python dump: cases = 4 archives.
 * layout per case: [N u16 lens][N u32 vals][N u32 lens_after] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned short u16; typedef unsigned int u32;
#define ALPHA_A 768
#define ALPHA_B 128

typedef struct { u32 value; u16 symbol; u16 next; } Node;
static int node_cmp(const void *pa, const void *pb) {
    const Node *a = (const Node *)pa, *b = (const Node *)pb;
    if (a->value != b->value) return a->value < b->value ? -1 : 1;
    { u32 da = (u32)a->symbol | ((u32)a->next << 16);
      u32 db = (u32)b->symbol | ((u32)b->next << 16);
      return da < db ? -1 : (da > db ? 1 : 0); }
}

static void build_codes(const u16 *in_lengths, int N, u32 *vals, u32 *clens) {
    Node *nodes = (Node *)malloc(sizeof(Node) * (size_t)N);
    int i, s;
    for (s = 0; s < N; s++) { nodes[s].value = in_lengths[s]; nodes[s].symbol = (u16)s; nodes[s].next = 0xFFFF; vals[s] = 0; clens[s] = 0; }
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

int main(void) {
    FILE *f = fopen("hc.bin", "rb");
    int j, bad = 0; u32 vv[768], ll[768], ev[768], el[768]; u16 lens[768];
    int N;
    for (j = 0; j < 4; j++) {
        N = (j % 2 == 0) ? ALPHA_A : ALPHA_B;
        { int i;
          for (i = 0; i < N; i++) { u16 x; if (fread(&x, 2, 1, f) != 1) { printf("read err\n"); return 1; } lens[i] = x; }
          for (i = 0; i < N; i++) { if (fread(&ev[i], 4, 1, f) != 1) { printf("read err\n"); return 1; } }
          for (i = 0; i < N; i++) { if (fread(&el[i], 4, 1, f) != 1) { printf("read err\n"); return 1; } }
          build_codes(lens, N, vv, ll);
          if (memcmp(vv, ev, sizeof(u32) * (size_t)N) != 0 || memcmp(ll, el, sizeof(u32) * (size_t)N) != 0) {
              printf("CASE %d MISMATCH\n", j);
              for (i = 0; i < N; i++) {
                  if (vv[i] != ev[i] || ll[i] != el[i]) {
                      if (bad < 10) printf("  sym %d: got v=%08X l=%u want v=%08X l=%u\n", i, vv[i], ll[i], ev[i], el[i]);
                      bad++;
                  }
              }
          } else {
              printf("CASE %d ok (%d syms)\n", j, N);
          }
        }
    }
    fclose(f);
    printf("mismatches: %d\n", bad);
    return bad != 0;
}
