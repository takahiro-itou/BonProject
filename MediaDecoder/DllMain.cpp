// DllMain.cpp: DLLエントリーポイントの定義
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BonSDK.h"

#include "ModCatalog.h"

#include "TsPacket.h"
#include "TsSection.h"
#include "TsDescBase.h"
#include "TsPidMapper.h"
#include "TsEpgCore.h"
#include "TsTable.h"

#include "TsSourceTuner.h"
#include "TsPacketSync.h"
#include "TsDescrambler.h"
#include "ProgAnalyzer.h"
#include "FileWriter.h"
#include "FileReader.h"
#include "MediaGrabber.h"


#ifdef _MANAGED
#pragma managed(push, off)
#endif


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
	::BON_REGISTER_CLASS<MediaDecoder>();

	::BON_REGISTER_CLASS<CTsPacket>();
	::BON_REGISTER_CLASS<CPsiSection>();
	::BON_REGISTER_CLASS<CDescBlock>();
	::BON_REGISTER_CLASS<CPsiSectionParser>();
	::BON_REGISTER_CLASS<CTsPidMapper>();
	::BON_REGISTER_CLASS<CEitItem>();
	::BON_REGISTER_CLASS<CEpgItem>();

	::BON_REGISTER_CLASS<CPatTable>();
	::BON_REGISTER_CLASS<CCatTable>();
	::BON_REGISTER_CLASS<CPmtTable>();
	::BON_REGISTER_CLASS<CNitTable>();
	::BON_REGISTER_CLASS<CSdtTable>();
	::BON_REGISTER_CLASS<CEitTable>();
	::BON_REGISTER_CLASS<CTotTable>();
	
	::BON_REGISTER_CLASS<CTsSourceTuner>();
	::BON_REGISTER_CLASS<CTsPacketSync>();
	::BON_REGISTER_CLASS<CTsDescrambler>();
	::BON_REGISTER_CLASS<CProgAnalyzer>();
	::BON_REGISTER_CLASS<CFileWriter>();
	::BON_REGISTER_CLASS<CFileReader>();
	::BON_REGISTER_CLASS<CMediaGrabber>();
}


#ifdef _MANAGED
#pragma managed(pop)
#endif
