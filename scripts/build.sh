#scons target=template_debug generate_bindings=no arch=arm64 dev_build=yes
scons target=template_release generate_bindings=no arch=arm64 precision=single
rm -rf demo/addons
cp -rf bin/addons demo/addons

