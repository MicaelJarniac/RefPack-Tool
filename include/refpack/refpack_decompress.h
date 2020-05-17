/* $Id$ */

#ifndef REFPACK_DECOMPRESS_H
#define REFPACK_DECOMPRESS_H


template <class Tout> struct LookBackStreamOutT : public BufferedStreamOutT<Tout> {
	typedef Tout dst_t;
	typedef BufferedStreamOutT<Tout> super;

	static const uint32 cBufferLength = 2 * 0x20000U;

	uint64            m_pos;
	byte              m_buf[cBufferLength];

	LookBackStreamOutT(dst_t &dst)
		: super(dst)
		, m_pos(0)
	{}

	template <class Tin> void Pump(ErrInfo &ei, BufferedStreamInT<Tin> &src, uint32 length)
	{
		if (ei.Succeeded()) {
			uint32 idx = m_pos % cBufferLength;
			byte *buf = m_buf + idx;
			if (idx + length <= cBufferLength) {
				src.Read(ei, buf, length);
				super::Write(ei, buf, length);
			} else {
				uint32 length_1 = cBufferLength - idx;
				uint32 length_2 = length - length_1;
				src.Read(ei, buf, length_1);
				super::Write(ei, buf, length_1);
				src.Read(ei, m_buf, length_2);
				super::Write(ei, m_buf, length_2);
			}
			m_pos += length;
		}
	}

	void Copy(ErrInfo &ei, uint32 offset, uint32 length)
	{
		if (ei.Succeeded()) {
			assert(offset < m_pos);
			uint32 idx = (m_pos - offset) % cBufferLength;
			byte *from = m_buf + (m_pos - offset) % cBufferLength;
			byte *to = m_buf + m_pos % cBufferLength;
			for (uint32 i = 0; i < length; i++) {
				*(to++) = *(from++);
				if (from >= endof(m_buf)) from = m_buf;
				if (to   >= endof(m_buf)) to   = m_buf;
			}
			if (idx + length <= cBufferLength) {
				super::Write(ei, m_buf + idx, length);
			} else {
				uint32 length_1 = cBufferLength - idx;
				super::Write(ei, m_buf + idx, length_1);
				super::Write(ei, m_buf, length - length_1);
			}
			m_pos += length;
		}
	}

	uint64 Tell()
	{
		return m_pos;
	}
};


template <class Tin, class Tout> struct RefPackDecompress : public RefPack {
	ErrInfo                  *m_ei;
	byte                      m_prefix[4];
	bool                      m_stop;
	uint32                    m_dst_length;
	BufferedStreamInT<Tin>    m_src;
	LookBackStreamOutT<Tout>  m_dst;

	RefPackDecompress(Tin &src, Tout &dst)
		: m_ei(NULL)
		, m_stop(false)
		, m_dst_length(0)
		, m_src(src)
		, m_dst(dst)
	{
	}

	~RefPackDecompress()
	{
	}

	void CopyShort()
	{
		/* read one more byte from compressed stream */
		m_src.Read(*m_ei, m_prefix + 1, 1);

		/* num_src_bytes ~ 0..3 */
		/* num_dst_bytes ~ 3..0x0A */
		/* dst_offset    ~ 1..0x400 */
		uint32 num_src_bytes = m_prefix[0] & 3;
		uint32 num_dst_bytes = ((m_prefix[0] & 0x1C) >> 2) + 3;
		uint32 dst_offset = (((m_prefix[0] & 0x60) << 3) | m_prefix[1]) + 1;

		m_dst.Pump(*m_ei, m_src, num_src_bytes);
		m_dst.Copy(*m_ei, dst_offset, num_dst_bytes);
	}

	void CopyMedium()
	{
		/* read two more bytes from compressed stream */
		m_src.Read(*m_ei, m_prefix + 1, 2);

		/* num_src_bytes ~ 0..3 */
		/* num_dst_bytes ~ 4..0x43 */
		/* dst_offset    ~ 1..0x4000 */
		uint32 num_src_bytes = m_prefix[1] >> 6;
		uint32 num_dst_bytes = (m_prefix[0] & 0x3F) + 4;
		uint32 dst_offset = (((m_prefix[1] & 0x3F) << 8) | m_prefix[2]) + 1;

		m_dst.Pump(*m_ei, m_src, num_src_bytes);
		m_dst.Copy(*m_ei, dst_offset, num_dst_bytes);
	}

	void CopyLong()
	{
		/* read three more bytes from compressed stream */
		m_src.Read(*m_ei, m_prefix + 1, 3);

		/* num_src_bytes ~ 0..3 */
		/* num_dst_bytes ~ 5..0x404 */
		/* dst_offset    ~ 1..0x20000 */
		uint32 num_src_bytes = m_prefix[0] & 3;
		uint32 num_dst_bytes = (((m_prefix[0] & 0x0C) << 6) | m_prefix[3]) + 5;
		uint32 dst_offset = (((((m_prefix[0] & 0x10) << 4) | m_prefix[1]) << 8) | m_prefix[2]) + 1;

		m_dst.Pump(*m_ei, m_src, num_src_bytes);
		m_dst.Copy(*m_ei, dst_offset, num_dst_bytes);
	}

	void ImmediateBytesAndFinish()
	{
		/* num_src_bytes ~ 0..3 and finish */
		uint32 num_src_bytes = m_prefix[0] & 3;

		m_dst.Pump(*m_ei, m_src, num_src_bytes);
		m_stop = true;

//		assert(m_src.Tell() == m_src_length);
		assert(m_dst.Tell() == m_dst_length);
	}

	void ImmediateBytesLong()
	{
		/* num_src_bytes ~ 4..0x70 step 4 */
		uint32 num_src_bytes = ((m_prefix[0] & 0x1F) + 1) * 4;

		m_dst.Pump(*m_ei, m_src, num_src_bytes);
	}

	void DecompressionOneStep()
	{
		/* read one byte from compressed stream */
		m_src.Read(*m_ei, m_prefix, 1);
		if (m_prefix[0] >= 0xC0) {
			if (m_prefix[0] >= 0xE0) {
				/* 0xE0..0xFF */
				if (m_prefix[0] >= 0xFC) {
					/* 0xFC..0xFF */
					ImmediateBytesAndFinish();
				} else {
					/* 0xE0..0xFB */
					ImmediateBytesLong();
				}
			} else {
				/* 0xC0..0xDF */
				CopyLong();
			}
		} else {
			if (m_prefix[0] >= 0x80) {
				/* 0x80..0xBF */
				CopyMedium();
			} else {
				/* 0x00..0x7F */
				CopyShort();
			}
		}
	}

	void DecompressionLoop()
	{
		uint32 progress_step = m_dst_length / 100;
		uint32 next_progress_update = progress_step;
		uint32 progress = 0;
		printf(" ...");

		while (!m_stop && m_ei->Succeeded()) {
			DecompressionOneStep();

			uint32 dst_pos = m_dst.Tell();
			if (dst_pos >= next_progress_update) {
				next_progress_update += progress_step;
				printf("\b\b\b\b%2u %%", ++progress);
			}
		}
	}

	void PumpAll()
	{
		/* should we do it? */
	}

	void ReadHeader()
	{
		byte hdr[2];
		m_src.Read(*m_ei, hdr, 2);
		/* hdr & 0x3EFF) == 0x10FB */
		if ((hdr[0] & 0x3E) != 0x10 || (hdr[1] != 0xFB)) {
			/* stream is not compressed */
			m_dst.Write(*m_ei, hdr, 2);
			PumpAll();
			m_stop = true;
		}
		/* read destination (uncompressed) length */
		bool is_long  = ((hdr[0] & 0x80) != 0);
		bool has_more = ((hdr[0] & 0x01) != 0);
		byte buf[8];
		m_src.Read(*m_ei, buf, (is_long ? 4 : 3) * (has_more ? 2 : 1));
		m_dst_length = (((buf[0] << 8) + buf[1]) << 8) + buf[2];
		if (is_long) {
			m_dst_length = (m_dst_length << 8) + buf[3];
		}
	}

	void Decompress(ErrInfo &ei)
	{
		m_ei = &ei;

		ReadHeader();
		DecompressionLoop();
		m_dst.Flush(*m_ei);

		m_ei = NULL;
	}

};


#endif /* REFPACK_DECOMPRESS_H */
