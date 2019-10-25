#pragma once

// CUtil.h :: 프로그램 전역에서 쓰이는 것들을 선언한 헤더

// 1개 패킷과 그 사이즈를 담는 구조체
typedef struct
{
	char* data;		// 통신을 위해 사용되는 char 배열
	int size;		// 배열 전체 데이터의 크기를 표현하는 int형 변수
}PER_IO_PACKET, * LPPER_IO_PACKET;

// CUtil :: 통신이나 그 외에 쓰일 수 있는 다용도 함수를 저장해놓은 클래스
class CUtil
{
public:
	static void shortToCharArray(char* dst, unsigned short n);
	static unsigned short charArrayToShort(char* arr);
};

