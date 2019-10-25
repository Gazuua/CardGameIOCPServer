#include "CUtil.h"

void CUtil::shortToCharArray(char* dst, unsigned short n)
{
	dst[0] = (char)((n >> 8) & 0xFF);
	dst[1] = (char)((n >> 0) & 0xFF);
}

unsigned short CUtil::charArrayToShort(char* arr)
{
	return (short)(((arr[0] & 0xFF) << 8) + (arr[1] & 0xFF));
}