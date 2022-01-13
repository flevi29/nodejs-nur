#include "nur-main.h"

WORD fetch_tags_from_module = 0;

///////////////// NotificationCallback /////////////////

NURError SetCallbackFunction(HANDLE nurHandle, NotificationCallback callBack) {
    return GetError(NurApiSetNotificationCallback(nurHandle, callBack));
}

NURError UnsetCallbackFunction(HANDLE nurHandle) {
    return GetError(NurApiSetNotificationCallback(nurHandle, nullptr));
}

///////////////// Device|Handle-independent /////////////////

int NURAPICALLBACK EnumCallback(const TCHAR *path, const TCHAR *friendlyName, LPVOID enumDevices) {
    const auto enumDevice = EnumDevices{std::wstring(path), std::wstring(friendlyName)};
    reinterpret_cast<std::vector<EnumDevices> *>(enumDevices)->push_back(enumDevice);
    return 0;
}

std::vector<EnumDevices> EnumerateUSBDevices() {
    std::vector<EnumDevices> enumDevices;
    NurUSBEnumerateDevices(EnumCallback, static_cast<LPVOID>(&enumDevices));
    return enumDevices;
}

///////////////// Device|Init|Free|Connection /////////////////

HANDLE CreateNurHandle() {
    return NurApiCreate();
}

NURError ConnectDeviceUSB(HANDLE &nurHandle, std::wstring path) {
    return GetError(NurApiConnectUsb(nurHandle, path.c_str()));
}

NURError FreeHandle(HANDLE &nurHandle) {
    if (nurHandle == INVALID_HANDLE_VALUE)
        return GetError(NUR_ERROR_INVALID_HANDLE);
    UnsetCallbackFunction(nurHandle);
    auto err = GetError(NurApiFree(nurHandle));
    nurHandle = INVALID_HANDLE_VALUE;
    return err;
}

Ping PingConnectedDevice(HANDLE nurHandle) {
    Ping ret{};
    ret.error = GetError(NurApiPing(nurHandle, ret.buffer));
    return ret;
}

bool IsDeviceConnected(HANDLE nurHandle) {
    return NurApiIsConnected(nurHandle) == NUR_SUCCESS;
}

NURError DisconnectDevice(HANDLE nurHandle) {
    return GetError(NurApiDisconnect(nurHandle));
}

///////////////// Device|Handle-dependent /////////////////

Rssi GetRssiLimits(HANDLE nurHandle) {
    NUR_MODULESETUP setup{};
    Rssi rssi;
    rssi.error = GetError(
            NurApiGetModuleSetup(nurHandle, NUR_SETUP_INVRSSIFILTER, &setup, sizeof(setup))
    );
    rssi.limits = setup.inventoryRssiFilter;
    return rssi;
}

NURError SetRssiMax(const char max, HANDLE nurHandle) {
    const auto current = GetRssiLimits(nurHandle);
    NUR_MODULESETUP setup{};
    setup.inventoryRssiFilter.max = max;
    setup.inventoryRssiFilter.min = current.limits.min;
    return GetError(
            NurApiSetModuleSetup(nurHandle, NUR_SETUP_INVRSSIFILTER, &setup, sizeof(setup))
    );
}

NURError SetRssiMin(const char min, HANDLE nurHandle) {
    const auto current = GetRssiLimits(nurHandle);
    NUR_MODULESETUP setup{};
    setup.inventoryRssiFilter.min = min;
    setup.inventoryRssiFilter.max = current.limits.max;
    return GetError(
            NurApiSetModuleSetup(nurHandle, NUR_SETUP_INVRSSIFILTER, &setup, sizeof(setup))
    );
}

NURError SetTxLevel(const int level, HANDLE nurHandle) {
    NUR_MODULESETUP setup{};
    setup.txLevel = level;
    return GetError(
            NurApiSetModuleSetup(nurHandle, NUR_SETUP_TXLEVEL, &setup, sizeof(setup))
    );
}

TX GetTxLevel(HANDLE nurHandle) {
    NUR_MODULESETUP setup{};
    TX tx{};
    tx.error = GetError(
            NurApiGetModuleSetup(nurHandle, NUR_SETUP_TXLEVEL, &setup, sizeof(setup))
    );
    tx.tx_level = setup.txLevel;
    return tx;
}

DeviceVersion GetDeviceVersion(HANDLE nurHandle) {
    DeviceVersion dv{};

    dv.error = GetError(NurApiGetReaderInfo(nurHandle, &dv.ri, sizeof(struct NUR_READERINFO)));
    if (dv.error.code != NUR_NO_ERROR) {
        return dv;
    }
    dv.error = GetError(NurApiGetDeviceCaps(nurHandle, &dv.dc));
    fetch_tags_from_module = dv.dc.szTagBuffer / 3;
    return dv;
}

InvCount GetTagsIntoReaderBuffer(HANDLE nurHandle, const int maxRounds, const int Q, const int session) {
    NUR_INVENTORY_RESPONSE invResp{};
    InvCount inv{};

    // Clear previously inventoried tags from NurApi internal tag storage and from module.
    inv.err = GetError(NurApiClearTags(nurHandle));
    if (inv.err.code != NUR_NO_ERROR) return inv;

//    int rounds = 0;
//    while (rounds++ < maxRounds) {
    // Perform inventory
    //inv.err = GetError(NurApiInventory(nurHandle, rounds, Q, session, &invResp), nurHandle);
    inv.err = GetError(NurApiInventory(nurHandle, maxRounds, Q, session, &invResp));

    if (inv.err.code != NUR_NO_ERROR && inv.err.code != NUR_ERROR_NO_TAG)
        return inv;
    if (invResp.numTagsMem > fetch_tags_from_module) {
        // Fetch tags from module (including tag meta) to NurApi
        // internal tag storage and clear tags from module.
        inv.err = GetError(NurApiFetchTags(nurHandle, TRUE, nullptr));
        if (inv.err.code != NUR_NO_ERROR) return inv;
    }
//    }

    inv.count = 0;
    inv.err = GetError(NurApiGetTagCount(nurHandle, &inv.count));
    return inv;
}

TagsArrayExtended GetTagsExtended(HANDLE nurHandle, const int maxRounds, const int Q, const int session) {
    TagsArrayExtended tagsStruct{};
    auto inv = GetTagsIntoReaderBuffer(nurHandle, maxRounds, Q, session);
    tagsStruct.error = inv.err;
    if (tagsStruct.error.code != NUR_NO_ERROR) return tagsStruct;

    // Loop through tags
    tagsStruct.tags = std::make_unique<NUR_TAG_DATA_EX[]>(inv.count);
    tagsStruct.length = inv.count;
    for (int idx = 0; idx < inv.count; idx++) {
        NUR_TAG_DATA_EX tagDataEx{};
        tagsStruct.error = GetError(NurApiGetTagDataEx(nurHandle, idx, &tagDataEx, sizeof(tagDataEx)));
        if (tagsStruct.error.code == NUR_NO_ERROR) tagsStruct.tags[idx] = tagDataEx;
        else if (tagsStruct.error.code != NUR_ERROR_NO_TAG) break;
    }

    return tagsStruct;
}

TagsArray GetTags(HANDLE nurHandle, const int maxRounds, const int Q, const int session) {
    TagsArray tagsStruct{};
    auto inv = GetTagsIntoReaderBuffer(nurHandle, maxRounds, Q, session);
    tagsStruct.error = inv.err;
    if (tagsStruct.error.code != NUR_NO_ERROR) return tagsStruct;

    // Loop through tags
    tagsStruct.tags = std::make_unique<NUR_TAG_DATA[]>(inv.count);
    tagsStruct.length = inv.count;
    for (int idx = 0; idx < inv.count; idx++) {
        NUR_TAG_DATA tagData{};
        tagsStruct.error = GetError(NurApiGetTagData(nurHandle, idx, &tagData));
        if (tagsStruct.error.code == NUR_NO_ERROR) tagsStruct.tags[idx] = tagData;
        else if (tagsStruct.error.code != NUR_ERROR_NO_TAG) break;
    }

    return tagsStruct;
}
