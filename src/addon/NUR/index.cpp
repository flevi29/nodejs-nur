#include "../include/nur-node.h"

Napi::Object Init(Napi::Env env, Napi::Object exports) {
	NodeJSNUR::Init(env, exports);
	return exports;
}

NODE_API_MODULE(NODE_NUR, Init)
