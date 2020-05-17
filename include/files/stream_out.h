/* $Id$ */

#ifndef STREAM_OUT_H
#define STREAM_OUT_H

template <class Tout> struct StreamOutT;

template <> struct StreamOutT<uint64>
{
	uint64 &m_length;

	StreamOutT(uint64 &length)
		: m_length(length)
	{}

	void Write(ErrInfo &ei, void *buf, uint32 length)
	{
		m_length += length;
	}
};

template <> struct StreamOutT<CBlobBaseSimple>
{
	CBlobBaseSimple &m_out;

	StreamOutT(CBlobBaseSimple &out)
		: m_out(out)
	{}

	void Write(ErrInfo &ei, void *buf, uint32 length)
	{
		if (ei.Succeeded()) {
			m_out.AppendRaw(buf, length);
		}
	}
};

template <> struct StreamOutT<BinaryFile>
{
	BinaryFile &m_out;

	StreamOutT(BinaryFile &out)
		: m_out(out)
	{}

	void Write(ErrInfo &ei, void *buf, uint32 length)
	{
		m_out.Write(ei, buf, length);
	}
};

template <class Tout, uint32 Tbuf_size = 0x20000> struct BufferedStreamOutT : public StreamOutT<Tout>
{
	typedef StreamOutT<Tout> super;

	byte         *m_buf;
	uint32        m_buf_len;

	CBlobT<byte>  m_buf_blob;

	static const uint32 cBufSize = Tbuf_size;

	BufferedStreamOutT(Tout &out)
		: super(out)
		, m_buf(NULL)
		, m_buf_len(0)
	{
		m_buf = m_buf_blob.GrowSizeNC(cBufSize);
	}

	~BufferedStreamOutT()
	{
		Flush(ErrInfo());
	}

	void Write(ErrInfo &ei, void *buf, uint32 length)
	{
		if (m_buf_len + length > cBufSize) {
			Flush(ei);
		}
		if (length > (cBufSize / 2) && m_buf_len == 0) {
			super::Write(ei, buf, length);
		} else {
			memcpy(m_buf + m_buf_len, buf, length);
			m_buf_len += length;
		}
	}

	void Flush(ErrInfo &ei)
	{
		if (m_buf_len > 0) {
			super::Write(ei, m_buf, m_buf_len);
			m_buf_len = 0;
		}
	}
};


#endif /* STREAM_OUT_H */
