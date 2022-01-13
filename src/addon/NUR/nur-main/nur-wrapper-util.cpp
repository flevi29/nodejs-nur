#include "nur-main.h"

NURError GetError(const int code) {
    NURError retError{L"", code};
    if (code != NUR_NO_ERROR && code != NUR_ERROR_NO_TAG) {
        TCHAR errorMsg[129]; // 128 + NULL
        const auto size = NurApiGetErrorMessage(code, errorMsg, 128);
        retError.msg = std::wstring(errorMsg, size);
    }
    return retError;
}

EPCDataStruct EPCAndDataToString(const BYTE *epc, const BYTE epcLen, const BYTE dataLen) {
    TCHAR epcStr[128], dataStr[128];
    BYTE n = 0;
    int pos = 0;
    for (; n < epcLen; n++)
        pos += _stprintf_s(&epcStr[pos], NUR_MAX_EPC_LENGTH - pos, _T("%02x"), epc[n]);
    epcStr[pos] = 0;

    for (; n < epcLen + dataLen; n++)
        pos += _stprintf_s(&dataStr[pos], NUR_MAX_EPC_LENGTH - pos, _T("%02x"), epc[n]);
    dataStr[pos] = 0;

    return EPCDataStruct{std::wstring(epcStr), std::wstring(dataStr)};
}

std::wstring EPCToString(const BYTE *epc, const BYTE epcLen) {
    TCHAR epcStr[128];
    int pos = 0;
    for (DWORD n = 0; n < epcLen; n++)
        pos += _stprintf_s(&epcStr[pos], NUR_MAX_EPC_LENGTH - pos, _T("%02x"), epc[n]);
    epcStr[pos] = 0;

    return std::wstring(epcStr);
}
