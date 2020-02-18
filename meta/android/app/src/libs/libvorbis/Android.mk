LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libvorbis
LOCAL_CFLAGS += -I$(LOCAL_PATH)/../include

LOCAL_SRC_FILES := \
	$(addprefix ../, $(shell cd $(LOCAL_PATH)/../; \
                           find -type f -name '*.c' | \
                           grep -v psytune.c | grep -v tone.c))
include $(BUILD_STATIC_LIBRARY)
