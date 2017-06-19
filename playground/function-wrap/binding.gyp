{
    'targets': [
	{
	    'target_name': 'function-wrap-native',
	    'sources': [ 'src/function-wrap.cpp' ],
	    'include_dirs': [
		"../../include",
		"<!@(node -p \"require('node-addon-api').include\")"
	    ],
	    'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
	    'cflags': ['-std=c++11', '-DJSBIND11_DEBUG'],
	    'cflags!': [ '-fno-exceptions' ],
	    'cflags_cc!': [ '-fno-exceptions' ],
	    'xcode_settings': {
		'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
		'CLANG_CXX_LIBRARY': 'libc++',
		'MACOSX_DEPLOYMENT_TARGET': '10.7'
	    },
	    'msvs_settings': {
		'VCCLCompilerTool': { 'ExceptionHandling': 1 },
	    }
	}
    ]
}
