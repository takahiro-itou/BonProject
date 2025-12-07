// PtTerTuner.h: PT地上波チューナクラス
//
/////////////////////////////////////////////////////////////////////////////

#pragma once


#include "PtBaseTuner.h"



/////////////////////////////////////////////////////////////////////////////
// PT地上波チューナクラス
/////////////////////////////////////////////////////////////////////////////

class CPtTerTuner :	public CPtBaseTuner
{
public:
// CBonObject
	DECLARE_IBONOBJECT(CPtTerTuner)

// CPtBaseTuner
	virtual const bool SetChannel(const DWORD dwSpace, const DWORD dwChannel);

// IConfigTarget
	virtual const bool SetProperty(const IConfigProperty *pProperty);

// CPtTerTuner
	CPtTerTuner(IBonObject *pOwner);
	virtual ~CPtTerTuner(void);

protected:
};
