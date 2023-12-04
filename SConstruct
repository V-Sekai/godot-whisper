#!/usr/bin/env python
import os
import sys

env = SConscript("thirdparty/godot-cpp/SConstruct")

env.Append(
    CPPDEFINES=[
        "HAVE_CONFIG_H",
        "PACKAGE=",
        "VERSION=",
        "CPU_CLIPS_POSITIVE=0",
        "CPU_CLIPS_NEGATIVE=0",
        "WEBRTC_APM_DEBUG_DUMP=0",
        "WHISPER_BUILD",
        "GGML_BUILD",
        "GGML_USE_CLBLAST",
        "OPENCL_API",
        "USE_ICD_LOADER",
        # Debug logs
        # "GGML_METAL_NDEBUG"
    ]
)

env.Prepend(CPPPATH=["thirdparty", "include", "thirdparty/opencl_headers", "thirdparty/clblast/include", "thirdparty/clblast/src"])
env.Append(CPPPATH=["src/"])
env.Append(CPPDEFINES=['WHISPER_SHARED', 'GGML_SHARED'])
sources = [Glob("src/*.cpp")]

if not env["use_mingw"] and env["platform"] == "windows":
        env.Append(CCFLAGS=['/arch:AVX'])
        env.Append(LINKFLAGS=['/arch:AVX'])
else:
    if env["platform"] != "web":
        env.Append(CCFLAGS=['-mavx'])

opencl_include_dir = os.environ.get('OpenCL_INCLUDE_DIR')
if opencl_include_dir:
    env.Append(CPPDEFINES=[opencl_include_dir])

opencl_library = os.environ.get('OpenCL_LIBRARY')
if opencl_library:
    env.Append(LIBS=[opencl_library])

sources.extend([
    Glob("thirdparty/libsamplerate/src/*.c"),
    Glob("thirdparty/whisper.cpp/*.c"),
    Glob("thirdparty/whisper.cpp/whisper.cpp"),
    "thirdparty/whisper.cpp/ggml.c",
    "thirdparty/whisper.cpp/ggml-opencl.cpp",
])

clblast_sources = [
    "thirdparty/clblast/src/database/database.cpp",
    "thirdparty/clblast/src/routines/common.cpp",
    "thirdparty/clblast/src/utilities/compile.cpp",
    "thirdparty/clblast/src/utilities/clblast_exceptions.cpp",
    "thirdparty/clblast/src/utilities/timing.cpp",
    "thirdparty/clblast/src/utilities/utilities.cpp",
    "thirdparty/clblast/src/api_common.cpp",
    "thirdparty/clblast/src/cache.cpp",
    "thirdparty/clblast/src/kernel_preprocessor.cpp",
    "thirdparty/clblast/src/routine.cpp",
    "thirdparty/clblast/src/tuning/configurations.cpp",
    # OpenCL specific sources
    "thirdparty/clblast/src/clblast.cpp",
    "thirdparty/clblast/src/clblast_c.cpp",
    "thirdparty/clblast/src/tuning/tuning_api.cpp"
]

databases = ['copy', 'pad', 'padtranspose', 'transpose', 'xaxpy', 'xdot', 
             'xgemm', 'xgemm_direct', 'xgemv', 'xgemv_fast', 'xgemv_fast_rot', 
             'xger', 'invert', 'gemm_routine', 'trsv_routine', 'xconvgemm']

for database in databases:
    clblast_sources.append('thirdparty/clblast/src/database/kernels/' + database + '/' + database + '.cpp')

sources.extend(clblast_sources)

routines = {
    'level1': Glob("thirdparty/clblast/src/routines/level1/*.cpp"),
    'level2': Glob("thirdparty/clblast/src/routines/level2/*.cpp"),
    'level3': Glob("thirdparty/clblast/src/routines/level3/*.cpp"),
    'levelx': Glob("thirdparty/clblast/src/routines/levelx/*.cpp"),
}

for level, files in routines.items():
    sources.extend(files)

sources.extend(Glob("thirdparty/clblast/src/tuners/*.cpp"))

if env["platform"] == "macos":
	library = env.SharedLibrary(
		"bin/addons/godot_whisper/bin/libgodot_whisper{}.framework/libgodot_whisper{}".format(
			env["suffix"], env["suffix"]
		),
		source=sources,
	)
else:
	library = env.SharedLibrary(
		"bin/addons/godot_whisper/bin/libgodot_whisper{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
		source=sources,
	)
Default(library)
