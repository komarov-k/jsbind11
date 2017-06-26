#include "jsbind11.h"

double mul(double a, double b) {
  return a * b;
}

JSBIND11_MODULE(addon, m) {
  m.function("add", [](double a, double b) { return a + b; });
  m.function("mul", &mul);
};
