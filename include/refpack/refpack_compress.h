#ifndef REFPACK_COMPRESS_H
#define REFPACK_COMPRESS_H


struct HistogramItem {
	uint32 m_num;
	uint16 m_extra[6];
};


template <class Tin, class Tout> struct RefPackCompress : public RefPack {
	static const uint32 cInvalidPos = 0xFFFFFFFFU;
	static const uint32 cHashEnd = 0x00010000U;
	static const uint32 cMaxSubstringsInChain = 0x40;//0xFFFFFFFFU;
	static const uint32 cNextIndexLength = 0x8000;//0x20000U;
	static const uint32 cBufferLength = 4 * cNextIndexLength;
	static const uint32 cBufferShadowLength = cNextIndexLength;

	struct ByFirst2BytesIndexItem {
		uint32 m_first;    // first (least recent) position of these 2 bytes in the stream
		uint32 m_last;     // last known (most recent) position of those 2 bytes in the stream
		uint32 m_num;      // number of occurrences of these 2 bytes inside the 'look back' window (0x20000 bytes)
		uint32 m_max_num;  // max of m_num

		static uint32& GetMaxMaxNum()
		{
			static uint32 max_max_num = 0;
			return max_max_num;
		}
	};

	uint32                    m_src_pos;
	uint32                    m_src_end_pos;
	uint32                    m_buf_end_pos;
	StreamInT<Tin>            m_src;
	BufferedStreamOutT<Tout>  m_dst;

	/* We will collect information about 0x20000 previous source (uncompressed
	 * stream) positions where potentially reusable substrings begin. First of
	 * all we will sort them by the first two bytes (0x10000 combinations) and
	 * store position of the first one in 'm_first' as array of 0x10000 uint32
	 * positions. */

	uint32           m_next[cNextIndexLength];  // index = lowest 17 bits of position in the stream
	                                            // value = next known position of the same 2 bytes
	uint16           m_next_same[cNextIndexLength];  // index = lowest 17 bits of position in the stream
	                                                 // value = how many bytes from the next substring are the same as in this substring

	byte             m_buf[cBufferLength + cBufferShadowLength];      // look back buffer

	ByFirst2BytesIndexItem m_by_first_2_bytes[cHashEnd];

	RefPackCompress(Tin &src, Tout &dst)
		: m_src_pos(0)
		, m_src_end_pos(0)
		, m_buf_end_pos(0)
		, m_src(src)
		, m_dst(dst)
	{
		/* clear our substring index */
		for (uint32 i = 0; i < lengthof(m_next); i++) {
			m_next[i] = cInvalidPos;
		}
		for (uint32 i = 0; i < lengthof(m_by_first_2_bytes); i++) {
			ByFirst2BytesIndexItem &by_first_2_bytes = m_by_first_2_bytes[i];
			by_first_2_bytes.m_first = cInvalidPos;
			by_first_2_bytes.m_last = cInvalidPos;
			by_first_2_bytes.m_num = 0;
			by_first_2_bytes.m_max_num = 0;
		}
	}

	~RefPackCompress()
	{
//		PrintStats();
	}

	void PrintStats()
	{
		/* make ByFirst2BytesIndexItem::m_max_num histogram */
		CBlobT<HistogramItem> blb_histo_max_num;
		CreateMaxNumHistogram(blb_histo_max_num);
		PrintHistogram(blb_histo_max_num);
	}

	void PrintHistogram(CBlobT<HistogramItem> &blb_histo)
	{
		printf("\n");
		uint32 max_val = FindHistogramMax(blb_histo);
		uint32 num_items = blb_histo.Size();
		HistogramItem *histo = blb_histo.Data();
		for (uint32 i = num_items - 1; i > 0; i--) {
			HistogramItem &item = histo[i];
			uint32 max_num = item.m_num;
			if (max_num == 0) {
				continue;
			}
			printf("%5u-%4u (%02X,%02X)", i, max_num, item.m_extra[0] >> 8, item.m_extra[1] & 0x00FF);
			uint32 perc = max_num * 100 / max_val;
			for (uint32 i = 0; i < perc; i++) {
				printf("-");
			}
			printf("\n");
		}
	}

	uint32 FindHistogramMax(CBlobT<HistogramItem> &blb_histo)
	{
		uint32 max_val = 0;
		uint32 num_items = blb_histo.Size();
		HistogramItem *histo = blb_histo.Data();
		for (uint32 i = 0; i < num_items; i++) {
			HistogramItem &item = histo[i];
			if (item.m_num > max_val) {
				max_val = item.m_num;
			}
		}
		return max_val;
	}

	void CreateMaxNumHistogram(CBlobT<HistogramItem> &blb_histo)
	{
		uint32 max_max_num = ByFirst2BytesIndexItem::GetMaxMaxNum();
		HistogramItem *histo = blb_histo.GrowSizeNC(max_max_num + 1);
		for (uint32 i = 0; i < lengthof(m_by_first_2_bytes); i++) {
			ByFirst2BytesIndexItem &by_first_2_bytes = m_by_first_2_bytes[i];
			HistogramItem &item = histo[by_first_2_bytes.m_max_num];
			if (item.m_num < lengthof(item.m_extra)) {
				item.m_extra[item.m_num] = i;
			}
			item.m_num++;
		}
	}

	FORCEINLINE void WriteBuffer(ErrInfo &ei, uint32 pos, uint32 length)
	{
		m_dst.Write(ei, m_buf + (pos % cBufferLength), length);
	}

	void ReadAhead(ErrInfo &ei, uint32 length)
	{
		if (ei.Failed()) return;
		assert(length % cNextIndexLength == 0);
		if (m_buf_end_pos >= m_src_end_pos) return;
		if (m_buf_end_pos + length > m_src_end_pos) {
			length = m_src_end_pos - m_buf_end_pos;
		}
		uint32 idx = m_buf_end_pos % cBufferLength;
		m_src.Read(ei, &m_buf[idx], length);
		if (idx < cBufferShadowLength) {
			memcpy(&m_buf[idx + cBufferLength], &m_buf[idx], MinT(length, cBufferShadowLength - idx));
		}
		m_buf_end_pos += length;
	}

	FORCEINLINE uint32 Hash(uint32 pos)
	{
		/* hash (made from first 2 chars of substring) used as index into m_first and m_last */
		byte *c = &m_buf[pos % cBufferLength];
		uint32 hash = (c[0] << 8) + c[1];
		return hash;
	}

	FORCEINLINE void RemoveKnownSubstring(uint32 pos)
	{
		/* hash (made from first 2 chars of substring) used as index into m_first and m_last */
		uint32 hash = Hash(pos);
		ByFirst2BytesIndexItem &by_first_2_bytes = m_by_first_2_bytes[hash];

		/* the first known occurrence should be the one we are about to remove */
//		assert(by_first_2_bytes.m_first == pos);
		if (by_first_2_bytes.m_first != pos) {
			/* this substring was probably not added due to cMaxSubstringsInChain limit */
			return;
		}

		by_first_2_bytes.m_num--;

		/* find the next one */
		uint32 &ref_next_pos = m_next[pos % cNextIndexLength];

		if (ref_next_pos == cInvalidPos) {
			/* it was also the last one (there is no next in the chain) */
			assert(by_first_2_bytes.m_last == pos);
			/* remove it */
			by_first_2_bytes.m_first = cInvalidPos;
			by_first_2_bytes.m_last = cInvalidPos;
		} else {
			/* it wasn't the last one (there is some more in the chain) */
			assert(by_first_2_bytes.m_last != cInvalidPos);
			assert(by_first_2_bytes.m_last != pos);
			by_first_2_bytes.m_first = ref_next_pos;
			ref_next_pos = cInvalidPos;
		}
	}

	FORCEINLINE void AddKnownSubstring(uint32 pos)
	{
		/* hash (made from first 2 chars of substring) used as index into m_first and m_last */
		uint32 hash = Hash(pos);
		ByFirst2BytesIndexItem &by_first_2_bytes = m_by_first_2_bytes[hash];

		if (by_first_2_bytes.m_num >= cMaxSubstringsInChain) {
			/* too many such substrings in this chunk
			 *  - remove the oldest one (performance optimization) */
			uint32 remove_from_pos = by_first_2_bytes.m_first;
			RemoveKnownSubstring(remove_from_pos);
		}

		by_first_2_bytes.m_num++;
		if (by_first_2_bytes.m_num > by_first_2_bytes.m_max_num) {
			by_first_2_bytes.m_max_num = by_first_2_bytes.m_num;
			if (by_first_2_bytes.m_max_num > ByFirst2BytesIndexItem::GetMaxMaxNum()) {
				ByFirst2BytesIndexItem::GetMaxMaxNum() = by_first_2_bytes.m_max_num;
			}
		}
		/* previous occurrence of similar substring (with the same hash) */
		uint32 prev_pos = by_first_2_bytes.m_last;

		/* new last in the chain will be our substring */
		by_first_2_bytes.m_last = pos;

		/* ensure that there is some 'first' in the chain */
		if (prev_pos == cInvalidPos) {
			/* we found the first occurrence of the substring */
			assert(by_first_2_bytes.m_first == cInvalidPos);
			by_first_2_bytes.m_first = pos;
			m_next_same[prev_pos % cNextIndexLength] = 2;
		} else {
			/* it isn't the first occurrence of the substring */
			assert(by_first_2_bytes.m_first != cInvalidPos);
			/* link the previous occurrence to the current one */
			assert(m_next[prev_pos % cNextIndexLength] == cInvalidPos);
			m_next[prev_pos % cNextIndexLength] = pos;
			m_next_same[prev_pos % cNextIndexLength] = (uint16)MatchingScore(prev_pos, pos, 2);
		}

		/* the current one has no next */
		assert(m_next[pos % cNextIndexLength] == cInvalidPos);

	}

	FORCEINLINE uint32 MatchingScore(uint32 pos1, uint32 pos2, uint32 min_score)
	{
		assert(pos1 < pos2);
		assert(pos2 < m_buf_end_pos);
		byte *c1 = &m_buf[pos1 % cBufferLength];
		byte *c2 = &m_buf[pos2 % cBufferLength];
		assert(memcmp(c1, c2, min_score) == 0);
		uint32 max_score = MinT(m_buf_end_pos - pos2 - 1, cNextIndexLength);
		uint32 score = min_score;
		while (score < max_score && c1[score] == c2[score]) {
			score++;
		}
		return score;
	}

	FORCEINLINE uint32 FindBestMatchingKnownSubstring(uint32 pos, uint32 *ret_best_score)
	{
		/* hash (made from first 2 chars of substring) used as index into m_first and m_last */
		uint32 hash = Hash(pos);
		ByFirst2BytesIndexItem &by_first_2_bytes = m_by_first_2_bytes[hash];

		/* enumerate whole chain of all known substrings that have same 2 bytes at their start
		 * and try to find the best matching known substring */
		uint32 best_score = 0;
		uint32 best_pos = cInvalidPos;
		uint32 same_bytes = 2;
		uint32 score = 2;
		for (uint32 other_pos = by_first_2_bytes.m_first; other_pos != cInvalidPos; same_bytes = m_next_same[other_pos % cNextIndexLength], other_pos = m_next[other_pos % cNextIndexLength]) {
			assert(other_pos < pos);
			assert(other_pos + lengthof(m_next) >= pos);
			assert(Hash(other_pos) == hash);
			uint32 min_score = MinT(same_bytes, score);
			score = MatchingScore(other_pos, pos, min_score);
			if (score >= best_score) {
				best_score = score;
				best_pos = other_pos;
			}
		}
		*ret_best_score = best_score;
		return best_pos;
	}

	FORCEINLINE void CopySourceAndFinish(ErrInfo &ei, uint32 src_copy_from, uint32 src_copy_num_bytes)
	{
//		printf("Final source copy: 0x%08X / 0x%08X [=%5u]\n", src_copy_from, src_copy_num_bytes, src_copy_num_bytes);

		while (src_copy_num_bytes > 3) {
			/* more than 3 bytes need to be copied from source */
			uint32 num_bytes = src_copy_num_bytes & ~3; // long copy allows num_bytes with step 4
			if (num_bytes > 0x70) num_bytes = 0x70; // max num_bytes for long copy is 0x70
			byte tag[] = {
				(byte)(0xE0 | ((num_bytes >> 2) - 1))
			};
			m_dst.Write(ei, tag, lengthof(tag));
			WriteBuffer(ei, src_copy_from, num_bytes);
			src_copy_from += num_bytes;
			src_copy_num_bytes -= num_bytes;
		}
		byte tag[] = {
			(byte)(0xFC | src_copy_num_bytes)
		};
		m_dst.Write(ei, tag, lengthof(tag));
		WriteBuffer(ei, src_copy_from, src_copy_num_bytes);
		src_copy_from += src_copy_num_bytes;
		src_copy_num_bytes = 0;
	}

	FORCEINLINE void CombinedCopy(ErrInfo &ei, uint32 src_copy_from, uint32 src_copy_num_bytes, uint32 dst_copy_from, uint32 dst_copy_num_bytes)
	{
//		printf("Source copy      : 0x%08X / 0x%08X [=%5u]\n", src_copy_from, src_copy_num_bytes, src_copy_num_bytes);
//		printf("Destination copy : 0x%08X / 0x%08X [=%5u]\n", dst_copy_from, dst_copy_num_bytes, dst_copy_num_bytes);

		while (src_copy_num_bytes > 3) {
			/* more than 3 bytes need to be copied from source */
			uint32 num_bytes = src_copy_num_bytes & ~3; // long copy allows num_bytes with step 4
			if (num_bytes > 0x70) num_bytes = 0x70; // max num_bytes for long copy is 0x70
			byte tag[] = {
				(byte)(0xE0 | ((num_bytes >> 2) - 1))
			};
			m_dst.Write(ei, tag, lengthof(tag));
			WriteBuffer(ei, src_copy_from, num_bytes);
			src_copy_from += num_bytes;
			src_copy_num_bytes -= num_bytes;
		}

		uint32 dst_copy_offset = src_copy_from + src_copy_num_bytes - dst_copy_from;
		while (dst_copy_num_bytes > 0 || src_copy_num_bytes > 0) {
			if (dst_copy_offset > 0x4000 || dst_copy_num_bytes > 0x43) {
				/* use large dst copy */
				uint32 num_bytes = dst_copy_num_bytes;
				/* max num_bytes for huge dst copy is 0x404 */
				if (num_bytes > 0x404) {
					num_bytes = 0x404;
					/* not less than 5 bytes should remain */
					if (dst_copy_num_bytes - num_bytes < 5) {
						num_bytes = dst_copy_num_bytes - 5;
					}
				}
				byte tag[] = {
					(byte)(0xC0 | (((dst_copy_offset - 1) >> 12) & 0x10) | (((num_bytes - 5) >> 6) & 0x0C) | (src_copy_num_bytes & 0x03)),
					(byte)((dst_copy_offset - 1) >> 8),
					(byte)(dst_copy_offset - 1),
					(byte)((num_bytes - 5)),
				};
				m_dst.Write(ei, tag, lengthof(tag));
				WriteBuffer(ei, src_copy_from, src_copy_num_bytes);
				src_copy_from += src_copy_num_bytes;
				src_copy_num_bytes = 0;
				dst_copy_num_bytes -= num_bytes;
			} else if (dst_copy_offset > 0x400 || dst_copy_num_bytes > 0x0A) {
				/* medium dst copy */
				uint32 num_bytes = dst_copy_num_bytes;
				/* max num_bytes for huge dst copy is 0x404 */
				byte tag[] = {
					(byte)(0x80 | (num_bytes - 4)),
					(byte)((src_copy_num_bytes << 6) | (dst_copy_offset - 1) >> 8),
					(byte)(dst_copy_offset - 1),
				};
				m_dst.Write(ei, tag, lengthof(tag));
				WriteBuffer(ei, src_copy_from, src_copy_num_bytes);
				src_copy_from += src_copy_num_bytes;
				src_copy_num_bytes = 0;
				dst_copy_num_bytes -= num_bytes;
			} else {
				/* short dst copy */
				uint32 num_bytes = dst_copy_num_bytes;
				/* max num_bytes for huge dst copy is 0x404 */
				byte tag[] = {
					(byte)(0x00 | (((dst_copy_offset - 1) >> 3) & 0x60) | (((num_bytes - 3) << 2) & 0x1C) | (src_copy_num_bytes & 0x03)),
					(byte)(dst_copy_offset - 1),
				};
				m_dst.Write(ei, tag, lengthof(tag));
				WriteBuffer(ei, src_copy_from, src_copy_num_bytes);
				src_copy_from += src_copy_num_bytes;
				src_copy_num_bytes = 0;
				dst_copy_num_bytes -= num_bytes;
			}
		}
	}

	FORCEINLINE bool IsGoodScoreForDstOffset(uint32 score, uint32 offset)
	{
		if (score < 3) {
			return false;
		}
		if (score < 4) {
			return (offset <= 0x400);
		}
		if (score < 5) {
			return (offset <= 0x4000);
		}
		return true;
	}

	FORCEINLINE void CompressionOneStep(ErrInfo &ei)
	{
		uint32 pos = m_src_pos;
		uint32 src_copy_start = pos;
		uint32 score = 0;
		uint32 dst_copy_from_pos = cInvalidPos;
		while (!IsGoodScoreForDstOffset(score, pos - dst_copy_from_pos)) {
			dst_copy_from_pos = FindBestMatchingKnownSubstring(pos, &score);
			if (pos >= lengthof(m_next)) {
				RemoveKnownSubstring(pos - lengthof(m_next));
			}
			AddKnownSubstring(pos);
			pos++;
			if (pos >= m_src_pos + 0x70) {
				CombinedCopy(ei, m_src_pos, pos - m_src_pos, m_src_pos, 0);
				m_src_pos = pos;
				return;
			}
			if (pos < m_src_end_pos) {
				continue;
			}
			/* eof found */
			CopySourceAndFinish(ei, src_copy_start, pos - src_copy_start);
			m_src_pos = pos;
			return;
		}
		/* score was at least 3 */
		CombinedCopy(ei, src_copy_start, pos - 1 - src_copy_start, dst_copy_from_pos, score);
		for ( ;score > 1; score--) {
			if (pos >= lengthof(m_next)) {
				RemoveKnownSubstring(pos - lengthof(m_next));
			}
			AddKnownSubstring(pos);
			pos++;
		}
		m_src_pos = pos;
	}

	void CompressionLoop(ErrInfo &ei)
	{
		printf(" ...");
		uint32 progress_step = m_src_end_pos / 100;
		uint32 next_progress_update = progress_step;

		/* read ahead some data into buffer */
		ReadAhead(ei, 2 * cNextIndexLength);

		while (ei.Succeeded() && m_src_pos < m_src_end_pos) {
			CompressionOneStep(ei);

			if (m_src_pos >= next_progress_update) {
				printf("\b\b\b\b%2u %%", m_src_pos / progress_step);
				next_progress_update += progress_step;
			}

			if (m_src_pos + cNextIndexLength >= m_buf_end_pos && m_buf_end_pos < m_src_end_pos) {
				ReadAhead(ei, cNextIndexLength);
			}
		}
	}

	void WriteHeader(ErrInfo &ei)
	{
		if (ei.Succeeded()) {

			bool is_long_src = (m_src_end_pos > 0xFFFFFF);
			
			/* 2 bytes header */
			byte hdr[2] = {is_long_src ? 0x90 : 0x10, 0xFB};
			m_dst.Write(ei, hdr, lengthof(hdr));

			/* 3 or 4 bytes big-endian source size */
			byte size[4] = {(byte)(m_src_end_pos >> 24), (byte)(m_src_end_pos >> 16), (byte)(m_src_end_pos >> 8), (byte)(m_src_end_pos >> 0)};
			m_dst.Write(ei, size + (is_long_src ? 0 : 1), 4 - (is_long_src ? 0 : 1));
		}
	}

	void Compress(ErrInfo &ei)
	{
		m_src_end_pos = m_src.Size(ei);

		WriteHeader(ei);
		CompressionLoop(ei);

		m_dst.Flush(ei);
	}
};


#endif /* REFPACK_COMPRESS_H */
