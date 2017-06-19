#ifndef JSBIND_H_
#define JSBIND_H_

#include <node_api.h>

#include <string>
#include <functional>
#include <tuple>
#include <memory>
#include <mutex>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdint>

#define JSBIND11_DEBUG

#ifndef JSBIND11_DEBUG
#define JSBIND11_DEBUG false
#define JSBIND11_CHECK_STATUS(S) \
  jsbind11::internal::napi::check_status(S)
#else
#undef JSBIND11_DEBUG
#define JSBIND11_DEBUG true
#define JSBIND11_CHECK_STATUS(S) \
  jsbind11::internal::napi::check_status(S, __FILE__, __LINE__)
#endif // JSBIND11_DEBUG

#define JSBIND11_PLUGIN(NAME)						\
  jsbind11::internal::module_init __ ## NAME ## _init_delegate();	\
  void __ ## NAME ## _init(napi_env env,				\
			   napi_value exports,				\
			   napi_value module,				\
			   void* p)					\
  {									\
    __ ## NAME ## _init_delegate()(env, exports, module, p);		\
  }									\
  NAPI_MODULE(NAME, __ ## NAME ## _init);			        \
  jsbind11::internal::module_init __ ## NAME ## _init_delegate() 

#define JSBIND11_MODULE(NAME, M)					\
  void __ ## NAME ## _init_function(jsbind11::module & M);		\
  jsbind11::internal::module_init __ ## NAME ## _init_delegate() {	\
    jsbind11::module m(#NAME);						\
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

namespace jsbind11 {

  using namespace std;

  namespace internal {
    namespace napi {
      inline void check_status(napi_status status) {
	if (status != napi_ok) {
	  throw runtime_error("NAPI status isn't ok...");
	}
      }

      inline void check_status(napi_status status, const char* file, size_t line) {
	switch (status) {
	  case napi_ok:
	    cout << "NAPI_STATUS: napi_ok FILE: " << file << " LINE: " << line << endl;
	    break;
	  case napi_invalid_arg:
	    cout << "NAPI_STATUS: napi_invalid_arg FILE: " << file << " LINE: " << line << endl;
	    break;
	  case napi_object_expected:
	    cout << "NAPI_STATUS: napi_object_expected FILE: " << file << " LINE: " << line << endl;
	    break;
	  case napi_string_expected:
	    cout << "NAPI_STATUS: napi_string_expected FILE: " << file << " LINE: " << line << endl;
	    break;
	  case napi_name_expected:
	    cout << "NAPI_STATUS: napi_name_expected FILE: " << file << " LINE: " << line << endl;
	    break;
	  case napi_function_expected:
	    cout << "NAPI_STATUS: napi_function_expected FILE: " << file << " LINE: " << line << endl;
	    break;
	  case napi_number_expected:
	    cout << "NAPI_STATUS: napi_number_expected FILE: " << file << " LINE: " << line << endl;
	    break;
	  case napi_boolean_expected:
	    cout << "NAPI_STATUS: napi_boolean_expected FILE: " << file << " LINE: " << line << endl;
	    break;
	  case napi_array_expected:
	    cout << "NAPI_STATUS: napi_array_expected FILE: " << file << " LINE: " << line << endl;
	    break;
	  case napi_generic_failure:
	    cout << "NAPI_STATUS: napi_generic_failure FILE: " << file << " LINE: " << line << endl;
	    break;
	  case napi_pending_exception:
	    cout << "NAPI_STATUS: napi_pending_exception FILE: " << file << " LINE: " << line << endl;
	    break;
	  case napi_cancelled:
	    cout << "NAPI_STATUS: napi_cancelled FILE: " << file << " LINE: " << line << endl;
	    break;
	  case napi_status_last:
	    cout << "NAPI_STATUS: napi_status_last FILE: " << file << " LINE: " << line << endl;
	    break;
	}

	if (status != napi_ok) {
	  throw runtime_error("NAPI status isn't ok...");
	}
      }
    }  // namespace napi
  }  // namespace internal
  
  template<typename S, typename T>
  struct type {
    static inline void cast(napi_env env, const S& s, T& t) { t = (S) s; }
  };

  /* TODO: figure out issues related to napi_create_boolean

  template<>
  struct type<napi_value, bool> {
    static void cast(napi_env env, const napi_value& source, bool& target) {
      internal::napi::
        check_status(napi_get_value_bool(env, source, &target));
    }
  };

  template<>
  struct type<bool, napi_value> {
    static void cast(napi_env env, const bool& source, napi_value& target) {
      internal::napi::
        check_status(napi_create_boolean(env, source, &target));
    }
  };
  
  */

  template <typename T>
  class type<napi_value, T> {
    static vector<function<void(napi_env, const napi_value&, T&)>> cast_function_list_;
  public:
    static void cast(napi_env env, const napi_value& source, T& target) {
      for (auto cast_function : cast_function_list_) {
	try {
	  cast_function(env, source, target);
	} catch (...) {
	  continue;
	}
	
	return;
      }
      
      throw runtime_error("Couldn't constuct object...");
    }


    // TODO: make this method private, maybe, and use friend class?
    static void add_cast_function(function<void(napi_env, const napi_value&, T&)> f) {
      cast_function_list_.push_back(f);
    }
  };

  template <typename T>
  vector<function<void(napi_env, const napi_value&, T&)>>
  type<napi_value, T>::cast_function_list_{};

  template <typename T>
  class type<T, napi_value> {
    static vector<function<void(napi_env, const T&, napi_value&)>> cast_function_list_;
  public:
    static void cast(napi_env env, const T& source, napi_value& target) {
      for (auto cast_function : cast_function_list_) {
	try {
	  cast_function(env, source, target);
	} catch (...) {
	  continue;
	}

	return;
      }

      throw runtime_error("Couldn't constuct object...");
    }

    static void add_cast_function(function<void(napi_env, const T&, napi_value&)> f) {
      cast_function_list_.push_back(f);
    }
  };

  template <typename T>
  vector<function<void(napi_env, const T&, napi_value&)>>
  type<T, napi_value>::cast_function_list_{};
  
  template<>
  struct type<napi_value, double> {
    static void cast(napi_env env, const napi_value& source, double& target) {
      auto status = napi_get_value_double(env, source, &target);
      
      JSBIND11_CHECK_STATUS(status);
    }
  };

  template<>
  struct type<double, napi_value> {
    static void cast(napi_env env, const double& source, napi_value& target) {
      auto status = napi_create_number(env, source, &target);
      
      JSBIND11_CHECK_STATUS(status);
    }
  };
  
  // TODO: add partial specializations of type to convert between JS and C++ types

  namespace internal {
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
    
    namespace napi {
      typedef function<napi_value(napi_env,napi_callback_info)> callback;

      class callback_table {
	static mutex mutex_;
	vector<callback> napi_callback_list_;
      public:	
	size_t add_callback(callback cb) {
	  lock_guard<mutex> lock(mutex_);

	  size_t callback_id = napi_callback_list_.size();
	  
	  napi_callback_list_.push_back(cb);

	  return callback_id;
	}
	
	const callback& get_callback(size_t cb_id) const {
	  return napi_callback_list_.at(cb_id);
	}

	const vector<callback>& get_callback_list() const {
	  return napi_callback_list_;
	}

	static napi_value callback_dispatch(napi_env env,
					    napi_callback_info info) {
	  // NAPI call status 
	  napi_status status;

	  // Construct call args
	  size_t argc = 0;
	  napi_value argv[1] = {0};
	  napi_value this_arg;
	  void* data;
	
	  // Get callback data
	  status = napi_get_cb_info(env, info, &argc, argv, &this_arg, &data);
	  // Make sure all is well!
	  JSBIND11_CHECK_STATUS(status); 
	
	  // Data must be valid.
	  if (data == nullptr) {
	    // TODO: raise runtime error
	  }

	  // Dispatch to appropriate callback
	  return (*(callback*)(data))(env, info); 
	}
      };

      mutex callback_table::mutex_;
      
      class property_descriptor_table {
	static mutex mutex_;
	vector<callback> napi_callback_list_;
	vector<napi_property_descriptor> napi_property_descriptor_list_;

	struct callback_selector {
	  callback* method;
	  callback* getter;
	  callback* setter;
	};

	vector<callback_selector> napi_callback_selector_list_;
	
	static callback_selector* get_callback_selector(napi_env env, napi_callback_info info) {
	  // NAPI call status 
	  napi_status status;

	  // Construct call args
	  size_t argc = 0;
	  napi_value argv[1] = {0};
	  napi_value this_arg;
	  callback_selector* cs;
	
	  // Get callback data
	  status = napi_get_cb_info(env, info, &argc, argv, &this_arg, (void**) &cs);
	  // Make sure all is well!
	  JSBIND11_CHECK_STATUS(status); 
	
	  // Callback selector must be valid.
	  if (cs == nullptr) {
	    // TODO: raise runtime error
	  }

	  return cs;
	}
	  
	static napi_value method_callback_dispatch(napi_env env,
						   napi_callback_info info) {
	  return (*(get_callback_selector(env, info)->method))(env, info);
 	}

	static napi_value getter_callback_dispatch(napi_env env,
						   napi_callback_info info) {
	  return (*(get_callback_selector(env, info)->getter))(env, info);
	}

	static napi_value setter_callback_dispatch(napi_env env,
						   napi_callback_info info) {
	  return (*(get_callback_selector(env, info)->setter))(env, info);
	}

      public:
	size_t add_read_only_property_descriptor(const string& name, callback getter) {
	  lock_guard<mutex> lock(mutex_);

	  napi_callback_list_.push_back(getter);

	  callback_selector cs = {
	    nullptr, // method callback pointer
	    &napi_callback_list_.back(), // getter callback pointer
	    nullptr  // setter callback pointer
	  };
	  
	  napi_callback_selector_list_.push_back(cs);

	  napi_property_descriptor pd = {
	    name.c_str(), // utf8name
	    nullptr, // name
	    nullptr, // method
	    &getter_callback_dispatch, // getter
	    nullptr, // setter
	    nullptr, // value
	    napi_default, // attributes
	    &napi_callback_selector_list_.back() // data
	  };
	  
	  napi_property_descriptor_list_.push_back(pd);
	  
	  return napi_property_descriptor_list_.size();
	}

	size_t add_write_only_property_descriptor(const string& name, callback setter) {
	  lock_guard<mutex> lock(mutex_);

	  napi_callback_list_.push_back(setter);

	  callback_selector cs = {
	    nullptr, // method callback pointer
	    nullptr, // getter callback pointer
	    &napi_callback_list_.back()  // setter callback pointer
	  };
	  
	  napi_callback_selector_list_.push_back(cs);

	  napi_property_descriptor pd = {
	    name.c_str(), // utf8name
	    nullptr, // name
	    nullptr, // method
	    nullptr, // getter
	    &setter_callback_dispatch, // setter
	    nullptr, // value
	    napi_default, // attributes
	    &napi_callback_selector_list_.back() // data
	  };
	  
	  napi_property_descriptor_list_.push_back(pd);
	  
	  return napi_property_descriptor_list_.size();
	}

	size_t add_read_write_property_descriptor(const string& name, callback getter, callback setter) {
 	  lock_guard<mutex> lock(mutex_);

	  napi_callback_list_.push_back(getter);
	  auto getter_ptr = &napi_callback_list_.back();
	  napi_callback_list_.push_back(setter);
	  auto setter_ptr = &napi_callback_list_.back();
	  
	  callback_selector cs = {
	    nullptr, // method callback pointer
	    getter_ptr, // getter callback pointer
	    setter_ptr  // setter callback pointer
	  };
	  
	  napi_callback_selector_list_.push_back(cs);

	  napi_property_descriptor pd = {
	    name.c_str(), // utf8name
	    nullptr, // name
	    nullptr, // method
	    &getter_callback_dispatch, // getter
	    &setter_callback_dispatch, // setter
	    nullptr, // value
	    napi_default, // attributes
	    &napi_callback_selector_list_.back() // data
	  };
	  
	  napi_property_descriptor_list_.push_back(pd);
	  
	  return napi_property_descriptor_list_.size();
	}

	size_t add_value_property_descriptor(const string& name, napi_value value) {
	  lock_guard<mutex> lock(mutex_);
	  
	  callback_selector cs = {
	    nullptr, // method callback pointer
	    nullptr, // getter callback pointer
	    nullptr  // setter callback pointer
	  };
	  
	  napi_callback_selector_list_.push_back(cs);

	  napi_property_descriptor pd = {
	    name.c_str(), // utf8name
	    nullptr, // name
	    nullptr, // method
	    nullptr, // getter
	    nullptr, // setter
	    value, // value
	    napi_default, // attributes
	    nullptr // data
	  };
	  
	  napi_property_descriptor_list_.push_back(pd);
	  
	  return napi_property_descriptor_list_.size();
	}
							 
	size_t add_method_property_descriptor(const string& name, callback method) {
 	  lock_guard<mutex> lock(mutex_);

	  napi_callback_list_.push_back(method);
	  
	  callback_selector cs = {
	    &napi_callback_list_.back(), // method callback pointer
	    nullptr, // getter callback pointer
	    nullptr  // setter callback pointer
	  };
	  
	  napi_callback_selector_list_.push_back(cs);

	  napi_property_descriptor pd = {
	    name.c_str(), // utf8name
	    nullptr, // name
	    method_callback_dispatch, // method
	    nullptr, // getter
	    nullptr, // setter
	    nullptr, // value
	    napi_default, // attributes
	    &napi_callback_selector_list_.back() // data
	  };
	  
	  napi_property_descriptor_list_.push_back(pd);
	  
	  return napi_property_descriptor_list_.size();
	}

	const napi_property_descriptor& get_property_descriptor(size_t id) const {
	  return napi_property_descriptor_list_.at(id);
	}
	
	const vector<napi_property_descriptor>& get_property_descriptor_list() const {
	  return napi_property_descriptor_list_;
	}
      };

      mutex property_descriptor_table::mutex_;
      
      class value {
      public:
	napi_value get_napi_value(napi_env env) {
	  return this->get_napi_value_(env);
	}
      private:
	virtual napi_value get_napi_value_(napi_env env) = 0;	  
      };
    }  // namespace napi

    namespace magic {
      template<typename ... ArgTypes>
      struct call_args_helper {};

      template<typename FirstArgType, typename ... ArgTypes>
      struct call_args_helper<FirstArgType, ArgTypes...> {
	template <typename T>
	static void parse_args(napi_env env, T& args, napi_value* argv) {
	  // Determine which argument to convert
	  const size_t N = std::tuple_size<T>::value -
	    std::tuple_size<std::tuple<FirstArgType, ArgTypes...>>::value;
	  // Cast Nth argument from NAPI value to FirstArgType
	  type<napi_value, FirstArgType>::cast(env, argv[N], std::get<N>(args));
	  // Convert the rest of arguments
	  call_args_helper<ArgTypes...>::parse_args(env, args, argv);
	}
      };

      template<>
      struct call_args_helper<> {
	template <typename T>
	static void parse_args(napi_env env, T& args, napi_value* argv) {}
      };

      template <typename ... ArgTypes>
      class call_args {
	// Note: it's unlikely that anyone would be using function call
	//       signatures with 256+ call parameters, so this should do...
	static const size_t max_argc = 256;
	
	tuple<ArgTypes...> args_;
	napi_value argv_[max_argc + 1];
      public:
	call_args(napi_env env,
		  napi_callback_info info) {
	  
	  // NAPI call status
	  napi_status status;
	  
	  // Make sure function type isn't insane 
	  static_assert(max_argc >= tuple_size<tuple<ArgTypes...>>::value,
			"Maximum number of jsbind11 function arguments is 256");

	  // Construct call args
	  size_t argc = max_argc;
	  
	  // Get callback info via NAPI
	  status = napi_get_cb_info(env, info, &argc, &argv_[0], &argv_[max_argc], nullptr);
	  
	  // Make sure all is well!
	  JSBIND11_CHECK_STATUS(status); 
	  
	  if (argc != tuple_size<tuple<ArgTypes...>>::value) {
	    cout << "Wrong callback: expected " << tuple_size<tuple<ArgTypes...>>::value << ", got " << argc << " args...\n" << std::flush;
	    // TODO: provide more meaningfull error message
	    throw runtime_error("Received wrong number of function arguments");
	  }
	  
	  // Recursively parse positional arguments...
	  call_args_helper<ArgTypes...>::parse_args(env, args_, argv_);
	}
	
	const tuple<ArgTypes...>& operator()() {
	  return args_;
	}

	const tuple<ArgTypes...>& get_args() const {
	  return args_;
	}

	void* get_this_arg(napi_env env) const {
	  // Native 'this' argument
	  void* this_ = nullptr;;
	  // Unwrap 'this' argument from JS object...
	  auto status = napi_unwrap(env, argv_[max_argc], &this_);
	  // Make sure all is well!
	  JSBIND11_CHECK_STATUS(status);
	  // Done!
	  return this_;
	}

	const napi_value& get_this_arg_napi_value() const {
	  return argv_[max_argc];
	}
      };

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
      
    }  // namespace magic

    class class_ : public napi::value {
      static napi::property_descriptor_table property_descriptor_table_;
      static vector<napi::callback_table> constructor_table_list_;
      size_t constructor_table_id_;
      string name_;
    public:
      class_(const string& name) : name_(name) {
	constructor_table_id_ = constructor_table_list_.size();
	constructor_table_list_.push_back(napi::callback_table());
      }

      const string& get_name() const {
	return name_;
      }

      void add_constructor(napi::callback cb) {
	constructor_table_list_.at(constructor_table_id_).add_callback(cb);
      }
      
      void add_method(const string& name, napi::callback cb) {
	property_descriptor_table_.add_method_property_descriptor(name, cb);
      }
      
    private:
      static napi_value construtor_callback(napi_env env, napi_callback_info info) {
	// NAPI call status 
	napi_status status;

	// Construct call args
	size_t argc = 0;
	napi_value argv[1] = {0};
	napi_value this_arg;
	napi::callback_table* cb_table = nullptr;
	
	// Get callback data
	status = napi_get_cb_info(env,
				  info,
				  &argc,
				  argv,
				  &this_arg,
				  (void**) &cb_table);
	// Make sure all is well!
	JSBIND11_CHECK_STATUS(status); 

	// Callback selector must be valid.
	if (cb_table == nullptr) {
	  cout << "Invalid callback table";
	  throw runtime_error("Invalid callback table");
	}

	for (auto cb : cb_table->get_callback_list()) {	  
	  try {
	    cout << "Trying some constructor...\n";
	    return cb(env, info);
	  } catch (...) {
	    continue;
	  }
	}

	cout << "About to die\n";
	
	throw runtime_error("Couldn't contruct object...");
      }
      
      napi_value get_napi_value_(napi_env env) {
	// NAPI call status
	napi_status status;

	// Construct call args
	const char* c_name = name_.c_str();
	napi_callback c_constructor = &construtor_callback;
	void* c_data = &constructor_table_list_[constructor_table_id_];
	
	const size_t c_property_descriptor_count = \
	  property_descriptor_table_.get_property_descriptor_list().size();
	const napi_property_descriptor* c_property_descriptor_array = \
	  property_descriptor_table_.get_property_descriptor_list().data();
	napi_value c_napi_value = nullptr;

	// Create NAPI value for this class
	status = napi_define_class(env,
				   c_name,
				   c_constructor,
				   c_data,
				   c_property_descriptor_count,
				   c_property_descriptor_array,
				   &c_napi_value);
	
	// Make sure all is well!
	JSBIND11_CHECK_STATUS(status);

	// Return created NAPI value
	return c_napi_value;
      }
    };

    napi::property_descriptor_table class_::property_descriptor_table_ = {};
    vector<napi::callback_table> class_::constructor_table_list_ = {};

    template <typename ClassType, typename RetType, typename ... ArgTypes>
    struct class_builder_helper {
      static napi::callback wrap_method(std::function<RetType(ClassType&, ArgTypes...)> m) { 
	auto cb = [=](napi_env env, napi_callback_info info) {
	  // Parse function call aguments from napi callback info
	  magic::call_args<ArgTypes...> m_call_args(env, info);
	  auto m_args = m_call_args.get_args();
	  auto m_this_arg = (ClassType*) m_call_args.get_this_arg(env);
	  auto m_this_and_args = tuple_cat(make_tuple(*m_this_arg), m_args);
	  // Forward call arguments to native C++ function
	  auto m_result = magic::call<RetType, ArgTypes...>(m, m_this_and_args)();
	  // Construct NAPI value from return value of native C++ function
	  napi_value m_result_napi_value = nullptr; // NAPI equivalent of m_result
	  // Cast C++ value to NAPI value
	  type<RetType, napi_value>::cast(env, m_result, m_result_napi_value);
	  // Return NAPI value
	  return m_result_napi_value;
	};
	return napi::callback(cb);
      }
    };

    template <typename ClassType, typename RetType>
    struct class_builder_helper<ClassType, RetType> {
      static napi::callback wrap_method(std::function<RetType(ClassType&)> m) {
	auto cb = [=](napi_env env, napi_callback_info info) {
	  // Parse function call aguments from napi callback info
	  magic::call_args<> m_call_args(env, info);
	  // Get unwrapped native 'this' argument
	  auto m_this_arg = (ClassType*) m_call_args.get_this_arg(env);
	  // Forward call arguments to native C++ function
	  auto m_result = m(*m_this_arg);
	  // Construct NAPI value from return value of native C++ function
	  napi_value m_result_napi_value = nullptr; // NAPI equivalent of m_result
	  // Cast C++ value to NAPI value
	  type<RetType, napi_value>::cast(env, m_result, m_result_napi_value);
	  // Return NAPI value
	  return m_result_napi_value;
	};
	return napi::callback(cb);	
      }
    };
    
    template <typename ClassType, typename ... ArgTypes>
    struct class_builder_helper<ClassType, void, ArgTypes...> {
      static napi::callback wrap_method(std::function<void(ClassType&, ArgTypes...)> m) {
	auto cb = [=](napi_env env, napi_callback_info info) {
	  // Parse function call aguments from napi callback info
	  magic::call_args<ArgTypes...> m_call_args(env, info);
	  auto m_args = m_call_args.get_args();
	  auto m_this_arg = (ClassType*) m_call_args.get_this_arg(env);
	  auto m_this_and_args = tuple_cat(make_tuple(*m_this_arg), m_args);
	  // Forward call arguments to native C++ function
	  magic::call<void, ArgTypes...>(m, m_this_and_args)();
	  // Construct NAPI value from return value of native C++ function
	  napi_value m_result_napi_value = nullptr; // NAPI equivalent of m_result
	  // Cast C++ value to NAPI value
	  auto status = napi_get_undefined(env, &m_result_napi_value);
	  // Make sure all is well!
	  JSBIND11_CHECK_STATUS(status); 
	  // Return NAPI value
	  return m_result_napi_value;
	};
	return napi::callback(cb);	
      }
    };

    template <typename ClassType>
    struct class_builder_helper<ClassType, void> {
      
      static napi::callback wrap_method(std::function<void(ClassType&)> m) {
	auto cb = [=](napi_env env, napi_callback_info info) {
	  // Parse function call aguments from napi callback info
	  magic::call_args<> m_call_args(env, info);
	  auto m_this_arg = (ClassType*) m_call_args.get_this_arg(env);
	  // Forward call arguments to native C++ function
	  m(*m_this_arg);
	  // Construct NAPI value from return value of native C++ function
	  napi_value m_result_napi_value = nullptr; // NAPI equivalent of m_result
	  // Cast C++ value to NAPI value
	  auto status = napi_get_undefined(env, &m_result_napi_value);
	  // Make sure all is well!
	  JSBIND11_CHECK_STATUS(status);
	  // Return NAPI value
	  return m_result_napi_value;
	};
	return napi::callback(cb);	
      }
    };
    
    
    template <typename ClassType>
    class class_builder {
      class_& c_;

      static napi_value finilize_callback_stub(napi_env env, napi_callback_info info) {
	napi_value v = nullptr;
	return v;
      }
    public:
      class_builder(class_& c) : c_(c) {}


      template <typename ... ArgTypes>
      class_builder& constructor() {
	std::function<ClassType*(ArgTypes...)> create = [](ArgTypes ... args) {
	  return new ClassType(args...);
	};
	
	napi::callback cb = [=](napi_env env, napi_callback_info info) {
	  cout << "Inside constructor\n";
	  // Parse function call aguments from napi callback info
	  magic::call_args<ArgTypes...> c_call_args(env, info);
	  const tuple<ArgTypes...>& c_args = c_call_args.get_args();
	  napi_value c_this_arg_napi_value = c_call_args.get_this_arg_napi_value();
	  ClassType* c_this_arg = magic::call<ClassType*, ArgTypes...>(create, c_args)();
	  napi_finalize c_finilize_callback = &class_builder<ClassType>::desctructor;
	  void* c_finilize_hint = nullptr;
	  
	  auto status = napi_wrap(env,
				  c_this_arg_napi_value,
				  c_this_arg,
				  c_finilize_callback,
				  c_finilize_hint,
				  nullptr);

	  // Make sure all is well!
	  JSBIND11_CHECK_STATUS(status);

	  // Return NAPI value
	  return c_this_arg_napi_value;
	};

	c_.add_constructor(cb);

	return *this;
      }

      template <typename RetType, typename ... ArgTypes>
      class_builder& method(const string& m_name, RetType (ClassType::*m_ptr)(ArgTypes...) const) {
	// Create function wrapper
	auto m_function = [=](ClassType& c, ArgTypes ... args) { return (c.*m_ptr)(args...); };
	// Delegate to other method
	return method_(m_name, std::function<RetType(ClassType&, ArgTypes...)>(m_function));
      }

      template <typename RetType, typename ... ArgTypes>
      class_builder& method(const string& m_name, RetType (ClassType::*m_ptr)(ArgTypes...)) {
	// Create function wrapper
	auto m_function = [=](ClassType& c, ArgTypes ... args) { return (c.*m_ptr)(args...); };
	// Delegate to other method
	return method_(m_name, std::function<RetType(ClassType&, ArgTypes...)>(m_function));
      }

      template <typename LambdaType>
      class_builder& method(const string& m_name, LambdaType m_lambda) {
	// Infer function type from lambda object
	typedef magic::function_traits<decltype(m_lambda)> f_traits;
	// Create function wrapper
	typename f_traits::function_type m = m_lambda;
	// Delegate to other function method
	return method_(m_name, m); 
      }

      template <typename RetType, typename ... ArgTypes>
      class_builder& method(const string& m_name, std::function<RetType(ClassType&, ArgTypes...)> m) {
	return method_(m_name, m);
      }

    private:
      template <typename RetType, typename ... ArgTypes>
      class_builder& method_(const string& m_name, std::function<RetType(const ClassType&, ArgTypes...)> m) {
	c_.add_method(m_name, class_builder_helper<ClassType, RetType, ArgTypes...>::wrap_method(m));
	return *this;
      }

      template <typename RetType, typename ... ArgTypes>
      class_builder& method_(const string& m_name, std::function<RetType(ClassType&, ArgTypes...)> m) {
	c_.add_method(m_name, class_builder_helper<ClassType, RetType, ArgTypes...>::wrap_method(m));
	return *this;
      }
    private:
      static void desctructor(napi_env env, void* data, void* hint) { delete (ClassType*) data; }
    };
      
    class function : public napi::value {  
      static napi::callback_table napi_callback_table_;
      size_t napi_callback_id_;
      string name_;

    public:
      function(const string& name, napi::callback c) : name_(name) {
	napi_callback_id_ = napi_callback_table_.add_callback(c);
      }

      const string& get_name() const {
	return name_;
      }

    private:

      napi_value get_napi_value_(napi_env env) {
	// NAPI call status 
	napi_status status;
	
	// Construct call args
	const char* f_name = name_.c_str();
	napi_callback f_napi_callback = &napi::callback_table::callback_dispatch;
	void* f_data = (void*)
	  &napi_callback_table_.get_callback(napi_callback_id_);
	napi_value f_napi_value; // initialized below
	
	// Create NAPI value for this function
	status =  napi_create_function(env,
				       f_name,
				       f_napi_callback,
				       f_data,
				       &f_napi_value);
	// Make sure all is well!
	JSBIND11_CHECK_STATUS(status); 

	// Return created NAPI value
	return f_napi_value;
      }
    };

    napi::callback_table function::napi_callback_table_;
    
    class module_desc {
      string name_;
      vector<class_> class_list_;
      vector<function> function_list_;
    public:
      explicit module_desc(const string& name) : name_(name) {}

      const string& get_name() const {
	return name_;
      }

      const vector<class_>& get_class_list() const {
	return class_list_;
      }

      class_& add_class(const class_& c) {
	class_list_.push_back(c);
	return class_list_.back();
      }
      
      const vector<function>& get_function_list() const {
	return function_list_;
      }
      
      function& add_function(const function& f) {
	function_list_.push_back(f);
	return function_list_.back();
      }
    };

    class module_init {
      shared_ptr<module_desc> m_;
    public:
      explicit module_init(shared_ptr<module_desc> m) : m_(m) {}

      void operator()(napi_env env,
		      napi_value exports,
		      napi_value module,
		      void* p) {
	// NAPI call status 
	napi_status status;

	// Export each class...
	for (auto c : m_->get_class_list()) {
	  auto c_name = c.get_name().c_str(); // class's name
	  auto c_napi_value = c.get_napi_value(env);
	  // Export NAPI value for this class under corresponding name
	  status = napi_set_named_property(env, exports, c_name, c_napi_value);
	  // Make sure all is well!
	  JSBIND11_CHECK_STATUS(status); 
	}

	// Export each function...
	for (auto f : m_->get_function_list()) {
	  auto f_name = f.get_name().c_str(); // function's name
	  auto f_napi_value = f.get_napi_value(env); // function's NAPI value
	  // Export NAPI value for this function under corresponding name
	  status = napi_set_named_property(env, exports, f_name, f_napi_value);
	  // Make sure all is well!
	  JSBIND11_CHECK_STATUS(status); 
	}
      }
    };

  }  // namespace internal

  template <typename T> class class_ {};

  class module {
    shared_ptr<internal::module_desc> desc_;
  public:
    module(const string& name) :
      desc_(make_shared<internal::module_desc>(name)) {}

    template <typename ClassType>
    internal::class_builder<ClassType> class_(const string& c_name) {
      // Add class to module descriptor and get its reference
      internal::class_& c = desc_->add_class(internal::class_(c_name));
      // Create class builder for ClassType and return it
      return internal::class_builder<ClassType>(c);
    }
    
    template <typename RetType, typename ... ArgTypes>
    internal::function& function(const string& f_name, RetType (*f_ptr)(ArgTypes...)) {
      // Create function wrapper object
      std::function<RetType(ArgTypes...)> f(f_ptr);
      // Delegate to other function method
      return function(f_name, f);
    }

    template <typename LambdaType>
    internal::function& function(const string& f_name, LambdaType f_lambda) {
      using namespace internal;
      // Infer function type from lambda object
      typedef magic::function_traits<decltype(f_lambda)> f_traits;
      // Create function wrapper
      typename f_traits::function_type f = f_lambda;
      // Delegate to other function method
      return function(f_name, f);
    }
    
    template <typename RetType, typename ... ArgTypes>
    internal::function& function(const string& f_name, std::
				 function<RetType(ArgTypes...)> f) {
      internal::function f_(f_name, [=](napi_env env, napi_callback_info info) {
	  using namespace internal;

	  // Parse function call aguments from napi callback info
	  auto f_args = magic::call_args<ArgTypes...>(env, info)();
	  // Forward call arguments to native C++ function
	  auto f_result = magic::call<RetType, ArgTypes...>(f, f_args)();
	  // Construct NAPI value from return value of native C++ function
	  napi_value f_result_napi_value = nullptr; // NAPI equivalent of f_result
	  // Cast C++ value to NAPI value
	  type<RetType, napi_value>::cast(env, f_result, f_result_napi_value);
	  // Return NAPI value
	  return f_result_napi_value;
	});
      return desc_->add_function(f_);
    }
    
    internal::module_init init() const {
      // Create init callback object
      internal::module_init init_(this->desc_);
      // Done!
      return init_;
    }
  };

  /*
  template <typename ObjectType>
  class class_ {
    template<typename T>
    struct function_ptr {
      typedef T* type;;
    };

  public:
    class_(module& m, const string& name) {
      // TODO: bind this class to module..
    }
    
    template <typename ... ArgTypes>
    class_& constructor(function<ObjectType*(ArgTypes...)> fun)
    {
      // TODO: register this constructor...

      return *this;
    }

    template <typename RetType, typename ... ArgTypes>
    class_& method(const string& name,
		   const typename function_ptr<RetType(ArgTypes...)>::type fun)
    {
      // TODO: register this method...

      return *this;
    }

    template <typename ValueType>
    class_& getter_method(const string& name,
			  const typename function_ptr<ValueType(void)>::type fun)
    {
      // TODO: register this method...

      return *this;
    }

    template <typename ValueType>
    class_& setter_method(const string& name,
			  const typename function_ptr<void(ValueType)>::type fun)
    {
      // TODO: register this method...

      return *this;
    }

    template <typename RetType, typename ... ArgTypes>
    class_& static_method(const string& name,
			  const typename function_ptr<RetType(ArgTypes...)>::type fun)
    {
      // TODO: register this method...

      return *this;
    }
    
  };
  */
  
}  //  namespace jsbind11

#undef JSBIND11_DEBUG
#undef JSBIND11_CHECK_STATUS

#endif  //  JSBIND_H_
