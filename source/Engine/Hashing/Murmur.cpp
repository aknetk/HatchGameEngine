#if INTERFACE

#include <Engine/Includes/Standard.h>

class Murmur {
public:

};
#endif

#include <Engine/Hashing/Murmur.h>
#include <Engine/Hashing/FNV1A.h>

PUBLIC STATIC Uint32 Murmur::EncryptString(char* message) {
    return Murmur::EncryptData(message, strlen(message), 0xDEADBEEF);
}
PUBLIC STATIC Uint32 Murmur::EncryptString(const char* message) {
    return Murmur::EncryptString((char*)message);
}

PUBLIC STATIC Uint32 Murmur::EncryptData(const void* data, Uint32 size) {
    return Murmur::EncryptData(data, size, 0xDEADBEEF);
}
PUBLIC STATIC Uint32 Murmur::EncryptData(const void* key, Uint32 size, Uint32 hash) {
    int i = size;
	const unsigned int m = 0x5bd1e995;
    const int r = 24;
	unsigned int h = hash ^ size;

	const unsigned char* data = (const unsigned char*)key;

	while (size >= 4) {
		unsigned int k = *(unsigned int *)data;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		data += 4;
		size -= 4;
	}

	// Handle the last few bytes of the input array
	switch (size) {
    	case 3: h ^= data[2] << 16;
    	case 2: h ^= data[1] << 8;
    	case 1: h ^= data[0];
    	        h *= m;
	}

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;
	return h;
}
