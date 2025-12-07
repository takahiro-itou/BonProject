// PtBaseTuner.cpp: PTチューナ基底クラス
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BonSDK.h"
#include "PtBaseTuner.h"
#include <TChar.h>


/////////////////////////////////////////////////////////////////////////////
// ファイルローカル定数設定
/////////////////////////////////////////////////////////////////////////////

// デバイス名
#define DEVICENAME_TER	TEXT("Earthsoft PT1 ISDB-T デジタルチューナ")
#define DEVICENAME_SAT	TEXT("Earthsoft PT1 ISDB-S デジタルチューナ")

// バッファ設定
#define FIFOBUFFTIME	2UL														// バッファ長 = 2秒
#define FIFOBLOCKSIZE	(CPtBoard::DMABLK_SIZE / CPtBoard::DMACHK_UNIT / 3UL)	// 初期ブロックサイズ
#define FIFOBUFFNUM		(0x3C0000UL / FIFOBLOCKSIZE * FIFOBUFFTIME)				// 平均30Mbpsとする


/////////////////////////////////////////////////////////////////////////////
// PT1チューナプロパティクラス
/////////////////////////////////////////////////////////////////////////////

const bool CPtBaseTunerProperty::LoadProperty(IConfigStorage *pStorage)
{
	if(!pStorage)return false;

	// 設定読み出し
	m_bDefaultLnbPower = pStorage->GetBoolItem(TEXT("DefaultLnbPower"	), false);
	m_bEnaleVHF		   = pStorage->GetBoolItem(TEXT("EnaleVHF"			), true	);
	m_bEnaleCATV	   = pStorage->GetBoolItem(TEXT("EnaleCATV"			), true	);
	m_bEnaleCS		   = pStorage->GetBoolItem(TEXT("EnaleCS"			), true	);
	m_bEnaleBS		   = pStorage->GetBoolItem(TEXT("EnaleBS"			), true	);

	return true;
}

const bool CPtBaseTunerProperty::SaveProperty(IConfigStorage *pStorage)
{
	if(!pStorage)return false;
	
	// 設定保存
	try{
		if(!pStorage->SetBoolItem(TEXT("DefaultLnbPower"), m_bDefaultLnbPower	))throw ::BON_EXPECTION();
		if(!pStorage->SetBoolItem(TEXT("EnaleVHF"		), m_bEnaleVHF			))throw ::BON_EXPECTION();
		if(!pStorage->SetBoolItem(TEXT("EnaleCATV"		), m_bEnaleCATV			))throw ::BON_EXPECTION();
		if(!pStorage->SetBoolItem(TEXT("EnaleCS"		), m_bEnaleCS			))throw ::BON_EXPECTION();
		if(!pStorage->SetBoolItem(TEXT("EnaleBS"		), m_bEnaleBS			))throw ::BON_EXPECTION();
		}
	catch(CBonException &Exception){
		// エラー発生
		Exception.Notify();
		return false;
		}
	
	return true;
}

IConfigTarget * CPtBaseTunerProperty::GetConfigTarget(void)
{
	// コンフィグターゲットを返す
	return m_pConfigTarget;
}

const bool CPtBaseTunerProperty::CopyProperty(const IConfigProperty *pProperty)
{
	const IPtBaseTunerProperty *pPtBaseTunerProperty = dynamic_cast<const IPtBaseTunerProperty *>(pProperty);
	if(!pPtBaseTunerProperty)return false;
	
	// プロパティをコピー
	*(static_cast<IPtBaseTunerProperty *>(this)) = *pPtBaseTunerProperty;
	
	return true;
}

const DWORD CPtBaseTunerProperty::GetDialogClassName(LPTSTR lpszClassName)
{
	// プロパティダイアログは非実装
	return 0UL;
}

CPtBaseTunerProperty::CPtBaseTunerProperty(IBonObject *pOwner)
	: CBonObject(pOwner)
	, m_pConfigTarget(dynamic_cast<IConfigTarget *>(pOwner))
{
	// デフォルト値を設定
	m_bDefaultLnbPower = false;
	m_bEnaleVHF = true;
	m_bEnaleCATV = true;
	m_bEnaleCS = true;
	m_bEnaleBS = true;
}

CPtBaseTunerProperty::~CPtBaseTunerProperty(void)
{
	// 何もしない
}


/////////////////////////////////////////////////////////////////////////////
// PT1チューナ基底クラス
/////////////////////////////////////////////////////////////////////////////

CPtManager *CPtBaseTuner::m_pPtManager = NULL;
DWORD CPtBaseTuner::m_dwInstanceCount = 0UL;

const BONGUID CPtBaseTuner::GetDeviceType(void)
{
	// デバイスのタイプを返す
	return ::BON_NAME_TO_GUID(TEXT("IHalTsTuner"));
}

const DWORD CPtBaseTuner::GetDeviceName(LPTSTR lpszName)
{
	static const TCHAR aszDeviceName[][256] = {DEVICENAME_TER, DEVICENAME_SAT};

	// デバイス名を返す
	if(lpszName)::_tcscpy(lpszName, aszDeviceName[m_dwTunerType]);

	return ::_tcslen(aszDeviceName[m_dwTunerType]);
}

const DWORD CPtBaseTuner::GetTotalDeviceNum(void)
{
	// チューナの総数を返す
	return m_pPtManager->GetTotalTunerNum(m_dwTunerType);
}

const DWORD CPtBaseTuner::GetActiveDeviceNum(void)
{
	// 使用中のチューナ数を返す
	return m_pPtManager->GetActiveTunerNum(m_dwTunerType);
}

const bool CPtBaseTuner::OpenTuner(void)
{
	// 一旦クローズ
	CloseTuner();
	
	try{
		// チューナをオープンする
		if(!(m_pTuner = m_pPtManager->OpenTuner(m_dwTunerType)))throw ::BON_EXPECTION(TEXT("チューナオープン失敗"));

		// LNB電源設定
		if(m_dwTunerType == Device::ISDB_S){
			if(!SetLnbPower(m_Property.m_bDefaultLnbPower))throw ::BON_EXPECTION(TEXT("初期LNB電源設定失敗"));
			}

		// デフォルトチャンネル設定
		SetChannel(m_dwCurSpace, m_dwCurChannel);

		// FIFOバッファ確保
		m_BlockPool.resize(FIFOBUFFNUM, CMediaData(FIFOBLOCKSIZE));
		m_itFreeBlock = m_BlockPool.begin();

		// ストリーム受信開始
		if(!m_pTuner->StartStream(this))throw ::BON_EXPECTION(TEXT("TSストリーム受信開始失敗"));
		}
	catch(CBonException &Exception){
		// エラー発生
		Exception.Notify();
		CloseTuner();
		return false;
		}
	
	return true;
}

void CPtBaseTuner::CloseTuner(void)
{
	// チューナをクローズする
	if(m_pTuner){
		m_pTuner->StopStream();
		m_pTuner->CloseTuner();
		m_pTuner = NULL;
		}
	
	// FIFOバッファ開放
	{
		CBlockLock AutoLock(&m_FifoLock);
	
		while(!m_FifoBuffer.empty())m_FifoBuffer.pop();
		m_BlockPool.clear();
	}
}

const DWORD CPtBaseTuner::EnumTuningSpace(const DWORD dwSpace, LPTSTR lpszSpace)
{
	// 使用可能なチューニング空間を返す
	if(dwSpace >= m_TuningSpace.size())return 0UL;

	// チューニング空間名をコピー
	if(lpszSpace)::_tcscpy(lpszSpace, m_TuningSpace[dwSpace]->szSpaceName);

	// チューニング空間名長を返す
	return ::_tcslen(m_TuningSpace[dwSpace]->szSpaceName);
}

const DWORD CPtBaseTuner::EnumChannelName(const DWORD dwSpace, const DWORD dwChannel, LPTSTR lpszChannel)
{
	// 使用可能なチャンネルを返す
	if(dwSpace >= m_TuningSpace.size())return 0UL;

	if(dwChannel >= m_TuningSpace[dwSpace]->dwChNum)return 0UL;

	// チャンネル名をコピー
	if(lpszChannel)::_tcscpy(lpszChannel, m_TuningSpace[dwSpace]->pChConfig[dwChannel].szChName);

	// チャンネル名長を返す
	return ::_tcslen(m_TuningSpace[dwSpace]->pChConfig[dwChannel].szChName);
}

const DWORD CPtBaseTuner::GetCurSpace(void)
{
	// 現在のチューニング空間を返す
	return m_dwCurSpace;
}

const DWORD CPtBaseTuner::GetCurChannel(void)
{
	// 現在のチャンネルを返す
	return m_dwCurChannel;
}

const bool CPtBaseTuner::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	// 何もしない、派生クラスでオーバーライドしなければならない
	::BON_ASSERT(false);
	
	return false;
}

const bool CPtBaseTuner::SetLnbPower(const bool bEnable)
{
	// LNB電源制御
	return (m_pTuner)? m_pTuner->SetLnbPower(bEnable) : false;
}

const float CPtBaseTuner::GetSignalLevel(void)
{
	// CNRを取得
	return (m_pTuner)? m_pTuner->GetSignalLevel() : 0.0f;
}

const bool CPtBaseTuner::GetStream(BYTE **ppStream, DWORD *pdwSize, DWORD *pdwRemain)
{
	CBlockLock AutoLock(&m_FifoLock);

	// 受信FIFOバッファからストリームを取り出す
	if(!m_pTuner){
		// チューナがオープンされていない
		return false;
		}
	else if(m_FifoBuffer.empty()){
		// 取り出し可能なデータがない
		if(ppStream)*ppStream = NULL;
		if(pdwSize)*pdwSize = 0UL;
		if(pdwRemain)*pdwRemain = 0UL;
		}
	else{	
		// データを取り出す
		if(ppStream)*ppStream = m_FifoBuffer.front()->GetData();
		if(pdwSize)*pdwSize = m_FifoBuffer.front()->GetSize();

		m_FifoBuffer.pop();
		
		if(pdwRemain)*pdwRemain = m_FifoBuffer.size();
		}

	return true;
}

void CPtBaseTuner::PurgeStream(void)
{
	// FIFOバッファクリア
	CBlockLock AutoLock(&m_FifoLock);
	
	while(!m_FifoBuffer.empty())m_FifoBuffer.pop();
}

const DWORD CPtBaseTuner::WaitStream(const DWORD dwTimeOut)
{
	// ストリーム到着ウェイト
	return m_FifoEvent.WaitEvent(dwTimeOut);
}

const DWORD CPtBaseTuner::GetAvailableNum(void)
{
	// 取り出し可能ストリーム数を返す
	return m_FifoBuffer.size();
}

const bool CPtBaseTuner::SetProperty(const IConfigProperty *pProperty)
{
	if(!pProperty)return false;

	// プロパティ設定
	return m_Property.CopyProperty(pProperty);
}

const bool CPtBaseTuner::GetProperty(IConfigProperty *pProperty)
{
	if(!pProperty)return false;

	// プロパティ取得
	return pProperty->CopyProperty(&m_Property);
}

const DWORD CPtBaseTuner::GetPropertyClassName(LPTSTR lpszClassName)
{
	static const TCHAR szPropertyClassName[] = TEXT("CPtBaseTunerProperty");

	// プロパティクラス名取得
	if(lpszClassName)::_tcscpy(lpszClassName, szPropertyClassName);

	return ::_tcslen(szPropertyClassName);
}

CPtBaseTuner::CPtBaseTuner(IBonObject *pOwner, const Device::ISDB dwTunerType)
	: CBonObject(pOwner)
	, m_Property(static_cast<IConfigTarget *>(this))
	, m_pTuner(NULL)
	, m_dwTunerType(dwTunerType)
	, m_dwCurSpace(0UL)
	, m_dwCurChannel(0UL)
{
	// マネージャインスタンス生成
	if(!m_dwInstanceCount++){
		m_pPtManager = new CPtManager;
		}

	::BON_ASSERT(m_pPtManager != NULL);

	// デフォルトプロパティ反映
	SetProperty(&m_Property);
}

CPtBaseTuner::~CPtBaseTuner(void)
{
	CloseTuner();
	
	// マネージャインスタンス開放
	if(!--m_dwInstanceCount){
		BON_SAFE_DELETE(m_pPtManager);
		}
}

void CPtBaseTuner::OnTsStream(CPtTuner *pTuner, const BYTE *pData, const DWORD dwSize)
{
	CBlockLock AutoLock(&m_FifoLock);
	
	if(m_FifoBuffer.size() < (FIFOBUFFNUM - 1UL)){
		// FIFOに投入
		m_itFreeBlock->SetData(pData, dwSize);
		m_FifoBuffer.push(&(*m_itFreeBlock));
		if(++m_itFreeBlock == m_BlockPool.end())m_itFreeBlock = m_BlockPool.begin();
		
		// イベントセット
		m_FifoEvent.SetEvent();
		}
	else{
		::BON_TRACE(TEXT("FIFOバッファオーバーフロー\n"));
		}
}
