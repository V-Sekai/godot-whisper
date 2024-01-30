scons target=template_debug generate_bindings=no arch=universal dev_build=yes
#scons target=template_release generate_bindings=no arch=universal precision=single
rm -rf demo/addons/bin
cp -rf bin/addons/bin demo/addons/bin

