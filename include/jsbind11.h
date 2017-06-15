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

  template<>
  struct type<napi_value, double> {
    static void cast(napi_env env, const napi_value& source, double& target) {
      internal::napi::
	check_status(napi_get_value_double(env, source, &target));
    }
  };

  template<>
  struct type<double, napi_value> {
    static void cast(napi_env env, const double& source, napi_value& target) {
      internal::napi::
	check_status(napi_create_number(env, source, &target));
    }
  };
  
  // TODO: add partial specializations of type to convert between JS and C++ types

  namespace internal {
    namespace magic {
      template<int ...>
      struct seq {};
      template<int N, int ...S>
      struct gens : gens<N-1, N-1, S...> {};
      template<int ...S>
      struct gens<0, S...>{ typedef seq<S...> type; };
      template <typename T, typename ...Args>
      class call {
	std::tuple<Args...> args_;
	std::function<T(Args...)> func_;
	//	T (*func_)(Args...);
      public:
	explicit call(T (*f)(Args...),
		      std::tuple<Args...> a) : args_(a), func_(f) {}
	explicit call(std::function<T(Args...)> f,
		      std::tuple<Args...> a) : args_(a), func_(f) {}
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
      
      class value {
      public:
	napi_value get_napi_value(napi_env env) {
	  return this->get_napi_value_(env);
	}
      private:
	virtual napi_value get_napi_value_(napi_env env) = 0;	  
      };
    }  // namespace napi

    class function : public napi::value {  
      static vector<napi::callback> napi_callback_list_;
      static mutex mutex_;
      size_t napi_callback_id_;
      string name_;

    public:
      function(const string& name, napi::callback c) : name_(name) {
	lock_guard<mutex> lock(mutex_);
	napi_callback_id_ = napi_callback_list_.size();
	napi_callback_list_.push_back(c);
      }

      const string& get_name() const {
	return name_;
      }

      static napi_value napi_callback_dispatch(napi_env env,
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
	napi::check_status(status);
	
	// Data must be valid.
	if (data == nullptr) {
	  // TODO: raise runtime error
	}

	// Dispatch to appropriate callback
	return (*(napi::callback*)(data))(env, info);
      }
    private:    
      napi_value get_napi_value_(napi_env env) {
	// NAPI call status 
	napi_status status;
	
	// Construct call args
	const char* f_name = name_.c_str();
	napi_callback f_napi_callback = &napi_callback_dispatch;
	void* f_data = &napi_callback_list_[napi_callback_id_];;
	napi_value f_napi_value; // initialized below
	
	// Create NAPI value for this function
	status =  napi_create_function(env,
				       f_name,
				       f_napi_callback,
				       f_data,
				       &f_napi_value);
	// Make sure all is well!
	napi::check_status(status);

	// Return created NAPI value
	return f_napi_value;
      }
    };

    vector<napi::callback> function::napi_callback_list_{};
    mutex function::mutex_{};

    class module_desc {
      string name_;
      vector<function> function_list_;
    public:
      explicit module_desc(const string& name) : name_(name) {}

      const string& get_name() const {
	return name_;
      }
      
      const vector<function>& get_function_list() const {
	return function_list_;
      }
      
      void add_function(const string& name, napi::callback cb) {
	function_list_.push_back(function(name, cb));
      }
    };

    class module_init {
      shared_ptr<module_desc> m_;
    public:
      explicit module_init(shared_ptr<module_desc> m) : m_(m) {}

      void operator()(napi_env env,
		      napi_value exports,
		      napi_value module,
		      void* p)
      {
	// NAPI call status 
	napi_status status;
	// Get list of functions to export
	auto function_list = m_->get_function_list();
	// Export each function in this list...
	for (auto f : function_list) {
	  //	for (size_t n = 0; n < function_list.size(); n++) {
	  //	  auto f = function_list[n]; // Get function object
	  auto f_napi_value = f.get_napi_value(env); // Get its NAPI value
	  auto f_name = f.get_name().c_str(); // Get its name
	  // Export NAPI value for this function under corresponding name
	  status = napi_set_named_property(env, exports, f_name, f_napi_value);
	  // Make sure all is well!
	  internal::napi::check_status(status);
	}
      }
    };

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
	tuple<ArgTypes...> args_;
      public:
	call_args(napi_env env, napi_callback_info info) {
	  // NAPI call status
	  napi_status status;

	  // Note: it's unlikely that anyone would be using function call
	  //       signatures with 256+ call parameters, so this should do...
	  const size_t max_argc = 256;

	  // Make sure function type isn't insane 
	  static_assert(max_argc >= tuple_size<tuple<ArgTypes...>>::value,
			"Maximum number of jsbind11 function arguments is 256");

	  // Construct call args
	  size_t argc = max_argc;
	  napi_value argv[max_argc];
	  napi_value this_arg;
	  void* data;

	  // Get callback info via NAPI
	  status = napi_get_cb_info(env, info, &argc, argv, &this_arg, &data);

	  // Make sure all is well!
	  napi::check_status(status);

	  if (argc != tuple_size<tuple<ArgTypes...>>::value) {
	    // TODO: provide more meaningfull error message
	    throw runtime_error("Received wrong number of function arguments");
	  }

	  call_args_helper<ArgTypes...>::parse_args(env, args_, argv);
	}
	
	tuple<ArgTypes...> operator()() {
	  return args_;
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
  }  // namespace internal

  class module {
    shared_ptr<internal::module_desc> desc_;
  public:
    module(const string& name) :
      desc_(make_shared<internal::module_desc>(name)) {}

    template <typename RetType, typename ... ArgTypes>
    void function(const string& f_name, RetType (*f_ptr)(ArgTypes...)) {
      // Create function wrapper object
      std::function<RetType(ArgTypes...)> f(f_ptr);
      // Delegate to other function method
      function(f_name, f);
    }

    template <typename LambdaType>
    void function(const string& f_name, LambdaType f_lambda) {
      using namespace internal;
      // Infer function type from lambda object
      typedef magic::function_traits<decltype(f_lambda)> f_traits;
      // Create function wrapper
      typename f_traits::function_type f = f_lambda;
      // Delegate to other function method
      function(f_name, f);
    }
    
    template <typename RetType, typename ... ArgTypes>
    void function(const string& f_name, std::function<RetType(ArgTypes...)> f) {
      desc_->add_function(f_name, [=](napi_env env, napi_callback_info info) {
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

    module& m_;
    string name_;
  public:
    class_(module& m, const string& name) :
      m_(m), name_(name)
    {
      // TODO: bind this class to module..
    }
    
    template <typename ... ArgTypes>
    class_& constructor()
    {
      // TODO: register this constructor...

      return *this;
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

#endif  //  JSBIND_H_
