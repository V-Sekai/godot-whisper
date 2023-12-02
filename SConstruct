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
        # Debug logs
        "GGML_METAL_NDEBUG"
    ]
)

enable_webrtc_logging = env["target"] == "debug"

if not enable_webrtc_logging:
    env.Append(CPPDEFINES=["RTC_DISABLE_LOGGING", "RTC_DISABLE_METRICS"])

if env["platform"] == "windows" or env["platform"] == "uwp":
    env.Append(CPPDEFINES=["WEBRTC_WIN"])
elif env["platform"] == "ios":
    env.Append(CPPDEFINES=["WEBRTC_POSIX", "WEBRTC_IOS"])
elif env["platform"] == "macos":
    env.Append(CPPDEFINES=["WEBRTC_POSIX", "WEBRTC_MAC"])
elif env["platform"] == "linuxbsd":
    env.Append(CPPDEFINES=["WEBRTC_POSIX", "WEBRTC_LINUX"])
elif env["platform"] == "android":
    env.Append(CPPDEFINES=["WEBRTC_POSIX", "WEBRTC_ANDROID"])
else:  # including if env["platform"] == "javascript":
    env.Append(CPPDEFINES=["WEBRTC_POSIX"])
env.Prepend(CPPPATH=["thirdparty", "include"])
env.Append(CPPPATH=["src/"])
env.Append(CPPDEFINES=['WHISPER_SHARED', 'GGML_SHARED'])
sources = [Glob("src/*.cpp")]
sources.extend([Glob("thirdparty/libsamplerate/src/*.c"), Glob("thirdparty/whisper.cpp/*.c"), Glob("thirdparty/whisper.cpp/whisper.cpp")])
if env["platform"] == "macos":
	library = env.SharedLibrary(
		"bin/addons/godot_whisper/bin/libgodot_whisper.{}.{}.framework/libgodot_whisper.{}.{}".format(
			env["platform"], env["target"], env["platform"], env["target"]
		),
		source=sources,
	)
else:
	library = env.SharedLibrary(
		"bin/addons/godot_whisper/bin/libgodot_whisper{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
		source=sources,
	)
Default(library)
