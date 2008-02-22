#ifndef __RANGE_CODER_HPP__
#define __RANGE_CODER_HPP__

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

template <class Iterator, unsigned N> class rc_decoder_t : public rc_type_t {
public:
  rc_decoder_t(const Iterator& _i, const Iterator _e) : iter(_i), iter_end(_e) {
    R = 0xFFFFFFFF;
    D = 0;
    for (int i = 0; i < 4; i++) {
      D = (D << 8) | next();
    }
  }
  uint decode(const uint total, const uint* cumFreq) {
    const uint r = R / total;
    const uint targetPos = std::min(total-1, D / r);
    
    //find target s.t. cumFreq[target] <= targetPos < cumFreq[target+1]
    uint left  = 0;
    uint right = N;
    while(left < right) {
      uint mid = (left+right)/2;
      if (cumFreq[mid+1] <= targetPos) left = mid+1;
      else                             right = mid;
    }
    
    const uint target = left;
    const uint low  = cumFreq[target];
    const uint high = cumFreq[target+1];
    
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
