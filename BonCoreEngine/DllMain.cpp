// DllMain.cpp: DLLエントリーポイントの定義
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BonSDK.h"
#include "BonCoreEngine.h"
#include "BonClassEnumerator.h"

#include "MediaBase.h"
#include "SmartFile.h"
#include "SmartSocket.h"
#include "RegCfgStorage.h"
#include "IniCfgStorage.h"


/////////////////////////////////////////////////////////////////////////////
// ファイルローカル変数
/////////////////////////////////////////////////////////////////////////////

static CBonCoreEngine BonEngine(NULL);


/////////////////////////////////////////////////////////////////////////////
// ファイルローカル関数プロトタイプ
/////////////////////////////////////////////////////////////////////////////

static void RegisterClass(void);


/////////////////////////////////////////////////////////////////////////////
// DLLエントリー
/////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call){
		case DLL_PROCESS_ATTACH :

			// メモリリーク検出有効
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF); 

			// コアエンジンスタートアップ
			BonEngine.Startup(hModule);

			// 標準クラス登録
			::RegisterClass();

			// Bonモジュールロード
			BonEngine.RegisterBonModule();

			break;

		case DLL_PROCESS_DETACH :

			// コアエンジンシャットダウン
			BonEngine.Shutdown();

			break;
		}

	return TRUE;
}

static void RegisterClass(void)
{
	// クラスファクトリー登録(標準組み込みクラス)
	::BON_REGISTER_CLASS<CBonCoreEngine>();
	::BON_REGISTER_CLASS<CBonClassEnumerator>();

	::BON_REGISTER_CLASS<CMediaData>();
	::BON_REGISTER_CLASS<CSmartFile>();
	::BON_REGISTER_CLASS<CSmartSocket>();
	::BON_REGISTER_CLASS<CRegCfgStorage>();
	::BON_REGISTER_CLASS<CIniCfgStorage>();
}


/////////////////////////////////////////////////////////////////////////////
// エクスポートAPI
/////////////////////////////////////////////////////////////////////////////

BONAPI const BONGUID BON_NAME_TO_GUID(LPCTSTR lpszName)
{
	// モジュール名/クラス名/インタフェース名をGUIDに変換
	return BonEngine.BonNameToGuid(lpszName);
}

BONAPI IBonObject * BON_CREATE_INSTANCE(LPCTSTR lpszClassName, IBonObject *pOwner)
{
	// クラスインスタンス生成
	return BonEngine.BonCreateInstance(lpszClassName, pOwner);
}

BONAPI const bool QUERY_BON_MODULE(LPCTSTR lpszModuleName)
{
	// モジュールの有無を返す
	return BonEngine.QueryBonModule(lpszModuleName);
}

BONAPI const DWORD GET_BON_MODULE_NAME(const BONGUID ModuleId, LPTSTR lpszModuleName)
{
	// モジュールのGUIDを名前に変換
	return BonEngine.GetBonModuleName(ModuleId, lpszModuleName);
}

BONAPI const bool REGISTER_BON_CLASS(LPCTSTR lpszClassName, const CLASSFACTORYMETHOD pfnClassFactory, const DWORD dwPriority)
{
	// クラスを登録
	return BonEngine.RegisterBonClass(lpszClassName, pfnClassFactory, dwPriority);
}

BONAPI const bool QUERY_BON_CLASS(LPCTSTR lpszClassName)
{
	// クラスの有無を返す
	return BonEngine.QueryBonClass(lpszClassName);
}

BONAPI const DWORD GET_BON_CLASS_NAME(const BONGUID ClassId, LPTSTR lpszClassName)
{
	// クラスのGUIDを名前に変換
	return BonEngine.GetBonClassName(ClassId, lpszClassName);
}

BONAPI const BONGUID GET_BON_CLASS_MODULE(const BONGUID ClassId)
{
	// クラスを提供するモジュールのGUIDを返す
	return BonEngine.GetBonClassModule(ClassId);
}

BONAPI IBonObject * GET_STOCK_INSTANCE(LPCTSTR lpszClassName)
{
	// ストックインスタンスを返す
	return BonEngine.GetStockInstance(lpszClassName);
}

BONAPI const bool REGISTER_STOCK_INSTANCE(LPCTSTR lpszClassName, IBonObject *pInstance)
{
	// ストックインスタンス登録
	return BonEngine.RegisterStockInstance(lpszClassName, pInstance);
}

BONAPI const bool UNREGISTER_STOCK_INSTANCE(LPCTSTR lpszClassName)
{
	// ストックインスタンス削除
	return BonEngine.UnregisterStockInstance(lpszClassName);
}
