#pragma once


//헤더 형식 define
#define dfHEADER_MAX_SIZE dfHEADER_NET
#define dfHEADER_LAN sizeof(stLanHeader)
#define dfHEADER_NET sizeof(stNetHeader)

//암호화 define

#pragma pack(1)
struct stNetHeader
{
	char  chCode;
	short shLen;
	char  chRandKey;
	char  chCheckSum;
};

struct stLanHeader
{
	short shPayload;
};
#pragma pack()