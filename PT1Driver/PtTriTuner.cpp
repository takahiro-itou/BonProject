// PtTriTuner.cpp: PT 地上/BS/110CSチューナクラス
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BonSDK.h"
#include "PtTriTuner.h"
#include <TChar.h>


/////////////////////////////////////////////////////////////////////////////
// ファイルローカル定数設定
/////////////////////////////////////////////////////////////////////////////

// デバイス名
#define DEVICENAME_TRI	TEXT("Earthsoft PT1 ISDB-T/S デジタルチューナ")

// バッファ設定
#define FIFOBUFFTIME	2UL												// バッファ長 = 2秒
#define FIFOBLOCKSIZE	(4096UL * 511UL / 3UL)							// 初期ブロックサイズ
#define FIFOBUFFNUM		(0x3C0000UL / FIFOBLOCKSIZE * FIFOBUFFTIME)		// 平均30Mbpsとする

// UHF チャンネル設定
static const CPtBaseTuner::PT_CH_CONFIG f_aConfigUHF[] =
{
	{TEXT("UHF 13ch"),  63UL, 0x0000U},
	{TEXT("UHF 14ch"),  64UL, 0x0000U},
	{TEXT("UHF 15ch"),  65UL, 0x0000U},
	{TEXT("UHF 16ch"),  66UL, 0x0000U},
	{TEXT("UHF 17ch"),  67UL, 0x0000U},
	{TEXT("UHF 18ch"),  68UL, 0x0000U},
	{TEXT("UHF 19ch"),  69UL, 0x0000U},
	{TEXT("UHF 20ch"),  70UL, 0x0000U},
	{TEXT("UHF 21ch"),  71UL, 0x0000U},
	{TEXT("UHF 22ch"),  72UL, 0x0000U},
	{TEXT("UHF 23ch"),  73UL, 0x0000U},
	{TEXT("UHF 24ch"),  74UL, 0x0000U},
	{TEXT("UHF 25ch"),  75UL, 0x0000U},
	{TEXT("UHF 26ch"),  76UL, 0x0000U},
	{TEXT("UHF 27ch"),  77UL, 0x0000U},
	{TEXT("UHF 28ch"),  78UL, 0x0000U},
	{TEXT("UHF 29ch"),  79UL, 0x0000U},
	{TEXT("UHF 30ch"),  80UL, 0x0000U},
	{TEXT("UHF 31ch"),  81UL, 0x0000U},
	{TEXT("UHF 32ch"),  82UL, 0x0000U},
	{TEXT("UHF 33ch"),  83UL, 0x0000U},
	{TEXT("UHF 34ch"),  84UL, 0x0000U},
	{TEXT("UHF 35ch"),  85UL, 0x0000U},
	{TEXT("UHF 36ch"),  86UL, 0x0000U},
	{TEXT("UHF 37ch"),  87UL, 0x0000U},
	{TEXT("UHF 38ch"),  88UL, 0x0000U},
	{TEXT("UHF 39ch"),  89UL, 0x0000U},
	{TEXT("UHF 40ch"),  90UL, 0x0000U},
	{TEXT("UHF 41ch"),  91UL, 0x0000U},
	{TEXT("UHF 42ch"),  92UL, 0x0000U},
	{TEXT("UHF 43ch"),  93UL, 0x0000U},
	{TEXT("UHF 44ch"),  94UL, 0x0000U},
	{TEXT("UHF 45ch"),  95UL, 0x0000U},
	{TEXT("UHF 46ch"),  96UL, 0x0000U},
	{TEXT("UHF 47ch"),  97UL, 0x0000U},
	{TEXT("UHF 48ch"),  98UL, 0x0000U},
	{TEXT("UHF 49ch"),  99UL, 0x0000U},
	{TEXT("UHF 50ch"), 100UL, 0x0000U},
	{TEXT("UHF 51ch"), 101UL, 0x0000U},
	{TEXT("UHF 52ch"), 102UL, 0x0000U},
	{TEXT("UHF 53ch"), 103UL, 0x0000U},
	{TEXT("UHF 54ch"), 104UL, 0x0000U},
	{TEXT("UHF 55ch"), 105UL, 0x0000U},
	{TEXT("UHF 56ch"), 106UL, 0x0000U},
	{TEXT("UHF 57ch"), 107UL, 0x0000U},
	{TEXT("UHF 58ch"), 108UL, 0x0000U},
	{TEXT("UHF 59ch"), 109UL, 0x0000U},
	{TEXT("UHF 60ch"), 110UL, 0x0000U},
	{TEXT("UHF 61ch"), 111UL, 0x0000U},
	{TEXT("UHF 62ch"), 112UL, 0x0000U}
};

// VHF チャンネル設定
static const CPtBaseTuner::PT_CH_CONFIG f_aConfigVHF[] =
{
	{TEXT("VHF 1ch"),   0UL, 0x0000U},
	{TEXT("VHF 2ch"),   1UL, 0x0000U},
	{TEXT("VHF 3ch"),   2UL, 0x0000U},
	{TEXT("VHF 4ch"),  13UL, 0x0000U},
	{TEXT("VHF 5ch"),  14UL, 0x0000U},
	{TEXT("VHF 6ch"),  15UL, 0x0000U},
	{TEXT("VHF 7ch"),  16UL, 0x0000U},
	{TEXT("VHF 8ch"),  17UL, 0x0000U},
	{TEXT("VHF 9ch"),  18UL, 0x0000U},
	{TEXT("VHF 10ch"), 19UL, 0x0000U},
	{TEXT("VHF 11ch"), 20UL, 0x0000U},
	{TEXT("VHF 12ch"), 21UL, 0x0000U}
};

// CATV チャンネル設定
static const CPtBaseTuner::PT_CH_CONFIG f_aConfigCATV[] =
{
	{TEXT("CATV 13ch"),  3UL, 0x0000U},
	{TEXT("CATV 14ch"),  4UL, 0x0000U},
	{TEXT("CATV 15ch"),  5UL, 0x0000U},
	{TEXT("CATV 16ch"),  6UL, 0x0000U},
	{TEXT("CATV 17ch"),  7UL, 0x0000U},
	{TEXT("CATV 18ch"),  8UL, 0x0000U},
	{TEXT("CATV 19ch"),  9UL, 0x0000U},
	{TEXT("CATV 20ch"), 10UL, 0x0000U},
	{TEXT("CATV 21ch"), 11UL, 0x0000U},
	{TEXT("CATV 22ch"), 12UL, 0x0000U},
	{TEXT("CATV 23ch"), 22UL, 0x0000U},
	{TEXT("CATV 24ch"), 23UL, 0x0000U},
	{TEXT("CATV 25ch"), 24UL, 0x0000U},
	{TEXT("CATV 26ch"), 25UL, 0x0000U},
	{TEXT("CATV 27ch"), 26UL, 0x0000U},
	{TEXT("CATV 28ch"), 27UL, 0x0000U},
	{TEXT("CATV 29ch"), 28UL, 0x0000U},
	{TEXT("CATV 30ch"), 29UL, 0x0000U},
	{TEXT("CATV 31ch"), 30UL, 0x0000U},
	{TEXT("CATV 32ch"), 31UL, 0x0000U},
	{TEXT("CATV 33ch"), 32UL, 0x0000U},
	{TEXT("CATV 34ch"), 33UL, 0x0000U},
	{TEXT("CATV 35ch"), 34UL, 0x0000U},
	{TEXT("CATV 36ch"), 35UL, 0x0000U},
	{TEXT("CATV 37ch"), 36UL, 0x0000U},
	{TEXT("CATV 38ch"), 37UL, 0x0000U},
	{TEXT("CATV 39ch"), 38UL, 0x0000U},
	{TEXT("CATV 40ch"), 39UL, 0x0000U},
	{TEXT("CATV 41ch"), 40UL, 0x0000U},
	{TEXT("CATV 42ch"), 41UL, 0x0000U},
	{TEXT("CATV 43ch"), 42UL, 0x0000U},
	{TEXT("CATV 44ch"), 43UL, 0x0000U},
	{TEXT("CATV 45ch"), 44UL, 0x0000U},
	{TEXT("CATV 46ch"), 45UL, 0x0000U},
	{TEXT("CATV 47ch"), 46UL, 0x0000U},
	{TEXT("CATV 48ch"), 47UL, 0x0000U},
	{TEXT("CATV 49ch"), 48UL, 0x0000U},
	{TEXT("CATV 50ch"), 49UL, 0x0000U},
	{TEXT("CATV 51ch"), 50UL, 0x0000U},
	{TEXT("CATV 52ch"), 51UL, 0x0000U},
	{TEXT("CATV 53ch"), 52UL, 0x0000U},
	{TEXT("CATV 54ch"), 53UL, 0x0000U},
	{TEXT("CATV 55ch"), 54UL, 0x0000U},
	{TEXT("CATV 56ch"), 55UL, 0x0000U},
	{TEXT("CATV 57ch"), 56UL, 0x0000U},
	{TEXT("CATV 58ch"), 57UL, 0x0000U},
	{TEXT("CATV 59ch"), 58UL, 0x0000U},
	{TEXT("CATV 60ch"), 59UL, 0x0000U},
	{TEXT("CATV 61ch"), 60UL, 0x0000U},
	{TEXT("CATV 62ch"), 61UL, 0x0000U},
	{TEXT("CATV 63ch"), 62UL, 0x0000U}
};

// BS チャンネル設定		※2008/10/12 現在
static const CPtBaseTuner::PT_CH_CONFIG f_aConfigBS[] =
{
	{TEXT("BS1/TS0 BS朝日"			), 0UL, 0x4010U},
	{TEXT("BS1/TS1 BS-i"			), 0UL, 0x4011U},
	{TEXT("BS3/TS0 WOWOW"			), 1UL, 0x4030U},
	{TEXT("BS3/TS1 BSジャパン"		), 1UL, 0x4031U},
	{TEXT("BS9/TS0 BS11"			), 4UL, 0x4090U},
	{TEXT("BS9/TS1 Star Channel HV"	), 4UL, 0x4091U},
	{TEXT("BS9/TS2 TwellV"			), 4UL, 0x4092U},
	{TEXT("BS13/TS0 BS日テレ"		), 6UL, 0x40D0U},
	{TEXT("BS13/TS1 BSフジ"			), 6UL, 0x40D1U},
	{TEXT("BS15/TS1 NHK BS1/2"		), 7UL, 0x40F1U},
	{TEXT("BS15/TS2 NHK BS-hi"		), 7UL, 0x40F2U}
};

// 110CS チャンネル設定		※2008/10/12 現在
static const CPtBaseTuner::PT_CH_CONFIG f_aConfigCS[] =
{
	{TEXT("ND2 110CS #1"  ), 12UL, 0x6020U},
	{TEXT("ND4 110CS #2"  ), 13UL, 0x7040U},
	{TEXT("ND6 110CS #3"  ), 14UL, 0x7060U},
	{TEXT("ND8 110CS #4"  ), 15UL, 0x6080U},
	{TEXT("ND10 110CS #5" ), 16UL, 0x60A0U},
	{TEXT("ND12 110CS #6" ), 17UL, 0x70C0U},
	{TEXT("ND14 110CS #7" ), 18UL, 0x70E0U},
	{TEXT("ND16 110CS #8" ), 19UL, 0x7100U},
	{TEXT("ND18 110CS #9" ), 20UL, 0x7120U},
	{TEXT("ND20 110CS #10"), 21UL, 0x7140U},
	{TEXT("ND22 110CS #11"), 22UL, 0x7160U},
	{TEXT("ND24 110CS #12"), 23UL, 0x7180U}
};

// チャンネル空間設定
static const CPtBaseTuner::PT_CH_SPACE f_aChSpace[] =
{
	{TEXT("UHF"  ), f_aConfigUHF,  sizeof(f_aConfigUHF)  / sizeof(*f_aConfigUHF) },
	{TEXT("VHF"  ), f_aConfigVHF,  sizeof(f_aConfigVHF)  / sizeof(*f_aConfigVHF) },
	{TEXT("CATV" ), f_aConfigCATV, sizeof(f_aConfigCATV) / sizeof(*f_aConfigCATV)},
	{TEXT("BS"   ),	f_aConfigBS,   sizeof(f_aConfigBS)   / sizeof(*f_aConfigBS)  },
	{TEXT("110CS"),	f_aConfigCS,   sizeof(f_aConfigCS)   / sizeof(*f_aConfigCS)  }
};


/////////////////////////////////////////////////////////////////////////////
// PT BS/110CSチューナクラス
/////////////////////////////////////////////////////////////////////////////

const DWORD CPtTriTuner::GetDeviceName(LPTSTR lpszName)
{
	// デバイス名を返す
	if(lpszName)::_tcscpy(lpszName, DEVICENAME_TRI);

	return ::_tcslen(DEVICENAME_TRI);
}

const DWORD CPtTriTuner::GetTotalDeviceNum(void)
{
	// チューナの総数を返す
	const DWORD dwSatTunerNum = m_pPtManager->GetTotalTunerNum(Device::ISDB_S);
	const DWORD dwTerTunerNum = m_pPtManager->GetTotalTunerNum(Device::ISDB_T);

	// 少ない方を返す
	return (dwSatTunerNum <= dwTerTunerNum)? dwSatTunerNum : dwTerTunerNum;
}

const DWORD CPtTriTuner::GetActiveDeviceNum(void)
{
	// 使用中のチューナ数を返す
	const DWORD dwSatTunerNum = m_pPtManager->GetActiveTunerNum(Device::ISDB_S);
	const DWORD dwTerTunerNum = m_pPtManager->GetActiveTunerNum(Device::ISDB_T);

	// 多い方を返す
	return (dwSatTunerNum >= dwTerTunerNum)? dwSatTunerNum : dwTerTunerNum;
}

const bool CPtTriTuner::OpenTuner(void)
{
	try{
		// ISDB-Tチューナをオープンする
		if(!CPtBaseTuner::OpenTuner())throw ::BON_EXPECTION(TEXT("ISDB-Tチューナオープン失敗"));
		
		// カレントチューナセット
		m_apTuner[Device::ISDB_T] = m_pTuner;

		// ISDB-Sチューナをオープンする
		if(!(m_apTuner[Device::ISDB_S] = m_pPtManager->OpenTuner(Device::ISDB_S)))throw ::BON_EXPECTION(TEXT("ISDB-Sチューナオープン失敗"));
		
		// LNB電源設定
		if(!m_apTuner[Device::ISDB_S]->SetLnbPower(m_Property.m_bDefaultLnbPower))throw ::BON_EXPECTION(TEXT("初期LNB電源設定失敗"));
		}
	catch(CBonException &Exception){
		// エラー発生
		Exception.Notify();
		CloseTuner();
		return false;
		}
	
	return true;
}

void CPtTriTuner::CloseTuner(void)
{
	// チューナをクローズする
	for(DWORD dwIndex = 0UL ; dwIndex < (sizeof(m_apTuner) / sizeof(*m_apTuner)) ; dwIndex++){
		if(m_apTuner[dwIndex]){
			m_apTuner[dwIndex]->StopStream();
			m_apTuner[dwIndex]->CloseTuner();
			m_apTuner[dwIndex] = NULL;
			}		
		}
	
	// カレントチューナクリア
	m_pTuner = NULL;
	
	// FIFOバッファ開放
	CPtBaseTuner::CloseTuner();
}

const bool CPtTriTuner::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	if(!m_pTuner || !EnumChannelName(dwSpace, dwChannel, NULL))return false;

	CBlockLock AutoLock(&m_FifoLock);

	// チューニングリクエストに対するチューナインスタンス
	CPtTuner *pReqTuner = (m_TuningSpace[dwSpace]->pChConfig[dwChannel].wTsID)? m_apTuner[Device::ISDB_S] : m_apTuner[Device::ISDB_T];

	if(pReqTuner && (m_pTuner != pReqTuner)){
		// カレントチューナストリーム受信停止
		m_pTuner->StopStream();
		
		// リクエストチューナストリーム受信開始
		if(!pReqTuner->StartStream(this))return false;
		
		// カレントチューナ更新
		m_pTuner = pReqTuner;
		}

	// チャンネル設定
	if(!m_pTuner->SetChannel(m_TuningSpace[dwSpace]->pChConfig[dwChannel].dwChIndex))return false;

	// TS ID設定
	if(m_TuningSpace[dwSpace]->pChConfig[dwChannel].wTsID){
		if(!m_pTuner->SetTsID(m_TuningSpace[dwSpace]->pChConfig[dwChannel].wTsID))return false;
		}

	// チャンネル更新
	m_dwCurSpace = dwSpace;
	m_dwCurChannel = dwChannel;

	return true;
}

const bool CPtTriTuner::SetLnbPower(const bool bEnable)
{
	// LNB電源状態設定
	return (m_apTuner[Device::ISDB_S])? m_apTuner[Device::ISDB_S]->SetLnbPower(bEnable) : false;
}

const bool CPtTriTuner::SetProperty(const IConfigProperty *pProperty)
{
	bool bReturn = CPtBaseTuner::SetProperty(pProperty);

	// チューニング空間再設定
	m_TuningSpace.clear();

	m_TuningSpace.push_back(&f_aChSpace[0]);							// UHF
	if(m_Property.m_bEnaleVHF )m_TuningSpace.push_back(&f_aChSpace[1]);	// VHF
	if(m_Property.m_bEnaleCATV)m_TuningSpace.push_back(&f_aChSpace[2]);	// CATV
	if(m_Property.m_bEnaleBS  )m_TuningSpace.push_back(&f_aChSpace[3]);	// BS
	if(m_Property.m_bEnaleCS  )m_TuningSpace.push_back(&f_aChSpace[4]);	// CS

	return bReturn;
}

CPtTriTuner::CPtTriTuner(IBonObject *pOwner)
	: CPtBaseTuner(pOwner, Device::ISDB_T)
{
	// チューナインスタンス初期化
	for(DWORD dwIndex = 0UL ; dwIndex < (sizeof(m_apTuner) / sizeof(*m_apTuner)) ; dwIndex++){
		m_apTuner[dwIndex] = NULL;
		}
		
	// チューニング空間初期設定
	SetProperty(NULL);
}

CPtTriTuner::~CPtTriTuner(void)
{
	// 何もしない
}

void CPtTriTuner::OnTsStream(CPtTuner *pTuner, const BYTE *pData, const DWORD dwSize)
{
	CBlockLock AutoLock(&m_FifoLock);
	
	// カレントチューナに一致するストリームのみ出力
	if(m_pTuner == pTuner){
		CPtBaseTuner::OnTsStream(pTuner, pData, dwSize);
		}
}
