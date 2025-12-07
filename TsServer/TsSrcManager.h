// TsSrcManager.h: TSソースマネージャクラス
//
/////////////////////////////////////////////////////////////////////////////

#pragma once


#include <vector>


/////////////////////////////////////////////////////////////////////////////
// TSソースマネージャクラス
/////////////////////////////////////////////////////////////////////////////

class CTsSrcManager	:	public CBonObject
{
public:
// CBonObject
	DECLARE_IBONOBJECT(CTsSrcManager)

// ITsSrcManager
	virtual const bool OpenManager(void);
	virtual void CloseManager(void);

	virtual IHalTsTuner * LendTuner(const DWORD dwIndex);
	virtual const bool RepayTuner(IHalTsTuner *pTsTuner);

	virtual const DWORD GetClassName(const DWORD dwIndex, LPTSTR lpszClassName);
	virtual const DWORD GetTunerName(const DWORD dwIndex, LPTSTR lpszTunerName);
	virtual const DWORD GetTunerIndex(const DWORD dwIndex);
	virtual const bool IsTunerLending(const DWORD dwIndex);

// CTsSrcManager
	CTsSrcManager(IBonObject *pOwner);
	virtual ~CTsSrcManager(void);
	
protected:
	struct TSSRC_SET
	{
		IHalTsTuner *pTsTuner;
		DWORD dwTunerIndex;
		bool bIsLend;
	};
	
	IBonClassEnumerator *m_pDriverCollection;
	std::vector<TSSRC_SET> m_TsSourceSet;
};
