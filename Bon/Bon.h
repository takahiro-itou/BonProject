// Bon.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです。
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h"		// メイン シンボル


// CBonApp:
// このクラスの実装については、Bon.cpp を参照してください。
//

class CBonApp : public CWinApp
{
public:
	CBonApp();
	~CBonApp();

// オーバーライド
	public:
	virtual BOOL InitInstance();

// 実装

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
	
protected:
};

extern CBonApp theApp;