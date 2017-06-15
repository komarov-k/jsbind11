
#include "jsbind11.h"

#include <iostream>
#include <string>

class Hello {
  double mValue;
public:
  explicit Hello(double value) : mValue(value) {}

  void sayHello() const {
    std::cout << "Hello " << mValue << std::endl;
  }
};

JSBIND11_MODULE(addon, m) {
  m.class_<Hello>("Hello")
    .constructor<double>()
    .method("sayHello", &Hello::sayHello)
    .method("sayHelloTwice", [](const Hello& hello) {
	hello.sayHello();
	hello.sayHello();
      });
};
