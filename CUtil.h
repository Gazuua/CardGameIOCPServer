#pragma once

// CUtil.h :: ���α׷� �������� ���̴� �͵��� ������ ���

// 1�� ��Ŷ�� �� ����� ��� ����ü
typedef struct
{
	char* data;		// ����� ���� ���Ǵ� char �迭
	int size;		// �迭 ��ü �������� ũ�⸦ ǥ���ϴ� int�� ����
}PER_IO_PACKET, * LPPER_IO_PACKET;

// CUtil :: ����̳� �� �ܿ� ���� �� �ִ� �ٿ뵵 �Լ��� �����س��� Ŭ����
class CUtil
{
public:
	static void shortToCharArray(char* dst, unsigned short n);
	static unsigned short charArrayToShort(char* arr);
};

