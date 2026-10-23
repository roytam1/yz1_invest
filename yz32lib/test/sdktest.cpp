#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yz32lib.h"

class CEV : public yzFileEv
{
public:
    bool yes;
    const char * pw;
    char inbuf[512];

    CEV() : yes(true), pw(0) { inbuf[0] = 0; }

    void MessEX(const char * s, const int & no)
    {
        printf("[EX %d] %s\n", no, s);
    }
    void MessINF(const char * s, const int & no)
    {
        printf("[INF %d] %s\n", no, s);
    }
    int MessYN(const char * s, const int & no)
    {
        printf("[YN %d] %s\n", no, s);
        return yes ? 0 : 1;
    }
    char * MessINPUT(const char * s, const int & no)
    {
        printf("[INPUT %d] %s\n", no, s);
        if (pw) {
            strncpy(inbuf, pw, sizeof(inbuf) - 1);
            inbuf[sizeof(inbuf) - 1] = 0;
            return inbuf;
        }
        if (!fgets(inbuf, sizeof(inbuf), stdin)) return 0;
        {
            size_t l = strlen(inbuf);
            while (l && (inbuf[l - 1] == '\n' || inbuf[l - 1] == '\r')) inbuf[--l] = 0;
        }
        return inbuf;
    }
    int gauge_EncodeInit(const char * in, FileName & out, FileTbl & fp, FileTbl & fl)
    {
        printf("[enc-init] %s -> %s (%d files)\n", in, out.c_str(), (int)fl.size());
        return 1;
    }
    int gauge_DecodeInit(const char * in, const yzDecodeHeader & h)
    {
        yzDecodeHeader & hh = const_cast<yzDecodeHeader &>(h);
        vector<yzFileStat> v = hh.filestat_Get();
        printf("[dec-init] %s (%d files)\n", in, (int)v.size());
        return 1;
    }
    int gauge_String(const char * s) { return 1; }
    int gauge_Value(const int & per, const int & et) { return 1; }
    int gauge_WriteInfo(const yzFileStat & st, const int & no, const int & max)
    {
        printf("[file %d/%d] %s (%ld bytes)\n", no + 1, max, st.m_file_name.c_str(), st.m_file_size);
        return 0;
    }
};

static int usage()
{
    printf("usage:\n");
    printf("  sdktest dec <archive.yz1> <outdir> [password]\n");
    printf("  sdktest hdr <archive.yz1> [password]\n");
    printf("  sdktest enc <file|dir> <outdir> [password] [self_exe]\n");
    printf("  sdktest encpub <file|dir> <outdir>\n");
    return 1;
}

int main(int argc, char ** argv)
{
    if (argc < 2) return usage();
    if (strcmp(argv[1], "hdr") == 0) {
        CEV ev;
        if (argc < 3) return usage();
        ev.pw = (argc > 3) ? argv[3] : 0;
        {
            yzFileDecode d(&ev, ".");
            int n = d.YzFile_DecodeHeader(argv[2]);
            if (n < 0) return 1;
            d.Print(argv[2]);
            return 0;
        }
    }
    if (strcmp(argv[1], "dec") == 0) {
        CEV ev;
        if (argc < 4) return usage();
        ev.pw = (argc > 4) ? argv[4] : 0;
        {
            yzFileDecode d(&ev, argv[3]);
            return d.DecodeFile(argv[2], 0) < 0 ? 1 : 0;
        }
    }
    if (strcmp(argv[1], "enc") == 0 || strcmp(argv[1], "encpub") == 0) {
        CEV ev;
        YZ_MAIN_ENCODE_PASSWD_MODE mode = YZ_MAIN_ENCODE_PASSWD_OFF;
        const char * sfx = 0;
        if (argc < 4) return usage();
        if (strcmp(argv[1], "encpub") == 0) {
            mode = YZ_MAIN_ENCODE_PASSWD_PUB;
        } else if (argc > 4 && argv[4][0]) {
            ev.pw = argv[4];
            mode = YZ_MAIN_ENCODE_PASSWD_ON;
        }
        if (strcmp(argv[1], "enc") == 0 && argc > 5) sfx = argv[5];
        {
            yzFileEncode e(&ev, argv[3], sfx, mode);
            return e.EncodeFile(argv[2]) < 0 ? 1 : 0;
        }
    }
    return usage();
}
