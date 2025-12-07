// PtTerTuner.cpp: PT地上波チューナクラス
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BonSDK.h"
#include "PtTerTuner.h"
#include <TChar.h>


/////////////////////////////////////////////////////////////////////////////
// ファイルローカル定数設定
/////////////////////////////////////////////////////////////////////////////

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

// チャンネル空間設定
static const CPtBaseTuner::PT_CH_SPACE f_aChSpace[] =
{
	{TEXT("UHF" ), f_aConfigUHF,  sizeof(f_aConfigUHF)  / sizeof(*f_aConfigUHF) },
	{TEXT("VHF" ), f_aConfigVHF,  sizeof(f_aConfigVHF)  / sizeof(*f_aConfigVHF) },
	{TEXT("CATV"), f_aConfigCATV, sizeof(f_aConfigCATV) / sizeof(*f_aConfigCATV)}
};


/////////////////////////////////////////////////////////////////////////////
// PT地上波チューナクラス
/////////////////////////////////////////////////////////////////////////////

const bool CPtTerTuner::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	if(!m_pTuner || !EnumChannelName(dwSpace, dwChannel, NULL))return false;

	// チャンネル設定
	if(!m_pTuner->SetChannel(m_TuningSpace[dwSpace]->pChConfig[dwChannel].dwChIndex))return false;

	// チャンネル更新
	m_dwCurSpace = dwSpace;
	m_dwCurChannel = dwChannel;

	return true;
}

const bool CPtTerTuner::SetProperty(const IConfigProperty *pProperty)
{
	bool bReturn = CPtBaseTuner::SetProperty(pProperty);

	// チューニング空間再設定
	m_TuningSpace.clear();
	
	m_TuningSpace.push_back(&f_aChSpace[0]);							// UHF
	if(m_Property.m_bEnaleVHF )m_TuningSpace.push_back(&f_aChSpace[1]);	// VHF
	if(m_Property.m_bEnaleCATV)m_TuningSpace.push_back(&f_aChSpace[2]);	// CATV
	
	return bReturn;
}

CPtTerTuner::CPtTerTuner(IBonObject *pOwner)
	: CPtBaseTuner(pOwner, Device::ISDB_T)
{
	// チューニング空間初期設定
	SetProperty(NULL);
}

CPtTerTuner::~CPtTerTuner(void)
{
	// 何もしない
}
