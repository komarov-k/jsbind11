#ifndef JSBIND11_TEST_HPP_
#define JSBIND11_TEST_HPP_

#include <jsbind11.h>

struct Export {
  Export(void (*)(jsbind11::module&));
};

#define JSBIND11_TEST(NAME, M)					\
  void __JSBIND11_TEST_ ## NAME (jsbind11::module& M);		\
  Export __JSBIND11_EXPORT ## NAME (__JSBIND11_TEST_ ## NAME);	\
  void __JSBIND11_TEST_ ## NAME (jsbind11::module& M)

#endif // JSBIND11_TEST_HPP_
