#if !defined(AFX_HISTSIZEDLG_H__0B0D0B22_B09B_11D3_AAB7_F4C9F573BE2D__INCLUDED_)
#define AFX_HISTSIZEDLG_H__0B0D0B22_B09B_11D3_AAB7_F4C9F573BE2D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HistsizeDlg.h : Header-Datei
//

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CHistsizeDlg 

class CHistsizeDlg : public CDialog
{
// Konstruktion
public:
	CHistsizeDlg(CWnd* pParent = NULL);   // Standardkonstruktor

// Dialogfelddaten
	//{{AFX_DATA(CHistsizeDlg)
	enum { IDD = IDD_HISTSIZE };
	int		m_nHistSize;
	//}}AFX_DATA


// �berschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktions�berschreibungen
	//{{AFX_VIRTUAL(CHistsizeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterst�tzung
	//}}AFX_VIRTUAL

// Implementierung
protected:

	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(CHistsizeDlg)
		// HINWEIS: Der Klassen-Assistent f�gt hier Member-Funktionen ein
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ f�gt unmittelbar vor der vorhergehenden Zeile zus�tzliche Deklarationen ein.

#endif // AFX_HISTSIZEDLG_H__0B0D0B22_B09B_11D3_AAB7_F4C9F573BE2D__INCLUDED_
