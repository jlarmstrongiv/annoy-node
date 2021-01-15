#ifndef ANNOYINDEXWRAPPER_H
#define ANNOYINDEXWRAPPER_H

#include <nan.h>
#include "annoylib.h"
#include <vector>

class AnnoyIndexWrapper : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports);
  int getDimensions();
  AnnoyIndexInterface<int, float> *annoyIndex;

 private:
  explicit AnnoyIndexWrapper(int dimensions, const char *metricString);
  virtual ~AnnoyIndexWrapper();

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void AddItem(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void OnDiskBuild(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void PrepDiskBuild(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Build(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Save(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Load(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Unload(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void GetItem(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void GetNNSByVector(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void GetNNSByItem(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void GetNItems(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void GetDistance(const Nan::FunctionCallbackInfo<v8::Value>& info);

  static Nan::Persistent<v8::Function> constructor;
  static bool getFloatArrayParam(const Nan::FunctionCallbackInfo<v8::Value>& info, 
    int paramIndex, float *vec);
  static bool getIntArrayParam(const Nan::FunctionCallbackInfo<v8::Value>& info, 
    int paramIndex, std::vector<int> vec);
  static void setNNReturnValues(
    int numberOfNeighbors, bool includeDistances,
    const std::vector<int>& nnIndexes, const std::vector<float>& distances,
    const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getSupplementaryGetNNsParams(
    const Nan::FunctionCallbackInfo<v8::Value>& info,
    int& numberOfNeighbors, int& searchK, bool& includeDistances);

  int annoyDimensions;
};

#endif
