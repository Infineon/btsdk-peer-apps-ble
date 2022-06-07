/*
 * Copyright 2016-2022, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */

// HelloClientDlg.cpp : implementation file
//

#include "stdafx.h"
#include <setupapi.h>
#include "HelloClient.h"
#include "HelloClientDlg.h"
#include "afxdialogex.h"
#include "DeviceSelectAdv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CHelloClientDlg dialog



CHelloClientDlg::CHelloClientDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CHelloClientDlg::IDD, pParent)
    , m_edBlinks(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_btInterface = NULL;
}

CHelloClientDlg::~CHelloClientDlg()
{
	delete m_btInterface;
}

void CHelloClientDlg::SetParam(BLUETOOTH_ADDRESS *bth, HMODULE hLib)
{
    if (m_bWin10)
        m_btInterface = new CBtWin10Interface(bth, this);
    else if (m_bWin8)
        m_btInterface = new CBtWin8Interface(bth, hLib, this);
    else
        m_btInterface = new CBtWin7Interface(bth, hLib, this);
}

void CHelloClientDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_WRITE, m_edBlinks);
	DDV_MinMaxInt(pDX, m_edBlinks, 0, 5);
}

BEGIN_MESSAGE_MAP(CHelloClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_MESSAGE(WM_CONNECTED, OnConnected)
    ON_MESSAGE(WM_NOTIFIED, OnNotified)
    ON_BN_CLICKED(IDC_READ_CHAR_1, &CHelloClientDlg::OnBnClickedReadChar1)
    ON_BN_CLICKED(IDC_READ_CHAR_2, &CHelloClientDlg::OnBnClickedReadChar2)
    ON_BN_CLICKED(IDC_WRITE_CHAR_2, &CHelloClientDlg::OnBnClickedWriteChar2)
    ON_CBN_SELCHANGE(IDC_ALLOW_TO_SEND_CHAR_1, &CHelloClientDlg::OnCbnSelchangeAllowToSendChar1)
END_MESSAGE_MAP()


// CHelloClientDlg message handlers

BOOL CHelloClientDlg::OnInitDialog()
{
    BOOL bConnected = TRUE;  // assume that device is connected which should generally be the case for hello sensor

	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    USHORT ClientConfDescrNotify = 0;

    CString s;
    WCHAR buf[200];
    GetDlgItemText(IDC_DESCRIPTION_CHAR_1, buf, 200);
    UuidToString (&buf[wcslen(buf)], 100, (GUID *)&UUID_HELLO_CHARACTERISTIC_NOTIFY);
    SetDlgItemText(IDC_DESCRIPTION_CHAR_1, buf);

    GetDlgItemText(IDC_DESCRIPTION_CHAR_2, buf, 200);
    UuidToString (&buf[wcslen(buf)], 100, (GUID *)&UUID_HELLO_CHARACTERISTIC_CONFIG);
    SetDlgItemText(IDC_DESCRIPTION_CHAR_2, buf);

    m_btInterface->Init();

    // on Win7 we will receive notification when device is connected and will intialize dialog there
    if (!m_bWin8 && !m_bWin10)
        return TRUE;

    UCHAR szManuName[9] = {0};
    UCHAR szModelNum[9] = {0};
    UCHAR SystemId[8] = {0};
    char bufs[80];
    m_btInterface->GetSystemInfo(szManuName, 8, szModelNum, 8, SystemId, 8);
    SetDlgItemTextA(m_hWnd, IDC_MANU_NAME, (LPCSTR)szManuName);
    SetDlgItemTextA(m_hWnd, IDC_MODEL_NUM, (LPCSTR)szModelNum);
    sprintf_s (bufs, "%02x %02x %02x %02x %02x %02x %02x %02x", SystemId[0], SystemId[1], SystemId[2], SystemId[3], SystemId[4], SystemId[5], SystemId[6], SystemId[7]);
    SetDlgItemTextA(m_hWnd, IDC_SYSTEM_ID, bufs);



    if (m_bWin10)
    {
        CBtWin10Interface *pWin10BtInterface = dynamic_cast<CBtWin10Interface *>(m_btInterface);

        // Assume that we are connected.  Failed attempt to read battery will change that to FALSE.
        pWin10BtInterface->m_bConnected = TRUE;

        if (pWin10BtInterface->m_bConnected)
        {
            BTW_GATT_VALUE gatt_value;
            gatt_value.len = 2;
            gatt_value.value[0] = 3;
            gatt_value.value[1] = 0;



            BYTE BatteryLevel = 0;
            m_btInterface->GetBatteryLevel(&BatteryLevel);
            SetDlgItemInt(IDC_BATTERY_LEVEL, BatteryLevel, FALSE);

            if (pWin10BtInterface->m_bConnected)
            {
                SetDlgItemText(IDC_DEVICE_STATE, L"Connected");

                USHORT ClientConfDescrNotify;
                m_btInterface->GetDescriptorValue(&ClientConfDescrNotify);
                ((CComboBox *)GetDlgItem(IDC_ALLOW_TO_SEND_CHAR_1))->SetCurSel(ClientConfDescrNotify);

                OnBnClickedReadChar2();
            }

            pWin10BtInterface->SetDescriptorValue(&guidSvcHello, &guidCharHelloNotify, BTW_GATT_UUID_DESCRIPTOR_CLIENT_CONFIG, &gatt_value);
            pWin10BtInterface->RegisterNotification(&guidSvcHello, &guidCharHelloNotify);

            this->PostMessage(WM_CONNECTED, (WPARAM)1, NULL);
        }
    }
    else if (m_bWin8)
    {
        CBtWin8Interface *pWin8BtInterface = dynamic_cast<CBtWin8Interface *>(m_btInterface);

        // Assume that we are connected.  Failed attempt to read battery will change that to FALSE.
        pWin8BtInterface->m_bConnected = TRUE;

        BYTE BatteryLevel = 0;
        m_btInterface->GetBatteryLevel(&BatteryLevel);
        SetDlgItemInt(IDC_BATTERY_LEVEL, BatteryLevel, FALSE);

        if (pWin8BtInterface->m_bConnected)
        {
            SetDlgItemText(IDC_DEVICE_STATE, L"Connected");

            USHORT ClientConfDescrNotify;
            m_btInterface->GetDescriptorValue(&ClientConfDescrNotify);
            ((CComboBox *)GetDlgItem(IDC_ALLOW_TO_SEND_CHAR_1))->SetCurSel(ClientConfDescrNotify);

            OnBnClickedReadChar2();
        }

        pWin8BtInterface->RegisterNotification();

    }


    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CHelloClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CHelloClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CHelloClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CHelloClientDlg::PostNcDestroy()
{
    CDialogEx::PostNcDestroy();
}


LRESULT CHelloClientDlg::OnConnected(WPARAM bConnected, LPARAM lparam)
{
    SetDlgItemText(IDC_DEVICE_STATE, bConnected ? L"Connected" : L"Disconnected");

    if (!bConnected)
        return S_OK;

    BYTE mBatteryLevel = 0;

    if (!m_bWin8 && !m_bWin10)
    {
        UCHAR szManuName[9] = {0};
        UCHAR szModelNum[9] = {0};
        UCHAR SystemId[8] = {0};
        char buf[80];
        m_btInterface->GetSystemInfo(szManuName, 8, szModelNum, 8, SystemId, 8);
        SetDlgItemTextA(m_hWnd, IDC_MANU_NAME, (LPCSTR)szManuName);
        SetDlgItemTextA(m_hWnd, IDC_MODEL_NUM, (LPCSTR)szModelNum);
        sprintf_s (buf, "%02x %02x %02x %02x %02x %02x %02x %02x", SystemId[0], SystemId[1], SystemId[2], SystemId[3], SystemId[4], SystemId[5], SystemId[6], SystemId[7]);
        SetDlgItemTextA(m_hWnd, IDC_SYSTEM_ID, buf);

        m_btInterface->GetBatteryLevel(&mBatteryLevel);

        USHORT ClientConfDescrNotify = 0;
        m_btInterface->GetDescriptorValue(&ClientConfDescrNotify);

        ((CComboBox *)GetDlgItem(IDC_ALLOW_TO_SEND_CHAR_1))->SetCurSel(ClientConfDescrNotify);

        OnBnClickedReadChar2();
    }
    SetDlgItemInt(IDC_BATTERY_LEVEL, mBatteryLevel, FALSE);
    return S_OK;
}

LRESULT CHelloClientDlg::OnNotified(WPARAM bConnected, LPARAM lparam)
{
    BTW_GATT_VALUE *pValue = (BTW_GATT_VALUE *)lparam;
    char buffer[30];
    memcpy (buffer, pValue->value, pValue->len);
    buffer[pValue->len] = 0;
    SetDlgItemTextA(m_hWnd, IDC_VALUE_CHAR_1, buffer);
    free (pValue);

    return S_OK;
}

void CHelloClientDlg::OnBnClickedReadChar1()
{
    BTW_GATT_VALUE value = {0};
    if (m_btInterface->GetHelloInput(&value))
    {
        value.value[value.len] = 0;
        SetDlgItemTextA(m_hWnd, IDC_VALUE_CHAR_1, (LPCSTR)value.value);
    }
}

void CHelloClientDlg::OnBnClickedReadChar2()
{
    BYTE Blinks = 0;
    m_btInterface->GetHelloConfig(&Blinks);
    m_edBlinks = Blinks;
    UpdateData(FALSE);
}


void CHelloClientDlg::OnBnClickedWriteChar2()
{
    if (!UpdateData(TRUE))
        return;

    m_btInterface->SetHelloConfig((BYTE)m_edBlinks);
}


void CHelloClientDlg::OnCbnSelchangeAllowToSendChar1()
{
    int Sel = ((CComboBox *)GetDlgItem(IDC_ALLOW_TO_SEND_CHAR_1))->GetCurSel();
    m_btInterface->SetDescriptorValue(Sel);
}
