#!/usr/bin/env python
import os
import sys

env = SConscript("thirdparty/godot-cpp/SConstruct")

module_env = env.Clone()

if not env.msvc:
    module_env.Append(CCFLAGS=["-Wno-error=non-virtual-dtor"])
    module_env.Append(CCFLAGS=["-Wno-error=ctor-dtor-privacy"])

module_env.Append(
    CPPDEFINES=[
        "HAVE_CONFIG_H",
        "PACKAGE=",
        "VERSION=",
        "CPU_CLIPS_POSITIVE=0",
        "CPU_CLIPS_NEGATIVE=0",
        "WEBRTC_APM_DEBUG_DUMP=0",
        "WHISPER_BUILD",
        "GGML_BUILD",
    ]
)

enable_webrtc_logging = env["target"] == "debug"

if not enable_webrtc_logging:
    module_env.Append(CPPDEFINES=["RTC_DISABLE_LOGGING", "RTC_DISABLE_METRICS"])

if env["platform"] == "windows" or env["platform"] == "uwp":
    module_env.Append(CPPDEFINES=["WEBRTC_WIN"])
elif env["platform"] == "ios":
    module_env.Append(CPPDEFINES=["WEBRTC_POSIX", "WEBRTC_IOS"])
elif env["platform"] == "macos":
    module_env.Append(CPPDEFINES=["WEBRTC_POSIX", "WEBRTC_MAC"])
elif env["platform"] == "linuxbsd":
    module_env.Append(CPPDEFINES=["WEBRTC_POSIX", "WEBRTC_LINUX"])
elif env["platform"] == "android":
    module_env.Append(CPPDEFINES=["WEBRTC_POSIX", "WEBRTC_ANDROID"])
else:  # including if env["platform"] == "javascript":
    module_env.Append(CPPDEFINES=["WEBRTC_POSIX"])

module_env.Prepend(CPPPATH=["thirdparty/libsamplerate/src"])
module_env.Prepend(CPPPATH=["include"])

env_thirdparty = module_env.Clone()
env_thirdparty.disable_warnings()

env_thirdparty.Prepend(CPPPATH=["#thirdparty/libogg"])

env_thirdparty.add_source_files(env.modules_sources, Glob("thirdparty/libsamplerate/src/*.c"))
env_thirdparty.add_source_files(env.modules_sources, Glob("thirdparty/whisper.cpp/*.c"))
env_thirdparty.add_source_files(env.modules_sources, Glob("thirdparty/whisper.cpp/whisper.cpp"))
env_thirdparty.Append(CPPPATH=['thirdparty/whisper.cpp'])
env_thirdparty.Append(CPPDEFINES=['WHISPER_SHARED', 'GGML_SHARED'])
module_env.add_source_files(env.modules_sources, "*.cpp")

# For the reference:
# - CCFLAGS are compilation flags shared between C and C++
# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=["src/"])
sources = [Glob("src/*.cpp")]
sources.extend([box2d_folder + 'src/' + box2d_src_file for box2d_src_file in box2d_src])

if env["platform"] == "macos":
	library = env.SharedLibrary(
		"bin/addons/godot-box2d/bin/libgodot-box2d.{}.{}.framework/libgodot-box2d.{}.{}".format(
			env["platform"], env["target"], env["platform"], env["target"]
		),
		source=sources,
	)
else:
	library = env.SharedLibrary(
		"bin/addons/godot-box2d/bin/libgodot-box2d{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
		source=sources,
	)
Default(library)
