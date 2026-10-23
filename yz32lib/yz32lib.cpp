#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/utime.h>
#include <io.h>
#include <direct.h>

#include "yz32lib.h"
#include "yz1codec.h"

/* internal helpers referenced by the header's forward declarations */
class MyBitGetEv { public: std::string body; };
class MemAlloc   { public: std::string table; };
class yzDecode   { public: std::string payload; };

typedef unsigned char  u8;
typedef unsigned int   u32;

YzMessage::language_type YzMessage::language = YzMessage::language_english;

static u32 be32(const unsigned char * p)
{
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | (u32)p[3];
}

static void put_be32(unsigned char * p, u32 v)
{
    p[0] = (u8)(v >> 24); p[1] = (u8)(v >> 16); p[2] = (u8)(v >> 8); p[3] = (u8)v;
}

static void evEX(yzFileEv * ev, const char * fmt, int no, ...)
{
    char buf[2048]; va_list ap;
    if (!ev) return;
    va_start(ap, no);
    _vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1] = 0;
    ev->MessEX(buf, no);
}

static void evINF(yzFileEv * ev, const char * fmt, int no, ...)
{
    char buf[2048]; va_list ap;
    if (!ev) return;
    va_start(ap, no);
    _vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1] = 0;
    ev->MessINF(buf, no);
}

static int evYN(yzFileEv * ev, const char * fmt, int no, ...)
{
    char buf[2048]; va_list ap;
    if (!ev) return 0;
    va_start(ap, no);
    _vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1] = 0;
    return ev->MessYN(buf, no);
}

static std::string top_component(const char * arg)
{
    const char * p, * top = arg;
    for (p = arg; *p; p++) if (*p == '\\') top = p + 1;
    return std::string(top);
}

static std::string join_path(const char * dir, const std::string & name)
{
    std::string s;
    if (dir && *dir) { s = dir; if (s[s.size() - 1] != '\\' && s[s.size() - 1] != '/') s += '\\'; }
    s += name;
    return s;
}

static std::string normalize_path(std::string s)
{
    std::string::size_type i;
    for (i = 0; i < s.size(); i++) if (s[i] == '/') s[i] = '\\';
    return s;
}

static std::string swap_ext(const std::string & name, const char * ext)
{
    std::string::size_type dot = name.find_last_of('.');
    std::string::size_type sep = name.find_last_of("\\/");
    if (dot == std::string::npos || (sep != std::string::npos && dot < sep))
        return name + ext;
    return name.substr(0, dot) + ext;
}

static int make_parents(const std::string & path)
{
    std::string tmp = normalize_path(path);
    std::string::size_type i;
    for (i = 1; i < tmp.size(); i++) {
        if (tmp[i] == '\\') {
            std::string part = tmp.substr(0, i);
            if (part.size() == 2 && part[1] == ':') continue;
            if (_mkdir(part.c_str()) != 0 && errno != EEXIST) {
                struct _stat st;
                if (_stat(part.c_str(), &st) != 0 || !(st.st_mode & _S_IFDIR)) return -1;
            }
        }
    }
    return 0;
}

static bool read_all(const char * path, std::string & out)
{
    FILE * f = fopen(path, "rb");
    long n;
    if (!f) return false;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
    n = ftell(f);
    if (n < 0) { fclose(f); return false; }
    rewind(f);
    out.resize((std::string::size_type)n);
    if (n && fread(&out[0], 1, (size_t)n, f) != (size_t)n) { fclose(f); return false; }
    fclose(f);
    return true;
}

static bool file_exists(const std::string & path)
{
    struct _stat st;
    return _stat(path.c_str(), &st) == 0;
}

static void set_mtime(const std::string & path, time_t t)
{
    struct _utimbuf ut;
    ut.actime = t; ut.modtime = t;
    _utime(path.c_str(), &ut);
}

static void sec_to_str(clock_t t0, char * buf, size_t n)
{
    double sec = (double)(clock() - t0) / (double)CLOCKS_PER_SEC;
    _snprintf(buf, n, "%.2f", sec);
    buf[n - 1] = 0;
}

/* ==================================================================== */
/* yzDecHead                                                            */
/* ==================================================================== */

yzDecHead::yzDecHead(
    const unsigned char * in_area,
    const long &          in_file_cnt,
    yzFileEv *            yzmev,
    const long &          in_size,
    bool *                success)
    : m_file_count(0), m_file_size_tbl(0), m_file_mtime_tbl(0),
      m_file_name_max_length(0), m_file_size_max(0), yzm(yzmev)
{
    long i;
    if (success) *success = false;
    if (in_file_cnt < 0 || !in_area) return;
    if (in_size < in_file_cnt * 8) return;

    {
        long off = in_file_cnt * 8;
        for (i = 0; i < in_file_cnt; i++) {
            size_t l = 0;
            while (off + (long)l < in_size && in_area[off + l]) l++;
            if (off + (long)l >= in_size) return;
            off += (long)l + 1;
        }
    }

    m_file_count = in_file_cnt;
    m_file_size_tbl = new long[in_file_cnt ? in_file_cnt : 1];
    m_file_mtime_tbl = new YZtime_t[in_file_cnt ? in_file_cnt : 1];

    for (i = 0; i < in_file_cnt; i++) {
        long sz = (long)be32(in_area + 4 * i);
        long mt = (long)be32(in_area + 4 * in_file_cnt + 4 * i);
        m_file_size_tbl[i] = sz;
        m_file_mtime_tbl[i] = (YZtime_t)mt;
        if (sz > m_file_size_max) m_file_size_max = sz;
    }
    {
        long off = in_file_cnt * 8;
        for (i = 0; i < in_file_cnt; i++) {
            FileName name((const char *)(in_area + off));
            if ((int)name.size() > m_file_name_max_length) m_file_name_max_length = (int)name.size();
            m_file_name_tbl.push_back(name);
            off += (long)name.size() + 1;
        }
    }
    if (success) *success = true;
}

int yzDecHead::MemChk(const char * in_dir_name, const int & in_n, const unsigned char * in_mem)
{
    std::string path, data;
    struct _stat st;
    if (in_n < 0 || in_n >= (int)m_file_count || !in_mem) return 0;
    path = normalize_path(join_path(in_dir_name, m_file_name_tbl[in_n]));

    if (_stat(path.c_str(), &st) != 0) {
        if (evYN(yzm, YZ_MSG_TBL_70, YZ_MSG_NO_70, m_file_name_tbl[in_n].c_str()) == 1)
            return -1;
        return 0;
    }
    if ((long)st.st_size != m_file_size_tbl[in_n]) {
        char a[32], b[32];
        _snprintf(a, sizeof(a), "%ld", (long)st.st_size);
        _snprintf(b, sizeof(b), "%ld", m_file_size_tbl[in_n]);
        if (evYN(yzm, YZ_MSG_TBL_71, YZ_MSG_NO_71, m_file_name_tbl[in_n].c_str(), a, b) == 1)
            return -1;
        return 0;
    }
    if (!read_all(path.c_str(), data)) {
        if (evYN(yzm, YZ_MSG_TBL_70, YZ_MSG_NO_70, m_file_name_tbl[in_n].c_str()) == 1)
            return -1;
        return 0;
    }
    {
        size_t k;
        for (k = 0; k < data.size(); k++) {
            if ((unsigned char)data[k] != in_mem[k]) {
                char a[32];
                _snprintf(a, sizeof(a), "%ld", (long)k);
                if (evYN(yzm, YZ_MSG_TBL_76, YZ_MSG_NO_76,
                         m_file_name_tbl[in_n].c_str(), a, "") == 1)
                    return -1;
                return 0;
            }
        }
    }
    return 1;
}

void yzDecHead::Print()
{
    long i;
    for (i = 0; i < m_file_count; i++) {
        time_t t = (time_t)m_file_mtime_tbl[i];
        char * ts = ctime(&t);
        if (!ts) ts = "\n";
        printf("%s\t%ld\t%s", m_file_name_tbl[i].c_str(), (long)m_file_size_tbl[i], ts);
    }
}

int yzDecHead::FileWrite(const char * in_dir_name, const int & in_n, const unsigned char * in_data)
{
    std::string path;
    FILE * f;
    if (in_n < 0 || in_n >= (int)m_file_count) return 1;
    path = normalize_path(join_path(in_dir_name, m_file_name_tbl[in_n]));
    if (make_parents(path) != 0) {
        evEX(yzm, YZ_MSG_TBL_77, YZ_MSG_NO_77, in_dir_name ? in_dir_name : "");
        return 1;
    }
    if (file_exists(path)) {
        if (evYN(yzm, YZ_MSG_TBL_27, YZ_MSG_NO_27, m_file_name_tbl[in_n].c_str()) == 1) {
            evINF(yzm, YZ_MSG_TBL_29, YZ_MSG_NO_29);
            return 0;
        }
    }
    f = fopen(path.c_str(), "wb");
    if (!f) {
        evEX(yzm, YZ_MSG_TBL_43, YZ_MSG_NO_43, m_file_name_tbl[in_n].c_str());
        return 1;
    }
    if (m_file_size_tbl[in_n] && fwrite(in_data, 1, (size_t)m_file_size_tbl[in_n], f) != (size_t)m_file_size_tbl[in_n]) {
        evEX(yzm, YZ_MSG_TBL_55, YZ_MSG_NO_55, m_file_name_tbl[in_n].c_str());
        fclose(f);
        return 1;
    }
    fclose(f);
    {
        struct _stat st;
        if (_stat(path.c_str(), &st) == 0 && (time_t)st.st_mtime != (time_t)m_file_mtime_tbl[in_n]) {
            set_mtime(path, (time_t)m_file_mtime_tbl[in_n]);
        }
    }
    return 0;
}

/* ==================================================================== */
/* yzFileStat                                                           */
/* ==================================================================== */

yzFileStat::yzFileStat(string in_file_name, long in_file_size, time_t in_file_mtime)
    : m_file_name(in_file_name), m_file_size(in_file_size), m_file_mtime(in_file_mtime)
{
}

/* ==================================================================== */
/* yzDecodeHeader                                                       */
/* ==================================================================== */

yzDecodeHeader::yzDecodeHeader(yzDecHead & in_yz_dec_head)
    : m_yz_dec_head(in_yz_dec_head)
{
}

vector<yzFileStat> yzDecodeHeader::filestat_Get()
{
    vector<yzFileStat> v;
    long i;
    for (i = 0; i < m_yz_dec_head.m_file_count; i++) {
        v.push_back(yzFileStat(m_yz_dec_head.m_file_name_tbl[i],
                               m_yz_dec_head.m_file_size_tbl[i],
                               (time_t)m_yz_dec_head.m_file_mtime_tbl[i]));
    }
    return v;
}

/* ==================================================================== */
/* yzFileDecode                                                         */
/* ==================================================================== */

yzFileDecode::yzFileDecode(yzFileEv * in_yzm, const char * in_dir_name)
    : yzfiledecode_output_dir_name(0), yzfiledecode_p_yz_dec_head(0),
      yzm(in_yzm), p_bit_get(0), p_head(0), p_yz_decode(0),
      dec_all_size(0), dec_head_size(0), dec_file_count(0), dec_data_size(0)
{
    if (in_dir_name) {
        yzfiledecode_output_dir_name = _strdup(in_dir_name);
    }
}

yzFileDecode::~yzFileDecode()
{
    if (yzfiledecode_p_yz_dec_head) {
        delete[] yzfiledecode_p_yz_dec_head->m_file_size_tbl;
        delete[] yzfiledecode_p_yz_dec_head->m_file_mtime_tbl;
        delete yzfiledecode_p_yz_dec_head;
        yzfiledecode_p_yz_dec_head = 0;
    }
    delete p_bit_get;
    delete p_head;
    delete p_yz_decode;
    p_bit_get = 0; p_head = 0; p_yz_decode = 0;
    if (yzfiledecode_output_dir_name) { free(yzfiledecode_output_dir_name); yzfiledecode_output_dir_name = 0; }
}

int yzFileDecode::YzFile_DecodeHeader(const char * in_file_name)
{
    std::string raw, body;
    yz1codec::Header hdr;
    size_t start = 0;
    std::string out;
    char err[256];

    if (yzfiledecode_p_yz_dec_head) {
        delete[] yzfiledecode_p_yz_dec_head->m_file_size_tbl;
        delete[] yzfiledecode_p_yz_dec_head->m_file_mtime_tbl;
        delete yzfiledecode_p_yz_dec_head;
        yzfiledecode_p_yz_dec_head = 0;
    }
    delete p_bit_get; delete p_head; delete p_yz_decode;
    p_bit_get = 0; p_head = 0; p_yz_decode = 0;

    if (!read_all(in_file_name, raw)) {
        evEX(yzm, YZ_MSG_TBL_20, YZ_MSG_NO_20, in_file_name);
        return -1;
    }
    if (raw.size() >= 24 && memcmp(raw.data(), "yz01", 4) != 0) {
        u32 tot = be32((const unsigned char *)raw.data() + raw.size() - 4);
        if (tot && tot + 4 <= raw.size()) start = raw.size() - 4 - tot;
    }
    if (!yz1codec::read_header((const unsigned char *)raw.data() + start,
                               raw.size() - start, &hdr) || hdr.all_size > 0x10000000u) {
        evEX(yzm, YZ_MSG_TBL_22, YZ_MSG_NO_22, in_file_name);
        return -1;
    }

    body.assign(raw.data() + start + 20, raw.size() - start - 20);

    if (hdr.encrypted()) {
        unsigned char key[4], chk = 0;
        char * pw = 0;
        do {
            pw = yzm ? yzm->MessINPUT(YZ_MSG_TBL_23, YZ_MSG_NO_23) : 0;
            if (!pw || !*pw) {
                evINF(yzm, YZ_MSG_TBL_24, YZ_MSG_NO_24);
                return -1;
            }
            if (yz1codec::parse_pub_password(pw, key, &chk)) { }
            else {
                yz1codec::derive_key(pw, key);
                chk = yz1codec::pw_chk(pw);
            }
            if (chk != hdr.chk) {
                evINF(yzm, YZ_MSG_TBL_25, YZ_MSG_NO_25);
                return -1;
            }
            break;
        } while (1);
        {
            size_t i;
            for (i = 0; i < body.size(); i++) body[i] ^= (char)key[i & 3];
        }
    }

    dec_all_size = hdr.all_size;
    dec_head_size = hdr.head_size;
    dec_file_count = hdr.file_count;
    dec_data_size = hdr.all_size - hdr.head_size;

    out.resize((std::string::size_type)hdr.all_size ? (std::string::size_type)hdr.all_size : 1);
    if (!yz1codec::decompress((const unsigned char *)body.data(), body.size(),
                              (size_t)hdr.head_size, (size_t)hdr.all_size,
                              (unsigned char *)&out[0], err, sizeof(err))) {
        evEX(yzm, YZ_MSG_TBL_92, YZ_MSG_NO_92);
        return -1;
    }

    p_bit_get = new MyBitGetEv;
    p_bit_get->body = body;
    p_head = new MemAlloc;
    p_head->table.assign(out.data(), (size_t)hdr.head_size);
    p_yz_decode = new yzDecode;
    p_yz_decode->payload.assign(out.data() + hdr.head_size, (size_t)(hdr.all_size - hdr.head_size));

    {
        bool ok = false;
        yzfiledecode_p_yz_dec_head = new yzDecHead(
            (const unsigned char *)p_head->table.data(),
            (long)hdr.file_count, yzm, (long)hdr.head_size, &ok);
        if (!ok) {
            evEX(yzm, YZ_MSG_TBL_92, YZ_MSG_NO_92);
            delete yzfiledecode_p_yz_dec_head;
            yzfiledecode_p_yz_dec_head = 0;
            return -1;
        }
    }
    if (yzm) {
        yzDecodeHeader yzh(*yzfiledecode_p_yz_dec_head);
        yzm->gauge_DecodeInit(in_file_name, yzh);
    }
    return (int)hdr.file_count;
}

int yzFileDecode::Print(const char * fileName)
{
    if (!yzfiledecode_p_yz_dec_head) {
        if (YzFile_DecodeHeader(fileName) < 0) return -1;
    }
    yzfiledecode_p_yz_dec_head->Print();
    return 0;
}

int yzFileDecode::DecodeFile(const char * fileName, const int & in_decode_mode)
{
    clock_t t0 = clock();
    long i;
    size_t off = 0;
    const char * dir = yzfiledecode_output_dir_name;
    char sec[32];

    if (!yzfiledecode_p_yz_dec_head) {
        if (YzFile_DecodeHeader(fileName) < 0) return -1;
    }
    if (!p_yz_decode) return -1;

    for (i = 0; i < yzfiledecode_p_yz_dec_head->m_file_count; i++) {
        yzDecHead * h = yzfiledecode_p_yz_dec_head;
        yzFileStat st(h->m_file_name_tbl[i], h->m_file_size_tbl[i],
                      (time_t)h->m_file_mtime_tbl[i]);
        int act = yzm ? yzm->gauge_WriteInfo(st, (int)i, (int)h->m_file_count) : 0;
        if (act == 2) return -1;
        if (act == 1) { off += (size_t)h->m_file_size_tbl[i]; continue; }
        if (in_decode_mode == 0) {
            if (h->FileWrite(dir, (int)i,
                             (const unsigned char *)p_yz_decode->payload.data() + off) != 0)
                return -1;
        }
        off += (size_t)h->m_file_size_tbl[i];
        if (yzm && h->m_file_count) {
            char ss[64];
            _snprintf(ss, sizeof(ss), "%s", h->m_file_name_tbl[i].c_str());
            yzm->gauge_String(ss);
            yzm->gauge_Value((int)((i + 1) * 1000 / h->m_file_count), 0);
        }
    }
    sec_to_str(t0, sec, sizeof(sec));
    evINF(yzm, YZ_MSG_TBL_38, YZ_MSG_NO_38, sec);
    return 0;
}

/* ==================================================================== */
/* yzFileEncode                                                         */
/* ==================================================================== */

namespace {

struct SEnt
{
    std::string fs;
    std::string name;
    long size;
    time_t mtime;
};

static void gather_dir(const std::string & fspath, const std::string & prefix,
                       std::vector<SEnt> & ents)
{
    std::string mask = fspath + "\\*";
    long h = -1;
    struct _finddata_t fd;
    h = _findfirst(mask.c_str(), &fd);
    if (h == -1) return;
    do {
        if (fd.name[0] == '.') continue;
        {
            SEnt e;
            e.fs = fspath + "\\" + fd.name;
            e.name = prefix + "\\" + fd.name;
            if (fd.attrib & _A_SUBDIR) {
                gather_dir(e.fs, e.name, ents);
            } else {
                struct _stat st;
                if (_stat(e.fs.c_str(), &st) != 0) continue;
                e.size = (long)st.st_size;
                e.mtime = (time_t)st.st_mtime;
                ents.push_back(e);
            }
        }
    } while (_findnext(h, &fd) == 0);
    _findclose(h);
}

}

yzFileEncode::yzFileEncode(
    yzFileEv *    in_yzm,
    const char *  in_dir_name,
    const char *  in_self_encode_file,
    const enum YZ_MAIN_ENCODE_PASSWD_MODE in_passwd_mode)
    : yzm(in_yzm), dirName(in_dir_name), SelfEncodeFile(in_self_encode_file),
      passwd_mode(in_passwd_mode)
{
}

int yzFileEncode::EncodeFile(const char * in_file_name)
{
    std::vector<SEnt> ents;
    std::string top, output, pw, table, payload;
    unsigned char key[4], chk = 0;
    bool enc = false;
    struct _stat st;
    size_t Bt, Atot, i;
    unsigned char * body = 0;
    size_t blen = 0;
    char err[256];
    clock_t t0 = clock();
    char sec[32];

    if (_stat(in_file_name, &st) != 0) {
        evEX(yzm, YZ_MSG_TBL_91, YZ_MSG_NO_91, in_file_name);
        return -1;
    }
    top = top_component(in_file_name);
    if (st.st_mode & _S_IFDIR) {
        gather_dir(in_file_name, top, ents);
    } else {
        SEnt e;
        e.fs = in_file_name;
        e.name = top;
        e.size = (long)st.st_size;
        e.mtime = (time_t)st.st_mtime;
        ents.push_back(e);
    }
    if (ents.empty()) {
        evEX(yzm, YZ_MSG_TBL_91, YZ_MSG_NO_91, in_file_name);
        return -1;
    }

    output = swap_ext(top, ".yz1");
    if (dirName) output = join_path(dirName, output);
    output = normalize_path(output);

    if (yzm) {
        FileName outname(output.c_str());
        FileTbl fullpaths, names;
        for (i = 0; i < ents.size(); i++) {
            fullpaths.push_back(ents[i].fs);
            names.push_back(ents[i].name);
        }
        if (yzm->gauge_EncodeInit(in_file_name, outname, fullpaths, names) == 0)
            return -1;
        output = outname;
        if (fullpaths.size() == names.size()) {
            std::vector<SEnt> ne;
            for (i = 0; i < fullpaths.size(); i++) {
                struct _stat s2;
                if (_stat(fullpaths[i].c_str(), &s2) != 0) continue;
                SEnt e;
                e.fs = fullpaths[i];
                e.name = names[i];
                e.size = (long)s2.st_size;
                e.mtime = (time_t)s2.st_mtime;
                ne.push_back(e);
            }
            if (!ne.empty()) ents = ne;
        }
    }

    if (passwd_mode == YZ_MAIN_ENCODE_PASSWD_ON) {
        char * p1 = yzm ? yzm->MessINPUT(YZ_MSG_TBL_23, YZ_MSG_NO_23) : 0;
        if (!p1 || !*p1) { evINF(yzm, YZ_MSG_TBL_24, YZ_MSG_NO_24); return -1; }
        pw = p1;
        for (;;) {
            char * p2 = yzm ? yzm->MessINPUT(YZ_MSG_TBL_46, YZ_MSG_NO_46) : 0;
            if (!p2) { evINF(yzm, YZ_MSG_TBL_49, YZ_MSG_NO_49); return -1; }
            if (pw == p2) break;
            if (evYN(yzm, YZ_MSG_TBL_48, YZ_MSG_NO_48) == 1) {
                evINF(yzm, YZ_MSG_TBL_49, YZ_MSG_NO_49);
                return -1;
            }
        }
        yz1codec::derive_key(pw.c_str(), key);
        chk = yz1codec::pw_chk(pw.c_str());
        enc = true;
    } else if (passwd_mode == YZ_MAIN_ENCODE_PASSWD_PUB) {
        char pub[64];
        unsigned int seed = (unsigned int)time(0) ^ (unsigned int)clock();
        int k;
        srand(seed);
        for (k = 0; k < 4; k++) key[k] = (unsigned char)(rand() & 0xFF);
        chk = (unsigned char)(key[0] ^ key[1] ^ key[2] ^ key[3]);
        yz1codec::make_pub_password(key, chk, pub, sizeof(pub));
        pw = pub;
        enc = true;
        evINF(yzm, YZ_MSG_TBL_50, YZ_MSG_NO_50, pub);
    }

    Bt = 8 * ents.size();
    for (i = 0; i < ents.size(); i++) Bt += ents[i].name.size() + 1;
    Atot = Bt;
    for (i = 0; i < ents.size(); i++) Atot += (size_t)ents[i].size;

    table.resize(Bt);
    {
        size_t off = 0;
        for (i = 0; i < ents.size(); i++) { put_be32((unsigned char *)&table[off], (u32)ents[i].size); off += 4; }
        for (i = 0; i < ents.size(); i++) { put_be32((unsigned char *)&table[off], (u32)ents[i].mtime); off += 4; }
        for (i = 0; i < ents.size(); i++) {
            table.replace(off, ents[i].name.size(), ents[i].name);
            off += ents[i].name.size();
            table[off++] = 0;
        }
    }
    payload = "";
    for (i = 0; i < ents.size(); i++) {
        std::string data;
        if (!read_all(ents[i].fs.c_str(), data)) {
            evEX(yzm, YZ_MSG_TBL_20, YZ_MSG_NO_20, ents[i].fs.c_str());
            return -1;
        }
        payload += data;
    }

    if (!yz1codec::compress((const unsigned char *)table.data(), Bt,
                            (const unsigned char *)payload.data(), payload.size(),
                            &body, &blen, err, sizeof(err))) {
        evEX(yzm, YZ_MSG_TBL_51, YZ_MSG_NO_51, "0");
        return -1;
    }

    if (enc) {
        for (i = 0; i < blen; i++) body[i] ^= key[i & 3];
    }

    {
        std::string in_norm = normalize_path(in_file_name);
        std::string::size_type a, b;
        if (in_norm.size() == output.size()) {
            bool same = true;
            for (a = 0, b = 0; a < in_norm.size(); a++, b++) {
                char ca = in_norm[a], cb = output[b];
                if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
                if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
                if (ca != cb) { same = false; break; }
            }
            if (same) {
                evEX(yzm, YZ_MSG_TBL_40, YZ_MSG_NO_40, in_file_name, "");
                free(body);
                return -1;
            }
        }
    }
    if (file_exists(output)) {
        if (evYN(yzm, YZ_MSG_TBL_41, YZ_MSG_NO_41, output.c_str()) == 1) {
            evINF(yzm, YZ_MSG_TBL_42, YZ_MSG_NO_42, output.c_str());
            free(body);
            return -1;
        }
    }
    if (20 + blen >= (size_t)st.st_size && (st.st_mode & _S_IFDIR) == 0) {
        if (evYN(yzm, YZ_MSG_TBL_57, YZ_MSG_NO_57, output.c_str()) == 1) {
            evINF(yzm, YZ_MSG_TBL_42, YZ_MSG_NO_42, output.c_str());
            free(body);
            return -1;
        }
    }

    {
        FILE * f = fopen(output.c_str(), "wb");
        unsigned char hd[20];
        if (!f) {
            evEX(yzm, YZ_MSG_TBL_43, YZ_MSG_NO_43, output.c_str());
            free(body);
            return -1;
        }
        if (SelfEncodeFile && *SelfEncodeFile) {
            std::string sfx;
            if (!read_all(SelfEncodeFile, sfx)) {
                evEX(yzm, YZ_MSG_TBL_20, YZ_MSG_NO_20, SelfEncodeFile);
                fclose(f); free(body);
                return -1;
            }
            if (!sfx.empty() && fwrite(sfx.data(), 1, sfx.size(), f) != sfx.size()) {
                evEX(yzm, YZ_MSG_TBL_55, YZ_MSG_NO_55, output.c_str());
                fclose(f); free(body);
                return -1;
            }
        }
        memcpy(hd, "yz0105", 6);
        hd[6] = enc ? 0x34 : 0x30;
        hd[7] = enc ? chk : 0x30;
        put_be32(hd + 8, (u32)Atot);
        put_be32(hd + 12, (u32)Bt);
        put_be32(hd + 16, (u32)ents.size());
        if (fwrite(hd, 1, 20, f) != 20 || (blen && fwrite(body, 1, blen, f) != blen)) {
            evEX(yzm, YZ_MSG_TBL_55, YZ_MSG_NO_55, output.c_str());
            fclose(f); free(body);
            return -1;
        }
        if (SelfEncodeFile && *SelfEncodeFile) {
            unsigned char t[4];
            put_be32(t, (u32)(20 + blen));
            if (fwrite(t, 1, 4, f) != 4) {
                evEX(yzm, YZ_MSG_TBL_55, YZ_MSG_NO_55, output.c_str());
                fclose(f); free(body);
                return -1;
            }
        }
        fclose(f);
        m_output_name = output.c_str();
    }

    free(body);
    sec_to_str(t0, sec, sizeof(sec));
    evINF(yzm, YZ_MSG_TBL_59, YZ_MSG_NO_59, sec);
    return 0;
}
