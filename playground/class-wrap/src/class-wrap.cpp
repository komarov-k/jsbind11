
#include "jsbind11.h"

#include <iostream>
#include <string>

class Adder {
  double mFirst;
  double mSecond;
public:
  Adder(double first, double second) :
    mFirst(first),
    mSecond(second) {}
  double getValue() const {
    return mFirst + mSecond;
  }
};

JSBIND11_MODULE(addon, m) {
  m.class_<Adder>("Adder")
    .constructor<double, double>()
    .method("getValue", &Adder::getValue)
    .method("getDoubleValue", [](const Adder& adder) {
	return adder.getValue() * 2;
      });
};
