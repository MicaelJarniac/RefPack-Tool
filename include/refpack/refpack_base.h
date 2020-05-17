#ifndef REFPACK_BASE_H
#define REFPACK_BASE_H

#include <deque>

struct RefPack {
	static bool IsCompressed(const byte *buf);
	static uint32 GetUncompressedLength(const byte *buf);
}; // struct RefPackBase

inline /*static*/ bool RefPack::IsCompressed(const byte *buf)
{
	// read first 2 bytes as big-endian number
	uint16 hdr = (buf[0] << 8) | buf[1];
	// now we can tell if it is compressed
	bool is_compressed = ((hdr & 0x3EFF) == 0x10FB);
	return is_compressed;
}

inline /*static*/ uint32 RefPack::GetUncompressedLength(const byte *buf)
{
	/* first 2 bytes of compressed stream contain header */
	if (IsCompressed(buf)) {
		/* bytes 2, 3, 4 (,5) contain original (uncompressed) stream length (big-endian) */
		uint32 length = (((buf[2] << 8) | buf[3]) << 8) | buf[4];
		if ((buf[0] & 0x80) != 0) {
			length = (length << 8) | buf[5];
		}
		return length;
	}
	return 0;
}


#endif /* REFPACK_BASE_H */
