#pragma once
#include <string>

#define INFINITE 0xFFFFFFFF
#define JSON_CHECK(root,name,func) (root.isMember(name) && root[name].func())

enum enErrCode
{
	ERR_OK = 0,
	ERR_INTERNAL,
	ERR_DB,
	ERR_WRITE_FILE,
};