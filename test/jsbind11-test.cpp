#include "jsbind11-test.hpp"

static std::vector<std::function<void(jsbind11::module&)>> testExports;

Export::Export(void (*t)(jsbind11::module&)) {
  testExports.push_back(std::function<void(jsbind11::module&)>(t));
}

JSBIND11_MODULE(test, m) {
  for (auto testExport : testExports) {
    testExport(m);
  }
};
