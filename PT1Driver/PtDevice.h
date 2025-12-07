// PtDevice.h: PTデバイスクラス
//
/////////////////////////////////////////////////////////////////////////////

#pragma once


#include "EARTH_PT.h"
#include "OS_Library.h"
#include <Vector>
#include <Queue>


using namespace EARTH;
using namespace OS;
using namespace PT;
using namespace std;


/////////////////////////////////////////////////////////////////////////////
// PTマネージャクラス
/////////////////////////////////////////////////////////////////////////////

class CPtBoard;
class CPtTuner;

class CPtManager
{
public:
	CPtManager(void);
	virtual ~CPtManager(void);

	const DWORD GetTotalTunerNum(const Device::ISDB dwTunerType);
	const DWORD GetActiveTunerNum(const Device::ISDB dwTunerType);

	CPtTuner * OpenTuner(const Device::ISDB dwTunerType);

protected:
	const bool EnumerateBoards(void);
	const bool ReleaseInstance(void);
	
	Bus *m_pPtBus;
	vector<CPtBoard *> m_Boards;

	Library *m_pPtLibrary;
};


/////////////////////////////////////////////////////////////////////////////
// PTボードクラス
/////////////////////////////////////////////////////////////////////////////

class CPtBoard
{
friend CPtTuner;

public:
	enum{TUNER_1ST = 0UL, TUNER_2ND = 1UL, TUNER_NUM = 2UL};

	enum{
		DMABUF_SIZE = 6UL,									// DMAブロック数
		DMABUF_NUM  = 1UL,									// DMAバッファ数
		DMABLK_SIZE = 4096UL * Device::BUFFER_PAGE_COUNT,	// DMAブロックサイズ
		DMACHK_UNIT = 1UL,									// DMA転送完了チェック分割数
		DMACHK_WAIT = 5UL,									// DMA転送完了チェックウェイト
		DMACHK_TIMEOUT = 1000UL								// DMA転送完了タイムアウト
		};

	CPtBoard(const DWORD dwBoardID, Device *pDevice, const Bus::DeviceInfo *pDeviceInfo);
	virtual ~CPtBoard(void);

	CPtTuner * OpenTuner(const Device::ISDB dwTunerType);
	void CloseTuner(CPtTuner *pTuner);

	const DWORD GetActiveTunerNum(const Device::ISDB dwTunerType);

protected:
	const bool StartStream(CPtTuner *pTuner);
	void StopStream(CPtTuner *pTuner);

	const bool SetLnbPower(CPtTuner *pTuner, const bool bEnable);

	const bool IsValidTuner(const CPtTuner *pTuner) const;
	const DWORD GetOpenTunerNum(void) const;
	const DWORD GetStreamTunerNum(void) const;
	const DWORD GetLnbOnTunerNum(void) const;

	struct DISPATCH_BUFF
	{
		DWORD dwPacketPos;
		BYTE *pDataPtr;
		BYTE abyDataBuff[DMABLK_SIZE * Device::ISDB_COUNT * TUNER_NUM];
	};
	
	const DWORD m_dwBoardID;
	Device *m_pDevice;
	Bus::DeviceInfo m_DeviceInfo;
	
	vector<CPtTuner *> m_TunerList;
	DISPATCH_BUFF m_aDispatchBuff[TUNER_NUM * Device::ISDB_COUNT];

	CSmartMutex m_BoardLock;
	CSmartLock m_TunerLock;

private:
	const bool OpenBoard(void);
	void CloseBoard(void);

	void StreamingThread(CSmartThread<CPtBoard> *pThread, bool &bKillSignal, PVOID pParam);
	void DispatchStream(const DWORD *pdwData, const DWORD dwNum);

	CSmartThread<CPtBoard> m_StreamingThread;
};


/////////////////////////////////////////////////////////////////////////////
// PTチューナクラス
/////////////////////////////////////////////////////////////////////////////

class CPtTuner
{
friend CPtBoard;

public:
	class ITsReceiver :	public IBonObject
	{
	public:
		virtual void OnTsStream(CPtTuner *pTuner, const BYTE *pData, const DWORD dwSize) = 0;
	};

	CPtTuner(CPtBoard &Board, const DWORD dwTunerID, const Device::ISDB dwTunerType);
	virtual ~CPtTuner();

	void CloseTuner(void);

	const bool StartStream(ITsReceiver *pTsReceiver);
	void StopStream(void);

	const bool SetChannel(const DWORD dwChannel, const DWORD dwOffset = 0UL);
	const bool SetTsID(const WORD wTsID);
	const bool SetLnbPower(const bool bEnable);
	const float GetSignalLevel(void);

protected:
	void OnTsStream(const BYTE *pData, const DWORD dwSize);

	CPtBoard &m_Board;

	const DWORD m_dwTunerID;
	const Device::ISDB m_dwTunerType;

	bool m_bIsOpen;
	bool m_bIsStream;
	bool m_bIsLnbOn;
	
	ITsReceiver *m_pTsReceiver;
	CSmartLock m_Lock;
};
