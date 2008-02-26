extern "C" {
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}
#include <algorithm>
#include "range_coder.hpp"

#include "table.c"

class writer_t {
  char **p, *max;
public:
  struct overrun_t {
  };
  writer_t(char **_p, char *_max) : p(_p), max(_max) {}
  writer_t &operator=(char c) {
    if (*p == max) {
      throw overrun_t();
    }
    *(*p)++ = c;
    return *this;
  }
  writer_t &operator*() { return *this; }
  writer_t &operator++() { return *this; }
  writer_t &operator++(int) { return *this; }
};

#define FREQ_BASE SHRT_MIN
#define LOOP_CNT 1024

int main(int argc, char **argv)
{
  char buf[1024 * 1024], cbuf[1024 * 1024], rbuf[1024 * 1024];
  size_t buflen, cbuflen;
  unsigned long long start;
  int i;
  
  /* read */
  buflen = fread(buf, 1, sizeof(buf) - 1, stdin);
  /* compress */
  start = rdtsc();
  for (i = 0; i < LOOP_CNT; i++) {
    char *cbufpt = cbuf;
    rc_encoder_t<writer_t> enc(writer_t(&cbufpt, cbuf + sizeof(cbuf)));
    for (const char *p = buf, *e = buf + buflen; p != e; p++) {
      unsigned ch = (unsigned char)*p;
#ifdef USE_ORDERED_TABLE
      ch = to_ordered[ch];
#endif
      assert(freq[ch] != freq[ch + 1]);
      enc.encode(freq[ch] - FREQ_BASE, freq[ch + 1] - FREQ_BASE, MAX_FREQ);
    }
    enc.final();
    cbuflen = cbufpt - cbuf;
  }
  printf("compression: %lu Mticks\n", (long)((rdtsc() - start) / 1024 / 1024));
  /* decompress */
  start = rdtsc();
  for (i = 0; i < LOOP_CNT; i++) {
    rc_decoder_t<const char*, rc_decoder_search_t<short, 256, FREQ_BASE> >
      dec(cbuf, cbuf + cbuflen);
    for (char *p = rbuf, *e = rbuf + buflen; p != e; p++) {
      unsigned ch = dec.decode(MAX_FREQ, freq);
#ifdef USE_ORDERED_TABLE
      ch = from_ordered[ch];
#endif
      *p = ch;
    }
  }
  printf("decompression: %lu Mticks\n",
	 (long)((rdtsc() - start) / 1024 / 1024));
  /* check result */
  if (memcmp(buf, rbuf, buflen) != 0) {
    fprintf(stderr, "original data and decompressed data does not match.\n");
    exit(99);
  }
  
  return 0;
}
