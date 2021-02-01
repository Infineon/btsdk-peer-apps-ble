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
#include <setupapi.h>
#include "HelloClient.h"
#include "HelloClientDlg.h"
#include "afxdialogex.h"
#include <vector>


#define ASSERT_SUCCEEDED(hr) {if(FAILED(hr)) return (hr == S_OK);}
#define RETURN_IF_FAILED(p, q) { FAILED(hr) ? { cout << p;} : q;}

typedef ITypedEventHandler<GattCharacteristic *, GattValueChangedEventArgs *> ValueChangedHandler;
typedef ITypedEventHandler<BluetoothLEDevice *, IInspectable *> StatusHandler;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//converts BD_ADDR to the __int64
#define bda2int(a) (a ? ((((__int64)a[0])<<40)+(((__int64)a[1])<<32)+(((__int64)a[2])<<24)+(((__int64)a[3])<<16)+(((__int64)a[4])<<8)+((__int64)a[5])) : 0)

Microsoft::WRL::ComPtr<ABI::Windows::Devices::Bluetooth::IBluetoothLEDevice> mDevice;

EventRegistrationToken mStatusChangedToken;

struct ValueChangedEntry {
    ValueChangedEntry() {}
    ValueChangedEntry(Microsoft::WRL::ComPtr<ABI::Windows::Devices::Bluetooth::GenericAttributeProfile::IGattCharacteristic> c,
        EventRegistrationToken t)
        : characteristic(c)
        , token(t)
    {
    }

    Microsoft::WRL::ComPtr<ABI::Windows::Devices::Bluetooth::GenericAttributeProfile::IGattCharacteristic> characteristic;
    EventRegistrationToken token;
};

std::vector <ValueChangedEntry> mValueChangedTokens;

struct ServiceCharEntry {
    ServiceCharEntry() {}
    ServiceCharEntry(GUID g, ComPtr<IVectorView<GattCharacteristic *>>  c)
        : svcGuid(g)
        , charVec(c)
    {
    }

    ComPtr<IVectorView<GattCharacteristic *>> charVec;
    GUID svcGuid;
};

std::vector <ServiceCharEntry> mServiceCharsVector;

tServiceState m_state;
tError m_error;

void CBtWin10Interface::setState(tServiceState state)
{
    m_state = state;
}

void CBtWin10Interface::setError(tError error)
{
    m_error = error;
}

char* guid2str(GUID guid)
{
    WCHAR szService[80];
    static char buff[256] = { 0 };
    UuidToString(szService, 80, &guid);
    sprintf_s(buff, "%S", szService);
    return buff;
}

template <typename T>
inline HRESULT _await_impl(const Microsoft::WRL::ComPtr<T> &asyncOp, UINT timeout)
{
    Microsoft::WRL::ComPtr<IAsyncInfo> asyncInfo;
    HRESULT hr = asyncOp.As(&asyncInfo);
    if (FAILED(hr))
        return hr;

    AsyncStatus status;

    while (SUCCEEDED(hr = asyncInfo->get_Status(&status)) && status == AsyncStatus::Started)
    {
        Sleep(1000);
    }

    if (FAILED(hr) || status != AsyncStatus::Completed) {
        HRESULT ec;
        hr = asyncInfo->get_ErrorCode(&ec);
        if (FAILED(hr))
            return hr;
        hr = asyncInfo->Close();
        if (FAILED(hr))
            return hr;
        return ec;
    }

    return hr;
}

template <typename T, typename U>
inline HRESULT await(const Microsoft::WRL::ComPtr<T> &asyncOp, U *results, UINT timeout = 0)
{
    HRESULT hr = _await_impl(asyncOp, timeout);
    if (FAILED(hr))
        return hr;

    return asyncOp->GetResults(results);
}

static char* byteArrayFromBuffer(const ComPtr<IBuffer> &buffer, bool isWCharString = false)
{
    ComPtr<Windows::Storage::Streams::IBufferByteAccess> byteAccess;
    HRESULT hr = buffer.As(&byteAccess);
    char *data;
    hr = byteAccess->Buffer(reinterpret_cast<byte **>(&data));
    UINT32 size;
    hr = buffer->get_Length(&size);

    return data;
}

ComPtr<IGattDeviceService> CBtWin10Interface::getNativeService(const GUID *p_guidServ)
{
    GUID serviceUuid = *p_guidServ;
    ComPtr<IGattDeviceService> deviceService;

    HRESULT hr;
    hr = mDevice->GetGattService(serviceUuid, &deviceService);
    if (FAILED(hr))
        ods("Could not obtain native service for Uuid: %S", guid2str(serviceUuid));
    return deviceService;
}

ComPtr<IGattCharacteristic> CBtWin10Interface::getNativeCharacteristic(const GUID *p_guidServ, const GUID *p_guidChar)
{
    GUID serviceUuid = *p_guidServ;
    GUID  charUuid = *p_guidChar;

    ComPtr<IGattCharacteristic> characteristic;

    ods("%S", __FUNCTION__);

    for (const ServiceCharEntry &entry : mServiceCharsVector) {
        HRESULT hr;

        if (memcmp(&serviceUuid, &entry.svcGuid, sizeof(GUID)) == 0)
        {
            UINT characteristicsCount = 0;
            hr = entry.charVec->get_Size(&characteristicsCount);

            for (UINT j = 0; j < characteristicsCount; j++) {
                hr = entry.charVec->GetAt(j, &characteristic);
                GUID charuuid = { 0 };
                characteristic->get_Uuid(&charuuid);
                if (memcmp(&charuuid, &charUuid, sizeof(GUID)) == 0)
                    return characteristic;
            }
        }
    }

    return characteristic;
}

void CBtWin10Interface::PostNotification(int charHandle, ComPtr<IBuffer> buffer)
{
    ods("Characteristic change notification");
    ComPtr<Windows::Storage::Streams::IBufferByteAccess> byteAccess;
    HRESULT hr = buffer.As(&byteAccess);

    char *data;
    hr = byteAccess->Buffer(reinterpret_cast<byte **>(&data));

    UINT32 size;
    hr = buffer->get_Length(&size);

    ComPtr<IGattCharacteristic> characteristic;
    BOOL bFound = FALSE;

    ods("%S", __FUNCTION__);

    GUID charuuid = { 0 };
    UINT16 charattrHandle = 0;

    for (const ServiceCharEntry &entry : mServiceCharsVector)
    {
        HRESULT hr;

        UINT characteristicsCount = 0;
        hr = entry.charVec->get_Size(&characteristicsCount);

        for (UINT j = 0; j < characteristicsCount; j++) {
            hr = entry.charVec->GetAt(j, &characteristic);

            characteristic->get_Uuid(&charuuid);
            charattrHandle = 0;
            characteristic->get_AttributeHandle(&charattrHandle);

            if (charattrHandle == charHandle)
            {
                bFound = TRUE;
                break;
            }
        }

        if (bFound)
            break;
    }

    if (!bFound)
        return;

    CHelloClientDlg *pDlg = (CHelloClientDlg *)m_NotificationContext;
    DWORD dwCharInstance = 0;
    BTW_GATT_VALUE *p = (BTW_GATT_VALUE *)malloc(sizeof(BTW_GATT_VALUE));
    if (!p)
        return;

    p->len = (USHORT)size;
    memcpy(p->value, data, size);
    pDlg->PostMessage(WM_NOTIFIED, (WPARAM)BTW_GATT_OPTYPE_NOTIFICATION, (LPARAM)p);
}

BOOL CBtWin10Interface::RegisterNotification(const GUID *p_guidServ, const GUID *p_guidChar)
{
    GUID serviceUuid = *p_guidServ;
    GUID charUuid = *p_guidChar;

    ods("RegisterNotification characteristic:%S", guid2str(charUuid));
    ods(" in service %S for value changes", guid2str(serviceUuid));

    for (const ValueChangedEntry &entry : mValueChangedTokens)
    {
        GUID guuid;
        HRESULT hr;
        hr = entry.characteristic->get_Uuid(&guuid);
        ASSERT_SUCCEEDED(hr);

        if (memcmp(&guuid, &charUuid, sizeof(GUID)) == 0)
        {
            ods("Already registered");
            return FALSE;
        }
    }

    ComPtr<IGattCharacteristic> characteristic = getNativeCharacteristic(p_guidServ, p_guidChar);

    if (!characteristic)
    {
        ods("Characteristic NULL");
        return FALSE;
    }

    EventRegistrationToken token;
    HRESULT hr;
    hr = characteristic->add_ValueChanged(Callback<ValueChangedHandler>([this](IGattCharacteristic *characteristic, IGattValueChangedEventArgs *args) {
        HRESULT hr;
        UINT16 handle;
        hr = characteristic->get_AttributeHandle(&handle);
        ASSERT_SUCCEEDED(hr);
        ComPtr<IBuffer> buffer;
        hr = args->get_CharacteristicValue(&buffer);
        ASSERT_SUCCEEDED(hr);
        PostNotification(handle, buffer);

        return (bool)TRUE;

    }).Get(), &token);
    ASSERT_SUCCEEDED(hr);

    mValueChangedTokens.push_back(ValueChangedEntry(characteristic, token));

    return TRUE;
}

BOOL CBtWin10Interface::Init()
{
    ods("%s", __FUNCTION__);

    __int64 remoteDevice = m_bth.ullLong;

    if (remoteDevice == 0) {
        ods("Invalid/null remote device address");
        setError(UnknownRemoteDeviceError);
        return FALSE;
    }

    setState(ConnectingState);

    HRESULT hr;
    ComPtr<IBluetoothLEDeviceStatics> deviceStatics;
    hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Devices_Bluetooth_BluetoothLEDevice).Get(), &deviceStatics);
    ASSERT_SUCCEEDED(hr);
    ComPtr<IAsyncOperation<BluetoothLEDevice *>> deviceFromIdOperation;
    hr = deviceStatics->FromBluetoothAddressAsync(remoteDevice, &deviceFromIdOperation);
    ASSERT_SUCCEEDED(hr);
    hr = await(deviceFromIdOperation, mDevice.GetAddressOf());
    ASSERT_SUCCEEDED(hr);

    if (!mDevice) {
        ods("Could not find LE device");
        setError(InvalidBluetoothAdapterError);
        setState(UnconnectedState);
        return FALSE;
    }
    BluetoothConnectionStatus status;
    hr = mDevice->get_ConnectionStatus(&status);
    ASSERT_SUCCEEDED(hr);

    hr = mDevice->add_ConnectionStatusChanged(Callback<StatusHandler>([this](IBluetoothLEDevice *dev, IInspectable *) {
        BluetoothConnectionStatus status;
        HRESULT hr;
        hr = dev->get_ConnectionStatus(&status);
        ASSERT_SUCCEEDED(hr);
        if (m_state == ConnectingState
            && status == BluetoothConnectionStatus::BluetoothConnectionStatus_Connected) {
            setState(ConnectedState);

            // PostMessage Connected message
        }
        else if (m_state == ConnectedState
            && status == BluetoothConnectionStatus::BluetoothConnectionStatus_Disconnected) {
            setState(UnconnectedState);
            // PostMessage disconnected message
        }
        return (bool)TRUE;
    }).Get(), &mStatusChangedToken);
    ASSERT_SUCCEEDED(hr);

    ASSERT_SUCCEEDED(hr);

    if (status == BluetoothConnectionStatus::BluetoothConnectionStatus_Connected) {
        setState(ConnectedState);
        // PostMessage Connected message
        return TRUE;
    }

    ComPtr<IAsyncOperation<GattDeviceServicesResult*>> op;
    ComPtr<IBluetoothLEDevice3> mDevice3;
    hr = mDevice.As(&mDevice3);

    hr = mDevice3->GetGattServicesWithCacheModeAsync(BluetoothCacheMode_Cached, &op);
    ASSERT_SUCCEEDED(hr);

    ComPtr<IGattDeviceServicesResult> servicesresult;
    hr = await(op, servicesresult.GetAddressOf());
    ASSERT_SUCCEEDED(hr);

    ComPtr<IVectorView <GattDeviceService *>> deviceServices;
    hr = servicesresult->get_Services(&deviceServices);
    ASSERT_SUCCEEDED(hr);

    UINT serviceCount;
    hr = deviceServices->get_Size(&serviceCount);
    ASSERT_SUCCEEDED(hr);

    for (UINT i = 0; i < serviceCount; i++) {

        ComPtr<IGattDeviceService> service;
        hr = deviceServices->GetAt(i, &service);
        ASSERT_SUCCEEDED(hr);

        GUID uuid = { 0 };
        service->get_Uuid(&uuid);

        UINT16 attribHandle = 0;
        service->get_AttributeHandle(&attribHandle);

        ComPtr<IGattDeviceService3> service3;
        hr = service.As(&service3);
        ASSERT_SUCCEEDED(hr);

        ComPtr<IAsyncOperation<GattCharacteristicsResult*>> charop;
        ComPtr<IGattCharacteristicsResult> charresult;
        ComPtr<IVectorView<GattCharacteristic *>> characteristics;

        hr = service3->GetCharacteristicsWithCacheModeAsync(BluetoothCacheMode_Cached, &charop);
        ASSERT_SUCCEEDED(hr);

        hr = await(charop, charresult.GetAddressOf());
        ASSERT_SUCCEEDED(hr);

        hr = charresult->get_Characteristics(&characteristics);
        ASSERT_SUCCEEDED(hr);

        if (hr == E_ACCESSDENIED) {
            // Everything will work as expected up until this point if the manifest capabilties
            // for bluetooth LE are not set.
            ods("Could not obtain characteristic list. Please check your manifest capabilities");
            setState(UnconnectedState);
            setError(ConnectionError);
            return FALSE;
        }
        else {
            ASSERT_SUCCEEDED(hr);
        }

        mServiceCharsVector.push_back(ServiceCharEntry(uuid, characteristics));

        UINT characteristicsCount;
        hr = characteristics->get_Size(&characteristicsCount);
        ASSERT_SUCCEEDED(hr);
        for (UINT j = 0; j < characteristicsCount; j++) {
            ComPtr<IGattCharacteristic> characteristic;
            hr = characteristics->GetAt(j, &characteristic);
            ASSERT_SUCCEEDED(hr);

            GUID charuuid = { 0 };
            characteristic->get_Uuid(&charuuid);
            UINT16 charattrHandle = 0;
            characteristic->get_AttributeHandle(&charattrHandle);

            ComPtr<IAsyncOperation<GattReadResult *>> op;
            GattCharacteristicProperties props;
            hr = characteristic->get_CharacteristicProperties(&props);
            ASSERT_SUCCEEDED(hr);
            if (!(props & GattCharacteristicProperties_Read))
                continue;
        }
    }

    return TRUE;
}

CBtWin10Interface::CBtWin10Interface(BLUETOOTH_ADDRESS *bth, LPVOID NotificationContext)
    : CBtInterface(bth, NULL, NotificationContext, OSVERSION_WINDOWS_10)
{
    m_bServiceFound = FALSE;
    m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_bDataWritePending = FALSE;
}

CBtWin10Interface::~CBtWin10Interface()
{
    ResetInterface();
}

void CBtWin10Interface::ResetInterface()
{
    if (mDevice && mStatusChangedToken.value)
        mDevice->remove_ConnectionStatusChanged(mStatusChangedToken);

    ods("Unregistering %d  value change tokens", mValueChangedTokens.size());

    for (const ValueChangedEntry &entry : mValueChangedTokens)
        entry.characteristic->remove_ValueChanged(entry.token);

    mValueChangedTokens.clear();
    mServiceCharsVector.clear();

    // Release pointer to disconnect
    mDevice = nullptr;
}

BOOL CBtWin10Interface::GetDescriptorValue(const GUID *p_guidServ, const GUID *p_guidChar, USHORT *Value)
{
    GUID serviceUuid = *p_guidServ;
    GUID charUuid = *p_guidChar;

    cout << "GetDescriptorValue characteristic " << guid2str(charUuid) << " in service " << guid2str(serviceUuid) << " for value changes";

    HRESULT hr;
    ComPtr<IGattCharacteristic> characteristic = getNativeCharacteristic(p_guidServ, p_guidChar);
    if (!characteristic) {
        cout << "Could not obtain native characteristic";

        return S_OK;
    }

    GattClientCharacteristicConfigurationDescriptorValue value = GattClientCharacteristicConfigurationDescriptorValue_None;

    ComPtr<IAsyncOperation<GattReadClientCharacteristicConfigurationDescriptorResult *>> readOp;
    hr = characteristic->ReadClientCharacteristicConfigurationDescriptorAsync(&readOp);
    ASSERT_SUCCEEDED(hr);
    ComPtr<IGattReadClientCharacteristicConfigurationDescriptorResult> readResult;
    hr = await(readOp, readResult.GetAddressOf());
    ASSERT_SUCCEEDED(hr);


    GattCommunicationStatus status;

    hr = readResult->get_Status(&status);

    //GattCommunicationStatus_Success = 0,
    //GattCommunicationStatus_Unreachable = 1,
    //GattCommunicationStatus_ProtocolError = 2,
    //GattCommunicationStatus_AccessDenied = 3

    if (status != GattCommunicationStatus_Success) {
        ods("Descriptor read operation failed");
        return S_OK;
    }

    hr = readResult->get_ClientCharacteristicConfigurationDescriptor(&value);
    ASSERT_SUCCEEDED(hr);
    UINT16 result = 0;
    BOOL correct = false;
    if (value & GattClientCharacteristicConfigurationDescriptorValue_Indicate) {
        result |= GattClientCharacteristicConfigurationDescriptorValue_Indicate;
        correct = true;
    }
    if (value & GattClientCharacteristicConfigurationDescriptorValue_Notify) {
        result |= GattClientCharacteristicConfigurationDescriptorValue_Notify;
        correct = true;
    }
    if (value == GattClientCharacteristicConfigurationDescriptorValue_None) {
        correct = true;
    }

    *Value = (USHORT)value;
    return (hr == S_OK);
}

BOOL CBtWin10Interface::SetDescriptorValue(const GUID *p_guidServ, const GUID *p_guidChar, USHORT uuidDescr, BTW_GATT_VALUE *pValue)
{
    GUID serviceUuid = *p_guidServ;
    GUID charUuid = *p_guidChar;

    cout << "SetDescriptorValue characteristic " << guid2str(charUuid) << " in service " << guid2str(serviceUuid) << " for value changes";
    cout << "Descriptor " << uuidDescr << endl;

    HRESULT hr;
    ComPtr<IGattCharacteristic> characteristic = getNativeCharacteristic(p_guidServ, p_guidChar);
    if (!characteristic) {
        cout << "Could not obtain native characteristic";

        return S_OK;
    }

    GattClientCharacteristicConfigurationDescriptorValue value = GattClientCharacteristicConfigurationDescriptorValue_None;

    if (((pValue->value[0] & 1) != 0)) // pValue->value[0] == 1)
        value = GattClientCharacteristicConfigurationDescriptorValue_Notify;

    else if (((pValue->value[0] & 2) != 0))
        value = (GattClientCharacteristicConfigurationDescriptorValue) (GattClientCharacteristicConfigurationDescriptorValue_Indicate);

    ComPtr<IAsyncOperation<enum GattCommunicationStatus>> writeOp;
    hr = characteristic->WriteClientCharacteristicConfigurationDescriptorAsync(value, &writeOp);
    ASSERT_SUCCEEDED(hr);
    hr = writeOp->put_Completed(Callback<IAsyncOperationCompletedHandler<GattCommunicationStatus >>([this](IAsyncOperation<GattCommunicationStatus> *op, AsyncStatus status)
    {
        if (status == AsyncStatus::Canceled || status == AsyncStatus::Error) {
            cout << "Descriptor write operation failed";
            //service->setError(QLowEnergyService::DescriptorWriteError);
            return S_OK;
        }
        GattCommunicationStatus result;
        HRESULT hr;
        hr = op->GetResults(&result);
        if (FAILED(hr)) {
            cout << "Could not obtain result for descriptor";

            return S_OK;
        }
        if (result != GattCommunicationStatus_Success) {
            cout << "Descriptor write operation failed";

            return S_OK;
        }
        return S_OK;
    }).Get());

    return (hr == S_OK);
}

BOOL CBtWin10Interface::ReadCharacteristic(const GUID *p_guidServ, const GUID *p_guidChar, BOOL bCached, BTW_GATT_VALUE *pValue)
{
    GUID serviceUuid = *p_guidServ;
    GUID charUuid = *p_guidChar;

    cout << "ReadCharacteristic characteristic " << guid2str(charUuid) << " in service " << guid2str(serviceUuid) << " for value changes";

    HRESULT hr;
    ComPtr<IGattCharacteristic> characteristic = getNativeCharacteristic(p_guidServ, p_guidChar);
    if (!characteristic) {
        cout << "Could not obtain native characteristic";

        return FALSE;
    }

    BluetoothCacheMode cacheMode = bCached ? BluetoothCacheMode_Cached : BluetoothCacheMode_Uncached;
    ComPtr<IAsyncOperation<GattReadResult *>> readOp;
    hr = characteristic->ReadValueWithCacheModeAsync(cacheMode, &readOp);
    ASSERT_SUCCEEDED(hr);
    ComPtr<IGattReadResult> readResult;
    hr = await(readOp, readResult.GetAddressOf());
    ASSERT_SUCCEEDED(hr);
    if (readResult)
    {
        ComPtr<ABI::Windows::Storage::Streams::IBuffer> buffer;
        hr = readResult->get_Value(&buffer);
        ASSERT_SUCCEEDED(hr);
        ComPtr<Windows::Storage::Streams::IBufferByteAccess> byteAccess;
        hr = buffer.As(&byteAccess);
        byte *data;
        hr = byteAccess->Buffer(&data);
        ASSERT_SUCCEEDED(hr);
        UINT32 size = 0;
        hr = buffer->get_Length(&size);
        ASSERT_SUCCEEDED(hr);
        // Copy the value to BTW_GATT_VALUE structure
        pValue->len = (USHORT)size;
        if(size)
            memcpy(pValue->value, data, size);
    }

    return (hr == S_OK);
}

BOOL CBtWin10Interface::WriteCharacteristic(const GUID *p_guidServ, const GUID *p_guidChar, BOOL without_resp, BTW_GATT_VALUE *pValue)
{
    GUID serviceUuid = *p_guidServ;
    GUID charUuid = *p_guidChar;

    cout << "WriteCharacteristic characteristic " << guid2str(charUuid) << " in service " << guid2str(serviceUuid) << " for value changes";
    const bool writeWithResponse = (without_resp == GattWriteOption_WriteWithResponse);

    HRESULT hr;
    ComPtr<IGattCharacteristic> characteristic = getNativeCharacteristic(p_guidServ, p_guidChar);
    if (!characteristic) {
        cout << "Could not obtain native characteristic";

        return FALSE;
    }
    ComPtr<ABI::Windows::Storage::Streams::IBufferFactory> bufferFactory;
    hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_Storage_Streams_Buffer).Get(), &bufferFactory);
    ASSERT_SUCCEEDED(hr);
    ComPtr<ABI::Windows::Storage::Streams::IBuffer> buffer;
    const int length = pValue->len;
    hr = bufferFactory->Create(length, &buffer);
    ASSERT_SUCCEEDED(hr);
    hr = buffer->put_Length(length);
    ASSERT_SUCCEEDED(hr);
    ComPtr<Windows::Storage::Streams::IBufferByteAccess> byteAccess;
    hr = buffer.As(&byteAccess);
    ASSERT_SUCCEEDED(hr);
    byte *bytes;
    hr = byteAccess->Buffer(&bytes);
    ASSERT_SUCCEEDED(hr);
    memcpy(bytes, pValue->value, length);
    ComPtr<IAsyncOperation<GattCommunicationStatus>> writeOp;
    GattWriteOption option = writeWithResponse ? GattWriteOption_WriteWithResponse : GattWriteOption_WriteWithoutResponse;
    hr = characteristic->WriteValueWithOptionAsync(buffer.Get(), option, &writeOp);
    ASSERT_SUCCEEDED(hr);

    hr = writeOp->put_Completed(Callback<IAsyncOperationCompletedHandler<GattCommunicationStatus>>([this](IAsyncOperation<GattCommunicationStatus> *op, AsyncStatus status)
    {
        if (status == AsyncStatus::Canceled || status == AsyncStatus::Error) {
            cout << "Characteristic write operation failed";
            return FALSE;
        }
        GattCommunicationStatus result;
        HRESULT hr;
        hr = op->GetResults(&result);
        if (hr == E_BLUETOOTH_ATT_INVALID_ATTRIBUTE_VALUE_LENGTH) {
            cout << "Characteristic write operation was tried with invalid value length";
            return FALSE;
        }

        if (result != GattCommunicationStatus_Success) {
            return FALSE;
        }

        if(m_bDataWritePending)
            SetEvent(m_hEvent);

        return TRUE;
    }).Get());


    ASSERT_SUCCEEDED(hr);
    return (hr == S_OK);
}

BOOL CBtWin10Interface::SetDescriptorValue(USHORT Value)
{
    HRESULT hr = E_FAIL;
    // register for notifications and indications with the status
    BTW_GATT_VALUE value = { 0 };
    value.len = 2;
    value.value[0] = (BYTE)(Value & 0xff);
    value.value[1] = (BYTE)((Value >> 8) & 0xff);

    hr = SetDescriptorValue(&guidSvcHello, &guidCharHelloNotify, BTW_GATT_UUID_DESCRIPTOR_CLIENT_CONFIG, &value);
    return (hr == S_OK);
}

BOOL CBtWin10Interface::GetDescriptorValue(USHORT *Value)
{
    GUID guid = UUID_HELLO_SERVICE;
    BOOL bReturn = FALSE;
    bReturn = GetDescriptorValue(&guidSvcHello, &guidCharHelloNotify, Value);

    return bReturn;
}

BOOL CBtWin10Interface::GetHelloInput(BTW_GATT_VALUE *pValue)
{
    ods("+%S\n", __FUNCTIONW__);
    DWORD dwResult = 0;
    BOOL bRetVal = FALSE;
    bRetVal = ReadCharacteristic(&guidSvcHello, &guidCharHelloNotify, FALSE, pValue);

    if (bRetVal)
    {
        ods("-%S %d\n", __FUNCTIONW__, dwResult);
        return TRUE;
    }
    ods("-%S %d\n", __FUNCTIONW__, dwResult);
    return FALSE;
}

BOOL CBtWin10Interface::GetHelloConfig(BYTE *pBlinks)
{
    ods("+%S\n", __FUNCTIONW__);
    BTW_GATT_VALUE value;
    DWORD dwResult = 0;
    BOOL bRetVal = FALSE;

    bRetVal = ReadCharacteristic(&guidSvcHello, &guidCharHelloConfig, FALSE, &value);

    if (bRetVal)
    {
        *pBlinks = value.value[0];
        ods("-%S %d\n", __FUNCTIONW__, dwResult);
        return TRUE;
    }
    ods("-%S %d\n", __FUNCTIONW__, dwResult);
    return FALSE;
}

BOOL CBtWin10Interface::SetHelloConfig(BYTE Blinks)
{
    ods("+%S\n", __FUNCTIONW__);
    BTW_GATT_VALUE value = { 1, Blinks };
    DWORD dwResult = 0;
    BOOL bRetVal = FALSE;

    bRetVal = WriteCharacteristic(&guidSvcHello, &guidCharHelloConfig, FALSE, &value);

    if (bRetVal)
    {
        ods("-%S %d\n", __FUNCTIONW__, dwResult);
        return TRUE;
    }
    ods("-%S %d\n", __FUNCTIONW__, dwResult);
    return FALSE;
}

BOOL CBtWin10Interface::GetBatteryLevel(BYTE *BatteryLevel)
{
    ods("+%S\n", __FUNCTIONW__);
    BTW_GATT_VALUE value = { 0 };
    DWORD dwResult = 0;
    BOOL bRetVal = FALSE;

    bRetVal = ReadCharacteristic(&guidSvcBattery, &guidCharBatLevel, FALSE, &value);

    if (bRetVal)
    {
        *BatteryLevel = value.value[0];
        ods("-%S %d\n", __FUNCTIONW__, dwResult);
        return TRUE;
    }
    ods("-%S %d\n", __FUNCTIONW__, dwResult);
    return FALSE;
}

void CBtWin10Interface::GetSystemInfo(UCHAR *szManuName, USHORT cbManuName, UCHAR *szModelNum, USHORT cbModelNum, UCHAR *SystemId, USHORT cbSystemId)
{
    ods("+%S\n", __FUNCTIONW__);
    BTW_GATT_VALUE value = { 0 };

    //(*pReadChar)(m_hReg, &m_bth, &guidSvcDeviceInfo, 0, &guidCharManufacturer, 0, 0, &value, TRUE, this);
    //memcpy(szManuName, value.value, value.len < cbManuName ? value.len : cbManuName);

    BOOL bRetVal = FALSE;
    bRetVal = ReadCharacteristic(&guidSvcDeviceInfo, &guidCharManufacturer, FALSE, &value);

    if (bRetVal)
    {
        memcpy(szManuName, value.value, value.len < cbManuName ? value.len : cbManuName);
        ods("-%S %d\n", __FUNCTIONW__, bRetVal);
    }
    else
    {
        ods("-%S %d Failed to read value\n", __FUNCTIONW__, bRetVal);
        return;
    }

    //(*pReadChar)(m_hReg, &m_bth, &guidSvcDeviceInfo, 0, &guidCharModelNumber, 0, 0, &value, TRUE, this);
    //memcpy(szModelNum, value.value, value.len < cbModelNum ? value.len : cbModelNum);

    bRetVal = ReadCharacteristic(&guidSvcDeviceInfo, &guidCharModelNumber, FALSE, &value);

    if (bRetVal)
    {
        memcpy(szModelNum, value.value, value.len < cbModelNum ? value.len : cbModelNum);
        ods("-%S %d\n", __FUNCTIONW__, bRetVal);
    }
    else
    {
        ods("-%S %d Failed to read value\n", __FUNCTIONW__, bRetVal);
        return;
    }

    //(*pReadChar)(m_hReg, &m_bth, &guidSvcDeviceInfo, 0, &guidCharSystemId, 0, 0, &value, TRUE, this);
    //memcpy(SystemId, value.value, value.len < cbSystemId ? value.len : cbSystemId);

    bRetVal = ReadCharacteristic(&guidSvcDeviceInfo, &guidCharSystemId, FALSE, &value);

    if (bRetVal)
    {
        memcpy(SystemId, value.value, value.len < cbSystemId ? value.len : cbSystemId);
        ods("-%S %d\n", __FUNCTIONW__, bRetVal);
    }
    else
    {
        ods("-%S %d Failed to read value\n", __FUNCTIONW__, bRetVal);
        return;
    }

    ods("-%S\n", __FUNCTIONW__);
}
