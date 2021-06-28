LOCAL_PATH := $(call my-dir)
# LOCAL_PATH = src/main/jni/src

include $(CLEAR_VARS)

LOCAL_MODULE := main

rwc = $(foreach d, $(wildcard $1*), $(call rwc,$d/,$2) $(filter $(subst *,%,$2),$d))

HCH_PATH := C:/Users/Justin/Dropbox/ImpostorEngine3
HCH_LIBS := $(HCH_PATH)/meta/android/app/src/libs

SDL_PATH := C:/AndroidLIB/SDL2-2.0.8/include/

LOCAL_C_INCLUDES := \
    $(SDL_PATH) \
    \
    $(HCH_PATH)/source/ \
    $(HCH_PATH)/include/ \
    \
    $(HCH_LIBS)/include/ \
    $(HCH_LIBS)/include/jpeg/ \
    $(HCH_LIBS)/libogg/ \
    $(HCH_LIBS)/libpng16/ \
    $(HCH_LIBS)/libvorbis/ \
    $(HCH_LIBS)/freetype/

# Add your application source files here...
LOCAL_SRC_FILES := \
    $(call rwc, $(HCH_PATH)/source/, *.cpp) \
	$(call rwc, $(HCH_PATH)/source/, *.c) \
	\
    $(call rwc, $(HCH_LIBS)/libogg/, *.c) \
    $(call rwc, $(HCH_LIBS)/libpng16/, *.c) \
    $(call rwc, $(HCH_LIBS)/libvorbis/, *.c) \
    \
    $(HCH_LIBS)/freetype/autofit/autofit.c \
    $(HCH_LIBS)/freetype/base/ftbase.c \
    $(HCH_LIBS)/freetype/base/ftbbox.c \
    $(HCH_LIBS)/freetype/base/ftbdf.c \
    $(HCH_LIBS)/freetype/base/ftbitmap.c \
    $(HCH_LIBS)/freetype/base/ftcid.c \
    $(HCH_LIBS)/freetype/base/ftfstype.c \
    $(HCH_LIBS)/freetype/base/ftgasp.c \
    $(HCH_LIBS)/freetype/base/ftglyph.c \
    $(HCH_LIBS)/freetype/base/ftgxval.c \
    $(HCH_LIBS)/freetype/base/ftinit.c \
    $(HCH_LIBS)/freetype/base/ftmm.c \
    $(HCH_LIBS)/freetype/base/ftotval.c \
    $(HCH_LIBS)/freetype/base/ftpatent.c \
    $(HCH_LIBS)/freetype/base/ftpfr.c \
    $(HCH_LIBS)/freetype/base/ftstroke.c \
    $(HCH_LIBS)/freetype/base/ftsynth.c \
    $(HCH_LIBS)/freetype/base/ftsystem.c \
    $(HCH_LIBS)/freetype/base/fttype1.c \
    $(HCH_LIBS)/freetype/base/ftwinfnt.c \
    $(HCH_LIBS)/freetype/bdf/bdf.c \
    $(HCH_LIBS)/freetype/cache/ftcache.c \
    $(HCH_LIBS)/freetype/cff/cff.c \
    $(HCH_LIBS)/freetype/cid/type1cid.c \
    $(HCH_LIBS)/freetype/gzip/ftgzip.c \
    $(HCH_LIBS)/freetype/lzw/ftlzw.c \
    $(HCH_LIBS)/freetype/pcf/pcf.c \
    $(HCH_LIBS)/freetype/pfr/pfr.c \
    $(HCH_LIBS)/freetype/psaux/psaux.c \
    $(HCH_LIBS)/freetype/pshinter/pshinter.c \
    $(HCH_LIBS)/freetype/psnames/psmodule.c \
    $(HCH_LIBS)/freetype/raster/raster.c \
    $(HCH_LIBS)/freetype/sfnt/sfnt.c \
    $(HCH_LIBS)/freetype/smooth/smooth.c \
    $(HCH_LIBS)/freetype/truetype/truetype.c \
    $(HCH_LIBS)/freetype/type1/type1.c \
    $(HCH_LIBS)/freetype/type42/type42.c \
    $(HCH_LIBS)/freetype/winfonts/winfnt.c \
    $(HCH_LIBS)/freetype/ftdebug.c

LOCAL_SHARED_LIBRARIES := SDL2
LOCAL_STATIC_LIBRARIES += libjpeg

LOCAL_LDLIBS := -lGLESv2 -lz -llog

include $(BUILD_SHARED_LIBRARY)
