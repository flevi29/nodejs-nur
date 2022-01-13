#ifndef _NUR_MAIN_H_
#define _NUR_MAIN_H_ 1

#include <iostream>
#include <NurAPI.h>
#include <vector>
#include <string>
#include <memory>

// Conflicts w/ g++ stdlib
#undef min
#undef max

struct NURError {
    std::wstring msg;
    int code;
};

struct EPCDataStruct {
    std::wstring epc;
    std::wstring data;
};

struct TagsArrayExtended {
    std::unique_ptr<NUR_TAG_DATA_EX[]> tags;
    int length = 0;
    NURError error;
};

struct TagsArray {
    std::unique_ptr<NUR_TAG_DATA[]> tags;
    int length = 0;
    NURError error;
};

struct DeviceVersion {
    NUR_READERINFO ri;
    NUR_DEVICECAPS dc;
    NURError error;
};

struct TX {
    int tx_level = 0;
    NURError error;
};

struct Rssi {
    NUR_RSSI_FILTER limits{};
    NURError error;
};

struct Ping {
    TCHAR buffer[16];
    NURError error;
};

struct EnumDevices {
    std::wstring path;
    std::wstring name;
};

struct Init {
    NURError error;
    HANDLE handle;
};

struct InvCount {
    NURError err;
    int count = 0;
};

NURError GetError(const int code);

EPCDataStruct EPCAndDataToString(const BYTE *epc, const BYTE epcLen, const BYTE dataLen);

std::wstring EPCToString(const BYTE *epc, const BYTE epcLen);

NURError ConnectDeviceUSB(HANDLE &nurHandle, std::wstring path);

HANDLE CreateNurHandle();

InvCount GetTagsIntoReaderBuffer(HANDLE nurHandle, int maxRounds, int Q, int session);

TagsArrayExtended GetTagsExtended(HANDLE nurHandle, int maxRounds = 0, int Q = 0, int session = 0);

TagsArray GetTags(HANDLE nurHandle, int maxRounds = 0, int Q = 0, int session = 0);

DeviceVersion GetDeviceVersion(HANDLE nurHandle);

NURError FreeHandle(HANDLE &nurHandle);

Ping PingConnectedDevice(HANDLE nurHandle);

TX GetTxLevel(HANDLE nurHandle);

NURError SetTxLevel(int level, HANDLE nurHandle);

Rssi GetRssiLimits(HANDLE nurHandle);

NURError SetRssiMin(char min, HANDLE nurHandle);

NURError SetRssiMax(char max, HANDLE nurHandle);

std::vector<EnumDevices> EnumerateUSBDevices();

bool IsDeviceConnected(HANDLE nurHandle);

NURError DisconnectDevice(HANDLE nurHandle);

NURError SetCallbackFunction(HANDLE nurHandle, NotificationCallback callBack);

NURError UnsetCallbackFunction(HANDLE nurHandle);

#endif /* _NUR_MAIN_H_ */
