#include "annoyindexwrapper.h"
#include "kissrandom.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <string>

// using v8::Context;
// using v8::Function;
// using v8::FunctionCallbackInfo;
// using v8::FunctionTemplate;
// using v8::Isolate;
// using v8::Local;
// using v8::Object;
// using v8::String;
// using v8::Value;
using namespace v8;
using namespace Nan;

#ifdef ANNOYLIB_MULTITHREADED_BUILD
#define THREADED_POLICY AnnoyIndexMultiThreadedBuildPolicy
#else
#define THREADED_POLICY AnnoyIndexSingleThreadedBuildPolicy
#endif

Nan::Persistent<v8::Function> AnnoyIndexWrapper::constructor;

AnnoyIndexWrapper::AnnoyIndexWrapper(int dimensions, const char *metricString) :
  annoyDimensions(dimensions) {

  if (strcmp(metricString, "Angular") == 0) {
    annoyIndex = new AnnoyIndex<int, float, Angular, Kiss64Random, THREADED_POLICY>(dimensions);
  }
  else if (strcmp(metricString, "Manhattan") == 0) {
    annoyIndex = new AnnoyIndex<int, float, Manhattan, Kiss64Random, THREADED_POLICY>(dimensions);
  }
  else {
    annoyIndex = new AnnoyIndex<int, float, Euclidean, Kiss64Random, THREADED_POLICY>(dimensions);
  }
}

AnnoyIndexWrapper::~AnnoyIndexWrapper() {
  delete annoyIndex;
}

void AnnoyIndexWrapper::Init(v8::Local<v8::Object> exports) {
  v8::Local<v8::Context> context = exports->CreationContext();

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Annoy").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(2);

  // Prototype
  // Nan::SetPrototypeMethod(tpl, "value", GetValue);
  // Nan::SetPrototypeMethod(tpl, "plusOne", PlusOne);
  // Nan::SetPrototypeMethod(tpl, "multiply", Multiply);
  Nan::SetPrototypeMethod(tpl, "addItem", AddItem);
  Nan::SetPrototypeMethod(tpl, "onDiskBuild", OnDiskBuild);
  Nan::SetPrototypeMethod(tpl, "build", Build);
  Nan::SetPrototypeMethod(tpl, "save", Save);
  Nan::SetPrototypeMethod(tpl, "load", Load);
  Nan::SetPrototypeMethod(tpl, "unload", Unload);
  Nan::SetPrototypeMethod(tpl, "getItem", GetItem);
  Nan::SetPrototypeMethod(tpl, "getNNsByVector", GetNNSByVector);
  Nan::SetPrototypeMethod(tpl, "getNNsByItem", GetNNSByItem);
  Nan::SetPrototypeMethod(tpl, "getNItems", GetNItems);
  Nan::SetPrototypeMethod(tpl, "getDistance", GetDistance);

  constructor.Reset(tpl->GetFunction(context).ToLocalChecked());
  exports->Set(context, Nan::New("Annoy").ToLocalChecked(), tpl->GetFunction(context).ToLocalChecked()).Check();
}

void AnnoyIndexWrapper::New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();

  if (info.IsConstructCall()) {
    // Invoked as constructor: `new AnnoyIndexWrapper(...)`
    double dimensions = info[0]->IsNullOrUndefined() ? 0 : info[0]->NumberValue(context).FromJust();
    Local<String> metricString;

    if (!info[1]->IsNullOrUndefined()) {
      Nan::MaybeLocal<String> s = Nan::To<String>(info[1]);
      if (!s.IsEmpty()) {
        metricString = s.ToLocalChecked();
      }
    }

    AnnoyIndexWrapper* obj = new AnnoyIndexWrapper(
      (int)dimensions, *Nan::Utf8String(metricString)
    );
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  }
}

void AnnoyIndexWrapper::AddItem(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();
  // Get out object.
  AnnoyIndexWrapper* obj = ObjectWrap::Unwrap<AnnoyIndexWrapper>(info.Holder());
  // Get out index.
  if (info[0]->IsNumber()) {
    int index = info[0]->NumberValue(context).FromJust();
    // Get out array.
    int length = obj->getDimensions();
    std::vector<float> vec(length, 0.0f);
    if (getFloatArrayParam(info, 1, vec.data())) {
      obj->annoyIndex->add_item(index, vec.data());
    }
  } else { // info[0] is null, undefined, or array
    int length = obj->getDimensions();
    std::vector<float> vec(length, 0.0f);
    if (getFloatArrayParam(info, 0, vec.data())) {
      obj->annoyIndex->add_item(obj->annoyIndex->get_n_items(), vec.data());
    }
  }
}

void AnnoyIndexWrapper::OnDiskBuild(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  // Get out object.
  AnnoyIndexWrapper* obj = ObjectWrap::Unwrap<AnnoyIndexWrapper>(info.Holder());
  // Get out filename.
  Local<String> filenameString;

  if (!info[0]->IsNullOrUndefined()) {
    Nan::MaybeLocal<String> s = Nan::To<String>(info[0]);
    if (!s.IsEmpty()) {
      filenameString = s.ToLocalChecked();
    }
  }
  obj->annoyIndex->on_disk_build(*Nan::Utf8String(filenameString));
}

void AnnoyIndexWrapper::PrepDiskBuild(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  AnnoyIndexWrapper::OnDiskBuild(info);
}

void AnnoyIndexWrapper::Build(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();
  // Get out object.
  AnnoyIndexWrapper* obj = ObjectWrap::Unwrap<AnnoyIndexWrapper>(info.Holder());
  // Get out numberOfTrees.
  int numberOfTrees = info[0]->IsNullOrUndefined() ? 1 : info[0]->NumberValue(context).FromJust();
  // printf("%s\n", "Calling build");
  obj->annoyIndex->build(numberOfTrees);
}

void AnnoyIndexWrapper::Save(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  bool result = false;

  // Get out object.
  AnnoyIndexWrapper* obj = ObjectWrap::Unwrap<AnnoyIndexWrapper>(info.Holder());
  // Get out file path.
  if (!info[0]->IsNullOrUndefined()) {
    Nan::MaybeLocal<String> maybeStr = Nan::To<String>(info[0]);
    v8::Local<String> str;
    if (maybeStr.ToLocal(&str)) {
      result = obj->annoyIndex->save(*Nan::Utf8String(str));
    }
  }
  info.GetReturnValue().Set(Nan::New(result));
}

void AnnoyIndexWrapper::Load(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();
  bool result = false;
  // Get out object.
  AnnoyIndexWrapper* obj = ObjectWrap::Unwrap<AnnoyIndexWrapper>(info.Holder());
  // Get out file path.
  if (!info[0]->IsNullOrUndefined()) {
    if (info[0]->IsArrayBuffer()) {
      v8::Local<Object> inputBuf = info[0]->ToObject(context).ToLocalChecked();
      // std::shared_ptr<BackingStore> bufContents = ArrayBuffer::Cast(*inputBuf)->GetBackingStore();
      // result = obj->annoyIndex->loadBuffer(bufContents->Data(), bufContents->ByteLength());
      ArrayBuffer::Contents bufContents = ArrayBuffer::Cast(*inputBuf)->GetContents();
      bool makeCopy = info[1]->IsBoolean() ? info[1]->BooleanValue(info.GetIsolate()) : false;
      result = obj->annoyIndex->loadBuffer(bufContents.Data(), bufContents.ByteLength(), makeCopy);
    } else if (info[0]->IsString()) {
      Nan::MaybeLocal<String> maybeStr = Nan::To<String>(info[0]);
      v8::Local<String> str;
      if (maybeStr.ToLocal(&str)) {
        result = obj->annoyIndex->load(*Nan::Utf8String(str));
      }
    }
  }
  info.GetReturnValue().Set(Nan::New(result));
}

void AnnoyIndexWrapper::Unload(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  AnnoyIndexWrapper* obj = ObjectWrap::Unwrap<AnnoyIndexWrapper>(info.Holder());
  obj->annoyIndex->unload();
}

void AnnoyIndexWrapper::GetItem(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();
  Nan::HandleScope scope;

  // Get out object.
  AnnoyIndexWrapper* obj = ObjectWrap::Unwrap<AnnoyIndexWrapper>(info.Holder());

  // Get out index.
  int index = info[0]->IsNullOrUndefined() ? 1 : info[0]->NumberValue(context).FromJust();

  int annoyIndexSize = obj->annoyIndex->get_n_items();
  if (index < 0) {
    index = annoyIndexSize - index;
  }
  if (index >= annoyIndexSize || index < 0) {
    return Nan::ThrowError(
      "getItem: Index out of bounds"
    );
  }

  // Get the vector.
  int length = obj->getDimensions();
  std::vector<float> vec(length, 0.0f);
  obj->annoyIndex->get_item(index, vec.data());

  // Allocate the return array.
  Local<Array> results = Nan::New<Array>(length);
  for (int i = 0; i < length; ++i) {
    // printf("Adding to array: %f\n", vec[i]);
    Nan::Set(results, i, Nan::New<Number>(vec[i]));
  }

  info.GetReturnValue().Set(results);
}

void AnnoyIndexWrapper::GetDistance(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();
  // Get out object.
  AnnoyIndexWrapper* obj = ObjectWrap::Unwrap<AnnoyIndexWrapper>(info.Holder());

  // Get out indexes.
  int indexA = info[0]->IsNullOrUndefined() ? 0 : info[0]->NumberValue(context).FromJust();
  int indexB = info[1]->IsNullOrUndefined() ? 0 : info[1]->NumberValue(context).FromJust();

  // Return the distances.
  info.GetReturnValue().Set(obj->annoyIndex->get_distance(indexA, indexB));
}

void AnnoyIndexWrapper::GetNNSByVector(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  Isolate *isolate = info.GetIsolate();
  Nan::HandleScope scope;

  int numberOfNeighbors, searchK;
  bool includeDistances;
  getSupplementaryGetNNsParams(info, numberOfNeighbors, searchK, includeDistances);

  // Get out object.
  AnnoyIndexWrapper* obj = ObjectWrap::Unwrap<AnnoyIndexWrapper>(info.Holder());

  int annoyIndexSize = obj->annoyIndex->get_n_items();
  if (numberOfNeighbors >= annoyIndexSize) {
    numberOfNeighbors = annoyIndexSize - 1;
  }

  // Get out input array.
  int length = obj->getDimensions();
  std::vector<float> vec(length, 0.0f);
  if (!getFloatArrayParam(info, 0, vec.data())) {
    return;
  }
  // Get out optional filter array.
  std::vector<int> filterVec;
  std::vector<int> *filterPtr = nullptr;

  Local<String> filterString = String::Empty(isolate);
  if (!info[4]->IsNullOrUndefined()) {
    Nan::MaybeLocal<String> s = Nan::To<String>(info[4]);
    if (!s.IsEmpty()) {
      filterString = s.ToLocalChecked();
    }
    Local<String> FILTER_INCLUDE = String::NewFromUtf8(isolate, "include").ToLocalChecked();
    Local<String> FILTER_EXCLUDE = String::NewFromUtf8(isolate, "exclude").ToLocalChecked();
    if ((info[4]->StrictEquals(FILTER_INCLUDE) || info[4]->StrictEquals(FILTER_EXCLUDE)) == false) {
      // Local<String> INVALID_FILTER_VALUE = String::NewFromUtf8(isolate,
      //   "Expected 'include' or 'exclude' for filter_type but received '" +

      // ).ToLocalChecked();
      return Nan::ThrowTypeError(
        "Expected 'include' or 'exclude' for filter_type"
      );
    }
    if (!info[5]->IsNullOrUndefined()) {
      filterPtr = &filterVec;
      if (!getIntArrayParam(info, 5, filterPtr)) {
        return Nan::ThrowError(
          "Library error: failed to parse filter_vector for values"
        );
      }
    }
  }

  std::vector<int> nnIndexes;
  std::vector<float> distances;
  std::vector<float> *distancesPtr = nullptr;

  if (includeDistances) {
    distancesPtr = &distances;
  }

  // Make the call.
  obj->annoyIndex->get_nns_by_vector(
    vec.data(), numberOfNeighbors, searchK, &nnIndexes, distancesPtr, *Nan::Utf8String(filterString), filterPtr
  );

  setNNReturnValues(numberOfNeighbors, includeDistances, nnIndexes, distances, info);
}

void AnnoyIndexWrapper::GetNNSByItem(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();
  Nan::HandleScope scope;

  // Get out object.
  AnnoyIndexWrapper* obj = ObjectWrap::Unwrap<AnnoyIndexWrapper>(info.Holder());

  if (info[0]->IsNullOrUndefined()) {
    return;
  }

  // Get out params.
  int index = info[0]->NumberValue(context).FromJust();

  int annoyIndexSize = obj->annoyIndex->get_n_items();
  if (index < 0) {
    index = annoyIndexSize - index;
  }
  if (index >= annoyIndexSize || index < 0) {
    return Nan::ThrowError(
      "getNNSByItem: Index out of bounds"
    );
  }

  int numberOfNeighbors, searchK;
  bool includeDistances;

  getSupplementaryGetNNsParams(info, numberOfNeighbors, searchK, includeDistances);

  // Get out optional filter array.
  std::vector<int> filterVec;
  std::vector<int> *filterPtr = nullptr;

  Local<String> filterString;
  if (!info[4]->IsNullOrUndefined()) {
    Nan::MaybeLocal<String> s = Nan::To<String>(info[4]);
    if (!s.IsEmpty()) {
      filterString = s.ToLocalChecked();
    }
    if (!info[5]->IsNullOrUndefined()) {
      filterPtr = &filterVec;
      getIntArrayParam(info, 5, filterPtr);
    }
  }

  std::vector<int> nnIndexes;
  std::vector<float> distances;
  std::vector<float> *distancesPtr = nullptr;

  if (includeDistances) {
    distancesPtr = &distances;
  }

  // Make the call.
  obj->annoyIndex->get_nns_by_item(
    index, numberOfNeighbors, searchK, &nnIndexes, distancesPtr, *Nan::Utf8String(filterString), filterPtr
  );

  setNNReturnValues(numberOfNeighbors, includeDistances, nnIndexes, distances, info);
}

void AnnoyIndexWrapper::getSupplementaryGetNNsParams(
  const Nan::FunctionCallbackInfo<v8::Value>& info,
  int& numberOfNeighbors, int& searchK, bool& includeDistances) {
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();

  // Get out number of neighbors.
  numberOfNeighbors = info[1]->IsNullOrUndefined() ? 1 : info[1]->NumberValue(context).FromJust();

  // Get out searchK.
  searchK = info[2]->IsNullOrUndefined() ? -1 : info[2]->NumberValue(context).FromJust();

  // Get out include distances flag.
  includeDistances = info[3]->IsNullOrUndefined() ? false : Nan::To<bool>(info[3]).FromJust();
}

void AnnoyIndexWrapper::setNNReturnValues(
  int numberOfNeighbors, bool includeDistances,
  const std::vector<int>& nnIndexes, const std::vector<float>& distances,
  const Nan::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();

  // note: numberOfNeighbors might not be needed
  int resultVectorSize = nnIndexes.size();
  int resultCount = resultVectorSize < numberOfNeighbors ? resultVectorSize : numberOfNeighbors;
  // Allocate the neighbors array.
  Local<Array> jsNNIndexes = Nan::New<Array>(resultCount);
  for (int i = 0; i < resultCount; ++i) {
    // printf("Adding to neighbors array: %d\n", nnIndexes[i]);
    Nan::Set(jsNNIndexes, i, Nan::New<Number>(nnIndexes[i]));
  }

  Local<Object> jsResultObject;
  Local<Array> jsDistancesArray;

  if (includeDistances) {
    // Allocate the distances array.
    jsDistancesArray = Nan::New<Array>(resultCount);

    for (int i = 0; i < resultCount; ++i) {
      // printf("Adding to distances array: %f\n", distances[i]);
      Nan::Set(jsDistancesArray, i, Nan::New<Number>(distances[i]));
    }

    jsResultObject = Nan::New<Object>();
    jsResultObject->Set(context, Nan::New("neighbors").ToLocalChecked(), jsNNIndexes).Check();
    jsResultObject->Set(context, Nan::New("distances").ToLocalChecked(), jsDistancesArray).Check();
  }
  else {
    jsResultObject = jsNNIndexes.As<Object>();
  }

  info.GetReturnValue().Set(jsResultObject);
}

void AnnoyIndexWrapper::GetNItems(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  // Get out object.
  AnnoyIndexWrapper* obj = ObjectWrap::Unwrap<AnnoyIndexWrapper>(info.Holder());
  Local<Number> count = Nan::New<Number>(obj->annoyIndex->get_n_items());
  info.GetReturnValue().Set(count);
}

// Returns true if it was able to get items out of the array. false, if not.
bool AnnoyIndexWrapper::getFloatArrayParam(
  const Nan::FunctionCallbackInfo<v8::Value>& info, int paramIndex, float *vec) {
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();

  bool succeeded = false;

  Local<Value> val;
  if (info[paramIndex]->IsArray()) {
    Local<Array> jsArray = Local<Array>::Cast(info[paramIndex]);
    Local<Value> val;
    for (unsigned int i = 0; i < jsArray->Length(); i++) {
      val = jsArray->Get(context, i).ToLocalChecked();
      // printf("Adding item to array: %f\n", (float)val->NumberValue(context).FromJust());
      vec[i] = (float)val->NumberValue(context).FromJust();
    }
    succeeded = true;
  }

  return succeeded;
}

// Returns true if it was able to get items out of the array. false, if not.
bool AnnoyIndexWrapper::getIntArrayParam(
  const Nan::FunctionCallbackInfo<v8::Value>& info, int paramIndex, std::vector<int> *vec) {
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();

  bool succeeded = false;

  Local<Value> val;
  if (info[paramIndex]->IsArray()) {
    Local<Array> jsArray = Local<Array>::Cast(info[paramIndex]);
    Local<Value> val;
    for (unsigned int i = 0; i < jsArray->Length(); i++) {
      val = jsArray->Get(context, i).ToLocalChecked();
      // printf("Adding item to array: %d\n", (int)val->NumberValue(context).FromJust());
      vec->push_back((int)val->NumberValue(context).FromJust());
    }
    succeeded = true;
  }

  return succeeded;
}

int AnnoyIndexWrapper::getDimensions() {
  return annoyDimensions;
}
