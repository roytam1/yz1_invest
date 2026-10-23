#ifndef __yz1codec_h__
#define __yz1codec_h__

#include <stddef.h>

namespace yz1codec {

struct Header
{
    unsigned long  all_size;
    unsigned long  head_size;
    unsigned long  file_count;
    unsigned char  flags;
    unsigned char  chk;

    bool encrypted() const { return (flags & 0x04) != 0; }
};

bool read_header(const unsigned char * data, size_t size, Header * hdr);

void derive_key(const char * password, unsigned char key[4]);
unsigned char pw_chk(const char * password);
bool parse_pub_password(const char * password, unsigned char key[4], unsigned char * chk);
void make_pub_password(const unsigned char key[4], unsigned char chk, char * out, size_t out_size);

bool decompress(const unsigned char * body, size_t body_size,
                size_t head_size, size_t total_size,
                unsigned char * out, char * err, size_t err_size);

bool compress(const unsigned char * table, size_t table_size,
              const unsigned char * payload, size_t payload_size,
              unsigned char ** body, size_t * body_size,
              char * err, size_t err_size);

}

#endif
