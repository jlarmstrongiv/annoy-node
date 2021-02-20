#include <nan.h>
#include "annoyindexwrapper.h"

using v8::Local;
using v8::Object;

void InitAll(Local<Object> exports) {
  AnnoyIndexWrapper::Init(exports);
}

NAN_MODULE_WORKER_ENABLED(NODE_GYP_MODULE_NAME, InitAll)
