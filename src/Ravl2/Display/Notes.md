
# Updating imgui in bgfx

cp cmake-build-debug/_deps/bgfx.cmake-src/bgfx/examples/common/imgui/* ./src/Ravl2/Display/ThirdParty/bgfx_imgui/imgui/
cp cmake-build-debug/_deps/bgfx.cmake-src/bgfx/examples/common/bgfx_utils.* ./src/Ravl2/Display/ThirdParty/bgfx_imgui/
cp cmake-build-debug/_deps/bgfx.cmake-src/bgfx/examples/common/args.* ./src/Ravl2/Display/ThirdParty/bgfx_imgui
mv src/Ravl2/Display/ThirdParty/bgfx_imgui/bgfx_utils.h src/Ravl2/Display/ThirdParty/bgfx_imgui/bgfx_utils.hh