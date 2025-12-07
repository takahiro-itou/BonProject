// DllMain.cpp: DLLエントリーポイントの定義
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BonSDK.h"
#include "BonWhiteTuner.h"
#include "BonBlackTuner.h"
#include "ModCatalog.h"


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

			// モジュールハンドル登録
			CBonWhiteTuner::SetModuleHandle(hModule);
			CBonBlackTuner::SetModuleHandle(hModule);

			// クラス登録
			g_hModule = hModule;
			::RegisterClass();

			break;

		case DLL_PROCESS_DETACH :
			
			break;
		}

	return TRUE;
}


static void RegisterClass(void)
{
	// クラス登録
	::BON_REGISTER_CLASS<FriioDriver>();
	::BON_REGISTER_CLASS<CBonWhiteTuner>();
	::BON_REGISTER_CLASS<CBonBlackTuner>();
}
