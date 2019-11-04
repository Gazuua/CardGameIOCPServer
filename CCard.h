#pragma once
#include<iostream>

using namespace std;

// 망통~갑오
#define CARD_MANGTONG 0
#define CARD_HANKUT 1
#define CARD_DUKUT 2
#define CARD_SEKUT 3
#define CARD_NEKUT 4
#define CARD_DASUTKUT 5
#define CARD_YUSUTKUT 6
#define CARD_ILGOPKUT 7
#define CARD_YUDULKUT 8
#define CARD_GABO 9

// 세륙~알리
#define CARD_SERYUK 101
#define CARD_JANGSA 102
#define CARD_JANGBBING 103
#define CARD_GUBBING 104
#define CARD_ALLI 105

// 삥땡~장땡
#define CARD_BBINGTTAENG 201
#define CARD_YITTAENG 202
#define CARD_SAMTTAENG 203
#define CARD_SATTAENG 204
#define CARD_OTTAENG 205
#define CARD_YUKTTAENG 206
#define CARD_CHILTTAENG 207
#define CARD_PALTTAENG 208
#define CARD_GUTTAENG 209
#define CARD_JANGTTAENG 210

// 일삼, 일팔광땡
#define CARD_ILSAM_GWANGTTAENG 301
#define CARD_ILPAL_GWANGTTAENG 302

// 무적의 삼팔광땡
#define CARD_SAMPAL_GWANGTTAENG 99999

// CCard :: 카드패에 대한 정보가 담긴 클래스이다.
class CCard
{
public:
	CCard(int num, bool special);
	~CCard() {};

	int GetNumber();
	bool GetSpecial();

	static int GetJokboCode(CCard* card_1, CCard* card_2);

private:
	int		m_nCardNumber;	// 짝패가 1~10중 무엇인지 표현
	bool	m_bSpecial;		// 짝패가 광인지 그냥 피인지 표현
};

