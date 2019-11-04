#pragma once
#include<iostream>

using namespace std;

// ����~����
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

// ����~�˸�
#define CARD_SERYUK 101
#define CARD_JANGSA 102
#define CARD_JANGBBING 103
#define CARD_GUBBING 104
#define CARD_ALLI 105

// �涯~�嶯
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

// �ϻ�, ���ȱ���
#define CARD_ILSAM_GWANGTTAENG 301
#define CARD_ILPAL_GWANGTTAENG 302

// ������ ���ȱ���
#define CARD_SAMPAL_GWANGTTAENG 99999

// CCard :: ī���п� ���� ������ ��� Ŭ�����̴�.
class CCard
{
public:
	CCard(int num, bool special);
	~CCard() {};

	int GetNumber();
	bool GetSpecial();

	static int GetJokboCode(CCard* card_1, CCard* card_2);

private:
	int		m_nCardNumber;	// ¦�а� 1~10�� �������� ǥ��
	bool	m_bSpecial;		// ¦�а� ������ �׳� ������ ǥ��
};

