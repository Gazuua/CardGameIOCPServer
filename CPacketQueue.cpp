#include "CPacketQueue.h"

CPacketQueue::CPacketQueue()
{
	this->m_PacketList.clear();
	this->init();
}

CPacketQueue::~CPacketQueue()
{
	
}

// WSARecv가 1회 호출될 때마다 수행되는 public 함수
int CPacketQueue::OnRecv(char* data, int recvBytes)
{	
	// 헤더조차 제대로 들어오지 않았다면
	// 네트워크가 불안정하거나, 제대로 된 프로토콜이 아닐 확률이 높으므로
	// 안전하게 접속을 차단시키도록 한다.
	if (recvBytes <= 8)
		return -1;

	// 우선 받은 패킷을 별도 할당한 메모리에 복사하며
	LPPER_IO_PACKET packet = (LPPER_IO_PACKET)malloc(sizeof(PER_IO_PACKET));

	packet->data = (char*)malloc(recvBytes);
	memset(packet->data, 0, recvBytes);
	memcpy(packet->data, data, recvBytes);
	packet->size = recvBytes;

	// 그것을 리스트에 추가한다.
	this->m_PacketList.push_back(packet);

	// 패킷을 리스트에 넣은 후 곧바로 분석을 시작한다.
	// private으로 선언된 이 분석 함수는 필요시 내부에서 재귀적으로 반복 수행되므로 1번만 호출하면 된다.
	this->analysis();

	// 분석이 끝나면 오류 여부에 따라 반환값을 달리한다.
	if (m_bFatalError)
	{
		this->init();
		return -1;
	}
	
	// 정상 수행 시 현재 즉시 작업이 가능한 패킷의 숫자를 반환한다.
	this->init();
	return this->m_AvailablePacketList.size();
}

CPacket* CPacketQueue::getPacket()
{
	CPacket* ret = this->m_AvailablePacketList.front();
	this->m_AvailablePacketList.pop();
	return ret;
}

// 1회 분석에 필요한 모든 데이터를 초기화시키는 함수이다.
void CPacketQueue::init()
{
	m_HeadIndex = { -1, -1 };
	m_Payload = 0;
	m_bEndChecked = false;
	this->m_bFatalError = false;
}

// 현재 리스트 내에 있는 모든 패킷을 분석하여 단위 패킷으로 분할 및 병합하는 함수이다.
void CPacketQueue::analysis()
{
	LPPER_IO_PACKET packet = this->m_PacketList.front();
	char* temp = NULL;


	// 처음 들어온(지금 받은) 패킷만 존재할 경우
	if (this->m_PacketList.size() == 1)
	{
		// 이게 골때리는 경우인데 패킷이 애매하게 붙어와서 헤더가 짤리는 경우이다.
		// 이 경우 다음 패킷이 들어올 때까지 기다린다.
		if (packet->size < 8) return;

		// 헤더 문자열 체크(4바이트)
		// 헤더는 패킷의 맨 앞에 있어야 하므로 항상 첫 부분만 검사한다
		if (packet->data[0] == '\0')
			if (packet->data[1] == 'e')
				if (packet->data[2] == 'n')
					if (packet->data[3] == 'd')
						this->m_HeadIndex = { 0, 0 };

		// 헤더를 찾지 못했을 경우 에러처리로 연결을 종료해야 한다
		if (m_HeadIndex.arrayIndex == -1) {
			m_bFatalError = true;
			return;
		}

		// 헤더 내용 체크(4바이트)
		// 앞의 2바이트는 패킷 타입, 뒤의 2바이트는 패킷 내 데이터 영역의 크기를 나타낸다.
		// 여기서는 데이터 영역의 크기 값만 필요함에 유의.
		m_Payload = CUtil::charArrayToShort(packet->data + 6);

		// 데이터 끝 체크(4바이트)
		// payload에 헤더를 합친 값이 클 경우 패킷이 덜 들어왔다는 뜻이므로 그냥 넘기고
		// 같은 경우는 한 덩어리만 정확하게 들어왔다는 뜻이며,
		// 작은 경우는 여러 패킷이 하나로 붙어왔다는 뜻인데
		// 어차피 나중에 후처리해 줄 것이므로 뒤의 두 조건은 같은 분기로 넘긴다.
		if (m_Payload + 12 <= packet->size)
		{
			// 이 경우 마지막으로 끝 문자열을 검사하여 최종 검증을 완료한다.
			if (packet->data[m_Payload + 11] == '\0')
				if (packet->data[m_Payload + 10] == 'e')
					if (packet->data[m_Payload + 9] == 'n')
						if (packet->data[m_Payload + 8] == 'd')
							this->m_bEndChecked = true;
		}

		// 검사 결과 완전한 패킷이 존재할 경우 클래스화하여 worker에서 빼서 쓸 수 있도록 저장한다
		if (m_bEndChecked && m_Payload != 0)
		{
			// 1회 분석 당 1개의 완전한 패킷이 분리된다.
			// 일단 현재 확인된 완전한 패킷 하나를 클래스화하여 완성시켜 주자.
			CPacket* cPacket = new CPacket(packet->data);
			if (cPacket != NULL)
				this->m_AvailablePacketList.push(cPacket);

			// 패킷을 클래스화했다면 현재 리스트 내에 다른 데이터가 아직 남아있는지 확인한다.
			if (m_Payload + 12 < packet->size)
			{
				// 남아있다면 그 남은 만큼 메모리를 재할당하여 복사해 두고
				// 기존 메모리는 릴리즈한다.
				char* remain = (char*)malloc(packet->size - (m_Payload + 12));
				memcpy(remain, packet->data + m_Payload + 12, packet->size - (m_Payload + 12));
				free(packet->data);
				packet->data = remain;
				packet->size = packet->size - (m_Payload + 12);
			}
			// 한 덩어리의 패킷만 정확하게 들어왔으면 더 분석할 남은 것은 없으므로 pop 및 free 해준다.
			else if (m_Payload + 12 == packet->size)
			{
				m_PacketList.pop_front();
				free(packet->data);
				free(packet);
			}

			// 1개 패킷 분석 및 분리가 완료되었다면 다음 분석을 위해 데이터를 초기화한다.
			this->init();

			// 미가공된 패킷이 남아있다면 분석을 재개한다.
			if (!this->m_PacketList.empty())
				this->analysis();
		}

		// 만약 이 조건으로 분기되었다면, 헤더는 찾았으나 끝 문자열을 찾지 못한 것이므로
		// 다음 WSARecv까지 기다려야 한다.
		// 여기다 Timeout 기능을 붙일까 하는데 일단 냅두자.
	}
	// 이미 리스트 안에 다른 패킷이 더 들어있었을 경우
	else if (this->m_PacketList.size() > 1)
	{
		// 우선 안에 패킷 내용들부터 검사해야 한다.
		list<LPPER_IO_PACKET>::iterator iter;
		int sum = 0;
		for (iter = m_PacketList.begin(); iter != m_PacketList.end(); iter++)
			sum += (*iter)->size;

		// 우선 패킷 내용의 검증을 위해서는 데이터를 이어붙이는 것이 좋다.
		char* mergeData = (char*)malloc(sum);
		char* temp = mergeData;
		for (iter = m_PacketList.begin(); iter != m_PacketList.end(); iter++)
		{
			memcpy(mergeData, (*iter)->data, (*iter)->size);
			mergeData += (*iter)->size;
		}
		mergeData = temp;

		// 이어붙인 새로운 패킷 덩어리를 메모리 할당하여 준다.
		LPPER_IO_PACKET mergePacket = (LPPER_IO_PACKET)malloc(sizeof(PER_IO_PACKET));
		mergePacket->data = mergeData;
		mergePacket->size = sum;

		// 다 이어붙였으면 기존 리스트에 있던 분리된 패킷 조각들을 싹 날려버린다.
		while (!m_PacketList.empty())
		{
			free(m_PacketList.front()->data);
			free(m_PacketList.front());
			m_PacketList.pop_front();
		}

		// 헤더를 아직 찾지 못한 경우가 있을 수도 있다.
		if (this->m_HeadIndex.arrayIndex == -1)
		{
			// 그러나 여전히 8바이트가 안된다면 병합한 패킷을 도로 집어넣고 다음 패킷을 기다려야 한다.
			if (sum < 8) 
			{
				this->m_PacketList.push_back(mergePacket);
				return;
			}
			// 8바이트가 넘으면 최소한 헤더 검사는 할 수 있다는 뜻이므로 검사해 준다.
			else 
			{
				if (mergeData[0] == '\0')
					if (mergeData[1] == 'e')
						if (mergeData[2] == 'n')
							if (mergeData[3] == 'd')
								this->m_HeadIndex = { 0, 0 };

				// 헤더를 찾지 못했을 경우 에러처리로 연결을 종료해야 한다
				if (m_HeadIndex.arrayIndex == -1) {
					m_bFatalError = true;
					return;
				}

				// 헤더 내용 체크(4바이트)
				// 앞의 2바이트는 패킷 타입, 뒤의 2바이트는 패킷 내 데이터 영역의 크기를 나타낸다.
				// 여기서는 데이터 영역의 크기 값만 필요함에 유의.
				m_Payload = CUtil::charArrayToShort(mergePacket->data + 6);
			}
		}

		// 또는 현재 패킷에 endIndex가 없으면 다음 WSARecv를 기다리기 위해 바로 리턴한다.
		if (m_Payload + 12 > sum) {
			puts("PENDING!!");
			return;
		}

		// 이어붙인 데이터의 인덱스를 활용하여 끝 문자열을 검사한다.
		if (mergeData[m_Payload + 11] == '\0')
			if (mergeData[m_Payload + 10] == 'e')
				if (mergeData[m_Payload + 9] == 'n')
					if (mergeData[m_Payload + 8] == 'd')
						this->m_bEndChecked = true;

		// 검사 결과 완전한 패킷이 존재할 경우 클래스화하여 worker에서 빼서 쓸 수 있도록 저장한다
		if (m_bEndChecked && m_Payload != 0)
		{
			// 1회 분석 당 1개의 완전한 패킷이 분리된다.
			// 일단 현재 확인된 완전한 패킷 하나를 클래스화하여 완성시켜 주자.
			CPacket* cPacket = new CPacket(mergeData);
			if (cPacket != NULL)
				this->m_AvailablePacketList.push(cPacket);

			// 패킷을 클래스화했다면 병합한 데이터 이외에 남는 것이 있는가 확인한다.
			if (m_Payload + 12 < sum)
			{
				// 클래스화한 패킷 이외에도 남아있다면 다시 패킷리스트에 집어넣어 준다.
				char* remain = (char*)malloc(mergePacket->size - (m_Payload + 12));
				memcpy(remain, mergePacket->data + m_Payload + 12, mergePacket->size - (m_Payload + 12));
				free(mergePacket->data);
				mergePacket->data = remain;
				mergePacket->size = mergePacket->size - (m_Payload + 12);
				m_PacketList.push_back(mergePacket);
			}

			// 아까 위에서 패킷 조각들을 리스트에서 다 날려버렸으므로 후속 작업은 필요없고
			// 1개 패킷 분석 및 분리가 완료되었다면 다음 분석을 위해 데이터를 초기화한다.
			this->init();

			// 미가공된 패킷이 남아있다면 분석을 재개한다.
			if (!this->m_PacketList.empty())
				this->analysis();
		}
	}
}