#pragma once
#ifndef  __PACKET__
#define  __PACKET__


#include "pch.h"
#include "Header.h"

#define dfPACKET_MAX_SIZE 256
#define dfPACKET_MAX_RESIZE dfPACKET_MAX_SIZE*3



class CPacket
{
public:
	/*---------------------------------------------------------------
	Packet Enum.

	----------------------------------------------------------------*/
	enum en_PACKET
	{
		eBUFFER_DEFAULT = dfPACKET_MAX_SIZE		// 패킷의 기본 버퍼 사이즈.
	};

	CPacket();
	CPacket(char chCode, char chKey);
	CPacket(int iBufferSize);
	CPacket(int iBufferSize, char chCode, char chKey);
	CPacket(CPacket& ref) = delete;
	virtual ~CPacket();

	inline void Clear(void)
	{
		m_pPayLoad = m_chpBuffer + dfHEADER_MAX_SIZE;
		m_iReadPos = m_pPayLoad;
		m_iWritePos = m_pPayLoad;
		m_iDataSize = 0;
	}



	int Resize(void);

	inline int		GetBufferSize(void) const { return m_iBufferSize; }

	inline int		GetDataSize(void) const { return m_iDataSize; }

	inline int   	GetSendSize(void) const { return (int)(m_iWritePos - m_pHeader); }


	inline char*	GetWritePtr(void) const { return m_iWritePos; }

	inline char*	GetReadPtr(void) const { return m_iReadPos; }

	int		MoveWritePos(int iSize)
	{

		if (m_chpBuffer + m_iBufferSize < m_iWritePos + iSize)
			return 0;

		m_iWritePos += iSize;
		m_iDataSize += iSize;

		return iSize;
	}

	int		MoveReadPos(int iSize)
	{

		if (m_chpBuffer + m_iBufferSize < m_iReadPos + iSize)
			return 0;

		m_iReadPos += iSize;
		m_iDataSize -= iSize;

		return iSize;
	}


	char*    GetPtr(void)
	{
		return m_pHeader;
	}


	bool Encode(short shType);

	bool Decode(short shType);

	static void SetHeaderCode(char chCode)
	{
		chNetHeaderCode = chCode;
	}

	static void SetHeaderKey(char chKey)
	{
		chNetHeaderKey = chKey;
	}

	CPacket& operator = (CPacket& clSrcPacket) = delete;

	CPacket& operator << (bool bValue) = delete;

	CPacket& operator << (unsigned char uchValue)
	{
		if (sizeof(uchValue) > m_iBufferSize - m_iDataSize)
			CRASH();

		*(unsigned char*)m_iWritePos = uchValue;

		m_iWritePos += sizeof(uchValue);
		m_iDataSize += sizeof(uchValue);

		return *this;
	}
	CPacket& operator << (char chValue)
	{
		if (sizeof(chValue) > m_iBufferSize - m_iDataSize)
			CRASH();

		*(char*)m_iWritePos = chValue;

		m_iWritePos += sizeof(chValue);
		m_iDataSize += sizeof(chValue);

		return *this;
	}

	CPacket& operator << (short shValue)
	{
		if (sizeof(shValue) > m_iBufferSize - m_iDataSize)
			CRASH();

		*(short*)m_iWritePos = shValue;

		m_iWritePos += sizeof(shValue);
		m_iDataSize += sizeof(shValue);

		return *this;
	}
	CPacket& operator << (unsigned short ushValue)
	{
		if (sizeof(ushValue) > m_iBufferSize - m_iDataSize)
			CRASH();

		*(unsigned short*)m_iWritePos = ushValue;

		m_iWritePos += sizeof(ushValue);
		m_iDataSize += sizeof(ushValue);

		return *this;
	}

	CPacket& operator << (int iValue)
	{
		if (sizeof(iValue) > m_iBufferSize - m_iDataSize)
			CRASH();

		*(int*)m_iWritePos = iValue;

		m_iWritePos += sizeof(iValue);
		m_iDataSize += sizeof(iValue);

		return *this;
	}

	CPacket& operator << (long lValue)
	{
		if (sizeof(lValue) > m_iBufferSize - m_iDataSize)
			CRASH();

		*(long*)m_iWritePos = lValue;

		m_iWritePos += sizeof(lValue);
		m_iDataSize += sizeof(lValue);

		return *this;
	}

	CPacket& operator << (unsigned long dwValue)
	{
		if (sizeof(dwValue) > m_iBufferSize - m_iDataSize)
			CRASH();

		*(unsigned long*)m_iWritePos = dwValue;

		m_iWritePos += sizeof(dwValue);
		m_iDataSize += sizeof(dwValue);

		return *this;
	}

	CPacket& operator << (float fValue)
	{
		if (sizeof(fValue) > m_iBufferSize - m_iDataSize)
			CRASH();

		*(float*)m_iWritePos = fValue;

		m_iWritePos += sizeof(fValue);
		m_iDataSize += sizeof(fValue);

		return *this;
	}

	CPacket& operator << (__int64 iValue)
	{
		if (sizeof(iValue) > m_iBufferSize - m_iDataSize)
			CRASH();

		*(__int64*)m_iWritePos = iValue;

		m_iWritePos += sizeof(iValue);
		m_iDataSize += sizeof(iValue);

		return *this;
	}
	CPacket& operator << (double dValue)
	{
		if (sizeof(dValue) > m_iBufferSize - m_iDataSize)
			CRASH();

		*(double*)m_iWritePos = dValue;

		m_iWritePos += sizeof(dValue);
		m_iDataSize += sizeof(dValue);

		return *this;
	}

	CPacket& operator >> (bool& bValue) = delete;
	CPacket& operator >> (unsigned char& uchValue)
	{
		if (sizeof(uchValue) > m_iDataSize)
			CRASH();

		uchValue = *(unsigned char*)m_iReadPos;

		m_iReadPos += sizeof(uchValue);
		m_iDataSize -= sizeof(uchValue);

		return *this;
	}
	CPacket& operator >> (char& chValue)
	{
		if (sizeof(chValue) > m_iDataSize)
			CRASH();

		chValue = *(unsigned char*)m_iReadPos;

		m_iReadPos += sizeof(chValue);
		m_iDataSize -= sizeof(chValue);

		return *this;
	}

	CPacket& operator >> (short& shValue)
	{
		if (sizeof(shValue) > m_iDataSize)
			CRASH();

		shValue = *(short*)m_iReadPos;

		m_iReadPos += sizeof(shValue);
		m_iDataSize -= sizeof(shValue);

		return *this;
	}

	CPacket& operator >> (unsigned short& wValue)
	{
		if (sizeof(wValue) > m_iDataSize)
			CRASH();

		wValue = *(unsigned short*)m_iReadPos;

		m_iReadPos += sizeof(wValue);
		m_iDataSize -= sizeof(wValue);

		return *this;
	}

	CPacket& operator >> (int& iValue)
	{
		if (sizeof(iValue) > m_iDataSize)
			CRASH();

		iValue = *(int*)m_iReadPos;

		m_iReadPos += sizeof(iValue);
		m_iDataSize -= sizeof(iValue);

		return *this;
	}
	CPacket& operator >> (long& lValue)
	{
		if (sizeof(lValue) > m_iDataSize)
			CRASH();

		lValue = *(long*)m_iReadPos;

		m_iReadPos += sizeof(lValue);
		m_iDataSize -= sizeof(lValue);

		return *this;
	}

	CPacket& operator >> (unsigned long& ulValue)
	{
		if (sizeof(ulValue) > m_iDataSize)
			CRASH();

		ulValue = *(unsigned long*)m_iReadPos;

		m_iReadPos += sizeof(ulValue);
		m_iDataSize -= sizeof(ulValue);

		return *this;
	}
	CPacket& operator >> (float& fValue)
	{
		if (sizeof(fValue) > m_iDataSize)
			CRASH();

		fValue = *(float*)m_iReadPos;

		m_iReadPos += sizeof(fValue);
		m_iDataSize -= sizeof(fValue);

		return *this;
	}

	CPacket& operator >> (__int64& iValue)
	{
		if (sizeof(iValue) > m_iDataSize)
			CRASH();

		iValue = *(__int64*)m_iReadPos;

		m_iReadPos += sizeof(iValue);
		m_iDataSize -= sizeof(iValue);

		return *this;
	}
	CPacket& operator >> (double& dValue)
	{
		if (sizeof(dValue) > m_iDataSize)
			CRASH();

		dValue = *(double*)m_iReadPos;

		m_iReadPos += sizeof(dValue);
		m_iDataSize -= sizeof(dValue);

		return *this;
	}


	int GetData(char* chpDest, int iSize)
	{
		if (iSize > m_iDataSize)
			return 0;

		memcpy(chpDest, m_iReadPos, iSize);

		m_iReadPos += iSize;
		m_iDataSize -= iSize;

		return iSize;
	}

	int PutData(char* chpSrc, int iSrcSize)
	{
		if (iSrcSize > m_iBufferSize - m_iDataSize)
			return 0;

		memcpy(m_iWritePos, chpSrc, iSrcSize);

		m_iWritePos += iSrcSize;
		m_iDataSize += iSrcSize;

		return iSrcSize;
	}


	//참조 카운트, 참조가 0이 되어야 할당해제
	void AddRef(void)
	{
		InterlockedIncrement((long*)&iRefCount);
	}

	void SubRef(void)
	{
		int ret = InterlockedDecrement((long*)&iRefCount);
		if (ret == 0)
		{
			Free(this);
			return;
		}
		if (ret < 0)
		{
			CRASH();
		}
	}

	static CPacket* Alloc(void)
	{
		CPacket* pPacket = PacketPool.Alloc();
		if (InterlockedExchange((long*)&pPacket->iRefCount, 1) != 0)
		{
			CRASH();
		}

		return pPacket;
	}

	static bool Free(CPacket* pPacket)
	{

		pPacket->bIsDecode = false;
		pPacket->bIsEncode = false;
		pPacket->Clear();
		return PacketPool.Free(pPacket);
		
	}

	static int GetPacketPoolSize(void)
	{
		return PacketPool.GetSize();
	}
	static int GetPacketPoolTotalSize(void)
	{
		return PacketPool.GetTotalSize();
	}
	static int GetPacketPoolAllocSize(void)
	{
		return PacketPool.GetAllocSize();
	}
	static char GetNetHeaderCode(void)
	{
		return chNetHeaderCode;
	}
	static char GetNetHeaderKey(void)
	{
		return chNetHeaderKey;
	}


	static CTlsPool<CPacket> PacketPool;

protected:

	char*	m_chpBuffer;
	char*   m_pHeader;
	char*   m_pPayLoad;
	int		m_iBufferSize;
	char*	m_iReadPos;
	char*	m_iWritePos;

	int		m_iDataSize;
	int     iRefCount;

	// 인코딩여부 확인
	bool    bIsEncode;
	bool    bIsDecode;

	static char chNetHeaderCode;
	static char chNetHeaderKey;


};

#endif