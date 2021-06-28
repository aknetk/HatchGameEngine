
# Uncomment this if you're using STL in your project
# See CPLUSPLUS-SUPPORT.html in the NDK documentation for more information
APP_STL := c++_static

APP_OPTIM := debug

APP_CFLAGS += -std=c11 -DNOCRYPT -DONOUNCRYPT -GL_GLEXT_PROTOTYPES -O3 -DNDEBUG -funsafe-math-optimizations -DFT2_BUILD_LIBRARY -DUNICODE -D_UNICODE \
    -DNO_CURL -DNO_LIBAV
APP_CPPFLAGS += -std=c++11 -GL_GLEXT_PROTOTYPES -DANDROID -DTARGET_NAME=\"HatchGameEngine\" -O3 -DNDEBUG -funsafe-math-optimizations

APP_ABI := armeabi armeabi-v7a x86

# Min SDK level
APP_PLATFORM=android-19

