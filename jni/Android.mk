LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := jni-codec

LOCAL_SRC_FILES := \
		AmrCodec.cpp \
		G711Codec.cpp \
		JniCodec.cpp

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/amrnb/common/include \
		$(LOCAL_PATH)/amrnb/enc/src \
		$(LOCAL_PATH)/amrnb/dec/src

LOCAL_SHARED_LIBRARIES := \
		libstagefright_amrnb_common

LOCAL_STATIC_LIBRARIES := \
		libstagefright_amrnbenc \
		libstagefright_amrnbdec

include $(BUILD_SHARED_LIBRARY)
							
include $(call all-makefiles-under,$(LOCAL_PATH))
