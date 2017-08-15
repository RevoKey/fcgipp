#include "CodeConverter.h"

CodeConverter::CodeConverter(const char *from_charset, const char *to_charset)
{
	cd = iconv_open(to_charset, from_charset);
}

CodeConverter::~CodeConverter()
{
	iconv_close(cd);
}

int CodeConverter::convert(char *inbuf, size_t inlen, char *outbuf, int outlen)
{
	char *pin = inbuf;
	char *pout = outbuf;
	size_t out_left = outlen;
	if (-1 == iconv(cd, &pin, &inlen, &pout, &out_left))
	{
		return -1;
	}
	auto count = outlen - out_left;
	outbuf[count] = 0;
	return count;
}