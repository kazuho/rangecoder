#ifndef __RANGE_CODER_HPP__
#define __RANGE_CODER_HPP__

#ifdef RANGE_CODER_USE_SSE
#include <xmmintrin.h>
#endif

// original work by Daisuke Okanohara 2006/06/16

struct rc_type_t {
  enum {
    TOP = 1U << 24,
    TOPMASK = TOP - 1,
  };
  typedef unsigned int uint;
  typedef unsigned char byte;
};

template <class Iter> class rc_encoder_t : public rc_type_t {
public:
  rc_encoder_t(const Iter &i) : iter(i) {
    L      = 0;
    R      = 0xFFFFFFFF;
    buffer = 0;
    carryN = 0;
    counter = 0;
    start  = true;
  }
  void encode(const uint low, const uint high, const uint total) {
    uint r    = R / total;
    if (high < total) {
      R = r * (high-low);
    } else {
      R -= r * low;
    }
    uint newL = L + r*low;
    if (newL < L) {
      //overflow occured (newL >= 2^32)
      //buffer FF FF .. FF -> buffer+1 00 00 .. 00
      buffer++;
      for (;carryN > 0; carryN--) {
	*iter++ = buffer;
	buffer = 0;
      }
    }
    L = newL;
    while (R < TOP) {
      const byte newBuffer = (L >> 24);
      if (start) {
	buffer = newBuffer;
	start  = false;
      } else if (newBuffer == 0xFF) {
	carryN++;
      } else {
	*iter++ = buffer;
	for (; carryN != 0; carryN--) {
	  *iter++ = 0xFF;
	}
	buffer = newBuffer;
      }
      L <<= 8;
      R <<= 8;
    }
    counter++;
  }
  void final() {
    *iter++ = buffer;
    for (; carryN != 0; carryN--) {
      *iter++ = 0xFF;
    }
    uint t = L + R;
    while (1) {
      uint t8 = t >> 24, l8 = L >> 24;
      *iter++ = l8;
      if (t8 != l8) {
	break;
      }
      t <<= 8;
      L <<= 8;
    }
  }
private:
  uint R;
  uint L;
  bool start;
  byte buffer;
  uint carryN;
  Iter iter;
  uint counter;
};

template <typename FreqType, unsigned _N, int _BASE> struct rc_decoder_search_traits_t : public rc_type_t {
  typedef FreqType freq_type;
  enum {
    N = _N,
    BASE = _BASE
  };
};

template <typename FreqType, unsigned _N, int _BASE = 0> struct rc_decoder_search_t : public rc_decoder_search_traits_t<FreqType, _N, _BASE> {
  static uint get_index(const FreqType *freq, FreqType pos) {
    uint left  = 0;
    uint right = _N;
    while(left < right) {
      uint mid = (left+right)/2;
      if (freq[mid+1] <= pos) left = mid+1;
      else                    right = mid;
    }
    return left;
  }
};

#ifdef RANGE_CODER_USE_SSE

template<int _BASE> struct rc_decoder_search_t<short, 256, _BASE> : public rc_decoder_search_traits_t<short, 256, _BASE> {
  static uint get_index(const short *freq, short pos) {
    __m128i v = _mm_set1_epi16(pos);
    unsigned i, mask = 0;
    for (i = 0; i < 256; i += 16) {
      __m128i x = *reinterpret_cast<const __m128i*>(freq + i);
      __m128i y = *reinterpret_cast<const __m128i*>(freq + i + 8);
      __m128i a = _mm_cmplt_epi16(v, x);
      __m128i b = _mm_cmplt_epi16(v, y);
      mask = (_mm_movemask_epi8(b) << 16) | _mm_movemask_epi8(a);
      if (mask) {
	return i + (__builtin_ctz(mask) >> 1) - 1;
      }
    }
    return 255;
  }
};

#endif

template <class Iterator, class SearchType> class rc_decoder_t : public rc_type_t {
public:
  typedef SearchType search_type;
  typedef typename search_type::freq_type freq_type;
  static const unsigned N = search_type::N;
  rc_decoder_t(const Iterator& _i, const Iterator _e) : iter(_i), iter_end(_e) {
    R = 0xFFFFFFFF;
    D = 0;
    for (int i = 0; i < 4; i++) {
      D = (D << 8) | next();
    }
  }
  uint decode(const uint total, const freq_type* cumFreq) {
    const uint r = R / total;
    const int targetPos = std::min(total-1, D / r);
    
    //find target s.t. cumFreq[target] <= targetPos < cumFreq[target+1]
    const uint target =
      search_type::get_index(cumFreq, targetPos + search_type::BASE);
    const uint low  = cumFreq[target] - search_type::BASE;
    const uint high = cumFreq[target+1] - search_type::BASE;
    
    D -= r * low;
    if (high != total) {
      R = r * (high-low);
    } else {
      R -= r * low;
    }
    
    while (R < TOP) {
      R <<= 8;
      D = (D << 8) | next();
    }
    
    return target;
  }
  byte next() {
    return iter != iter_end ? (byte)*iter++ : 0xff;
  }
private:
  uint R;
  uint D;
  Iterator iter, iter_end;
};

#endif
