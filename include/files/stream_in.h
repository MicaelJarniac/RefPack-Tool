/* $Id$ */

#ifndef STREAM_IN_H
#define STREAM_IN_H

template <class Tin> struct StreamInT;

template <> struct StreamInT<CBlobBaseSimple>
{
	const CBlobBaseSimple &m_in;
	uint32                 m_pos;

	StreamInT(const CBlobBaseSimple &in)
		: m_in(in)
		, m_pos(0)
	{}

	void Read(ErrInfo &ei, void *buf, uint32 length)
	{
		if (ei.Succeeded()) {
			if (m_pos + length > (uint32)m_in.RawSize()) {
				ei.Set(ERANGE);
				ei.Add("what: Reading behind EOF");
				ei.Add("at StreamInT<CBlobBaseSimple>::ReadExact()");
			} else {
				memcpy(buf, m_in.RawData() + m_pos, length);
				m_pos += length;
			}
		}
	}

	uint64 Size(ErrInfo &ei)
	{
		return m_in.RawSize();
	}

	uint64 Tell()
	{
		return m_pos;
	}
};

template <> struct StreamInT<BinaryFile>
{
	BinaryFile &m_in;

	StreamInT(BinaryFile &in)
		: m_in(in)
	{}

	void Read(ErrInfo &ei, void *buf, uint32 length)
	{
		m_in.Read(ei, buf, length);
	}

	uint64 Size(ErrInfo &ei)
	{
		return m_in.Length(ei);
	}

	uint64 Tell()
	{
		return m_in.Tell();
	}
};


template <class Tin, uint32 Tbuf_size = 0x20000> struct BufferedStreamInT : public StreamInT<Tin>
{
	typedef StreamInT<Tin> super;

	byte         *m_buf;
	uint64        m_pos;
	uint64        m_buf_pos;
	uint64        m_buf_end;
	uint64        m_size;

	CBlobT<byte>  m_buf_blob;

	static const uint32 cBufSize = Tbuf_size;

	BufferedStreamInT(Tin &src)
		: super(src)
		, m_buf(NULL)
		, m_pos(super::Tell())
		, m_buf_pos(0)
		, m_buf_end(0)
		, m_size(0)
	{
		m_buf = m_buf_blob.GrowSizeNC(cBufSize);
		m_size = Size(ErrInfo());
	}

	void Read(ErrInfo &ei, void *buf, uint32 length)
	{
		if (m_buf_pos <= m_pos && m_pos < m_buf_end) {
			/* at least part of the chunk is in our buffer */
			uint32 from_buffer = MinT(length, (uint32)(m_buf_end - m_pos));
			memcpy(buf, m_buf + (m_pos - m_buf_pos), from_buffer);
			m_pos += from_buffer;
			*(byte**)&buf += from_buffer;
			length -= from_buffer;
		}
		while (length > 0) {
			/* we need to read more data from the stream */
			m_buf_pos = m_pos;
			m_buf_end = MinT(m_pos + cBufSize, m_size);
			super::Read(ei, m_buf, (uint32)(m_buf_end - m_buf_pos));
			uint32 from_buffer = MinT(length, (uint32)(m_buf_end - m_pos));
			memcpy(buf, m_buf + (m_pos - m_buf_pos), from_buffer);
			m_pos += from_buffer;
			*(byte**)&buf += from_buffer;
			length -= from_buffer;
		}
	}

	uint64 Size(ErrInfo &ei)
	{
		m_size = super::Size(ei);
		return m_size;
	}

	uint64 Tell()
	{
		return m_pos;
	}
};

#endif /* STREAM_IN_H */
