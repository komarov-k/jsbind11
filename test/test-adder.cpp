#include "jsbind11-test.hpp"

template <typename T>
class TestAdder {
  T mFirst;
  T mSecond;
public:
  TestAdder(T first, T second) :
    mFirst(first), mSecond(second) {
  }
  T getValue() const {
    return mFirst + mSecond;
  }
};

template <typename T>
static void registerTestAdder(jsbind11::module& m, std::string name) {
  m.class_<TestAdder<T>>(name)
    .template constructor<T,T>()
    .method("getValue", &TestAdder<T>::getValue);
}

JSBIND11_TEST(TestAdder, m) {
  registerTestAdder<float>(m, "TestFloatAdder");
  registerTestAdder<double>(m, "TestDoubleAdder");
  registerTestAdder<int8_t>(m, "TestInt8Adder");
  registerTestAdder<int16_t>(m, "TestInt16Adder");
  registerTestAdder<int32_t>(m, "TestInt32Adder");
  registerTestAdder<int64_t>(m, "TestInt64Adder");
  registerTestAdder<uint8_t>(m, "TestUInt8Adder");
  registerTestAdder<uint16_t>(m, "TestUInt16Adder");
  registerTestAdder<uint32_t>(m, "TestUInt32Adder");
  registerTestAdder<uint64_t>(m, "TestUInt64Adder");  
}
