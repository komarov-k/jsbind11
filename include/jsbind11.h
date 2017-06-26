#ifndef JSBIND_H_
#define JSBIND_H_

#include <node_api.h>

#include <string>
#include <functional>
#include <tuple>
#include <memory>
#include <mutex>
#include <vector>
#include <list>
#include <iostream>
#include <stdexcept>
#include <cstdint>
#include <unordered_map>

#ifndef JSBIND11_DEBUG
#define JSBIND11_DEBUG_INFO(MESSAGE) {}
#else
#define JSBIND11_DEBUG_INFO(MESSAGE)		\
  std::cout << "JSBIND11: " MESSAGE " | "	\
            << __FILE__ << __LINE__ << std::endl
#endif // JSBIND11_DEBUG

#define JSBIND11_MODULE(NAME, M)					\
  void __ ## NAME ## _init_function(jsbind11::module & M);		\
  jsbind11::detail::module_init __ ## NAME ## _init_delegate() {	\
    static jsbind11::module m(#NAME);					\
    __ ## NAME ## _init_function(m);					\
    return m.init();							\
  }									\
  void __ ## NAME ## _init(napi_env env,				\
			   napi_value exports,				\
			   napi_value module,				\
			   void* priv)					\
  {									\
    __ ## NAME ## _init_delegate()(env, exports, module, priv);		\
  }									\
  NAPI_MODULE(NAME, __ ## NAME ## _init);				\
  void __ ## NAME ## _init_function(jsbind11::module & M)


////////////////////////////////////////////////////////////////////////////////
// Functional Template Magic
////////////////////////////////////////////////////////////////////////////////

namespace jsbind11 {
  namespace detail {
    namespace magic {
      template<int ...> struct seq {};

      template<int N, int ...S>
      struct gens : gens<N-1, N-1, S...> {};
      
      template<int ...S>
      struct gens<0, S...>{
	typedef seq<S...> type;
      };

      template <typename T, typename ...Args>
      class call {
	std::tuple<Args...> args_;
	std::function<T(Args...)> func_;
      public:
	explicit call(T (*f)(Args...),
		      const std::tuple<Args...>& a) : args_(a), func_(f) {}
	explicit call(std::function<T(Args...)> f,
		      const std::tuple<Args...>& a) : args_(a), func_(f) {}
	T operator()() const {
	  return call_with_args(typename gens<sizeof...(Args)>::type());
	}
      private:
	template<int ...S>
	T call_with_args(seq<S...>) const {
	  return func_(std::get<S>(args_)...);
	}
      };
    }  // namespace magic
  } // namespace detail
} // namespace jsbind11

////////////////////////////////////////////////////////////////////////////////
// Function Traits Magic
////////////////////////////////////////////////////////////////////////////////

namespace jsbind11 {
  namespace detail {
    template <typename FunctionType>
    struct function_traits :
      public function_traits<decltype(&FunctionType::operator())> {};
    
    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...) const> {
      enum { arity = sizeof...(Args) };
      typedef std::function<ReturnType(Args...)> function_type;
      typedef ReturnType result_type;
      template <size_t I>
      struct arg {
	typedef typename std::tuple_element<I, std::tuple<Args...>>::type type;
      };
    };
  } // namespace detail
} // namespase jsbind11

////////////////////////////////////////////////////////////////////////////////
// NAPI - helpers
////////////////////////////////////////////////////////////////////////////////

#define JSBIND11_NAPI_(F) \
  napi::api_call<decltype(napi_ ## F)>(napi_ ## F, #F, __FILE__, __LINE__)

namespace jsbind11 {
  namespace napi {
    
    typedef std::function<napi_value(napi_env, napi_callback_info)> callback;

    template <typename F>
    class api_call {
      F* call_;
      const char* call_name_;
      const char* call_file_;
      size_t call_line_;
    public:
      explicit api_call(F call,
			const char* call_name,
			const char* call_file,
			size_t call_line) :
        call_(call),
	call_name_(call_name),
	call_file_(call_file),
	call_line_(call_line) {}
      template <typename ... ArgTypes>
      void operator()(ArgTypes ... args) {
	napi_status status = call_(args...);
#ifdef JSBIND11_DEBUG
#define JSBIND11_LOG_STATUS(S)			\
	case S:					\
	  std::cout << " NAPI_CALL: "		\
		    << "napi_" << call_name_	\
		    << " NAPI_STATUS: "		\
		    << #S			\
		    << " FILE: "		\
		    << call_file_		\
		    << " LINE: "		\
		    << call_line_		\
		    << std::endl << std::flush;	\
	  break
	switch (status) {
	  JSBIND11_LOG_STATUS(napi_ok);
	  JSBIND11_LOG_STATUS(napi_invalid_arg);
	  JSBIND11_LOG_STATUS(napi_object_expected);
	  JSBIND11_LOG_STATUS(napi_string_expected);
	  JSBIND11_LOG_STATUS(napi_name_expected);
	  JSBIND11_LOG_STATUS(napi_function_expected);
	  JSBIND11_LOG_STATUS(napi_number_expected);
	  JSBIND11_LOG_STATUS(napi_boolean_expected);
	  JSBIND11_LOG_STATUS(napi_array_expected);
	  JSBIND11_LOG_STATUS(napi_generic_failure);
	  JSBIND11_LOG_STATUS(napi_pending_exception);
	  JSBIND11_LOG_STATUS(napi_cancelled);
	  JSBIND11_LOG_STATUS(napi_status_last);
	}
#undef JSBIND11_LOG_STATUS
#endif // JSBIND11_DEBUG
	if (status != napi_ok) {
	  throw std::runtime_error("NAPI status isn't ok...");
	}
      }
    };

    ////////////////////////////////////////////////////////////////////////////
    // Type Conversion Utilities
    ////////////////////////////////////////////////////////////////////////////

    template <typename T>
    struct type_cast {
      static T from_napi_value(napi_env env, napi_value v) {}
      static napi_value to_napi_value(napi_env env, const T& v) { return nullptr; }
    };

    template <>
    struct type_cast<double> {
      static double from_napi_value(napi_env env, napi_value v) {
	double result = 0.0;
	
	JSBIND11_NAPI_(get_value_double)(env, v, &result);

	return result;
      }
      static napi_value to_napi_value(napi_env env, double v) {
	napi_value result = nullptr;
	
	JSBIND11_NAPI_(create_number)(env, v, &result);

	return result;
      }
    };
    
    template <>
    struct type_cast<float> {
      static float from_napi_value(napi_env env, napi_value v) {
	return type_cast<double>::from_napi_value(env, v);
      }
      static napi_value to_napi_value(napi_env env, float v) {
	return type_cast<double>::to_napi_value(env, v);
      }
    };

    template <>
    struct type_cast<int8_t> {
      static int8_t from_napi_value(napi_env env, napi_value v) {
	return type_cast<double>::from_napi_value(env, v);
      }
      static napi_value to_napi_value(napi_env env, int8_t v) {
	return type_cast<double>::to_napi_value(env, v);
      }
    };

    template <>
    struct type_cast<int16_t> {
      static int16_t from_napi_value(napi_env env, napi_value v) {
	return type_cast<double>::from_napi_value(env, v);
      }
      static napi_value to_napi_value(napi_env env, int16_t v) {
	return type_cast<double>::to_napi_value(env, v);
      }
    };

    template <>
    struct type_cast<int32_t> {
      static int32_t from_napi_value(napi_env env, napi_value v) {
	return type_cast<double>::from_napi_value(env, v);
      }
      static napi_value to_napi_value(napi_env env, int32_t v) {
	return type_cast<double>::to_napi_value(env, v);
      }
    };

    template <>
    struct type_cast<int64_t> {
      static int64_t from_napi_value(napi_env env, napi_value v) {
	return type_cast<double>::from_napi_value(env, v);
      }
      static napi_value to_napi_value(napi_env env, int64_t v) {
	return type_cast<double>::to_napi_value(env, v);
      }
    };

    template <>
    struct type_cast<uint8_t> {
      static uint8_t from_napi_value(napi_env env, napi_value v) {
	return type_cast<double>::from_napi_value(env, v);
      }
      static napi_value to_napi_value(napi_env env, uint8_t v) {
	return type_cast<double>::to_napi_value(env, v);
      }
    };

    template <>
    struct type_cast<uint16_t> {
      static uint16_t from_napi_value(napi_env env, napi_value v) {
	return type_cast<double>::from_napi_value(env, v);
      }
      static napi_value to_napi_value(napi_env env, uint16_t v) {
	return type_cast<double>::to_napi_value(env, v);
      }
    };

    template <>
    struct type_cast<uint32_t> {
      static uint32_t from_napi_value(napi_env env, napi_value v) {
	return type_cast<double>::from_napi_value(env, v);
      }
      static napi_value to_napi_value(napi_env env, uint32_t v) {
	return type_cast<double>::to_napi_value(env, v);
      }
    };

    template <>
    struct type_cast<uint64_t> {
      static uint64_t from_napi_value(napi_env env, napi_value v) {
	return type_cast<double>::from_napi_value(env, v);
      }
      static napi_value to_napi_value(napi_env env, uint64_t v) {
	return type_cast<double>::to_napi_value(env, v);
      }
    };
    
    template <>
    struct type_cast<void> {
      static napi_value to_napi_value(napi_env env) { return nullptr; }
    };

    template<typename ... ArgTypes>
    struct call_helper {};

    template<typename FirstArgType, typename ... ArgTypes>
    struct call_helper<FirstArgType, ArgTypes...> {
      template <typename T>
      static void parse_args(napi_env env, T& args, napi_value* argv) {
	// Determine which argument to convert
	const size_t N = std::tuple_size<T>::value -
	  std::tuple_size<std::tuple<FirstArgType, ArgTypes...>>::value;
	// Cast Nth argument from NAPI value to FirstArgType
	std::get<N>(args) = type_cast<FirstArgType>::from_napi_value(env, argv[N]);
	// Convert the rest of arguments
	call_helper<ArgTypes...>::parse_args(env, args, argv);
      }
    };

    template<>
    struct call_helper<> {
      template <typename T>
      static void parse_args(napi_env env, T& args, napi_value* argv) {}
    };

    template <typename RetType, typename ... ArgTypes>
    struct call_operator_helper {
      static napi_value call(napi_env env,
			     const std::function<RetType(ArgTypes...)>& f,
			     const std::tuple<ArgTypes...>& f_args) {
	auto value = detail::magic::call<RetType, ArgTypes...>(f, f_args)();

	return type_cast<RetType>::to_napi_value(env, value);
      }
    };

    template <typename ... ArgTypes>
    struct call_operator_helper<void, ArgTypes...> {
      static napi_value call(napi_env env,
			     const std::function<void(ArgTypes...)>& f,
			     const std::tuple<ArgTypes...>& f_args) {
	detail::magic::call<void, ArgTypes...>(f, f_args)();

	return type_cast<void>::to_napi_value(env);
      }
    };

    template <typename ... ArgTypes>
    class call {
      // Note: it's unlikely that anyone would be using function call
      //       signatures with 64+ call parameters, so this should do...
      static const size_t max_argc = 64;
      std::tuple<ArgTypes...> args_;
      napi_value argv_[max_argc + 1];
      napi_env env_;
      napi_callback_info info_;;
    public:
      call(napi_env env, napi_callback_info info) :
        env_(env), info_(info) {	  
	// Make sure function type isn't insane (e.g. there's less than 64 args) 
	static_assert(max_argc >= std::tuple_size<std::tuple<ArgTypes...>>::value,
		      "Maximum number of jsbind11 function arguments is 256");

	// Construct call args
	size_t argc = max_argc;

	// Get callback info via NAPI
	JSBIND11_NAPI_(get_cb_info)(env,
				    info,
				    &argc,
				    &argv_[0],
				    &argv_[max_argc],
				    nullptr);
	  
	if (argc != std::tuple_size<std::tuple<ArgTypes...>>::value) {
	  throw std::runtime_error("Received wrong number of function arguments");
	}
	  
	// Recursively parse positional arguments...
	call_helper<ArgTypes...>::parse_args(env, args_, argv_);
      }
      
      template <typename RetType>
      napi_value operator()(const std::function<RetType(ArgTypes...)>& f) {
	return call_operator_helper<RetType, ArgTypes...>::call(env_, f, args_);
      }
      
      template <typename RetType, typename Type>
      napi_value operator()(const std::function<RetType(Type&, ArgTypes...)>& f) {
	Type* this_ = nullptr;
	
	JSBIND11_NAPI_(unwrap)(env_, argv_[max_argc], (void**) &this_);

	auto f_lambda = [f, this_](ArgTypes ... args) -> RetType {
	  return f(*this_,  args...);
	};

	auto f_ = std::function<RetType(ArgTypes...)>(f_lambda);

	return call_operator_helper<RetType, ArgTypes...>::call(env_, f_, args_);
      }

      template <typename Type>
      napi_value constructor() {
	auto this_ = detail::magic::
 	  call<Type*, ArgTypes...>(create<Type>, args_)();
	napi_value this_js = argv_[max_argc];

	JSBIND11_NAPI_(wrap)(env_,
			     this_js, // JS 'this'
			     this_,
			     destroy<Type>, // finilize callback
			     nullptr, // finilize hint
			     nullptr); // reference
	
	return this_js;     
      }
    private:
      template <typename Type>
      static Type* create(ArgTypes ... args) {
	return new Type(args...);
      }
    
      template <typename Type>
      static void destroy(napi_env env, void* this_, void*) {
	delete (Type*) this_;
      }
    };
  }  // namespace napi
}  // namespace jsbind11

////////////////////////////////////////////////////////////////////////////////
// Table ADT
////////////////////////////////////////////////////////////////////////////////

namespace jsbind11 {  
  namespace detail {
    template <typename T>
    class table : protected std::vector<T> {
    public:
      T& add_item(const T& item) {
	static std::mutex mutex_;
	std::lock_guard<std::mutex> lock(mutex_);
	this->push_back(item);
	return this->back();
      }
      
      using std::vector<T>::begin;
      using std::vector<T>::end;
      using std::vector<T>::size;
    };
  } // namespace detail
} // namespace jsbind11

////////////////////////////////////////////////////////////////////////////////
// jsbind11::detail::retain
////////////////////////////////////////////////////////////////////////////////

namespace jsbind11 {
  namespace detail { 
    template <typename T>
    T& retain(T value) {
      static std::mutex mutex_;
      static std::list<T> values;
      
      std::lock_guard<std::mutex> lock(mutex_);
      
      values.push_back(value);
      
      return values.back();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// jsbind11::detail::class_
////////////////////////////////////////////////////////////////////////////////

namespace jsbind11 {
  namespace detail {
    class class_ {
      const std::string& name_;
      std::vector<napi_property_descriptor> pd_vec_;
      static std::unordered_multimap<std::string, napi::callback> cb_map_;
      static std::unordered_map<std::string, napi_value> value_map_;
    public:
      explicit class_(const std::string& name) : name_(retain(name)) {}
    private:
      void add_callback(std::string cb_name,
			std::string cb_type,
			napi::callback& cb) {
	cb_map_.emplace(name_ + ":" + cb_name + ":" + cb_type, cb);	
      }
      static napi_value dispatch_callback(napi_env env,
					  napi_callback_info info,
					  std::string suffix = "") {
	char* cb_name_c_str = nullptr;

	JSBIND11_NAPI_(get_cb_info)(env,
				    info,
				    nullptr,
				    nullptr,
				    nullptr,
				    (void**) &cb_name_c_str);

	std::string cb_name = std::string(cb_name_c_str) + suffix;

	for (auto it = cb_map_.find(cb_name); it != cb_map_.end(); it++) {
	  const napi::callback& cb = it->second;

	  try {
	    return cb(env, info);
	  } catch (...) {
	    continue;
	  }
	}

	throw std::runtime_error("Callback failed");
      }

      static napi_value constructor_cb(napi_env env, napi_callback_info info) {
	return dispatch_callback(env, info);
      }      
      static napi_value method_cb(napi_env env, napi_callback_info info) {
	return dispatch_callback(env, info, ":method");
      }
      static napi_value getter_cb(napi_env env, napi_callback_info info) {
	return dispatch_callback(env, info, ":getter");
      }
      static napi_value setter_cb(napi_env env, napi_callback_info info) {
	return dispatch_callback(env, info, ":setter");
      }
    public:
      void add_constructor(napi::callback cb) {
	cb_map_.insert(std::make_pair(name_, cb));
      }

      void add_method(std::string name, napi::callback cb) {
	add_callback(name, "method", cb);

	napi_property_descriptor pd = {
	  retain(name).c_str(), // utf8name
	  nullptr, // name
	  method_cb, // method
	  nullptr, // getter
	  nullptr, // setter
	  nullptr, // value
	  napi_default, // attributes
	  (void*) retain(name_ + ":" + name).c_str() // data
	};

	pd_vec_.push_back(pd);
      }
      void add_static_method(std::string name, napi::callback cb) {
	add_callback(name, "static_method", cb);

	// TODO: implement this...
      }
      void add_getter(std::string name, napi::callback cb) {
	add_callback(name, "getter", cb);

	napi_property_descriptor pd = {
	  retain(name).c_str(), // utf8name
	  nullptr, // name
	  nullptr, // method
	  getter_cb, // getter
	  nullptr, // setter
	  nullptr, // value
	  napi_default, // attributes
	  (void*) retain(name_ + ":" + name).c_str() // data
	};

	pd_vec_.push_back(pd);
      }
      void add_setter(std::string name, napi::callback cb) {
	add_callback(name, "setter", cb);

	napi_property_descriptor pd = {
	  retain(name).c_str(), // utf8name
	  nullptr, // name
	  nullptr, // method
	  nullptr, // getter
	  setter_cb, // setter
	  nullptr, // value
	  napi_default, // attributes
	  (void*) retain(name_ + ":" + name).c_str() // data
	};

	pd_vec_.push_back(pd);
      }
      const char* get_name() const {
	return name_.c_str();
      }
      napi_value get_napi_value(napi_env env) const {
	try {
	  return value_map_.at(name_);
	} catch (const std::out_of_range& e) {
	  napi_value value = nullptr;

	  JSBIND11_NAPI_(define_class)(env,
				       this->name_.c_str(),
				       this->constructor_cb,
				       (void*) this->name_.c_str(),
				       this->pd_vec_.size(),
				       this->pd_vec_.data(),
				       &value);

	  value_map_.insert(std::make_pair(name_, value));

	  return value;
	}	
      }
    };
  }
}

////////////////////////////////////////////////////////////////////////////////
// jsbind11::detail::class_builder
////////////////////////////////////////////////////////////////////////////////

namespace jsbind11 {
  namespace detail {
    template <typename ClassType, typename RetType, typename ... ArgTypes>
    struct class_method {
      static napi::callback wrap(std::function<RetType(ClassType&, ArgTypes...)> m) { 
	return [=](napi_env env, napi_callback_info info) {
	  return napi::call<ArgTypes...>(env, info)(m);
	};
      }
    };
    
    template <typename Type>
    class class_builder {
      class_& c_;
    public:
      class_builder(class_& c) : c_(c) {}

      template <typename ... ArgTypes>
      class_builder& constructor() {
	napi::callback cb = [=](napi_env env, napi_callback_info info) {
	  return napi::call<ArgTypes...>(env, info).template constructor<Type>();
	};

	c_.add_constructor(cb);
	return *this;
      }

      template <typename RetType, typename ... ArgTypes>
      class_builder& method(const std::string& m_name,
			    RetType (Type::*m)(ArgTypes...) const) {
	return method_(m_name, std::function<RetType(Type&, ArgTypes...)>(std::mem_fn(m)));
      }

      template <typename RetType, typename ... ArgTypes>
      class_builder& method(const std::string& m_name, RetType (Type::*m)(ArgTypes...)) {
	return method_(m_name, std::function<RetType(Type&, ArgTypes...)>(std::mem_fn(m)));
      }

      template <typename LambdaType>
      class_builder& method(const std::string& m_name, LambdaType m_lambda) {
	// Infer function type from lambda object
	typedef function_traits<decltype(m_lambda)> f_traits;
	// Create function wrapper
	typename f_traits::function_type m = m_lambda;
	// Delegate to other function method
	return method_(m_name, m); 
      }

      template <typename RetType, typename ... ArgTypes>
      class_builder& method(const std::string& m_name,
			    std::function<RetType(Type&, ArgTypes...)> m) {
	return method_(m_name, m);
      }

    private:
      template <typename RetType, typename ... ArgTypes>
      class_builder& method_(const std::string& m_name,
			     std::function<RetType(const Type&, ArgTypes...)> m) {
	c_.add_method(m_name, class_method<Type, RetType, ArgTypes...>::wrap(m));
	return *this;
      }

      template <typename RetType, typename ... ArgTypes>
      class_builder& method_(const std::string& m_name,
			     std::function<RetType(Type&, ArgTypes...)> m) {
	c_.add_method(m_name, class_method<Type, RetType, ArgTypes...>::wrap(m));
	return *this;
      }
    };
  }
}

////////////////////////////////////////////////////////////////////////////////
// jsbind11::detail::function
////////////////////////////////////////////////////////////////////////////////

namespace jsbind11 {
  namespace detail {
    class function {
      const std::string& name_;
      const napi::callback& cb_;
    public:
      function(const std::string& name, const napi::callback& cb) :
	name_(retain<std::string>(name)),
	cb_(retain<napi::callback>(cb)) {}
      const char* get_name() const {
	return name_.c_str();
      }
      napi_value get_napi_value(napi_env env) const {
	napi_value f = nullptr;
	
	JSBIND11_NAPI_(create_function)(env,
					this->name_.c_str(), // name
					this->cb, // callback function
					(void*) &this->cb_, // callback data
					&f);
	return f;
      }
    private:
      static napi_value cb(napi_env env, napi_callback_info info) {
	napi::callback* c;

	// Get callback info via NAPI
	JSBIND11_NAPI_(get_cb_info)(env,
				    info,
				    nullptr,
				    nullptr,
				    nullptr,
				    (void**) &c);

	return (*c)(env, info);
      }
    };
  }
}

////////////////////////////////////////////////////////////////////////////////
// jsbind11::detail::function_builder
////////////////////////////////////////////////////////////////////////////////

namespace jsbind11 {
  namespace detail {
  }
}

////////////////////////////////////////////////////////////////////////////////
// jdbind11::module
////////////////////////////////////////////////////////////////////////////////

namespace jsbind11 {
  namespace detail {
    class module_desc {
      table<class_> class_table_;
      table<function> function_table_;
    public:
      class_& add_class(class_ c) {
	return class_table_.add_item(c);
      }
      function& add_function(function f) {
	return function_table_.add_item(f);
      }
      const table<class_>& get_class_table() const {
	return class_table_;
      }
      const table<function>& get_function_table() const {
	return function_table_;
      }
    };

    class module_init {
      std::shared_ptr<module_desc> m_;
    public:
      explicit module_init(std::shared_ptr<module_desc> m) : m_(m) {}

      void operator()(napi_env env,
		      napi_value exports,
		      napi_value module,
		      void* p) {

	// Export each class...
	for (const auto& c : m_->get_class_table()) {
	  JSBIND11_NAPI_(set_named_property)(env,
					     exports,
					     c.get_name(),
					     c.get_napi_value(env));
	}

	// Export each function...
	for (const auto& f : m_->get_function_table()) {
	  JSBIND11_NAPI_(set_named_property)(env,
					     exports,
					     f.get_name(),
					     f.get_napi_value(env));
	}
      }
    };
  }

  class module {
    std::shared_ptr<detail::module_desc> desc_;
    std::string name_;
  public:
    explicit module(const std::string& name) :
      desc_(std::make_shared<detail::module_desc>()),
      name_(name) {}

    template <typename ClassType>
    detail::class_builder<ClassType> class_(const std::string& c_name) {
      // Add class to module descriptor and get its reference
      detail::class_& c = desc_->add_class(detail::class_(c_name));
      // Create class builder for ClassType and return it
      return detail::class_builder<ClassType>(c);
    }
    
    template <typename RetType, typename ... ArgTypes>
    detail::function& function(const std::string& f_name, RetType (*f_ptr)(ArgTypes...)) {
      // Create function wrapper object
      std::function<RetType(ArgTypes...)> f(f_ptr);
      // Delegate to other function method
      return function(f_name, f);
    }

    template <typename LambdaType>
    detail::function& function(const std::string& f_name, LambdaType f_lambda) {
      using namespace detail;
      // Infer function type from lambda object
      typedef function_traits<decltype(f_lambda)> f_traits;
      // Create function wrapper
      typename f_traits::function_type f = f_lambda;
      // Delegate to other function method
      return function(f_name, f);
    }
    
    template <typename RetType, typename ... ArgTypes>
    detail::function& function(const std::string& f_name,
			       std::function<RetType(ArgTypes...)> f) {
      detail::function f_(f_name, [=](napi_env env, napi_callback_info info) {
	  return napi::call<ArgTypes...>(env, info)(f);
	});
      return desc_->add_function(f_);
    }
    
    detail::module_init init() const {
      // Create init callback object
      detail::module_init init_(this->desc_);
      // Done!
      return init_;
    }
  };
}

#undef JSBIND11_NAPI_
#endif // JSBIND_H_

