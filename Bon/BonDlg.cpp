// BonDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "Bon.h"
#include "BonDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// ダイアログ データ
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CBonDlg ダイアログ
CBonDlg::CBonDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBonDlg::IDD, pParent)
	, CBonObject(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CBonDlg::CBonDlg(IBonObject *pOwner)
	: CDialog(CBonDlg::IDD, NULL)
	, CBonObject(pOwner)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CBonDlg::~CBonDlg(void)
{

}

void CBonDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CBonDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON1, &CBonDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CBonDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CBonDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CBonDlg::OnBnClickedButton4)
END_MESSAGE_MAP()


// CBonDlg メッセージ ハンドラ

BOOL CBonDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// "バージョン情報..." メニューをシステム メニューに追加します。

	// IDM_ABOUTBOX は、システム コマンドの範囲内になければなりません。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SetIcon(m_hIcon, TRUE);			// 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

	// TODO: 初期化をここに追加します。
	m_pTsSourceTuner	= ::BON_SAFE_CREATE<ITsSourceTuner *>(TEXT("CTsSourceTuner"), dynamic_cast<IMediaDecoderHandler *>(this));
	m_pTsPacketSync		= ::BON_SAFE_CREATE<ITsPacketSync *>(TEXT("CTsPacketSync"), dynamic_cast<IMediaDecoderHandler *>(this));
	m_pProgAnalyzer		= ::BON_SAFE_CREATE<IProgAnalyzer *>(TEXT("CProgAnalyzer"), dynamic_cast<IMediaDecoderHandler *>(this));
	m_pTsDescrambler	= ::BON_SAFE_CREATE<ITsDescrambler *>(TEXT("CTsDescrambler"), dynamic_cast<IMediaDecoderHandler *>(this));

	dynamic_cast<IMediaDecoder *>(m_pTsSourceTuner)->SetOutputDecoder(dynamic_cast<IMediaDecoder *>(m_pTsPacketSync));
	dynamic_cast<IMediaDecoder *>(m_pTsPacketSync)->SetOutputDecoder(dynamic_cast<IMediaDecoder *>(m_pProgAnalyzer));
	dynamic_cast<IMediaDecoder *>(m_pProgAnalyzer)->SetOutputDecoder(dynamic_cast<IMediaDecoder *>(m_pTsDescrambler));

	if(m_pTsSourceTuner->OpenTuner(TEXT("CPtTriTuner")))::AfxMessageBox(TEXT("チューナオープン失敗"));
	if(!m_pTsDescrambler->OpenDescrambler(TEXT("CBcasCardReader")))::AfxMessageBox(TEXT("B-CASカードオープン失敗"));
	
	m_pTsDescrambler->EnableEmmProcess(true);
	m_pTsSourceTuner->GetHalTsTuner()->SetLnbPower(true);
	
	// 地デジ
	m_pTsSourceTuner->SetChannel(0UL, 1UL);			// UHF 14ch YTV
	::Sleep(1000UL);

	// BS
//	m_pTsSourceTuner->SetChannel(3UL, 5UL);		// Star Channel
//	m_pTsSourceTuner->SetChannel(3UL, 10UL);		// BS-hi
	
	// CS
//	m_pTsSourceTuner->SetChannel(4UL, 1UL);			// ND4 110CS #2

	dynamic_cast<IMediaDecoder *>(m_pTsSourceTuner)->PlayDecoder();

	SetTimer(1UL, 1000UL, NULL);

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CBonDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void CBonDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CBonDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CBonDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: ここにメッセージ ハンドラ コードを追加します。

	KillTimer(1);
	
	dynamic_cast<IMediaDecoder *>(m_pTsSourceTuner)->StopDecoder();
	m_pTsSourceTuner->CloseTuner();
	m_pTsDescrambler->CloseDescrambler();

	BON_SAFE_RELEASE(m_pTsSourceTuner);
	BON_SAFE_RELEASE(m_pTsPacketSync);
	BON_SAFE_RELEASE(m_pProgAnalyzer);
	BON_SAFE_RELEASE(m_pTsDescrambler);
}

void CBonDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: ここにメッセージ ハンドラ コードを追加するか、既定の処理を呼び出します。

//	::BON_TRACE(TEXT("ScramblePacketNum = %lu\n"), m_pTsDescrambler->GetScramblePacketNum());



	CDialog::OnTimer(nIDEvent);
}

const DWORD CBonDlg::OnDecoderEvent(IMediaDecoder *pDecoder, const DWORD dwEventID, PVOID pParam)
{
	if(pDecoder == dynamic_cast<IMediaDecoder *>(m_pTsPacketSync)){
		switch(dwEventID){
			case ITsPacketSync::EID_SYNC_ERR :
				::BON_TRACE(TEXT("ITsPacketSync::EID_SYNC_ERR\n"));
				break;
			
			case ITsPacketSync::EID_FORMAT_ERR :
				::BON_TRACE(TEXT("ITsPacketSync::EID_FORMAT_ERR\n"));
				break;

			case ITsPacketSync::EID_TRANSPORT_ERR :
//				::BON_TRACE(TEXT("ITsPacketSync::EID_TRANSPORT_ERR\n"));
				break;

			case ITsPacketSync::EID_CONTINUITY_ERR :
				::BON_TRACE(TEXT("ITsPacketSync::EID_CONTINUITY_ERR\n"));
				break;
			}
		}
	else if(pDecoder == dynamic_cast<IMediaDecoder *>(m_pTsDescrambler)){
		switch(dwEventID){
			case ITsDescrambler::EID_CANT_DESCRAMBLE :
//				::BON_TRACE(TEXT("ITsDescrambler::EID_CANT_DESCRAMBLE\n"));
				break;
			
			case ITsDescrambler::EID_ECM_PROCESSED :
//				::BON_TRACE(TEXT("ITsDescrambler::EID_ECM_PROCESSED\n"));

				break;

			case ITsDescrambler::EID_EMM_PROCESSED :
				::BON_TRACE(TEXT("ITsDescrambler::EID_EMM_PROCESSED\n"));
				break;
			}
		}
	else if(pDecoder == dynamic_cast<IMediaDecoder *>(m_pFileWriter)){
		switch(dwEventID){
			case IFileWriter::EID_BUFF_OVERFLOW :
				//::BON_TRACE(TEXT("IFileWriter::EID_BUFF_OVERFLOW\n"));
				return true;
				break;
			}		
		}
	else if(pDecoder == dynamic_cast<IMediaDecoder *>(m_pFileReader)){
		switch(dwEventID){
			case IFileReader::EID_READ_START :
				::BON_TRACE(TEXT("IFileReader::EID_READ_START\n"));
				break;

			case IFileReader::EID_PRE_READ :
//				::BON_TRACE(TEXT("IFileReader::EID_PRE_READ\n"));
				break;

			case IFileReader::EID_POST_READ :
//				::BON_TRACE(TEXT("IFileReader::EID_POST_READ\n"));
				break;

			case IFileReader::EID_READ_END :
				::BON_TRACE(TEXT("IFileReader::EID_READ_END\n"));
				break;
			}		
		}

	return 0UL;
}

void CBonDlg::TestThread(CSmartThread<CBonDlg> *pThread, bool &bKillSignal, PVOID pParam)
{
	ISmartSocket *pSock = ::BON_SAFE_CREATE<ISmartSocket *>(TEXT("CSmartSocket"));

	BYTE abyRecvBuff[1024];

	pSock->Listen(5000);

	::BON_TRACE(TEXT("RecvFrom() Start\n"));

	while(!bKillSignal){
		
		ISmartSocket *pNewSock = pSock->Accept(500);
		
		if(pNewSock){
			pNewSock->Close();
			pNewSock->Release();
			::BON_TRACE(TEXT("Accept!!!!!!!\n"));
			}
		else if(pSock->GetLastError() == ISmartSocket::EC_TIME_OUT){
			::BON_TRACE(TEXT("Timeout\n"));
			}
		}

	::BON_TRACE(TEXT("RecvFrom() End\n"));

	pSock->Close();

	BON_SAFE_RELEASE(pSock);
}

void CBonDlg::OnBnClickedButton1()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。

	IBonClassEnumerator *pClassEnumerator = ::BON_SAFE_CREATE<IBonClassEnumerator *>(TEXT("CBonClassEnumerator"));

	if(pClassEnumerator->EnumBonClass(TEXT("IHalTsTuner"))){
		DWORD dwIndex = 0UL;
		TCHAR szText[256];
		
		while(pClassEnumerator->GetBonClassName(dwIndex++, szText)){
			::BON_TRACE(TEXT("[%lu] : %s\n"), dwIndex - 1UL, szText);
			}
		
		
		}

	BON_SAFE_RELEASE(pClassEnumerator);


/*
	static ITsSourceTuner *pTsSourceTuner = NULL;

	if(!pTsSourceTuner){
		pTsSourceTuner = ::BON_SAFE_CREATE<ITsSourceTuner *>(TEXT("CTsSourceTuner"));
		::BON_ASSERT(pTsSourceTuner->OpenTuner(TEXT("CPtTerTuner")) == ITsSourceTuner::EC_OPEN_SUCCESS);
		dynamic_cast<IMediaDecoder *>(pTsSourceTuner)->PlayDecoder();
		::BON_TRACE(TEXT("ISDB-T #1 Open\n"));
		}
	else{
		pTsSourceTuner->CloseTuner();
		BON_SAFE_RELEASE(pTsSourceTuner);
		::BON_TRACE(TEXT("ISDB-T #1 Close\n"));		
		}
*/
//	m_pTsSourceTuner->SetChannel(0UL, 0UL);

//	if(!m_TestThread.IsRunning()){
//		m_TestThread.StartThread(this, &CBonDlg::TestThread);
//		}
//	else{
//		m_TestThread.EndThread();
//		}
}

void CBonDlg::OnBnClickedButton2()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
/*
	static ITsSourceTuner *pTsSourceTuner = NULL;

	if(!pTsSourceTuner){
		pTsSourceTuner = ::BON_SAFE_CREATE<ITsSourceTuner *>(TEXT("CTsSourceTuner"));
		::BON_ASSERT(pTsSourceTuner->OpenTuner(TEXT("CPtSatTuner")) == ITsSourceTuner::EC_OPEN_SUCCESS);
		pTsSourceTuner->GetHalTsTuner()->SetLnbPower(true);
		dynamic_cast<IMediaDecoder *>(pTsSourceTuner)->PlayDecoder();
		::BON_TRACE(TEXT("ISDB-S #1 Open\n"));
		}
	else{
		pTsSourceTuner->CloseTuner();
		BON_SAFE_RELEASE(pTsSourceTuner);
		::BON_TRACE(TEXT("ISDB-S #1 Close\n"));		
		}
*/
//	m_pTsSourceTuner->SetChannel(0UL, 1UL);



	ISmartSocket *pSock = ::BON_SAFE_CREATE<ISmartSocket *>(TEXT("CSmartSocket"));

	BYTE abySendBuff[1024];

	if(pSock->Connect(TEXT("127.0.0.1"), 5000U, 1000UL)){
		pSock->Send(abySendBuff, 100, 1000);
		}

	pSock->Close();

	BON_SAFE_RELEASE(pSock);

}

void CBonDlg::OnBnClickedButton3()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。

	static ITsSourceTuner *pTsSourceTuner = NULL;

	if(!pTsSourceTuner){
		pTsSourceTuner = ::BON_SAFE_CREATE<ITsSourceTuner *>(TEXT("CTsSourceTuner"));
		::BON_ASSERT(pTsSourceTuner->OpenTuner(TEXT("CPtTerTuner")) == ITsSourceTuner::EC_OPEN_SUCCESS);
		dynamic_cast<IMediaDecoder *>(pTsSourceTuner)->PlayDecoder();
		::BON_TRACE(TEXT("ISDB-T #2 Open\n"));
		}
	else{
		pTsSourceTuner->CloseTuner();
		BON_SAFE_RELEASE(pTsSourceTuner);
		::BON_TRACE(TEXT("ISDB-T #2 Close\n"));		
		}

//	m_pTsSourceTuner->SetChannel(3UL, 0UL);
}

void CBonDlg::OnBnClickedButton4()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。

	static ITsSourceTuner *pTsSourceTuner = NULL;

	if(!pTsSourceTuner){
		pTsSourceTuner = ::BON_SAFE_CREATE<ITsSourceTuner *>(TEXT("CTsSourceTuner"));
		::BON_ASSERT(pTsSourceTuner->OpenTuner(TEXT("CPtSatTuner")) == ITsSourceTuner::EC_OPEN_SUCCESS);
		pTsSourceTuner->GetHalTsTuner()->SetLnbPower(true);
		dynamic_cast<IMediaDecoder *>(pTsSourceTuner)->PlayDecoder();
		::BON_TRACE(TEXT("ISDB-S #2 Open\n"));
		}
	else{
		pTsSourceTuner->CloseTuner();
		BON_SAFE_RELEASE(pTsSourceTuner);
		::BON_TRACE(TEXT("ISDB-S #2 Close\n"));		
		}

//	m_pTsSourceTuner->SetChannel(3UL, 1UL);
}
