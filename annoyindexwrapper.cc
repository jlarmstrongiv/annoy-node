#include "annoyindexwrapper.h"
#include "kissrandom.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <string>

// FIXME: temporary logs
#include <fstream>
#include <iostream>

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
  bool result = false;
  // Get out object.
  AnnoyIndexWrapper* obj = ObjectWrap::Unwrap<AnnoyIndexWrapper>(info.Holder());
  // Get out file path.
  if (!info[0]->IsNullOrUndefined()) {
    Nan::MaybeLocal<String> maybeStr = Nan::To<String>(info[0]);
    v8::Local<String> str;
    if (maybeStr.ToLocal(&str)) {
      result = obj->annoyIndex->load(*Nan::Utf8String(str));
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
  Nan::HandleScope scope;

  int numberOfNeighbors, searchK;
  bool includeDistances;
  getSupplementaryGetNNsParams(info, numberOfNeighbors, searchK, includeDistances);

  // Get out object.
  AnnoyIndexWrapper* obj = ObjectWrap::Unwrap<AnnoyIndexWrapper>(info.Holder());
  // Get out input array.
  int length = obj->getDimensions();
  std::vector<float> vec(length, 0.0f);
  if (!getFloatArrayParam(info, 0, vec.data())) {
    return;
  }
  // Get out optional exclude array.
  std::vector<int> excludeVec;
  std::vector<int> *excludePtr = nullptr;
  // FIXME: temporary logs		
  std::ofstream myFile;		
  std::string filepath = "/Users/jlarmst/Desktop/code/font-scraper-cache/reverse-image-search/all-google-fonts/log0.txt";

  myFile.open(filepath, std::ios_base::app);
  if (!info[4]->IsNullOrUndefined()) {
    excludePtr = &excludeVec;
    getIntArrayParam(info, 4, excludePtr);
    for (auto itr = excludePtr->begin(); itr != excludePtr->end(); itr++) {		
      myFile << *itr << std::endl;		
    }
  }
  myFile.close();

  std::vector<int> nnIndexes;
  std::vector<float> distances;
  std::vector<float> *distancesPtr = nullptr;

  if (includeDistances) {
    distancesPtr = &distances;
  }

  // Make the call.
  obj->annoyIndex->get_nns_by_vector(
    vec.data(), numberOfNeighbors, searchK, &nnIndexes, distancesPtr, excludePtr
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
  // Get out optional exclude array.
  std::vector<int> excludeVec;
  std::vector<int> *excludePtr = nullptr;
  if (!info[1]->IsNullOrUndefined()) {
    excludePtr = &excludeVec;
    getIntArrayParam(info, 1, excludePtr);
  }
  int numberOfNeighbors, searchK;
  bool includeDistances;

  getSupplementaryGetNNsParams(info, numberOfNeighbors, searchK, includeDistances);

  std::vector<int> nnIndexes;
  std::vector<float> distances;
  std::vector<float> *distancesPtr = nullptr;

  if (includeDistances) {
    distancesPtr = &distances;
  }

  // Make the call.
  obj->annoyIndex->get_nns_by_item(
    index, numberOfNeighbors, searchK, &nnIndexes, distancesPtr, excludePtr
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
  // FIXME: temporary logs
  std::ofstream myFile;
  std::string filepath = "/Users/jlarmst/Desktop/code/font-scraper-cache/reverse-image-search/all-google-fonts/log0.txt";

  myFile.open(filepath, std::ios_base::app);
  myFile << "setNNSReturnValues 1" << std::endl;
  myFile.close();

  // Allocate the neighbors array.
  Local<Array> jsNNIndexes = Nan::New<Array>(numberOfNeighbors);
  myFile.open(filepath, std::ios_base::app);
  myFile << "setNNSReturnValues 2 " << numberOfNeighbors << " =?= " << nnIndexes.size() << std::endl;
  myFile.close();
  for (int i = 0; i < numberOfNeighbors; ++i) {
    // printf("Adding to neighbors array: %d\n", nnIndexes[i]);
    Nan::Set(jsNNIndexes, i, Nan::New<Number>(nnIndexes[i]));
  }
  
  myFile.open(filepath, std::ios_base::app);
  myFile << "setNNSReturnValues 3" << std::endl;
  myFile.close();

  Local<Object> jsResultObject;
  Local<Array> jsDistancesArray;

  if (includeDistances) {
    // Allocate the distances array.
    jsDistancesArray = Nan::New<Array>(numberOfNeighbors);
  
    myFile.open(filepath, std::ios_base::app);
    myFile << "setNNSReturnValues 4" << std::endl;
    myFile.close();
    for (int i = 0; i < numberOfNeighbors; ++i) {
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
    // TODO: Make sure it really is OK to use Local instead of Handle here.
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
    // TODO: Make sure it really is OK to use Local instead of Handle here.
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
