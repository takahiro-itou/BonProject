// BonDlg.h : ヘッダー ファイル
//

#pragma once


// CBonDlg ダイアログ
class CBonDlg :	public CDialog,
				public CBonObject,
				protected IMediaDecoderHandler
{
// コンストラクション
public:
// CBonObject
	DECLARE_IBONOBJECT(CBonDlg)

	CBonDlg(IBonObject *pOwner);
	CBonDlg(CWnd* pParent = NULL);	// 標準コンストラクタ
	virtual ~CBonDlg(void);

// ダイアログ データ
	enum { IDD = IDD_BON_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV サポート


// 実装
protected:
	// 生成された、メッセージ割り当て関数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	virtual const DWORD OnDecoderEvent(IMediaDecoder *pDecoder, const DWORD dwEventID, PVOID pParam);
	virtual void TestThread(CSmartThread<CBonDlg> *pThread, bool &bKillSignal, PVOID pParam);

	HICON m_hIcon;

	ITsSourceTuner *m_pTsSourceTuner;
	ITsPacketSync  *m_pTsPacketSync;
	IProgAnalyzer  *m_pProgAnalyzer;
	ITsDescrambler *m_pTsDescrambler;
	IFileWriter	   *m_pFileWriter;
	IFileReader	   *m_pFileReader;
	
	CSmartThread<CBonDlg> m_TestThread;

public:
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
};
