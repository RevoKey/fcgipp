#pragma once

#include <iconv.h>

class CodeConverter
{
public:
	CodeConverter(const char *from_charset, const char *to_charset);
	~CodeConverter();
	int convert(char *inbuf, size_t inlen, char *outbuf, int outlen);
private:
	iconv_t cd;
};

