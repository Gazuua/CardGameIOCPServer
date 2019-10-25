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

// ���� Ŭ������ ����� ������ ���·� ����ȭ���ִ� �Լ�
LPPER_IO_PACKET CPacket::Encode()
{
	LPPER_IO_PACKET packet;
	char type[2], size[2];

	// �޸𸮸� �Ҵ��Ѵ�
	packet = (LPPER_IO_PACKET)malloc(sizeof(PER_IO_PACKET));
	packet->size = this->m_DataSize + 12;
	packet->data = (char*)malloc(packet->size);

	// ����� ���� 4����Ʈ = '\0' 'e' 'n' 'd'
	packet->data[0] = '\0', packet->data[1] = 'e', packet->data[2] = 'n', packet->data[3] = 'd';

	// ����� 5, 6��° ����Ʈ = ��Ŷ Ÿ��
	CUtil::shortToCharArray(type, this->m_PacketType);
	memcpy(packet->data + 4, type, 2);

	// ����� 7, 8��° ����Ʈ = ���빰�� ������
	CUtil::shortToCharArray(size, this->m_DataSize);
	memcpy(packet->data + 6, size, 2);

	// 8����Ʈ(���) ���Ĵ� ������ ����
	memcpy(packet->data + 8, this->m_Content, this->m_DataSize);

	// �� ���ڿ� = 'd' 'n' 'e' '\0'
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