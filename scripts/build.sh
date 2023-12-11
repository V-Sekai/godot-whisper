scons target=template_release generate_bindings=no arch=universal precision=single
rm -rf demo/addons
cp -rf bin/addons demo/addons

