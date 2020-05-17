/* $Id:$ */

#ifndef ERR_INFO_H
#define ERR_INFO_H

struct ErrInfo {
	uint32     m_errno;
	CStrA      m_errmsg;

	ErrInfo()
		: m_errno(0)
		, m_errmsg()
	{}

	bool Succeeded() const;
	bool Failed() const;
	void Resume();
	void Set(uint32 err_no, const char *format, ...);
	void Add(const char *format, ...);
};

inline bool ErrInfo::Succeeded() const
{
	return (m_errno == 0) && (m_errmsg.Size() == 0);
}

inline bool ErrInfo::Failed() const
{
	return !Succeeded();
}

inline void ErrInfo::Resume()
{
	m_errno = 0;
	m_errmsg.Free();
}

inline void ErrInfo::Set(uint32 err_no, const char *format = "", ...)
{
	if (m_errno == 0) {
		m_errno = err_no;
	} else {
		m_errmsg.AddFormat("Error #%d\n", err_no);
	}
	va_list args;
	va_start(args, format);
	m_errmsg.AddFormatL(format, args);
	m_errmsg.AppendStr("\n");
	va_end(args);
}

inline void ErrInfo::Add(const char *format, ...)
{
	m_errmsg.AppendStr(" ");
	va_list args;
	va_start(args, format);
	m_errmsg.AddFormatL(format, args);
	va_end(args);
	m_errmsg.AppendStr("\n");
}

#endif /* ERR_INFO_H */
