// BonWhiteTuner.cpp: 白Friioチューナークラス
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Resource.h"
#include "BonSDK.h"
#include "BonWhiteTuner.h"
#include <Math.h>
#include <TChar.h>


/////////////////////////////////////////////////////////////////////////////
// ファイルローカル定数設定
/////////////////////////////////////////////////////////////////////////////

// デバイス名
#define DEVICENAME	TEXT("白 Friio USB 2.0 Digital TV Receiver")

// バッファ設定
#define ASYNCBUFFTIME		2UL											// バッファ長 = 2秒
#define ASYNCBUFFNUM		(0x400000UL / TSRECVSIZE * ASYNCBUFFTIME)	// 平均32Mbpsとする

#define REQRESERVNUM		16UL			// 非同期リクエスト予約数
#define REQPOLLINGWAIT		10UL			// 非同期リクエストポーリング間隔(ms)

// I/Oコントロールコード
#define IOCODE_CONTROL		0x00220020UL	// チューナコントロール
#define IOCODE_SETTSSIZE	0x00220038UL	// TSストリームサイズ設定
#define IOCODE_ABORTPIPE	0x00220044UL	// TSストリーム中止
#define IOCODE_STREAM		0x0022004BUL	// ストリームリクエスト


/////////////////////////////////////////////////////////////////////////////
// 白Friio用　TSチューナ HALインタフェース実装クラス
/////////////////////////////////////////////////////////////////////////////

HINSTANCE CBonWhiteTuner::m_hModule = NULL;

const BONGUID CBonWhiteTuner::GetDeviceType(void)
{
	// デバイスのタイプを返す
	return ::BON_NAME_TO_GUID(TEXT("IHalTsTuner"));
}

const DWORD CBonWhiteTuner::GetDeviceName(LPTSTR lpszName)
{
	// デバイス名を返す
	if(lpszName)::_tcscpy(lpszName, DEVICENAME);

	return ::_tcslen(DEVICENAME);
}

const DWORD CBonWhiteTuner::GetTotalDeviceNum(void)
{
	// 白Friioの総数を返す
	return (m_BonEnumerator.EnumDevice(false))? m_BonEnumerator.GetTotalNum() : 0UL;
}

const DWORD CBonWhiteTuner::GetActiveDeviceNum(void)
{
	// 使用中の白Friioの数を返す
	return (m_BonEnumerator.EnumDevice(false))? m_BonEnumerator.GetActiveNum() : 0UL;
}

const bool CBonWhiteTuner::OpenTuner(void)
{
	// 一旦クローズ
	CloseTuner();
	
	try{
		// ドライバ列挙
		if(!m_BonEnumerator.EnumDevice(false))throw __LINE__;
	
		// 未使用のチューナを取得
		LPCTSTR lpszDriverPath = m_BonEnumerator.GetAvailableDriverPath();
		if(!lpszDriverPath)throw __LINE__;

		// ドライバオープン
		if((m_hFriioDriver = ::CreateFile(lpszDriverPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE)throw __LINE__;

		// ドライバロック
		if(!m_BonEnumerator.LockDevice(lpszDriverPath))throw __LINE__;
		
		// 初期化シーケンス送信
		if(!SendFixedRequest(IDR_INISEQ_WHITE))throw __LINE__;
		
		// デフォルトチャンネル設定
		if(!SetChannel(0UL, 0UL))throw __LINE__;
		
		// イベントリセット
		if(!m_TsRecvEvent.ResetEvent())throw __LINE__;
		
		// バッファ確保
		if(!AllocateBuffer(ASYNCBUFFNUM + REQRESERVNUM))throw __LINE__;
		
		// スレッド起動
		if(!m_IoReqThread.StartThread(this, &CBonWhiteTuner::IoReqThread))throw __LINE__;
		if(!m_IoRecvThread.StartThread(this, &CBonWhiteTuner::IoRecvThread))throw __LINE__;
		}
	catch(...){
		// エラー発生
		CloseTuner();
		return false;
		}
	
	return true;
}

void CBonWhiteTuner::CloseTuner(void)
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
		SendFixedRequest(IDR_ENDSEQ_WHITE);

		// ドライバクローズ
		::CloseHandle(m_hFriioDriver);
		m_hFriioDriver = INVALID_HANDLE_VALUE;
		}

	// ドライバロック開放
	m_BonEnumerator.ReleaseDevice();
	
	// チャンネル初期化
	m_dwCurChannel = 0UL;
}

const DWORD CBonWhiteTuner::EnumTuningSpace(const DWORD dwSpace, LPTSTR lpszSpace)
{
	static const TCHAR aszTuningSpace[][16] = 
	{
		TEXT("地デジ")
	};

	// 使用可能なチューニング空間を返す
	if(dwSpace >= (sizeof(aszTuningSpace) / sizeof(aszTuningSpace[0])))return 0UL;

	// チューニング空間名をコピー
	if(lpszSpace)::_tcscpy(lpszSpace, aszTuningSpace[dwSpace]);

	// チューニング空間名長を返す
	return ::_tcslen(aszTuningSpace[dwSpace]);
}

const DWORD CBonWhiteTuner::EnumChannelName(const DWORD dwSpace, const DWORD dwChannel, LPTSTR lpszChannel)
{
	// 使用可能なチャンネルを返す
	static const TCHAR aszChannelName[][16] = 
	{
		TEXT("UHF 13ch"), TEXT("UHF 14ch"), TEXT("UHF 15ch"), TEXT("UHF 16ch"), TEXT("UHF 17ch"), TEXT("UHF 18ch"), TEXT("UHF 19ch"),
		TEXT("UHF 20ch"), TEXT("UHF 21ch"), TEXT("UHF 22ch"), TEXT("UHF 23ch"), TEXT("UHF 24ch"), TEXT("UHF 25ch"), TEXT("UHF 26ch"), TEXT("UHF 27ch"), TEXT("UHF 28ch"), TEXT("UHF 29ch"),
		TEXT("UHF 30ch"), TEXT("UHF 31ch"), TEXT("UHF 32ch"), TEXT("UHF 33ch"), TEXT("UHF 34ch"), TEXT("UHF 35ch"), TEXT("UHF 36ch"), TEXT("UHF 37ch"), TEXT("UHF 38ch"), TEXT("UHF 39ch"), 
		TEXT("UHF 40ch"), TEXT("UHF 41ch"), TEXT("UHF 42ch"), TEXT("UHF 43ch"), TEXT("UHF 44ch"), TEXT("UHF 45ch"), TEXT("UHF 46ch"), TEXT("UHF 47ch"), TEXT("UHF 48ch"), TEXT("UHF 49ch"), 
		TEXT("UHF 50ch"), TEXT("UHF 51ch"), TEXT("UHF 52ch"), TEXT("UHF 53ch"), TEXT("UHF 54ch"), TEXT("UHF 55ch"), TEXT("UHF 56ch"), TEXT("UHF 57ch"), TEXT("UHF 58ch"), TEXT("UHF 59ch"), 
		TEXT("UHF 60ch"), TEXT("UHF 61ch"), TEXT("UHF 62ch")
	};

	// 使用可能なチャンネルを返す
	if(!EnumTuningSpace(dwSpace, NULL))return 0UL;
	if(dwChannel >= (sizeof(aszChannelName) / sizeof(aszChannelName[0])))return 0UL;

	// チャンネル名をコピー
	if(lpszChannel)::_tcscpy(lpszChannel, aszChannelName[dwChannel]);

	// チャンネル名長を返す
	return ::_tcslen(aszChannelName[dwSpace]);
}

const DWORD CBonWhiteTuner::GetCurSpace(void)
{
	// 現在のチューニング空間を返す
	return 0UL;
}

const DWORD CBonWhiteTuner::GetCurChannel(void)
{
	// 現在のチャンネルを返す
	return m_dwCurChannel;
}

const bool CBonWhiteTuner::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	// チューニングシーケンス可変部分
	static const BYTE abyPllConfA[] = {0x81U};
	static const BYTE abyPllConfB[] = {0x40U, 0x03U, 0x00U, 0x30U, 0xFEU, 0x00U, 0x05U, 0x00U, 0x0AU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x26U, 0x00U, 0x00U, 0x00U, 0x05U, 0x00U, 0x00U, 0x00U, 0xC0U, 0x00U, 0x00U, 0xB2U, 0x08U};
	static const BYTE abyPllConfC[] = {0x40U, 0x03U, 0x00U, 0x30U, 0xFEU, 0x00U, 0x05U, 0x00U, 0x0AU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x26U, 0x00U, 0x00U, 0x00U, 0x05U, 0x00U, 0x00U, 0x00U, 0xC0U, 0x00U, 0x00U, 0x9AU, 0x50U};
	static const BYTE abyPllConfD[] = {0xC0U, 0x02U, 0x00U, 0x30U, 0xB0U, 0x00U, 0x01U, 0x00U, 0x0AU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x26U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U};
	static const BYTE abyPllConfE[] = {0xC0U, 0x02U, 0x00U, 0x30U, 0x80U, 0x00U, 0x01U, 0x00U, 0x0AU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x26U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U};
	static const BYTE abyPllConfF[] = {0x81U, 0x00U, 0xBCU, 0x00U, 0x00U};

	// チューニング範囲チェック
	if(!EnumChannelName(dwSpace, dwChannel, NULL))return false;

	// チャンネルデータ計算
	const WORD wChCode = 0x0E7F + 0x002A * (WORD)dwChannel;

	BYTE TxdData[256];

	// 1回目リクエスト送信
	if(SendIoRequest(IOCODE_ABORTPIPE, abyPllConfA, sizeof(abyPllConfA), m_RxdBuff, 0UL) == -1)return false;
	if(SendIoRequest(IOCODE_ABORTPIPE, abyPllConfA, sizeof(abyPllConfA), m_RxdBuff, 0UL) == -1)return false;

	// 2回目リクエスト設定
	::CopyMemory(TxdData, abyPllConfB, sizeof(abyPllConfB));
	TxdData[39] = (BYTE)(wChCode >> 8);
	TxdData[40] = (BYTE)(wChCode & 0xFF);

	// 2回目リクエスト送信
	if(SendIoRequest(IOCODE_CONTROL, TxdData, sizeof(abyPllConfB), m_RxdBuff, sizeof(abyPllConfB)) == -1)return false;

	// 3回目リクエスト設定
	CopyMemory(TxdData, abyPllConfC, sizeof(abyPllConfC));
	TxdData[39] = (BYTE)(wChCode >> 8);
	TxdData[40] = (BYTE)(wChCode & 0xFF);

	// 3回目リクエスト送信
	if(SendIoRequest(IOCODE_CONTROL, TxdData, sizeof(abyPllConfC), m_RxdBuff, sizeof(abyPllConfC)) == -1)return false;

	// 4回目リクエスト送信
	if(SendIoRequest(IOCODE_CONTROL, abyPllConfD, sizeof(abyPllConfD), m_RxdBuff, sizeof(abyPllConfD)) == -1)return false;

	// 5回目リクエスト送信
	if(SendIoRequest(IOCODE_CONTROL, abyPllConfE, sizeof(abyPllConfE), m_RxdBuff, sizeof(abyPllConfE)) == -1)return false;

	// 6回目リクエスト送信
	if(SendIoRequest(IOCODE_SETTSSIZE, abyPllConfF, sizeof(abyPllConfF), m_RxdBuff, sizeof(abyPllConfF)) == -1)return false;

	// チャンネル情報更新
	m_dwCurChannel = dwChannel;

	// ストリームパージ
	PurgeStream();

	return true;
}

const bool CBonWhiteTuner::SetLnbPower(const bool bEnable)
{
	// 常に非対応
	return false;
}

const float CBonWhiteTuner::GetSignalLevel(void)
{
	// 信号レベル要求
	static const BYTE abyGetLevReq[] = {0xC0U, 0x02U, 0x00U, 0x30U, 0x89U, 0x00U, 0x25U, 0x00U, 0x0AU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x26U, 0x00U, 0x00U, 0x00U, 0x25U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x31U, 0x3AU, 0x4BU, 0x03U, 0x00U, 0x34U, 0x08U, 0x78U, 0x00U, 0x00U, 0x40U, 0x7FU, 0xDCU, 0xACU, 0x0FU, 0x00U, 0x00U, 0x00U, 0x01U, 0xE3U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x02U, 0x00U, 0x02U, 0x00U, 0x00U, 0xF4U};

	// 受信バッファクリア
	::ZeroMemory(m_RxdBuff, sizeof(abyGetLevReq));

	// 信号レベル要求
	if(SendIoRequest(IOCODE_CONTROL, (BYTE *)abyGetLevReq, sizeof(abyGetLevReq), m_RxdBuff, sizeof(abyGetLevReq)) == -1)return 0.0f;

	// 信号レベル計算
	const DWORD dwHex = ((DWORD)m_RxdBuff[40] << 16) | ((DWORD)m_RxdBuff[41] << 8) | (DWORD)m_RxdBuff[42];
	if(!dwHex)return 0.0f;

	return (float)(68.4344993f - log10((double)dwHex) * 10.0f + pow(log10((double)dwHex) - 5.045f, (double)4.0f) / 4.09f);
}

const bool CBonWhiteTuner::GetStream(BYTE **ppStream, DWORD *pdwSize, DWORD *pdwRemain)
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

void CBonWhiteTuner::PurgeStream(void)
{
	CBlockLock AutoLock(&m_AsyncReqLock);

	// 受信キュークリア
	while(!m_AsyncRecvQue.empty())m_AsyncRecvQue.pop();
}

const DWORD CBonWhiteTuner::WaitStream(const DWORD dwTimeOut)
{
	// ストリーム到着ウェイト
	return m_TsRecvEvent.WaitEvent(dwTimeOut);
}

const DWORD CBonWhiteTuner::GetAvailableNum(void)
{
	// 取り出し可能ストリーム数を返す
	return m_AsyncRecvQue.size();
}

CBonWhiteTuner::CBonWhiteTuner(IBonObject *pOwner)
	: CBonObject(pOwner)
	, m_hFriioDriver(NULL)
	, m_dwCurChannel(0UL)
{

}

CBonWhiteTuner::~CBonWhiteTuner(void)
{
	CloseTuner();
}

void CBonWhiteTuner::SetModuleHandle(const HINSTANCE hModule)
{
	// モジュールハンドルを登録
	m_hModule = hModule;
}

const int CBonWhiteTuner::SendIoRequest(const DWORD dwIoCode, const BYTE *pTxdData, const DWORD dwTxdSize, BYTE *pRxdData, const DWORD dwRxdSize)
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

const bool CBonWhiteTuner::SendFixedRequest(const UINT nID)
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

const bool CBonWhiteTuner::AllocateBuffer(const DWORD dwBufferNum)
{
	// バッファを確保する
	m_AsyncReqPool.resize(dwBufferNum);
	if(m_AsyncReqPool.size() < dwBufferNum)return false;

	// バッファプール位置初期化
	m_itAsyncReq = m_AsyncReqPool.begin();

	return true;
}

void CBonWhiteTuner::ReleaseBuffer(void)
{
	// リクエストキュー開放
	while(!m_AsyncReqQue.empty())m_AsyncReqQue.pop();

	// 受信キュー開放
	while(!m_AsyncRecvQue.empty())m_AsyncRecvQue.pop();

	// バッファを開放する
	m_AsyncReqPool.clear();
}

void CBonWhiteTuner::IoReqThread(CSmartThread<CBonWhiteTuner> *pThread, bool &bKillSignal, PVOID pParam)
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

void CBonWhiteTuner::IoRecvThread(CSmartThread<CBonWhiteTuner> *pThread, bool &bKillSignal, PVOID pParam)
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

const bool CBonWhiteTuner::PushIoRequest(void)
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

const bool CBonWhiteTuner::PopIoRequest(void)
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
