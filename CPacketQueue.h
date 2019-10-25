#pragma once
#include<iostream>
#include<list>
#include<queue>

#include"CUtil.h"
#include"CPacket.h"
using namespace std;

// 패킷 헤더 및 끝이 있는 위치와 인덱스를 표현하는 구조체
typedef struct
{
	int listIndex;	// 리스트의 몇 번째 인덱스에 헤더가 있는지 표현하는 int형 변수
	int arrayIndex;	// char 배열의 몇 번째 인덱스에 헤더가 있는지 표현하는 int형 변수
}INDEX;

// CPacketQueue :: WSARecv로 전송된 패킷을 임시 저장 및 가공하여 반환하는 클래스
class CPacketQueue
{
public:
	CPacketQueue();
	~CPacketQueue();

	int OnRecv(char * data, int recvBytes);		// WSARecv 결과를 받았을 때 외부에서 호출되는 함수
	CPacket* getPacket();						// 사용 가능한 패킷을 밖으로 꺼내는 함수

	// 통신 프로토콜
	// 헤더는 총 8바이트로 구성된다.
	// >> 최초 4바이트 -> '\0' 'e' 'n' 'd'의 고유 문자로 구성된다.
	// ex) 최초 4바이트에서 '\0' 'e' 'n' 'd' 발견 시 올바른 프로토콜로 간주
	// >> 중간 4바이트 -> 최초 2바이트는 패킷 타입, 최후 2바이트는 데이터 영역의 크기를 나타낸다.
	// 헤더 이후에는 데이터 영역이며, 크기 제한은 없으나 큰 데이터가 갈 일도 없을 듯.
	// >> 데이터 영역의 끝을 알리는 4바이트 -> 'd' 'n' 'e' '\0' 발견 시 패킷 분석 마무리.

	// 0. 최초 데이터가 들어왔을 경우 (큐에 아무것도 없을 때)
	// 우선 헤더부터 검사하여 이 패킷이 올바른 프로토콜에 의한 것인지 검증한다.
	// 헤더에 이상한 값이 들어있을 경우 올바르지 못한 프로토콜에 의한 통신이므로 차단한다
	// >> 데이터 순서가 꼬일 경우 ?? >> TCP는 그런거 없으므로 헤더부터 검사.
	// ex) 데이터의 최초 인덱스부터 

	// 경우의 수 두가지 -> 데이터가 잘려서 옴 / 붙어서 옴

	// 1. 1개 배열이 잘려서 왔을 경우 (끝을 알리는 문자배열이 없을 경우)
	// 1A -> 다음 패킷을 받아야 하므로 데이터 추가에 대한 타임아웃을 걸고 대기한다.
	// ex) n초 안에 다음 패킷이 들어오지 않으면 통신 에러로 간주 접속종료
	// 단, 타임아웃만 걸어놓으면 계속 올바르지 않은 데이터가 들어와 접속이 유지될 수 있으므로
	// 이상한 데이터가 들어오면 접속종료를 시키는 알고리즘도 추가한다.

	// 1.1. 2~n번째 배열에서 끝을 알리는 문자배열을 찾았을 경우
	// 1.1A -> 2~n번째 배열까지의 데이터를 합치고 헤더에 정해진 대로 데이터를 꺼낸다.
	// 만약 단위 패킷 이외의 잡스런 데이터가 남아있으면 현재 패킷에 대한 후처리 이후
	// 최초단계부터 다시 수행한다.
	
	// 1.2. 헤더는 찾았으나 끝을 계속해서 못찾으면??
	// 1.2A -> 타임아웃이 걸려 알아서 나가리될 것이니 신경 안써도 된다.

	// 2. 데이터가 붙어서 옴
	// 2A -> 붙어서 와도 한 덩어리가 제대로 있으면 분석이 가능하므로 검사 후 후처리를 한다.
	// 단위 데이터를 떼어놓고 후처리 이후 다음 단위 데이터를 또 분석하는 식으로 하면 된다.

private:
	list<LPPER_IO_PACKET>			m_PacketList;
	queue<CPacket*>					m_AvailablePacketList;
	INDEX							m_HeadIndex;
	unsigned short					m_Payload;
	bool							m_bEndChecked;
	bool							m_bFatalError;

	void init();
	void analysis();
};