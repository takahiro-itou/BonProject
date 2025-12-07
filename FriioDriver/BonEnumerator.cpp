// BonEnumerator.cpp: Friioドライバ列挙クラス
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BonSDK.h"
#include "BonEnumerator.h"
#include <TChar.h>
#include <SetupApi.h>
#include <CfgMgr32.h>
#include <InitGuid.h>


#pragma comment(lib, "SetupApi.lib")


/////////////////////////////////////////////////////////////////////////////
// ファイルローカル定数設定
/////////////////////////////////////////////////////////////////////////////

// ドライバインスタンスのGUID
DEFINE_GUID( GUID_FRIIO_DRIVER,	0x5a56d255L, 0xe23a, 0x4b4d, 0x8c, 0x78, 0xc2, 0x65, 0x93, 0x0b, 0x6c, 0x68 );


/////////////////////////////////////////////////////////////////////////////
// Friioドライバ列挙クラス
/////////////////////////////////////////////////////////////////////////////

CBonEnumerator::CBonEnumerator(void)
{

}

CBonEnumerator::~CBonEnumerator(void)
{
	m_DeviceLock.Close();
}

const bool CBonEnumerator::EnumDevice(const bool bBlackFriio)
{
	// デバイス情報セットのハンドル取得
	HDEVINFO hDevInfo = ::SetupDiGetClassDevs(&GUID_FRIIO_DRIVER, 0UL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
	if(hDevInfo == INVALID_HANDLE_VALUE)return false;

	// ドライバリストをクリア
	m_DriverList.clear();

	// デバイスインタフェースを列挙
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);

	DWORD dwDeviceIndex = 0UL;
	TCHAR szDriverPath[_MAX_PATH];

	while(::SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_FRIIO_DRIVER, dwDeviceIndex++, &DeviceInterfaceData)){
		// デバイスインタフェースの詳細を格納するのに必要なバッファサイズを取得
		DWORD dwBuffLen = 0UL;
		::SetupDiGetDeviceInterfaceDetail(hDevInfo, &DeviceInterfaceData, NULL, 0, &dwBuffLen, NULL);

		// バッファを確保
		dwBuffLen += sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + 1;
		SP_DEVICE_INTERFACE_DETAIL_DATA *pDeviceInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA *)new BYTE [dwBuffLen];
		if(!pDeviceInterfaceDetailData)continue;
		pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		// デバイスの詳細を取得
		SP_DEVINFO_DATA DevInfoData;
		DevInfoData.cbSize = sizeof(DevInfoData);
		if(!::SetupDiGetDeviceInterfaceDetail(hDevInfo, &DeviceInterfaceData, pDeviceInterfaceDetailData, dwBuffLen, NULL, &DevInfoData))continue;

		// 上位デバイスを取得「USB 複合デバイス」
		if(::CM_Get_Parent(&DevInfoData.DevInst, DevInfoData.DevInst, 0UL) == CR_SUCCESS){
	
			// 上位デバイスを取得「汎用 USB ハブ」
			if(::CM_Get_Parent(&DevInfoData.DevInst, DevInfoData.DevInst, 0UL) == CR_SUCCESS){
			
				// 下位デバイスを取得　白:「Multi-Slots USB SmartCard Reader」、黒:「Generic Usb Smart Card Reader」
				if(::CM_Get_Child(&DevInfoData.DevInst, DevInfoData.DevInst, 0UL) == CR_SUCCESS){
				
					// 下位デバイスを取得　白:「Child A Device」、黒:「存在しない」
					if(((::CM_Get_Child(&DevInfoData.DevInst, DevInfoData.DevInst, 0UL) == CR_SUCCESS)? false : true) == bBlackFriio){
						
						// DriverPathを追加
						::_tcscpy(szDriverPath, pDeviceInterfaceDetailData->DevicePath);
						m_DriverList.push_back(szDriverPath);
						}
					}
				}
			}		

		// バッファを開放
		delete [] ((BYTE *)pDeviceInterfaceDetailData);
		}

	// デバイス情報セットのハンドル開放
	::SetupDiDestroyDeviceInfoList(hDevInfo);

	return true;
}

LPCTSTR CBonEnumerator::GetAvailableDriverPath(void) const
{
	// 利用可能なデバイスを検索
	for(DWORD dwIndex = 0UL ; dwIndex < m_DriverList.size() ; dwIndex++){
		if(IsAvailableDevice(m_DriverList[dwIndex].c_str()))return m_DriverList[dwIndex].c_str();
		}

	return NULL;
}

const DWORD CBonEnumerator::GetTotalNum(void) const
{
	// デバイスの総数を返す
	return m_DriverList.size();
}

const DWORD CBonEnumerator::GetActiveNum(void) const
{
	DWORD dwActiveNum = 0UL;

	// 使用中のデバイスを検索
	for(DWORD dwIndex = 0UL ; dwIndex < m_DriverList.size() ; dwIndex++){
		if(!IsAvailableDevice(m_DriverList[dwIndex].c_str()))dwActiveNum++;
		}

	return dwActiveNum;
}

const DWORD CBonEnumerator::GetAvailableNum(void) const
{
	DWORD AvailableNum = 0UL;

	// 使用中でないデバイスを検索
	for(DWORD dwIndex = 0UL ; dwIndex < m_DriverList.size() ; dwIndex++){
		if(!IsAvailableDevice(m_DriverList[dwIndex].c_str()))AvailableNum++;
		}

	return AvailableNum;
}

const bool CBonEnumerator::LockDevice(LPCTSTR lpszDriverPath)
{
	if(!lpszDriverPath)return false;
	if(!::_tcslen(lpszDriverPath))return false;

	// ドライバパス名の'\'を'/'に置換する
	TCHAR szMutexName[_MAX_PATH];
	::_tcscpy(szMutexName, lpszDriverPath);
	
	for(TCHAR *pszPos = szMutexName ; *pszPos ; pszPos++){
		if(*pszPos == TEXT('\\'))*pszPos = TEXT('/');
		}

	// Mutexを取得
	return m_DeviceLock.Create(szMutexName);
}

const bool CBonEnumerator::ReleaseDevice(void)
{
	// Mutexを開放
	return m_DeviceLock.Close();
}

const bool CBonEnumerator::IsAvailableDevice(LPCTSTR lpszDriverPath)
{
	if(!lpszDriverPath)return false;
	if(!::_tcslen(lpszDriverPath))return false;

	// ドライバパス名の'\'を'/'に置換する
	TCHAR szMutexName[_MAX_PATH];
	::_tcscpy(szMutexName, lpszDriverPath);

	for(TCHAR *pszPos = szMutexName ; *pszPos ; pszPos++){
		if(*pszPos == TEXT('\\'))*pszPos = TEXT('/');
		}

	// Mutexの取得有無を返す
	return CSmartMutex::IsExist(szMutexName)? false : true;
}
