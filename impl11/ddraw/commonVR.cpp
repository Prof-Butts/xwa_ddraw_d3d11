#include "commonVR.h"
#include "EffectsCommon.h"
#include "DirectXMath.h"

const float DEFAULT_IPD = 6.5f; // Ignored in SteamVR mode.
const float IPD_SCALE_FACTOR = 100.0f; // Transform centimeters to meters (IPD = 6.5 becomes 0.065)
const float RAD_TO_DEG = 180.0f / 3.141593f;
float g_fIPD = DEFAULT_IPD / IPD_SCALE_FACTOR;
float g_fHalfIPD = g_fIPD / 2.0f;

Matrix4 g_projLeft, g_projRight;
vr::HmdMatrix34_t g_EyeMatrixLeft, g_EyeMatrixRight;
Matrix4 g_EyeMatrixLeftInv, g_EyeMatrixRightInv;
Matrix4 g_FullProjMatrixLeft, g_FullProjMatrixRight, g_viewMatrix;

Vector3 g_headCenter; // The head's center: this value should be re-calibrated whenever we set the headset
bool g_bResetHeadCenter = true; // Reset the head center on startup

bool g_bUseSeparateEyeBuffers = false; // The system will set this flag if the user requested SteamVR/OpenXR and it was initialized properly
StereoRenderer* g_stereoRenderer = new VRRendererOpenXR();


// NewIPD is in cms
void EvaluateIPD(float NewIPD) {
	if (NewIPD < 0.0f)
		NewIPD = 0.0f;

	g_fIPD = NewIPD / IPD_SCALE_FACTOR;
	log_debug("[DBG] NewIPD: %0.3f, Actual g_fIPD: %0.6f", NewIPD, g_fIPD);
	g_fHalfIPD = g_fIPD / 2.0f;
}

// Delta is in cms here
void IncreaseIPD(float Delta) {
	float NewIPD = g_fIPD * IPD_SCALE_FACTOR + Delta;
	EvaluateIPD(NewIPD);
}

Matrix4 XMFLOAT44toMatrix4(const DirectX::XMFLOAT4X4 & mat) {
	Matrix4 matrixObj(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0], //1st column
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1], //2nd column
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2], //3rd column
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]  //4th column
	);
	return matrixObj;
}