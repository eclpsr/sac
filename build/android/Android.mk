LOCAL_PATH := $(call my-dir)/../../

include $(CLEAR_VARS)

LOCAL_MODULE := sac

LOCAL_CFLAGS := -DANDROID_NDK \
				-I$(BASE_PATH)

LOCAL_CXXFLAGS := -DANDROID_NDK -DDEBUG \
                -I$(LOCAL_PATH)/base \
				-I$(LOCAL_PATH)/systems \
				-I$(LOCAL_PATH)/libs \
				-I$(LOCAL_PATH)/libs/libpng/jni


LOCAL_SRC_FILES := \
	base/Game.cpp \
    base/MathUtil.cpp \
    base/Vector2.cpp \
    base/Color.cpp \
    base/CircularBuffer.cpp \
    base/EntityManager.cpp \
    base/TouchInputManager.cpp \
    base/Log.cpp \
    base/PlacementHelper.cpp \
    base/TimeUtil.cpp \
    systems/ADSRSystem.cpp \
    systems/ButtonSystem.cpp \
    systems/ContainerSystem.cpp \
    systems/MorphingSystem.cpp \
    systems/MusicSystem.cpp \
    systems/ParticuleSystem.cpp \
    systems/PhysicsSystem.cpp \
    systems/RenderingSystem.cpp \
    systems/RenderingSystem_Texture.cpp \
    systems/RenderingSystem_Render.cpp \
    systems/ScrollingSystem.cpp \
    systems/SoundSystem.cpp \
    systems/System.cpp \
    systems/TransformationSystem.cpp \
    systems/TextRenderingSystem.cpp \
    api/android/AdAPIAndroidImpl.cpp \
    api/android/AssetAPIAndroidImpl.cpp \
    api/android/MusicAPIAndroidImpl.cpp \
    api/android/SoundAPIAndroidImpl.cpp \
    api/android/LocalizeAPIAndroidImpl.cpp \
    api/android/NameInputAPIAndroidImpl.cpp \
    api/android/ExitAPIAndroidImpl.cpp \
    api/android/SuccessAPIAndroidImpl.cpp \
	util/ImageLoader.cpp \
	util/IntersectionUtil.cpp \



#LOCAL_LDLIBS    += -lGLESv2 -llog

include $(BUILD_STATIC_LIBRARY)
