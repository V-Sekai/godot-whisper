#!/usr/bin/env python
import os
import sys

env = SConscript("thirdparty/godot-cpp/SConstruct")

env.Prepend(CPPPATH=["thirdparty"])
env.Append(CPPPATH=["src/"])
sources = [Glob("src/*.cpp")]
sources.extend([Glob("libsamplerate/src/*.c"), Glob("thirdparty/whisper.cpp/*.c"), Glob("thirdparty/whisper.cpp/whisper.cpp")])
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
