#include "jsbind11.h"

namespace jsbind11 {
  namespace detail {
    std::unordered_multimap<std::string, napi::callback> class_::cb_map_;
    std::unordered_map<std::string, napi_value> class_::value_map_;
  }
}
