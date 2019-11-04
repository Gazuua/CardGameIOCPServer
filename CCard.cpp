#include "CCard.h"

CCard::CCard(int num, bool special)
{
	this->m_nCardNumber = num;
	this->m_bSpecial = special;
}

int CCard::GetNumber()
{
	return this->m_nCardNumber;
}

bool CCard::GetSpecial()
{
	return this->m_bSpecial;
}

// ����, ���, ���, ����, �˸�, ��, ���� �����ϰ�
// �������� ���ڸ� ���� �Ǵ�.
int CCard::GetJokboCode(CCard* card_1, CCard* card_2)
{
	// ����
	if ((card_1->GetNumber() == 4 && card_2->GetNumber() == 6) ||
		(card_1->GetNumber() == 6 && card_2->GetNumber() == 4))
		return CARD_SERYUK;

	// ���
	if ((card_1->GetNumber() == 10 && card_2->GetNumber() == 4) ||
		(card_1->GetNumber() == 4 && card_2->GetNumber() == 10))
		return CARD_JANGSA;

	// ���
	if ((card_1->GetNumber() == 10 && card_2->GetNumber() == 1) ||
		(card_1->GetNumber() == 1 && card_2->GetNumber() == 10))
		return CARD_JANGBBING;

	// ����
	if ((card_1->GetNumber() == 9 && card_2->GetNumber() == 1) ||
		(card_1->GetNumber() == 1 && card_2->GetNumber() == 9))
		return CARD_GUBBING;

	// �˸�
	if ((card_1->GetNumber() == 2 && card_2->GetNumber() == 1) ||
		(card_1->GetNumber() == 1 && card_2->GetNumber() == 2))
		return CARD_ALLI;

	// ��(�� ī�� ��ȣ�� ���� ���)
	if (card_1->GetNumber() == card_2->GetNumber())
	{
		switch (card_1->GetNumber())
		{
			// �涯
		case 1:
			return CARD_BBINGTTAENG;
			
			// �̶�
		case 2:
			return CARD_YITTAENG;

			// �ﶯ
		case 3:
			return CARD_SAMTTAENG;

			// �綯
		case 4:
			return CARD_SATTAENG;

			// ����
		case 5:
			return CARD_OTTAENG;

			// ����
		case 6:
			return CARD_YUKTTAENG;

			// ĥ��
		case 7:
			return CARD_CHILTTAENG;
			// �ȶ�
		case 8:
			return CARD_PALTTAENG;

			// ����
		case 9:
			return CARD_GUTTAENG;

			// �嶯
		case 10:
			return CARD_JANGTTAENG;
		}
	}

	// �ϻﱤ��
	if ((card_1->GetNumber() == 1 && card_1->GetSpecial()) &&
		(card_2->GetNumber() == 3 && card_2->GetSpecial()))
		return CARD_ILSAM_GWANGTTAENG;
	if ((card_1->GetNumber() == 3 && card_1->GetSpecial()) &&
		(card_2->GetNumber() == 1 && card_2->GetSpecial()))
		return CARD_ILSAM_GWANGTTAENG;

	// ���ȱ���
	if ((card_1->GetNumber() == 1 && card_1->GetSpecial()) &&
		(card_2->GetNumber() == 8 && card_2->GetSpecial()))
		return CARD_ILPAL_GWANGTTAENG;
	if ((card_1->GetNumber() == 8 && card_1->GetSpecial()) &&
		(card_2->GetNumber() == 1 && card_2->GetSpecial()))
		return CARD_ILPAL_GWANGTTAENG;

	// ���ȱ���
	if ((card_1->GetNumber() == 3 && card_1->GetSpecial()) &&
		(card_2->GetNumber() == 8 && card_2->GetSpecial()))
		return CARD_SAMPAL_GWANGTTAENG;
	if ((card_1->GetNumber() == 8 && card_1->GetSpecial()) &&
		(card_2->GetNumber() == 3 && card_2->GetSpecial()))
		return CARD_SAMPAL_GWANGTTAENG;

	// �� �ܿ��� ���ڸ��� �������ش�.
	int sum = card_1->GetNumber() + card_2->GetNumber();
	if (sum >= 10) sum -= 10;

	return sum;
}
