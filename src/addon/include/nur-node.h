#ifndef _NUR_NODE_H_
#define _NUR_NODE_H_ 1

#include <napi.h>
#include <locale>
#include <memory>
#include <codecvt>
#include <set>
#include <map>
#include <thread>
#include "nur-main.h"
#include "nur-node-util.h"

const std::string noArgsErrMsg = "Function accepts no arguments";

struct Tag {
    std::wstring epc;
    signed char rssi;
};

struct PendingTagStruct {
    Tag tag;
    short int missingNumber;
    short int foundNumber;
    bool occurredLastScan;
};

struct WStr {
    const wchar_t *wStr;
    size_t size;
};

struct WStrArr {
    WStr wStr;
    unsigned __int64 size;
};

class TagsAsyncMapper : public Napi::AsyncProgressQueueWorker<WStrArr> {
public:
    explicit TagsAsyncMapper(HANDLE handle, int maxRounds, int Q, int session, signed char rssiMin,
                             signed char factor, int delay, bool sendAll, Napi::Function &okCallback,
                             Napi::Function &errorCallback, Napi::Function &progressCallback) :
            Napi::AsyncProgressQueueWorker<WStrArr>(okCallback), handle(handle), maxRounds(maxRounds),
            Q(Q), session(session), rssiMin(rssiMin), factor(factor),
            delay(std::chrono::milliseconds(delay)), sendAll(sendAll) {
        this->errorCallback.Reset(errorCallback, 1);
        this->progressCallback.Reset(progressCallback, 1);
    };

    ~TagsAsyncMapper() {}

    void Stop();

    void Execute(const ExecutionProgress &progress) override;

    void OnProgress(const WStrArr *strArr, size_t) override;

    void OnOK() override;

    void OnError(Napi::Error const &error) override;

private:
    bool isTagsFaulty(const TagsArray &tags);

    void MapTags(const ExecutionProgress &progress);

    Napi::FunctionReference lastCallback;
    Napi::FunctionReference progressCallback;
    Napi::FunctionReference errorCallback;
    bool continueLoop = true;
    int session;
    int Q;
    int maxRounds;
    signed char rssiMin;
    signed char factor;
    bool sendAll;
    std::chrono::milliseconds delay;
    std::set<std::wstring> passedTags;
    std::map<std::wstring, PendingTagStruct> pendingTags;
    HANDLE handle;
};

class NodeJSNUR : public Napi::ObjectWrap<NodeJSNUR> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);

    NodeJSNUR(const Napi::CallbackInfo &info);

private:
    static Napi::Value EnumerateUSBDevices(const Napi::CallbackInfo &info);

    void InitHandle(Napi::Env env);

    void Release(const Napi::CallbackInfo &info);

    NURError FreeHandle();

    void ConnectDeviceUSB(const Napi::CallbackInfo &info);

    void DisconnectDevice(const Napi::CallbackInfo &info);

    Napi::Value IsDeviceConnected(const Napi::CallbackInfo &info);

    Napi::Value PingConnectedDevice(const Napi::CallbackInfo &info);

    Napi::Value IsAsyncWorkerRunning(const Napi::CallbackInfo &info);

    void StartTagsStream(const Napi::CallbackInfo &info);

    void StopTagsStream(const Napi::CallbackInfo &info);

    Napi::Value GetTXLevel(const Napi::CallbackInfo &info);

    void SetTXLevel(const Napi::CallbackInfo &info);

    Napi::Value GetRSSILimits(const Napi::CallbackInfo &info);

    void SetRSSIMin(const Napi::CallbackInfo &info);

    void SetRSSIMax(const Napi::CallbackInfo &info);

    void DisableRSSIFilters(const Napi::CallbackInfo &info);

    void HandleNURError(Napi::Env env, const NURError error);

    void CheckHandleAndWorker(Napi::Env env);

    HANDLE nurHandle = INVALID_HANDLE_VALUE;
};

#endif /* _NUR_NODE_H_ */
