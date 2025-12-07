// BonBlackTuner.cpp: 黒Friioチューナークラス
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Resource.h"
#include "BonSDK.h"
#include "BonBlackTuner.h"
#include <TChar.h>


/////////////////////////////////////////////////////////////////////////////
// ファイルローカル定数設定
/////////////////////////////////////////////////////////////////////////////

// デバイス名
#define DEVICENAME	TEXT("黒 Friio USB 2.0 Digital TV Receiver")

// バッファ設定
#define ASYNCBUFFTIME		2UL											// バッファ長 = 2秒
#define ASYNCBUFFNUM		(0x400000UL / TSRECVSIZE * ASYNCBUFFTIME)	// 平均32Mbpsとする

#define REQRESERVNUM		16UL			// 非同期リクエスト予約数
#define REQPOLLINGWAIT		10UL			// 非同期リクエストポーリング間隔(ms)

// I/Oコントロールコード
#define IOCODE_CONTROL		0x00220020UL	// チューナコントロール
#define IOCODE_STREAM		0x0022004BUL	// ストリームリクエスト


/////////////////////////////////////////////////////////////////////////////
// 黒Friio用　TSチューナ HALインタフェース実装クラス
/////////////////////////////////////////////////////////////////////////////

HINSTANCE CBonBlackTuner::m_hModule = NULL;

const BONGUID CBonBlackTuner::GetDeviceType(void)
{
	// デバイスのタイプを返す
	return ::BON_NAME_TO_GUID(TEXT("IHalTsTuner"));
}

const DWORD CBonBlackTuner::GetDeviceName(LPTSTR lpszName)
{
	// デバイス名を返す
	if(lpszName)::_tcscpy(lpszName, DEVICENAME);

	return ::_tcslen(DEVICENAME);
}

const DWORD CBonBlackTuner::GetTotalDeviceNum(void)
{
	// 黒Friioの総数を返す
	return (m_BonEnumerator.EnumDevice(true))? m_BonEnumerator.GetTotalNum() : 0UL;
}

const DWORD CBonBlackTuner::GetActiveDeviceNum(void)
{
	// 使用中の黒Friioの数を返す
	return (m_BonEnumerator.EnumDevice(true))? m_BonEnumerator.GetActiveNum() : 0UL;
}

const bool CBonBlackTuner::OpenTuner(void)
{
	// 一旦クローズ
	CloseTuner();
	
	try{
		// ドライバ列挙
		if(!m_BonEnumerator.EnumDevice(true))throw __LINE__;
	
		// 未使用のチューナを取得
		LPCTSTR lpszDriverPath = m_BonEnumerator.GetAvailableDriverPath();
		if(!lpszDriverPath)throw __LINE__;

		// ドライバオープン
		if((m_hFriioDriver = ::CreateFile(lpszDriverPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE)throw __LINE__;

		// ドライバロック
		if(!m_BonEnumerator.LockDevice(lpszDriverPath))throw __LINE__;
		
		// 初期化シーケンス送信
		if(!SendFixedRequest(IDR_INISEQ_BLACK))throw __LINE__;
		
		// デフォルトチャンネル設定
		if(!SetChannel(0UL, 0UL))throw __LINE__;
		
		// イベントリセット
		if(!m_TsRecvEvent.ResetEvent())throw __LINE__;
		
		// バッファ確保
		if(!AllocateBuffer(ASYNCBUFFNUM + REQRESERVNUM))throw __LINE__;
		
		// スレッド起動
		if(!m_IoReqThread.StartThread(this, &CBonBlackTuner::IoReqThread))throw __LINE__;
		if(!m_IoRecvThread.StartThread(this, &CBonBlackTuner::IoRecvThread))throw __LINE__;
		}
	catch(...){
		// エラー発生
		CloseTuner();
		return false;
		}
	
	return true;
}

void CBonBlackTuner::CloseTuner(void)
{
	// スレッド停止
	m_IoReqThread.EndThread();
	m_IoRecvThread.EndThread();

	// イベントセット(待機スレッド開放)
	m_TsRecvEvent.SetEvent();

	// バッファ開放
	ReleaseBuffer();
	
	if(m_hFriioDriver != INVALID_HANDLE_VALUE){
		// 終了シーケンス送信
		SendFixedRequest(IDR_ENDSEQ_BLACK);

		// ドライバクローズ
		::CloseHandle(m_hFriioDriver);
		m_hFriioDriver = INVALID_HANDLE_VALUE;
		}

	// ドライバロック開放
	m_BonEnumerator.ReleaseDevice();
	
	// チャンネル初期化
	m_dwCurSpace = 0UL;
	m_dwCurChannel = 0UL;
}

const DWORD CBonBlackTuner::EnumTuningSpace(const DWORD dwSpace, LPTSTR lpszSpace)
{
	static const TCHAR aszTuningSpace[][16] = 
	{
		TEXT("BS"	),
		TEXT("110CS")
	};

	// 使用可能なチューニング空間を返す
	if(dwSpace >= (sizeof(aszTuningSpace) / sizeof(aszTuningSpace[0])))return 0UL;

	// チューニング空間名をコピー
	if(lpszSpace)::_tcscpy(lpszSpace, aszTuningSpace[dwSpace]);

	// チューニング空間名長を返す
	return ::_tcslen(aszTuningSpace[dwSpace]);
}

const DWORD CBonBlackTuner::EnumChannelName(const DWORD dwSpace, const DWORD dwChannel, LPTSTR lpszChannel)
{
	// 使用可能なチャンネルを返す
	static const TCHAR aszChannelNameBS[][32] = 
	{
		TEXT("BS1/TS0 BS朝日"			),
		TEXT("BS1/TS1 BS-i"				),
		TEXT("BS3/TS0 WOWOW"			),
		TEXT("BS3/TS1 BSジャパン"		),
		TEXT("BS9/TS0 BS11"				),
		TEXT("BS9/TS1 Star Channel HV"	),
		TEXT("BS9/TS2 TwellV"			),
		TEXT("BS13/TS0 BS日テレ"		),
		TEXT("BS13/TS1 BSフジ"			),
		TEXT("BS15/TS1 NHK BS1/2"		),
		TEXT("BS15/TS2 NHK BS-hi"		)
	};

	static const TCHAR aszChannelNameCS[][32] = 
	{
		TEXT("ND2 110CS #1"  ),
		TEXT("ND4 110CS #2"  ),
		TEXT("ND6 110CS #3"  ),
		TEXT("ND8 110CS #4"  ),
		TEXT("ND10 110CS #5" ),
		TEXT("ND12 110CS #6" ),
		TEXT("ND14 110CS #7" ),
		TEXT("ND16 110CS #8" ),
		TEXT("ND18 110CS #9" ),
		TEXT("ND20 110CS #10"),
		TEXT("ND22 110CS #11"),
		TEXT("ND24 110CS #12")
	};

	switch(dwSpace){
		case 0UL :
			// BS
			if(dwChannel >= (sizeof(aszChannelNameBS) / sizeof(aszChannelNameBS[0])))return 0UL;

			// チャンネル名をコピー
			if(lpszChannel)::_tcscpy(lpszChannel, aszChannelNameBS[dwChannel]);

			// チャンネル名長を返す
			return ::_tcslen(aszChannelNameBS[dwSpace]);
		
		case 1UL :
			// 110CS
			if(dwChannel >= (sizeof(aszChannelNameCS) / sizeof(aszChannelNameCS[0])))return 0UL;

			// チャンネル名をコピー
			if(lpszChannel)::_tcscpy(lpszChannel, aszChannelNameCS[dwChannel]);

			// チャンネル名長を返す
			return ::_tcslen(aszChannelNameCS[dwSpace]);
		
		default  :
			// 非対応のチューニング空間
			return 0UL;
		}
}

const DWORD CBonBlackTuner::GetCurSpace(void)
{
	// 現在のチューニング空間を返す
	return m_dwCurSpace;
}

const DWORD CBonBlackTuner::GetCurChannel(void)
{
	// 現在のチャンネルを返す
	return m_dwCurChannel;
}

const bool CBonBlackTuner::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	// チューニングシーケンス可変部分
	static const BYTE abyPllConfA[] = {0x08U, 0x08U, 0x08U, 0x08U, 0x09U, 0x09U, 0x09U, 0x0AU, 0x0AU, 0x0AU, 0x0AU, 0x0CU, 0x0CU, 0x0DU, 0x0DU, 0x0DU, 0x0EU, 0x0EU, 0x0EU, 0x0FU, 0x0FU, 0x0FU, 0x10U};	// 1/17回目 +40
	static const BYTE abyPllConfB[] = {0x32U, 0x32U, 0x80U, 0x80U, 0x64U, 0x64U, 0x64U, 0x00U, 0x00U, 0x4CU, 0x4CU, 0x9AU, 0xEAU, 0x3AU, 0x8AU, 0xDAU, 0x2AU, 0x7AU, 0xCAU, 0x1AU, 0x6AU, 0xBAU, 0x0AU};	// 2/18回目 +40
	static const BYTE abyPllConfC[] = {0x40U, 0x40U, 0x40U, 0x40U, 0x40U, 0x40U, 0x40U, 0x40U, 0x40U, 0x40U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};	// 75回目	+02
	static const BYTE abyPllConfD[] = {0x10U, 0x11U, 0x30U, 0x31U, 0x90U, 0x91U, 0x92U, 0xD0U, 0xD1U, 0xF1U, 0xF2U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};	// 76回目	+02

	// チューニング範囲チェック
	if(!EnumChannelName(dwSpace, dwChannel, NULL))return false;

	// リソースハンドル取得
	HRSRC hRsrc = ::FindResource(m_hModule, MAKEINTRESOURCE(IDR_CHSEQ_BLACK), TEXT("BINARY"));
	if(!hRsrc)return false;

	// リソースロード
	HGLOBAL hGlobal = ::LoadResource(m_hModule, hRsrc);
	if(!hGlobal)return false;

	// メモリポインタ取得
	BYTE *pFixedBuff = (BYTE *)::LockResource(hGlobal);
	if(!pFixedBuff)return false;

	// リソースサイズを取得
	const DWORD dwIoReqSize = ::SizeofResource(m_hModule, hRsrc);

	// リソースデータを送信
	const DWORD dwChIndex = dwSpace * 11UL + dwChannel;
	DWORD dwXmitNum = 0UL;
	DWORD dwXmited = 0UL;
	FIXED_REQ_HEAD *pFixedReqHead;

	while(dwXmited < dwIoReqSize){
		// コントロール情報取得
		pFixedReqHead = (FIXED_REQ_HEAD *)pFixedBuff;
		pFixedBuff += sizeof(FIXED_REQ_HEAD);

		// 送信データ準備
		::CopyMemory(m_RxdBuff, pFixedBuff, pFixedReqHead->wTxdSize);

		switch(dwXmitNum++){
			case 0UL  :	m_RxdBuff[40] = abyPllConfA[dwChIndex];	break;
			case 1UL  :	m_RxdBuff[40] = abyPllConfB[dwChIndex];	break;
			case 16UL :	m_RxdBuff[40] = abyPllConfA[dwChIndex];	break;
			case 17UL :	m_RxdBuff[40] = abyPllConfB[dwChIndex];	break;
			case 74UL :	m_RxdBuff[ 2] = abyPllConfC[dwChIndex];	break;
			case 75UL :	m_RxdBuff[ 2] = abyPllConfD[dwChIndex];	break;
			}

		// データ送信
		if(SendIoRequest(pFixedReqHead->dwIoCode, m_RxdBuff, pFixedReqHead->wTxdSize, m_RxdBuff, pFixedReqHead->wRxdSize) == -1)return false;
		pFixedBuff += pFixedReqHead->wTxdSize;
		
		// 送信済サイズ更新
		dwXmited += (sizeof(FIXED_REQ_HEAD) + pFixedReqHead->wTxdSize);
		}

	// チャンネル情報更新
	m_dwCurSpace = dwSpace;
	m_dwCurChannel = dwChannel;

	// ストリームパージ
	PurgeStream();

	return true;
}

const bool CBonBlackTuner::SetLnbPower(const bool bEnable)
{
	// 常に非対応(過電流保護機能のないFriioでは焼損の可能性があるため)
	return false;
}

const float CBonBlackTuner::GetSignalLevel(void)
{
	// 信号レベル要求
	static const BYTE abyGetLevReq[] = {0xC0, 0x02 ,0x00, 0x32, 0xBA, 0x00, 0x04, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	// 線形補完で近似する
	static const float afLevelTable[] =
	{
		24.07f,	// 00	00	0		24.07dB
		24.07f,	// 10	00	4096	24.07dB
		18.61f,	// 20	00	8192	18.61dB
		15.21f,	// 30	00	12288	15.21dB
		12.50f,	// 40	00	16384	12.50dB
		10.19f,	// 50	00	20480	10.19dB
		8.140f,	// 60	00	24576	8.140dB
		6.270f,	// 70	00	28672	6.270dB
		4.550f,	// 80	00	32768	4.550dB
		3.730f,	// 88	00	34816	3.730dB
		3.630f,	// 88	FF	35071	3.630dB
		2.940f,	// 90	00	36864	2.940dB
		1.420f,	// A0	00	40960	1.420dB
		0.000f	// B0	00	45056	-0.01dB
	};

	// 受信バッファクリア
	::ZeroMemory(m_RxdBuff, sizeof(abyGetLevReq));

	// 信号レベル要求
	if(SendIoRequest(IOCODE_CONTROL, (BYTE *)abyGetLevReq, sizeof(abyGetLevReq), m_RxdBuff, sizeof(abyGetLevReq)) == -1)return 0.0f;

	// 信号レベル計算
	if(m_RxdBuff[40] <= 0x10U){
		// 最大クリップ
		return 24.07f;
		}
	else if(m_RxdBuff[40] >= 0xB0U){
		// 最小クリップ
		return 0.0f;
		}
	else{
		// 線形補完
		const float fMixRate = (float)(((WORD)(m_RxdBuff[40] & 0x0FU) << 8) | (WORD)m_RxdBuff[41]) / 4095.0f;
		return afLevelTable[m_RxdBuff[40] >> 4] * (1.0f - fMixRate) + afLevelTable[(m_RxdBuff[40] >> 4) + 0x01U] * fMixRate;
		}
}

const bool CBonBlackTuner::GetStream(BYTE **ppStream, DWORD *pdwSize, DWORD *pdwRemain)
{
	CBlockLock AutoLock(&m_AsyncReqLock);

	// 受信キューから処理済データを取り出す
	if(m_hFriioDriver == INVALID_HANDLE_VALUE){
		// チューナがオープンされていない
		return false;
		}
	else if(m_AsyncRecvQue.empty()){
		// 取り出し可能なデータがない
		if(ppStream)*ppStream = NULL;
		if(pdwSize)*pdwSize = 0UL;
		if(pdwRemain)*pdwRemain = 0UL;
		}
	else{	
		// データを取り出す
		if(ppStream)*ppStream = m_AsyncRecvQue.front()->RxdBuff;
		if(pdwSize)*pdwSize = m_AsyncRecvQue.front()->dwRxdSize;

		m_AsyncRecvQue.pop();
		
		if(pdwRemain)*pdwRemain = m_AsyncRecvQue.size();
		}

	return true;
}

void CBonBlackTuner::PurgeStream(void)
{
	CBlockLock AutoLock(&m_AsyncReqLock);

	// 受信キュークリア
	while(!m_AsyncRecvQue.empty())m_AsyncRecvQue.pop();
}

const DWORD CBonBlackTuner::WaitStream(const DWORD dwTimeOut)
{
	// ストリーム到着ウェイト
	return m_TsRecvEvent.WaitEvent(dwTimeOut);
}

const DWORD CBonBlackTuner::GetAvailableNum(void)
{
	// 取り出し可能ストリーム数を返す
	return m_AsyncRecvQue.size();
}

CBonBlackTuner::CBonBlackTuner(IBonObject *pOwner)
	: CBonObject(pOwner)
	, m_hFriioDriver(NULL)
	, m_dwCurSpace(0UL)
	, m_dwCurChannel(0UL)
{

}

CBonBlackTuner::~CBonBlackTuner(void)
{
	CloseTuner();
}

void CBonBlackTuner::SetModuleHandle(const HINSTANCE hModule)
{
	// モジュールハンドルを登録
	m_hModule = hModule;
}

const int CBonBlackTuner::SendIoRequest(const DWORD dwIoCode, const BYTE *pTxdData, const DWORD dwTxdSize, BYTE *pRxdData, const DWORD dwRxdSize)
{
	// オープンチェック
	if(m_hFriioDriver == INVALID_HANDLE_VALUE)return -1;

	// 送信用データ設定
	OVERLAPPED OverLapped;
	::ZeroMemory(&OverLapped, sizeof(OverLapped));
	if(!(OverLapped.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL)))return -1;

	// 送信データコピー
	::CopyMemory(m_TxdBuff, pTxdData, dwTxdSize);

	// データ送信
	DWORD dwReturned = 0UL;
	const BOOL bReturn = ::DeviceIoControl(m_hFriioDriver, dwIoCode, m_TxdBuff, dwTxdSize, pRxdData, dwRxdSize, &dwReturned, &OverLapped);
	
	// イベント削除
	::CloseHandle(OverLapped.hEvent);

	// エラー判定、受信データ取得
	if(!bReturn){
		if(::GetLastError() != ERROR_IO_PENDING)return -1;
		if(!::GetOverlappedResult(m_hFriioDriver, &OverLapped, &dwReturned, TRUE))return -1;
		}

	return (int)dwReturned;
}

const bool CBonBlackTuner::SendFixedRequest(const UINT nID)
{
	// リソースハンドル取得
	HRSRC hRsrc = ::FindResource(m_hModule, MAKEINTRESOURCE(nID), TEXT("BINARY"));
	if(!hRsrc)return false;

	// リソースロード
	HGLOBAL hGlobal = ::LoadResource(m_hModule, hRsrc);
	if(!hGlobal)return false;

	// メモリポインタ取得
	BYTE *pFixedBuff = (BYTE *)::LockResource(hGlobal);
	if(!pFixedBuff)return false;

	// リソースサイズを取得
	const DWORD dwIoReqSize = ::SizeofResource(m_hModule, hRsrc);

	// リソースデータを送信
	DWORD dwXmited = 0UL;
	FIXED_REQ_HEAD *pFixedReqHead;

	while(dwXmited < dwIoReqSize){
		// コントロール情報取得
		pFixedReqHead = (FIXED_REQ_HEAD *)pFixedBuff;
		pFixedBuff += sizeof(FIXED_REQ_HEAD);

		// データ送信
		if(SendIoRequest(pFixedReqHead->dwIoCode, pFixedBuff, pFixedReqHead->wTxdSize, m_RxdBuff, pFixedReqHead->wRxdSize) == -1)return false;
		pFixedBuff += pFixedReqHead->wTxdSize;
		
		// 送信済サイズ更新
		dwXmited += (sizeof(FIXED_REQ_HEAD) + pFixedReqHead->wTxdSize);
		}

	return true;
}

const bool CBonBlackTuner::AllocateBuffer(const DWORD dwBufferNum)
{
	// バッファを確保する
	m_AsyncReqPool.resize(dwBufferNum);
	if(m_AsyncReqPool.size() < dwBufferNum)return false;

	// バッファプール位置初期化
	m_itAsyncReq = m_AsyncReqPool.begin();

	return true;
}

void CBonBlackTuner::ReleaseBuffer(void)
{
	// リクエストキュー開放
	while(!m_AsyncReqQue.empty())m_AsyncReqQue.pop();

	// 受信キュー開放
	while(!m_AsyncRecvQue.empty())m_AsyncRecvQue.pop();

	// バッファを開放する
	m_AsyncReqPool.clear();
}

void CBonBlackTuner::IoReqThread(CSmartThread<CBonBlackTuner> *pThread, bool &bKillSignal, PVOID pParam)
{
	// ドライバにストリームリクエストを発行する
	while(!bKillSignal){
		// リクエスト処理待ちが規定未満なら追加する
		if(m_AsyncReqQue.size() < REQRESERVNUM){

			// ドライバにTSデータリクエストを発行する
			if(!PushIoRequest())break;

			continue;
			}

		// リクエスト処理待ちがフルの場合はウェイト
		::Sleep(REQPOLLINGWAIT);
		}
}

void CBonBlackTuner::IoRecvThread(CSmartThread<CBonBlackTuner> *pThread, bool &bKillSignal, PVOID pParam)
{
	// 処理済リクエストをポーリングしてリクエストを完了させる
	while(!bKillSignal){
		// 処理済データがあればリクエストを完了する
		if(m_AsyncReqQue.size() && (m_AsyncRecvQue.size() < ASYNCBUFFNUM)){

			// リクエストを完了する
			if(!PopIoRequest())break;

			continue;
			}

		// 処理済キューがフルの場合はウェイト
		::Sleep(REQPOLLINGWAIT);
		}
}

const bool CBonBlackTuner::PushIoRequest(void)
{
	// ストリームリクエスト
	static const BYTE abyStreamReq[]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	// リクエストキューにリクエストを追加する
	if(m_hFriioDriver == INVALID_HANDLE_VALUE)return false;

	// リクエスト設定
	m_itAsyncReq->dwRxdSize = 0UL;
	::CopyMemory(m_itAsyncReq->TxdBuff, abyStreamReq, sizeof(abyStreamReq));
	::ZeroMemory(&m_itAsyncReq->OverLapped, sizeof(OVERLAPPED));
	if(!(m_itAsyncReq->OverLapped.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL)))return false;
	
	// リクエスト発行
	if(!::DeviceIoControl(m_hFriioDriver, IOCODE_STREAM, m_itAsyncReq->TxdBuff, sizeof(abyStreamReq), m_itAsyncReq->RxdBuff, TSRECVSIZE, &m_itAsyncReq->dwRxdSize, &m_itAsyncReq->OverLapped)){
		if(::GetLastError() != ERROR_IO_PENDING){
			::CloseHandle(m_itAsyncReq->OverLapped.hEvent);
			return false;
			}
		}

	// キューに投入
	m_AsyncReqLock.Lock();
	m_AsyncReqQue.push(&(*m_itAsyncReq));
	if(++m_itAsyncReq == m_AsyncReqPool.end())m_itAsyncReq = m_AsyncReqPool.begin();
	m_AsyncReqLock.Unlock();
	
	return true;
}

const bool CBonBlackTuner::PopIoRequest(void)
{
	// リクエストキューから処理済データを取り出し、受信キューに追加する
	if(m_hFriioDriver == INVALID_HANDLE_VALUE)return false;

	// リクエスト処理結果取得
	m_AsyncReqLock.Lock();
	ASYNC_REQ_DATA *pFrontReq = m_AsyncReqQue.front();
	m_AsyncReqLock.Unlock();
	
	if(!::GetOverlappedResult(m_hFriioDriver, &pFrontReq->OverLapped, &pFrontReq->dwRxdSize, FALSE)){
		if(::GetLastError() == ERROR_IO_INCOMPLETE){
			// 処理未完了、ウェイト
			::Sleep(REQPOLLINGWAIT);
			return true;
			}
		else{
			// エラー発生
			::CloseHandle(pFrontReq->OverLapped.hEvent);		
			return false;
			}
		}

	// イベント開放
	::CloseHandle(pFrontReq->OverLapped.hEvent);

	// キーから取り出し、追加
	m_AsyncReqLock.Lock();
	m_AsyncReqQue.pop();
	m_AsyncRecvQue.push(pFrontReq);
	m_AsyncReqLock.Unlock();

	// イベントセット
	m_TsRecvEvent.SetEvent();

	return true;
}
