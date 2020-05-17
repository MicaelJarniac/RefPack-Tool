/* $Id$ */


#ifndef ENDIANESS_H
#define ENDIANESS_H

template <typename T> struct EndianBaseT {
	T m_t;

	EndianBaseT()
		: m_t(0)
	{}

	T& Raw()
	{
		return m_t;
	}
};

template <typename T, size_t Tsize = sizeof(T)> struct LittleEndianT;
template <typename T, size_t Tsize = sizeof(T)> struct BigEndianT;

template <typename T> struct LittleEndianT<T, 2> : public EndianBaseT<T> {
	T Get()
	{
		return FromLE16(m_t);
	}
	void Set(T t)
	{
		m_t = ToLE16(t);
	}
};

template <typename T> struct LittleEndianT<T, 4> : public EndianBaseT<T> {
	T Get()
	{
		return FromLE32(m_t);
	}
	void Set(T t)
	{
		m_t = ToLE32(t);
	}
};

template <typename T> struct BigEndianT<T, 2> : public EndianBaseT<T> {
	T Get()
	{
		return FromBE16(m_t);
	}
	void Set(T t)
	{
		m_t = ToBE16(t);
	}
};

template <typename T> struct BigEndianT<T, 4> : public EndianBaseT<T> {
	T Get()
	{
		return FromBE32(m_t);
	}
	void Set(T t)
	{
		m_t = ToBE32(t);
	}
};


#endif /* ENDIANESS_H */
