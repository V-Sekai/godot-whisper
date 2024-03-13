scons target=template_release generate_bindings=no arch=universal precision=single
rm -rf samples/godot_whisper/addons/godot_whisper/bin
cp -rf bin/addons/godot_whisper/bin samples/godot_whisper/addons/godot_whisper/bin

