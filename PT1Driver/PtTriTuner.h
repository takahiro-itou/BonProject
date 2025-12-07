// PtTriTuner.h: PT 地上/BS/110CSチューナクラス
//
/////////////////////////////////////////////////////////////////////////////

#pragma once


#include "PtBaseTuner.h"



/////////////////////////////////////////////////////////////////////////////
// PT 地上/BS/110CSチューナクラス
/////////////////////////////////////////////////////////////////////////////

class CPtTriTuner :	public CPtBaseTuner
{
public:
// CBonObject
	DECLARE_IBONOBJECT(CPtTriTuner)

// CPtBaseTuner(IHalDevice)
	virtual const DWORD GetDeviceName(LPTSTR lpszName);
	virtual const DWORD GetTotalDeviceNum(void);
	virtual const DWORD GetActiveDeviceNum(void);

// CPtBaseTuner(IHalTsTuner)
	virtual const bool OpenTuner(void);
	virtual void CloseTuner(void);

	virtual const bool SetChannel(const DWORD dwSpace, const DWORD dwChannel);
	virtual const bool SetLnbPower(const bool bEnable);

// IConfigTarget
	virtual const bool SetProperty(const IConfigProperty *pProperty);

// CPtSatTuner
	CPtTriTuner(IBonObject *pOwner);
	virtual ~CPtTriTuner(void);

protected:
	virtual void OnTsStream(CPtTuner *pTuner, const BYTE *pData, const DWORD dwSize);

	CPtTuner *m_apTuner[Device::ISDB_COUNT];
};
