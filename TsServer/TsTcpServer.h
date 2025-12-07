// TsTcpServer.h: TCP版 TSサーバクラス
//
/////////////////////////////////////////////////////////////////////////////

#pragma once


#include <vector>
#include <queue>


/////////////////////////////////////////////////////////////////////////////
// TCP版 TSサーバクラス
/////////////////////////////////////////////////////////////////////////////

class CTsTcpServer	:	public CBonObject
{
public:
// CBonObject
	DECLARE_IBONOBJECT(CTsTcpServer)

// ITsTcpServer
	virtual const bool OpenServer(const WORD wServerPort /* ITsSrcManager */);
	virtual void CloseServer(void);

// CTsTcpServer
	CTsTcpServer(IBonObject *pOwner);
	virtual ~CTsTcpServer(void);
	
protected:
	virtual void AcceptThread(CSmartThread<CTsTcpServer> *pThread, bool &bKillSignal, PVOID pParam);

	CSmartThread<CTsTcpServer> m_AcceptThread;

	ISmartSocket *m_pServerSocket;
};
