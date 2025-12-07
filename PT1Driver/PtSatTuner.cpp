// PtSatTuner.cpp: PT BS/110CSチューナクラス
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BonSDK.h"
#include "PtSatTuner.h"
#include <TChar.h>


/////////////////////////////////////////////////////////////////////////////
// ファイルローカル定数設定
/////////////////////////////////////////////////////////////////////////////

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
	{TEXT("BS"   ),	f_aConfigBS, sizeof(f_aConfigBS) / sizeof(*f_aConfigBS)},
	{TEXT("110CS"),	f_aConfigCS, sizeof(f_aConfigCS) / sizeof(*f_aConfigCS)}
};


/////////////////////////////////////////////////////////////////////////////
// PT BS/110CSチューナクラス
/////////////////////////////////////////////////////////////////////////////

const bool CPtSatTuner::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	if(!m_pTuner || !EnumChannelName(dwSpace, dwChannel, NULL))return false;

	// チャンネル設定
	if(!m_pTuner->SetChannel(m_TuningSpace[dwSpace]->pChConfig[dwChannel].dwChIndex))return false;

	// TS ID設定
	if(!m_pTuner->SetTsID(m_TuningSpace[dwSpace]->pChConfig[dwChannel].wTsID))return false;

	// チャンネル更新
	m_dwCurSpace = dwSpace;
	m_dwCurChannel = dwChannel;

	return true;
}

const bool CPtSatTuner::SetProperty(const IConfigProperty *pProperty)
{
	bool bReturn = CPtBaseTuner::SetProperty(pProperty);

	// チューニング空間再設定
	m_TuningSpace.clear();
	
	if(m_Property.m_bEnaleBS)m_TuningSpace.push_back(&f_aChSpace[0]);	// BS
	if(m_Property.m_bEnaleCS)m_TuningSpace.push_back(&f_aChSpace[1]);	// CS

	return bReturn;
}

CPtSatTuner::CPtSatTuner(IBonObject *pOwner)
	: CPtBaseTuner(pOwner, Device::ISDB_S)
{
	// チューニング空間初期設定
	SetProperty(NULL);
}

CPtSatTuner::~CPtSatTuner(void)
{
	// 何もしない
}
