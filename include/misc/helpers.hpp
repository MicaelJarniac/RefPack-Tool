/* $Id:$ */

/** @file helpers.hpp */

#ifndef HELPERS_HPP
#define HELPERS_HPP

/** Type-safe version of memcpy().
* @param d destination buffer
* @param s source buffer
* @param num_items number of items to be copied (!not number of bytes!) */
template <class Titem_>
FORCEINLINE void MemCpyT(Titem_* d, const Titem_* s, size_t num_items = 1)
{
	memcpy(d, s, num_items * sizeof(Titem_));
}

/** big-endian / little-endian conversion */
inline uint16 ByteSwap16(uint16 n)
{
	return (n >> 8) | (n << 8);
}

/** big-endian / little-endian conversion */
inline uint32 ByteSwap32(uint32 n)
{
	n = (n >> 16) | (n << 16);
	return ((n & 0xFF00FF00U) >> 8) | ((n & 0x00FF00FFU) << 8);
}

#if CPU_LITTLE_ENDIAN
# define ToLE32(n)   (n)
# define ToLE16(n)   (n)
# define FromLE32(n) (n)
# define FromLE16(n) (n)
	static inline uint32 ToBE32(uint32 n)   { return ByteSwap32(n); }
	static inline uint16 ToBE16(uint16 n)   { return ByteSwap16(n); }
	static inline uint32 FromBE32(uint32 n) { return ByteSwap32(n); }
	static inline uint16 FromBE16(uint16 n) { return ByteSwap16(n); }
#else
	static inline uint32 ToLE32(uint32 n)   { return ByteSwap32(n); }
	static inline uint16 ToLE16(uint16 n)   { return ByteSwap16(n); }
	static inline uint32 FromLE32(uint32 n) { return ByteSwap32(n); }
	static inline uint16 FromLE16(uint16 n) { return ByteSwap16(n); }
# define ToBE32(n)   (n)
# define ToBE16(n)   (n)
# define FromBE32(n) (n)
# define FromBE16(n) (n)
#endif /* TTD_BIG_ENDIAN */


/** When allocating using malloc/calloc in C++ it is usually needed to cast the return value
*  from void* to the proper pointer type. Another alternative would be MallocT<> as follows */
template <typename T> FORCEINLINE T* MallocT(size_t num_elements)
{
	T *t_ptr = (T*)malloc(num_elements * sizeof(T));
	return t_ptr;
}
/** When allocating using malloc/calloc in C++ it is usually needed to cast the return value
*  from void* to the proper pointer type. Another alternative would be MallocT<> as follows */
template <typename T> FORCEINLINE T* CallocT(size_t num_elements)
{
	T *t_ptr = (T*)calloc(num_elements, sizeof(T));
	return t_ptr;
}
/** When allocating using malloc/calloc in C++ it is usually needed to cast the return value
*  from void* to the proper pointer type. Another alternative would be MallocT<> as follows */
template <typename T> FORCEINLINE T* ReallocT(T* t_ptr, size_t num_elements)
{
	t_ptr = (T*)realloc(t_ptr, num_elements * sizeof(T));
	return t_ptr;
}


/** type safe swap operation */
template<typename T> void Swap(T& a, T& b)
{
	T t = a;
	a = b;
	b = t;
}


/** returns the absolute value of (scalar) variable. @note assumes variable to be signed */
template <typename T> static inline T myabs(T a) { return a < (T)0 ? -a : a; }
/** returns the (absolute) difference between two (scalar) variables */
template <typename T> static inline T delta(T a, T b) { return a < b ? b - a : a - b; }

/** Some enums need to have allowed incrementing */
#define DECLARE_POSTFIX_INCREMENT(type) \
	FORCEINLINE type operator ++(type& e, int) \
	{ \
		type e_org = e; \
		e = (type)((int)e + 1); \
		return e_org; \
	} \
	FORCEINLINE type operator --(type& e, int) \
	{ \
		type e_org = e; \
		e = (type)((int)e - 1); \
		return e_org; \
	}



/** Operators to allow to work with enum as with type safe bit set in C++ */
# define DECLARE_ENUM_AS_BIT_SET(mask_t) \
	FORCEINLINE mask_t operator | (mask_t m1, mask_t m2) {return (mask_t)((int)m1 | m2);} \
	FORCEINLINE mask_t operator & (mask_t m1, mask_t m2) {return (mask_t)((int)m1 & m2);} \
	FORCEINLINE mask_t operator ^ (mask_t m1, mask_t m2) {return (mask_t)((int)m1 ^ m2);} \
	FORCEINLINE mask_t& operator |= (mask_t& m1, mask_t m2) {m1 = m1 | m2; return m1;} \
	FORCEINLINE mask_t& operator &= (mask_t& m1, mask_t m2) {m1 = m1 & m2; return m1;} \
	FORCEINLINE mask_t& operator ^= (mask_t& m1, mask_t m2) {m1 = m1 ^ m2; return m1;} \
	FORCEINLINE mask_t operator ~(mask_t m) {return (mask_t)(~(int)m);}

/** bit manipulators */
template <typename T> void ClrBitT(T &t, int bit_index)
{
	t = (T)(t & ~((T)1 << bit_index));
}

/** bit manipulators */
template <typename T> void SetBitT(T &t, int bit_index)
{
	t = (T)(t | ((T)1 << bit_index));
}

/** bit manipulators */
template <typename T> void ToggleBitT(T &t, int bit_index)
{
	t = (T)(t ^ ((T)1 << bit_index));
}

/** min/max */
template <typename T> T MinT(const T &t1, const T &t2)
{
	return ((t1 < t2) ? t1 : t2);
}

/** min/max */
template <typename T> T MaxT(const T &t1, const T &t2)
{
	return ((t1 < t2) ? t2 : t1);
}

#endif /* HELPERS_HPP */
