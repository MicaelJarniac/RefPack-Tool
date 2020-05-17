/* $Id$ */

#ifndef  STR_HPP
#define  STR_HPP

#include <errno.h>
#include <stdarg.h>

// simple string implementation
template <typename Tchar, bool TcaseInsensitive>
struct CStrT : public CBlobT<Tchar>
{
	typedef CBlobT<Tchar> base;
	typedef CStrApiT<Tchar, TcaseInsensitive> Api;
	typedef typename base::bsize_t bsize_t;
	typedef typename base::OnTransfer OnTransfer;


	FORCEINLINE CStrT(const Tchar* str = NULL)
	{
		AppendStr(str);
	}

	FORCEINLINE CStrT(const Tchar* str, bsize_t num_chars) : base(str, num_chars)
	{
		base::FixTail();
	}

	FORCEINLINE CStrT(const Tchar* str, const Tchar* end)
		: base(str, end - str)
	{
		base::FixTail();
	}

	FORCEINLINE CStrT(const CBlobBaseSimple& src)
		: base(src)
	{
		base::FixTail();
	}
	
	FORCEINLINE CStrT(const CStrT& src)
		: base(src)
	{
		base::FixTail();
	}

	/** Take ownership constructor */
	FORCEINLINE CStrT(const OnTransfer& ot)
		: base(ot)
	{
	}

	FORCEINLINE Tchar* GrowSizeNC(bsize_t count)
	{
		Tchar* ret = base::GrowSizeNC(count);
		base::FixTail();
		return ret;
	}

	FORCEINLINE void AppendStr(const Tchar* str)
	{
		if (str != NULL && str[0] != '\0') {
			base::Append(str, (bsize_t)Api::StrLen(str));
			base::FixTail();
		}
	}

	FORCEINLINE CStrT& operator = (const Tchar* src)
	{
		base::Clear();
		AppendStr(src);
		return *this;
	}

	FORCEINLINE bool operator < (const CStrT &other) const
	{
		return (Api::StrCmp(base::Data(), other.Data()) < 0);
	}

	int AddFormatL(const Tchar *format, va_list args)
	{
		bsize_t addSize = Api::StrLen(format);
		if (addSize < 16) addSize = 16;
		addSize += addSize > 1;
		int ret;
		do {
			Tchar *buf = MakeFreeSpace(addSize);
			ret = Api::SPrintFL(buf, base::GetReserve(), format, args);
			addSize *= 2;
		} while(ret < 0 && (errno == ERANGE || errno == 0));
		if (ret > 0) {
			GrowSizeNC(ret);
		} else {
//			int err = errno;
			base::FixTail();
		}
		return ret;
	}

	int AddFormat(const Tchar *format, ...)
	{
		va_list args;
		va_start(args, format);
		int ret = AddFormatL(format, args);
		va_end(args);
		return ret;
	}

	int FormatL(const Tchar *format, va_list args)
	{
		Free();
		AddFormatL(format, args);
	}

	int Format(const Tchar *format, ...)
	{
		Free();
		va_list args;
		va_start(args, format);
		int ret = AddFormatL(format, args);
		va_end(args);
		return ret;
	}
};

typedef CStrT<char   , false> CStrA;
typedef CStrT<char   , true > CStrCiA;
typedef CStrT<wchar_t, false> CStrW;
typedef CStrT<wchar_t, true > CStrCiW;

#endif /* STR_HPP */
