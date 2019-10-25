#include "CPacketQueue.h"

CPacketQueue::CPacketQueue()
{
	this->m_PacketList.clear();
	this->init();
}

CPacketQueue::~CPacketQueue()
{
	
}

// WSARecv�� 1ȸ ȣ��� ������ ����Ǵ� public �Լ�
int CPacketQueue::OnRecv(char* data, int recvBytes)
{	
	// ������� ����� ������ �ʾҴٸ�
	// ��Ʈ��ũ�� �Ҿ����ϰų�, ����� �� ���������� �ƴ� Ȯ���� �����Ƿ�
	// �����ϰ� ������ ���ܽ�Ű���� �Ѵ�.
	if (recvBytes <= 8)
		return -1;

	// �켱 ���� ��Ŷ�� ���� �Ҵ��� �޸𸮿� �����ϸ�
	LPPER_IO_PACKET packet = (LPPER_IO_PACKET)malloc(sizeof(PER_IO_PACKET));

	packet->data = (char*)malloc(recvBytes);
	memset(packet->data, 0, recvBytes);
	memcpy(packet->data, data, recvBytes);
	packet->size = recvBytes;

	// �װ��� ����Ʈ�� �߰��Ѵ�.
	this->m_PacketList.push_back(packet);

	// ��Ŷ�� ����Ʈ�� ���� �� ��ٷ� �м��� �����Ѵ�.
	// private���� ����� �� �м� �Լ��� �ʿ�� ���ο��� ��������� �ݺ� ����ǹǷ� 1���� ȣ���ϸ� �ȴ�.
	this->analysis();

	// �м��� ������ ���� ���ο� ���� ��ȯ���� �޸��Ѵ�.
	if (m_bFatalError)
	{
		this->init();
		return -1;
	}
	
	// ���� ���� �� ���� ��� �۾��� ������ ��Ŷ�� ���ڸ� ��ȯ�Ѵ�.
	this->init();
	return this->m_AvailablePacketList.size();
}

CPacket* CPacketQueue::getPacket()
{
	CPacket* ret = this->m_AvailablePacketList.front();
	this->m_AvailablePacketList.pop();
	return ret;
}

// 1ȸ �м��� �ʿ��� ��� �����͸� �ʱ�ȭ��Ű�� �Լ��̴�.
void CPacketQueue::init()
{
	m_HeadIndex = { -1, -1 };
	m_Payload = 0;
	m_bEndChecked = false;
	this->m_bFatalError = false;
}

// ���� ����Ʈ ���� �ִ� ��� ��Ŷ�� �м��Ͽ� ���� ��Ŷ���� ���� �� �����ϴ� �Լ��̴�.
void CPacketQueue::analysis()
{
	LPPER_IO_PACKET packet = this->m_PacketList.front();
	char* temp = NULL;


	// ó�� ����(���� ����) ��Ŷ�� ������ ���
	if (this->m_PacketList.size() == 1)
	{
		// �̰� �񶧸��� ����ε� ��Ŷ�� �ָ��ϰ� �پ�ͼ� ����� ©���� ����̴�.
		// �� ��� ���� ��Ŷ�� ���� ������ ��ٸ���.
		if (packet->size < 8) return;

		// ��� ���ڿ� üũ(4����Ʈ)
		// ����� ��Ŷ�� �� �տ� �־�� �ϹǷ� �׻� ù �κи� �˻��Ѵ�
		if (packet->data[0] == '\0')
			if (packet->data[1] == 'e')
				if (packet->data[2] == 'n')
					if (packet->data[3] == 'd')
						this->m_HeadIndex = { 0, 0 };

		// ����� ã�� ������ ��� ����ó���� ������ �����ؾ� �Ѵ�
		if (m_HeadIndex.arrayIndex == -1) {
			m_bFatalError = true;
			return;
		}

		// ��� ���� üũ(4����Ʈ)
		// ���� 2����Ʈ�� ��Ŷ Ÿ��, ���� 2����Ʈ�� ��Ŷ �� ������ ������ ũ�⸦ ��Ÿ����.
		// ���⼭�� ������ ������ ũ�� ���� �ʿ��Կ� ����.
		m_Payload = CUtil::charArrayToShort(packet->data + 6);

		// ������ �� üũ(4����Ʈ)
		// payload�� ����� ��ģ ���� Ŭ ��� ��Ŷ�� �� ���Դٴ� ���̹Ƿ� �׳� �ѱ��
		// ���� ���� �� ����� ��Ȯ�ϰ� ���Դٴ� ���̸�,
		// ���� ���� ���� ��Ŷ�� �ϳ��� �پ�Դٴ� ���ε�
		// ������ ���߿� ��ó���� �� ���̹Ƿ� ���� �� ������ ���� �б�� �ѱ��.
		if (m_Payload + 12 <= packet->size)
		{
			// �� ��� ���������� �� ���ڿ��� �˻��Ͽ� ���� ������ �Ϸ��Ѵ�.
			if (packet->data[m_Payload + 11] == '\0')
				if (packet->data[m_Payload + 10] == 'e')
					if (packet->data[m_Payload + 9] == 'n')
						if (packet->data[m_Payload + 8] == 'd')
							this->m_bEndChecked = true;
		}

		// �˻� ��� ������ ��Ŷ�� ������ ��� Ŭ����ȭ�Ͽ� worker���� ���� �� �� �ֵ��� �����Ѵ�
		if (m_bEndChecked && m_Payload != 0)
		{
			// 1ȸ �м� �� 1���� ������ ��Ŷ�� �и��ȴ�.
			// �ϴ� ���� Ȯ�ε� ������ ��Ŷ �ϳ��� Ŭ����ȭ�Ͽ� �ϼ����� ����.
			CPacket* cPacket = new CPacket(packet->data);
			if (cPacket != NULL)
				this->m_AvailablePacketList.push(cPacket);

			// ��Ŷ�� Ŭ����ȭ�ߴٸ� ���� ����Ʈ ���� �ٸ� �����Ͱ� ���� �����ִ��� Ȯ���Ѵ�.
			if (m_Payload + 12 < packet->size)
			{
				// �����ִٸ� �� ���� ��ŭ �޸𸮸� ���Ҵ��Ͽ� ������ �ΰ�
				// ���� �޸𸮴� �������Ѵ�.
				char* remain = (char*)malloc(packet->size - (m_Payload + 12));
				memcpy(remain, packet->data + m_Payload + 12, packet->size - (m_Payload + 12));
				free(packet->data);
				packet->data = remain;
				packet->size = packet->size - (m_Payload + 12);
			}
			// �� ����� ��Ŷ�� ��Ȯ�ϰ� �������� �� �м��� ���� ���� �����Ƿ� pop �� free ���ش�.
			else if (m_Payload + 12 == packet->size)
			{
				m_PacketList.pop_front();
				free(packet->data);
				free(packet);
			}

			// 1�� ��Ŷ �м� �� �и��� �Ϸ�Ǿ��ٸ� ���� �м��� ���� �����͸� �ʱ�ȭ�Ѵ�.
			this->init();

			// �̰����� ��Ŷ�� �����ִٸ� �м��� �簳�Ѵ�.
			if (!this->m_PacketList.empty())
				this->analysis();
		}

		// ���� �� �������� �б�Ǿ��ٸ�, ����� ã������ �� ���ڿ��� ã�� ���� ���̹Ƿ�
		// ���� WSARecv���� ��ٷ��� �Ѵ�.
		// ����� Timeout ����� ���ϱ� �ϴµ� �ϴ� ������.
	}
	// �̹� ����Ʈ �ȿ� �ٸ� ��Ŷ�� �� ����־��� ���
	else if (this->m_PacketList.size() > 1)
	{
		// �켱 �ȿ� ��Ŷ �������� �˻��ؾ� �Ѵ�.
		list<LPPER_IO_PACKET>::iterator iter;
		int sum = 0;
		for (iter = m_PacketList.begin(); iter != m_PacketList.end(); iter++)
			sum += (*iter)->size;

		// �켱 ��Ŷ ������ ������ ���ؼ��� �����͸� �̾���̴� ���� ����.
		char* mergeData = (char*)malloc(sum);
		char* temp = mergeData;
		for (iter = m_PacketList.begin(); iter != m_PacketList.end(); iter++)
		{
			memcpy(mergeData, (*iter)->data, (*iter)->size);
			mergeData += (*iter)->size;
		}
		mergeData = temp;

		// �̾���� ���ο� ��Ŷ ����� �޸� �Ҵ��Ͽ� �ش�.
		LPPER_IO_PACKET mergePacket = (LPPER_IO_PACKET)malloc(sizeof(PER_IO_PACKET));
		mergePacket->data = mergeData;
		mergePacket->size = sum;

		// �� �̾�ٿ����� ���� ����Ʈ�� �ִ� �и��� ��Ŷ �������� �� ����������.
		while (!m_PacketList.empty())
		{
			free(m_PacketList.front()->data);
			free(m_PacketList.front());
			m_PacketList.pop_front();
		}

		// ����� ���� ã�� ���� ��찡 ���� ���� �ִ�.
		if (this->m_HeadIndex.arrayIndex == -1)
		{
			// �׷��� ������ 8����Ʈ�� �ȵȴٸ� ������ ��Ŷ�� ���� ����ְ� ���� ��Ŷ�� ��ٷ��� �Ѵ�.
			if (sum < 8) 
			{
				this->m_PacketList.push_back(mergePacket);
				return;
			}
			// 8����Ʈ�� ������ �ּ��� ��� �˻�� �� �� �ִٴ� ���̹Ƿ� �˻��� �ش�.
			else 
			{
				if (mergeData[0] == '\0')
					if (mergeData[1] == 'e')
						if (mergeData[2] == 'n')
							if (mergeData[3] == 'd')
								this->m_HeadIndex = { 0, 0 };

				// ����� ã�� ������ ��� ����ó���� ������ �����ؾ� �Ѵ�
				if (m_HeadIndex.arrayIndex == -1) {
					m_bFatalError = true;
					return;
				}

				// ��� ���� üũ(4����Ʈ)
				// ���� 2����Ʈ�� ��Ŷ Ÿ��, ���� 2����Ʈ�� ��Ŷ �� ������ ������ ũ�⸦ ��Ÿ����.
				// ���⼭�� ������ ������ ũ�� ���� �ʿ��Կ� ����.
				m_Payload = CUtil::charArrayToShort(mergePacket->data + 6);
			}
		}

		// �Ǵ� ���� ��Ŷ�� endIndex�� ������ ���� WSARecv�� ��ٸ��� ���� �ٷ� �����Ѵ�.
		if (m_Payload + 12 > sum) {
			puts("PENDING!!");
			return;
		}

		// �̾���� �������� �ε����� Ȱ���Ͽ� �� ���ڿ��� �˻��Ѵ�.
		if (mergeData[m_Payload + 11] == '\0')
			if (mergeData[m_Payload + 10] == 'e')
				if (mergeData[m_Payload + 9] == 'n')
					if (mergeData[m_Payload + 8] == 'd')
						this->m_bEndChecked = true;

		// �˻� ��� ������ ��Ŷ�� ������ ��� Ŭ����ȭ�Ͽ� worker���� ���� �� �� �ֵ��� �����Ѵ�
		if (m_bEndChecked && m_Payload != 0)
		{
			// 1ȸ �м� �� 1���� ������ ��Ŷ�� �и��ȴ�.
			// �ϴ� ���� Ȯ�ε� ������ ��Ŷ �ϳ��� Ŭ����ȭ�Ͽ� �ϼ����� ����.
			CPacket* cPacket = new CPacket(mergeData);
			if (cPacket != NULL)
				this->m_AvailablePacketList.push(cPacket);

			// ��Ŷ�� Ŭ����ȭ�ߴٸ� ������ ������ �̿ܿ� ���� ���� �ִ°� Ȯ���Ѵ�.
			if (m_Payload + 12 < sum)
			{
				// Ŭ����ȭ�� ��Ŷ �̿ܿ��� �����ִٸ� �ٽ� ��Ŷ����Ʈ�� ����־� �ش�.
				char* remain = (char*)malloc(mergePacket->size - (m_Payload + 12));
				memcpy(remain, mergePacket->data + m_Payload + 12, mergePacket->size - (m_Payload + 12));
				free(mergePacket->data);
				mergePacket->data = remain;
				mergePacket->size = mergePacket->size - (m_Payload + 12);
				m_PacketList.push_back(mergePacket);
			}

			// �Ʊ� ������ ��Ŷ �������� ����Ʈ���� �� �����������Ƿ� �ļ� �۾��� �ʿ����
			// 1�� ��Ŷ �м� �� �и��� �Ϸ�Ǿ��ٸ� ���� �м��� ���� �����͸� �ʱ�ȭ�Ѵ�.
			this->init();

			// �̰����� ��Ŷ�� �����ִٸ� �м��� �簳�Ѵ�.
			if (!this->m_PacketList.empty())
				this->analysis();
		}
	}
}