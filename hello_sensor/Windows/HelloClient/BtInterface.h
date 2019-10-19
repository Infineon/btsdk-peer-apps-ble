
// BtInterface.h : header file
//

#pragma once


typedef enum {
    OSVERSION_WINDOWS_7 = 0,
    OSVERSION_WINDOWS_8,
    OSVERSION_WINDOWS_10
}tOSVersion;


class CBtInterface
{
public:
    CBtInterface(BLUETOOTH_ADDRESS *bth, HMODULE hLib, LPVOID NotificationContext, tOSVersion osversion)
    {
        m_bth = *bth;
        m_hLib = hLib;
        m_NotificationContext = NotificationContext;
        m_bWin8 = (osversion == OSVERSION_WINDOWS_8) ? TRUE : FALSE;
        m_bWin10 = (osversion == OSVERSION_WINDOWS_10) ? TRUE : FALSE;
    };

    virtual BOOL Init() = NULL;
    virtual void GetSystemInfo(UCHAR *szManuName, USHORT cbManuName, UCHAR *szModelNum, USHORT cbModelNum, UCHAR *SystemId, USHORT cbSystemId) = NULL;
    virtual BOOL GetDescriptorValue(USHORT *DescriptorValue) = NULL;
    virtual BOOL SetDescriptorValue(USHORT Value) = NULL;
    virtual BOOL GetBatteryLevel(BYTE *BatteryLevel) = NULL;
    virtual BOOL GetHelloInput(BTW_GATT_VALUE *pValue) = NULL;
    virtual BOOL GetHelloConfig(BYTE *pBlinks) = NULL;
    virtual BOOL SetHelloConfig(BYTE Blinks) = NULL;

    BLUETOOTH_ADDRESS m_bth;
    HMODULE m_hLib;
    LPVOID m_NotificationContext;
    BOOL m_bWin8;
    BOOL m_bWin10;
    tOSVersion m_osversion;
};
