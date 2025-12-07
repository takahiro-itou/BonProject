// BonEnumerator.h: Friioドライバ列挙クラス
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Vector>
#include <String>


/////////////////////////////////////////////////////////////////////////////
// Friioドライバ列挙クラス
/////////////////////////////////////////////////////////////////////////////

class CBonEnumerator
{
public:
	CBonEnumerator(void);
	virtual ~CBonEnumerator(void);

	const bool EnumDevice(const bool bBlackFriio = false);

	LPCTSTR GetAvailableDriverPath(void) const;
	
	const DWORD GetTotalNum(void) const;
	const DWORD GetActiveNum(void) const;
	const DWORD GetAvailableNum(void) const;

	const bool LockDevice(LPCTSTR lpszDriverPath);
	const bool ReleaseDevice(void);
	static const bool IsAvailableDevice(LPCTSTR lpszDriverPath);

protected:
	std::vector<std::wstring> m_DriverList;
	CSmartMutex m_DeviceLock;
};
