#include "../../include/nur-node.h"

bool asyncRunning = false;
TagsAsyncMapper *mapper = nullptr;

bool TagsAsyncMapper::isTagsFaulty(const TagsArray &tags) {
    if (tags.error.code == NUR_NO_ERROR || tags.error.code == NUR_ERROR_NO_TAG) return false;
    SetError("NodeNURReaderError" + std::to_string(tags.error.code));
    continueLoop = false;
    return true;
}

void TagsAsyncMapper::MapTags(const ExecutionProgress &progress) {
    const auto inventoryDetails = GetTags(handle, maxRounds, Q, session);
    if (isTagsFaulty(inventoryDetails)) return;

    bool newElement = false, removedElement = false;
    for (int i = 0; i < inventoryDetails.length; i++) {
        const auto *tag = &inventoryDetails.tags[i];
        if (rssiMin != 0 && tag->rssi < rssiMin) continue;

        const auto epc = EPCToString(tag->epc, tag->epcLen);
        auto tagDetails = PendingTagStruct{
                Tag{epc, tag->rssi},
                0,
                1,
                true
        };

        const auto getTagIterator = pendingTags.find(epc);
        if (getTagIterator != pendingTags.end()) {
            const auto getTag = getTagIterator->second;
            tagDetails.foundNumber = getTag.foundNumber;
            if (getTag.foundNumber > factor) {
                if (!passedTags.contains(epc)) {
                    passedTags.insert(epc);
                    if (sendAll) {
                        const auto strArr = WStrArr{
                                WStr{getTagIterator->first.c_str(), getTagIterator->first.length()},
                                1
                        };
                        progress.Send(&strArr, 1);
                    } else {
                        newElement = true;
                    }
                }
            } else {
                tagDetails.foundNumber++;
            }
        }
        pendingTags.insert_or_assign(epc, tagDetails);
    }

    for (auto it = pendingTags.begin(); it != pendingTags.end(); it++) {
        auto tagT = it->second;
        const auto key = it->first;

        if (!tagT.occurredLastScan) {
            if (tagT.missingNumber > factor) {
                it = pendingTags.erase(it);
                passedTags.erase(key);
                if (sendAll) removedElement = true;
            } else {
                tagT.missingNumber++;
                pendingTags.insert_or_assign(key, tagT);
            }
        } else {
            tagT.occurredLastScan = false;
            pendingTags.insert_or_assign(key, tagT);
        }
    }

    if (sendAll || (!newElement && !removedElement)) return;
    auto passedTagsSize = passedTags.size();
    if (passedTagsSize != 1) {
        const auto strArr = WStrArr{
                WStr{nullptr, 0},
                static_cast<unsigned __int64>(passedTagsSize)
        };
        progress.Send(&strArr, 1);
    } else {
        const auto strArr = WStrArr{
                WStr{passedTags.begin()->c_str(), passedTags.begin()->length()},
                1
        };
        progress.Send(&strArr, 1);
    }
    //if (status != napi_ok) std::cout << "napi_status != napi_ok at the end of NURNotificationCB\r\n";
}

void TagsAsyncMapper::Stop() {
    continueLoop = false;
}

void TagsAsyncMapper::OnProgress(const WStrArr *strArr, size_t) {
    auto env = Env();
    Napi::HandleScope scope(env);
    Napi::Value val;
    if (strArr->size > 1)
        val = Napi::Number::New(env, strArr->size);
    else
        val = NewNapiStr(env, std::wstring(strArr->wStr.wStr, strArr->wStr.size));
    if (!progressCallback.IsEmpty()) progressCallback.Call(Receiver().Value(), {val});
}

void TagsAsyncMapper::Execute(const ExecutionProgress &progress) {
    while (continueLoop) {
        MapTags(progress);
        std::this_thread::sleep_for(delay);
    }
}

void TagsAsyncMapper::OnOK() {
    Napi::HandleScope OnOKScope(Env());
    Callback().Call({});
    asyncRunning = false;
}

void TagsAsyncMapper::OnError(Napi::Error const &error) {
    Napi::HandleScope OnErrorScope(Env());
    if (!this->errorCallback.IsEmpty()) {
        if (error.Message().substr(0, 18).compare("NodeNURReaderError") == 0) {
            const auto errCode = std::stoi(error.Message().substr(18));
            this->errorCallback.Call(Receiver().Value(), {NewNapiStr(Env(), GetError(errCode).msg)});
        } else {
            this->errorCallback.Call(Receiver().Value(), {Napi::String::New(Env(), error.Message())});
        }
    }
    asyncRunning = false;
}

Napi::ThreadSafeFunction threadSafeFunction;

void ConnectionCallback(Napi::Env env, Napi::Function jsCallback, bool *connected) {
    jsCallback.Call({Napi::String::New(env, "connection"), Napi::Boolean::New(env, *connected)});
    delete connected;
}

void TransportCallback(Napi::Env env, Napi::Function jsCallback, bool *connected) {
    jsCallback.Call({Napi::String::New(env, "transport"), Napi::Boolean::New(env, *connected)});
    delete connected;
}

void ModuleBootedCallback(Napi::Env env, Napi::Function jsCallback) {
    jsCallback.Call({Napi::String::New(env, "moduleBooted")});
}

void UnhandledNotificationCallback(Napi::Env env, Napi::Function jsCallback) {
    jsCallback.Call({Napi::String::New(env, "unhandled")});
}

void NURAPICALLBACK NURNotificationCB(HANDLE hApi, DWORD timestamp, int type, LPVOID data, int dataLen) {
    if (threadSafeFunction.Acquire() == napi_closing) {
        return;
    }
    napi_status status;
    switch (type) {
        case NUR_NOTIFICATION_TRDISCONNECTED:
//          Transport disconnected
            status = threadSafeFunction.BlockingCall(new bool(false), TransportCallback);
            break;
        case NUR_NOTIFICATION_MODULEBOOT:
//          Module booted
            status = threadSafeFunction.BlockingCall(ModuleBootedCallback);
            break;
        case NUR_NOTIFICATION_TRCONNECTED:
//          Transport connected
            status = threadSafeFunction.BlockingCall(new bool(true), TransportCallback);
            break;
        default:
//          Unhandled notification
            status = threadSafeFunction.BlockingCall(UnhandledNotificationCallback);
            break;
    }

    if (status != napi_ok) { /*no-op for now*/ }
    threadSafeFunction.Release();
}

Napi::Object NodeJSNUR::Init(Napi::Env env, Napi::Object exports) {
    const auto className = "NodeJSNUR";
    Napi::Function func = DefineClass(env, className, {
            StaticMethod<&NodeJSNUR::EnumerateUSBDevices>(
                    "EnumerateUSBDevices",
                    static_cast<napi_property_attributes>(napi_static)
            ),
            InstanceMethod<&NodeJSNUR::Free>("Free"),
            InstanceMethod<&NodeJSNUR::ConnectDeviceUSB>("ConnectDeviceUSB"),
            InstanceMethod<&NodeJSNUR::DisconnectDevice>("DisconnectDevice"),
            InstanceMethod<&NodeJSNUR::IsDeviceConnected>("IsDeviceConnected"),
            InstanceMethod<&NodeJSNUR::PingConnectedDevice>("PingConnectedDevice"),
            InstanceMethod<&NodeJSNUR::IsAsyncWorkerRunning>("IsAsyncWorkerRunning"),
            InstanceMethod<&NodeJSNUR::StartTagsStream>("StartTagsStream"),
            InstanceMethod<&NodeJSNUR::StopTagsStream>("StopTagsStream"),
            InstanceMethod<&NodeJSNUR::GetTXLevel>("GetTXLevel"),
            InstanceMethod<&NodeJSNUR::SetTXLevel>("SetTXLevel"),
            InstanceMethod<&NodeJSNUR::GetRSSILimits>("GetRSSILimits"),
            InstanceMethod<&NodeJSNUR::SetRSSIMin>("SetRSSIMin"),
            InstanceMethod<&NodeJSNUR::SetRSSIMax>("SetRSSIMax"),
            InstanceMethod<&NodeJSNUR::DisableRSSIFilters>("DisableRSSIFilters")
    });
    Napi::FunctionReference *constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    exports.Set(className, func);
    env.SetInstanceData<Napi::FunctionReference>(constructor);
    return exports;
}

NodeJSNUR::NodeJSNUR(const Napi::CallbackInfo &info) :
        Napi::ObjectWrap<NodeJSNUR>(info) {
    Napi::Env env = info.Env();
    if (info.Length() != 1) throw Napi::TypeError::New(env, "Expected 1 argument");
    if (!info[0].IsFunction()) throw Napi::TypeError::New(env, "Argument must be a function");
    tsfn = Napi::ThreadSafeFunction::New(
            env,
            info[0].As<Napi::Function>(),
            "emitter for NUR Notification Callback",
            0,
            1
    );
    threadSafeFunction = tsfn;
    freed = false;
    InitHandle(env);
}

Napi::Value NodeJSNUR::EnumerateUSBDevices(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() != 0) throw Napi::TypeError::New(env, noArgsErrMsg);
    auto devices = ::EnumerateUSBDevices();
    auto retArr = Napi::Array::New(env, devices.size());
    for (uint32_t i = 0; i < devices.size(); i++) {
        auto array = Napi::Array::New(env, 2);
        array[0u] = NewNapiStr(env, devices[i].path);
        array[1u] = NewNapiStr(env, devices[i].name);
        retArr[i] = array;
    }
    return retArr;
}

Napi::Value NodeJSNUR::IsDeviceConnected(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() != 0) throw Napi::TypeError::New(env, noArgsErrMsg);
    CheckHandleAndWorker(env);
    return Napi::Boolean::New(env, ::IsDeviceConnected(nurHandle));
}

void NodeJSNUR::ConnectDeviceUSB(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() != 1 || !info[0].IsString())
        throw Napi::TypeError::New(env, "Invalid arguments");
    CheckHandleAndWorker(env);
    HandleNURError(env, ::ConnectDeviceUSB(nurHandle, NewWStr(info[0].As<Napi::String>())));
    auto connected = new bool(true);
    tsfn.BlockingCall(connected, ConnectionCallback);
}

void NodeJSNUR::DisconnectDevice(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() != 0) throw Napi::TypeError::New(env, noArgsErrMsg);
    CheckHandleAndWorker(env);
    HandleNURError(env, ::DisconnectDevice(nurHandle));
    tsfn.BlockingCall(new bool(false), ConnectionCallback);
}

Napi::Value NodeJSNUR::IsAsyncWorkerRunning(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() != 0) throw Napi::TypeError::New(env, noArgsErrMsg);
    return Napi::Boolean::New(env, asyncRunning);
}

void NodeJSNUR::HandleNURError(Napi::Env env, const NURError error) {
    if (error.code == NUR_NO_ERROR) return;
    if (FreeHandle().code != NUR_NO_ERROR) return;
    InitHandle(env);
    throw NapiNURError(env, error);
}

void NodeJSNUR::CheckHandleAndWorker(Napi::Env env) {
    if (nurHandle == INVALID_HANDLE_VALUE)
        throw Napi::Error::New(env, "Handle is invalid, this object is no longer operational");
    if (asyncRunning)
        throw Napi::Error::New(env, "An async worker must be awaited");
}

void NodeJSNUR::InitHandle(Napi::Env env) {
    nurHandle = CreateNurHandle();
    if (nurHandle == INVALID_HANDLE_VALUE)
        throw Napi::Error::New(env, "Error on NUR handle creation");
    if (SetCallbackFunction(nurHandle, NURNotificationCB).code != NUR_NO_ERROR)
        throw Napi::Error::New(env, "Error on callback function assignment");
}

void NodeJSNUR::Finalize(Napi::Env env) {
    if (nurHandle != INVALID_HANDLE_VALUE) FreeHandle();
    if (freed) return;
    tsfn.Abort();
    tsfn.Release();
}

void NodeJSNUR::Free(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() != 0) throw Napi::TypeError::New(env, noArgsErrMsg);
    if (nurHandle != INVALID_HANDLE_VALUE) FreeHandle();
    tsfn.Abort();
    tsfn.Release();
    freed = true;
}

NURError NodeJSNUR::FreeHandle() {
    if (::IsDeviceConnected(nurHandle))
        tsfn.BlockingCall(new bool(false), ConnectionCallback);
    return ::FreeHandle(nurHandle);
}

Napi::Value NodeJSNUR::PingConnectedDevice(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() != 0) throw Napi::TypeError::New(env, noArgsErrMsg);
    CheckHandleAndWorker(env);
    const auto ping = ::PingConnectedDevice(nurHandle);
    HandleNURError(env, ping.error);
    return NewNapiStr(env, std::wstring(ping.buffer));
}

void NodeJSNUR::StartTagsStream(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() < 4)
        throw Napi::TypeError::New(env, "Expected 4 arguments");
    if (!info[0].IsFunction() || !info[1].IsFunction() || !info[2].IsFunction() || !info[3].IsObject())
        throw Napi::TypeError::New(env, "Invalid arguments");
    CheckHandleAndWorker(env);

    Napi::Function progressCb = info[0].As<Napi::Function>(),
            okCb = info[1].As<Napi::Function>(),
            errorCb = info[2].As<Napi::Function>();
    auto options = info[3].As<Napi::Object>();
    const auto modeObj = options.Get("mode"), factorObj = options.Get("factor"),
            rssiMinObj = options.Get("rssiMin"), delayObj = options.Get("delay"),
            maxRoundsObj = options.Get("maxRounds"), QObj = options.Get("Q"),
            sessionObj = options.Get("session");
    const auto mode = modeObj.IsUndefined() || modeObj.As<Napi::String>().Utf8Value() == "all";
    const int factor = factorObj.As<Napi::Number>().Int32Value(),
            rssiMin = rssiMinObj.As<Napi::Number>().Int32Value(),
            delay = delayObj.IsUndefined() ? 100 : delayObj.As<Napi::Number>().Int32Value(),
            maxRounds = maxRoundsObj.IsUndefined() ? 0 : maxRoundsObj.As<Napi::Number>().Int32Value(),
            Q = QObj.IsUndefined() ? 0 : QObj.As<Napi::Number>().Int32Value(),
            session = sessionObj.IsUndefined() ? 0 : sessionObj.As<Napi::Number>().Int32Value();

    if (Q < 0 || Q > 15 || session < 0 || session > 3 || maxRounds < 0 || maxRounds > 10 ||
        factor < 0 || delay < 0 || rssiMin > 0 || rssiMin < -128)
        throw Napi::RangeError::New(env,
                                    "Arguments must respect following rules: factor >= 0; -128 <= rssiMin <= 0; sleepFor >= 0; 0 <= Q <= 15; 0 <= session <= 3; 0 <= maxRounds <= 10");

    mapper = new TagsAsyncMapper(nurHandle, maxRounds, Q, session, rssiMin, factor,
                                delay, mode, okCb, errorCb, progressCb);
    mapper->Queue();
    asyncRunning = true;
}

void NodeJSNUR::StopTagsStream(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() != 0) throw Napi::TypeError::New(env, noArgsErrMsg);
    if (asyncRunning) {
        mapper->Stop();
        mapper = nullptr;
    }
}

Napi::Value NodeJSNUR::GetTXLevel(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() != 0) throw Napi::TypeError::New(env, noArgsErrMsg);
    CheckHandleAndWorker(env);
    const auto txLevel = GetTxLevel(nurHandle);
    HandleNURError(env, txLevel.error);
    return Napi::Number::New(env, txLevel.tx_level);
}

void NodeJSNUR::SetTXLevel(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() != 1 || !info[0].IsNumber())
        throw Napi::TypeError::New(env, "Invalid arguments");
    CheckHandleAndWorker(env);
    auto level = info[0].As<Napi::Number>().Int32Value();
    if (level > 20 || level < 0) throw Napi::RangeError::New(env, "Argument must be between 0 and 20");
    HandleNURError(env, SetTxLevel(info[0].As<Napi::Number>().Int32Value(), nurHandle));
}

Napi::Value NodeJSNUR::GetRSSILimits(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() != 0)
        throw Napi::TypeError::New(env, noArgsErrMsg);
    CheckHandleAndWorker(env);
    const auto rssiLimits = GetRssiLimits(nurHandle);
    HandleNURError(env, rssiLimits.error);
    auto levelsObj = Napi::Object::New(env);
    levelsObj["max"] = Napi::Number::New(env, rssiLimits.limits.max);
    levelsObj["min"] = Napi::Number::New(env, rssiLimits.limits.min);
    return levelsObj;
}

void NodeJSNUR::SetRSSIMin(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() != 1 || !info[0].IsNumber())
        throw Napi::TypeError::New(env, "Invalid arguments");
    CheckHandleAndWorker(env);
    const int32_t int32v = info[0].As<Napi::Number>().Int32Value();
    if (int32v < -128 || int32v > 0) throw Napi::RangeError::New(env, "Argument must be between -128 and 0");
    HandleNURError(env, SetRssiMin(static_cast<const char>(int32v), nurHandle));
}

void NodeJSNUR::SetRSSIMax(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() != 1 || !info[0].IsNumber())
        throw Napi::TypeError::New(env, "Invalid arguments");
    CheckHandleAndWorker(env);
    const int32_t int32v = info[0].As<Napi::Number>().Int32Value();
    if (int32v < -128 || int32v > 0) throw Napi::RangeError::New(env, "Argument must be between -128 and 0");
    HandleNURError(env, SetRssiMax(static_cast<const char>(int32v), nurHandle));
}

void NodeJSNUR::DisableRSSIFilters(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() != 0) throw Napi::TypeError::New(env, noArgsErrMsg);
    CheckHandleAndWorker(env);
    HandleNURError(env, SetRssiMax(0, nurHandle));
    HandleNURError(env, SetRssiMin(0, nurHandle));
}
