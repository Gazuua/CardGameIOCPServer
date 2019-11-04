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

// ¼¼·ú, Àå»ç, Àå»æ, ±¸»æ, ¾Ë¸®, ¶¯, ±¤¶¯ Á¦¿ÜÇÏ°í
// ³ª¸ÓÁö´Â ²ıÀÚ¸® ¼ö·Î ÆÇ´Ü.
int CCard::GetJokboCode(CCard* card_1, CCard* card_2)
{
	// ¼¼·ú
	if ((card_1->GetNumber() == 4 && card_2->GetNumber() == 6) ||
		(card_1->GetNumber() == 6 && card_2->GetNumber() == 4))
		return CARD_SERYUK;

	// Àå»ç
	if ((card_1->GetNumber() == 10 && card_2->GetNumber() == 4) ||
		(card_1->GetNumber() == 4 && card_2->GetNumber() == 10))
		return CARD_JANGSA;

	// Àå»æ
	if ((card_1->GetNumber() == 10 && card_2->GetNumber() == 1) ||
		(card_1->GetNumber() == 1 && card_2->GetNumber() == 10))
		return CARD_JANGBBING;

	// ±¸»æ
	if ((card_1->GetNumber() == 9 && card_2->GetNumber() == 1) ||
		(card_1->GetNumber() == 1 && card_2->GetNumber() == 9))
		return CARD_GUBBING;

	// ¾Ë¸®
	if ((card_1->GetNumber() == 2 && card_2->GetNumber() == 1) ||
		(card_1->GetNumber() == 1 && card_2->GetNumber() == 2))
		return CARD_ALLI;

	// ¶¯(µÎ Ä«µå ¹øÈ£°¡ °°Àº °æ¿ì)
	if (card_1->GetNumber() == card_2->GetNumber())
	{
		switch (card_1->GetNumber())
		{
			// »æ¶¯
		case 1:
			return CARD_BBINGTTAENG;
			
			// ÀÌ¶¯
		case 2:
			return CARD_YITTAENG;

			// »ï¶¯
		case 3:
			return CARD_SAMTTAENG;

			// »ç¶¯
		case 4:
			return CARD_SATTAENG;

			// ¿À¶¯
		case 5:
			return CARD_OTTAENG;

			// À°¶¯
		case 6:
			return CARD_YUKTTAENG;

			// Ä¥¶¯
		case 7:
			return CARD_CHILTTAENG;
			// ÆÈ¶¯
		case 8:
			return CARD_PALTTAENG;

			// ±¸¶¯
		case 9:
			return CARD_GUTTAENG;

			// Àå¶¯
		case 10:
			return CARD_JANGTTAENG;
		}
	}

	// ÀÏ»ï±¤¶¯
	if ((card_1->GetNumber() == 1 && card_1->GetSpecial()) &&
		(card_2->GetNumber() == 3 && card_2->GetSpecial()))
		return CARD_ILSAM_GWANGTTAENG;
	if ((card_1->GetNumber() == 3 && card_1->GetSpecial()) &&
		(card_2->GetNumber() == 1 && card_2->GetSpecial()))
		return CARD_ILSAM_GWANGTTAENG;

	// ÀÏÆÈ±¤¶¯
	if ((card_1->GetNumber() == 1 && card_1->GetSpecial()) &&
		(card_2->GetNumber() == 8 && card_2->GetSpecial()))
		return CARD_ILPAL_GWANGTTAENG;
	if ((card_1->GetNumber() == 8 && card_1->GetSpecial()) &&
		(card_2->GetNumber() == 1 && card_2->GetSpecial()))
		return CARD_ILPAL_GWANGTTAENG;

	// »ïÆÈ±¤¶¯
	if ((card_1->GetNumber() == 3 && card_1->GetSpecial()) &&
		(card_2->GetNumber() == 8 && card_2->GetSpecial()))
		return CARD_SAMPAL_GWANGTTAENG;
	if ((card_1->GetNumber() == 8 && card_1->GetSpecial()) &&
		(card_2->GetNumber() == 3 && card_2->GetSpecial()))
		return CARD_SAMPAL_GWANGTTAENG;

	// ±× ¿Ü¿¡´Â ²ıÀÚ¸®·Î ±¸ºĞÇØÁØ´Ù.
	int sum = card_1->GetNumber() + card_2->GetNumber();
	if (sum >= 10) sum -= 10;

	return sum;
}
