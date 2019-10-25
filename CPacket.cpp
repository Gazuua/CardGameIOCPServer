#include "CPacket.h"

CPacket::CPacket(char* data)
{
	unsigned short type, size;
	char* ptPacketContent = data + 8;
	type = CUtil::charArrayToShort(data + 4);
	size = CUtil::charArrayToShort(data + 6);
	
	this->m_PacketType = type;
	this->m_DataSize = size;
	this->m_Content = (char*)malloc(size);
	memcpy(this->m_Content, ptPacketContent, size);
}

CPacket::~CPacket()
{
	free(this->m_Content);
}

// 현재 클래스를 통신이 가능한 형태로 직렬화해주는 함수
LPPER_IO_PACKET CPacket::Encode()
{
	LPPER_IO_PACKET packet;
	char type[2], size[2];

	// 메모리를 할당한다
	packet = (LPPER_IO_PACKET)malloc(sizeof(PER_IO_PACKET));
	packet->size = this->m_DataSize + 12;
	packet->data = (char*)malloc(packet->size);

	// 헤더의 최초 4바이트 = '\0' 'e' 'n' 'd'
	packet->data[0] = '\0', packet->data[1] = 'e', packet->data[2] = 'n', packet->data[3] = 'd';

	// 헤더의 5, 6번째 바이트 = 패킷 타입
	CUtil::shortToCharArray(type, this->m_PacketType);
	memcpy(packet->data + 4, type, 2);

	// 헤더의 7, 8번째 바이트 = 내용물의 사이즈
	CUtil::shortToCharArray(size, this->m_DataSize);
	memcpy(packet->data + 6, size, 2);

	// 8바이트(헤더) 이후는 데이터 영역
	memcpy(packet->data + 8, this->m_Content, this->m_DataSize);

	// 끝 문자열 = 'd' 'n' 'e' '\0'
	packet->data[this->m_DataSize + 8] = 'd', packet->data[this->m_DataSize + 9] = 'n',
		packet->data[this->m_DataSize + 10] = 'e', packet->data[this->m_DataSize + 11] = '\0';

	return packet;
}

unsigned short CPacket::GetPacketType()
{
	return this->m_PacketType;
}

unsigned short CPacket::GetDataSize()
{
	return this->m_DataSize;
}

char* CPacket::GetContent()
{
	return this->m_Content;
}

CStandardPacketContent::CStandardPacketContent(char* data)
{
	this->SetCommand(data);
}

string CStandardPacketContent::GetCommand()
{
	return this->m_Command;
}

void CStandardPacketContent::SetCommand(string command)
{
	this->m_Command = command;
}