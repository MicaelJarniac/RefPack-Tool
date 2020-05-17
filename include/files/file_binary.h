/* $Id$ */

#ifndef FILE_BINARY_H
#define FILE_BINARY_H

#include <ctype.h>
#include <limits.h>

struct BinaryFile {
	enum OPENMODE {
		omNone = 0,
		omRead = 0,
		omWrite = 1,
	};

	CStrA      m_filename;
	OPENMODE   m_mode;
	FILE      *m_file;
	uint64     m_filesize;

	BinaryFile()
		: m_filename()
		, m_mode(omNone)
		, m_file(NULL)
		, m_filesize(0)
	{}

	~BinaryFile()
	{
		if (m_file != NULL) {
			fclose(m_file);
			m_file = NULL;
		}
	}

	virtual void Open(ErrInfo &ei, const char *filename, OPENMODE mode);
	virtual void Close(ErrInfo &ei);

	uint64 Length(ErrInfo &ei);
	uint64 Tell();
	void Seek(ErrInfo &ei, uint64 offset);
	uint32 Read(ErrInfo &ei, void *buf, uint32 max_length);
	void ReadExact(ErrInfo &ei, void *buf, uint32 length);
	void Write(ErrInfo &ei, const void *buf, uint32 length);

	template <typename T, class Tendianess> void ReadE(ErrInfo &ei, T &t);

	template <typename T> void ReadLE(ErrInfo &ei, T &t);
	template <typename T> void ReadBE(ErrInfo &ei, T &t);

	template <typename T, class Tendianess> void WriteE(ErrInfo &ei, const T &t);

	template <typename T> void WriteLE(ErrInfo &ei, const T &t);
	template <typename T> void WriteBE(ErrInfo &ei, const T &t);
};

inline void BinaryFile::Open(ErrInfo &ei, const char *filename, OPENMODE mode)
{
	bool entered_ok = ei.Succeeded();

	if (ei.Succeeded()) {
		assert(m_file == NULL);
		m_file = fopen(filename, ((mode & omWrite) != 0) ? "wb" : "rb");
		if (m_file == NULL) {
			ei.Set(errno);
			ei.Add("at fopen(\"%s\", %d)", filename, mode);
		}
	}

	if (ei.Succeeded()) {
		m_filename = filename;
		m_mode = mode;
		/* cache the total file size (on disk) */
		fseek(m_file, 0, SEEK_END);
		m_filesize = _ftelli64(m_file);
		fseek(m_file, 0, SEEK_SET);
	}

	if (ei.Failed() && entered_ok) {
		ei.Add("at BinaryFile::Open()");
	}
}

inline void BinaryFile::Close(ErrInfo &ei)
{
	bool entered_ok = ei.Succeeded();

	if (ei.Succeeded()) {
		assert(m_file != NULL);
	}
	if (m_file != 0 && fclose(m_file) != 0) {
		ei.Set(errno, " at fclose()");
		ei.Add(" file: \"%s\"", m_filename.Data());
	}
	m_file = NULL;

	if (ei.Failed() && entered_ok) {
		ei.Add("at BinaryFile::Close()");
	}
}

inline uint64 BinaryFile::Length(ErrInfo &ei)
{
	if (ei.Failed()) {
		return 0;
	}

	int64 old_pos = _ftelli64(m_file);

	if (_fseeki64(m_file, 0, SEEK_END) != 0) {
		ei.Set(errno);
		ei.Add("at fseek(END)");
		ei.Add("at BinaryFile::Lenght()");
	}

	int64 end = _ftelli64(m_file);

	if (_fseeki64(m_file, old_pos, SEEK_SET) != 0) {
		ei.Set(errno);
		ei.Add("at fseek(POS)");
		ei.Add("at BinaryFile::Lenght()");
	}

	return end;
}

inline uint64 BinaryFile::Tell()
{
	return _ftelli64(m_file);
}

inline void BinaryFile::Seek(ErrInfo &ei, uint64 offset)
{
	bool entered_ok = ei.Succeeded();

	if (ei.Succeeded()) {
		assert(m_file != NULL);
	}

	if (ei.Succeeded() && _fseeki64(m_file, offset, SEEK_SET) != 0) {
		ei.Set(errno);
		ei.Add("at fseek()");
		ei.Add("what: offset %d [0x%08X]", offset, offset);
	}

	if (ei.Failed() && entered_ok) {
		ei.Add("at BinaryFile::ReadE()");
	}
}

inline uint32 BinaryFile::Read(ErrInfo &ei, void *buf, uint32 max_length)
{
	bool entered_ok = ei.Succeeded();

	uint32 offset = 0;
	if (ei.Succeeded()) {
		assert(m_file != NULL);
		assert((m_mode & omWrite) == 0);

		offset = ftell(m_file);
	}

	uint32 length = 0;
	if (ei.Succeeded() && (length = (uint32)fread(buf, 1, max_length, m_file)) < max_length && ferror(m_file) != 0) {
		ei.Set(errno);
		ei.Add("at fread()");
		ei.Add("offset: %d [0x%08X]", offset, offset);
		ei.Add("expected: %d bytes", max_length);
		ei.Add("read: %d bytes", length);
	}

	if (ei.Failed() && entered_ok) {
		ei.Add("at BinaryFile::Read()");
	}
	return length;
}

inline void BinaryFile::ReadExact(ErrInfo &ei, void *buf, uint32 length)
{
	bool entered_ok = ei.Succeeded();

	uint32 read_length = Read(ei, buf, length);

	if (ei.Succeeded() && read_length != length) {
		ei.Set(EOF);
		ei.Add("Unexpected end of file while reading");
		ei.Add("expected: %d bytes", length);
		ei.Add("read: %d bytes", read_length);
	}

	if (ei.Failed() && entered_ok) {
		ei.Add("at BinaryFile::ReadExact()");
	}
}

inline void BinaryFile::Write(ErrInfo &ei, const void *buf, uint32 length)
{
	bool entered_ok = ei.Succeeded();

	uint32 offset = 0;
	if (ei.Succeeded()) {
		assert(m_file != NULL);
		assert((m_mode & omWrite) != 0);

		offset = ftell(m_file);
	}

	uint32 wr_length = 0;
	if (ei.Succeeded() && (wr_length = (uint32)fwrite(buf, 1, length, m_file)) != length) {
		ei.Set(ferror(m_file) != 0 ? errno : EOF);
		ei.Add("at fread()");
		ei.Add("offset: %d [0x%08X]", offset, offset);
		ei.Add("expected: %d bytes", length);
		ei.Add("written: %d bytes", wr_length);
	}

	if (ei.Failed() && entered_ok) {
		ei.Add("at BinaryFile::Write()");
	}
}


template <typename T, class Tendianess> inline void BinaryFile::ReadE(ErrInfo &ei, T &t)
{
	bool entered_ok = ei.Succeeded();

	uint32 offset = 0;

	if (ei.Succeeded()) {
		assert(m_file != NULL);
		assert((m_mode & omWrite) == 0);

		offset = ftell(m_file);
	}

	Tendianess tmp;

	if (ei.Succeeded() && fread(&tmp.Raw(), sizeof(T), 1, m_file) != 1) {
		ei.Set(errno);
		ei.Add("at fread()");
		ei.Add("what: offset %d [0x%08X]", offset, offset);
		ei.Add("what: %d bytes", sizeof(T));
	}

	if (ei.Succeeded()) {
		t = tmp.Get();
	}

	if (ei.Failed() && entered_ok) {
		ei.Add("at BinaryFile::ReadE()");
	}
}

template <typename T> inline void BinaryFile::ReadLE(ErrInfo &ei, T &t)
{
	ReadE<T, LittleEndianT<T> >(ei, t);
}

template <typename T> inline void BinaryFile::ReadBE(ErrInfo &ei, T &t)
{
	ReadE<T, BigEndianT<T> >(ei, t);
}

template <typename T, class Tendianess> inline void BinaryFile::WriteE(ErrInfo &ei, const T &t)
{
	bool entered_ok = ei.Succeeded();

	uint32 offset = 0;

	if (ei.Succeeded()) {
		assert(m_file != NULL);
		assert((m_mode & omWrite) != 0);

		offset = ftell(m_file);
	}

	Tendianess tmp;
	tmp.Set(t);

	if (ei.Succeeded() && fwrite(&tmp.Raw(), sizeof(T), 1, m_file) != 1) {
		ei.Set(errno);
		ei.Add("at fwrite()");
		ei.Add("what: offset %d [0x%08X]", offset, offset);
		ei.Add("what: %d bytes", sizeof(T));
	}

	if (ei.Failed() && entered_ok) {
		ei.Add("at BinaryFile::WriteE()");
	}
}

template <typename T> inline void BinaryFile::WriteLE(ErrInfo &ei, const T &t)
{
	WriteE<T, LittleEndianT<T> >(ei, t);
}

template <typename T> inline void BinaryFile::WriteBE(ErrInfo &ei, const T &t)
{
	WriteE<T, BigEndianT<T> >(ei, t);
}



#endif /* FILE_BINARY_H */
