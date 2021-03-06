{
    'targets': [
	{
	    'target_name': 'test',
	    'sources': [
		'../src/jsbind11.cpp',
		'jsbind11-test.cpp',
		'test-adder.cpp',
		'test-function-export.cpp'
	    ],
	    'include_dirs': [
	    	"../include",
	   	"<!@(node -p \"require('node-addon-api').include\")",
	    ],
	    'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
	    'cflags': ['-std=c++11'],
	    'cflags!': [ '-fno-exceptions'],
	    'cflags_cc!': [ '-fno-exceptions' ],
	    'xcode_settings': {
		'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
		'GCC_ENABLE_CPP_RTTI': 'YES',
		'CLANG_CXX_LIBRARY': 'libc++',
		'MACOSX_DEPLOYMENT_TARGET': '10.7'
	    },
	    'msvs_settings': {
		'VCCLCompilerTool': { 'ExceptionHandling': 1 },
	    }
	}
    ]
}
