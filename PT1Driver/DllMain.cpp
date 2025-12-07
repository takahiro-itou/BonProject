// DllMain.cpp: DLLエントリーポイントの定義
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BonSDK.h"
#include "PtTerTuner.h"
#include "PtSatTuner.h"
#include "PtTriTuner.h"
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
	::BON_REGISTER_CLASS<PT1Driver>();
	::BON_REGISTER_CLASS<CPtTerTuner>();
	::BON_REGISTER_CLASS<CPtSatTuner>();
	::BON_REGISTER_CLASS<CPtTriTuner>();

	::BON_REGISTER_CLASS<CPtBaseTunerProperty>();
}
