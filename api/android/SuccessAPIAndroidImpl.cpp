#include "SuccessAPIAndroidImpl.h"

SuccessAPIAndroidImpl::SuccessAPIAndroidImpl() : JNIWrapper<jni_success_api::Enum>("net/damsy/soupeaucaillou/heriswap/api/SuccessAPI", true) {
    declareMethod(jni_success_api::SuccessCompleted, "unlockAchievement", "(I)V");
    declareMethod(jni_success_api::OpenLeaderboard, "openLeaderboard", "(II)V");
    declareMethod(jni_success_api::OpenDashboard, "openDashboard", "()V");
}

void SuccessAPIAndroidImpl::successCompleted(const char* description, unsigned long successId) {
	SuccessAPI::successCompleted(description, successId);
	env->CallVoidMethod(instance, methods[jni_success_api::SuccessCompleted], (int)successId);
}

void SuccessAPIAndroidImpl::openLeaderboard(int mode, int diff) {
    env->CallVoidMethod(instance, methods[jni_success_api::OpenLeaderboard], mode, diff);
}

void SuccessAPIAndroidImpl::openDashboard() {
	env->CallVoidMethod(instance, methods[jni_success_api::OpenDashboard]);
}
