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

// HelloClient.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CHelloClientApp:
// See HelloClient.cpp for the implementation of this class
//

class CHelloClientApp : public CWinApp
{
public:
	CHelloClientApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

#define BTW_GATT_UUID_SERVCLASS_BATTERY                     0x180F    /* Battery Service  */
#define BTW_GATT_UUID_SERVCLASS_DEVICE_INFO                 0x180A    /* Device Information Service  */

#define BTW_GATT_UUID_CHAR_DIS_SYSTEM_ID                    0x2A23
#define BTW_GATT_UUID_CHAR_DIS_MODEL_NUMBER                 0x2A24
#define BTW_GATT_UUID_CHAR_DIS_SERIAL_NUMBER                0x2A25
#define BTW_GATT_UUID_CHAR_DIS_FIRMWARE_REVISION            0x2A26
#define BTW_GATT_UUID_CHAR_DIS_HARDWARE_REVISION            0x2A27
#define BTW_GATT_UUID_CHAR_DIS_SOFTWARE_REVISION            0x2A28
#define BTW_GATT_UUID_CHAR_DIS_MANUFACTURER_NAME            0x2A29

#define BTW_GATT_UUID_CHAR_BATTERY_LEVEL                    0x2A19
#define BTW_GATT_UUID_CHAR_BATTERY_STATE                    0x2A1A
#define BTW_GATT_UUID_CHAR_BATTERY_STATELEVEL               0x2A1B

#define BTW_GATT_UUID_DESCRIPTOR_CLIENT_CONFIG              0x2902      /*  Client Characteristic Configuration */

const GUID guidSvcBattery                       = {BTW_GATT_UUID_SERVCLASS_BATTERY,          0, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};
const GUID guidCharBatLevel                     = {BTW_GATT_UUID_CHAR_BATTERY_LEVEL,         0, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};
const GUID guidClntConfigDesc                   = {BTW_GATT_UUID_CHAR_CLIENT_CONFIG,         0, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};
const GUID guidSvcDeviceInfo                    = {BTW_GATT_UUID_SERVCLASS_DEVICE_INFO,      0, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};
const GUID guidCharModelNumber                  = {BTW_GATT_UUID_CHAR_DIS_MODEL_NUMBER,      0, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};
const GUID guidCharManufacturer                 = {BTW_GATT_UUID_CHAR_DIS_MANUFACTURER_NAME, 0, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};
const GUID guidCharSystemId                     = {BTW_GATT_UUID_CHAR_DIS_SYSTEM_ID,         0, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

extern GUID guidSvcHello;
extern GUID guidCharHelloConfig;
extern GUID guidCharHelloNotify;

// {1B7E8251-2877-41C3-B46E-CF057C562023}
static const GUID UUID_HELLO_SERVICE               = { 0x1b7e8251, 0x2877, 0x41c3, { 0xb4, 0x6e, 0xcf, 0x5, 0x7c, 0x56, 0x20, 0x23 } };

// {5E9BF2A8-F93F-4481-A67E-3B2F4A07891A}
static const GUID UUID_HELLO_CHARACTERISTIC_CONFIG = { 0x5e9bf2a8, 0xf93f, 0x4481, { 0xa6, 0x7e, 0x3b, 0x2f, 0x4a, 0x7, 0x89, 0x1a } };

// {8AC32D3F-5CB9-4D44-BEC2-EE689169F626}
static const GUID UUID_HELLO_CHARACTERISTIC_NOTIFY = { 0x8ac32d3f, 0x5cb9, 0x4d44, { 0xbe, 0xc2, 0xee, 0x68, 0x91, 0x69, 0xf6, 0x26 } };

extern void ods(const char * fmt_str, ...);
extern void BdaToString (PWCHAR buffer, BLUETOOTH_ADDRESS *btha);
extern void UuidToString(LPWSTR buffer, size_t buffer_size, GUID *uuid);

extern CHelloClientApp theApp;
