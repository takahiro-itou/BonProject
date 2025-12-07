// BonBlackTuner.h: 黒Friioチューナークラス
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Vector>
#include <Queue>
#include "BonEnumerator.h"


/////////////////////////////////////////////////////////////////////////////
// 黒Friio用　TSチューナ HALインタフェース実装クラス
/////////////////////////////////////////////////////////////////////////////

#define TSRECVSIZE				48128UL			// TSデータ受信サイズ

class CBonBlackTuner :	public CBonObject,
						public IHalDevice,
						public IHalTsTuner
{
public:
// CBonObject
	DECLARE_IBONOBJECT(CBonBlackTuner)

// IHalDevice
	virtual const BONGUID GetDeviceType(void);
	virtual const DWORD GetDeviceName(LPTSTR lpszName);
	virtual const DWORD GetTotalDeviceNum(void);
	virtual const DWORD GetActiveDeviceNum(void);

// IHalTsTuner
	virtual const bool OpenTuner(void);
	virtual void CloseTuner(void);
	
	virtual const DWORD EnumTuningSpace(const DWORD dwSpace, LPTSTR lpszSpace);
	virtual const DWORD EnumChannelName(const DWORD dwSpace, const DWORD dwChannel, LPTSTR lpszChannel);
	
	virtual const DWORD GetCurSpace(void);
	virtual const DWORD GetCurChannel(void);

	virtual const bool SetChannel(const DWORD dwSpace, const DWORD dwChannel);
	virtual const bool SetLnbPower(const bool bEnable);
	virtual const float GetSignalLevel(void);

	virtual const bool GetStream(BYTE **ppStream, DWORD *pdwSize, DWORD *pdwRemain);

	virtual void PurgeStream(void);
	virtual const DWORD WaitStream(const DWORD dwTimeOut = 0UL);
	virtual const DWORD GetAvailableNum(void);

// CBonBlackTuner
	CBonBlackTuner(IBonObject *pOwner);
	virtual ~CBonBlackTuner(void);
	
	static void SetModuleHandle(const HINSTANCE hModule);
	
protected:
	const int SendIoRequest(const DWORD dwIoCode, const BYTE *pTxdData, const DWORD dwTxdSize, BYTE *pRxdData, const DWORD dwRxdSize);
	const bool SendFixedRequest(const UINT nID);

	const bool AllocateBuffer(const DWORD dwBufferNum);
	void ReleaseBuffer(void);

	void IoReqThread(CSmartThread<CBonBlackTuner> *pThread, bool &bKillSignal, PVOID pParam);
	void IoRecvThread(CSmartThread<CBonBlackTuner> *pThread, bool &bKillSignal, PVOID pParam);

	const bool PushIoRequest(void);
	const bool PopIoRequest(void);

	// 固定I/Oリクエストヘッダ
	#pragma pack(1)
	struct FIXED_REQ_HEAD
	{
		DWORD dwIoCode;		// I/Oコントロールコード
		WORD wTxdSize;		// 送信データサイズ
		WORD wRxdSize;		// 受信データサイズ
	};
	#pragma pack()

	// I/Oリクエストキューデータ
	struct ASYNC_REQ_DATA
	{
		OVERLAPPED OverLapped;
		DWORD dwRxdSize;
		BYTE TxdBuff[64];
		BYTE RxdBuff[TSRECVSIZE];
	};

	CBonEnumerator m_BonEnumerator;

	HANDLE m_hFriioDriver;
	BYTE m_TxdBuff[256];
	BYTE m_RxdBuff[256];

	std::vector<ASYNC_REQ_DATA> m_AsyncReqPool;
	std::vector<ASYNC_REQ_DATA>::iterator m_itAsyncReq;
	std::queue<ASYNC_REQ_DATA *> m_AsyncReqQue;
	std::queue<ASYNC_REQ_DATA *> m_AsyncRecvQue;
	
	CSmartThread<CBonBlackTuner> m_IoReqThread;
	CSmartThread<CBonBlackTuner> m_IoRecvThread;
	
	CSmartLock m_AsyncReqLock;
	CSmartEvent m_TsRecvEvent;
	
	DWORD m_dwCurSpace;
	DWORD m_dwCurChannel;
	
	static HINSTANCE m_hModule;
};
