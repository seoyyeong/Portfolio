//#ifndef __GODDAMNBUG_ONLINE_PROTOCOL__
//#define __GODDAMNBUG_ONLINE_PROTOCOL__

#pragma pack(1)

struct stMonitorSSLogin
{
	char chServerType;
	char chResult;
};

struct stMonitorSSData
{
	char chDataType;
	int  iDataValue;
	int  iTimeStamp;
};

struct stMonitorSCData
{
	char chServerNo;
	char chDataType;
	int  iDataValue;
	int  iTimeStamp;
};
#pragma pack()


enum en_MONITORING_SERVER_TYPE
{
	TYPE_CHATSERVER = 0,
	TYPE_LOGINSERVER = 1,
	TYPE_GAMESERVER = 2
};

enum en_PACKET_TYPE
{
	////////////////////////////////////////////////////////
	//
	//	Client & Server Protocol
	//
	////////////////////////////////////////////////////////

	//------------------------------------------------------
	// Chatting Server
	//------------------------------------------------------
	en_PACKET_CS_CHAT_SERVER = 0,

	//------------------------------------------------------------
	// ä�ü��� �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]				// null ����
	//		WCHAR	Nickname[20]		// null ����
	//		char	SessionKey[64];
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_LOGIN,

	//------------------------------------------------------------
	// ä�ü��� �α��� ����
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status				// 0:����	1:����
	//		INT64	AccountNo
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_RES_LOGIN,

	//------------------------------------------------------------
	// ä�ü��� ���� �̵� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_SECTOR_MOVE,

	//------------------------------------------------------------
	// ä�ü��� ���� �̵� ���
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_RES_SECTOR_MOVE,

	//------------------------------------------------------------
	// ä�ü��� ä�ú����� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null ������
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_MESSAGE,

	//------------------------------------------------------------
	// ä�ü��� ä�ú����� ����  (�ٸ� Ŭ�� ���� ä�õ� �̰ɷ� ����)
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]						// null ����
	//		WCHAR	Nickname[20]				// null ����
	//		
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null ������
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_RES_MESSAGE,

	//------------------------------------------------------------
	// ��Ʈ��Ʈ
	//
	//	{
	//		WORD		Type
	//	}
	//
	//
	// Ŭ���̾�Ʈ�� �̸� 30�ʸ��� ������.
	// ������ 40�� �̻󵿾� �޽��� ������ ���� Ŭ���̾�Ʈ�� ������ ������� ��.
	//------------------------------------------------------------	
	en_PACKET_CS_CHAT_REQ_HEARTBEAT,






	//------------------------------------------------------
	// Login Server
	//------------------------------------------------------
	en_PACKET_CS_LOGIN_SERVER = 100,

	//------------------------------------------------------------
	// �α��� ������ Ŭ���̾�Ʈ �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		char	SessionKey[64]
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_LOGIN_REQ_LOGIN,

	//------------------------------------------------------------
	// �α��� �������� Ŭ���̾�Ʈ�� �α��� ����
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		BYTE	Status				// 0 (���ǿ���) / 1 (����) ...  �ϴ� defines ���
	//
	//		WCHAR	ID[20]				// ����� ID		. null ����
	//		WCHAR	Nickname[20]		// ����� �г���	. null ����
	//
	//		WCHAR	GameServerIP[16]	// ���Ӵ�� ����,ä�� ���� ����
	//		USHORT	GameServerPort
	//		WCHAR	ChatServerIP[16]
	//		USHORT	ChatServerPort
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_LOGIN_RES_LOGIN,








	//------------------------------------------------------
	// Game Server
	//------------------------------------------------------
	en_PACKET_CS_GAME_SERVER = 1000,




	/*  ���� ��ū DB ������� �̻��
	*
	*
		////////////////////////////////////////////////////////
		//
		//   Server & Server Protocol  / LAN ����� �⺻���� ������ ���� ����.
		//
		////////////////////////////////////////////////////////
		en_PACKET_SS_LAN						= 10000,
		//------------------------------------------------------
		// GameServer & LoginServer & ChatServer Protocol
		//------------------------------------------------------

		//------------------------------------------------------------
		// �ٸ� ������ �α��� ������ �α���.
		// �̴� ������ ������, �׳� �α��� ��.
		//
		//	{
		//		WORD	Type
		//
		//		BYTE	ServerType			// dfSERVER_TYPE_GAME / dfSERVER_TYPE_CHAT
		//
		//		WCHAR	ServerName[32]		// �ش� ������ �̸�.
		//	}
		//
		//------------------------------------------------------------
		en_PACKET_SS_LOGINSERVER_LOGIN,



		//------------------------------------------------------------
		// �α��μ������� ����.ä�� ������ ���ο� Ŭ���̾�Ʈ ������ �˸�.
		//
		//	{
		//		WORD	Type
		//
		//		INT64	AccountNo
		//		CHAR	SessionKey[64]
		//	}
		//
		//------------------------------------------------------------
		// en_PACKET_SS_NEW_CLIENT_LOGIN,	// �ű� �������� ����Ű ������Ŷ�� ��û,���䱸���� ���� 2017.01.05


		//------------------------------------------------------------
		// �α��μ������� ����.ä�� ������ ���ο� Ŭ���̾�Ʈ ������ �˸�.
		//
		// �������� Parameter �� ����Ű ������ ���� ������ Ȯ���� ���� � ��. �̴� ���� ������� �ٽ� �ް� ��.
		// ä�ü����� ���Ӽ����� Parameter �� ���� ó���� �ʿ� ������ �״�� Res �� ������� �մϴ�.
		//
		//	{
		//		WORD	Type
		//
		//		INT64	AccountNo
		//		CHAR	SessionKey[64]
		//		INT64	Parameter
		//	}
		//
		//------------------------------------------------------------
		en_PACKET_SS_REQ_NEW_CLIENT_LOGIN,

		//------------------------------------------------------------
		// ����.ä�� ������ ���ο� Ŭ���̾�Ʈ ������Ŷ ���Ű���� ������.
		// ���Ӽ�����, ä�ü����� ��Ŷ�� ������ ������, �α��μ����� Ÿ ������ ���� �� CHAT,GAME ������ �����ϹǷ�
		// �̸� ����ؼ� �˾Ƽ� ���� �ϵ��� ��.
		//
		// �÷��̾��� ���� �α��� �Ϸ�� �� ��Ŷ�� Chat,Game ���ʿ��� �� �޾��� ������.
		//
		// ������ �� Parameter �� �̹� ����Ű ������ ���� ������ �� �ִ� Ư�� ��
		// ClientID �� ����, ���� ī������ ���� ��� ����.
		//
		// �α��μ����� ���Ӱ� �������� �ݺ��ϴ� ��� ������ ���������� ���� ������ ���� ��������
		// �����Ͽ� �ٸ� ����Ű�� ��� ���� ������ ����.
		//
		//	{
		//		WORD	Type
		//
		//		INT64	AccountNo
		//		INT64	Parameter
		//	}
		//
		//------------------------------------------------------------
		en_PACKET_SS_RES_NEW_CLIENT_LOGIN,

		*/


		en_PACKET_CS_CHAT_REQ_LOGIN_CHECK,

};



enum en_PACKET_CS_LOGIN_RES_LOGIN
{
	dfLOGIN_STATUS_NONE = -1,		// ����������
	dfLOGIN_STATUS_FAIL = 0,		// ���ǿ���
	dfLOGIN_STATUS_OK = 1,		// ����
	dfLOGIN_STATUS_GAME = 2,		// ������
	dfLOGIN_STATUS_ACCOUNT_MISS = 3,		// account ���̺� AccountNo ����
	dfLOGIN_STATUS_SESSION_MISS = 4,		// Session ���̺� AccountNo ����
	dfLOGIN_STATUS_STATUS_MISS = 5,		// Status ���̺� AccountNo ����
	dfLOGIN_STATUS_NOSERVER = 6,		// �������� ������ ����.
};


enum en_PACKET_CS_GAME_RES_LOGIN
{
	dfGAME_LOGIN_FAIL = 0,		// ����Ű ���� �Ǵ� Account ���̺���� ����
	dfGAME_LOGIN_OK = 1,		// ����
	dfGAME_LOGIN_NOCHARACTER = 2,		// ���� / ĳ���� ���� > ĳ���� ����ȭ������ ��ȯ. 
	dfGAME_LOGIN_VERSION_MISS = 3,		// ����,Ŭ�� ���� �ٸ�
};




//#endif