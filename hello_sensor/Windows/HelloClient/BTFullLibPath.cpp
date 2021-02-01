/*
 * Copyright 2016-2021, Cypress Semiconductor Corporation (an Infineon company) or
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

#include "stdafx.h"
#include "BTFullLibPath.h"
#include "windows.h"

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define BT_INSTALL_PATH_REG_HIVE                (HKEY_LOCAL_MACHINE)
#define BT_INSTALL_PATH_REG_KEY                 _T("Software\\WIDCOMM\\Install")
#define BT_INSTALL_PATH_REG_VALUE               _T("INSTALLDIR")
#define BT_INSTALL_PATH_SYSWOW64                _T("SysWow64\\")
#define BTREZ_DLL_FILENAME                      _T("btrez.dll")


static bool s_bCheckWow64 = false;
static bool s_bIsWow64Proc = false;
typedef bool (WINAPI *LPFN_ISWOW64PROC) (HANDLE, bool*);
static LPFN_ISWOW64PROC fnIsWow64Proc = NULL;

static bool IsWow64Proc()
{
    if(!s_bCheckWow64)
    {
        s_bCheckWow64 = true;
        if (NULL == fnIsWow64Proc)
        {
            fnIsWow64Proc = (LPFN_ISWOW64PROC)GetProcAddress(GetModuleHandle(_T("kernel32")),"IsWow64Process");
        }
        if (NULL != fnIsWow64Proc)
        {
            fnIsWow64Proc(GetCurrentProcess(), &s_bIsWow64Proc);
        }
    }
    return s_bIsWow64Proc;
}

long CBTFullLibPath::m_nRefCount = 0;
bool CBTFullLibPath::m_fLoadedFullPath = false;
TCHAR CBTFullLibPath::m_szInstallDir[MAX_PATH+1] = { '\0' };
TCHAR CBTFullLibPath::m_szInstDirSysWow64[MAX_PATH+1] = { '\0' };
TCHAR CBTFullLibPath::m_szBtRezPath[MAX_PATH+1] = { '\0' };
bool CBTFullLibPath::m_bIsWow64 = false;
CRITICAL_SECTION cs_libpath;

CBTFullLibPath::CBTFullLibPath()
{
	if (1 == InterlockedIncrement(&m_nRefCount))
    {
		InitializeCriticalSection(&cs_libpath);
    }
    EnterCriticalSection(&cs_libpath);
    if (!m_fLoadedFullPath)
    {
        // Retrieve BTW Install Directory from Registry and generate full BtRez.dll path
        DWORD dwResult = ERROR_SUCCESS;
        DWORD dwFlag;
        HKEY   hKey;
        dwFlag = KEY_READ;

        m_bIsWow64 = IsWow64Proc();
        if (m_bIsWow64)
        {
            dwFlag |= KEY_WOW64_64KEY;
        }

        if (ERROR_SUCCESS == (dwResult = RegOpenKeyEx(BT_INSTALL_PATH_REG_HIVE, BT_INSTALL_PATH_REG_KEY, 0, dwFlag, &hKey)))
        {
            // Retrieve BTW Install Directory Path
            DWORD dwBytes = sizeof(m_szInstallDir);
            dwResult = RegQueryValueEx (hKey, BT_INSTALL_PATH_REG_VALUE, NULL, NULL, (LPBYTE)m_szInstallDir, &dwBytes);
            RegCloseKey(hKey);

            // Include trailing '\' if not already included
            size_t path_len = _tcslen(m_szInstallDir);
            if ((path_len > 0) && (path_len < MAX_PATH))
            {
                TCHAR last_char = m_szInstallDir[path_len-1];
                if ((last_char != '\\') && (last_char != '/'))
                {
                    _tcscat_s(m_szInstallDir, MAX_PATH, _T("\\"));
                }
            }

            if (ERROR_SUCCESS == dwResult)
            {
                // Also set SysWow64 path from install directory
                if (-1 == _stprintf_s(m_szInstDirSysWow64, MAX_PATH, _T("%s%s"), (LPCTSTR)m_szInstallDir, (LPCTSTR)BT_INSTALL_PATH_SYSWOW64))
                {
                    dwResult = ERROR_BAD_PATHNAME;
                }
            }

            if (ERROR_SUCCESS == dwResult)
            {
                // Generate full BtRez.dll path and take WOW64 into account
                TCHAR* target_folder = (IsWow64Proc() ? m_szInstDirSysWow64 : m_szInstallDir);
                if (-1 == _stprintf_s(m_szBtRezPath, MAX_PATH, _T("%s%s"), (LPCTSTR)target_folder, (LPCTSTR)BTREZ_DLL_FILENAME))
                {
                    dwResult = ERROR_BAD_PATHNAME;
                }
            }
        }

        bool bPathsLoaded = (ERROR_SUCCESS == dwResult);
        if (!bPathsLoaded)
        {
            // One or more steps failed; Fall back to BtRez name only
            _stprintf_s(m_szBtRezPath, MAX_PATH, _T("%s"), BTREZ_DLL_FILENAME);
        }
        m_fLoadedFullPath = bPathsLoaded;
    }
	LeaveCriticalSection(&cs_libpath);
} // CBTFullLibPath::CBTFullLibPath

bool    CBTFullLibPath::IsFullPath()
{
    EnterCriticalSection(&cs_libpath);
    bool bResult = m_fLoadedFullPath;
    LeaveCriticalSection(&cs_libpath);
    return bResult;
}

bool    CBTFullLibPath::GetFullInstallPathOf(TCHAR* szFileNameOnly, TCHAR* pOutBuf, size_t OutBufCapacity)
{
    // Note: OutBufCapacity size is in TCHARs
    bool bResult = false;
    if ((NULL != szFileNameOnly) && (NULL != pOutBuf) && (OutBufCapacity > 0))
    {
        EnterCriticalSection(&cs_libpath);
        if (m_fLoadedFullPath)
        {
            _stprintf_s(pOutBuf, OutBufCapacity, _T("%s%s"), (LPCTSTR)m_szInstallDir, (LPCTSTR)szFileNameOnly);
            bResult = true;
        }
        else
        {
            _stprintf_s(pOutBuf, OutBufCapacity, _T("%s"), (LPCTSTR)szFileNameOnly);
        }
        LeaveCriticalSection(&cs_libpath);
    }
    return bResult;
}

bool    CBTFullLibPath::GetConditionalDllPathOf(TCHAR* szFileNameOnly, TCHAR* pOutBuf, size_t OutBufCapacity)
{
    // Note: OutBufCapacity size is in TCHARs
    bool bResult = false;
    if ((NULL != szFileNameOnly) && (NULL != pOutBuf) && (OutBufCapacity > 0))
    {
        EnterCriticalSection(&cs_libpath);
        if (m_fLoadedFullPath)
        {
            // Use SysWow64 path if detected as WOW64
            TCHAR* target_folder = (IsWow64Proc() ? m_szInstDirSysWow64 : m_szInstallDir);
            _stprintf_s(pOutBuf, OutBufCapacity, _T("%s%s"), (LPCTSTR)target_folder, (LPCTSTR)szFileNameOnly);
            bResult = true;
        }
        else
        {
            _stprintf_s(pOutBuf, OutBufCapacity, _T("%s"), (LPCTSTR)szFileNameOnly);
        }
        LeaveCriticalSection(&cs_libpath);
    }
    return bResult;
}

CBTFullLibPath::~CBTFullLibPath()
{
	if(0 == InterlockedDecrement(&m_nRefCount))
    {
		DeleteCriticalSection(&cs_libpath);
    }
} // CBTFullLibPath::~CBTFullLibPath
