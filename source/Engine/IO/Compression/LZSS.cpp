#if INTERFACE
#include <Engine/Includes/Standard.h>
class LZSS {
public:

};
#endif

#include <Engine/IO/Compression/LZSS.h>

PUBLIC STATIC bool LZSS::Decompress(uint8_t* in, size_t in_sz, uint8_t* out, size_t out_sz) {
    /*
    uint8_t      flags = 0;
      uint8_t      mask  = 0;
      unsigned int len;
      unsigned int disp;

      while(size > 0)
      {
        if(mask == 0)
        {
          // read in the flags data
          // from bit 7 to bit 0:
          //     0: raw byte
          //     1: compressed block
          if(!buffer_get(buffer, &flags, callback, userdata))
            return false;
          mask = 0x80;
        }

        if(flags & mask) // compressed block
        {
          uint8_t displen[2];
          if(!buffer_get(buffer, &displen[0], callback, userdata)
          || !buffer_get(buffer, &displen[1], callback, userdata))
            return false;

          // disp: displacement
          // len:  length
          len  = ((displen[0] & 0xF0) >> 4) + 3;
          disp = displen[0] & 0x0F;
          disp = disp << 8 | displen[1];

          if(len > size)
            len = size;

          size -= len;

          iov_iter in = out;
          iov_sub(&in, disp+1);
          iov_memmove(&out, &in, len);
        }
        else // uncompressed block
        {
          // copy a raw byte from the input to the output
          if(!buffer_get(buffer, iov_addr(&out), callback, userdata))
            return false;

          --size;
          iov_increment(&out);
        }

        mask >>= 1;
      }
//*/
      return true;
}
