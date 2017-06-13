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

  //  template<typename A, typename B> struct type {}

  // TODO: add partial specializations of type to convert between JS and C++ types

  namespace internal {
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
      
      inline void check_status(napi_status status) {
	if (status != napi_ok) {
	  throw runtime_error("NAPI status isn't ok...");
	}
      }
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
  }  // namespace internal

  class module {
    shared_ptr<internal::module_desc> desc_;
  public:
    module(const string& name) :
      desc_(make_shared<internal::module_desc>(name)) {}

    template <typename FunType>
    void function(const string& name, const FunType fun) {
      auto cb = [=](napi_env env, napi_callback_info info) {
	napi_value v = nullptr;
	napi_create_number(env, 420.0, &v);
	return v;
	// TODO: get parameters using napi_callback_info,
	//       convert them to C++ types, and dispatch call to fun
      };
      desc_->add_function(name, cb);
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
