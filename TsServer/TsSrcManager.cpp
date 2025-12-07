// TsSrcManager.cpp: TSソースマネージャクラス
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BonSDK.h"
#include "TsSrcManager.h"


/////////////////////////////////////////////////////////////////////////////
// TSソースマネージャクラス
/////////////////////////////////////////////////////////////////////////////

const bool CTsSrcManager::OpenManager(void)
{
	// 一旦クローズ
	CloseManager();
	
	// クラス列挙インスタンス生成
	m_pDriverCollection = ::BON_SAFE_CREATE<IBonClassEnumerator *>(TEXT("CBonClassEnumerator"));
	::BON_ASSERT(m_pDriverCollection != NULL);

	// IHalTsTunerクラス列挙
	m_pDriverCollection->EnumBonClass(TEXT("IHalTsTuner"));
	
	
	return true;
}

void CTsSrcManager::CloseManager(void)
{
	// クラス列挙インスタンス開放
	BON_SAFE_RELEASE(m_pDriverCollection);


}

IHalTsTuner * CTsSrcManager::LendTuner(const DWORD dwIndex)
{
	return NULL;
}

const bool CTsSrcManager::RepayTuner(IHalTsTuner *pTsTuner)
{
	return true;
}

const DWORD CTsSrcManager::GetClassName(const DWORD dwIndex, LPTSTR lpszClassName)
{
	return 0UL;
}

const DWORD CTsSrcManager::GetTunerName(const DWORD dwIndex, LPTSTR lpszTunerName)
{
	return 0UL;
}

const DWORD CTsSrcManager::GetTunerIndex(const DWORD dwIndex)
{
	return 0UL;
}

const bool CTsSrcManager::IsTunerLending(const DWORD dwIndex)
{
	return true;
}

CTsSrcManager::CTsSrcManager(IBonObject *pOwner)
	: CBonObject(pOwner)
	, m_pDriverCollection(NULL)
{


}

CTsSrcManager::~CTsSrcManager(void)
{
	// マネージャクローズ
	CloseManager();
}
