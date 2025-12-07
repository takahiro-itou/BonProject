// PtDevice.cpp: PTデバイスクラス
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BonSDK.h"
#include "PtDevice.h"
#include <TChar.h>
#include <MmSystem.h>


#pragma comment(lib, "WinMm.lib")


/////////////////////////////////////////////////////////////////////////////
// PTマネージャクラス
/////////////////////////////////////////////////////////////////////////////

CPtManager::CPtManager(void)
	: m_pPtBus(NULL)
	, m_pPtLibrary(new Library)
{
	// タイマ初期化
	::timeBeginPeriod(1UL);

	::BON_ASSERT(m_pPtLibrary != NULL);
	::BON_ASSERT(EnumerateBoards());
}

CPtManager::~CPtManager(void)
{
	::BON_ASSERT(ReleaseInstance());

	BON_SAFE_DELETE(m_pPtLibrary);

	// タイマ開放
	::timeEndPeriod(1UL);
}

const DWORD CPtManager::GetTotalTunerNum(const Device::ISDB dwTunerType)
{
	// 装着されているPT1のチューナ数を返す
	return m_Boards.size() * CPtBoard::TUNER_NUM;
}

const DWORD CPtManager::GetActiveTunerNum(const Device::ISDB dwTunerType)
{
	// 使用されているPT1のチューナ数を返す
	DWORD dwActiveNum = 0UL;

	for(DWORD dwIndex = 0UL ; dwIndex < m_Boards.size() ; dwIndex++){
		dwActiveNum += m_Boards[dwIndex]->GetActiveTunerNum(dwTunerType);
		}

	return dwActiveNum;
}

CPtTuner * CPtManager::OpenTuner(const Device::ISDB dwTunerType)
{
	// 未使用のチューナを検索して返す
	for(DWORD dwBoardID = 0UL ; dwBoardID < m_Boards.size() ; dwBoardID++){
		CPtTuner *pTuner = m_Boards[dwBoardID]->OpenTuner(dwTunerType);
		if(pTuner)return pTuner;
		}

	return NULL;
}

const bool CPtManager::EnumerateBoards(void)
{
	// PT1ボードを列挙する
	try{
		// Busインスタンスファクトリー取得
		Bus::NewBusFunction pfnBusFactory = m_pPtLibrary->Function();
		if(!pfnBusFactory)throw ::BON_EXPECTION(TEXT("SDK_EARTHSOFT_PT1.dllをロードできません"));

		// Busインスタンス生成
		if(pfnBusFactory(&m_pPtBus) != STATUS_OK)throw ::BON_EXPECTION(TEXT("Busインスタンス生成失敗"));
		
		// PT1ボード列挙
		Bus::DeviceInfo DeviceInfo[8];
		DWORD dwDeviceInfoNum = sizeof(DeviceInfo) / sizeof(*DeviceInfo);
		
		if(m_pPtBus->Scan(DeviceInfo, &dwDeviceInfoNum) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTボード列挙失敗"));

		// PT1デバイスインスタンス生成
		for(DWORD dwBoardID = 0UL ; dwBoardID < dwDeviceInfoNum ; dwBoardID++){
			// 全てのPT1ボードのデバイスインスタンスを生成する
			Device *pNewDevice = NULL;
			
			if(m_pPtBus->NewDevice(&DeviceInfo[dwBoardID], &pNewDevice) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTデバイスインスタンス生成失敗"));
			
			// リストに追加
			m_Boards.push_back(new CPtBoard(dwBoardID, pNewDevice, &DeviceInfo[dwBoardID]));
			}
		}
	catch(CBonException &Exception){
		// エラー発生
		Exception.Notify();
		ReleaseInstance();
		return false;
		}
	
	return true;
}

const bool CPtManager::ReleaseInstance(void)
{
	// PTボードのインスタンスを開放する
	for(DWORD dwBoardID = 0UL ; dwBoardID < m_Boards.size() ; dwBoardID++){
		delete m_Boards[dwBoardID];
		}
		
	m_Boards.clear();

	// バスインスタンス開放
	if(m_pPtBus){
		m_pPtBus->Delete();
		m_pPtBus = NULL;
		}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// PTボードクラス
/////////////////////////////////////////////////////////////////////////////

CPtBoard::CPtBoard(const DWORD dwBoardID, Device *pDevice, const Bus::DeviceInfo *pDeviceInfo)
	: m_dwBoardID(dwBoardID)
	, m_pDevice(pDevice)
	, m_DeviceInfo(*pDeviceInfo)
{
	::BON_ASSERT(m_pDevice != NULL);

	// チューナインスタンス生成
	for(DWORD dwTunerID = 0UL ; dwTunerID < TUNER_NUM ; dwTunerID++){
		m_TunerList.push_back(new CPtTuner(*this, dwTunerID, Device::ISDB_S));
		m_TunerList.push_back(new CPtTuner(*this, dwTunerID, Device::ISDB_T));
		}
}

CPtBoard::~CPtBoard(void)
{
	// チューナインスタンス開放
	for(DWORD dwIndex = 0UL ; dwIndex < m_TunerList.size() ; dwIndex++){
		delete m_TunerList[dwIndex];
		}

	// デバイスインスタンス開放
	m_pDevice->Delete();
}

CPtTuner * CPtBoard::OpenTuner(const Device::ISDB dwTunerType)
{
	CBlockLock AutoLock(&m_TunerLock);
	CPtTuner *pOpenTuner = NULL;

	// オープンするチューナを検索する(アンテナ端子S1/T2優先)
	DWORD dwTunerID = m_TunerList.size();
	
	while(dwTunerID--){
		if(!m_TunerList[dwTunerID]->m_bIsOpen && (m_TunerList[dwTunerID]->m_dwTunerType == dwTunerType)){
			// 未使用チューナかつ種類が一致
			pOpenTuner = m_TunerList[dwTunerID];
			}
		}

	if(!pOpenTuner)return NULL;
	
	if(!GetOpenTunerNum()){
		// 全チューナが未使用の場合はボードをオープン
		if(!OpenBoard())return NULL;
		}
		
	// オープンフラグセット
	pOpenTuner->m_bIsOpen = true;

	return pOpenTuner;
}

void CPtBoard::CloseTuner(CPtTuner *pTuner)
{
	CBlockLock AutoLock(&m_TunerLock);

	if(!IsValidTuner(pTuner))return;
	if(!pTuner->m_bIsOpen)return;

	// ストリーム停止、オープンフラグクリア
	pTuner->StopStream();
	pTuner->m_bIsOpen = false;

	if(!GetOpenTunerNum()){
		// 全チューナが未使用の場合はボードをクローズ
		CloseBoard();
		}
}

const DWORD CPtBoard::GetActiveTunerNum(const Device::ISDB dwTunerType)
{
	// ボードグローバルロックMutex存在確認	※「/バス番号/スロット番号/ファンクション/PT1/プロセスID/」
	TCHAR szMutexName[1024];
	::_stprintf(szMutexName, TEXT("/%lu/%lu/%lu/PT1/"), m_DeviceInfo.Bus, m_DeviceInfo.Slot, m_DeviceInfo.Function);

	if(!CSmartMutex::IsExist(szMutexName)){
		// ボードが未使用の場合
		return 0UL;
		}
	else if(!GetOpenTunerNum()){
		// 他プロセスがボードを使用している場合
		return TUNER_NUM;
		}
	else{
		// 自プロセスがボードを使用している場合
		DWORD dwActiveNum = 0UL;

		for(DWORD dwIndex = 0UL ; dwIndex < m_TunerList.size() ; dwIndex++){
			if(m_TunerList[dwIndex]->m_dwTunerType == dwTunerType){
				if(m_TunerList[dwIndex]->m_bIsOpen)dwActiveNum++;
				}
			}
		
		// 使用中のチューナ数を返す
		return dwActiveNum;
		}
}

const bool CPtBoard::StartStream(CPtTuner *pTuner)
{
	if(!IsValidTuner(pTuner))return false;
	if(pTuner->m_bIsStream)return true;

	try{
		// ストリーム転送開始
		if(m_pDevice->SetStreamEnable(pTuner->m_dwTunerID, pTuner->m_dwTunerType, true) != STATUS_OK)::BON_EXPECTION(TEXT("PTチューナストリーム転送開始失敗"));

		if(!GetStreamTunerNum()){
			// ストリーミングスレッド開始
			if(!m_StreamingThread.StartThread(this, &CPtBoard::StreamingThread, NULL, true))throw ::BON_EXPECTION(TEXT("ストリーミングスレッド起動失敗"));
			}
		}
	catch(CBonException &Exception){
		// エラー発生
		Exception.Notify();
		return false;
		}

	// ストリーム受信中フラグセット
	pTuner->m_bIsStream = true;

	return true;
}

void CPtBoard::StopStream(CPtTuner *pTuner)
{
	if(!IsValidTuner(pTuner))return;
	if(!pTuner->m_bIsStream)return;

	// ストリーム受信中フラグクリア
	pTuner->m_bIsStream = false;

	if(!GetStreamTunerNum()){
		// ストリーミングスレッド停止
		m_StreamingThread.EndThread();
		}

	// ストリーム転送終了
	::BON_ASSERT(m_pDevice->SetStreamEnable(pTuner->m_dwTunerID, pTuner->m_dwTunerType, false) == STATUS_OK, TEXT("PTチューナストリーム転送停止失敗"));
}

const bool CPtBoard::SetLnbPower(CPtTuner *pTuner, const bool bEnable)
{
	if(!IsValidTuner(pTuner))return false;
	if(pTuner->m_dwTunerType != Device::ISDB_S)return false;

	// 更新前LNB電源状態取得
	const bool bLastState = (GetLnbOnTunerNum())? true : false;
	
	// LNB電源状態更新
	pTuner->m_bIsLnbOn = bEnable;

	// 更新後LNB電源状態取得
	const bool bNewState = (GetLnbOnTunerNum())? true : false;
	if(bLastState == bNewState)return true;			

	// LNB電源状態設定
	::BON_ASSERT(m_pDevice->SetLnbPower((bNewState)? Device::LNB_POWER_15V : Device::LNB_POWER_OFF) == STATUS_OK, TEXT("PTチューナLNB電源制御失敗"));
	
	return true;
}

const bool CPtBoard::IsValidTuner(const CPtTuner *pTuner) const
{
	// 有効なチューナインスタンスの有無を返す
	for(DWORD dwIndex = 0UL ; dwIndex < m_TunerList.size() ; dwIndex++){
		if(m_TunerList[dwIndex] == pTuner)return true;
		}

	return false;
}

const DWORD CPtBoard::GetOpenTunerNum(void) const
{
	// オープン済みチューナ数を返す
	DWORD dwOpenNum = 0UL;

	for(DWORD dwIndex = 0UL ; dwIndex < m_TunerList.size() ; dwIndex++){
		if(m_TunerList[dwIndex]->m_bIsOpen)dwOpenNum++;
		}

	return dwOpenNum;
}

const DWORD CPtBoard::GetLnbOnTunerNum(void) const
{
	// LNB電源ONのチューナ数を返す
	DWORD dwLnbOnNum = 0UL;

	for(DWORD dwIndex = 0UL ; dwIndex < m_TunerList.size() ; dwIndex++){
		if((m_TunerList[dwIndex]->m_dwTunerType == Device::ISDB_S) && m_TunerList[dwIndex]->m_bIsLnbOn)dwLnbOnNum++;
		}

	return dwLnbOnNum;
}

const DWORD CPtBoard::GetStreamTunerNum(void) const
{
	// ストリーム受信中のチューナ数を返す
	DWORD dwOpenNum = 0UL;

	for(DWORD dwIndex = 0UL ; dwIndex < m_TunerList.size() ; dwIndex++){
		if(m_TunerList[dwIndex]->m_bIsStream)dwOpenNum++;
		}

	return dwOpenNum;
}

const bool CPtBoard::OpenBoard(void)
{
	// ボードをオープンする
	try{
		// デバイスオープン
		if(m_pDevice->Open() != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTデバイスオープン失敗"));
		
		// チューナー「オン/イネーブル」
		if(m_pDevice->SetTunerPowerReset(Device::TUNER_POWER_ON_RESET_ENABLE) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTチューナリセット移行失敗"));

		// 20msウェイト
		::Sleep(20UL);

		// チューナー「オン/ディセーブル」
		if(m_pDevice->SetTunerPowerReset(Device::TUNER_POWER_ON_RESET_DISABLE) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTチューナリセット解除失敗"));

		// 1msウェイト
		::Sleep(1UL);
		
		for(DWORD dwTunerID = 0UL ; dwTunerID < TUNER_NUM ; dwTunerID++){
			// チューナモジュール初期化
			if(m_pDevice->InitTuner(dwTunerID) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTチューナ初期化失敗"));

			// チューナスリープ解除
			for(DWORD dwTunerType = 0UL ; dwTunerType < Device::ISDB_COUNT ; dwTunerType++){
				if(m_pDevice->SetTunerSleep(dwTunerID, (Device::ISDB)dwTunerType, false) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTチューナ省電力解除失敗"));
				}
			}
		
		// ボードグローバルロックMutex作成	※「/バス番号/スロット番号/ファンクション/PT1/」
		TCHAR szMutexName[1024];
		::_stprintf(szMutexName, TEXT("/%lu/%lu/%lu/PT1/"), m_DeviceInfo.Bus, m_DeviceInfo.Slot, m_DeviceInfo.Function);
		if(!m_BoardLock.Create(szMutexName))throw ::BON_EXPECTION(TEXT("PTボードグローバルロックMutex作成失敗"));
		}
	catch(CBonException &Exception){
		// エラー発生
		Exception.Notify();
		CloseBoard();		
		return false;
		}

	return true;
}

void CPtBoard::CloseBoard(void)
{
	// ボードをクローズする
	try{
		// チューナスリープ移行
		for(DWORD dwTunerID = 0UL ; dwTunerID < TUNER_NUM ; dwTunerID++){
			for(DWORD dwTunerType = 0UL ; dwTunerType < Device::ISDB_COUNT ; dwTunerType++){
				if(m_pDevice->SetTunerSleep(dwTunerID, (Device::ISDB)dwTunerType, false) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTチューナ省電力移行失敗"));
				}
			}	

		// ボードをクローズする
		if(m_pDevice->Close() != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTデバイスクローズ失敗"));
		}
	catch(CBonException &Exception){
		// エラー発生
		Exception.Notify();
		}

	// ボードロックMutex開放
	m_BoardLock.Close();
}

void CPtBoard::StreamingThread(CSmartThread<CPtBoard> *pThread, bool &bKillSignal, PVOID pParam)
{
	::BON_TRACE(TEXT("CPtBoard::StreamingThread() Start\n"));

	struct PT_DMABUF_SET
	{
		DWORD *pdwPacket;			// バッファポインタ
		DWORD *pdwEndMark;			// 転送完了マークのポインタ
		PT_DMABUF_SET *pNextSet;	// 次のバッファセットへのポインタ
	};

	// DMAバッファセット
	PT_DMABUF_SET aDmaBufSet[DMABUF_SIZE * DMABUF_NUM * DMACHK_UNIT];
	PT_DMABUF_SET *pDmaBufSet = aDmaBufSet;

	// ディスパッチバッファ
	DWORD dwWaitTime = 0UL;
	DWORD dwDispatchTerm = 0UL;
	CMediaData DispatchBuffer(DMABLK_SIZE);

	try{
		// DMAバッファサイズ設定
		const static Device::BufferInfo BufInfo = {DMABUF_SIZE, DMABUF_NUM, DMABUF_SIZE};
		if(m_pDevice->SetBufferInfo(&BufInfo) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTデバイスDMAバッファ確保失敗"));

		// DMAバッファポインタ取得(環状リストを形成する)
		for(DWORD dwBuffIndex = 0UL ; dwBuffIndex < DMABUF_NUM ; dwBuffIndex++){
			// バッファポインタ取得		
			DWORD *pdwPointer = NULL;
			if(m_pDevice->GetBufferPtr(dwBuffIndex, (PVOID*)&pdwPointer) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTデバイスDMAバッファポインタ取得失敗"));
		
			for(DWORD dwBlockIndex = 0UL ; dwBlockIndex < (DMABUF_SIZE * DMACHK_UNIT) ; dwBlockIndex++){
				// バッファポインタセット
				pDmaBufSet->pdwPacket = &pdwPointer[0]; 
			
				// 転送完了マークポインタセット
				pDmaBufSet->pdwEndMark = &pdwPointer[DMABLK_SIZE / DMACHK_UNIT / 4 - 1];
				pdwPointer = &pDmaBufSet->pdwEndMark[1];
		
				// 次のセットへのポインタをセット
				pDmaBufSet->pNextSet = &pDmaBufSet[1];
				pDmaBufSet++;
				}
			}

		// 最終要素の次セットポインタ補正
		aDmaBufSet[sizeof(aDmaBufSet) / sizeof(*aDmaBufSet) - 1].pNextSet = aDmaBufSet;

		// 万が一DMA転送が停止した場合はここからやり直す(gotoは使いたくないがとりあえずやむなし)
		StartDmaTransfer:

		pDmaBufSet = aDmaBufSet;

		// 転送完了マークリセット
		for(DWORD dwIndex = 0UL ; dwIndex < (sizeof(aDmaBufSet) / sizeof(*aDmaBufSet)) ; dwIndex++){
			*aDmaBufSet[dwIndex].pdwEndMark = 0UL;
			}

		// ディスパッチバッファリセット
		for(DWORD dwIndex = 0UL ; dwIndex < m_TunerList.size() ; dwIndex++){
			m_aDispatchBuff[dwIndex].dwPacketPos = 0UL;
			}
		
		dwDispatchTerm = 0UL;
		DispatchBuffer.ClearSize();

		// DMA転送停止
		if(m_pDevice->SetTransferEnable(false) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTデバイスDMA転送停止失敗"));
	
		// 転送カウンタリセット
		if(m_pDevice->ResetTransferCounter() != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTデバイスDMA転送カウンタリセット失敗"));

		// 転送カウンタインクリメント
		for(DWORD dwIndex = 0UL ; dwIndex < (DMABUF_SIZE * DMABUF_NUM) ; dwIndex++){
			if(m_pDevice->IncrementTransferCounter() != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTデバイスDMA転送カウンタインクリメント失敗"));
			}

		// DMA転送開始
		if(m_pDevice->SetTransferEnable(true) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTデバイスDMA転送開始失敗"));

		// DMA転送処理ループ
		while(!bKillSignal){
		
			for(DWORD dwUnit = 0UL ; !bKillSignal && (dwUnit < DMACHK_UNIT) ; dwUnit++){

				// 転送完了ウェイト
				while(!bKillSignal){

					//	タイムアウトカウンタリセット
					dwWaitTime = 0UL;

					// ブロック転送完了チェック
					if(*pDmaBufSet->pdwEndMark){

						// ディスパッチデータコピー(DMAバッファはCPUキャッシュされないため)
						DispatchBuffer.AddData((const BYTE *)pDmaBufSet->pdwPacket, DMABLK_SIZE / DMACHK_UNIT);

						// マイクロパケットディスパッチ
						if(++dwDispatchTerm >= GetStreamTunerNum()){
							DispatchStream((const DWORD *)DispatchBuffer.GetData(), DispatchBuffer.GetSize() / 4UL);
							DispatchBuffer.ClearSize();
							dwDispatchTerm = 0UL;
							}
					
						// 転送完了マーカーリセット
						*pDmaBufSet->pdwEndMark = 0UL;
	
						// DMAバッファセット更新
						pDmaBufSet = pDmaBufSet->pNextSet;
					
						break;
						}
					else{
						if(++dwWaitTime >= (DMACHK_TIMEOUT / DMACHK_WAIT)){
							// DMA転送タイムアウト時の復帰処理
							::BON_TRACE(TEXT("DMA転送タイムアウト\n"));
							goto StartDmaTransfer;
							}
						else{						
							// ウェイト
							::Sleep(DMACHK_WAIT);
							}				
						}
					}
				}

			// 転送カウンタインクリメント
			if(m_pDevice->IncrementTransferCounter() != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTデバイスDMA転送カウンタインクリメント失敗"));
			}

		// DMA転送停止
		if(m_pDevice->SetTransferEnable(false) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTデバイスDMA転送停止失敗"));

		// DMAバッファ開放
		if(m_pDevice->SetBufferInfo(NULL) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTデバイスDMAバッファ開放失敗"));
		}
	catch(CBonException &Exception){
		// エラー発生
		Exception.Notify();
		return;
		}

	::BON_TRACE(TEXT("CPtBoard::StreamingThread() End\n"));
}

void CPtBoard::DispatchStream(const DWORD *pdwData, const DWORD dwNum)
{
	#pragma pack(1)
	struct MICRO_PACKET
	{
		BYTE abyData[3];	// データ	※順序は 2 → 1 → 0 となる
		BYTE Error : 1;		// エラーフラグ
		BYTE Store : 1;		// TSパケット開始フラグ
		BYTE Count : 3;		// ストリームカウンタ
		BYTE Tuner : 3;		// ストリームインデックス
	};
	#pragma pack()

	const MICRO_PACKET *pPacket = (const MICRO_PACKET *)pdwData;

	// ディスパッチバッファリセット
	for(DWORD dwIndex = 0UL ; dwIndex < m_TunerList.size() ; dwIndex++){
		m_aDispatchBuff[dwIndex].pDataPtr = m_aDispatchBuff[dwIndex].abyDataBuff;
		}

	// マイクロパケット展開、ディスパッチ
	for(DWORD dwPos = 0UL ; dwPos < dwNum ; dwPos++, pPacket++){
		if((pPacket->Tuner >= 1) && (pPacket->Tuner <= 4)){
			DISPATCH_BUFF &DispatchBuff = m_aDispatchBuff[pPacket->Tuner - 1];

			for(DWORD dwData = 0UL ; dwData < 3UL ; dwData++){
				if(!DispatchBuff.dwPacketPos){
					// TSパケット待ち中
					if(pPacket->Store && (pPacket->abyData[2 - dwData] == 0x47)){
						// TSパケット先頭
						*(DispatchBuff.pDataPtr++) = pPacket->abyData[2 - dwData];
						DispatchBuff.dwPacketPos = 1UL;
						}
					}
				else if(DispatchBuff.dwPacketPos++ < 188UL){
					// TSパケットストア中
					*(DispatchBuff.pDataPtr++) = pPacket->abyData[2 - dwData];
					
					if(DispatchBuff.dwPacketPos == 188UL){
						// TSパケットストア完了
						DispatchBuff.dwPacketPos = 0UL;
						}
					}
				}
			}		
		}

	// 各チューナに出力
	for(DWORD dwIndex = 0UL ; dwIndex < m_TunerList.size() ; dwIndex++){
		if(m_TunerList[dwIndex]->m_bIsStream){
			m_TunerList[dwIndex]->OnTsStream(m_aDispatchBuff[dwIndex].abyDataBuff, m_aDispatchBuff[dwIndex].pDataPtr - m_aDispatchBuff[dwIndex].abyDataBuff);
			}
		}
}


/////////////////////////////////////////////////////////////////////////////
// PTチューナクラス
/////////////////////////////////////////////////////////////////////////////

CPtTuner::CPtTuner(CPtBoard &Board, const DWORD dwTunerID, const Device::ISDB dwTunerType)
	: m_Board(Board)
	, m_dwTunerID(dwTunerID)
	, m_dwTunerType(dwTunerType)
	, m_bIsOpen(false)
	, m_bIsStream(false)
	, m_bIsLnbOn(false)
	, m_pTsReceiver(NULL)
{
	// 何もしない
}

CPtTuner::~CPtTuner()
{
	// チューナをクローズ
	CloseTuner();
}

void CPtTuner::CloseTuner(void)
{
	// LNB電源OFF
	SetLnbPower(false);

	// チューナをクローズ
	m_Board.CloseTuner(this);
}

const bool CPtTuner::StartStream(ITsReceiver *pTsReceiver)
{
	// TSレシーバインスタンス登録
	{
		CBlockLock AutoLock(&m_Lock);
		m_pTsReceiver = pTsReceiver;
	}

	// ストリーム受信スタート
	return m_Board.StartStream(this);
}

void CPtTuner::StopStream(void)
{
	// TSレシーバインスタンス登録解除
	{
		CBlockLock AutoLock(&m_Lock);
		m_pTsReceiver = NULL;
	}

	// ストリーム受信終了
	m_Board.StopStream(this);
}

const bool CPtTuner::SetChannel(const DWORD dwChannel, const DWORD dwOffset)
{
	try{
		// 受信周波数設定
		if(m_Board.m_pDevice->SetFrequency(m_dwTunerID, m_dwTunerType, dwChannel, dwOffset) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PT受信周波数設定失敗"));
		}
	catch(CBonException &Exception){
		// エラー発生
		Exception.Notify();
		return false;
		}
	
	return true;
}

const bool CPtTuner::SetTsID(const WORD wTsID)
{
	DWORD dwCurTsID;

	try{
		// チューナの種類をチェック
		if(m_dwTunerType != Device::ISDB_S)throw ::BON_EXPECTION(TEXT("PT受信TS ID設定非対応"));
	
		// 受信TS ID設定
		if(m_Board.m_pDevice->SetIdS(m_dwTunerID, wTsID) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PT受信TS ID設定失敗"));
		
		// 反映ウェイト
		const DWORD dwStartTime = ::GetTickCount();
		
		do{
			::Sleep(1UL);

			// TS ID取得
			if(m_Board.m_pDevice->GetIdS(m_dwTunerID, &dwCurTsID) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PT受信TS ID取得失敗"));
			
			// TS ID設定タイムアウト判定
			if((::GetTickCount() - dwStartTime) >= 500UL){
				// TS IDタイムアウト
				::BON_EXPECTION(TEXT("PT受信TS ID設定タイムアウト")).Notify(false);
				return false;
				}
			}
		while((WORD)dwCurTsID != wTsID);
		}
	catch(CBonException &Exception){
		// エラー発生
		Exception.Notify();
		return false;
		}
	
	return true;
}

const bool CPtTuner::SetLnbPower(const bool bEnable)
{
	// LNB電源制御
	return m_Board.SetLnbPower(this, bEnable);
}

const float CPtTuner::GetSignalLevel(void)
{
	DWORD dwCnr100 = 0UL, dwCurAgc = 0UL, dwMaxAgc = 0UL;
	
	try{
		// CNRを取得
		if(m_Board.m_pDevice->GetCnAgc(m_dwTunerID, m_dwTunerType, &dwCnr100, &dwCurAgc, &dwMaxAgc) != STATUS_OK)throw ::BON_EXPECTION(TEXT("PTチューナCNR取得失敗"));
		}
	catch(CBonException &Exception){
		// エラー発生
		Exception.Notify();
		return 0.0f;
		}
		
	// dBに変換して返す
	return (float)dwCnr100 / 100.0f;
}

void CPtTuner::OnTsStream(const BYTE *pData, const DWORD dwSize)
{
	CBlockLock AutoLock(&m_Lock);

	// TSレシーバに転送
	if(m_pTsReceiver){
		m_pTsReceiver->OnTsStream(this, pData, dwSize);
		}
}
