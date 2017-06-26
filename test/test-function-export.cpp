#include "jsbind11-test.hpp"

template <typename T>
T testBypassFunction(T value) { return value; };

JSBIND11_TEST(TestBypassFunction, m) {
  m.function("testFloatBypassFunction", &testBypassFunction<float>);
  m.function("testDoubleBypassFunction", &testBypassFunction<double>);
  m.function("testInt8BypassFunction", &testBypassFunction<int8_t>);
  m.function("testInt16BypassFunction", &testBypassFunction<int16_t>);
  m.function("testInt32BypassFunction", &testBypassFunction<int32_t>);
  m.function("testInt64BypassFunction", &testBypassFunction<int64_t>);
  m.function("testUInt8BypassFunction", &testBypassFunction<uint8_t>);
  m.function("testUInt16BypassFunction", &testBypassFunction<uint16_t>);
  m.function("testUInt32BypassFunction", &testBypassFunction<uint32_t>);
  m.function("testUInt64BypassFunction", &testBypassFunction<uint64_t>);
}
