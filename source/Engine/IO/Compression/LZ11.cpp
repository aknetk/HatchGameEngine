#if INTERFACE
#include <Engine/Includes/Standard.h>
class LZ11 {
public:

};
#endif

#include <Engine/IO/Compression/LZ11.h>

PUBLIC STATIC bool LZ11::Decompress(uint8_t* in, size_t in_sz, uint8_t* out, size_t out_sz) {
    typedef uint8_t datat_t;
    int      i;
    datat_t  flags;
    datat_t* in_head = (datat_t*)in;
    datat_t* out_head = (datat_t*)out;
    size_t   size = out_sz;

    int bits = sizeof(flags) << 3;
    // int mask = (1 << (bits - 1));

    while (size > 0) {
        // Read in the flags data
        // From bit 7 to bit 0, following blocks:
        //     0: raw byte
        //     1: compressed block
        flags = *in_head++;
        if (in_head >= in_sz + (datat_t*)in)
            return false;

        // printf("Flag: %04X (", flags);
        for (i = 0; i < bits; i++) {
            // if (!(i & 7) && i)
                // printf(" ");
            // printf("%d", ((flags << i) & mask) >> (bits - 1));
        }
        // printf(")\n");

        // the first flag cannot start with 1, as that needs data to pull from

        for (i = 0; i < bits && size > 0; i++, flags <<= 1) {
            // Compressed block
            if (flags & 0x80) {
                datat_t displen[4];

                displen[0] = *in_head++;
                if (in_head >= in_sz + (datat_t*)in)
                    return false;

                size_t len;     // length
                size_t disp;    // displacement
                size_t pos = 0; // displen position

                switch (displen[pos] >> 4) {
                    case 0: // extended block
                        displen[1] = *in_head++; if (in_head >= in_sz + (datat_t*)in) return false;
                        displen[2] = *in_head++; if (in_head >= in_sz + (datat_t*)in) return false;

                        len   = displen[pos++] << 4;
                        len  |= displen[pos] >> 4;
                        len  += 0x11;
                        break;

                    case 1: // extra extended block
                        displen[1] = *in_head++; if (in_head >= in_sz + (datat_t*)in) return false;
                        displen[2] = *in_head++; if (in_head >= in_sz + (datat_t*)in) return false;
                        displen[3] = *in_head++; if (in_head >= in_sz + (datat_t*)in) return false;

                        len   = (displen[pos++] & 0x0F) << 12;
                        len  |= (displen[pos++]) << 4;
                        len  |= displen[pos] >> 4;
                        len  += 0x111;
                        break;

                    default: // normal block
                        // if (!buffer_get(buffer, &displen[1], callback, userdata))
                        //     return false;
                        displen[1] = *in_head++; if (in_head >= in_sz + (datat_t*)in) return false;

                        len   = (displen[pos] >> 4) + 1;
                        break;
                }

                disp  = (displen[pos++] & 0x0F) << 8;
                disp |= displen[pos];

                if (len > size)
                    len = size;

                size -= len;

                // iov_iter in = out;
                // iov_sub(&in, disp + 1);
                // iov_memmove(&out, &in, len);

                datat_t* sup = out_head - (disp + 1);
                // printf("Len: %d\n", lol);
                while (len-- > 0) {
                    // printf("Out (cmp): %04X\n", *sup);
                    *out_head++ = *sup++;
                }
            }
            // Uncompressed block
            else {
                // printf("Out (raw): %04X\n", *in_head);
                // copy a raw byte from the input to the output
                *out_head++ = *in_head++; if (in_head >= in_sz + (datat_t*)in) return false;
                --size;
            }
        }
    }

    //*/
    return true;
}
