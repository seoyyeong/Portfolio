
#include "pch.h"
#include "CPacket.h"


CTlsPool<CPacket> CPacket::PacketPool(3000);
char CPacket::chNetHeaderCode = 0;
char CPacket::chNetHeaderKey = 0;


//////////////////////////////////////////////////////////////////////////
// 생성자, 파괴자.
//
// Return:
//////////////////////////////////////////////////////////////////////////
CPacket::CPacket()
{
	m_chpBuffer = (char*)malloc(eBUFFER_DEFAULT);
	m_pPayLoad = m_chpBuffer + dfHEADER_MAX_SIZE;

	m_iReadPos = m_pPayLoad;
	m_iWritePos = m_pPayLoad;

	m_iBufferSize = eBUFFER_DEFAULT;
	m_iDataSize = 0;

	bIsEncode = FALSE;
	bIsDecode = FALSE;

	chNetHeaderCode = 0x77;
	chNetHeaderKey = 0x32;

	iRefCount = 0;
}
CPacket::CPacket(char chCode, char chKey)
{
	m_chpBuffer = (char*)malloc(eBUFFER_DEFAULT);
	m_pPayLoad = m_chpBuffer + dfHEADER_MAX_SIZE;

	m_iReadPos = m_pPayLoad;
	m_iWritePos = m_pPayLoad;

	m_iBufferSize = eBUFFER_DEFAULT;
	m_iDataSize = 0;

	bIsEncode = FALSE;
	bIsDecode = FALSE;

	chNetHeaderCode = chCode;
	chNetHeaderKey = chKey;

	iRefCount = 0;
}

CPacket::CPacket(int iBufferSize)
{
	m_chpBuffer = (char*)malloc(iBufferSize);
	m_pPayLoad = m_chpBuffer + dfHEADER_MAX_SIZE;

	m_iReadPos = m_pPayLoad;
	m_iWritePos = m_pPayLoad;

	m_iBufferSize = iBufferSize;
	m_iDataSize = 0;

	bIsEncode = FALSE;
	bIsDecode = FALSE;

	chNetHeaderCode = 0x77;
	chNetHeaderKey = 0x32;

	iRefCount = 0;

}

CPacket::CPacket(int iBufferSize, char chCode, char chKey)
{
	m_chpBuffer = (char*)malloc(iBufferSize);
	m_pPayLoad = m_chpBuffer + dfHEADER_MAX_SIZE;

	m_iReadPos = m_pPayLoad;
	m_iWritePos = m_pPayLoad;

	m_iBufferSize = iBufferSize;
	m_iDataSize = 0;

	bIsEncode = FALSE;
	bIsDecode = FALSE;

	chNetHeaderCode = chCode;
	chNetHeaderKey = chKey;

	iRefCount = 0;

}
CPacket::~CPacket()
{
	free(m_chpBuffer);
}


//////////////////////////////////////////////////////////////////////////
// 버퍼 사이즈 조정(재할당)
//
// Parameters: 없음.
// Return: (int) Resize된 버퍼 총 크기.
//////////////////////////////////////////////////////////////////////////
int CPacket::Resize(void)
{
	if (m_iBufferSize * 2 > dfPACKET_MAX_RESIZE)
		return 0;

	char* temp = m_chpBuffer;
	m_chpBuffer = (char*)malloc(sizeof(m_iBufferSize * 2));
	m_pPayLoad = m_chpBuffer + dfHEADER_MAX_SIZE;

	memcpy(m_pPayLoad, m_iReadPos, m_iDataSize);
	m_iReadPos = m_pPayLoad;
	m_iWritePos = m_pPayLoad + m_iDataSize;
	m_iBufferSize *= 2;

	free(temp);

	return m_iBufferSize;
}

bool CPacket::Encode(short shType)
{
	stLanHeader* pLanHeader;
	stNetHeader* pNetHeader;

	if (InterlockedExchange8((char*)&bIsEncode, TRUE) == TRUE)
	{
		return TRUE;
	}

	switch (shType)
	{
	case dfHEADER_LAN:
		m_pHeader = m_pPayLoad - dfHEADER_LAN;
		pLanHeader = (stLanHeader*)m_pHeader;
		pLanHeader->shPayload = m_iDataSize;
		break;

	case dfHEADER_NET:
		//Header Set
		m_pHeader = m_pPayLoad - dfHEADER_NET;
		pNetHeader = (stNetHeader*)m_pHeader;

		pNetHeader->chCode = chNetHeaderCode;
		pNetHeader->shLen = m_iDataSize;
		pNetHeader->chCheckSum = 0;
		pNetHeader->chRandKey = rand();


		char K = chNetHeaderKey;
		char RK = pNetHeader->chRandKey;

		char* chCheckSum = &pNetHeader->chCheckSum;
		char* chReadPos = m_pPayLoad;
		for (int i = 0; i < m_iDataSize; i++)
		{
			*chCheckSum += *chReadPos;
			chReadPos++;
		}
		*chCheckSum %= 256;

		//Encoding
		char* d = &pNetHeader->chCheckSum;
		char p = *d ^ (RK + 1);
		char e = p ^ (K + 1);
		*d = e;
		d++;

		for (int i = 2; i <= m_iDataSize + 1; i++)
		{
			p = *d ^ (p + RK + i);
			e = p ^ (e + K + i);
			*d = e;
			d++;
		}

		break;

	defalut:
		return FALSE;
	}

	return TRUE;
}


bool CPacket::Decode(short shType)
{
	stNetHeader* pNetHeader;

	char K;
	char RK;

	if (InterlockedExchange8((char*)&bIsDecode, TRUE) == TRUE)
	{
		return FALSE;
	}

	switch (shType)
	{
	case dfHEADER_LAN:
		m_pHeader = m_chpBuffer + dfHEADER_MAX_SIZE;
		m_pPayLoad = m_pHeader + dfHEADER_LAN;
		MoveReadPos(dfHEADER_LAN);
		break;

	case dfHEADER_NET:
		m_pHeader = m_chpBuffer + dfHEADER_MAX_SIZE;
		m_pPayLoad = m_pHeader + dfHEADER_NET;
		MoveReadPos(dfHEADER_NET);
		pNetHeader = (stNetHeader*)m_pHeader;

		//K = pNetHeader->chCode;
		K = chNetHeaderKey;
		RK = pNetHeader->chRandKey;

		char* pE = &pNetHeader->chCheckSum;
		char  oldE = *pE;

		char  oldP = oldE ^ (K + 1);
		char  oldD = oldP ^ (RK + 1);

		char  D;
		char  P;
		*pE = oldD;

		pE++;

		for (int i = 2; i <= m_iDataSize + 1; i++)
		{
			P = *pE ^ (oldE + K + i);
			D = P ^ (oldP + RK + i);

			oldE = *pE;
			*pE = D;

			oldP = P;
			pE++;
		}

		//체크썸 확인
		char* chCheckSum = &pNetHeader->chCheckSum;
		char* chReadPos = m_iReadPos;
		for (int i = 0; i <= m_iDataSize + 1; i++)
		{
			*chCheckSum += *chReadPos;
			chReadPos++;
		}
		*chCheckSum %= 256;

		if (pNetHeader->chCheckSum != *chCheckSum)
		{
			return 0;
		}

		break;
	defalut:
		return FALSE;
	}
	return TRUE;
}
