scons target=template_debug generate_bindings=no arch=arm64 dev_build=yes
rm -rf demo/addons
cp -rf bin/addons demo/addons

