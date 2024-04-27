// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes (c) 2019
#include "common.h"
#include <ScreenGrab.h>
#include <wincodec.h>

#include "hook_config.h"
#include "DeviceResources.h"
#include "PrimarySurface.h"
#include "BackbufferSurface.h"
#include "FrontbufferSurface.h"
#include "FreePIE.h"
#include "Matrices.h"
#include "Direct3DTexture.h"
#include "XWAFramework.h"
#include "XwaDrawTextHook.h"
#include "XwaDrawRadarHook.h"
#include "XwaDrawBracketHook.h"
#include "XwaConcourseHook.h"
#include "xwa_structures.h"
#include "effects.h"
#include "globals.h"
#include "commonVR.h"
#include "VRConfig.h"
#include "SharedMem.h"
#include "D3dRenderer.h"
#include "EffectsRenderer.h"
#include "LBVH.h"

#include <DDSTextureLoader.h>

extern D3dRenderer* g_current_renderer;
extern LBVH* g_ACTLASTree;

extern float g_f0x08C1600, g_f0x0686ACC;
extern float g_f0x080ACF8, g_f0x07B33C0, g_f0x064D1AC;

// Text Rendering
TimedMessage g_TimedMessages[MAX_TIMED_MESSAGES];

void ZToDepthRHW(float Z, float *sz, float *rhw);
void GetCraftViewMatrix(Matrix4 *result);
void GetGunnerTurretViewMatrixSpeedEffect(Matrix4 * result);
void DisplayBox(char *name, Box box);
Vector3 project(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix /*, float *sx, float *sy */);
inline void backProject(float sx, float sy, float rhw, Vector3 *P);
inline void backProjectMetric(float sx, float sy, float rhw, Vector3 *P);

// These are the actual functions for (back)-projection currently used as of Feb 2024:
float4 TransformProjection(float3 input);
float4 TransformProjectionScreen(float3 input);
float3 InverseTransformProjectionScreen(float4 input);

Vector3 projectMetric(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix, bool bForceNonVR);
inline Vector3 projectToInGameCoords(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix);
inline Vector3 projectToInGameOrPostProcCoordsMetric(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix, bool bForceNonVR = false);
//void ComputeBaryCoords(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector3& P,
//	float& u, float& v);
bool rayTriangleIntersect(
	const Vector3 &orig, const Vector3 &dir,
	const Vector3 &v0, const Vector3 &v1, const Vector3 &v2,
	float &t, Vector3 &P, float &u, float &v, float margin);
float ClosestPointOnTriangle(
	const Vector3& orig, const Vector3& v0, const Vector3& v1, const Vector3& v2,
	Vector3& P, float& u, float& v, float margin);
void ResetXWALightInfo();
void DumpHyperspaceVertexBuffer(float width, float height);
inline Matrix4 XwaTransformToMatrix4(const XwaTransform& M);
float4 TransformProjection(float3 input);
float4 TransformProjectionScreen(float3 input);
void EmulMouseWithVRControllers();

int MakeKeyFromGroupIdImageId(int groupId, int imageId);

void SetPresentCounter(int val, int bResetReticle) {
	g_iPresentCounter = val;
	if (g_pSharedDataCockpitLook != nullptr && g_SharedMemCockpitLook.IsDataReady() && bResetReticle) {
		g_pSharedDataCockpitLook->bIsReticleSetup = 0;
	}
}

// S0x0000001
// L00439B30
void XwaVector3Transform(XwaVector3* A4, const XwaMatrix3x3* A8)
{
	float A4_00 = A8->_31 * A4->z + A8->_21 * A4->y + A8->_11 * A4->x;
	float A4_04 = A8->_32 * A4->z + A8->_22 * A4->y + A8->_12 * A4->x;
	float A4_08 = A8->_33 * A4->z + A8->_23 * A4->y + A8->_13 * A4->x;

	A4->x = A4_00;
	A4->y = A4_04;
	A4->z = A4_08;
}

/*
void DumpXwaTransform(char *name, XwaTransform *t) {
	log_debug("[DBG] ********************");
	log_debug("[DBG] %s", name);
	log_debug("[DBG] Rotation:");
	log_debug("[DBG] %0.3f, %0.3f, %0.3f", t->Rotation._11, t->Rotation._12, t->Rotation._13);
	log_debug("[DBG] %0.3f, %0.3f, %0.3f", t->Rotation._21, t->Rotation._22, t->Rotation._23);
	log_debug("[DBG] %0.3f, %0.3f, %0.3f", t->Rotation._31, t->Rotation._32, t->Rotation._33);
	log_debug("[DBG] Translation:");
	log_debug("[DBG] %0.3f, %0.3f, %0.3f", t->Position.x, t->Position.y, t->Position.z);
	log_debug("[DBG] ********************");
}
*/

/*
void DumpGlobalLights()
{
	std::ostringstream str;

	int s_XwaCurrentSceneCompData = *(int*)0x009B6D02;
	int s_XwaSceneCompDatasOffset = *(int*)0x009B6CF8;

	XwaTransform* ViewTransform = (XwaTransform*)(s_XwaSceneCompDatasOffset + s_XwaCurrentSceneCompData * 284 + 0x0008);
	XwaTransform* WorldTransform = (XwaTransform*)(s_XwaSceneCompDatasOffset + s_XwaCurrentSceneCompData * 284 + 0x0038);

	//DumpXwaTransform("ViewTransform", ViewTransform);
	//DumpXwaTransform("WorldTransform", WorldTransform);

	for (int i = 0; i < *s_XwaGlobalLightsCount; i++)
	//int i = 0;
	{
		log_debug("[DBG] ***************************");
		//str << std::endl;
		//log_debug("[DBG] light[%d], pos: (%0.3f, %0.3f, %0.3f)",
		//	i, s_XwaGlobalLights[i].PositionX, s_XwaGlobalLights[i].PositionY, s_XwaGlobalLights[i].PositionZ);
		log_debug("[DBG] light[%d], dir: [%0.3f, %0.3f, %0.3f]",
			i, s_XwaGlobalLights[i].DirectionX, s_XwaGlobalLights[i].DirectionY, s_XwaGlobalLights[i].DirectionZ);

		XwaVector3 viewDir = { s_XwaGlobalLights[i].DirectionX, s_XwaGlobalLights[i].DirectionY, s_XwaGlobalLights[i].DirectionZ };
		XwaVector3Transform(&viewDir, &ViewTransform->Rotation);
		log_debug("[DBG] light[%d], viewDir: [%0.3f, %0.3f, %0.3f]", i, viewDir.x,  viewDir.y, viewDir.z);

		XwaVector3 worldDir = { s_XwaGlobalLights[i].DirectionX, s_XwaGlobalLights[i].DirectionY, s_XwaGlobalLights[i].DirectionZ };
		XwaVector3Transform(&worldDir, &WorldTransform->Rotation);
		log_debug("[DBG] light[%d], worldDir: [%0.3f, %0.3f, %0.3f]", i, worldDir.x, worldDir.y, worldDir.z);

		XwaVector3 worlViewDir = { s_XwaGlobalLights[i].DirectionX, s_XwaGlobalLights[i].DirectionY, s_XwaGlobalLights[i].DirectionZ };
		XwaVector3Transform(&worlViewDir, &WorldTransform->Rotation);
		XwaVector3Transform(&worlViewDir, &ViewTransform->Rotation);
		log_debug("[DBG] light[%d], worldViewDir: [%0.3f, %0.3f, %0.3f]", 
			i, worlViewDir.x, worlViewDir.y, worlViewDir.z);
		log_debug("[DBG] ***************************");
	}

	//LogText(str.str());
}
*/

inline float lerp(float x, float y, float s) {
	return x + s * (y - x);
}

/*
void ShowXWAMatrix(const XwaTransform &m) {
	log_debug("[DBG] -----------------------------");
	log_debug("[DBG] %0.4f, %0.4f, %0.4f, %0.4f", m.Rotation._11, m.Rotation._12, m.Rotation._13, m.Position.x);
	log_debug("[DBG] %0.4f, %0.4f, %0.4f, %0.4f", m.Rotation._21, m.Rotation._22, m.Rotation._23, m.Position.y);
	log_debug("[DBG] %0.4f, %0.4f, %0.4f, %0.4f", m.Rotation._31, m.Rotation._32, m.Rotation._33, m.Position.z);
	log_debug("[DBG] -----------------------------");
}

void ShowXWAVector(char *msg, const XwaVector3 &v) {
	log_debug("[DBG] %s [%0.4f, %0.4f, %0.4f]", msg, v.x, v.y, v.z);
}
*/

/*
void ComputeRotationMatrixFromXWAView(Vector4 *light, int num_lights) {
	Vector4 tmpL[2], T, B, N;
	// Compute the full rotation
	float yaw   = PlayerDataTable[*g_playerIndex].yaw   / 65536.0f * 360.0f;
	float pitch = PlayerDataTable[*g_playerIndex].pitch / 65536.0f * 360.0f;
	float roll  = PlayerDataTable[*g_playerIndex].roll  / 65536.0f * 360.0f;

	// To test how (x,y,z) is aligned with either the Y+ or Z+ axis, just multiply rotMatrixPitch * rotMatrixYaw * (x,y,z)
	Matrix4 rotMatrixFull, rotMatrixYaw, rotMatrixPitch, rotMatrixRoll;
	rotMatrixFull.identity();
	rotMatrixYaw.identity();   rotMatrixYaw.rotateY(-yaw);
	rotMatrixPitch.identity(); rotMatrixPitch.rotateX(-pitch);
	rotMatrixRoll.identity();  rotMatrixRoll.rotateY(-roll); // Z or Y?

	// rotMatrixYaw aligns the orientation with the y-z plane (x --> 0)
	// rotMatrixPitch * rotMatrixYaw aligns the orientation with y+ (x --> 0 && z --> 0)
	// so the remaining rotation must be around the y axis (?)
	// DEBUG, x = z, y = x, z = y;
	// The yaw is indeed the y-axis rotation, it goes from -180 to 0 to 180.
	// When pitch == 90, the craft is actually seeing the horizon
	// When pitch == 0, the craft is looking towards the sun
	// When entering hyperspace, yaw,pitch,raw = (0,90,0), so that's probably the "home" orientation: level and looking forward
	// New approach: let's build a TBN system here to avoid the gimbal lock problem
	float cosTheta, cosPhi, sinTheta, sinPhi;
	cosTheta = cos(yaw * DEG2RAD), sinTheta = sin(yaw * DEG2RAD);
	cosPhi = cos(pitch * DEG2RAD), sinPhi = sin(pitch * DEG2RAD);
	N.z = cosTheta * sinPhi;
	N.x = sinTheta * sinPhi;
	N.y = cosPhi;
	N.w = 0;

	// This transform chain will always transform (x,y,z) into (0, 1, 0):
	// To make an orthonormal basis, we need x+ and z+
	N = rotMatrixPitch * rotMatrixYaw * N;
	B.x = 0; B.y = 0; B.z = 1; B.w = 0;
	T.x = 1; T.y = 0; T.z = 0; T.w = 0;
	// In this space we now need to rotate B and T around the Y axis by "roll" degrees
	B = rotMatrixRoll * B;
	T = rotMatrixRoll * T;
	// Our new basis is T,B,N; but we need to invert the yaw/pitch rotation we applied
	rotMatrixFull = rotMatrixPitch * rotMatrixYaw;
	rotMatrixFull.invert();
	T = rotMatrixFull * T;
	B = rotMatrixFull * B;
	N = rotMatrixFull * N;
	// Our TBN basis is now in absolute coordinates
	rotMatrixFull.identity();
	rotMatrixFull.set(
		T.x, B.x, N.x, 0,
		T.y, B.y, N.y, 0,
		T.z, B.z, N.z, 0,
		0, 0, 0, 1
	);
	// We further rotate the light vectors by 180 degrees so that Y+ points up:
	Matrix4 rot;
	rot.rotateX(180.0f);
	rotMatrixFull = rotMatrixFull * rot;

	for (int i = 0; i < num_lights; i++)
		tmpL[i] = rotMatrixFull * g_LightVector[i];

	// tmpL is now the light's direction in view space. We need to further transform this
	// to compensate for camera rotation

	// TODO: Switch between cockpit and external cameras -- apply the external camera rotation
	float viewYaw, viewPitch;
	if (PlayerDataTable[*g_playerIndex].Camera.ExternalCamera) {
		viewYaw   = PlayerDataTable[*g_playerIndex].Camera.Yaw   / 65536.0f * 360.0f;
		viewPitch = PlayerDataTable[*g_playerIndex].Camera.Pitch / 65536.0f * 360.0f;
	}
	else {
		viewYaw   = PlayerDataTable[*g_playerIndex].cockpitCameraYaw   / 65536.0f * 360.0f;
		viewPitch = PlayerDataTable[*g_playerIndex].cockpitCameraPitch / 65536.0f * 360.0f;
	}
	Matrix4 viewMatrixYaw, viewMatrixPitch, viewMatrixFull;
	viewMatrixYaw.identity();
	viewMatrixPitch.identity();
	viewMatrixYaw.rotateY(g_fViewYawSign   * viewYaw);
	viewMatrixYaw.rotateX(g_fViewPitchSign * viewPitch);
	viewMatrixFull = viewMatrixPitch * viewMatrixYaw;
	for (int i = 0; i < num_lights; i++)
		light[i] = viewMatrixFull * tmpL[i];

	//log_debug("[DBG] [AO] ypr: (%0.3f, %0.3f, %0.3f); sph: [%0.3f, %0.3f, %0.3f], pos: [%0.3f, %0.3f, %0.3f]",
	//	yaw, pitch, roll, x, y, z, tmp.x, tmp.y, tmp.z);
	//log_debug("[DBG] [AO] ypr: (%0.3f, %0.3f, %0.3f); T: [%0.3f, %0.3f, %0.3f], B: [%0.3f, %0.3f, %0.3f], N: [%0.3f, %0.3f, %0.3f]",
	//	yaw, pitch, roll, T.x, T.y, T.z, B.x, B.y, B.z, N.x, N.y, N.z);
	//log_debug("[DBG] [AO] ypr: (%0.3f, %0.3f, %0.3f); light: [%0.3f, %0.3f, %0.3f]",
	//	yaw, pitch, roll, light->x, light->y, light->z);
	//log_debug("[DBG] [AO] ypr: (%0.3f, %0.3f, %0.3f); camYP: (%0.3f, %0.3f), light: [%0.3f, %0.3f, %0.3f]",
	//	yaw, pitch, roll, viewYaw, viewPitch, light->x, light->y, light->z);
	// DEBUG

	//log_debug("[DBG] [AO] ypr: (%0.3f, %0.3f, %0.3f), L: [%0.3f, %0.3f, %0.3f]",
	//	yaw, pitch, roll, light->x, light->y, light->z);

	//log_debug("[DBG] [AO] ypr: (%0.3f, %0.3f, %0.3f); light: [%0.3f, %0.3f, %0.3f]",
	//	yaw, pitch, roll, tmplight.x, tmplight.y, tmplight.z);

	//log_debug("[DBG] [AO] XwaGlobalLightsCount: %d", *XwaGlobalLightsCount);
}
*/

void GetFakeYawPitchRollFromKeyboard(float *yaw, float *pitch, float *roll) {
	static float fake_yaw = 0.0f, fake_pitch = 0.0f, fake_roll = 0.0f;
	bool LeftKey = (GetAsyncKeyState(VK_LEFT) & 0x8000) == 0x8000;
	bool RightKey = (GetAsyncKeyState(VK_RIGHT) & 0x8000) == 0x8000;
	bool UpKey = (GetAsyncKeyState(VK_UP) & 0x8000) == 0x8000;
	bool DownKey = (GetAsyncKeyState(VK_DOWN) & 0x8000) == 0x8000;

	if (LeftKey)
		fake_yaw -= 1.0f;
	if (RightKey)
		fake_yaw += 1.0f;

	if (UpKey)
		fake_pitch += 1.0f;
	if (DownKey)
		fake_pitch -= 1.0f;

	*yaw = fake_yaw;
	*pitch = fake_pitch;
	*roll = fake_roll;
}

/*
 * Returns the current yaw, pitch in PlayerDataTable in degrees
 */
void GetFakeYawPitchRollFromMouseLook(float *yaw, float *pitch, float *roll) {
	*yaw   = -(float)PlayerDataTable[*g_playerIndex].MousePositionX / 65536.0f * 360.0f;
	*pitch =  (float)PlayerDataTable[*g_playerIndex].MousePositionY / 65536.0f * 360.0f;
	*roll  =  0.0f;
}

struct MainVertex
{
	float pos[2];
	float tex[2];
	MainVertex() {}
	MainVertex(float x, float y, float tx, float ty) {
		pos[0] = x; pos[1] = y;
		tex[0] = tx; tex[1] = ty;
	}
};

// void capture()
//#ifdef DBR_VR
void PrimarySurface::capture(int time_delay, ComPtr<ID3D11Texture2D> buffer, const wchar_t *filename)
{
	bool bDoCapture = false;
	HRESULT hr = D3D_OK;

	hr = DirectX::SaveWICTextureToFile(this->_deviceResources->_d3dDeviceContext.Get(),
		buffer.Get(), GUID_ContainerFormatJpeg, filename);

	if (SUCCEEDED(hr))
		log_debug("[DBG] CAPTURED (1)!");
	else
		log_debug("[DBG] NOT captured, hr: %d", hr);
}

static bool g_PrimarySurfaceInitialized = false;

PrimarySurface::PrimarySurface(DeviceResources* deviceResources, bool hasBackbufferAttached)
{
	this->_refCount = 1;
	this->_deviceResources = deviceResources;

	this->_hasBackbufferAttached = hasBackbufferAttached;

	if (this->_hasBackbufferAttached)
	{
		*this->_backbufferSurface.GetAddressOf() = new BackbufferSurface(this->_deviceResources);
		this->_deviceResources->_backbufferSurface = this->_backbufferSurface;
	}

	this->_flipFrames = 0;

	g_PrimarySurfaceInitialized = true;
	InitHeadingMatrix();
}

PrimarySurface::~PrimarySurface()
{
	if (this->_hasBackbufferAttached)
	{
		for (ULONG ref = this->_backbufferSurface->AddRef(); ref > 1; ref--)
		{
			this->_backbufferSurface->Release();
		}
	}

	if (this->_deviceResources->_primarySurface == this)
	{
		this->_deviceResources->_primarySurface = nullptr;
	}

	g_PrimarySurfaceInitialized = false;
	this->RenderText();
	this->RenderRadar();
	this->RenderBracket();
	this->RenderSynthDCElems();
}

HRESULT PrimarySurface::QueryInterface(
	REFIID riid,
	LPVOID* obp
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tE_NOINTERFACE");
	LogText(str.str());
#endif

	return E_NOINTERFACE;
}

ULONG PrimarySurface::AddRef()
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	this->_refCount++;

#if LOGGER
	str.str("");
	str << "\t" << this->_refCount;
	LogText(str.str());
#endif

	return this->_refCount;
}

ULONG PrimarySurface::Release()
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	this->_refCount--;

#if LOGGER
	str.str("");
	str << "\t" << this->_refCount;
	LogText(str.str());
#endif

	if (this->_refCount == 0)
	{
		delete this;
		return 0;
	}

	return this->_refCount;
}

HRESULT PrimarySurface::AddAttachedSurface(
	LPDIRECTDRAWSURFACE lpDDSAttachedSurface
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::AddOverlayDirtyRect(
	LPRECT lpRect
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::Blt(
	LPRECT lpDestRect,
	LPDIRECTDRAWSURFACE lpDDSrcSurface,
	LPRECT lpSrcRect,
	DWORD dwFlags,
	LPDDBLTFX lpDDBltFx
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	str << tostr_RECT(lpDestRect);

	if (lpDDSrcSurface == this->_deviceResources->_backbufferSurface)
	{
		str << " BackbufferSurface";
	}
	else
	{
		str << " " << lpDDSrcSurface;
	}

	str << tostr_RECT(lpSrcRect);

	if ((dwFlags & DDBLT_COLORFILL) != 0 && lpDDBltFx != nullptr)
	{
		str << " " << (void*)lpDDBltFx->dwFillColor;
	}

	LogText(str.str());
#endif

	if (dwFlags & DDBLT_COLORFILL)
	{
		if (lpDDBltFx == nullptr)
		{
			return DDERR_INVALIDPARAMS;
		}

		return DD_OK;
	}

	if (lpDDSrcSurface != nullptr)
	{
		if (lpDDSrcSurface == this->_deviceResources->_backbufferSurface)
		{
			return this->Flip(NULL, 0);
		}
	}

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::BltBatch(
	LPDDBLTBATCH lpDDBltBatch,
	DWORD dwCount,
	DWORD dwFlags
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::BltFast(
	DWORD dwX,
	DWORD dwY,
	LPDIRECTDRAWSURFACE lpDDSrcSurface,
	LPRECT lpSrcRect,
	DWORD dwTrans
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	str << " " << dwX << " " << dwY;

	if (lpDDSrcSurface == nullptr)
	{
		str << " NULL";
	}
	else if (lpDDSrcSurface == this->_deviceResources->_backbufferSurface)
	{
		str << " BackbufferSurface";
	}
	else
	{
		str << " " << lpDDSrcSurface;
	}

	str << tostr_RECT(lpSrcRect);

	if (dwTrans & DDBLTFAST_SRCCOLORKEY)
	{
		str << " SRCCOLORKEY";
	}

	if (dwTrans & DDBLTFAST_DESTCOLORKEY)
	{
		str << " DESTCOLORKEY";
	}

	if (dwTrans & DDBLTFAST_WAIT)
	{
		str << " WAIT";
	}

	if (dwTrans & DDBLTFAST_DONOTWAIT)
	{
		str << " DONOTWAIT";
	}

	LogText(str.str());
#endif

	if (lpDDSrcSurface != nullptr)
	{
		if (lpDDSrcSurface == this->_deviceResources->_backbufferSurface)
		{
			return this->Flip(this->_deviceResources->_backbufferSurface, 0);
		}
	}

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::DeleteAttachedSurface(
	DWORD dwFlags,
	LPDIRECTDRAWSURFACE lpDDSAttachedSurface
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::EnumAttachedSurfaces(
	LPVOID lpContext,
	LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::EnumOverlayZOrders(
	DWORD dwFlags,
	LPVOID lpContext,
	LPDDENUMSURFACESCALLBACK lpfnCallback
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

void PrimarySurface::SetScissoRectFullScreen()
{
	auto& context = this->_deviceResources->_d3dDeviceContext;

	D3D11_RECT rect;
	rect.left = 0; rect.top = 0;
	rect.right = (LONG)g_fCurScreenWidth; rect.bottom = (LONG)g_fCurScreenHeight;
	context->RSSetScissorRects(1, &rect);
}

void PrimarySurface::SaveContext()
{
	auto &context = _deviceResources->_d3dDeviceContext;

	_oldVSCBuffer = g_VSCBuffer;
	_oldPSCBuffer = g_PSCBuffer;
	_oldDCPSCBuffer = g_DCPSCBuffer;

	context->VSGetConstantBuffers(0, 1, _oldVSConstantBuffer.GetAddressOf());
	context->PSGetConstantBuffers(0, 1, _oldPSConstantBuffer.GetAddressOf());

	context->VSGetShaderResources(0, 3, _oldVSSRV[0].GetAddressOf());
	context->PSGetShaderResources(0, 13, _oldPSSRV[0].GetAddressOf());

	context->VSGetShader(_oldVertexShader.GetAddressOf(), nullptr, nullptr);
	// TODO: Use GetCurrentPixelShader here instead of PSGetShader and do *not* Release()
	// _oldPixelShader in RestoreContext
	context->PSGetShader(_oldPixelShader.GetAddressOf(), nullptr, nullptr);

	context->PSGetSamplers(0, 2, _oldPSSamplers[0].GetAddressOf());

	context->OMGetRenderTargets(8, _oldRTVs[0].GetAddressOf(), _oldDSV.GetAddressOf());
	context->OMGetDepthStencilState(_oldDepthStencilState.GetAddressOf(), &_oldStencilRef);
	context->OMGetBlendState(_oldBlendState.GetAddressOf(), _oldBlendFactor, &_oldSampleMask);

	context->IAGetInputLayout(_oldInputLayout.GetAddressOf());
	context->IAGetPrimitiveTopology(&_oldTopology);
	context->IAGetVertexBuffers(0, 1, _oldVertexBuffer.GetAddressOf(), &_oldStride, &_oldOffset);
	context->IAGetIndexBuffer(_oldIndexBuffer.GetAddressOf(), &_oldFormat, &_oldIOffset);

	_oldNumViewports = 2;
	context->RSGetViewports(&_oldNumViewports, _oldViewports);
}

void PrimarySurface::RestoreContext()
{
	auto &resources = _deviceResources;
	auto &context = _deviceResources->_d3dDeviceContext;

	// Restore a previously-saved context
	g_VSCBuffer = _oldVSCBuffer;
	g_PSCBuffer = _oldPSCBuffer;
	g_DCPSCBuffer = _oldDCPSCBuffer;
	resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
	resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
	resources->InitPSConstantBufferDC(resources->_PSConstantBufferDC.GetAddressOf(), &g_DCPSCBuffer);

	// The hyperspace effect needs the current VS constants to work properly
	if (g_HyperspacePhaseFSM == HS_INIT_ST)
		context->VSSetConstantBuffers(0, 1, _oldVSConstantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _oldPSConstantBuffer.GetAddressOf());

	context->VSSetShaderResources(0, 3, _oldVSSRV[0].GetAddressOf());
	context->PSSetShaderResources(0, 13, _oldPSSRV[0].GetAddressOf());

	// It's important to use the Init*Shader methods here, or the shaders won't be
	// applied sometimes.
	resources->InitVertexShader(_oldVertexShader);
	resources->InitPixelShader(_oldPixelShader);

	context->PSSetSamplers(0, 2, _oldPSSamplers[0].GetAddressOf());
	context->OMSetRenderTargets(8, _oldRTVs[0].GetAddressOf(), _oldDSV.Get());
	context->OMSetDepthStencilState(_oldDepthStencilState.Get(), _oldStencilRef);
	context->OMSetBlendState(_oldBlendState.Get(), _oldBlendFactor, _oldSampleMask);

	resources->InitInputLayout(_oldInputLayout);
	resources->InitTopology(_oldTopology);
	context->IASetVertexBuffers(0, 1, _oldVertexBuffer.GetAddressOf(), &_oldStride, &_oldOffset);
	context->IASetIndexBuffer(_oldIndexBuffer.Get(), _oldFormat, _oldIOffset);

	context->RSSetViewports(_oldNumViewports, _oldViewports);

	// Release everything. Previous calls to *Get* increase the refcount
	_oldVSConstantBuffer.Release();
	_oldPSConstantBuffer.Release();

	for (int i = 0; i < 3; i++)
		_oldVSSRV[i].Release();
	for (int i = 0; i < 13; i++)
		_oldPSSRV[i].Release();

	_oldVertexShader.Release();
	_oldPixelShader.Release();

	for (int i = 0; i < 2; i++)
		_oldPSSamplers[i].Release();

	for (int i = 0; i < 8; i++)
		_oldRTVs[i].Release();
	_oldDSV.Release();
	_oldDepthStencilState.Release();
	_oldBlendState.Release();
	_oldInputLayout.Release();
	_oldVertexBuffer.Release();
	_oldIndexBuffer.Release();
}

/*
 * Applies the barrel distortion effect on the 2D window (Concourse, menus, etc).
 */
void PrimarySurface::barrelEffect2D(int iteration) {
	/*
	We need to avoid resolving the offscreen buffer multiple times. It's probably easier to
	skip method this altogether if we already rendered all this in the first iteration.
	*/
	if (iteration > 0)
		return;

	D3D11_VIEWPORT viewport{};
	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;

	// Temporarily disable ZWrite: we won't need it for the barrel effect
	D3D11_DEPTH_STENCIL_DESC desc;
	ComPtr<ID3D11DepthStencilState> depthState;
	desc.DepthEnable = FALSE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	desc.StencilEnable = FALSE;
	resources->InitDepthStencilState(depthState, &desc);

	float screen_res_x = (float)resources->_backbufferWidth;
	float screen_res_y = (float)resources->_backbufferHeight;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	// Set the vertex buffer
	UINT stride = sizeof(MainVertex);
	UINT offset = 0;
	resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);

	// Set Primitive Topology
	// This is probably an opportunity for an optimization: let's use the same topology everywhere?
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitInputLayout(resources->_mainInputLayout);

	// At this point, offscreenBuffer contains the image that would be rendered to the screen by
	// copying to the backbuffer. So, resolve the offscreen buffer into offscreenBuffer2 to use it
	// as input, so that we can render the image twice.
	context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer,
		0, BACKBUFFER_FORMAT);

#ifdef DBG_VR
	if (g_bCapture2DOffscreenBuffer) {
		static int frame = 0;
		if (frame == 0) {
			log_debug("[DBG] [Capture] display Width, Height: %d, %d",
				this->_deviceResources->_displayWidth, this->_deviceResources->_displayHeight);
			log_debug("[DBG] [Capture] backBuffer Width, Height: %d, %d",
				this->_deviceResources->_backbufferWidth, this->_deviceResources->_backbufferHeight);
		}
		wchar_t filename[120];
		swprintf_s(filename, 120, L"c:\\temp\\offscreenBuf-%d-A.jpg", frame++);
		capture(0, this->_deviceResources->_offscreenBuffer2, filename);
		if (frame >= 40)
			g_bCapture2DOffscreenBuffer = false;
	}
#endif

	// Render the barrel effect
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	if (!g_b3DVisionEnabled) {
		viewport.Width = screen_res_x;
		viewport.Height = screen_res_y;
	}
	else {
		viewport.Width = screen_res_x * 2.0f;
		viewport.Height = screen_res_y + 1;
	}
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	viewport.MinDepth = D3D11_MIN_DEPTH;

	resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(), 0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices
	resources->InitVertexShader(resources->_mainVertexShaderVR);
	if (!g_b3DVisionEnabled)
		resources->InitPixelShader(resources->_barrelPixelShader);
	else
		resources->InitPixelShader(resources->_simpleResizePS);

	if (!g_b3DVisionEnabled) {
		// Set the lens distortion constants for the barrel shader
		resources->InitPSConstantBufferBarrel(resources->_barrelConstantBuffer.GetAddressOf(), g_fLensK1, g_fLensK2, g_fLensK3);
	} 
	else {
		// When 3D vision is enabled, k1 is the v coordinate where the signature must be added
		float v = 1.0f - 1.0f / viewport.Height;
		resources->InitPSConstantBufferBarrel(resources->_barrelConstantBuffer.GetAddressOf(), v, 0, 0);
	}

	if (!g_b3DVisionEnabled) {
		context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		ID3D11RenderTargetView *rtvs[5] = {
			resources->_renderTargetViewPost.Get(),
			NULL, NULL, NULL, NULL,
		};
		context->OMSetRenderTargets(5, rtvs, NULL);
	}
	else {
		context->ClearRenderTargetView(resources->_RTVvision3DPost, bgColor);
		ID3D11RenderTargetView *rtvs[5] = {
			resources->_RTVvision3DPost.Get(),
			NULL, NULL, NULL, NULL,
		};
		context->OMSetRenderTargets(5, rtvs, NULL);
	}
	resources->InitPSShaderResourceView(resources->_offscreenAsInputShaderResourceView);
	if (g_b3DVisionEnabled) {
		context->PSSetShaderResources(1, 1, resources->_vision3DSignatureSRV.GetAddressOf());
		context->PSSetSamplers(1, 1, resources->_noInterpolationSamplerState.GetAddressOf());
		// To properly blend the signature in _vision3DSignature, we need to disable transparency:
		resources->InitBlendState(resources->_mainBlendState, nullptr);
	}

	resources->InitViewport(&viewport);
	context->Draw(6, 0);

	// Restore the original rendertargetview
	context->OMSetRenderTargets(1, this->_deviceResources->_renderTargetView.GetAddressOf(), this->_deviceResources->_depthStencilViewL.Get());
	if (g_b3DVisionEnabled) {
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width = screen_res_x;
		viewport.Height = screen_res_y;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		resources->InitViewport(&viewport);
		context->PSSetSamplers(1, 1, resources->_mainSamplerState.GetAddressOf());
	}
}

/*
 * Applies the barrel distortion effect on the 3D window.
 * Resolves the offscreenBuffer into offscreenBufferAsInput
 * Renders to offscreenBufferPost
 */
void PrimarySurface::barrelEffect3D() {
	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;

	// Set the vertex buffer
	UINT stride = sizeof(MainVertex);
	UINT offset = 0;
	resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);

	// Set Primitive Topology
	// Opportunity for optimization? Make all draw calls use the same topology?
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitInputLayout(resources->_mainInputLayout);
	
	// Temporarily disable ZWrite: we won't need it for the barrel effect
	D3D11_DEPTH_STENCIL_DESC desc;
	ComPtr<ID3D11DepthStencilState> depthState;
	desc.DepthEnable = FALSE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	desc.StencilEnable = FALSE;
	resources->InitDepthStencilState(depthState, &desc);
	
	float screen_res_x = (float)resources->_backbufferWidth;
	float screen_res_y = (float)resources->_backbufferHeight;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	// Create a new viewport to render the offscreen buffer as a texture
	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	if (!g_b3DVisionEnabled) {
		viewport.Width = screen_res_x;
		viewport.Height = screen_res_y;
	}
	else {
		viewport.Width = screen_res_x * 2.0f;
		viewport.Height = screen_res_y + 1;
	}
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	
	context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer,
		0, BACKBUFFER_FORMAT);

	resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(), 0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices
	resources->InitVertexShader(resources->_mainVertexShaderVR);
	if (!g_b3DVisionEnabled)
		resources->InitPixelShader(resources->_barrelPixelShader);
	else
		resources->InitPixelShader(resources->_simpleResizePS);
	
	if (!g_b3DVisionEnabled) {
		// Set the lens distortion constants for the barrel shader
		resources->InitPSConstantBufferBarrel(resources->_barrelConstantBuffer.GetAddressOf(), g_fLensK1, g_fLensK2, g_fLensK3);
	}
	else {
		// When 3D vision is enabled, k1 is the v coordinate where the signature must be added
		float v = 1.0f - 1.0f / viewport.Height;
		resources->InitPSConstantBufferBarrel(resources->_barrelConstantBuffer.GetAddressOf(), v, 0, 0);
	}

	// Clear the render target
	if (!g_b3DVisionEnabled) {
		context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		ID3D11RenderTargetView *rtvs[5] = {
			resources->_renderTargetViewPost.Get(),
			NULL, NULL, NULL, NULL,
		};
		context->OMSetRenderTargets(5, rtvs, NULL);
	}
	else {
		context->ClearRenderTargetView(resources->_RTVvision3DPost, bgColor);
		ID3D11RenderTargetView *rtvs[5] = {
			resources->_RTVvision3DPost.Get(),
			NULL, NULL, NULL, NULL,
		};
		context->OMSetRenderTargets(5, rtvs, NULL);
	}
	context->PSSetShaderResources(0, 1, resources->_offscreenAsInputShaderResourceView.GetAddressOf());
	if (g_b3DVisionEnabled) {
		context->PSSetShaderResources(1, 1, resources->_vision3DSignatureSRV.GetAddressOf());
		context->PSSetSamplers(1, 1, resources->_noInterpolationSamplerState.GetAddressOf());
		// To properly blend the signature in _vision3DSignature, we need to disable transparency:
		resources->InitBlendState(resources->_mainBlendState, nullptr);
	}
	resources->InitViewport(&viewport);
	context->IASetInputLayout(resources->_mainInputLayout);
	context->Draw(6, 0);

#ifdef DBG_VR
	if (g_bCapture2DOffscreenBuffer) {
		static int frame = 0;
		wchar_t filename[120];

		swprintf_s(filename, 120, L"c:\\temp\\offscreenBuf-%d.jpg", frame);
		capture(0, this->_deviceresources->_offscreenBuffer, filename);

		swprintf_s(filename, 120, L"c:\\temp\\offscreenBuf2-%d.jpg", frame);
		capture(0, this->_deviceResources->_offscreenBuffer2, filename);

		swprintf_s(filename, 120, L"c:\\temp\\offscreenBuf3-%d.jpg", frame);
		capture(0, this->_deviceResources->_offscreenBuffer3, filename);

		frame++;
		if (frame >= 5)
			g_bCapture2DOffscreenBuffer = false;
	}
#endif

	// Restore previous rendertarget, etc
	resources->InitInputLayout(resources->_inputLayout);
	context->OMSetRenderTargets(1, this->_deviceResources->_renderTargetView.GetAddressOf(), 
		this->_deviceResources->_depthStencilViewL.Get());
	if (g_b3DVisionEnabled) {
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width = screen_res_x;
		viewport.Height = screen_res_y;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		resources->InitViewport(&viewport);
		context->PSSetSamplers(1, 1, resources->_mainSamplerState.GetAddressOf());
	}
}

/*
 * Applies the barrel distortion effect on the 3D window for SteamVR (so each image is
 * independent and the singleBarrelPixelShader is used.
 * Input: _offscreenBuffer and _offscreenBufferR
 * Output: _offscreenBufferPost and _offscreenBufferPostR
 */
// TODO: Remove, it is not necessary anymore as barrel distortion is done by SteamVR
void PrimarySurface::barrelEffectSteamVR() {
	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;

	// Set the vertex buffer
	UINT stride = sizeof(MainVertex);
	UINT offset = 0;
	resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);

	// Set Primitive Topology
	// Opportunity for optimization? Make all draw calls use the same topology?
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitInputLayout(resources->_mainInputLayout);

	// Temporarily disable ZWrite: we won't need it for the barrel effect
	D3D11_DEPTH_STENCIL_DESC desc;
	ComPtr<ID3D11DepthStencilState> depthState;
	desc.DepthEnable = FALSE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	desc.StencilEnable = FALSE;
	resources->InitDepthStencilState(depthState, &desc);

	// Create a new viewport to render the offscreen buffer as a texture
	D3D11_VIEWPORT viewport{};
	float screen_res_x = (float)resources->_backbufferWidth;
	float screen_res_y = (float)resources->_backbufferHeight;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = (float)screen_res_x;
	viewport.Height = (float)screen_res_y;
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	resources->InitViewport(&viewport);

	// Resolve both images
	context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer,
		0, BACKBUFFER_FORMAT);
	context->ResolveSubresource(resources->_offscreenBufferAsInputR, 0, resources->_offscreenBufferR,
		0, BACKBUFFER_FORMAT);

#ifdef DBG_VR
	if (g_bCapture2DOffscreenBuffer) {
		static int frame = 0;
		wchar_t filename[120];

		log_debug("[DBG] Capturing buffers");
		swprintf_s(filename, 120, L"c:\\temp\\offscreenBuf-%d.jpg", frame);
		capture(0, this->_deviceresources->_offscreenBuffer, filename);

		swprintf_s(filename, 120, L"c:\\temp\\offscreenBufAsInput-%d.jpg", frame);
		capture(0, this->_deviceResources->_offscreenBufferAsInput, filename);

		frame++;
		if (frame >= 1)
			g_bCapture2DOffscreenBuffer = false;
	}
#endif

	resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(), 0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices
	resources->InitVertexShader(resources->_mainVertexShaderVR);
	resources->InitPixelShader(resources->_singleBarrelPixelShader);

	// Set the lens distortion constants for the barrel shader
	resources->InitPSConstantBufferBarrel(resources->_barrelConstantBuffer.GetAddressOf(), g_fLensK1, g_fLensK2, g_fLensK3);

	// Clear the render targets
	context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
	context->ClearRenderTargetView(resources->_renderTargetViewPostR, bgColor);
	context->IASetInputLayout(resources->_mainInputLayout);

	ID3D11RenderTargetView *rtvs[5] = {
		resources->_renderTargetViewPost.Get(),
		NULL, NULL, NULL, NULL,
	};
	context->OMSetRenderTargets(5, rtvs, NULL);
	//context->PSSetShaderResources(0, 1, resources->_offscreenAsInputShaderResourceView.GetAddressOf());
	resources->InitPSShaderResourceView(resources->_offscreenAsInputShaderResourceView);
	context->Draw(6, 0);

	rtvs[0] = resources->_renderTargetViewPostR.Get();
	context->OMSetRenderTargets(5, rtvs, NULL);
	//context->PSSetShaderResources(0, 1, resources->_offscreenAsInputShaderResourceViewR.GetAddressOf());
	resources->InitPSShaderResourceView(resources->_offscreenAsInputShaderResourceViewR);
	context->Draw(6, 0);

#ifdef DBG_VR
	if (g_bCapture2DOffscreenBuffer) {
		static int frame = 0;
		wchar_t filename[120];

		swprintf_s(filename, 120, L"c:\\temp\\offscreenBufR-%d.jpg", frame);
		capture(0, resources->_offscreenBufferR, filename);

		swprintf_s(filename, 120, L"c:\\temp\\offscreenBufAsInputR-%d.jpg", frame);
		capture(0, resources->_offscreenBufferAsInputR, filename);

		swprintf_s(filename, 120, L"c:\\temp\\offscreenBufPostR-%d.jpg", frame);
		capture(0, resources->_offscreenBufferPostR, filename);

		frame++;
		if (frame >= 1)
			g_bCapture2DOffscreenBuffer = false;
	}
#endif

	// Restore previous rendertarget, etc
	resources->InitInputLayout(resources->_inputLayout);
	context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(),
		resources->_depthStencilViewL.Get());
}

/*
 * When rendering for SteamVR, we may be rendering at a resolution that is different
 * from the current screen resolution, so some stretching will happen in the mirror
 * window. We need to resize the offscreenBuffer before presenting it.
 * Also, in 2D we need to generate a buffer with the proper aspect to copy into the VR overlay
 * Bonus points: Adjust the FOV to the original (~50) to avoid showing the distortion
 * caused by the larger FOV used in most headsets.
 * Input: _offscreenBuffer or _offscreenBufferHd if the HD Concourse is enabled.
 * Output: _steamVRPresentBuffer
 * Output: _steamVROverlayBuffer
 */
void PrimarySurface::resizeForSteamVR(int iteration, bool is_2D) {
	/*
	We need to avoid resolving the offscreen buffer multiple times. It's probably easier to
	skip method this altogether if we already rendered all this in the first iteration.
	*/
	if (iteration > 0)
		return;

	this->_deviceResources->BeginAnnotatedEvent(L"resizeForSteamVR");

	D3D11_VIEWPORT viewport{};
	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;

	float screen_res_x = (float)g_WindowWidth;
	float screen_res_y = (float)g_WindowHeight;
	float bgColor[4]   = { 0.0f, 0.0f, 0.0f, 0.0f };
	const bool bIsInConcourseHD = resources->IsInConcourseHd();
	const bool bInHdTechRoom = bIsInConcourseHD && g_bInTechRoom;

	// Set the vertex buffer
	UINT stride = sizeof(MainVertex);
	UINT offset = 0;
	resources->InitVertexBuffer(resources->_mainVertexBuffer.GetAddressOf(), &stride, &offset);
	resources->InitIndexBuffer(resources->_mainIndexBuffer, false);

	// Set Primitive Topology
	// This is probably an opportunity for an optimization: let's use the same topology everywhere?
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitInputLayout(resources->_mainInputLayout);

	// Temporarily disable ZWrite: we won't need it here
	D3D11_DEPTH_STENCIL_DESC desc;
	ComPtr<ID3D11DepthStencilState> depthState;
	desc.DepthEnable    = FALSE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc      = D3D11_COMPARISON_ALWAYS;
	desc.StencilEnable  = FALSE;
	resources->InitDepthStencilState(depthState, &desc);

	if (bInHdTechRoom)
	{
		context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer,
			0, BACKBUFFER_FORMAT);
		context->ResolveSubresource(
			resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
			resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
	}
	else
	{
		// At this point, offscreenBuffer/offscreenBufferHd contains the image that would be rendered
		// to the screen by copying to the backbuffer. So, resolve the offscreen buffer into
		// offscreenBufferAsInput to use it as input in the shader
		if (g_bSteamVRMirrorWindowLeftEye)
		{
			// If the HD Concourse is enabled, then offscreenBufferHd is already non-MSAA and SRV-ready
			if (!bIsInConcourseHD)
				context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer,
					0, BACKBUFFER_FORMAT);
		}
		else
		{
			if (!bIsInConcourseHD)
				context->ResolveSubresource(
					resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
					resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
		}
	}

#ifdef DBG_VR
	if (g_bCapture2DOffscreenBuffer) {
		static int frame = 0;
		if (frame == 0) {
			log_debug("[DBG] [Capture] display Width, Height: %d, %d",
				this->_deviceResources->_displayWidth, this->_deviceResources->_displayHeight);
			log_debug("[DBG] [Capture] backBuffer Width, Height: %d, %d",
				this->_deviceResources->_backbufferWidth, this->_deviceResources->_backbufferHeight);
			log_debug("[DBG] screen_res_x,y: %0.1f, %0.1f", screen_res_x, screen_res_y);
			log_debug("[DBG] g_steamVRWidth,Height: %d, %d", g_steamVRWidth, g_steamVRHeight);
			log_debug("[DBG] scale, newWidth,Height: %0.3f, (%0.3f, %0.3f)", scale, newWidth, newHeight);
		}
		wchar_t filename[120];
		swprintf_s(filename, 120, L"c:\\temp\\offscreenBuf-%d-A.jpg", frame++);
		capture(0, this->_deviceResources->_offscreenBufferAsInput, filename);
		if (frame >= 1)
			g_bCapture2DOffscreenBuffer = false;
	}
#endif

	/*******************************************************************************/
	/************** Resize to _steamVRPresentBuffer for the mirror window **********/
	/*******************************************************************************/

	// The viewport has to be in the range (0,0)-(g_steamVRWidth,g_steamVRHeight) or the image
	// will be cropped. This is because the backbuffer has those same dimensions.
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width    = (float)g_steamVRWidth;
	viewport.Height   = (float)g_steamVRHeight;
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	resources->InitViewport(&viewport);

	if (!g_bRendering3D && bIsInConcourseHD)
	{
		g_WindowWidth  = HD_CONCOURSE_WIDTH;
		g_WindowHeight = HD_CONCOURSE_HEIGHT;
	}
	float mirrorWindowAspectRatio = (float)g_WindowWidth / (float)g_WindowHeight;
	float steamVR_aspect_ratio = (float)g_steamVRWidth / (float)g_steamVRHeight;
	float window_factor_x = (float)g_steamVRWidth / (float)g_WindowWidth;
	// If the display window has height > width, we can't make the y axis smaller to compensate.
	// Instead, I'm expanding the x-axis to keep the original aspect ratio. That'll cause the image
	// to become bigger too, so we'll shrink the image by adjusting the scale as well:
	float window_factor_y = (float)g_WindowHeight / (float)g_steamVRHeight;
	float scale = 1.0f / window_factor_y;

	float window_scale_x = (float)g_WindowWidth / (float)g_steamVRWidth;
	float window_scale_y = (float)g_WindowHeight / (float)g_steamVRHeight;
	float window_scale = min(window_scale_x, window_scale_y);
	scale *= window_scale;
	
	float aspect_ratio = 1.0f;
	if (g_bRendering3D)
	{
		if (g_fSteamVRMirrorWindowAspectRatio > 0.01f)
			aspect_ratio = g_fSteamVRMirrorWindowAspectRatio;
		else			
			aspect_ratio = 1.0f / steamVR_aspect_ratio * window_factor_x * window_factor_y;
	}
	else
	{
		// The 2D image is rendered stretched over the full backbuffer (steamVR), but the aspect ratio will be modified when the backbuffer
		// gets squeezed into the window (usually 16:9). So effectively the 2D image will have Window aspect ratio.
		// We need to undo it and that's why we add (1/window aspect ratio) below:
		aspect_ratio = (1.0f / mirrorWindowAspectRatio) * (g_fConcourseAspectRatio);
	}

	// We have a problem here: the CB for the VS and PS are the same (_mainShadersConstantBuffer), so
	// we have to use the same settings on both.
	resources->InitPSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
		0.0f, aspect_ratio, scale, 1.0f, g_bRendering3D ? g_fSteamVRMirrorWindow3DScale : 1.0f);
	resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
		0.0f, aspect_ratio, scale, 1.0f, 0.0f); // Don't use 3D projection matrices
	resources->InitVertexShader(resources->_mainVertexShaderVR);
	resources->InitPixelShader(resources->_steamVRMirrorPixelShader);

	context->ClearRenderTargetView(resources->_renderTargetViewSteamVRResize, bgColor);
	/*context->OMSetRenderTargets(1, resources->_renderTargetViewSteamVRResize.GetAddressOf(),
		resources->_depthStencilViewL.Get());*/
	context->OMSetRenderTargets(1, resources->_renderTargetViewSteamVRResize.GetAddressOf(),nullptr);
	if (g_bSteamVRMirrorWindowLeftEye)
	{
		if (!g_bRendering3D && bIsInConcourseHD)
			resources->InitPSShaderResourceView(resources->_offscreenBufferHdSRV);
		else
			resources->InitPSShaderResourceView(resources->_offscreenAsInputShaderResourceView);
	}
	else
	{
		if (!g_bRendering3D && bIsInConcourseHD)
			resources->InitPSShaderResourceView(resources->_offscreenBufferHdSRV);
		else
			resources->InitPSShaderResourceView(resources->_offscreenAsInputShaderResourceViewR);
	}
	if (bInHdTechRoom)
		context->PSSetShaderResources(1, 1, resources->_offscreenAsInputShaderResourceView.GetAddressOf());
	context->DrawIndexed(6, 0, 0);

	/*******************************************************************************/
	/********* Resize to _steamVROverlayBuffer for the VR overlay in the HMD *******/
	/*******************************************************************************/
	{
		if (!g_bRendering3D) // Only render for 2D content, no need to waste resources in 3D
		{
			aspect_ratio = g_fConcourseAspectRatio * (1.0f / steamVR_aspect_ratio);
			scale = 1.0f / aspect_ratio;
			resources->InitPSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
				0.0f, aspect_ratio, scale, 1.0f, 1.0f);
			resources->InitPixelShader(resources->_steamVRMirrorPixelShader);

			context->ClearRenderTargetView(resources->_renderTargetViewSteamVROverlayResize, bgColor);
			/*context->OMSetRenderTargets(1, resources->_renderTargetViewSteamVROverlayResize.GetAddressOf(),
				resources->_depthStencilViewL.Get());*/
			context->OMSetRenderTargets(1, resources->_renderTargetViewSteamVROverlayResize.GetAddressOf(), nullptr);
			context->DrawIndexed(6, 0, 0);
		}
	}

#ifdef DISABLED
	/*******************************************************************************/
	/********* Resize to _steamVROverlayBuffer for the VR overlay in the HMD *******/
	/*******************************************************************************/
	{
		if (!g_bRendering3D) // Only render for 2D content, no need to waste resources in 3D
		{
			aspect_ratio = g_fConcourseAspectRatio * (1.0f / steamVR_aspect_ratio);
			scale = 1.0f / aspect_ratio;
			resources->InitPSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
				0.0f, aspect_ratio, scale, 1.0f, 1.0f);
			resources->InitPixelShader(resources->_steamVRMirrorPixelShader);

			context->ClearRenderTargetView(resources->_renderTargetViewSteamVROverlayResize, bgColor);
			context->OMSetRenderTargets(1, resources->_renderTargetViewSteamVROverlayResize.GetAddressOf(),	NULL);
			context->DrawIndexed(6, 0, 0);
		}
	}
#endif

#ifdef DBG_VR
	if (g_bCapture2DOffscreenBuffer) {
		static int frame = 0;
		wchar_t filename[120];
		swprintf_s(filename, 120, L"c:\\temp\\offscreenBuf-%d.jpg", frame);
		capture(0, this->_deviceresources->_offscreenBuffer, filename);

		swprintf_s(filename, 120, L"c:\\temp\\offscreenBufAsInput-%d.jpg", frame);
		capture(0, this->_deviceResources->_offscreenBufferAsInput, filename);

		swprintf_s(filename, 120, L"c:\\temp\\steamVRPresentBuf-%d.jpg", frame);
		capture(0, this->_deviceResources->_steamVRPresentBuffer, filename);

		log_debug("[DBG] viewport: (%f,%f)-(%f,%f)", viewport.TopLeftX, viewport.TopLeftY,
			viewport.Width, viewport.Height);

		frame++;
		if (frame >= 0)
			g_bCapture2DOffscreenBuffer = false;
	}
#endif

	// Restore previous rendertarget, etc
	context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(),
		resources->_depthStencilViewL.Get());

	this->_deviceResources->EndAnnotatedEvent();
}

 /*
  * Applies a single bloom effect pass. Depending on the input:
  * pass = 0: Initial horizontal blur pass from the bloom mask.
  * pass = 1: Vertical pass from internal temporary ping-pong buffer.
  * pass = 2: Horizontal pass from internal temporary ping-pong buffer.
  * pass = 3: Final combine pass (deprecated, used for 32-bit UNORM mode)
  * pass = 4: Linear add between current bloom pass and bloomSum
  * pass = 5: Final combine between bloomSum and offscreenBuffer
  *
  * For pass 0, the input texture must be resolved already to 
  * _offscreenBufferAsInputReshadeMask, _offscreenBufferAsInputReshadeMaskR
  */
void PrimarySurface::BloomBasicPass(int pass, float fZoomFactor) {

	//this->_deviceResources->BeginAnnotatedEvent(L"BloomBasicPass");
	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;

	// Set the vertex buffer... we probably need another vertex buffer here
	UINT stride = sizeof(MainVertex);
	UINT offset = 0;
	resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);

	// Set Primitive Topology
	// Opportunity for optimization? Make all draw calls use the same topology?
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitInputLayout(resources->_mainInputLayout);

	// Temporarily disable ZWrite: we won't need it for this effect
	D3D11_DEPTH_STENCIL_DESC desc;
	ComPtr<ID3D11DepthStencilState> depthState;
	desc.DepthEnable = FALSE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	desc.StencilEnable = FALSE;
	resources->InitDepthStencilState(depthState, &desc);

	// Create a new viewport to render the offscreen buffer as a texture
	float screen_res_x = (float)resources->_backbufferWidth;
	float screen_res_y = (float)resources->_backbufferHeight;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	if (pass < 3) {
		viewport.Width  = screen_res_x / fZoomFactor;
		viewport.Height = screen_res_y / fZoomFactor;
	} else { // The final pass should be performed at full resolution
		viewport.Width  = screen_res_x;
		viewport.Height = screen_res_y;
	}

	viewport.MaxDepth = D3D11_MAX_DEPTH;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	resources->InitViewport(&viewport);

	// Set the constant buffers
	resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
		0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices
	context->IASetInputLayout(resources->_mainInputLayout);
	resources->InitVertexShader(g_bUseSteamVR ? resources->_mainVertexShaderVR : resources->_mainVertexShader);

	// The input texture must be resolved already to
	// _offscreenBufferAsInputReshadeMask, _offscreenBufferAsInputReshadeMaskR
	switch (pass) {
		case 0: // Horizontal Gaussian Blur
			// Input: _offscreenAsInputReshadeSRV
			// Output _bloomOutput1
			resources->InitPixelShader(g_bUseSteamVR ? resources->_bloomHGaussPS_VR : resources->_bloomHGaussPS);
			context->PSSetShaderResources(0, 1, resources->_offscreenAsInputBloomMaskSRV.GetAddressOf());
			context->ClearRenderTargetView(resources->_renderTargetViewBloom1, bgColor);
			context->OMSetRenderTargets(1, resources->_renderTargetViewBloom1.GetAddressOf(), NULL);
			break;
		case 1: // Vertical Gaussian Blur
			// Input:  _bloomOutput1
			// Output: _bloomOutput2
			resources->InitPixelShader(g_bUseSteamVR ? resources->_bloomVGaussPS_VR : resources->_bloomVGaussPS);
			context->PSSetShaderResources(0, 1, resources->_bloomOutput1SRV.GetAddressOf());
			context->ClearRenderTargetView(resources->_renderTargetViewBloom2, bgColor);
			context->OMSetRenderTargets(1, resources->_renderTargetViewBloom2.GetAddressOf(), NULL);
			break;
		case 2: // Horizontal Gaussian Blur
			// Input:  _bloomOutput2
			// Output: _bloomOutput1
			resources->InitPixelShader(g_bUseSteamVR ? resources->_bloomHGaussPS_VR : resources->_bloomHGaussPS);
			context->PSSetShaderResources(0, 1, resources->_bloomOutput2SRV.GetAddressOf());
			context->ClearRenderTargetView(resources->_renderTargetViewBloom1, bgColor);
			context->OMSetRenderTargets(1, resources->_renderTargetViewBloom1.GetAddressOf(), NULL);
			break;
		case 3: // Final pass to combine the bloom texture with the offscreenBuffer
			// Input:  _bloomOutput2, _offscreenBufferAsInput
			// Output: _offscreenBuffer (_bloomOutput1?)
			resources->InitPixelShader(g_bUseSteamVR ? resources->_bloomCombinePS_VR : resources->_bloomCombinePS);
			context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer,
				0, BACKBUFFER_FORMAT);
			if (g_bUseSteamVR) {
				context->ResolveSubresource(
					resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
					resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
			}
			context->PSSetShaderResources(0, 1, resources->_offscreenAsInputShaderResourceView.GetAddressOf());
			context->PSSetShaderResources(1, 1, resources->_bloomOutput2SRV.GetAddressOf());
			context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(), NULL);
			break;
		case 4:
			// Input: _bloomOutput2, _bloomSum
			// Output: _bloomOutput1
			resources->InitPixelShader(g_bUseSteamVR ? resources->_bloomBufferAddPS_VR : resources->_bloomBufferAddPS);
			context->PSSetShaderResources(0, 1, resources->_bloomOutput2SRV.GetAddressOf());
			context->PSSetShaderResources(1, 1, resources->_bloomOutputSumSRV.GetAddressOf());
			context->ClearRenderTargetView(resources->_renderTargetViewBloom1, bgColor);
			context->OMSetRenderTargets(1, resources->_renderTargetViewBloom1.GetAddressOf(), NULL);
			break;
		case 5: // Final pass to combine the bloom accumulated texture with the offscreenBuffer
			// Input:  _bloomSum, _offscreenBufferAsInput
			// Output: _offscreenBuffer
			resources->InitPixelShader(g_bUseSteamVR ? resources->_bloomCombinePS_VR : resources->_bloomCombinePS);
			context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer,
				0, BACKBUFFER_FORMAT);
			if (g_bUseSteamVR) {
				context->ResolveSubresource(
					resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
					resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
			}
			context->PSSetShaderResources(0, 1, resources->_offscreenAsInputShaderResourceView.GetAddressOf());
			context->PSSetShaderResources(1, 1, resources->_bloomOutputSumSRV.GetAddressOf());
			/*
			ID3D11RenderTargetView *rtvs[2] = {
				resources->_renderTargetView.Get(),
				resources->_renderTargetViewBloom1.Get()
			};
			context->OMSetRenderTargets(2, rtvs, NULL);
			*/
			context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(), NULL);
			break;
	}
	if (g_bUseSteamVR)
		context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
	else
		context->Draw(6, 0);
\
	// Restore previous rendertarget, etc
	// TODO: Is this really needed?
	viewport.Width  = screen_res_x;
	viewport.Height = screen_res_y;
	resources->InitViewport(&viewport);
	resources->InitInputLayout(resources->_inputLayout);
	context->OMSetRenderTargets(1, this->_deviceResources->_renderTargetView.GetAddressOf(),
		this->_deviceResources->_depthStencilViewL.Get());

	//this->_deviceResources->EndAnnotatedEvent();
}

/*
 * Performs a full bloom blur pass to build a pyramidal bloom. This function
 * calls BloomBasicPass internally several times.
 * Input: Bloom mask (_offscreenBufferAsInputReshadeMask, _offscreenBufferAsInputReshadeMaskR)
 * Output: 
 *		_offscreenBuffer (with accumulated bloom)
 *		_offscreenBufferAsInputReshadeMask, _offscreenBufferAsInputReshadeMaskR (blurred and downsampled from this pass)
 */
void PrimarySurface::BloomPyramidLevelPass(int PyramidLevel, int AdditionalPasses, float fZoomFactor, bool debug=false) {
	this->_deviceResources->BeginAnnotatedEvent(L"BloomPyramidLevelPass");

	auto &resources = this->_deviceResources;
	auto &context = resources->_d3dDeviceContext;
	float fPixelScale = g_fBloomSpread[PyramidLevel];
	float fFirstPassZoomFactor = fZoomFactor / 2.0f;

	this->_deviceResources->BeginAnnotatedEvent(L"HorizontalBloomPass");

	// The textures are always going to be g_fCurScreenWidth x g_fCurScreenHeight; but the step
	// size will be twice as big in the next pass due to the downsample, so we have to compensate
	// with a zoom factor:
	g_BloomPSCBuffer.pixelSizeX			= fPixelScale * g_fCurScreenWidthRcp  / fFirstPassZoomFactor;
	g_BloomPSCBuffer.pixelSizeY			= fPixelScale * g_fCurScreenHeightRcp / fFirstPassZoomFactor;
	g_BloomPSCBuffer.amplifyFactor		= 1.0f / fFirstPassZoomFactor;
	g_BloomPSCBuffer.bloomStrength		= g_fBloomLayerMult[PyramidLevel];
	g_BloomPSCBuffer.saturationStrength = g_BloomConfig.fSaturationStrength;
	g_BloomPSCBuffer.uvStepSize			= g_BloomConfig.uvStepSize1;
	//g_BloomPSCBuffer.uvStepSize			= 1.5f;
	resources->InitPSConstantBufferBloom(resources->_bloomConstantBuffer.GetAddressOf(), &g_BloomPSCBuffer);

	// DEBUG
	/*if (g_bDumpSSAOBuffers) {
		wchar_t filename[80];
		swprintf_s(filename, 80, L"c:\\temp\\_offscreenInputBloomMask-%d.dds", PyramidLevel);
		DirectX::SaveDDSTextureToFile(context, resources->_offscreenBufferAsInputBloomMask, filename);
	}*/
	// DEBUG

	// Initial Horizontal Gaussian Blur from Masked Buffer. input: reshade mask, output: bloom1
	// This pass will downsample the image according to fViewportDivider:
	BloomBasicPass(0, fZoomFactor);
	this->_deviceResources->EndAnnotatedEvent();
	// DEBUG
	/*if (g_bDumpSSAOBuffers) {
		wchar_t filename[80];
		swprintf_s(filename, 80, L"c:\\temp\\_bloom1-pass0-level-%d.dds", PyramidLevel);
		DirectX::SaveDDSTextureToFile(context, resources->_bloomOutput1, filename);
	}*/
	// DEBUG

	this->_deviceResources->BeginAnnotatedEvent(L"VerticalBloomPass");
	// Second Vertical Gaussian Blur: adjust the pixel size since this image was downsampled in
	// the previous pass:
	g_BloomPSCBuffer.pixelSizeX		= fPixelScale * g_fCurScreenWidthRcp / fZoomFactor;
	g_BloomPSCBuffer.pixelSizeY		= fPixelScale * g_fCurScreenHeightRcp / fZoomFactor;
	// The UVs should now go to half the original range because the image was downsampled:
	g_BloomPSCBuffer.amplifyFactor	= 1.0f / fZoomFactor;
	resources->InitPSConstantBufferBloom(resources->_bloomConstantBuffer.GetAddressOf(), &g_BloomPSCBuffer);
	// Vertical Gaussian Blur. input: bloom1, output: bloom2
	BloomBasicPass(1, fZoomFactor);
	this->_deviceResources->EndAnnotatedEvent();
	// DEBUG
	/*if (g_bDumpSSAOBuffers) {
		wchar_t filename[80];
		swprintf_s(filename, 80, L"c:\\temp\\_bloom2-pass1-level-%d.dds", PyramidLevel);
		DirectX::SaveDDSTextureToFile(context, resources->_bloomOutput2, filename);
	}*/
	// DEBUG

	for (int i = 0; i < AdditionalPasses; i++) {
		this->_deviceResources->BeginAnnotatedEvent(L"AdditionalBloomPass");
		// Alternating between 2.0 and 1.5 avoids banding artifacts
		//g_BloomPSCBuffer.uvStepSize = (i % 2 == 0) ? 2.0f : 1.5f;
		//g_BloomPSCBuffer.uvStepSize = 1.5f + (i % 3) * 0.7f;
		g_BloomPSCBuffer.uvStepSize = (i % 2 == 0) ? g_BloomConfig.uvStepSize2 : g_BloomConfig.uvStepSize1;
		resources->InitPSConstantBufferBloom(resources->_bloomConstantBuffer.GetAddressOf(), &g_BloomPSCBuffer);
		// Horizontal Gaussian Blur. input: bloom2, output: bloom1
		BloomBasicPass(2, fZoomFactor);
		// Vertical Gaussian Blur. input: bloom1, output: bloom2
		BloomBasicPass(1, fZoomFactor);
		this->_deviceResources->EndAnnotatedEvent();
	}
	// The blur output will *always* be in bloom2, let's copy it to the bloom masks to reuse it for the
	// next pass:
	context->CopyResource(resources->_offscreenBufferAsInputBloomMask, resources->_bloomOutput2);
	if (g_bUseSteamVR)
		context->CopyResource(resources->_offscreenBufferAsInputBloomMaskR, resources->_bloomOutput2R);

	// DEBUG
	/*if (g_iPresentCounter == 100 || g_bDumpBloomBuffers) {
		wchar_t filename[80];
		swprintf_s(filename, 80, L"c:\\temp\\_bloom2Buffer-Level-%d.jpg", PyramidLevel);
		DirectX::SaveWICTextureToFile(context, resources->_bloomOutput2, GUID_ContainerFormatJpeg, filename);
	}*/
	// DEBUG

	g_BloomPSCBuffer.amplifyFactor = 1.0f / fZoomFactor;
	resources->InitPSConstantBufferBloom(resources->_bloomConstantBuffer.GetAddressOf(), &g_BloomPSCBuffer);
	this->_deviceResources->BeginAnnotatedEvent(L"MergeBloomPass");
	// Combine. input: offscreenBuffer (will be resolved), bloom2; output: offscreenBuffer/bloom1
	//BloomBasicPass(3, fZoomFactor);

	// Accummulate the bloom buffer, input: bloom2, bloomSum; output: bloom1
	BloomBasicPass(4, fZoomFactor);

	// Copy _bloomOutput1 over _bloomOutputSum
	context->CopyResource(resources->_bloomOutputSum, resources->_bloomOutput1);
	if (g_bUseSteamVR)
		context->CopyResource(resources->_bloomOutputSumR, resources->_bloomOutput1R);
	this->_deviceResources->EndAnnotatedEvent();
	// DEBUG
	/*if (g_bDumpSSAOBuffers) {
		wchar_t filename[80];
		swprintf_s(filename, 80, L"c:\\temp\\_bloomOutputSum-Level-%d.dds", PyramidLevel);
		DirectX::SaveDDSTextureToFile(context, resources->_bloomOutputSum, filename);
	}*/
	// DEBUG

	// To make this step compatible with the rest of the code, we need to copy the results
	// to offscreenBuffer and offscreenBufferR (in SteamVR mode).
	/*context->CopyResource(resources->_offscreenBuffer, resources->_bloomOutput1);
	if (g_bUseSteamVR)
		context->CopyResource(resources->_offscreenBufferR, resources->_bloomOutput1R);*/

	this->_deviceResources->EndAnnotatedEvent();
}

void PrimarySurface::ClearBox(uvfloat4 box, D3D11_VIEWPORT *viewport, D3DCOLOR clearColor) {
	HRESULT hr;
	auto& resources = _deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;

	D3D11_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = resources->InitBlendState(nullptr, &blendDesc);

	// Set the vertex buffer
	// D3DCOLOR seems to be AARRGGBB
	D3DTLVERTEX vertices[6];
	vertices[0].sx = box.x0; vertices[0].sy = box.y0; vertices[0].sz = 0.98f; vertices[0].rhw = 34.0f; vertices[0].color = clearColor;
	vertices[1].sx = box.x1; vertices[1].sy = box.y0; vertices[1].sz = 0.98f; vertices[1].rhw = 34.0f; vertices[1].color = clearColor;
	vertices[2].sx = box.x1; vertices[2].sy = box.y1; vertices[2].sz = 0.98f; vertices[2].rhw = 34.0f; vertices[2].color = clearColor;

	vertices[3].sx = box.x1; vertices[3].sy = box.y1; vertices[3].sz = 0.98f; vertices[3].rhw = 34.0f; vertices[3].color = clearColor;
	vertices[4].sx = box.x0; vertices[4].sy = box.y1; vertices[4].sz = 0.98f; vertices[4].rhw = 34.0f; vertices[4].color = clearColor;
	vertices[5].sx = box.x0; vertices[5].sy = box.y0; vertices[5].sz = 0.98f; vertices[5].rhw = 34.0f; vertices[5].color = clearColor;

	D3D11_MAPPED_SUBRESOURCE map;
	hr = context->Map(resources->_clearHUDVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
	if (SUCCEEDED(hr)) {
		memcpy(map.pData, vertices, sizeof(D3DTLVERTEX) * 6);
		context->Unmap(resources->_clearHUDVertexBuffer, 0);
	}

	D3D11_DEPTH_STENCIL_DESC zDesc;
	// Temporarily disable ZWrite: we won't need it to display the HUD
	ComPtr<ID3D11DepthStencilState> depthState;
	zDesc.DepthEnable = FALSE;
	zDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	zDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	zDesc.StencilEnable = FALSE;
	resources->InitDepthStencilState(depthState, &zDesc);

	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitRasterizerState(resources->_rasterizerState);

	// Change the shaders
	resources->InitVertexShader(resources->_passthroughVertexShader);
	resources->InitPixelShader(resources->_pixelShaderClearBox);
	// Change the render target
	// Set three RTVs: Foreground HUD, Background HUD and HUD Text
	ID3D11RenderTargetView *rtvs[3] = {
		resources->_renderTargetViewDynCockpitAsInput.Get(),
		resources->_renderTargetViewDynCockpitAsInputBG.Get(),
		resources->_DCTextAsInputRTV.Get(),
	};
	context->OMSetRenderTargets(3, rtvs, NULL);

	// Set the viewport
	resources->InitViewport(viewport);
	// Set the vertex buffer (map the vertices from the box)
	UINT stride = sizeof(D3DTLVERTEX);
	UINT offset = 0;
	resources->InitVertexBuffer(resources->_clearHUDVertexBuffer.GetAddressOf(), &stride, &offset);
	// Draw
	context->Draw(6, 0);
}

int PrimarySurface::ClearHUDRegions() {
	D3D11_VIEWPORT viewport = { 0 };
	int num_regions_erased = 0;
	
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width    = (float )_deviceResources->_backbufferWidth;
	viewport.Height   = (float )_deviceResources->_backbufferHeight;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	viewport.MaxDepth = D3D11_MAX_DEPTH;

	this->_deviceResources->BeginAnnotatedEvent(L"ClearHUDRegions");

	int size = g_iNumDCElements;
	for (int i = 0; i < size; i++) {
		dc_element *dc_elem = &g_DCElements[i];
		if (g_sCurrentCockpit[0] != 0 && !dc_elem->bNameHasBeenTested)
		{
			if (strstr(dc_elem->name, g_sCurrentCockpit) != NULL) {
				dc_elem->bActive = true;
				dc_elem->bNameHasBeenTested = true;
				//log_debug("[DBG] [DC] ACTIVATED: '%s'", dc_elem->name);
			}
		}

		// Only clear HUD regions for active dc_elements
		if (!dc_elem->bActive)
			continue;

		if (dc_elem->num_erase_slots == 0)
			continue;

		for (int j = 0; j < dc_elem->num_erase_slots; j++) {
			int erase_slot = dc_elem->erase_slots[j];
			DCHUDRegion *dcSrcBox = &g_DCHUDRegions.boxes[erase_slot];
			if (dcSrcBox->bLimitsComputed) {
				ClearBox(dcSrcBox->erase_coords, &viewport, 0x0);
				num_regions_erased++;
			}
		}
	}

	this->_deviceResources->EndAnnotatedEvent();
	return num_regions_erased;
}

/*
 * Renders the HUD foreground and background and applies the move_region 
 * commands if DC is enabled
 */
void PrimarySurface::DrawHUDVertices() {
	this->_deviceResources->BeginAnnotatedEvent(L"DrawHUDVertices");

	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	D3D11_VIEWPORT viewport;
	HRESULT hr;

	D3D11_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = resources->InitBlendState(nullptr, &blendDesc);

	// We don't need to clear the current vertex and pixel constant buffers.
	// Since we've just finished rendering 3D, they should contain values that
	// can be reused. So let's just overwrite the values that we need.
	g_VSCBuffer.aspect_ratio      =  g_fAspectRatio;
	g_VSCBuffer.z_override        = -1.0f;
	g_VSCBuffer.sz_override       = -1.0f;
	g_VSCBuffer.mult_z_override   = -1.0f;
	g_VSCBuffer.apply_uv_comp     =  g_bEnableVR;
	if (g_bEnableVR) {
		g_VSCBuffer.viewportScale[0] = 1.0f / resources->_displayWidth;
		g_VSCBuffer.viewportScale[1] = 1.0f / resources->_displayHeight;
	}
	else {
		g_VSCBuffer.viewportScale[0] =  2.0f / resources->_displayWidth;
		g_VSCBuffer.viewportScale[1] = -2.0f / resources->_displayHeight;
	}
	// Reduce the scale for GUI elements, except for the HUD
	g_VSCBuffer.viewportScale[3]  = g_fGUIElemsScale;
	// bPreventTransform: false
	g_VSCBuffer.bPreventTransform = 0.0f;
	// Enable/Disable the fixed GUI (default: true)
	g_VSCBuffer.bFullTransform	  = g_bFixedGUI ? 1.0f : 0.0f; // g_bFixedGUI is true by default
	
	// Apply view transform to the HUD to fix it in space. It needs the inverse of the fullViewMat that contains the headtracking pose.
	g_VSMatrixCB.fullViewMat.invert();
	// Since the HUD is all rendered on a flat surface, we lose the vrparams that make the 3D object
	// and text float
	g_VSCBuffer.z_override		  = g_fFloatingGUIDepth;
	g_VSCBuffer.scale_override	  = g_fGUIElemsScale;

	g_PSCBuffer.brightness		  = 1.0f;
	g_PSCBuffer.bUseCoverTexture  = 0;
	
	// Add the move_regions commands.
	int numCoords = 0;
	if (g_bDynCockpitEnabled) 
	{
		for (int i = 0; i < g_DCMoveRegions.numCoords; i++) {
			int region_slot = g_DCMoveRegions.region_slot[i];
			// Skip invalid src slots
			if (region_slot < 0)
				continue;
			// Skip regions if their limits haven't been computed
			if (!g_DCHUDRegions.boxes[region_slot].bLimitsComputed) {
				//log_debug("[DBG] [DC] Skipping move_region command for slot %d", region_slot);
				continue;
			}
			// Fetch the source uv coords:
			g_DCPSCBuffer.src[numCoords] = g_DCHUDRegions.boxes[region_slot].erase_coords;
			// Fetch the destination uv coords:
			g_DCPSCBuffer.dst[numCoords] = g_DCMoveRegions.dst[i];
			numCoords++;
		}
	}
	g_PSCBuffer.DynCockpitSlots = numCoords;

	resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
	resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
	resources->InitPSConstantBufferDC(resources->_PSConstantBufferDC.GetAddressOf(), &g_DCPSCBuffer);

	UINT stride = sizeof(D3DTLVERTEX);
	UINT offset = 0;
	resources->InitVertexBuffer(resources->_HUDVertexBuffer.GetAddressOf(), &stride, &offset);
	resources->InitInputLayout(resources->_inputLayout);
	if (g_bEnableVR)
		resources->InitVertexShader(resources->_sbsVertexShader); // if (g_bEnableVR)
	else
		// The original code used _vertexShader:
		resources->InitVertexShader(resources->_vertexShader);
	resources->InitPixelShader(resources->_pixelShaderHUD);
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitRasterizerState(resources->_rasterizerState);

	// Temporarily disable ZWrite: we won't need it to display the HUD
	D3D11_DEPTH_STENCIL_DESC desc;
	ComPtr<ID3D11DepthStencilState> depthState;
	desc.DepthEnable = FALSE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	desc.StencilEnable = FALSE;
	resources->InitDepthStencilState(depthState, &desc);

	// Don't clear the render target, the offscreenBuffer already has the 3D render in it
	// Render the left image
	if (g_bUseSteamVR)
		context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(), NULL);
	else
		context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(), NULL);
	// VIEWPORT-LEFT
	if (g_bEnableVR) {
		if (g_bUseSteamVR)
			viewport.Width = (float)resources->_backbufferWidth;
		else
			viewport.Width = (float)resources->_backbufferWidth / 2.0f;
	}
	else // Non-VR path
		viewport.Width = (float)resources->_backbufferWidth;
	viewport.Height = (float)resources->_backbufferHeight;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	resources->InitViewport(&viewport);
	// Set the left projection matrix
	g_VSMatrixCB.projEye[0] = g_FullProjMatrixLeft;
	g_VSMatrixCB.projEye[1] = g_FullProjMatrixRight;
	// The viewMatrix is set at the beginning of the frame
	resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);
	// Set the HUD foreground, background and Text textures:
	ID3D11ShaderResourceView *srvs[3] = {
		resources->_offscreenAsInputDynCockpitSRV.Get(),
		resources->_offscreenAsInputDynCockpitBG_SRV.Get(),
		resources->_DCTextSRV.Get()
	};
	context->PSSetShaderResources(0, 3, srvs);
	// Draw the Left Image
	//if (RenderHUD)
	if (g_bUseSteamVR)
		context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
	else
		context->Draw(6, 0);

	if (!g_bEnableVR || g_bUseSteamVR) // Shortcut for the SteamVR and non-VR path
	{
		goto out;
	}

	// Render the right image in SBS mode
	context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(), NULL);

	// VIEWPORT-RIGHT
	viewport.Width = (float)resources->_backbufferWidth / 2.0f;
	viewport.TopLeftX = 0.0f + viewport.Width;
	viewport.Height = (float)resources->_backbufferHeight;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	resources->InitViewport(&viewport);
	// Set the right projection matrix
	g_VSMatrixCB.projEye[0] = g_FullProjMatrixRight;
	resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);
	// Draw the Right Image
	//if (RenderHUD)
		context->Draw(6, 0);

out:
	/*
	// Restore the state
	ID3D11ShaderResourceView *null_srvs[3] = { NULL, NULL, NULL };
	context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(), resources->_depthStencilViewL.Get());
	context->PSSetShaderResources(0, 3, null_srvs);
	resources->InitViewport(&g_nonVRViewport);
	resources->InitVertexShader(resources->_vertexShader);
	resources->InitPixelShader(resources->_pixelShaderTexture);
	resources->InitRasterizerState(resources->_rasterizerState);
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitInputLayout(resources->_inputLayout);
	//UINT stride = sizeof(D3DTLVERTEX);
	//UINT offset = 0;
	//resources->InitVertexBuffer(_vertexBuffer.GetAddressOf(), &stride, &offset);
	*/
	this->_deviceResources->EndAnnotatedEvent();

}

/// <summary>
/// Uses mobileObject->transformMatrix to get the player's craft orientation.
/// When invert=false, the matrix returned maps world OPT coords to viewspace OPT coords
/// When invert=true, the matrix returned maps viewspace OPT coords to world OPT coords
/// World OPT coords in this case look like this:
/// X+ --> Right
/// Y+ --> Up
/// Z+ --> Foward <-- OPT coords normally have Z- mapped to forwards. Set scaleFs to -1
/// to invert Fs
/// </summary>
Matrix4 GetPlayerCraftMatrix(Vector4& Rs, Vector4& Us, Vector4& Fs, float scaleFs, bool invert=false, bool debug=false)
{
	ObjectEntry* object = NULL;
	MobileObjectEntry* mobileObject = NULL;
	Matrix4 H;

	if (GetPlayerCraftInstanceSafe(&object, &mobileObject) != NULL)
	{
		Rs.x = mobileObject->transformMatrix.Right_X / 32768.0f;
		Rs.y = mobileObject->transformMatrix.Right_Y / 32768.0f;
		Rs.z = mobileObject->transformMatrix.Right_Z / 32768.0f;

		Us.x = mobileObject->transformMatrix.Up_X / 32768.0f;
		Us.y = mobileObject->transformMatrix.Up_Y / 32768.0f;
		Us.z = mobileObject->transformMatrix.Up_Z / 32768.0f;

		Fs.x = scaleFs * mobileObject->transformMatrix.Front_X / 32768.0f;
		Fs.y = scaleFs * mobileObject->transformMatrix.Front_Y / 32768.0f;
		Fs.z = scaleFs * mobileObject->transformMatrix.Front_Z / 32768.0f;

		if (!invert)
		{
			H = Matrix4(
				Rs.x, Fs.x, Us.x, 0,
				Rs.y, Fs.y, Us.y, 0,
				Rs.z, Fs.z, Us.z, 0,
				0, 0, 0, 1
			);
		}
		else
		{
			H = Matrix4(
				Rs.x, Rs.y, Rs.z, 0,
				Fs.x, Fs.y, Fs.z, 0,
				Us.x, Us.y, Us.z, 0,
				0, 0, 0, 1
			);
		}

		if (debug)
		{
			log_debug("[DBG] RUF: [%0.3f, %0.3f, %0.3f], [%0.3f, %0.3f, %0.3f], [%0.3f, %0.3f, %0.3f]",
				Rs.x, Rs.y, Rs.z,
				Us.x, Us.y, Us.z,
				Fs.x, Fs.y, Fs.z);
		}
	}

	return H;
}

/*
 * Sets the lights in the constant buffer used in the New Shading System.
 */
void SetLights(DeviceResources *resources, float fSSDOEnabled) {
	Vector4 light;
	int i;

	// XWA's world coord system *for this case*:
	// X+: Right
	// Y+: Forwards
	// Z+: Up
	Vector4 Rs, Us, Fs;
	Matrix4 H = GetPlayerCraftMatrix(Rs, Us, Fs, 1.0f, false, false); // SetLights()
	// I tried reading the 3x3 matrix from CockpitLook before the roll inertia was applied and putting
	// that in SharedMem in order to fix "dancing RT shadows". Didn't work. Still, this code might be
	// useful later.
#ifdef DISABLED
	if (g_bUseSteamVR)
	{
		const float* h = g_pSharedDataCockpitLook->headRotation;
		const float m[16] = {
			-h[0], h[6], -h[3], 0,
			-h[1], h[7], -h[4], 0,
			-h[2], h[8], -h[5], 0,
			0, 0, 0, 1 };
		H.set(m);
		log_debug_vr(0xFFFFFF, "R:[%0.3f, %0.3f, %0.3f]", Rs.x, Rs.y, Rs.z);
		log_debug_vr(0x10FF10, "R:[%0.3f, %0.3f, %0.3f]", -h[0], -h[1], -h[2]);

		log_debug_vr(0xFFFFFF, "F:[%0.3f, %0.3f, %0.3f]", Fs.x, Fs.y, Fs.z);
		log_debug_vr(0x10FF10, "F:[%0.3f, %0.3f, %0.3f]", h[6], h[7], h[8]);

		log_debug_vr(0xFFFFFF, "U:[%0.3f, %0.3f, %0.3f]", Us.x, Us.y, Us.z);
		log_debug_vr(0x10FF10, "U:[%0.3f, %0.3f, %0.3f]", -h[3], -h[4], -h[5]);
	}
#endif

	// Normally, Y- points forwards in OPT coords, but in this case, Y+ points forwards.
	// So, in this case, we only flip the Z coord to convert to SteamVR coords:
	const static Matrix4 swapFlip({ 1,0,0,0,  0,0,-1,0,  0,1,0,0,  0,0,0,1 });
	Matrix4 Vinv = g_VSMatrixCB.fullViewMat;
	Vinv.invert();

	static int prevMissionIndex = -1;
	const char* xwaMissionFileName = (const char*)0x06002E8;
	//const int missionFileNameIndex = *(int*)0x06002E4;
	if (*missionIndexLoaded != prevMissionIndex)
	{
		log_debug("[DBG] [MSN] Mission index changed. Previous: %d, New: %d",
			prevMissionIndex, *missionIndexLoaded);
		if (*missionIndexLoaded != 0) // Skirmish missions are index 0
		{
			log_debug("[DBG] [MSN] Mission: %s", xwaMissionFileName);
			std::string mission = xwaMissionFileName;
			int dot = mission.find_last_of('.');
			mission = mission.substr(0, dot) + ".ini";
			log_debug("[DBG] [MSN] Loading ini: %s", mission.c_str());
			auto lines = GetFileLines(mission, "Headlights");
			g_bCurrentMissionUsesHeadLights = GetFileKeyValueInt(lines, "Enabled");
			log_debug("[DBG] [MSN] g_bCurrentMissionUsesHeadLights: %d", g_bCurrentMissionUsesHeadLights);
		}
		else
		{
			// Skirmish mission, disable headlights
			g_bCurrentMissionUsesHeadLights = false;
		}
		prevMissionIndex = *missionIndexLoaded;
	}

	const bool bDS2Mission = (missionIndexLoaded != nullptr) && (*missionIndexLoaded == DEATH_STAR_MISSION_INDEX);
	const bool autoTurnOn = g_bCurrentMissionUsesHeadLights ? false : g_bHeadLightsAutoTurnOn;

	g_bEnableHeadLights = (g_bCurrentMissionUsesHeadLights && !*g_playerInHangar);

	if (autoTurnOn) {
		if (*s_XwaGlobalLightsCount == 0) {
			//log_debug("[DBG] AUTO Turning headlights ON");
			g_bEnableHeadLights = true;
		}
		else {
			//log_debug("[DBG] AUTO Turning headlights OFF");
			g_bEnableHeadLights = false;
		}
	}

	/*
	if (g_bOverrideLightPos) {
		for (int i = 0; i < 2; i++) {
			light[i].x = g_LightVector[i].x;
			light[i].y = g_LightVector[i].y;
			light[i].z = g_LightVector[i].z;
		}
	}
	else
		ComputeRotationMatrixFromXWAView(light, 2);
	*/

	// Use the heading matrix to move the lights
	//Matrix4 H = GetCurrentHeadingViewMatrix();
	// In skirmish mode, the light with the highest intensity seems to be in index 1 and it matches the 
	// position of the Sun/Nebula. However, in regular missions, the main light source seems to come from
	// index 0 and index 1 is a secondary light like a big planet or nebula or similar.
	// We need to find the light with the highest intensity and use that for SSDO
	float maxIntensity = -1.0;
	int maxIdx = -1, maxLights = min(MAX_XWA_LIGHTS, *s_XwaGlobalLightsCount);
	float cur_ambient = g_ShadingSys_PSBuffer.ambient;
	//if (*s_XwaGlobalLightsCount == 0)
	//	log_debug("[DBG] NO GLOBAL LIGHTS!");
	if (g_bDumpSSAOBuffers) {
		if (missionIndexLoaded != nullptr)
			log_debug("[DBG] [INT] Mission: %d", *missionIndexLoaded);
		else
			log_debug("[DBG] NULL mission index");
		log_debug("[DBG] [INT] current region: %d", PlayerDataTable[*g_playerIndex].currentRegion);
		log_debug("[DBG] s_XwaGlobalLightsCount: %d", *s_XwaGlobalLightsCount);
		//DumpGlobalLights();
	}

	if (g_bUseTextureCube)
	{
		g_ShadowMapVSCBuffer.Camera = g_CurrentHeadingViewMatrix;
		g_ShadowMapVSCBuffer.Camera.invert();
	}

	if (g_HyperspacePhaseFSM != HS_HYPER_TUNNEL_ST)
	{
		for (int i = 0; i < maxLights; i++)
		{
			Vector4 col;
			float intensity, value;

			Vector4 xwaLight = Vector4(
				s_XwaGlobalLights[i].PositionX / 32768.0f,
				s_XwaGlobalLights[i].PositionY / 32768.0f,
				s_XwaGlobalLights[i].PositionZ / 32768.0f,
				0.0f);

			if (g_bUseSteamVR)
			{
				light = Vinv * swapFlip * H * xwaLight;
			}
			else
			{
				light = g_CurrentHeadingViewMatrix * xwaLight;
				light.z = -light.z; // Once more we invert Z because normal mapping has Z+ pointing to the camera
			}

			// Forward: -Z, Up: +Y, Right: +X
			//log_debug("[DBG] light, I: %0.3f, [%0.3f, %0.3f, %0.3f]",
			//	s_XwaGlobalLights[i].Intensity, light.x, light.y, light.z);
			g_ShadingSys_PSBuffer.LightVector[i].x = light.x;
			g_ShadingSys_PSBuffer.LightVector[i].y = light.y;
			g_ShadingSys_PSBuffer.LightVector[i].z = light.z;
			g_ShadingSys_PSBuffer.LightVector[i].w = 0.0f;

			col.x = s_XwaGlobalLights[i].ColorR;
			col.y = s_XwaGlobalLights[i].ColorG;
			col.z = s_XwaGlobalLights[i].ColorB;
			col.w = 0.0f;

			// Prevent black suns.
			// Some international languages may cause suns to get a black color if the
			// FGs are messed. This code prevents such black suns, but it's currently
			// disabled because the proper fix is to have a *good* xwa.tab. I leave this
			// here just in case we ever need it again.
			// For testing purposes, install German and try the second region of B5M3.
			/*
			if (g_XWALightInfo[i].bIsSun && (col.x + col.y + col.z) < 0.0001f)
			{
				col = Vector4(1, 1, 1, 0);
			}
			*/

			intensity = s_XwaGlobalLights[i].Intensity;
			// Compute the value: use Luma to approximate it
			value = 0.299f * col.x + 0.587f * col.y + 0.114f * col.z;
			// Normalize the colors if the intensity is above 1
			if (intensity > 1.0f)
				intensity = value;
			else if (g_bApplyXWALightsIntensity)
				// Tone down the current color according to its intensity (?)
				col *= intensity;

			// Change the saturation of the lights. The idea here is that we're
			// using a vector gray = vec3(value) and then computing col - gray, and then
			// we interpolate between col and gray depending on the saturation setting
			col.x = value + g_fXWALightsSaturation * (col.x - value);
			col.y = value + g_fXWALightsSaturation * (col.y - value);
			col.z = value + g_fXWALightsSaturation * (col.z - value);

			// We should not fade the lights while we're in the hangar. The lighting doesn't match
			// the suns anyway.
			float Lightness = g_bFadeLights && !*g_playerInHangar ?
				max(g_fMinLightIntensity, 1.0f - g_ShadowMapVSCBuffer.sm_black_levels[i]) : 1.0f;

			g_ShadingSys_PSBuffer.LightColor[i].x = Lightness * g_fXWALightsIntensity * col.x;
			g_ShadingSys_PSBuffer.LightColor[i].y = Lightness * g_fXWALightsIntensity * col.y;
			g_ShadingSys_PSBuffer.LightColor[i].z = Lightness * g_fXWALightsIntensity * col.z;
			g_ShadingSys_PSBuffer.LightColor[i].w = intensity;

			// Light normalization does not look great in the DS2 mission
			if (g_ShadingSys_PSBuffer.HDREnabled && !bDS2Mission) {
				g_ShadingSys_PSBuffer.LightColor[i].x *= g_fHDRLightsMultiplier;
				g_ShadingSys_PSBuffer.LightColor[i].y *= g_fHDRLightsMultiplier;
				g_ShadingSys_PSBuffer.LightColor[i].z *= g_fHDRLightsMultiplier;
			}

			// Normalize low-intensity lights
			// Do not normalize the lights while parked in the hangar (so that we can see red alert lights)
			// Do not normalize lights when jumping into hyperspace
			if (g_bNormalizeLights && g_XWALightInfo[i].bIsSun && !*g_playerInHangar && g_HyperspacePhaseFSM == HS_INIT_ST)
			{
				Vector3 col;
				col.x = g_ShadingSys_PSBuffer.LightColor[i].x;
				col.y = g_ShadingSys_PSBuffer.LightColor[i].y;
				col.z = g_ShadingSys_PSBuffer.LightColor[i].z;

				// Approx luma
				float value = (col.x + col.y + col.z) / 3.0f;
				if (value < 0.9f)
				{
					if (g_bDumpSSAOBuffers)
						log_debug("[DBG] Normalizing light %d, col:[%0.3f, %0.3f, %0.3f]", i, col.x, col.y, col.z);
					if (!bDS2Mission)
						col = 0.9f * g_fHDRLightsMultiplier * col.normalize();
					else
						col = 0.9f * col.normalize();
					if (g_bDumpSSAOBuffers)
						log_debug("[DBG] New color:[%0.3f, %0.3f, %0.3f]", col.x, col.y, col.z);
					g_ShadingSys_PSBuffer.LightColor[i].x = col.x;
					g_ShadingSys_PSBuffer.LightColor[i].y = col.y;
					g_ShadingSys_PSBuffer.LightColor[i].z = col.z;
					g_ShadingSys_PSBuffer.LightColor[i].w = 1.0f;
				}
			}

			// Keep track of the light with the highest intensity
			if (intensity > maxIntensity) {
				maxIntensity = intensity;
				maxIdx = i;
			}

			if (g_bDumpSSAOBuffers)
			{
				log_debug("[DBG] light[%d], I: %0.3f: i: %0.3f, V:[%0.3f, %0.3f, %0.3f], XWACOL: (%0.3f, %0.3f, %0.3f), shader col: (%0.3f, %0.3f, %0.3f)",
					i, s_XwaGlobalLights[i].Intensity, intensity,
					g_ShadingSys_PSBuffer.LightVector[i].x, g_ShadingSys_PSBuffer.LightVector[i].y, g_ShadingSys_PSBuffer.LightVector[i].z,
					s_XwaGlobalLights[i].ColorR, s_XwaGlobalLights[i].ColorG, s_XwaGlobalLights[i].ColorB,
					g_ShadingSys_PSBuffer.LightColor[i].x, g_ShadingSys_PSBuffer.LightColor[i].y, g_ShadingSys_PSBuffer.LightColor[i].z
				);
			}
		}

		if (*g_playerInHangar) {
			// I don't remember why we're forcing 2 lights in the hangar, but removing the
			// next line disables hangar shadow mapping, so they are related somehow; see
			// commit 19d7bd03. The point is that forcing 2 lights may cause "leftover"
			// lights from a mission to cast illumination in the hangar if the mission is
			// terminated by pressing H. To avoid this problem, I'm also forcing the color
			// of the second light to 0 to disable it. That seems to "fix" the leftover
			// lighting.
			maxLights = 2;
			g_ShadingSys_PSBuffer.LightColor[1].x = 0;
			g_ShadingSys_PSBuffer.LightColor[1].y = 0;
			g_ShadingSys_PSBuffer.LightColor[1].z = 0;
		}
		g_ShadingSys_PSBuffer.LightCount  = maxLights;
		g_ShadingSys_PSBuffer.MainLight.x = g_ShadingSys_PSBuffer.LightVector[maxIdx].x;
		g_ShadingSys_PSBuffer.MainLight.y = g_ShadingSys_PSBuffer.LightVector[maxIdx].y;
		g_ShadingSys_PSBuffer.MainLight.z = g_ShadingSys_PSBuffer.LightVector[maxIdx].z;
		g_ShadingSys_PSBuffer.MainColor   = g_ShadingSys_PSBuffer.LightColor[maxIdx];

		// If the headlights are on, then replace the main light with the headlight
		if (g_bEnableHeadLights) {
			Vector4 headLightDir(0.0, 0.0, 1.0, 0.0);
			Vector4 headLightPos(g_HeadLightsPosition.x, g_HeadLightsPosition.y, g_HeadLightsPosition.z, 1.0);
			Matrix4 ViewMatrix;
			if (g_bEnableVR) {
				// Apply the headtracking transform to the ViewMatrix
				ViewMatrix = g_VSMatrixCB.fullViewMat;
				ViewMatrix.invert();
				if (PlayerDataTable[*g_playerIndex].gunnerTurretActive) {
					Matrix4 GunnerMatrix;
					Matrix4 S = Matrix4().scale(1, 1, -1);
					GetGunnerTurretViewMatrixSpeedEffect(&GunnerMatrix);
					ViewMatrix = S * ViewMatrix * S * GunnerMatrix;
				}
				else {
					Matrix4 S = Matrix4().scale(1, 1, -1);
					ViewMatrix = S * g_VSMatrixCB.fullViewMat * S;
				}
			}
			else {
				GetCockpitViewMatrixSpeedEffect(&ViewMatrix, true);
			}

			// We're going to need another vector for the headlights direction... let's shift all
			// the light vectors one spot to the right and place the direction on the first slot
			for (int i = 0; i < maxLights; i++)
				if (i + 1 < MAX_XWA_LIGHTS)
				{
					g_ShadingSys_PSBuffer.LightVector[i + 1] = g_ShadingSys_PSBuffer.LightVector[i];
					g_ShadingSys_PSBuffer.LightColor[i + 1] = g_ShadingSys_PSBuffer.LightColor[i];
				}
			maxLights = min(MAX_XWA_LIGHTS, maxLights + 1);
			// Transform the headlights' direction and position:
			headLightDir = ViewMatrix * headLightDir;
			ViewMatrix.transpose();
			headLightPos = ViewMatrix * headLightPos;
			headLightPos.z = -headLightPos.z; // Shaders expect Z to be negative so that Z+ points towards the camera
			//log_debug("[DBG] headLightDir: %0.3f, %0.3f, %0.3f", headLightDir.x, headLightDir.y, headLightDir.z);
			//log_debug("[DBG] headLightPos: %0.3f, %0.3f, %0.3f", headLightPos.x, headLightPos.y, headLightPos.z);
			g_ShadingSys_PSBuffer.LightVector[0].x = headLightDir.x;
			g_ShadingSys_PSBuffer.LightVector[0].y = headLightDir.y;
			g_ShadingSys_PSBuffer.LightVector[0].z = headLightDir.z;

			// DEBUG: Remove all lights!
			//g_ShadingSys_PSBuffer.LightCount = 0;
			// DEBUG
			g_ShadingSys_PSBuffer.ambient = g_fHeadLightsAmbient;
			g_ShadingSys_PSBuffer.MainLight.x = headLightPos.x;
			g_ShadingSys_PSBuffer.MainLight.y = headLightPos.y;
			g_ShadingSys_PSBuffer.MainLight.z = headLightPos.z;

			g_ShadingSys_PSBuffer.MainColor.x = g_HeadLightsColor.x;
			g_ShadingSys_PSBuffer.MainColor.y = g_HeadLightsColor.y;
			g_ShadingSys_PSBuffer.MainColor.z = g_HeadLightsColor.z;
			g_ShadingSys_PSBuffer.MainColor.w = g_fHeadLightsDistance;

			g_ShadingSys_PSBuffer.headlights_angle_cos = g_fHeadLightsAngleCos;
		}
	}
	else 
	{
		// Hyper tunnel lights
		g_ShadingSys_PSBuffer.LightCount = 2;
		for (int i = 0; i < 2; i++) {
			g_ShadingSys_PSBuffer.LightVector[i].x = g_LightVector[i].x;
			g_ShadingSys_PSBuffer.LightVector[i].y = g_LightVector[i].y;
			g_ShadingSys_PSBuffer.LightVector[i].z = g_LightVector[i].z;

			g_ShadingSys_PSBuffer.LightColor[i].x = g_LightColor[i].x;
			g_ShadingSys_PSBuffer.LightColor[i].y = g_LightColor[i].y;
			g_ShadingSys_PSBuffer.LightColor[i].z = g_LightColor[i].z;
		}
		g_ShadingSys_PSBuffer.MainLight.x = g_ShadingSys_PSBuffer.LightVector[0].x;
		g_ShadingSys_PSBuffer.MainLight.y = g_ShadingSys_PSBuffer.LightVector[0].y;
		g_ShadingSys_PSBuffer.MainLight.z = g_ShadingSys_PSBuffer.LightVector[0].z;
		g_ShadingSys_PSBuffer.MainColor   = g_ShadingSys_PSBuffer.LightColor[0];
	}

	if (g_bEnableLaserLights) {
		// DEBUG
		/*
		g_ShadingSys_PSBuffer.num_lasers = 1;
		g_ShadingSys_PSBuffer.LightPoint[0].x =  g_LaserPointDebug.x;
		g_ShadingSys_PSBuffer.LightPoint[0].y =  g_LaserPointDebug.y;
		g_ShadingSys_PSBuffer.LightPoint[0].z = -g_LaserPointDebug.z;

		g_ShadingSys_PSBuffer.LightPointColor[0].x = 1.0f;
		g_ShadingSys_PSBuffer.LightPointColor[0].y = 0.0f;
		g_ShadingSys_PSBuffer.LightPointColor[0].z = 0.0f;
		*/
		// DEBUG

		//g_LaserList.remove_duplicates();
		int num_lasers = g_LaserList._size;
		g_ShadingSys_PSBuffer.num_lasers = num_lasers;
		for (i = 0; i < num_lasers; i++) {
			// Set the lights from the lasers
			g_ShadingSys_PSBuffer.LightPoint[i].x =  g_LaserList._elems[i].P.x;
			g_ShadingSys_PSBuffer.LightPoint[i].y =  g_LaserList._elems[i].P.y;
			g_ShadingSys_PSBuffer.LightPoint[i].z = -g_LaserList._elems[i].P.z;
			g_ShadingSys_PSBuffer.LightPoint[i].w =  g_LaserList._elems[i].falloff;

			g_ShadingSys_PSBuffer.LightPointColor[i].x = g_LaserList._elems[i].col.x;
			g_ShadingSys_PSBuffer.LightPointColor[i].y = g_LaserList._elems[i].col.y;
			g_ShadingSys_PSBuffer.LightPointColor[i].z = g_LaserList._elems[i].col.z;
			// g_ShadingSys_PSBuffer.LightPointColor[i].w is unused

			g_ShadingSys_PSBuffer.LightPointDirection[i].x = g_LaserList._elems[i].dir.x;
			g_ShadingSys_PSBuffer.LightPointDirection[i].y = g_LaserList._elems[i].dir.y;
			g_ShadingSys_PSBuffer.LightPointDirection[i].z = g_LaserList._elems[i].dir.z;
			g_ShadingSys_PSBuffer.LightPointDirection[i].w = g_LaserList._elems[i].angle;
		}
	}
	else
		g_ShadingSys_PSBuffer.num_lasers = 0;
	
	if (g_bDumpSSAOBuffers) 
	{
		log_debug("[DBG] LightCount: %d, maxIdx: %d", g_ShadingSys_PSBuffer.LightCount, maxIdx);
		for (uint32_t i = 0; i < g_ShadingSys_PSBuffer.LightCount; i++) {
			log_debug("[DBG] light[%d], I: %0.3f: V:[%0.3f, %0.3f, %0.3f], col: (%0.3f, %0.3f, %0.3f)",
				i, g_ShadingSys_PSBuffer.LightColor[i].w,
				g_ShadingSys_PSBuffer.LightVector[i].x, g_ShadingSys_PSBuffer.LightVector[i].y, g_ShadingSys_PSBuffer.LightVector[i].z,
				g_ShadingSys_PSBuffer.LightColor[i].x, g_ShadingSys_PSBuffer.LightColor[i].y, g_ShadingSys_PSBuffer.LightColor[i].z
			);
		}
	}

	// TODO:
	g_ShadingSys_PSBuffer.ambient = *g_playerInHangar ? g_fHangarAmbient : g_fGlobalAmbient;
	g_ShadingSys_PSBuffer.ssdo_enabled = fSSDOEnabled;
	g_ShadingSys_PSBuffer.sso_disable = g_bEnableSSAOInShader ? 0.0f : 1.0f;
	g_ShadingSys_PSBuffer.HDREnabled = g_bHDREnabled;
	g_ShadingSys_PSBuffer.HDR_white_point = g_fHDRWhitePoint;
	if (g_bEnableHeadLights) g_ShadingSys_PSBuffer.ambient = cur_ambient;
	resources->InitPSConstantShadingSystem(resources->_shadingSysBuffer.GetAddressOf(), &g_ShadingSys_PSBuffer);
}

void PrimarySurface::SSAOPass(float fZoomFactor) {

	this->_deviceResources->BeginAnnotatedEvent(L"SSAOPass");

	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	
	// Set the vertex buffer... we probably need another vertex buffer here
	UINT stride = sizeof(MainVertex);
	UINT offset = 0;
	resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);

	// Set Primitive Topology
	// Opportunity for optimization? Make all draw calls use the same topology?
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitInputLayout(resources->_mainInputLayout);

	// Temporarily disable ZWrite: we won't need it for this effect
	D3D11_DEPTH_STENCIL_DESC desc;
	ComPtr<ID3D11DepthStencilState> depthState;
	desc.DepthEnable = FALSE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	desc.StencilEnable = FALSE;
	resources->InitDepthStencilState(depthState, &desc);

	// Create a new viewport to render the offscreen buffer as a texture
	float screen_res_x = (float)resources->_backbufferWidth;
	float screen_res_y = (float)resources->_backbufferHeight;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = screen_res_x / fZoomFactor;
	viewport.Height = screen_res_y / fZoomFactor;
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	resources->InitViewport(&viewport);

	// Set the constant buffers
	resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
		0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices

	// Set the SSAO pixel shader constant buffer
	g_SSAO_PSCBuffer.screenSizeX = g_fCurScreenWidth;
	g_SSAO_PSCBuffer.screenSizeY = g_fCurScreenHeight;
	g_SSAO_PSCBuffer.fn_enable   = g_bFNEnable;
	g_SSAO_PSCBuffer.debug		 = g_bShowSSAODebug;
	g_SSAO_PSCBuffer.enable_dist_fade = (float)g_b3DSkydomePresent;
	resources->InitPSConstantBufferSSAO(resources->_ssaoConstantBuffer.GetAddressOf(), &g_SSAO_PSCBuffer);

	// Set the layout
	context->IASetInputLayout(resources->_mainInputLayout);
	resources->InitVertexShader(g_bUseSteamVR ? resources->_mainVertexShaderVR : resources->_mainVertexShader);

	// Set the lights and set the Shading System Constant Buffer
	g_ShadingSys_PSBuffer.spec_intensity       = g_bGlobalSpecToggle ? g_fSpecIntensity : 0.0f;
	g_ShadingSys_PSBuffer.spec_bloom_intensity = g_bGlobalSpecToggle ? g_fSpecBloomIntensity : 0.0f;
	SetLights(resources, 0.0f);

	// SSAO Computation, Left Image
	// The pos/depth texture must be resolved to _depthAsInput/_depthAsInputR already
	// Input: _depthBuf, _depthBuf2, _normBuf, _offscreenBuf (resolved here)
	// Output _ssaoBuf, _bentBuf
	{
		// Resolve offscreenBuf
		context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer,
			0, BACKBUFFER_FORMAT);
		if (g_bUseSteamVR)
			context->ResolveSubresource(
				resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
				resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
		ID3D11ShaderResourceView *srvs_pass1[3] = {
			resources->_depthBufSRV.Get(),
			resources->_normBufSRV.Get(),
			resources->_offscreenAsInputShaderResourceView
		};
		resources->InitPixelShader(g_bUseSteamVR ? resources->_ssaoPS_VR : resources->_ssaoPS);
		if (!g_bBlurSSAO && g_bShowSSAODebug) {
			ID3D11RenderTargetView *rtvs[1] = {
				resources->_renderTargetView.Get(),
			};
			context->ClearRenderTargetView(resources->_renderTargetView, bgColor);
			context->OMSetRenderTargets(1, rtvs, NULL);
			context->PSSetShaderResources(0, 3, srvs_pass1);
			if (g_bUseSteamVR)
				context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
			else
				context->Draw(6, 0);
			goto out;
		}
		else {
			ID3D11RenderTargetView *rtvs[1] = {
				resources->_renderTargetViewSSAO.Get(),
			};
			context->ClearRenderTargetView(resources->_renderTargetViewSSAO, bgColor);
			context->OMSetRenderTargets(1, rtvs, NULL);
			context->PSSetShaderResources(0, 3, srvs_pass1);
			if (g_bUseSteamVR)
				context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
			else
				context->Draw(6, 0);
		}
	}

	// Setup the constant buffers to upscale the buffers
	// The textures are always going to be g_fCurScreenWidth x g_fCurScreenHeight; but the step
	// size will be twice as big in the next pass due to the downsample, so we have to compensate
	// with a zoom factor:
	float fPixelScale = fZoomFactor;
	g_BloomPSCBuffer.pixelSizeX			= fPixelScale * g_fCurScreenWidthRcp;
	g_BloomPSCBuffer.pixelSizeY			= fPixelScale * g_fCurScreenHeightRcp;
	g_BloomPSCBuffer.amplifyFactor		= 1.0f / fZoomFactor;
	g_BloomPSCBuffer.uvStepSize			= 1.0f;
	g_BloomPSCBuffer.depth_weight		= g_SSAO_PSCBuffer.max_dist;
	resources->InitPSConstantBufferBloom(resources->_bloomConstantBuffer.GetAddressOf(), &g_BloomPSCBuffer);

	// SSAO Blur, Left Image
	// input: _bloomOutput1SRV (with a copy of the ssaoBuf), depthBuf, normBuf
	// output: ssaoBuf
	if (g_bBlurSSAO) {
		resources->InitPixelShader(g_bUseSteamVR ? resources->_ssaoBlurPS_VR : resources->_ssaoBlurPS);
		// Copy the SSAO buffer to bloom1(HDR) -- we'll use it as temp buffer
		// to blur the SSAO buffer
		context->CopyResource(resources->_bloomOutput1, resources->_ssaoBuf);
		context->ClearRenderTargetView(resources->_renderTargetViewSSAO.Get(), bgColor);
		ID3D11ShaderResourceView *srvs[3] = {
				resources->_bloomOutput1SRV.Get(),
				resources->_depthBufSRV.Get(),
				resources->_normBufSRV.Get(),
		};
		
		ID3D11RenderTargetView *rtvs[1] = {
			resources->_renderTargetViewSSAO.Get(),
		};
		context->OMSetRenderTargets(1, rtvs, NULL);
		context->PSSetShaderResources(0, 3, srvs);
		if (g_bUseSteamVR)
			context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(6, 0);
	}

	// Final combine, Left Image
	{
		// input: offscreenAsInput (resolved here), bloomMask, ssaoBuf
		// output: offscreenBuf
		// Reset the viewport for the final SSAO combine
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width = screen_res_x;
		viewport.Height = screen_res_y;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		resources->InitViewport(&viewport);

		ID3D11ShaderResourceView *null_srvs4[4] = { NULL, NULL, NULL, NULL };
		context->PSSetShaderResources(0, 4, null_srvs4);
		// ssaoBuf was bound as an RTV, so let's bind the RTV first to unbind ssaoBuf
		// so that it can be used as an SRV
		ID3D11RenderTargetView *rtvs[5] = {
			resources->_renderTargetView.Get(),
			resources->_renderTargetViewBloomMask.Get(),
			NULL, NULL, NULL,
		};
		context->OMSetRenderTargets(5, rtvs, NULL);
		if (!g_bEnableHeadLights)
			resources->InitPixelShader(resources->_ssaoAddPS);
		else
			resources->InitPixelShader(resources->_headLightsSSAOPS);
		// Resolve offscreenBuf
		context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer,
			0, BACKBUFFER_FORMAT);
		context->ResolveSubresource(resources->_backgroundBufferAsInput, 0, resources->_backgroundBuffer,
			0, BACKBUFFER_FORMAT);
		if (g_bUseSteamVR) {
			context->ResolveSubresource(
				resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
				resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
			context->ResolveSubresource(
				resources->_backgroundBufferAsInput, D3D11CalcSubresource(0, 1, 1),
				resources->_backgroundBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
		}
		else {
			context->ResolveSubresource(resources->_ReticleBufAsInput, 0, resources->_ReticleBufMSAA,
				0, BACKBUFFER_FORMAT);
		}

		// In map mode, the background is empty, so we'll pass offscreenBuffer for the background SRV instead:
		if (g_bMapMode) context->CopyResource(resources->_backgroundBufferAsInput, resources->_offscreenBufferAsInput);

		ID3D11ShaderResourceView *srvs_pass2[8] = {
			resources->_offscreenAsInputShaderResourceView.Get(), // 0: Color buffer
			resources->_ssaoBufSRV.Get(),                         // 1: SSAO component
			resources->_backgroundBufferSRV.Get(),                // 2: Background buffer
			resources->_ssaoMaskSRV.Get(),                        // 3: SSAO Mask

			resources->_depthBufSRV.Get(),                        // 4: Depth buffer
			resources->_normBufSRV.Get(),                         // 5: Normals
			resources->_ssMaskSRV.Get(),                          // 6: Shading System Mask buffer
			g_ShadowMapping.bEnabled ?                            // 7: The shadow map
				resources->_shadowMapArraySRV.Get() : NULL,
		};
		context->PSSetShaderResources(0, 8, srvs_pass2);

		if (g_bRTEnabled)
		{
			ID3D11ShaderResourceView* srvs[] = {
				resources->_RTBvhSRV.Get(),        // 14
				resources->_RTMatricesSRV.Get(),   // 15
				resources->_RTTLASBvhSRV.Get(),    // 16
				resources->_rtShadowMaskSRV.Get(), // 17
			};
			// Slots 14-17 are used for Raytracing buffers (BLASes, Matrices, TLAS, RTShadowMask)
			context->PSSetShaderResources(14, 4, srvs);
		}

		{
			ID3D11ShaderResourceView* srvs[] = {
				resources->_transp1SRV.Get(), // 18
				resources->_transp2SRV.Get(), // 19
				resources->_ReticleSRV.Get(), // 20
			};
			context->PSSetShaderResources(18, 3, srvs);
		}

		if (g_bUseSteamVR)
			context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(6, 0);
	}

out:
	// Restore previous rendertarget, etc
	// TODO: Is this really needed?
	viewport.Width = screen_res_x;
	viewport.Height = screen_res_y;
	resources->InitViewport(&viewport);
	resources->InitInputLayout(resources->_inputLayout);
	context->OMSetRenderTargets(1, this->_deviceResources->_renderTargetView.GetAddressOf(),
		this->_deviceResources->_depthStencilViewL.Get());

	this->_deviceResources->EndAnnotatedEvent();
}

void PrimarySurface::SSDOPass(float fZoomFactor, float fZoomFactor2) {

	this->_deviceResources->BeginAnnotatedEvent(L"SSDOPass");

	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	float x0, y0, x1, y1;

	GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
	g_SSAO_PSCBuffer.x0 = x0;
	g_SSAO_PSCBuffer.y0 = y0;
	g_SSAO_PSCBuffer.x1 = x1;
	g_SSAO_PSCBuffer.y1 = y1;
	g_SSAO_PSCBuffer.enable_dist_fade = (float)g_b3DSkydomePresent;
	g_ShadingSys_PSBuffer.spec_intensity = g_bGlobalSpecToggle ? g_fSpecIntensity : 0.0f;
	g_ShadingSys_PSBuffer.spec_bloom_intensity = g_bGlobalSpecToggle ? g_fSpecBloomIntensity : 0.0f;

	// Set the vertex buffer... we probably need another vertex buffer here
	UINT stride = sizeof(MainVertex);
	UINT offset = 0;
	resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);

	// Set Primitive Topology
	// Opportunity for optimization? Make all draw calls use the same topology?
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitInputLayout(resources->_mainInputLayout);

	// Temporarily disable ZWrite: we won't need it for this effect
	D3D11_DEPTH_STENCIL_DESC desc;
	ComPtr<ID3D11DepthStencilState> depthState;
	desc.DepthEnable = FALSE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	desc.StencilEnable = FALSE;
	resources->InitDepthStencilState(depthState, &desc);

	/*
	// Set the proper blending state
	D3D11_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	//blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	//blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_COLOR;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_DEST_COLOR;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	//blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	resources->InitBlendState(nullptr, &blendDesc);
	*/

	// Create a new viewport to render the offscreen buffer as a texture
	float screen_res_x = (float)resources->_backbufferWidth;
	float screen_res_y = (float)resources->_backbufferHeight;
	float black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width    = screen_res_x / fZoomFactor;
	viewport.Height   = screen_res_y / fZoomFactor;
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	resources->InitViewport(&viewport);
	
	// Set the lights and set the Shading System Constant Buffer
	SetLights(resources, 1.0f);

#ifdef DEATH_STAR
	static float iTime = 0.0f;
	g_DeathStarBuffer.iMouse[0] = 0;
	g_DeathStarBuffer.iMouse[1] = 0;
	g_DeathStarBuffer.iTime = iTime;
	g_DeathStarBuffer.iResolution[0] = g_fCurScreenWidth;
	g_DeathStarBuffer.iResolution[1] = g_fCurScreenHeight;
	resources->InitPSConstantBufferDeathStar(resources->_deathStarConstantBuffer.GetAddressOf(), &g_DeathStarBuffer);
	iTime += 0.1f;
#endif

	// Set the Vertex Shader Constant buffers
	resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
		0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices
	// Set the SSDO Pixel Shader constant buffer
	g_SSAO_PSCBuffer.screenSizeX   = g_fCurScreenWidth;
	g_SSAO_PSCBuffer.screenSizeY   = g_fCurScreenHeight;
	g_SSAO_PSCBuffer.amplifyFactor = 1.0f / fZoomFactor;
	g_SSAO_PSCBuffer.fn_enable     = g_bFNEnable;
	g_SSAO_PSCBuffer.moire_offset  = g_fMoireOffsetDir;
	//g_SSAO_PSCBuffer.debug		   = g_bShowSSAODebug;
	resources->InitPSConstantBufferSSAO(resources->_ssaoConstantBuffer.GetAddressOf(), &g_SSAO_PSCBuffer);

	// Set the layout
	context->IASetInputLayout(resources->_mainInputLayout);
	resources->InitVertexShader(g_bUseSteamVR ? resources->_mainVertexShaderVR : resources->_mainVertexShader);
	
	// SSDO Direct Lighting, Left Image
	// The pos/depth texture must be resolved to _depthAsInput/_depthAsInputR already
	// Input: _randBuf, _depthBuf, _depthBuf2, _normBuf, _offscreenBuf (resolved here)
	// Output _ssaoBuf, _bentBuf
	{
		// Resolve offscreenBuf
		context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer,
			0, BACKBUFFER_FORMAT);
		context->ResolveSubresource(resources->_backgroundBufferAsInput, 0, resources->_backgroundBuffer,
			0, BACKBUFFER_FORMAT);
		if (g_bUseSteamVR) {
			context->ResolveSubresource(
				resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
				resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
			context->ResolveSubresource(
				resources->_backgroundBufferAsInput, D3D11CalcSubresource(0, 1, 1),
				resources->_backgroundBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
		}
		else {
			context->ResolveSubresource(resources->_ReticleBufAsInput, 0, resources->_ReticleBufMSAA,
				0, BACKBUFFER_FORMAT);
		}

		ID3D11ShaderResourceView *srvs_pass1[5] = {
			resources->_depthBufSRV.Get(),
			resources->_normBufSRV.Get(),
			resources->_offscreenAsInputShaderResourceView.Get(),
			resources->_ssaoMaskSRV.Get(),
			resources->_offscreenAsInputBloomMaskSRV.Get(),
		};
		//if (g_SSAO_Type == SSO_BENT_NORMALS)
			//resources->InitPixelShader(resources->_ssdoDirectBentNormalsPS);
			//resources->InitPixelShader(resources->_deathStarPS);
		//else
		resources->InitPixelShader(g_bUseSteamVR ? resources->_ssdoDirectPS_VR : resources->_ssdoDirectPS);
		//resources->InitPixelShader(resources->_ssaoPS); // Should be _ssdoDirectPS; but this will also work here

		//context->ClearRenderTargetView(resources->_renderTargetViewEmissionMask.Get(), black);
		if (g_bShowSSAODebug && !g_bBlurSSAO && !g_bEnableIndirectSSDO) {
			ID3D11RenderTargetView *rtvs[1] = {
				resources->_renderTargetView.Get(),
				//resources->_renderTargetViewBentBuf.Get(),
				//resources->_renderTargetViewEmissionMask.Get(),
			};
			context->ClearRenderTargetView(resources->_renderTargetView, black);
			//context->ClearRenderTargetView(resources->_renderTargetViewBentBuf, black);
			context->OMSetRenderTargets(1, rtvs, NULL);
			context->PSSetShaderResources(0, 5, srvs_pass1);
			if (g_bUseSteamVR)
				context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
			else
				context->Draw(6, 0);
			goto out1;
		}
		else {
			ID3D11RenderTargetView *rtvs[1] = {
				resources->_renderTargetViewSSAO.Get(),
				//resources->_renderTargetViewBentBuf.Get(),
				//resources->_renderTargetViewEmissionMask.Get(),
			};
			context->ClearRenderTargetView(resources->_renderTargetViewSSAO, black);
			//context->ClearRenderTargetView(resources->_renderTargetViewBentBuf, black);
			context->OMSetRenderTargets(1, rtvs, NULL);
			context->PSSetShaderResources(0, 5, srvs_pass1);
			if (g_bUseSteamVR)
				context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
			else
				context->Draw(6, 0);
		}
	}
	
	// Setup the constant buffers to upscale the buffers
	// The textures are always going to be g_fCurScreenWidth x g_fCurScreenHeight; but the step
	// size will be twice as big in the next pass due to the downsample, so we have to compensate
	// with a zoom factor:
	float fPixelScale = fZoomFactor;
	g_BloomPSCBuffer.pixelSizeX			= fPixelScale * g_fCurScreenWidthRcp;
	g_BloomPSCBuffer.pixelSizeY			= fPixelScale * g_fCurScreenHeightRcp;
	g_BloomPSCBuffer.amplifyFactor		= 1.0f / fZoomFactor;
	g_BloomPSCBuffer.uvStepSize			= 1.0f;
	g_BloomPSCBuffer.depth_weight		= g_SSAO_PSCBuffer.max_dist;
	// The SSDO component currently encodes the AO component in the Y channel. If
	// the SSDO buffer is displayed directly, it's hard to understand. So, instead, we
	// have to code the following logic:
	// If SSDO Indirect is disabled, then enable debug mode in the blur shader. This mode
	// returns ssao.xxx so that the SSDO direct component can be visualized properly
	// If SSDO Indirect is enabled, then we can't return ssao.xxx, because that'll wipe
	// out the AO component needed for SSDO Indirect. In that case we just pass along the
	// debug flag directly -- but the SSDO Direct buffer will be unreadable
	if (g_bShowSSAODebug) {
		if (!g_bEnableIndirectSSDO)
			g_BloomPSCBuffer.debug = 1;
		else
			g_BloomPSCBuffer.debug = g_SSAO_PSCBuffer.debug;
	}
	else
		g_BloomPSCBuffer.debug = 0;
	resources->InitPSConstantBufferBloom(resources->_bloomConstantBuffer.GetAddressOf(), &g_BloomPSCBuffer);

	// SSDO Direct Blur, Left Image
	// input: bloomOutput1 (with a copy of the ssaoBuf), depthBuf, bentBufR (with a copy of bentBuf), normBuf
	// output: ssaoBuf, bentBuf
	if (g_bBlurSSAO) 
		for (int i = 0; i < g_iSSAOBlurPasses; i++) {
			resources->InitPixelShader(g_bUseSteamVR ? resources->_ssdoBlurPS_VR : resources->_ssdoBlurPS);
			// Copy the SSAO buffer to offscreenBufferAsInput/bloomOutput1(HDR) -- we'll use it as temp buffer
			// to blur the SSAO buffer
			//context->CopyResource(resources->_offscreenBufferAsInput, resources->_ssaoBuf);
			context->CopyResource(resources->_bloomOutput1, resources->_ssaoBuf);
			//context->CopyResource(resources->_offscreenBufferAsInput, resources->_ssEmissionMask);
			// Here I'm reusing bentBufR as a temporary buffer for bentBuf, in the SteamVR path I'll do
			// the opposite. This is just to avoid having to make a temporary buffer to blur the bent normals.
			//context->CopyResource(resources->_bentBufR, resources->_bentBuf);

			//Clear the destination buffers: the blur will re-populate them
			context->ClearRenderTargetView(resources->_renderTargetViewSSAO.Get(), black);
			//context->ClearRenderTargetView(resources->_renderTargetViewBentBuf.Get(), black);
			ID3D11ShaderResourceView *srvs[3] = {
					//resources->_offscreenAsInputShaderResourceView.Get(), // LDR
					resources->_bloomOutput1SRV.Get(), // HDR
					resources->_depthBufSRV.Get(),
					resources->_normBufSRV.Get(),
					//resources->_bentBufSRV_R.Get(),
					//resources->_offscreenAsInputShaderResourceView.Get(), // emission mask
			};
			if (g_bShowSSAODebug && i == g_iSSAOBlurPasses - 1 && !g_bEnableIndirectSSDO && 	g_SSAO_Type != SSO_BENT_NORMALS) {
				context->ClearRenderTargetView(resources->_renderTargetView, black);
				// DirectX doesn't like mixing MSAA and non-MSAA RTVs. _renderTargetView is MSAA; but renderTargetViewBentBuf is not.
				// If both are set and MSAA is active, nothing gets rendered -- even with a NULL depth stencil.
				// The solution here is to avoid setting the renderTargetViewBentBuf if MSAA is active
				// Alternatively, we could change renderTargetViewBentBuf to be MSAA too, just like I did for ssMaskMSAA, etc; but... eh...
				ID3D11RenderTargetView *rtvs[1] = {
					resources->_renderTargetView.Get(), // resources->_renderTargetViewSSAO.Get(),
					//resources->_useMultisampling ? NULL : resources->_renderTargetViewBentBuf.Get(),
					//resources->_useMultisampling ? NULL : resources->_renderTargetViewEmissionMask.Get(),
				};
				context->OMSetRenderTargets(1, rtvs, NULL);
				context->PSSetShaderResources(0, 3, srvs);
				// DEBUG: Enable the following line to display the bent normals (it will also blur the bent normals buffer
				//context->PSSetShaderResources(0, 1, resources->_bentBufSRV.GetAddressOf());
				// DEBUG: Enable the following line to display the normals
				//context->PSSetShaderResources(0, 1, resources->_normBufSRV.GetAddressOf());
				// DEBUG
				if (g_bUseSteamVR)
					context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
				else
					context->Draw(6, 0);
				goto out1;
			}
			else {
				ID3D11RenderTargetView *rtvs[1] = {
					resources->_renderTargetViewSSAO.Get(),
					//resources->_renderTargetViewBentBuf.Get(),
					//resources->_renderTargetViewEmissionMask.Get(),
				};
				context->OMSetRenderTargets(1, rtvs, NULL);
				context->PSSetShaderResources(0, 3, srvs);
				if (g_bUseSteamVR)
					context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
				else
					context->Draw(6, 0);
			}
		}

	// SSDO Indirect Lighting, Left Image

	// Clear the Indirect SSDO buffer -- we have to do this regardless of whether the indirect
	// illumination is computed or not.
	context->ClearRenderTargetView(resources->_renderTargetViewSSAO_R.Get(), black);

	// The pos/depth texture must be resolved to _depthAsInput/_depthAsInputR already
	// Input: _depthBuf, _depthBuf2, _normBuf, offscreenAsInput (with a copy of _ssaoBuf -- not anymore! Should I fix this?)
	// Output _ssaoBufR
	if (g_bEnableIndirectSSDO)
	{
		resources->InitPixelShader(resources->_ssdoIndirectPS);
		// Set the SSDO pixel shader constant buffer
		//g_SSAO_PSCBuffer.screenSizeX = g_fCurScreenWidth  / fZoomFactor; // Not used in the shader
		//g_SSAO_PSCBuffer.screenSizeY = g_fCurScreenHeight / fZoomFactor; // Not used in the shader
		g_SSAO_PSCBuffer.amplifyFactor  = 1.0f / fZoomFactor;
		g_SSAO_PSCBuffer.amplifyFactor2 = 1.0f / fZoomFactor2;
		g_SSAO_PSCBuffer.moire_offset   = g_fMoireOffsetInd;
		g_SSAO_PSCBuffer.fn_enable      = g_bFNEnable;
		resources->InitPSConstantBufferSSAO(resources->_ssaoConstantBuffer.GetAddressOf(), &g_SSAO_PSCBuffer);

		// Copy the SSAO buffer to offscreenBufferAsInput -- this is the accumulated
		// color + SSDO buffer
		// Not anymore! The accumulation of color + SSDO is now done in the Add shader.
		// What happens if I just send the regular color buffer instead?
		//context->CopyResource(resources->_offscreenBufferAsInput/_bloomOutput1, resources->_ssaoBuf);

		// Resolve offscreenBuf, we need the original color buffer and it may have been overwritten
		// in the previous steps
		context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer,
			0, BACKBUFFER_FORMAT);
		if (g_bUseSteamVR)
			context->ResolveSubresource(
				resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
				resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
		ID3D11ShaderResourceView *srvs[3] = {
			resources->_depthBufSRV.Get(),  // FG Depth Buffer
			resources->_normBufSRV.Get(),   // Normal Buffer
			resources->_offscreenAsInputShaderResourceView.Get(), // Color Buffer
			//resources->_ssaoBufSRV.Get(),   // Direct SSDO from the previous pass
		};
		
		// DEBUG
		if (g_bShowSSAODebug && !g_bBlurSSAO && g_bEnableIndirectSSDO) {
			context->ClearRenderTargetView(resources->_renderTargetView, black);
			ID3D11RenderTargetView *rtvs[1] = {
				resources->_renderTargetView.Get(),
			};
			context->OMSetRenderTargets(1, rtvs, NULL);
		}
		// DEBUG
		else {
			// _renderTargetViewSSAO_R is used for the LEFT eye (and viceversa). THIS IS NOT A BUG
			context->ClearRenderTargetView(resources->_renderTargetViewSSAO_R, black);
			ID3D11RenderTargetView *rtvs[1] = {
				resources->_renderTargetViewSSAO_R.Get(),
			};
			context->OMSetRenderTargets(1, rtvs, NULL);
		}
		context->PSSetShaderResources(0, 3, srvs);
		if (g_bUseSteamVR)
			context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(6, 0);
	}

	// Blur the Indirect SSDO buffer
	if (g_bEnableIndirectSSDO && g_bBlurSSAO) {
		resources->InitPixelShader(g_bUseSteamVR ? resources->_ssdoBlurPS_VR : resources->_ssdoBlurPS);
		for (int i = 0; i < g_iSSAOBlurPasses; i++) {
			// Copy the SSDO Indirect Buffer (ssaoBufR) to offscreenBufferAsInput -- we'll use it as temp buffer
			// to blur the SSAO buffer
			context->CopyResource(resources->_bloomOutput1, resources->_ssaoBufR); // _ssaoBufR is 16bit floating point, so we use bloomOutput as temp buffer
			// Here I'm reusing bentBufR as a temporary buffer for bentBuf, in the SteamVR path I'll do
			// the opposite. This is just to avoid having to make a temporary buffer to blur the bent normals.
			//context->CopyResource(resources->_bentBufR, resources->_bentBuf);
			// Clear the destination buffers: the blur will re-populate them
			context->ClearRenderTargetView(resources->_renderTargetViewSSAO_R.Get(), black);
			ID3D11ShaderResourceView *srvs[3] = {
					//resources->_offscreenAsInputShaderResourceView.Get(), // ssaoBufR
					resources->_bloomOutput1SRV.Get(), // ssaoBufR HDR
					resources->_depthBufSRV.Get(),
					resources->_normBufSRV.Get(),
					//resources->_bentBufSRV_R.Get(),
			};
			if (g_bShowSSAODebug && i == g_iSSAOBlurPasses - 1) {
				context->ClearRenderTargetView(resources->_renderTargetView, black);
				//context->OMSetRenderTargets(1, resources->_renderTargetViewSSAO_R.GetAddressOf(), NULL);
				ID3D11RenderTargetView *rtvs[2] = {
					//resources->_renderTargetViewSSAO_R.Get(),
					resources->_renderTargetView.Get(), // resources->_renderTargetViewBentBuf.Get(),
					NULL,
				};
				context->OMSetRenderTargets(2, rtvs, NULL);
				context->PSSetShaderResources(0, 3, srvs);
				// DEBUG: Enable the following line to display the bent normals (it will also blur the bent normals buffer
				//context->PSSetShaderResources(0, 1, resources->_bentBufSRV.GetAddressOf());
				// DEBUG: Enable the following line to display the normals
				//context->PSSetShaderResources(0, 1, resources->_normBufSRV.GetAddressOf());
				if (g_bUseSteamVR)
					context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
				else
					context->Draw(6, 0);
				goto out1;
			}
			else {
				context->ClearRenderTargetView(resources->_renderTargetViewSSAO_R, black);
				ID3D11RenderTargetView *rtvs[2] = {
					resources->_renderTargetViewSSAO_R.Get(),
					NULL,
				};
				context->OMSetRenderTargets(2, rtvs, NULL);
				context->PSSetShaderResources(0, 3, srvs);
				if (g_bUseSteamVR)
					context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
				else
					context->Draw(6, 0);
			}
		}
	}
	
	// Final combine, Left Image
	{
		// DEBUG: REMOVE LATER: WE DON'T NEED TO SET THIS BUFFER ON EVERY FRAME!
		//g_BloomPSCBuffer.debug = g_SSAO_PSCBuffer.debug;
		//resources->InitPSConstantBufferBloom(resources->_bloomConstantBuffer.GetAddressOf(), &g_BloomPSCBuffer);
		// DEBUG

		// input: offscreenAsInput (resolved here), bloomMask (removed?), ssaoBuf
		// output: offscreenBuf, bloomMask?
		//if (g_SSAO_Type == SSO_BENT_NORMALS)
			//resources->InitPixelShader(resources->_hyperspacePS);
#ifndef DEATH_STAR
			//resources->InitPixelShader(resources->_ssdoAddBentNormalsPS);
#else
			resources->InitPixelShader(resources->_deathStarPS);
#endif
		if (!g_bEnableHeadLights)
		{
			if (g_bUseSteamVR)
				resources->InitPixelShader(g_SSAO_Type == SSO_PBR ? resources->_pbrAddPS_VR : resources->_ssdoAddPS_VR); // VR path
			else
				resources->InitPixelShader(g_SSAO_Type == SSO_PBR ? resources->_pbrAddPS : resources->_ssdoAddPS); // Non-VR path
		}
		else
		{
			resources->InitPixelShader(g_bUseSteamVR ? resources->_headLightsPS_VR : resources->_headLightsPS);
		}

		// Reset the viewport for the final SSAO combine
		viewport.TopLeftX	= 0.0f;
		viewport.TopLeftY	= 0.0f;
		viewport.Width		= screen_res_x;
		viewport.Height		= screen_res_y;
		viewport.MaxDepth	= D3D11_MAX_DEPTH;
		viewport.MinDepth	= D3D11_MIN_DEPTH;
		resources->InitViewport(&viewport);
		// ssaoBuf was bound as an RTV, so let's bind the RTV first to unbind ssaoBuf
		// so that it can be used as an SRV
		ID3D11RenderTargetView *rtvs[2] = {
			resources->_renderTargetView.Get(),			 // MSAA
			resources->_renderTargetViewBloomMask.Get(), // MSAA
			//NULL, //resources->_renderTargetViewBentBuf.Get(), // DEBUG REMOVE THIS LATER! Non-MSAA
			//NULL, NULL,
		};
		context->OMSetRenderTargets(2, rtvs, NULL);
		// Resolve offscreenBuf
		context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer,
			0, BACKBUFFER_FORMAT);
		if (g_bUseSteamVR) {
			context->ResolveSubresource(
				resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
				resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
			context->ResolveSubresource(
				resources->_backgroundBufferAsInput, D3D11CalcSubresource(0, 1, 1),
				resources->_backgroundBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
		}
		else {
			context->ResolveSubresource(resources->_ReticleBufAsInput, 0, resources->_ReticleBufMSAA,
				0, BACKBUFFER_FORMAT);
		}

		// In map mode, the background is empty, so we'll pass offscreenBuffer for the background SRV instead:
		if (g_bMapMode) context->CopyResource(resources->_backgroundBufferAsInput, resources->_offscreenBufferAsInput);

		ID3D11ShaderResourceView *srvs_pass2[8] = {
			resources->_offscreenAsInputShaderResourceView.Get(),	// 0: Color buffer
			resources->_ssaoBufSRV.Get(),                           // 1: SSDO Direct Component
			//resources->_ssaoBufSRV_R.Get(),                       // SSDO Indirect
			resources->_backgroundBufferSRV.Get(),                  // 2: Background buffer
			resources->_ssaoMaskSRV.Get(),                          // 3: SSAO Mask

			resources->_depthBufSRV.Get(),                          // 4: Depth buffer
			resources->_normBufSRV.Get(),                           // 5: Normals buffer
			resources->_ssMaskSRV.Get(),                            // 6: Shading System Mask buffer
			g_ShadowMapping.bEnabled ?                              // 7: The shadow map
				resources->_shadowMapArraySRV.Get() : NULL,
		};
		context->PSSetShaderResources(0, 8, srvs_pass2);

		{
			ID3D11ShaderResourceView* srvs[] = {
				resources->_transp1SRV.Get(), // 18
				resources->_transp2SRV.Get(), // 19
				resources->_ReticleSRV.Get(), // 20
			};
			context->PSSetShaderResources(18, 3, srvs);
		}

		if (g_bUseSteamVR)
			context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(6, 0);
	}

	/*******************************************************************************
	 START PROCESS FOR THE RIGHT EYE, STEAMVR MODE
	 *******************************************************************************/
out1:
	// Restore previous rendertarget, etc
	// TODO: Is this really needed?
	viewport.Width  = screen_res_x;
	viewport.Height = screen_res_y;
	resources->InitViewport(&viewport);
	resources->InitInputLayout(resources->_inputLayout);
	context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(),
		resources->_depthStencilViewL.Get());

	this->_deviceResources->EndAnnotatedEvent();
}

/* Regular deferred shading with fake HDR, no SSAO */
void PrimarySurface::DeferredPass()
{
	this->_deviceResources->BeginAnnotatedEvent(L"DeferredLightingPass");

	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	float x0, y0, x1, y1;

	if (g_bDumpSSAOBuffers)
	{
		DirectX::SaveDDSTextureToFile(context, resources->_offscreenBuffer, L"C:\\Temp\\_offscreenBufferBeforeDeferredPass.dds");
	}

	GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
	g_SSAO_PSCBuffer.x0 = x0;
	g_SSAO_PSCBuffer.y0 = y0;
	g_SSAO_PSCBuffer.x1 = x1;
	g_SSAO_PSCBuffer.y1 = y1;
	g_SSAO_PSCBuffer.enable_dist_fade = (float)g_b3DSkydomePresent;
	g_ShadingSys_PSBuffer.spec_intensity = g_bGlobalSpecToggle ? g_fSpecIntensity : 0.0f;
	g_ShadingSys_PSBuffer.spec_bloom_intensity = g_bGlobalSpecToggle ? g_fSpecBloomIntensity : 0.0f;

	// Set the vertex buffer... we probably need another vertex buffer here
	UINT stride = sizeof(MainVertex);
	UINT offset = 0;
	resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);

	// Set Primitive Topology
	// Opportunity for optimization? Make all draw calls use the same topology?
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitInputLayout(resources->_mainInputLayout);

	// Temporarily disable ZWrite: we won't need it for this effect
	D3D11_DEPTH_STENCIL_DESC desc;
	ComPtr<ID3D11DepthStencilState> depthState;
	desc.DepthEnable = FALSE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	desc.StencilEnable = FALSE;
	resources->InitDepthStencilState(depthState, &desc);

	// Create a new viewport to render the offscreen buffer as a texture
	float screen_res_x = (float)resources->_backbufferWidth;
	float screen_res_y = (float)resources->_backbufferHeight;
	float black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width    = screen_res_x;
	viewport.Height   = screen_res_y;
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	resources->InitViewport(&viewport);

	// Set the lights and the Shading System Constant Buffer
	SetLights(_deviceResources, 0.0f);

	// Set the Vertex Shader Constant buffers
	resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
		0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices
	// Set the SSDO Pixel Shader constant buffer
	g_SSAO_PSCBuffer.screenSizeX = g_fCurScreenWidth;
	g_SSAO_PSCBuffer.screenSizeY = g_fCurScreenHeight;
	g_SSAO_PSCBuffer.amplifyFactor = 1.0f;
	g_SSAO_PSCBuffer.fn_enable = g_bFNEnable;
	g_SSAO_PSCBuffer.debug = g_bShowSSAODebug;
	resources->InitPSConstantBufferSSAO(resources->_ssaoConstantBuffer.GetAddressOf(), &g_SSAO_PSCBuffer);

	// Set the layout
	context->IASetInputLayout(resources->_mainInputLayout);
	resources->InitVertexShader(g_bUseSteamVR ? resources->_mainVertexShaderVR : resources->_mainVertexShader);

	if (!g_bEnableHeadLights)
	{
		if (g_bUseSteamVR)
			resources->InitPixelShader(g_SSAO_Type == SSO_PBR ? resources->_pbrAddPS_VR : resources->_ssdoAddPS_VR); // VR path
		else
			resources->InitPixelShader(g_SSAO_Type == SSO_PBR ? resources->_pbrAddPS : resources->_ssdoAddPS); // Non-VR path
	}
	else
	{
		resources->InitPixelShader(g_bUseSteamVR ? resources->_headLightsPS_VR : resources->_headLightsPS);
	}

	// Set the PCF sampler state
	if (g_ShadowMapping.bEnabled)
		context->PSSetSamplers(7, 1, resources->_shadowPCFSamplerState.GetAddressOf());

	// The pos/depth texture must be resolved to _depthAsInput/_depthAsInputR already
	// Deferred pass, Left Image
	{
		// input: offscreenAsInput (resolved here), normBuf
		// output: offscreenBuf, bloomMask
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width	  = screen_res_x;
		viewport.Height   = screen_res_y;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		resources->InitViewport(&viewport);
		// ssaoBuf was bound as an RTV, so let's bind the RTV first to unbind ssaoBuf
		// so that it can be used as an SRV
		ID3D11RenderTargetView *rtvs[5] = {
			resources->_renderTargetView.Get(),
			resources->_renderTargetViewBloomMask.Get(),
			NULL, NULL, NULL,
		};
		context->OMSetRenderTargets(5, rtvs, NULL);

		// Resolve offscreenBuf
		context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer,
			0, BACKBUFFER_FORMAT);
		context->ResolveSubresource(resources->_backgroundBufferAsInput, 0, resources->_backgroundBuffer,
			0, BACKBUFFER_FORMAT);
		if (g_bUseSteamVR) {
			context->ResolveSubresource(
				resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
				resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
			context->ResolveSubresource(
				resources->_backgroundBufferAsInput, D3D11CalcSubresource(0, 1, 1),
				resources->_backgroundBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
		}
		else {
			context->ResolveSubresource(resources->_ReticleBufAsInput, 0, resources->_ReticleBufMSAA,
				0, BACKBUFFER_FORMAT);
		}

		// In map mode, the background is empty, so we'll pass offscreenBuffer for the background SRV instead:
		if (g_bMapMode) context->CopyResource(resources->_backgroundBufferAsInput, resources->_offscreenBufferAsInput);

		ID3D11ShaderResourceView *srvs_pass[10] = {
			resources->_offscreenAsInputShaderResourceView.Get(), // 0: Color buffer
			NULL,                                                 // 1: SSDO Direct Component (LDR)
			resources->_backgroundBufferSRV.Get(),                // 2: Background buffer
			resources->_ssaoMaskSRV.Get(),                        // 3: SSAO Mask

			resources->_depthBufSRV.Get(),                        // 4: Depth buffer
			resources->_normBufSRV.Get(),                         // 5: Normals buffer
			resources->_ssMaskSRV.Get(),                          // 6: Shading System buffer
			g_ShadowMapping.bEnabled ?                            // 7: The shadow map
				resources->_shadowMapArraySRV.Get() : NULL,
		};
		context->PSSetShaderResources(0, 8, srvs_pass);

		if (g_bRTEnabled)
		{
			ID3D11ShaderResourceView* srvs[] = {
				resources->_RTBvhSRV.Get(),        // 14
				resources->_RTMatricesSRV.Get(),   // 15
				resources->_RTTLASBvhSRV.Get(),    // 16
				resources->_rtShadowMaskSRV.Get(), // 17
			};
			// Slots 14-17 are used for Raytracing buffers (BLASes, Matrices, TLAS, RTShadowMask)
			context->PSSetShaderResources(14, 4, srvs);
		}

		{
			ID3D11ShaderResourceView* srvs[] = {
				resources->_transp1SRV.Get(), // 18
				resources->_transp2SRV.Get(), // 19
				resources->_ReticleSRV.Get(), // 20
			};
			context->PSSetShaderResources(18, 3, srvs);
		}

		if (g_bUseTextureCube)
		{
			ID3D11ShaderResourceView* srvs[] = {
				resources->_textureCubeSRV /* .Get() */, // 21
			};
			context->PSSetShaderResources(21, 1, srvs);
		}

		if (g_bUseSteamVR)
			context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(6, 0);
	}

	// Clear RT SRVs slots that are only used for the shadows to avoid DirectX errors
	ID3D11ShaderResourceView* srvs[] = { NULL, NULL, NULL, NULL };
	context->PSSetShaderResources(14, 4, srvs);

	// Restore previous rendertarget, etc
	// TODO: Is this really needed?
	viewport.Width = screen_res_x;
	viewport.Height = screen_res_y;
	resources->InitViewport(&viewport);
	resources->InitInputLayout(resources->_inputLayout);
	context->OMSetRenderTargets(1, this->_deviceResources->_renderTargetView.GetAddressOf(),
		this->_deviceResources->_depthStencilViewL.Get());

	if (g_bDumpSSAOBuffers)
	{
		DirectX::SaveDDSTextureToFile(context, resources->_offscreenBuffer, L"C:\\Temp\\_offscreenBufferAfterDeferredPass.dds");
	}

	this->_deviceResources->EndAnnotatedEvent();
}


void InitHeadingMatrix() {
	/*
	Matrix4 rotX, refl;
	rotX.identity();
	rotX.rotateX(90.0f);
	refl.set(
		1,  0,  0,  0,
		0, -1,  0,  0,
		0,  0,  1,  0,
		0,  0,  0,  1
	);
	g_ReflRotX = refl * rotX;
	static bool bFirstTime = true;
	if (bFirstTime) {
		ShowMatrix4(g_ReflRotX, "g_ReflRotX");
	}

	// This is the matrix we want (looks like it's a simple Y-Z axis swap
		[14444][DBG] 1.000000, 0.000000, 0.000000, 0.000000
		[14444][DBG] 0.000000, 0.000000, 1.000000, 0.000000
		[14444][DBG] 0.000000, 1.000000, 0.000000, 0.000000
		[14444][DBG] 0.000000, 0.000000, 0.000000, 1.000000
	*/

	g_ReflRotX.set(
		1.0, 0.0, 0.0, 0.0, // 1st column
		0.0, 0.0, 1.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 1.0
	);
}

/*
 * For a simple direction vector Fs (like a light vector), return
 * a matrix R such that:
 * 
 * R * Fs = |F| * [0, 0, 1]
 * 
 * In other words, R aligns Fs with Z+.
 */
Matrix4 GetSimpleDirectionMatrix(Vector4 Fs, bool invert) {
	Vector4 temp = Fs;
	temp.normalize(); // TODO: Check if we need to normalize every time
	// Rotate the vector around the X-axis to align it with the X-Z plane
	float AngX = atan2(temp.y, temp.z) / 0.01745f;
	float AngY = -asin(temp.x) / 0.01745f;
	Matrix4 rotX, rotY, rotFull;
	rotX.rotateX(AngX);
	rotY.rotateY(AngY);
	rotFull = rotY * rotX;
	// DEBUG
	//Vector4 debugFs = rotFull * temp;
	// The following line should always display: (0,0,1)
	//log_debug("[DBG] debugFs: %0.3f, %0.3f, %0.3f", debugFs.x, debugFs.y, debugFs.z);
	// DEBUG
	if (invert)
		rotFull.invert(); // Full inversion for now, should replace for either transpose, or just invert AngX,AngY
	return rotFull;
}

/*
 * For a *unitary* direction vector Fs, return a matrix R such that:
 *
 * R * Fs = [0, 0, 1]
 *
 * In other words, R aligns Fs with Z+.
 */
Matrix4 GetDirectionMatrixUnitaryVector(Vector4 Fs) {
	Vector4 temp = Fs;
	//if (temp.x < -1.0f) temp.x = -1.0f;
	//if (temp.x > 1.0f) temp.x = 1.0f;
	// Rotate the vector around the X-axis to align it with the X-Z plane
	float AngX = atan2(temp.y, temp.z) * RAD_TO_DEG;
	float AngY = -asin(temp.x) * RAD_TO_DEG;
	Matrix4 rotX, rotY, rotFull;
	rotX.rotateX(AngX);
	rotY.rotateY(AngY);
	rotFull = rotY * rotX;
	// DEBUG
	//Vector4 debugFs = rotFull * temp;
	// The following line should always display: (0,0,1)
	//log_debug("[DBG] debugFs: %0.3f, %0.3f, %0.3f", debugFs.x, debugFs.y, debugFs.z);
	// DEBUG
	return rotFull;
}

/*
 * Return a rotation matrix that rotates about axis A.
 * A must be an unitary direction vector.
 */
Matrix4 RotateAroundAxis(const Vector4 A, const float degrees)
{
	Matrix4 R = GetDirectionMatrixUnitaryVector(A);
	Matrix4 Rinv = R;
	Matrix4 Rot;
	Rot.rotate(degrees, 0.0f, 0.0f, 1.0f);
	Rinv.invert(); // TODO: Maybe we only need to do a transpose here, R *should* be orthonormal
	return Rinv * Rot * R;
}

/*
 * XWA's coordinate system:
 * X+ = right 
 * Y+ = forward
 * Z+ = up
 */
Vector4 PlayerRs(1,0,0, 0);
Vector4 PlayerUs(0,0,1, 0);
Vector4 PlayerFs(0,1,0, 0);

float AngleDiff(float deg1, float deg2)
{
	while (deg1 < -0.001f) deg1 += 360.0f;
	while (deg2 < -0.001f) deg2 += 360.0f;
	return fabs(deg1 - deg2);
}

void ReOrthogonalize(Vector4& R, Vector4& U, Vector4& F)
{
	// Compute a new U
	// Rx Ry Rz
	// Fx Fy Fz
	U.x = R.y * F.z - R.z * F.y;
	U.y = R.z * F.x - R.x * F.z;
	U.z = R.x * F.y - R.y * F.x;

	// Compute a new R
	// Fx Fy Fz
	// Ux Uy Uz
	R.x = F.y * U.z - F.z * U.y;
	R.y = F.z * U.x - F.x * U.z;
	R.z = F.x * U.y - F.y * U.x;
}

/// <summary>
/// Converts an orthonormal Us,Fs system to XWA's yaw, pitch, roll
/// </summary>
/// <param name="Us">The unitary up vector</param>
/// <param name="Fs">The unitary forward vector</param>
/// <param name="yaw_out"></param>
/// <param name="pitch_out"></param>
/// <param name="roll_out"></param>
/// <param name="invert_roll"></param>
void ShipOrientationToYPR(const Vector4& Us, const Vector4& Fs, float* yaw_out, float* pitch_out, float* roll_out)
{
	Matrix4 rotMatrixYaw, rotMatrixPitch, rotMatrixFull;
	*yaw_out = atan2(Fs.y, Fs.x);
	*pitch_out = acos(Fs.z / Fs.length());

	rotMatrixYaw.identity();   rotMatrixYaw.rotateZ(90.0f - (*yaw_out * RAD_TO_DEG));
	rotMatrixPitch.identity(); rotMatrixPitch.rotateX((*pitch_out * RAD_TO_DEG) - 90.0f);

	// The following transform should align Fs with Y+:
	rotMatrixFull = rotMatrixPitch * rotMatrixYaw;
	// This transformation makes testU lie in the X-Z plane (y --> 0)
	Vector4 testU = rotMatrixFull * Us;
	testU.normalize();
	//Vector4 F = rotMatrixFull * Fs;
	//log_debug("[DBG] (CHECK) F: [%0.3f, %0.3f, %0.3f]", F.x, F.y, F.z);
	//log_debug("[DBG] (CHECK) testU: [%0.3f, %0.3f, %0.3f]", testU.x, testU.y, testU.z);
	//log_debug("[DBG] (CHECK) Us: [%0.3f, %0.3f, %0.3f], testU: [%0.3f, %0.3f, %0.3f]",
	//	Us.x, Us.y, Us.z,
	//	testU.x, testU.y, testU.z);

	*roll_out = -acos(testU.z);
	if (testU.x > 0) *roll_out = -(*roll_out);
}

void YPRToShipOrientation(float yaw_deg, float pitch_deg, float roll_deg, Vector4 &Rs, Vector4& Us, Vector4& Fs)
{
	Matrix4 rotMatrixYaw, rotMatrixPitch, rotMatrixRoll, rotMatrixFull;

	rotMatrixYaw.identity();   rotMatrixYaw.rotateZ(90.0f - yaw_deg);
	rotMatrixPitch.identity(); rotMatrixPitch.rotateX(pitch_deg - 90.0f);
	rotMatrixRoll.identity();  rotMatrixPitch.rotateY(-roll_deg);

	// The following transform should align Fs with Y+:
	rotMatrixFull = rotMatrixPitch * rotMatrixYaw;
	// Now we build the canonical orthonormal system
	Rs.x = 1; Rs.y = 0; Rs.z = 0; Rs.w = 0;
	Us.x = 0; Us.y = 0; Us.z = 1; Us.w = 0;
	Fs.x = 0; Fs.y = 1; Fs.z = 0; Fs.w = 0;
	// Apply the roll
	Rs = rotMatrixRoll * Rs;
	Us = rotMatrixRoll * Us;
	// Invert the transform
	rotMatrixFull.invert();
	// Transform RUF back into the global coord system
	Rs = rotMatrixFull * Rs;
	Us = rotMatrixFull * Us;
	Fs = rotMatrixFull * Fs;
	ReOrthogonalize(Rs, Us, Fs);
	Rs.normalize(); Us.normalize(); Fs.normalize();
}

void InitializePlayerYawPitchRoll()
{
	ObjectEntry* object = NULL;
	MobileObjectEntry* mobileObject = NULL;

	if (GetPlayerCraftInstanceSafe(&object, &mobileObject) == NULL)
		return;
	if (object == NULL)
		return;

	Vector4 Rs, Us, Fs;
	Rs.y = -mobileObject->transformMatrix.Right_X / 32768.0f;
	Rs.x = -mobileObject->transformMatrix.Right_Y / 32768.0f;
	Rs.z = -mobileObject->transformMatrix.Right_Z / 32768.0f;
	Rs.w = 0;
	float R = Rs.length();

	Us.y = mobileObject->transformMatrix.Up_X / 32768.0f;
	Us.x = mobileObject->transformMatrix.Up_Y / 32768.0f;
	Us.z = mobileObject->transformMatrix.Up_Z / 32768.0f;
	Us.w = 0;
	float U = Us.length();

	Fs.y = mobileObject->transformMatrix.Front_X / 32768.0f;
	Fs.x = mobileObject->transformMatrix.Front_Y / 32768.0f;
	Fs.z = mobileObject->transformMatrix.Front_Z / 32768.0f;
	Fs.w = 0;
	float F = Fs.length();

	if (R < 0.5f || U < 0.5f || F < 0.5f)
		// RUF must be orthonormal, but in some cases, the rotation matrix is
		// not ready yet and we need to ignore it.
		return;

	float dotR = Rs.x * PlayerRs.x + Rs.y * PlayerRs.y + Rs.z * PlayerRs.z;
	float dotU = Us.x * PlayerUs.x + Us.y * PlayerUs.y + Us.z * PlayerUs.z;
	float dotF = Fs.x * PlayerFs.x + Fs.y * PlayerFs.y + Fs.z * PlayerFs.z;
	//log_debug("[DBG] dotRUF: %0.3f, %0.3f, %0.3f", dotR, dotU, dotF);
	// If any of these dot products is lower than 0.99, then the game has
	// modified the orientation of the ship and we need to update PlayerRUF
	// accordingly
	if (dotR < 0.999f || dotU < 0.999f || dotF < 0.999f)
	{
		PlayerRs = Rs;
		PlayerUs = Us;
		PlayerFs = Fs;
		ReOrthogonalize(PlayerRs, PlayerUs, PlayerFs);
		PlayerRs.normalize();
		PlayerUs.normalize();
		PlayerFs.normalize();
		//log_debug("[DBG] Initialized PlayerRUF");
		/*
		log_debug("[DBG] Init:RUF: [%0.3f, %0.3f, %0.3f], [%0.3f, %0.3f, %0.3f], [%0.3f, %0.3f, %0.3f]",
			PlayerRs.x, PlayerRs.y, PlayerRs.z,
			PlayerUs.x, PlayerUs.y, PlayerUs.z,
			PlayerFs.x, PlayerFs.y, PlayerFs.z);
		*/
	}

	/*
	const float yaw   = object->yaw   / 65535.0f * 360.0f;
	const float pitch = object->pitch / 65535.0f * 360.0f;
	const float roll  = object->roll  / 65535.0f * 360.0f;
	YPRToShipOrientation(yaw, pitch, roll, PlayerRs, PlayerUs, PlayerFs);
	*/
}

void ApplyYawPitchRoll(float yaw_inc_deg, float pitch_inc_deg, float roll_inc_deg)
{
	ObjectEntry* object = NULL;

	if (GetPlayerCraftInstanceSafe(&object) == NULL)
		return;
	if (object == NULL)
		return;

	// This is how we could conceivably write the RUF matrix directly to the
	// mobileObject, but we're going to need a hook:
	/*
	mobileObject->transformMatrix.Right_X = (short)(PlayerRs.x * 32768.0f);
	mobileObject->transformMatrix.Right_Y = (short)(PlayerRs.y * 32768.0f);
	mobileObject->transformMatrix.Right_Z = (short)(PlayerRs.z * 32768.0f);

	mobileObject->transformMatrix.Up_X = (short)(PlayerUs.x * 32768.0f);
	mobileObject->transformMatrix.Up_Y = (short)(PlayerUs.y * 32768.0f);
	mobileObject->transformMatrix.Up_Z = (short)(PlayerUs.z * 32768.0f);

	mobileObject->transformMatrix.Front_X = (short)(PlayerFs.x * 32768.0f);
	mobileObject->transformMatrix.Front_Y = (short)(PlayerFs.y * 32768.0f);
	mobileObject->transformMatrix.Front_Z = (short)(PlayerFs.z * 32768.0f);
	return;
	*/

	// The player's Rs,Us,Fs system must be rotated about the Us axis to apply yaw
	Matrix4 R_yaw = RotateAroundAxis(PlayerUs, yaw_inc_deg);
	PlayerRs = R_yaw * PlayerRs;
	PlayerUs = R_yaw * PlayerUs;
	PlayerFs = R_yaw * PlayerFs;
	PlayerRs.normalize(); PlayerUs.normalize(); PlayerFs.normalize();

	// Now we need to rotate the system around the Rs axis
	Matrix4 R_pitch = RotateAroundAxis(PlayerRs, pitch_inc_deg);
	PlayerRs = R_pitch * PlayerRs;
	PlayerUs = R_pitch * PlayerUs;
	PlayerFs = R_pitch * PlayerFs;
	PlayerRs.normalize(); PlayerUs.normalize(); PlayerFs.normalize();

	// Finally, we can rotate the system around the Fs axis
	Matrix4 R_roll = RotateAroundAxis(PlayerFs, roll_inc_deg);
	PlayerRs = R_roll * PlayerRs;
	PlayerUs = R_roll * PlayerUs;
	PlayerFs = R_roll * PlayerFs;
	ReOrthogonalize(PlayerRs, PlayerUs, PlayerFs);
	PlayerRs.normalize(); PlayerUs.normalize(); PlayerFs.normalize();

	// The following matrix satisfies:
	// M * RUF = I
	// In other words, M transforms RUF back into the major axes
	// meaning that M' transforms major axes into RUF
	/*
	Matrix4 M(
		PlayerRs.x, PlayerUs.x, PlayerFs.x, 0,
		PlayerRs.y, PlayerUs.y, PlayerFs.y, 0,
		PlayerRs.z, PlayerUs.z, PlayerFs.z, 0,
		0, 0, 0, 1
	);
	Vector4 Rs = M * PlayerRs;
	Vector4 Us = M * PlayerUs;
	Vector4 Fs = M * PlayerFs;
	// The following should always display (1,0,0), (0,1,0), (0,0,1)
	log_debug("[DBG] RUFs: [%0.3f, %0.3f, %0.3f], [%0.3f, %0.3f, %0.3f], [%0.3f, %0.3f, %0.3f]",
		Rs.x, Rs.y, Rs.z,
		Us.x, Us.y, Us.z,
		Fs.x, Fs.y, Fs.z);
	*/

	float yaw, pitch, roll;
	ShipOrientationToYPR(PlayerUs, PlayerFs, &yaw, &pitch, &roll);

	object->yaw   = (short)(yaw   / 3.141593f * 32768.0f);
	object->pitch = (short)(pitch / 3.141593f * 32768.0f);
	object->roll  = (short)(roll  / 3.141593f * 32768.0f);
}

// This is the function that I used to figure out the formulas used to convert an
// orthonormal RUF system into XWA yaw, pitch, roll
void TestShipOrientationToYPR(Vector4& Rs_out, Vector4& Us_out, Vector4& Fs_out, bool invert=false, bool debug=false)
{
	Matrix4 rotMatrixFull, rotMatrixYaw, rotMatrixPitch, rotMatrixRoll;
	Vector4 Rs, Us, Fs;
	ObjectEntry* object = NULL;

	if (GetPlayerCraftInstanceSafe(&object) == NULL)
		return;
	if (object == NULL)
		return;

	// The range for these angles is [0..65535] = [0..360]
	const float yaw   = object->yaw   / 65535.0f * 360.0f;
	const float pitch = object->pitch / 65535.0f * 360.0f;
	const float roll  = object->roll  / 65535.0f * 360.0f;

	// Compute the full rotation that aligns ypr with Y+ (forward)
	rotMatrixFull.identity();
	rotMatrixYaw.identity();   rotMatrixYaw.rotateZ(90 - yaw);
	rotMatrixPitch.identity(); rotMatrixPitch.rotateX(pitch - 90);
	rotMatrixRoll.identity();  rotMatrixRoll.rotateY(-roll);

	// rotMatrixYaw aligns the orientation with the y-z plane (x --> 0)
	// rotMatrixPitch aligns the orientation with the x-z plane (z --> 0)
	// So the vector is aligned with Y+ (forward) and the roll is applied along that axis.

	float cosTheta, cosPhi, sinTheta, sinPhi;
	cosTheta = cos(yaw * DEG_TO_RAD), sinTheta = sin(yaw * DEG_TO_RAD);
	cosPhi = cos(pitch * DEG_TO_RAD), sinPhi = sin(pitch * DEG_TO_RAD);

	Fs.x = sinPhi * cosTheta;
	Fs.y = sinPhi * sinTheta;
	Fs.z = cosPhi;
	const float test_yaw = atan2(Fs.y, Fs.x);
	const float test_pitch = acos(Fs.z / Fs.length());

	// The yaw is indeed the z-axis (up) rotation, it goes from -180 to 0 to 180.
	// When pitch == 90, the craft is actually seeing the horizon
	// When pitch == 0, the craft is looking towards the sun

	// The following transform chain will always transform (Fs.x,Fs.y,Fs.z) into (0, 1, 0) (forward)
	// To verify this, display Fs after multiplying it by rotMatrixFull
	rotMatrixFull = rotMatrixPitch * rotMatrixYaw;
	//Fs = rotMatrixFull * Fs;
	//log_debug("[DBG] Fs(DEBUG): [%0.3f, %0.3f, %0.3f]", Fs.x, Fs.y, Fs.z);

	// Fs = (0, 1, 0) after the previous transform
	Us.x = 0; Us.y = 0; Us.z = 1; Us.w = 0; // Up vector
	Rs.x = 1; Rs.y = 0; Rs.z = 0; Rs.w = 0; // Right vector
	// Apply the roll around the Y+ axis (forward)
	Us = rotMatrixRoll * Us;
	Rs = rotMatrixRoll * Rs;
	float test_roll = acos(Us.z);
	if (Us.x > 0) test_roll = -test_roll;

	// Invert the transforms to go back to the original frame of reference, but now
	// we'll have a new orthonormal system: testRUF. testRUF should be equivalent
	// to the original ypr
	rotMatrixFull.invert();
	Vector4 testR = rotMatrixFull * Rs;
	Vector4 testU = rotMatrixFull * Us;
	Vector4 testF = rotMatrixFull * Vector4(0, 1, 0, 0);
	// TEST: testF and F should be equal
	//float dotF = Fs.x * testF.x + Fs.y * testF.y + Fs.z * testF.z;
	// TEST: if testF = [0,1,0], then testR = [1,0,0] and testU = [0,0,1]
	/*
	log_debug("[DBG] Fs:[%0.3f, %0.3f, %0.3f] (%d) Rs: [%0.3f, %0.3f, %0.3f], Us: [%0.3f, %0.3f, %0.3f]",
		testF.x, testF.y, testF.z, fabs(dotF - 1.0) < 0.001f,
		testR.x, testR.y, testR.z,
		testU.x, testU.y, testU.z);
	*/
	
	float diff_y = AngleDiff(yaw,   test_yaw   * RAD_TO_DEG);
	float diff_p = AngleDiff(pitch, test_pitch * RAD_TO_DEG);
	float diff_r = AngleDiff(roll,  test_roll  * RAD_TO_DEG);
	
	// Here we're comparing the original yaw,pitch,roll with the ypr coming
	// from testRUF. They should be equal, thus proving that our formulas are
	// correct.
	log_debug("[DBG] ypr(1): (%0.1f, %0.1f, %0.1f)-(%0.1f, %0.1f, %0.1f)::eq:[%d, %d, %d]",
		yaw, pitch, roll,
		test_yaw * RAD_TO_DEG, test_pitch * RAD_TO_DEG, test_roll * RAD_TO_DEG,
		diff_y < 0.01f, diff_p < 0.01f, diff_r < 0.01f);

	// Now let's test that ShipOrientationToYPR() also returns the same angles
	float y, p, r;
	ShipOrientationToYPR(testU, testF, &y, &p, &r);
	r = -r;
	diff_y = AngleDiff(yaw,   y * RAD_TO_DEG);
	diff_p = AngleDiff(pitch, p * RAD_TO_DEG);
	diff_r = AngleDiff(roll,  r * RAD_TO_DEG);
	log_debug("[DBG] ypr(2): (%0.1f, %0.1f, %0.1f)-(%0.1f, %0.1f, %0.1f)::eq:[%d, %d, %d]",
		yaw, pitch, roll,
		y * RAD_TO_DEG, p * RAD_TO_DEG, r * RAD_TO_DEG,
		diff_y < 0.01f, diff_p < 0.01f, diff_r < 0.01f);

	// Apply the new ypr to object (this operation should be idempotent)
	//object->yaw   = (short)(y / 3.141593f * 32768.0f);
	//object->pitch = (short)(p / 3.141593f * 32768.0f);
	//object->roll  = (short)(r / 3.141593f * 32768.0f);
}

/*
 * Compute the current ship's orientation. Returns:
 * Rs: The "Right" vector in global coordinates
 * Us: The "Up" vector in global coordinates
 * Fs: The "Forward" vector in global coordinates
 * When invert=false:
 *     A matrix that maps [Rs, Us, Fs] to the major [X, Y, Z] axes
 */
Matrix4 GetCurrentHeadingMatrix(Vector4 &Rs, Vector4 &Us, Vector4 &Fs, bool invert=false, bool debug=false)
{
	const float DEG2RAD = 3.141593f / 180;
	float yaw, pitch, roll;
	Matrix4 rotMatrixFull, rotMatrixYaw, rotMatrixPitch, rotMatrixRoll;
	Vector4 T, B, N;
	// Compute the full rotation
	yaw   = PlayerDataTable[*g_playerIndex].Camera.CraftYaw   / 65536.0f * 360.0f;
	pitch = PlayerDataTable[*g_playerIndex].Camera.CraftPitch / 65536.0f * 360.0f;
	roll  = PlayerDataTable[*g_playerIndex].Camera.CraftRoll  / 65536.0f * 360.0f;

	// To test how (x,y,z) is aligned with either the Y+ or Z+ axis, just multiply rotMatrixPitch * rotMatrixYaw * (x,y,z)
	//Matrix4 rotMatrixFull, rotMatrixYaw, rotMatrixPitch, rotMatrixRoll;
	rotMatrixFull.identity();
	rotMatrixYaw.identity();   rotMatrixYaw.rotateY(-yaw);
	rotMatrixPitch.identity(); rotMatrixPitch.rotateX(-pitch);
	rotMatrixRoll.identity();  rotMatrixRoll.rotateY(roll);

	// rotMatrixYaw aligns the orientation with the y-z plane (x --> 0)
	// rotMatrixPitch * rotMatrixYaw aligns the orientation with y+ (x --> 0 && z --> 0)
	// so the remaining rotation must be around the y axis (?)
	// DEBUG, x = z, y = x, z = y;
	// The yaw is indeed the y-axis rotation, it goes from -180 to 0 to 180.
	// When pitch == 90, the craft is actually seeing the horizon
	// When pitch == 0, the craft is looking towards the sun
	// New approach: let's build a TBN system here to avoid the gimbal lock problem
	float cosTheta, cosPhi, sinTheta, sinPhi;
	cosTheta = cos(yaw * DEG2RAD), sinTheta = sin(yaw * DEG2RAD);
	cosPhi = cos(pitch * DEG2RAD), sinPhi = sin(pitch * DEG2RAD);
	N.z = cosTheta * sinPhi;
	N.x = sinTheta * sinPhi;
	N.y = cosPhi;
	N.w = 0;

	// This transform chain will always transform (N.x,N.y,N.z) into (0, 1, 0)
	// To make an orthonormal basis, we need x+ and z+
	N = rotMatrixPitch * rotMatrixYaw * N;
	//log_debug("[DBG] N(DEBUG): %0.3f, %0.3f, %0.3f", N.x, N.y, N.z); // --> displays (0,1,0)
	B.x = 0; B.y = 0; B.z = -1; B.w = 0;
	T.x = 1; T.y = 0; T.z =  0; T.w = 0;
	B = rotMatrixRoll * B;
	T = rotMatrixRoll * T;
	// Our new basis is T,B,N; but we need to invert the yaw/pitch rotation we applied
	rotMatrixFull = rotMatrixPitch * rotMatrixYaw;
	rotMatrixFull.invert();
	T = rotMatrixFull * T;
	B = rotMatrixFull * B;
	N = rotMatrixFull * N;
	// Our TBN basis is now in absolute coordinates
	Fs = g_ReflRotX * N;
	Us = g_ReflRotX * B;
	Rs = g_ReflRotX * T;
	Fs.w = 0; Rs.w = 0; Us.w = 0;
	// This transform chain gets us the orientation of the craft in XWA's coord system:
	// [1,0,0] is right, [0,1,0] is forward, [0,0,1] is up
	//log_debug("[DBG] [GUN] Fs: [%0.3f, %0.3f, %0.3f] ypr: %0.3f, %0.3f, %0.3f ***********", Fs.x, Fs.y, Fs.z, yaw, pitch, roll);

	// Facing forward we get: T: [1, 0, 0], B: [0, -1, 0], N: [0, 0, -1]
	if (debug)
		log_debug("[DBG] [H] Rs: [%0.3f, %0.3f, %0.3f], Us: [%0.3f, %0.3f, %0.3f], Fs: [%0.3f, %0.3f, %0.3f]",
			Rs.x, Rs.y, Rs.z, Us.x, Us.y, Us.z, Fs.x, Fs.y, Fs.z);

	Matrix4 viewMatrix;
	if (!invert) { // Transform current ship's heading to Global Coordinates (Major Axes)
		viewMatrix = Matrix4(
			Rs.x, Us.x, Fs.x, 0,
			Rs.y, Us.y, Fs.y, 0,
			Rs.z, Us.z, Fs.z, 0,
			0, 0, 0, 1
		);
		// Rs, Us, Fs is an orthonormal basis
	} else { // Transform Global Coordinates to the Ship's Coordinate System
		viewMatrix = Matrix4(
			Rs.x, Rs.y, Rs.z, 0,
			Us.x, Us.y, Us.z, 0,
			Fs.x, Fs.y, Fs.z, 0,
			0, 0, 0, 1
		);
		// Rs, Us, Fs is an orthonormal basis
	}
	return viewMatrix;
}

/*
 * Get the combined Heading + Cockpit Camera View matrix that transforms from XWA's system to
 * PixelShader (Z+) coordinates.
 */
Matrix4 GetCurrentHeadingViewMatrix() {
	Vector4 Rs, Us, Fs;
	Matrix4 H = GetCurrentHeadingMatrix(Rs, Us, Fs, false, false);

	if (g_bEnableVR) {
		Matrix4 ViewMatrix = g_VSMatrixCB.fullViewMat;
		// Apply the headtracking transform to the ViewMatrix
		ViewMatrix.invert();
		if (PlayerDataTable[*g_playerIndex].gunnerTurretActive) {
			Matrix4 GunnerMatrix;
			Matrix4 S = Matrix4().scale(1, 1, -1);
			GetGunnerTurretViewMatrixSpeedEffect(&GunnerMatrix);
			ViewMatrix = S * ViewMatrix * S * GunnerMatrix;
		}
		else {
			Matrix4 S = Matrix4().scale(1, 1, -1);
			ViewMatrix = S * g_VSMatrixCB.fullViewMat * S * H;
		}
		return ViewMatrix;
	}

	float viewYaw, viewPitch;
	if (PlayerDataTable[*g_playerIndex].Camera.ExternalCamera) {
		viewYaw   = PlayerDataTable[*g_playerIndex].Camera.Yaw   / 65536.0f * 360.0f;
		viewPitch = PlayerDataTable[*g_playerIndex].Camera.Pitch / 65536.0f * 360.0f;
	}
	else {
		viewYaw   = PlayerDataTable[*g_playerIndex].MousePositionX / 65536.0f * 360.0f;
		viewPitch = PlayerDataTable[*g_playerIndex].MousePositionY / 65536.0f * 360.0f;
	}
	Matrix4 viewMatrixYaw, viewMatrixPitch;
	viewMatrixYaw.identity();
	viewMatrixPitch.identity();
	viewMatrixYaw.rotateY(g_fViewYawSign   * viewYaw);
	viewMatrixYaw.rotateX(g_fViewPitchSign * viewPitch);
	return viewMatrixPitch * viewMatrixYaw * H;
}

void GetCockpitViewMatrix(Matrix4 *result, bool invert=true) {
	float yaw, pitch;
	
	if (PlayerDataTable[*g_playerIndex].Camera.ExternalCamera) {
		yaw   = (float)PlayerDataTable[*g_playerIndex].Camera.Yaw   / 65536.0f * 360.0f + 180.0f;
		pitch = (float)PlayerDataTable[*g_playerIndex].Camera.Pitch / 65536.0f * 360.0f;
	}
	else {
		yaw   = (float)PlayerDataTable[*g_playerIndex].MousePositionX / 65536.0f * 360.0f + 180.0f;
		pitch = (float)PlayerDataTable[*g_playerIndex].MousePositionY / 65536.0f * 360.0f;
	}

	Matrix4 rotMatrixFull, rotMatrixYaw, rotMatrixPitch, rotMatrixRoll;
	rotMatrixFull.identity();
	rotMatrixYaw.identity();   rotMatrixYaw.rotateY(yaw);
	rotMatrixPitch.identity(); rotMatrixPitch.rotateX(pitch);
	rotMatrixFull = rotMatrixPitch * rotMatrixYaw;
	if (invert)
		*result = rotMatrixFull.invert();
	else
		*result = rotMatrixFull;
}

/*
 * Returns the current camera or external matrix for the speed effect. The coord sys is slightly
 * different from the one used in the shadertoy pixel shaders, so we need a new version of
 * GetCockpitViewMatrix.
 */
void GetCockpitViewMatrixSpeedEffect(Matrix4 *result, bool invert=true) {
	
	Matrix4 rotMatrixFull;

	if (PlayerDataTable[*g_playerIndex].gunnerTurretActive)
	{
		GetGunnerTurretViewMatrixSpeedEffect(&rotMatrixFull);
	}
	else 
	{
		float yaw, pitch;

		if (PlayerDataTable[*g_playerIndex].Camera.ExternalCamera) {
			yaw = -(float)PlayerDataTable[*g_playerIndex].Camera.Yaw / 65536.0f * 360.0f;
			pitch = (float)PlayerDataTable[*g_playerIndex].Camera.Pitch / 65536.0f * 360.0f;
		}
		else {
			yaw = -(float)PlayerDataTable[*g_playerIndex].MousePositionX / 65536.0f * 360.0f;
			pitch = (float)PlayerDataTable[*g_playerIndex].MousePositionY / 65536.0f * 360.0f;
		}

		Matrix4 rotMatrixYaw, rotMatrixPitch, rotMatrixRoll;
		rotMatrixFull.identity();
		rotMatrixYaw.identity();   rotMatrixYaw.rotateY(yaw);
		rotMatrixPitch.identity(); rotMatrixPitch.rotateX(pitch);
		rotMatrixFull = rotMatrixPitch * rotMatrixYaw;
	}

	if (invert)
		*result = rotMatrixFull.invert();
	else
		*result = rotMatrixFull;
}

/// <summary>
/// Same as GetCockpitViewMatrixSpeedEffect(), but yaw is applied around the Z axis because
/// in OPT coords, that's the vertical axis. Used to rotate objects in global OPT coords.
/// </summary>
void GetCockpitViewMatrixSpeedEffectOPTSys(Matrix4 *result, bool invert=true) {

	Matrix4 rotMatrixFull;

	if (PlayerDataTable[*g_playerIndex].gunnerTurretActive)
	{
		// TODO:
		GetGunnerTurretViewMatrixSpeedEffect(&rotMatrixFull);
	}
	else
	{
		float yaw, pitch;

		if (PlayerDataTable[*g_playerIndex].Camera.ExternalCamera) {
			yaw = -(float)PlayerDataTable[*g_playerIndex].Camera.Yaw / 65536.0f * 360.0f;
			pitch = (float)PlayerDataTable[*g_playerIndex].Camera.Pitch / 65536.0f * 360.0f;
		}
		else {
			yaw = -(float)PlayerDataTable[*g_playerIndex].MousePositionX / 65536.0f * 360.0f;
			pitch = (float)PlayerDataTable[*g_playerIndex].MousePositionY / 65536.0f * 360.0f;
		}

		Matrix4 rotMatrixYaw, rotMatrixPitch, rotMatrixRoll;
		rotMatrixFull.identity();
		rotMatrixYaw.identity();   rotMatrixYaw.rotateZ(yaw);
		rotMatrixPitch.identity(); rotMatrixPitch.rotateX(pitch);
		rotMatrixFull = rotMatrixPitch * rotMatrixYaw;
	}

	if (invert)
		*result = rotMatrixFull.invert();
	else
		*result = rotMatrixFull;
}

/*
 * Returns the gunner turret view matrix
 */
void GetGunnerTurretViewMatrix(Matrix4 *result) {
	// This is what the matrix looks like when looking forward:
	// F: [-0.257, 0.963, 0.080], R: [0.000, 0.083, -0.996], U: [-0.966, -0.256, -0.021]
	const float factor = 32768.0f;
	/*
	short *Turret = (short *)(0x8B94E0 + 0x21E);
	Vector3 F(Turret[0] / factor, Turret[1] / factor, Turret[2] / factor);
	Vector3 R(Turret[3] / factor, Turret[4] / factor, Turret[5] / factor);
	Vector3 U(Turret[6] / factor, Turret[7] / factor, Turret[8] / factor);
	*/
	Vector3 F(
		PlayerDataTable[*g_playerIndex].gunnerTurretF[0] / factor,
		PlayerDataTable[*g_playerIndex].gunnerTurretF[1] / factor,
		PlayerDataTable[*g_playerIndex].gunnerTurretF[2] / factor
	);
	Vector3 R(
		PlayerDataTable[*g_playerIndex].gunnerTurretR[0] / factor,
		PlayerDataTable[*g_playerIndex].gunnerTurretR[1] / factor,
		PlayerDataTable[*g_playerIndex].gunnerTurretR[2] / factor
	);
	Vector3 U(
		PlayerDataTable[*g_playerIndex].gunnerTurretU[0] / factor,
		PlayerDataTable[*g_playerIndex].gunnerTurretU[1] / factor,
		PlayerDataTable[*g_playerIndex].gunnerTurretU[2] / factor
	);

	// Pointing straight at the sun or "straight up":
	// [4344] [DBG] F: [-0.015, -0.003, 1.000], R: [0.000, 1.000, 0.002], U: [-1.000, 0.000, -0.015]
	// [12316] [DBG] F: [-0.004, 0.146, 0.989], R: [0.000, 0.989, -0.146], U: [-1.000, -0.000, -0.004]
	// Pointing up again. ship looking at 90 to the right:
	// [12316] [DBG] F: [0.020, 0.001, 0.999], R: [0.984, -0.177, -0.020], U: [0.177, 0.984, -0.004]
	// Pointing up again, ship looking at 45 in the horizon:
	// [12316] [DBG] F: [0.004, 0.061, 0.998], R: [0.691, 0.721, -0.047], U: [-0.723, 0.690, -0.039]
	// Pointing up again; ship looking backwards:
	// [12316] [DBG] F: [0.040, -0.013, 0.999], R: [-0.030, -0.999, -0.011], U: [0.998, -0.030, -0.040]
	// Pointing up again, ship looking -90 left:
	// [12316] [DBG] F: [-0.060, 0.003, 0.998], R: [-0.993, 0.097, -0.060], U: [-0.097, -0.995, -0.002]

	// Pointing forward:
	// [12316] [DBG] F: [0.001, 0.996, 0.076], R: [0.090, 0.076, -0.992], U: [-0.995, 0.008, -0.090]
	// Pointing right:
	// [12316][DBG] F: [0.992, -0.101, 0.071], R : [0.069, -0.028, -0.997], U : [0.102, 0.994, -0.021]
	// Pointing backwards:
	// [12316] [DBG] F: [-0.019, -0.992, -0.116], R: [-0.179, -0.110, 0.977], U: [-0.983, 0.039, -0.176]
	// Pointing right because the ship rolled 90 deg right:
	// [12316] [DBG] F: [0.974, 0.022, 0.221], R: [-0.002, 0.995, -0.089], U: [-0.222, 0.087, 0.970]
	// Pointing left because the ship rolled 90 deg left:
	// [12316] [DBG] F: [-0.998, 0.000, 0.043], R: [-0.004, 0.995, -0.090], U: [-0.043, -0.090, -0.995]

	// Ship facing down from starting position, so that turret looks straight ahead:
	// [16224][DBG] F: [0.000, 0.997, 0.069], R : [0.000, 0.069, -0.997], U : [-1.000, 0.000, 0.000]

	Vector4 Rs, Us, Fs;
	Matrix4 Heading = GetCurrentHeadingMatrix(Rs, Us, Fs);
	//log_debug("[DBG] [GUN] (1) R: [%0.3f, %0.3f, %0.3f], U: [%0.3f, %0.3f, %0.3f], F: [%0.3f, %0.3f, %0.3f]",
	//	R.x, R.y, R.z, U.x, U.y, U.z, F.x, F.y, F.z);

	// Transform the turret's orientation into the canonical x-y-z axes:
	R = Heading * R;
	U = Heading * U;
	F = Heading * F;
	// At this point the original [R, U, F] should always map to [0,0,1], [-1,0,0], [0,1,0] and this should only
	// change if the turret moves
	//log_debug("[DBG] [GUN] (2) R: [%0.3f, %0.3f, %0.3f], U: [%0.3f, %0.3f, %0.3f], F: [%0.3f, %0.3f, %0.3f]",
	//	R.x, R.y, R.z, U.x, U.y, U.z, F.x, F.y, F.z);

	//float tpitch = atan2(F.y, F.x); // .. range?
	//float tyaw   = acos(F.z); // 0..PI
	//log_debug("[DBG] [GUN] tpitch: %0.3f, tyaw: %0.3f", 	tpitch / DEG2RAD, tyaw / DEG2RAD);

	Matrix4 viewMat = Matrix4(
		-R.x, -U.x, F.x, 0,
		-R.y, -U.y, F.y, 0,
		-R.z, -U.z, F.z, 0,
		0, 0, 0, 1
	);
	Matrix4 rotX;
	rotX.rotateX(180.0f);
	*result = rotX * viewMat.invert();
}

/*
 * Returns the current camera or external matrix for the hyperspace effect.
 */
void PrimarySurface::GetHyperspaceEffectMatrix(Matrix4 *result) {
	Matrix4 rotMatrixFull;

	// SteamVR and DirectSBS use a new tracking system that require a different transform chain
	if (g_bEnableVR) {
		if (PlayerDataTable[*g_playerIndex].gunnerTurretActive) {
			Matrix4 viewMat;
			GetGunnerTurretViewMatrix(&viewMat);
			*result = viewMat * Matrix4().scale(1, -1, 1) * g_VSMatrixCB.fullViewMat * Matrix4().scale(1, -1, 1);
		} else {
			GetCockpitViewMatrix(result);
			*result = g_VSMatrixCB.fullViewMat * (*result);
		}
		return;
	}

	if (PlayerDataTable[*g_playerIndex].gunnerTurretActive)
	{
		GetGunnerTurretViewMatrixSpeedEffect(&rotMatrixFull);
	}
	else
	{
		float yaw, pitch;

		if (PlayerDataTable[*g_playerIndex].Camera.ExternalCamera) {
			yaw = -(float)PlayerDataTable[*g_playerIndex].Camera.Yaw / 65536.0f * 360.0f;
			pitch = (float)PlayerDataTable[*g_playerIndex].Camera.Pitch / 65536.0f * 360.0f;
		}
		else {
			yaw = -(float)PlayerDataTable[*g_playerIndex].MousePositionX / 65536.0f * 360.0f;
			pitch = (float)PlayerDataTable[*g_playerIndex].MousePositionY / 65536.0f * 360.0f;
		}

		Matrix4 rotMatrixYaw, rotMatrixPitch, rotMatrixRoll;
		rotMatrixFull.identity();
		rotMatrixYaw.identity();   rotMatrixYaw.rotateY(yaw);
		rotMatrixPitch.identity(); rotMatrixPitch.rotateX(pitch);
		rotMatrixFull = rotMatrixPitch * rotMatrixYaw;
	}

	*result = rotMatrixFull;
}

/*
 * Returns the gunner turret view matrix for the speed effect shaders
 */
void GetGunnerTurretViewMatrixSpeedEffect(Matrix4 *result) {
	// This is what the matrix looks like when looking forward:
	// F: [-0.257, 0.963, 0.080], R: [0.000, 0.083, -0.996], U: [-0.966, -0.256, -0.021]
	float factor = 32768.0f;
	/*
	short *Turret = (short *)(0x8B94E0 + 0x21E);
	Vector3 F(Turret[0] / factor, Turret[1] / factor, Turret[2] / factor);
	Vector3 R(Turret[3] / factor, Turret[4] / factor, Turret[5] / factor);
	Vector3 U(Turret[6] / factor, Turret[7] / factor, Turret[8] / factor);
	*/
	Vector3 F(
		PlayerDataTable[*g_playerIndex].gunnerTurretF[0] / factor, 
		PlayerDataTable[*g_playerIndex].gunnerTurretF[1] / factor, 
		PlayerDataTable[*g_playerIndex].gunnerTurretF[2] / factor
	);
	Vector3 R(
		PlayerDataTable[*g_playerIndex].gunnerTurretR[0] / factor,
		PlayerDataTable[*g_playerIndex].gunnerTurretR[1] / factor,
		PlayerDataTable[*g_playerIndex].gunnerTurretR[2] / factor
	);
	Vector3 U(
		PlayerDataTable[*g_playerIndex].gunnerTurretU[0] / factor,
		PlayerDataTable[*g_playerIndex].gunnerTurretU[1] / factor,
		PlayerDataTable[*g_playerIndex].gunnerTurretU[2] / factor
	);

	Vector4 Rs, Us, Fs;
	Matrix4 Heading = GetCurrentHeadingMatrix(Rs, Us, Fs);
	//log_debug("[DBG] [GUN] (1) R: [%0.3f, %0.3f, %0.3f], U: [%0.3f, %0.3f, %0.3f], F: [%0.3f, %0.3f, %0.3f]",
	//	R.x, R.y, R.z, U.x, U.y, U.z, F.x, F.y, F.z);

	// Transform the turret's orientation into the canonical x-y-z axes:
	R = Heading * R;
	U = Heading * U;
	F = Heading * F;
	// At this point the original [R, U, F] should always map to [0,0,1], [-1,0,0], [0,1,0] and this should only
	// change if the turret moves
	//log_debug("[DBG] [GUN] (2) R: [%0.3f, %0.3f, %0.3f], U: [%0.3f, %0.3f, %0.3f], F: [%0.3f, %0.3f, %0.3f]",
	//	R.x, R.y, R.z, U.x, U.y, U.z, F.x, F.y, F.z);

	Matrix4 viewMat = Matrix4(
		R.x, U.x, -F.x, 0,
		R.y, U.y, -F.y, 0,
		R.z, U.z, -F.z, 0,
		0, 0, 0, 1
	);
	Matrix4 rotX;
	rotX.rotateX(180.0f);
	*result = rotX * viewMat;
}

/*
 * Returns either the cockpit view matrix or the gunner turret view matrix.
 */
void GetCraftViewMatrix(Matrix4 *result) {
	const float DEG2RAD = 0.01745f;
	if (PlayerDataTable[*g_playerIndex].gunnerTurretActive)
		GetGunnerTurretViewMatrix(result);
	else
		GetCockpitViewMatrix(result);

	// Apply the head pose matrix to the view matrix to fix the reticle and other 2D elements positioning.
	// With the current CockpitLook headtracking mode, MousePositionX,Y = 0 so the reticle is fixed to the camera
	// It should stay aligned with the craft heading instead.
	if (g_bEnableVR) {
		if (PlayerDataTable[*g_playerIndex].gunnerTurretActive) {
			Matrix4 viewMatrix;
			GetCockpitViewMatrix(&viewMatrix);
			
			// This fixes the position of the reticle in the gunner turrets. This is why:
			// The reticle is now always fixed to the center of the screen. When we turn
			// our heads, we use g_VSMatrixCB.fullViewMat to place the reticle in the right
			// position. The same logic applies for the gunner turret.
			*result = g_VSMatrixCB.fullViewMat * viewMatrix;
		} 
		else
			*result = g_VSMatrixCB.fullViewMat * (*result);
	}
}

/*
 * Computes Heading Difference (this is similar to cockpit inertia; but we don't care about smoothing the effect)
 * Input: The current Heading matrix H, the current forward vector Fs
 * Output: The X,Y,Z displacement
 */
/*
void ComputeHeadingDifference(const Matrix4 &H, Vector4 Fs, Vector4 Us, float fCurSpeed, int playerIndex, float *XDisp, float *YDisp, float *ZDisp) {
	const float g_fCockpitInertia = 0.35f, g_fCockpitMaxInertia = 0.2f;
	static bool bFirstFrame = true;
	static float fLastSpeed = 0.0f;
	static time_t prevT = 0;
	time_t curT = time(NULL);

	// Reset the first frame if the time between successive queries is too big: this
	// implies the game was either paused or a new mission was loaded
	bFirstFrame |= curT - prevT > 2; // Reset if +2s have elapsed
	// Skip the very first frame: there's no inertia to compute yet
	if (bFirstFrame || g_bHyperspaceTunnelLastFrame || g_bHyperspaceLastFrame)
	{
		bFirstFrame = false;
		*XDisp = *YDisp = *ZDisp = 0.0f;
		//log_debug("Resetting X/Y/ZDisp");
		// Update the previous heading vectors
		g_prevFs = Fs;
		g_prevUs = Us;
		prevT = curT;
		fLastSpeed = fCurSpeed;
		return;
	}

	Matrix4 HT = H;
	HT.transpose();
	// Multiplying the current Rs, Us, Fs with H will yield the major axes:
	//Vector4 X = HT * Rs; // --> always returns [1, 0, 0]
	//Vector4 Y = HT * Us; // --> always returns [0, 1, 0]
	//Vector4 Z = HT * Fs; // --> always returns [0, 0, 1]
	//log_debug("[DBG] X: [%0.3f, %0.3f, %0.3f], Y: [%0.3f, %0.3f, %0.3f], Z: [%0.3f, %0.3f, %0.3f]",
	//	X.x, X.y, X.z, Y.x, Y.y, Y.z, Z.x, Z.y, Z.z);

	//Vector4 X = HT * g_prevRs; // --> returns something close to [1, 0, 0]
	Vector4 Y = HT * g_prevUs; // --> returns something close to [0, 1, 0]
	Vector4 Z = HT * g_prevFs; // --> returns something close to [0, 0, 1]
	//log_debug("[DBG] X: [%0.3f, %0.3f, %0.3f], Y: [%0.3f, %0.3f, %0.3f], Z: [%0.3f, %0.3f, %0.3f]",
	//	X.x, X.y, X.z, Y.x, Y.y, Y.z, Z.x, Z.y, Z.z);
	Vector4 curFs(0, 0, 1, 0);
	Vector4 curUs(0, 1, 0, 0);
	Vector4 diffZ = curFs - Z;
	Vector4 diffY = curUs - Y;

	*XDisp = g_fCockpitInertia * diffZ.x;
	*YDisp = g_fCockpitInertia * diffZ.y;
	*ZDisp = g_fCockpitInertia * diffY.x;
	if (*XDisp < -g_fCockpitMaxInertia) *XDisp = -g_fCockpitMaxInertia; else if (*XDisp > g_fCockpitMaxInertia) *XDisp = g_fCockpitMaxInertia;
	if (*YDisp < -g_fCockpitMaxInertia) *YDisp = -g_fCockpitMaxInertia; else if (*YDisp > g_fCockpitMaxInertia) *YDisp = g_fCockpitMaxInertia;
	if (*ZDisp < -g_fCockpitMaxInertia) *ZDisp = -g_fCockpitMaxInertia; else if (*ZDisp > g_fCockpitMaxInertia) *ZDisp = g_fCockpitMaxInertia;

	// Update the previous heading smoothly, otherwise the cockpit may shake a bit
	g_prevFs = 0.1f * Fs + 0.9f * g_prevFs;
	g_prevUs = 0.1f * Us + 0.9f * g_prevUs;
	fLastSpeed = 0.1f * fCurSpeed + 0.9f * fLastSpeed;
	prevT = curT;

	//if (g_HyperspacePhaseFSM == HS_HYPER_ENTER_ST || g_HyperspacePhaseFSM == HS_INIT_ST)
	//if (g_bHyperspaceLastFrame || g_bHyperspaceTunnelLastFrame)
	//	log_debug("[%d] X/YDisp: %0.3f, %0.3f",  g_iHyperspaceFrame, *XDisp, *YDisp);
}
*/

/*
 * Input: _shaderToyAuxBuf (already resolved): Should hold the background (everything minus the cockpit)
 *		  _shaderToyBuf (already resolved): Contains the foreground (only the cockpit) when exiting hyperspace.
 *		  (Probably) unused in all other circumstances.
 * Output: Renders over _offscreenBufferPost and copies to _offscreenBuffer.
 *		   Overwrites _offscreenBufferAsInputShaderResourceView
 *
 * When rendering regular 3D content, we capture the cockpit to the shadertoyBuffers using SelectOffscreenBuffer().
 * These buffers are resolved before this method is called. Here we render the hyperspace effect on a distant,
 * flat screen, and then compose the cockpit, in the form of a flat image (or 2 for the VR path) on top of it.
 * Hyperzoom is computed first when needed.
 */
void PrimarySurface::RenderHyperspaceEffect(D3D11_VIEWPORT *lastViewport,
	ID3D11PixelShader *lastPixelShader, Direct3DTexture *lastTextureSelected,
	ID3D11Buffer *lastVertexBuffer, UINT *lastVertexBufStride, UINT *lastVertexBufOffset)
{
	/*
	  Jedi Fallen Order sample hyperspace entry effect:
	  https://youtu.be/GLZoDkbTakg?t=197
	*/
	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	float x0, y0, x1, y1;
	static float iTime = 0.0f, iTimeAtHyperExit = 0.0f;
	static float fLightRotationAngle = 0.0f;
	float timeInHyperspace = (float)PlayerDataTable[*g_playerIndex].timeInHyperspace;
	float iLinearTime = 0.0f; // We need a "linear" time that we can use to control the speed of the shake and light rotation
	float fShakeAmplitude = 0.0f;
	bool bBGTextureAvailable = (g_HyperspacePhaseFSM == HS_POST_HYPER_EXIT_ST);
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	D3D11_VIEWPORT viewport{};

	// We need this to ensure backface culling is disabled
	resources->InitRasterizerState(resources->_rasterizerState);

	// Prevent rendering the hyperspace effect multiple times per frame:
	if (g_bHyperspaceEffectRenderedOnCurrentFrame)
		return;
	g_bHyperspaceEffectRenderedOnCurrentFrame = true;

	/*
		Hyperspace Data:
			2: Entering hyperspace
				max time: 489, 553
			4: Traveling through hyperspace (animation plays back at this point)
				max time: 1291, 1290
			3: Exiting hyperspace
				max time: 236, 231
	*/

	this->_deviceResources->BeginAnnotatedEvent(L"RenderHyperspaceEffect");

	// Constants for the post-hyper-exit effect:
	const float T2 = 2.0f; // Time in seconds for the trails
	const float T2_ZOOM = 1.5f; // Time in seconds for the hyperzoom
	const float T_OVERLAP = 1.5f; // Overlap between the trails and the zoom

	static float fXRotationAngle = 0.0f, fYRotationAngle = 0.0f, fZRotationAngle = 0.0f;
	Matrix4 ShakeMat;

	if (g_bHyperDebugMode && g_iHyperStyle == HYPER_INTERDICTION_STYLE)
		g_bInterdictionActive = true;

	// Adjust the time according to the current hyperspace phase
	//switch (PlayerDataTable[*g_playerIndex].hyperspacePhase) 
	switch (g_HyperspacePhaseFSM)
	{
	case HS_HYPER_ENTER_ST:
		// Max internal time: ~500
		// Max shader time: 2.0 (t2)
		g_bKeybExitHyperspace = false;
		resources->InitPixelShader(resources->_hyperEntryPS);
		timeInHyperspace = timeInHyperspace / 650.0f; // 550.0f
		iTime = lerp(0.0f, 2.0f, timeInHyperspace);
		if (iTime > 2.0f) iTime = 2.0f;
		fShakeAmplitude = lerp(0.0f, 4.0f, timeInHyperspace);
		iLinearTime = iTime;
		g_ShadertoyBuffer.bloom_strength = g_BloomConfig.fHyperStreakStrength;
		//log_debug("[DBG] [INT] Destination Region: %d, Mission: %s",
		//	PlayerDataTable[*g_playerIndex].criticalMessageObjectIndex, xwaMissionFileName);
		break;
	case HS_HYPER_TUNNEL_ST:
	{
		/*
		From Jeremy:
		To increase the time in hyperspace:
		At offset 0029D6, replace 1205 with XXXX.
		Base is 0x0400C00 (?)

		XXXX is a hex value.
		The default value is 0x0512 which correspond to 1298 (5.5s).

		To set 10s, the value will be 2360 (10 * 236). XXXX will be 3809
		*/
		// Max internal time: ~1290 (about 5.5s, it's *tunnelTime / 236.0)
		// Max shader time: 4.0 (arbitrary)
		uint16_t *tunnelTime = (uint16_t *)(0x0400C00 + 0x0029D6); // 0x0400C00 is XWA's base process address.
		//*tunnelTime = 2360;
		resources->InitPixelShader(resources->_hyperTunnelPS);
		//timeInHyperspace = timeInHyperspace / 1290.0f;
		timeInHyperspace = timeInHyperspace / (float )(*tunnelTime); // It's better to read the hyperspace time from this offset
		iTime = lerp(0.0f, 4.0f, timeInHyperspace);

		// Rotate the lights while travelling through the hyper-tunnel:
		for (int i = 0; i < 2; i++) {
			g_LightVector[i].x = (float)cos((fLightRotationAngle + (i * 90.0f)) * 0.01745f);
			g_LightVector[i].y = (float)sin((fLightRotationAngle + (i * 90.0f)) * 0.01745f);
			g_LightVector[i].z = 0.0f;

			// lerp the color of the lights from blue to red if an interdiction is happening
			if (g_bInterdictionActive) {
				const float t2 = min(max(0.0f, 2.0f * timeInHyperspace - 0.5f), 1.0f);
				g_LightColor[i].x = lerp(0.10f, 1.50f, t2);
				g_LightColor[i].y = lerp(0.15f, 0.15f, t2);
				g_LightColor[i].z = lerp(1.50f, 0.10f, t2);
			}
		}
		fShakeAmplitude = lerp(4.0f, 7.0f, timeInHyperspace);
		iLinearTime = 2.0f + iTime;
		g_ShadertoyBuffer.bloom_strength = g_BloomConfig.fHyperTunnelStrength;
		// Re-set the twirl for the tunnel. This field is used in other places with different meanings
		g_ShadertoyBuffer.twirl = 1.0f;

		if (g_config.StayInHyperspace) {
			if (!g_bKeybExitHyperspace) {
				if (PlayerDataTable[*g_playerIndex].timeInHyperspace > 900 && PlayerDataTable[*g_playerIndex].timeInHyperspace < 1000)
					PlayerDataTable[*g_playerIndex].timeInHyperspace -= 500;
			}
			else {
				PlayerDataTable[*g_playerIndex].timeInHyperspace = 1050;
				g_bKeybExitHyperspace = false;
			}
		}
		break;
	}
	case HS_HYPER_EXIT_ST:
		g_bKeybExitHyperspace = false;
		// Time is reversed here and in post-hyper-exit because this effect was originally
		// the hyper-entry in reverse; but then I changed it and I didn't "un-revert" the
		// time axis. Oh well, yet another TODO for me...
		// Max internal time: ~236
		// Max shader time: 1.5 (t2 minus a small fraction)
		resources->InitPixelShader(resources->_hyperExitPS);
		timeInHyperspace = timeInHyperspace / 200.0f;
		iTime = lerp(T2 + T2_ZOOM - T_OVERLAP, T2_ZOOM, timeInHyperspace);
		//iTime = lerp(0.0f, T2 - T_OVERLAP, timeInHyperspace);
		iTimeAtHyperExit = iTime;
		iLinearTime = 6.0f + ((T2 + T2_ZOOM - T_OVERLAP) - iTime);
		fShakeAmplitude = lerp(7.0f, 0.0f, timeInHyperspace);
		g_ShadertoyBuffer.bloom_strength = g_BloomConfig.fHyperStreakStrength;
		break;
	case HS_POST_HYPER_EXIT_ST:
		// Max internal time: MAX_POST_HYPER_EXIT_FRAMES
		// Max shader time: T2_ZOOM
		resources->InitPixelShader(resources->_hyperZoomPS); // hyperExitPS is activated after the HyperZoom shader
		timeInHyperspace = (float)g_iHyperExitPostFrames / MAX_POST_HYPER_EXIT_FRAMES;
		if (timeInHyperspace > 1.0f) timeInHyperspace = 1.0f;
		iTime = lerp(iTimeAtHyperExit, 0.0f, timeInHyperspace);
		iLinearTime = ((T2 + T2_ZOOM - T_OVERLAP) - iTimeAtHyperExit) + timeInHyperspace;
		//iTime = lerp(iTimeAtHyperExit, T2 + T2_ZOOM - T_OVERLAP, timeInHyperspace);
		g_ShadertoyBuffer.bloom_strength = g_BloomConfig.fHyperStreakStrength;
		break;
	}

	//#ifdef HYPER_OVERRIDE
		
	if (g_bHyperDebugMode) {
		//iTime = g_fHyperTimeOverride;

		static float TimeOverride = 0.0f;
		TimeOverride += 0.01f;
		if (TimeOverride > 2.0f) TimeOverride = 0.0f;
		iTime = TimeOverride;
	}
	//#endif

	// Shake the cockpit if an interdiction is happening
	if (g_bInterdictionActive && g_HyperspacePhaseFSM == HS_HYPER_TUNNEL_ST) {
		Matrix4 RX, RY;
		float fTotalInterdictionShake = g_fInterdictionShake;
		if (g_bEnableVR || g_bSteamVREnabled)
			fTotalInterdictionShake *= g_fInterdictionShakeInVR;
		const float t2 = max(0.0f, (iTime * 1.25f) - 0.9f);
		float shake_amp = 3.0f * t2 * fTotalInterdictionShake;
		float ax = -1.2f * shake_amp * cos(g_fInterdictionAngleScale * 20.0f * t2);
		float ay =  2.3f * shake_amp * sin(g_fInterdictionAngleScale * 20.0f * t2);

		shake_amp = 0.5f * t2 * fTotalInterdictionShake;
		ax += shake_amp * cos(g_fInterdictionAngleScale * 100.0f * t2);
		ay += shake_amp * sin(g_fInterdictionAngleScale * 100.0f * t2);

		RX.rotateX(ax);
		RY.rotateY(ay);
		ShakeMat = RX * RY;
	}

	fLightRotationAngle = 25.0f * iLinearTime * g_fHyperLightRotationSpeed;
	// TODO: Where am I setting the lights for the new shading model?
	//		 Am I setting the ssMask in the RTVs like I do during Execute()? Do I even need to do that?
	
	/*
	// TODO: The extra geometry shader shakes by itself when this block is enabled. Cockpit Inertia kills this
	//       effect, so we need to read that flag from CockpitLook.cfg... or we can just remove this whole block.

	fXRotationAngle = 25.0f * iLinearTime * g_fHyperShakeRotationSpeed;
	fYRotationAngle = 30.0f * iLinearTime * g_fHyperShakeRotationSpeed;
	fZRotationAngle = 35.0f * iLinearTime * g_fHyperShakeRotationSpeed;

	// Shake the cockpit a little bit:
	float fShakeX = cos(fXRotationAngle * 0.01745f);
	float fShakeZ = sin(fZRotationAngle * 0.01745f);
	float fShakeY = cos(fYRotationAngle * 0.01745f);;
	int iShakeX = (int)(fShakeAmplitude * fShakeX);
	int iShakeY = (int)(fShakeAmplitude * fShakeY);
	int iShakeZ = (int)(fShakeAmplitude * fShakeZ);

	if (*xwa::numberOfPlayersInGame == 1 && !g_bCockpitInertiaEnabled) 
	{
		PlayerDataTable[*g_playerIndex].Camera.ShakeX = iShakeX;
		PlayerDataTable[*g_playerIndex].Camera.ShakeY = iShakeY;
		PlayerDataTable[*g_playerIndex].Camera.ShakeZ = iShakeZ;
	}
	*/

	// DEBUG Test the hyperzoom
	/*
	if (g_HyperspacePhaseFSM == HS_HYPER_EXIT_ST || g_HyperspacePhaseFSM == HS_POST_HYPER_EXIT_ST) {
		float fGetTime = iTime - T2_ZOOM + T_OVERLAP;
		log_debug("[DBG] FSM %d, iTime: %0.3f, getTime: %0.3f, bUseHyperZoom: %d, g_iHyperExitPostFrames: %d",
			g_HyperspacePhaseFSM, iTime, fGetTime, bBGTextureAvailable, g_iHyperExitPostFrames);
	}
	*/
	// DEBUG
	bool bDirectSBS = (g_bEnableVR && !g_bUseSteamVR);
	GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
	if (g_bEnableVR)
		GetHyperspaceEffectMatrix(&g_ShadertoyBuffer.viewMat); // New version for SteamVR
	else
		GetCraftViewMatrix(&g_ShadertoyBuffer.viewMat); // Original version

	// Apply the interdiction shake computed before
	if (g_bInterdictionActive && g_HyperspacePhaseFSM == HS_HYPER_TUNNEL_ST)
	{
		g_ShadertoyBuffer.viewMat = ShakeMat * g_ShadertoyBuffer.viewMat;
	}

	g_ShadertoyBuffer.x0 = x0;
	g_ShadertoyBuffer.y0 = y0;
	g_ShadertoyBuffer.x1 = x1;
	g_ShadertoyBuffer.y1 = y1;
	g_ShadertoyBuffer.tunnel_speed = g_fHyperspaceTunnelSpeed;
	g_ShadertoyBuffer.srand = g_fHyperspaceRand;
	g_ShadertoyBuffer.iTime = iTime;
	g_ShadertoyBuffer.VRmode = bDirectSBS;
	g_ShadertoyBuffer.iResolution[0] = g_fCurScreenWidth;
	g_ShadertoyBuffer.iResolution[1] = g_fCurScreenHeight;
	g_ShadertoyBuffer.hyperspace_phase = g_HyperspacePhaseFSM;
	/*if (g_bEnableVR) {
		if (g_HyperspacePhaseFSM == HS_HYPER_TUNNEL_ST)
			g_ShadertoyBuffer.iResolution[1] *= g_fAspectRatio;
	}*/
	resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);

	//const int CAPTURE_FRAME = 75; // Hyper-entry
	//const int CAPTURE_FRAME = 150; // Tunnel

	// Pre-render: Apply the hyperzoom if necessary
	// input: shadertoyAuxBufSRV
	// output: Renders to renderTargetViewPost, then resolves the output to shadertoyAuxBuf
	if (bBGTextureAvailable)
	{
		context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		viewport.Width  = g_fCurScreenWidth;
		viewport.Height = g_fCurScreenHeight;
		// VIEWPORT-LEFT
		if (g_bUseSteamVR) {
			viewport.Width  = (float)resources->_backbufferWidth;
			viewport.Height = (float)resources->_backbufferHeight;
		}
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		resources->InitViewport(&viewport);

		// We don't need to clear the current vertex and pixel constant buffers.
		// Since we've just finished rendering 3D, they should contain values that
		// can be reused. So let's just overwrite the values that we need.
		g_VSCBuffer.aspect_ratio      =  g_fAspectRatio;
		g_VSCBuffer.z_override        = -1.0f;
		g_VSCBuffer.sz_override       = -1.0f;
		g_VSCBuffer.mult_z_override   = -1.0f;
		g_VSCBuffer.apply_uv_comp     =  false;
		g_VSCBuffer.bPreventTransform =  0.0f;
		g_VSCBuffer.bFullTransform    =  0.0f;
		if (g_bUseSteamVR)
		{
			g_VSCBuffer.viewportScale[0] = 1.0f / resources->_displayWidth;
			g_VSCBuffer.viewportScale[1] = 1.0f / resources->_displayHeight;
		}
		else
		{
			// non-VR and DirectSBS cases:
			g_VSCBuffer.viewportScale[0] =  2.0f / resources->_displayWidth;
			g_VSCBuffer.viewportScale[1] = -2.0f / resources->_displayHeight;
		}

		// Since the HUD is all rendered on a flat surface, we lose the vrparams that make the 3D object
		// and text float
		g_VSCBuffer.z_override  = 65535.0f;
		g_VSCBuffer.scale_override = 1.0f;

		// Set the left projection matrix (the viewMatrix is set at the beginning of the frame)
		g_VSMatrixCB.projEye[0] = g_FullProjMatrixLeft;
		g_VSMatrixCB.projEye[1] = g_FullProjMatrixRight;

		resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
		resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

		UINT stride = sizeof(D3DTLVERTEX), offset = 0;
		resources->InitVertexBuffer(resources->_hyperspaceVertexBuffer.GetAddressOf(), &stride, &offset);
		resources->InitInputLayout(resources->_inputLayout);
		if (g_bUseSteamVR)
			resources->InitVertexShader(resources->_sbsVertexShader); // if (g_bEnableVR)
		else
			// The original (non-VR) code used _vertexShader:
			resources->InitVertexShader(resources->_vertexShader);

		resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Set the RTV:
		context->OMSetRenderTargets(1, resources->_renderTargetViewPost.GetAddressOf(), NULL);
		// Set the SRV:
		context->PSSetShaderResources(0, 1, resources->_shadertoyAuxSRV.GetAddressOf());
		if (g_bUseSteamVR)
			context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(6, 0);
		context->ResolveSubresource(resources->_shadertoyAuxBuf, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);
		if (g_bUseSteamVR)
			context->ResolveSubresource(
				resources->_shadertoyAuxBuf, D3D11CalcSubresource(0, 1, 1),
				resources->_offscreenBufferPost, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);

		// Activate the hyperExitPS
		resources->InitPixelShader(resources->_hyperExitPS);
	}
	// End of hyperzoom

	// First render: Render the hyperspace effect itself
	// input: None
	// output: renderTargetViewPost
	{
		// Set the new viewport (a full quad covering the full screen)
		viewport.Width  = g_fCurScreenWidth;
		viewport.Height = g_fCurScreenHeight;
		// VIEWPORT-LEFT
		if (g_bEnableVR) {
			if (g_bUseSteamVR)
				viewport.Width = (float)resources->_backbufferWidth;
			else
				viewport.Width = (float)resources->_backbufferWidth / 2.0f;
		}
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		resources->InitViewport(&viewport);

		// We don't need to clear the current vertex and pixel constant buffers.
		// Since we've just finished rendering 3D, they should contain values that
		// can be reused. So let's just overwrite the values that we need.

		g_VSCBuffer.aspect_ratio      =  g_fAspectRatio;
		g_VSCBuffer.z_override        = -1.0f;
		g_VSCBuffer.sz_override       = -1.0f;
		g_VSCBuffer.mult_z_override   = -1.0f;
		g_VSCBuffer.apply_uv_comp     =  false;
		g_VSCBuffer.bPreventTransform =  0.0f;
		g_VSCBuffer.bFullTransform    =  0.0f;
		g_VSCBuffer.s_V0x08B94CC      = *(float*)0x08B94CC;
		g_VSCBuffer.s_V0x05B46B4      = *(float*)0x05B46B4;
		if (g_bEnableVR) 
		{
			g_VSCBuffer.viewportScale[0] = 1.0f / resources->_displayWidth;
			g_VSCBuffer.viewportScale[1] = 1.0f / resources->_displayHeight;
		}
		else
		{
			g_VSCBuffer.viewportScale[0] =  2.0f / resources->_displayWidth;
			g_VSCBuffer.viewportScale[1] = -2.0f / resources->_displayHeight;
		}
		//g_VSCBuffer.viewportScale[3] = 1.0f;
		//g_VSCBuffer.viewportScale[3] = g_fGlobalScale;

		// Since the HUD is all rendered on a flat surface, we lose the vrparams that make the 3D object
		// and text float
		g_VSCBuffer.z_override		= 65535.0f;
		g_VSCBuffer.scale_override	= 1.0f;

		// Set the left projection matrix (the viewMatrix is set at the beginning of the frame)
		g_VSMatrixCB.projEye[0] = g_FullProjMatrixLeft;
		g_VSMatrixCB.projEye[1] = g_FullProjMatrixRight;
		resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
		//resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
		resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

		UINT stride = sizeof(D3DTLVERTEX), offset = 0;
		resources->InitVertexBuffer(resources->_hyperspaceVertexBuffer.GetAddressOf(), &stride, &offset);
		resources->InitInputLayout(resources->_inputLayout);
		if (g_bEnableVR)
			resources->InitVertexShader(resources->_sbsVertexShader); // if (g_bEnableVR)
		else
			// The original (non-VR) code used _vertexShader:
			resources->InitVertexShader(resources->_vertexShader);
		resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		// Set the RTV:
		ID3D11RenderTargetView *rtvs[1] = {
			resources->_renderTargetViewPost.Get(), // Render to offscreenBufferPost instead of offscreenBuffer
		};

		context->OMSetRenderTargets(1, rtvs, NULL);
		if (g_bUseSteamVR)
			context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(6, 0);

		// Render the right image
		if (g_bEnableVR && !g_bUseSteamVR) {
			// VIEWPORT-RIGHT
			viewport.Width = (float)resources->_backbufferWidth / 2.0f;
			viewport.TopLeftX = (float)viewport.Width;
			viewport.Height = (float)resources->_backbufferHeight;
			viewport.TopLeftY = 0.0f;
			viewport.MinDepth = D3D11_MIN_DEPTH;
			viewport.MaxDepth = D3D11_MAX_DEPTH;
			resources->InitViewport(&viewport);
			// Set the right projection matrix
			g_VSMatrixCB.projEye[0] = g_FullProjMatrixRight;
			resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

			if (g_bUseSteamVR)
				context->OMSetRenderTargets(1, resources->_renderTargetViewPostR.GetAddressOf(), NULL);
			else
				context->OMSetRenderTargets(1, resources->_renderTargetViewPost.GetAddressOf(), NULL);
			context->Draw(6, 0);
		}
	}
	
	// DEBUG
	/*if (g_iPresentCounter == CAPTURE_FRAME)
	{
		DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferPost, GUID_ContainerFormatJpeg,
			L"C:\\Temp\\_offscreenBufferPost-0.jpg");
	}*/
	// DEBUG

	// We can't capture the background here (yet). The hyper effects needs to be combined with the
	// hyperzoom effect and that happens when we call hyperComposePS

	// Second render: compose the cockpit over the previous effect
	{
		// Reset the viewport for non-VR mode:
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width	  = g_fCurScreenWidth;
		viewport.Height	  = g_fCurScreenHeight;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		resources->InitViewport(&viewport);

		// Reset the vertex shader to regular 2D post-process
		// Set the Vertex Shader Constant buffers
		resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
			0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices

		// Set/Create the VertexBuffer and set the topology, etc
		UINT stride = sizeof(MainVertex), offset = 0;
		resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);
		resources->InitInputLayout(resources->_mainInputLayout);
		resources->InitVertexShader(g_bUseSteamVR ? resources->_mainVertexShaderVR : resources->_mainVertexShader);

		// Reset the UV limits for this shader
		GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
		g_ShadertoyBuffer.x0 = x0;
		g_ShadertoyBuffer.y0 = y0;
		g_ShadertoyBuffer.x1 = x1;
		g_ShadertoyBuffer.y1 = y1;
		g_ShadertoyBuffer.iResolution[0] = g_fCurScreenWidth;
		g_ShadertoyBuffer.iResolution[1] = g_fCurScreenHeight;
		// Tint the bloom red, but only if an interdiction is happening and only
		// after the second half of the hyper tunnel (we don't want to tint the
		// entry into the hypertunnel red).
		if (g_bInterdictionActive && g_HyperspacePhaseFSM > HS_HYPER_ENTER_ST)
			g_ShadertoyBuffer.twirl = min(1.0f, 2.5f * timeInHyperspace);
		else
			g_ShadertoyBuffer.twirl = 0.0f;
		resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);

		resources->InitPixelShader(g_bUseSteamVR ? resources->_hyperComposePS_VR : resources->_hyperComposePS);
		// Clear all the render target views
		ID3D11RenderTargetView *rtvs_null[6] = {
			NULL, // Main RTV
			NULL, // Bloom
			NULL, // Depth
			NULL, // Norm Buf
			NULL, // SSAO Mask
			NULL, // SS Mask
		};
		context->OMSetRenderTargets(6, rtvs_null, NULL);

		// DEBUG
		/*
		if (g_iPresentCounter == CAPTURE_FRAME)
		{
			DirectX::SaveWICTextureToFile(context, resources->_shadertoyAuxBuf, GUID_ContainerFormatJpeg,
				L"C:\\Temp\\_shadertoyAuxBuf.jpg");
			DirectX::SaveWICTextureToFile(context, resources->_shadertoyBuf, GUID_ContainerFormatJpeg,
				L"C:\\Temp\\_shadertoyBuf.jpg");
			DirectX::SaveWICTextureToFile(context, resources->_offscreenBuffer, GUID_ContainerFormatJpeg,
				L"C:\\Temp\\_offscreenBuf-1.jpg");
			DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferPost, GUID_ContainerFormatJpeg,
				L"C:\\Temp\\_offscreenBufferPost-1.jpg");
		}
		*/
		// DEBUG

		// The output from the previous effect will be in offscreenBufferPost, so let's resolve it
		// to _offscreenBufferAsInput to re-use in the next step:
		context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);
		context->ResolveSubresource(resources->_shadertoyBuf, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
		if (g_bUseSteamVR)
		{
			context->ResolveSubresource(
				resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
				resources->_offscreenBufferPost, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
			context->ResolveSubresource(
				resources->_shadertoyBuf, D3D11CalcSubresource(0, 1, 1),
				resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
		}

		context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		if (!g_bReshadeEnabled) {
			ID3D11RenderTargetView *rtvs[1] = {
				resources->_renderTargetViewPost.Get(),
			};
			context->OMSetRenderTargets(1, rtvs, NULL);
		}
		else {
			ID3D11RenderTargetView *rtvs[6] = {
				resources->_renderTargetViewPost.Get(), // Render to offscreenBufferPost instead of offscreenBuffer
				resources->_renderTargetViewBloomMask.Get(),
				NULL, // Depth
				NULL, // Norm Buf
				NULL, // SSAO Mask
				NULL, // SS Mask //resources->_renderTargetViewSSMask.Get(),
			};
			context->OMSetRenderTargets(6, rtvs, NULL);
		}
		if (g_bDumpSSAOBuffers) {
			// This is the foreground of the hyperspace effect (the cockpit). We can dump this texture to check
			// that the transparency is OK.
			DirectX::SaveDDSTextureToFile(context, resources->_shadertoyBuf, L"c:\\temp\\_hyperFG.dds");
			DirectX::SaveDDSTextureToFile(context, resources->_shadertoyAuxBufR, L"c:\\temp\\_hyperBG.dds");
			DirectX::SaveDDSTextureToFile(context, resources->_offscreenBufferAsInput, L"c:\\temp\\_hyperEffect.dds");
		}
		// Set the SRVs:
		ID3D11ShaderResourceView *srvs[3] = {
			resources->_shadertoySRV.Get(),    // Foreground (cockpit)
			resources->_shadertoyAuxSRV.Get(), // Background (stellar background, hyperzoomed)
			resources->_offscreenAsInputShaderResourceView.Get(), // Previous hyper effect (trails or tunnel)
		};
		context->PSSetShaderResources(0, 3, srvs);
		if (g_bUseSteamVR)
			context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(6, 0);

		// Capture the background buffer: it should contain the hyper effect + hyper zoom, and we'll
		// need it for the final DeferredPass() below
		context->CopyResource(resources->_backgroundBuffer, resources->_offscreenBufferPost);
		g_bBackgroundCaptured = true;
		if (g_bDumpSSAOBuffers)
			DirectX::SaveDDSTextureToFile(context, resources->_backgroundBuffer, L"c:\\temp\\_backgroundBuffer.dds");
	}

	// Restore the original state: VertexBuffer, Shaders, Topology, Z-Buffer state, etc...
	resources->InitViewport(lastViewport);
	// TODO: None of these functions will actually *apply* any changes if they don't internally see
	//       any difference. The fix is to use a proper InitXXX() above to update the internal state
	//	     of these functions.
	if (lastTextureSelected != NULL) {
		lastTextureSelected->_refCount++;
		context->PSSetShaderResources(0, 1, lastTextureSelected->_textureView.GetAddressOf());
		lastTextureSelected->_refCount--;
	}
	resources->InitInputLayout(resources->_inputLayout);
	if (g_bEnableVR)
		this->_deviceResources->InitVertexShader(resources->_sbsVertexShader); // if (g_bEnableVR)
	else
		this->_deviceResources->InitVertexShader(resources->_vertexShader);
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitPixelShader(lastPixelShader);
	resources->InitRasterizerState(resources->_rasterizerState);
	if (lastVertexBuffer != NULL)
		resources->InitVertexBuffer(&lastVertexBuffer, lastVertexBufStride, lastVertexBufOffset);
	resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
	resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);

	this->_deviceResources->EndAnnotatedEvent();
}

void PrimarySurface::RenderFXAA()
{
	this->_deviceResources->BeginAnnotatedEvent(L"RenderFXAA");

	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	float x0, y0, x1, y1;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	D3D11_VIEWPORT viewport{};

	// We only need the iResolution field from the shadertoy CB in the FXAA shader
	// ... and maybe the UV limits; but that's it. I think we can safely re-use the CB
	GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
	g_ShadertoyBuffer.x0 = x0;
	g_ShadertoyBuffer.y0 = y0;
	g_ShadertoyBuffer.x1 = x1;
	g_ShadertoyBuffer.y1 = y1;
	g_ShadertoyBuffer.iResolution[0] = g_fCurScreenWidth;
	g_ShadertoyBuffer.iResolution[1] = g_fCurScreenHeight;
	resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);

	// Reset the viewport for non-VR mode:
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width    = g_fCurScreenWidth;
	viewport.Height   = g_fCurScreenHeight;
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	resources->InitViewport(&viewport);
	SetScissoRectFullScreen();

	// Reset the vertex shader to regular 2D post-process
	// Set the Vertex Shader Constant buffers
	resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
		0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices

	// Set/Create the VertexBuffer and set the topology, etc
	UINT stride = sizeof(MainVertex), offset = 0;
	resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitInputLayout(resources->_mainInputLayout);

	resources->InitVertexShader(g_bUseSteamVR ? resources->_mainVertexShaderVR : resources->_mainVertexShader);
	resources->InitPixelShader(g_bUseSteamVR ? resources->_fxaaPS_VR : resources->_fxaaPS);
	// Clear all the render target views
	ID3D11RenderTargetView *rtvs_null[5] = {
		NULL, // Main RTV
		NULL, // Bloom
		NULL, // Depth
		NULL, // Norm Buf
		NULL, // SSAO Mask
	};
	context->OMSetRenderTargets(5, rtvs_null, NULL);

	// Do we need to resolve the offscreen buffer?
	context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
	if (g_bUseSteamVR)
		context->ResolveSubresource(
			resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
			resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
	context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);

	ID3D11RenderTargetView *rtvs[1] = {
		resources->_renderTargetViewPost.Get(),
	};
	context->OMSetRenderTargets(1, rtvs, NULL);
	// Set the SRVs:
	ID3D11ShaderResourceView *srvs[1] = {
		resources->_offscreenAsInputShaderResourceView.Get(),
	};
	context->PSSetShaderResources(0, 1, srvs);
	if (g_bUseSteamVR)
		context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
	else
		context->Draw(6, 0);

	// Copy the result (_offscreenBufferPost) to the _offscreenBuffer so that it gets displayed
	context->CopyResource(resources->_offscreenBuffer, resources->_offscreenBufferPost);

	// Restore previous rendertarget, etc
	resources->InitInputLayout(resources->_inputLayout); // Not sure this is really needed

	this->_deviceResources->EndAnnotatedEvent();
}

void PrimarySurface::RenderLevels()
{
	this->_deviceResources->BeginAnnotatedEvent(L"RenderLevels");
	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	D3D11_VIEWPORT viewport{};

	// Save the current context
	SaveContext();

	// Apply the constants for this effect
	g_ShadertoyBuffer.twirl = g_fLevelsWhitePoint;
	g_ShadertoyBuffer.bloom_strength = g_fLevelsBlackPoint;
	resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);

	// Reset the viewport for non-VR mode:
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = g_fCurScreenWidth;
	viewport.Height = g_fCurScreenHeight;
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	resources->InitViewport(&viewport);
	SetScissoRectFullScreen();

	// Reset the vertex shader to regular 2D post-process
	// Set the Vertex Shader Constant buffers
	resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
		0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices

	// Set/Create the VertexBuffer and set the topology, etc
	UINT stride = sizeof(MainVertex), offset = 0;
	resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitInputLayout(resources->_mainInputLayout);

	resources->InitVertexShader(g_bUseSteamVR ? resources->_mainVertexShaderVR : resources->_mainVertexShader);
	resources->InitPixelShader(resources->_levelsPS);
	// Clear all the render target views
	ID3D11RenderTargetView *rtvs_null[5] = {
		NULL, // Main RTV
		NULL, // Bloom
		NULL, // Depth
		NULL, // Norm Buf
		NULL, // SSAO Mask
	};
	context->OMSetRenderTargets(5, rtvs_null, NULL);

	// Do we need to resolve the offscreen buffer?
	context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
	if (g_bUseSteamVR)
		context->ResolveSubresource(
			resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
			resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
	context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);

	ID3D11RenderTargetView *rtvs[1] = {
		resources->_renderTargetViewPost.Get(),
	};
	context->OMSetRenderTargets(1, rtvs, NULL);
	// Set the SRVs:
	ID3D11ShaderResourceView *srvs[1] = {
		resources->_offscreenAsInputShaderResourceView.Get(),
	};
	context->PSSetShaderResources(0, 1, srvs);
	if (g_bUseSteamVR)
		context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
	else
		context->Draw(6, 0);

	// Copy the result (_offscreenBufferPost) to the _offscreenBuffer so that it gets displayed
	context->CopyResource(resources->_offscreenBuffer, resources->_offscreenBufferPost);

	// Restore the previous context
	RestoreContext();
	this->_deviceResources->EndAnnotatedEvent();
}

void PrimarySurface::RenderStarDebug()
{
	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	bool bDirectSBS = (g_bEnableVR && !g_bUseSteamVR);
	float x0, y0, x1, y1;
	D3D11_VIEWPORT viewport;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const bool bExternalView = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;

	GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
	GetCraftViewMatrix(&g_ShadertoyBuffer.viewMat);
	g_ShadertoyBuffer.x0 = x0;
	g_ShadertoyBuffer.y0 = y0;
	g_ShadertoyBuffer.x1 = x1;
	g_ShadertoyBuffer.y1 = y1;
	//g_ShadertoyBuffer.iTime = 0;
	//g_ShadertoyBuffer.VRmode = bDirectSBS;
	g_ShadertoyBuffer.VRmode = g_bEnableVR ? (bDirectSBS ? 1 : 2) : 0; // 0 = non-VR, 1 = SBS, 2 = SteamVR
	g_ShadertoyBuffer.iResolution[0] = g_fCurScreenWidth;
	g_ShadertoyBuffer.iResolution[1] = g_fCurScreenHeight;
	g_ShadertoyBuffer.y_center = bExternalView ? 0.0f : g_fYCenter;
	g_ShadertoyBuffer.FOVscale = g_fFOVscale;

	// DEBUG: Display the light at index i:
	int i = 1;
	Vector4 xwaLight = Vector4(
		s_XwaGlobalLights[i].PositionX / 32768.0f,
		s_XwaGlobalLights[i].PositionY / 32768.0f,
		s_XwaGlobalLights[i].PositionZ / 32768.0f,
		0.0f);
	// Convert the XWA light direction into viewspace coordinates:
	Vector4 light = g_CurrentHeadingViewMatrix * xwaLight;

	Vector3 L(light.x, light.y, light.z);
	L *= 65536.0f;
	Vector3 p = projectMetric(L, g_viewMatrix, g_FullProjMatrixLeft, false);
	//log_debug("[DBG] p: %0.3f, %0.3f", p.x, p.y);
	g_ShadingSys_PSBuffer.MainLight.x = p.x;
	g_ShadingSys_PSBuffer.MainLight.y = g_bEnableVR ? -p.y : p.y;
	g_ShadingSys_PSBuffer.MainLight.z = light.z;
	resources->InitPSConstantShadingSystem(resources->_shadingSysBuffer.GetAddressOf(), &g_ShadingSys_PSBuffer);

	resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
	resources->InitPixelShader(resources->_starDebugPS);
	// We need this to ensure backface culling is disabled
	resources->InitRasterizerState(resources->_rasterizerState);

	context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
	if (g_bUseSteamVR)
		context->ResolveSubresource(
			resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1), 
			resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);

	// Render the star centroid
	{
		// Set the new viewport (a full quad covering the full screen)
		viewport.Width = g_fCurScreenWidth;
		viewport.Height = g_fCurScreenHeight;
		// VIEWPORT-LEFT
		if (g_bEnableVR) {
			if (g_bUseSteamVR)
				viewport.Width = (float)resources->_backbufferWidth;
			else
				viewport.Width = (float)resources->_backbufferWidth / 2.0f;
		}
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		resources->InitViewport(&viewport);

		// We don't need to clear the current vertex and pixel constant buffers.
		// Since we've just finished rendering 3D, they should contain values that
		// can be reused. So let's just overwrite the values that we need.
		g_VSCBuffer.aspect_ratio = g_fAspectRatio;
		g_VSCBuffer.z_override = -1.0f;
		g_VSCBuffer.sz_override = -1.0f;
		g_VSCBuffer.mult_z_override = -1.0f;
		g_VSCBuffer.apply_uv_comp = false;
		g_VSCBuffer.bPreventTransform = 0.0f;
		g_VSCBuffer.bFullTransform = 0.0f;
		if (g_bEnableVR)
		{
			g_VSCBuffer.viewportScale[0] = 1.0f / resources->_displayWidth;
			g_VSCBuffer.viewportScale[1] = 1.0f / resources->_displayHeight;
		}
		else
		{
			g_VSCBuffer.viewportScale[0] = 2.0f / resources->_displayWidth;
			g_VSCBuffer.viewportScale[1] = -2.0f / resources->_displayHeight;
		}

		// These are the same settings we used previously when rendering the HUD during a draw() call.
		// We use these settings to place the HUD at different depths
		g_VSCBuffer.z_override = g_fHUDDepth;
		// The Aiming HUD is now visible in external view using the exterior hook, let's put it at
		// infinity
		if (bExternalView)
			g_VSCBuffer.z_override = 65536.0f;
		if (g_bFloatingAimingHUD)
			g_VSCBuffer.bPreventTransform = 1.0f;

		// Set the left projection matrix (the viewMatrix is set at the beginning of the frame)
		g_VSMatrixCB.projEye[0] = g_FullProjMatrixLeft;
		g_VSMatrixCB.projEye[1] = g_FullProjMatrixRight;
		resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
		resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

		//resources->FillReticleVertexBuffer(g_fCurInGameWidth, g_fCurInGameHeight);
		UINT stride = sizeof(D3DTLVERTEX), offset = 0;
		resources->InitVertexBuffer(resources->_hyperspaceVertexBuffer.GetAddressOf(), &stride, &offset);
		resources->InitInputLayout(resources->_inputLayout);
		if (g_bEnableVR)
			resources->InitVertexShader(resources->_sbsVertexShader); // if (g_bEnableVR)
		else
			resources->InitVertexShader(resources->_vertexShader);

		resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		// Set the RTV:
		ID3D11RenderTargetView *rtvs[1] = {
			resources->_renderTargetViewPost.Get(), // Render to offscreenBufferPost instead of offscreenBuffer
		};
		context->OMSetRenderTargets(1, rtvs, NULL);
		// Set the SRVs:
		ID3D11ShaderResourceView *srvs[2] = {
			resources->_offscreenAsInputShaderResourceView.Get(),
			resources->_ReticleSRV.Get(),
		};
		context->PSSetShaderResources(0, 2, srvs);
		if (g_bUseSteamVR)
			context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(6, 0);

		if (!g_bEnableVR)
			goto out;

		// Second render: If VR mode is enabled, then compose the previous render with the offscreen buffer
		{
			// Reset the viewport for non-VR mode, post-proc viewport (cover the whole screen)
			viewport.TopLeftX = 0.0f;
			viewport.TopLeftY = 0.0f;
			viewport.Width = g_fCurScreenWidth;
			viewport.Height = g_fCurScreenHeight;
			viewport.MaxDepth = D3D11_MAX_DEPTH;
			viewport.MinDepth = D3D11_MIN_DEPTH;
			resources->InitViewport(&viewport);

			// Reset the vertex shader to regular 2D post-process
			// Set the Vertex Shader Constant buffers
			resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
				0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices

			// Set/Create the VertexBuffer and set the topology, etc
			UINT stride = sizeof(MainVertex), offset = 0;
			resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);
			resources->InitInputLayout(resources->_mainInputLayout);
			resources->InitVertexShader(g_bUseSteamVR ? resources->_mainVertexShaderVR : resources->_mainVertexShader);

			// Reset the UV limits for this shader
			GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
			g_ShadertoyBuffer.x0 = x0;
			g_ShadertoyBuffer.y0 = y0;
			g_ShadertoyBuffer.x1 = x1;
			g_ShadertoyBuffer.y1 = y1;
			g_ShadertoyBuffer.iResolution[0] = g_fCurScreenWidth;
			g_ShadertoyBuffer.iResolution[1] = g_fCurScreenHeight;
			resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
			resources->InitPixelShader(resources->_addGeomComposePS);

			// The output from the previous effect will be in offscreenBufferPost, so let's resolve it
			// to _shadertoyBuf to use it now:
			context->ResolveSubresource(resources->_shadertoyBuf, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);
			if (g_bUseSteamVR)
				context->ResolveSubresource(
					resources->_shadertoyBuf, D3D11CalcSubresource(0, 1, 1),
					resources->_offscreenBufferPost, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);

			context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
			ID3D11RenderTargetView *rtvs[1] = {
				resources->_renderTargetViewPost.Get(), // Render to offscreenBufferPost instead of offscreenBuffer
			};
			context->OMSetRenderTargets(1, rtvs, NULL);
			// Set the SRVs:
			ID3D11ShaderResourceView *srvs[2] = {
				resources->_offscreenAsInputShaderResourceView.Get(), // The current render
				resources->_shadertoySRV.Get(),	 // The effect rendered in the previous pass
			};
			context->PSSetShaderResources(0, 2, srvs);
			if (g_bUseSteamVR)
				context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
			else
				context->Draw(6, 0);
		}
	}

out:
	// Copy the result (_offscreenBufferPost) to the _offscreenBuffer so that it gets displayed
	context->CopyResource(resources->_offscreenBuffer, resources->_offscreenBufferPost);

	// Restore previous rendertarget, etc
	resources->InitInputLayout(resources->_inputLayout); // Not sure this is really needed
}

/*
 * The large FOV needed for VR causes the reticle to become huge as well. This shader is being reused
 * to resize the reticle to a more reasonable size in VR. It may also help in reducing the distortion
 * that happens when looking near the edges of the screen in VR mode.
 * Render an aiming reticle in cockpit/external view. Made obsolete by Jeremy's exterior hook, but
 * good for debugging the FOVscale and y_center params -- and good for displaying the lights as
 * well.
 */
void PrimarySurface::RenderExternalHUD()
{
	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	bool bDirectSBS = (g_bEnableVR && !g_bUseSteamVR);
	float x0, y0, x1, y1;
	D3D11_VIEWPORT viewport;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const bool bExternalView = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;
	const bool bReticleInvisible = g_ReticleCentroid.x < 0.0f || g_ReticleCentroid.y < 0.0f;
	const bool bTriangleInvisible = g_TriangleCentroid.x < 0.0f || g_TriangleCentroid.y < 0.0f;

	// The reticle centroid is not visible and the triangle pointer isn't visible: nothing to do
	if (bReticleInvisible && bTriangleInvisible)
		return;

	this->_deviceResources->BeginAnnotatedEvent(L"RenderExternalHUD");

	// DEBUG
	//g_MetricRecCBuffer.mr_debug_value = g_fDebugFOVscale;
	//resources->InitVSConstantBufferMetricRec(resources->_metricRecVSConstantBuffer.GetAddressOf(), &g_MetricRecCBuffer);
	//resources->InitPSConstantBufferMetricRec(resources->_metricRecPSConstantBuffer.GetAddressOf(), &g_MetricRecCBuffer);
	// DEBUG

	//float sz, rhw;
	//ZToDepthRHW(g_fHUDDepth, &sz, &rhw);
	//log_debug("[DBG] sz: %0.3f, rhw: %0.3f", sz, rhw);

	// g_ReticleCentroid is in in-game coordinates. For the shader, we need to transform that into UVs:
	//float x = g_ReticleCentroid.x / g_fCurInGameWidth, y = g_ReticleCentroid.y / g_fCurInGameHeight;
	float x, y;
	InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
		(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, g_ReticleCentroid.x, g_ReticleCentroid.y, &x, &y);
	x /= g_fCurScreenWidth;
	y /= g_fCurScreenHeight;

	//log_debug("[DBG] g_ReticleCentroid, in-game coords: %0.3f, %0.3f", g_ReticleCentroid.x, g_ReticleCentroid.y);
	//log_debug("[DBG] ReticleCentroid UV: %0.3f, %0.3f, coords: %0.3f, %0.3f", x, y, x * g_fCurScreenWidth, y * g_fCurScreenHeight);

	// SunCoords[0].xy: Reticle Centroid
	// SunCoords[0].z: Inverse reticle scale
	// SunCoords[0].w: Reticle visible (0 off, 1 on)

	// SunCoords[1].x: Triangle pointer angle
	// SunCoords[1].y: Triangle pointer displacement
	// SunCoords[1].z: Triangle pointer scale (animated)
	// SunCoords[1].w: Render triangle pointer (0 off, 1 on)

	// Send the reticle centroid to the shader:
	g_ShadertoyBuffer.SunCoords[0].x = x;
	g_ShadertoyBuffer.SunCoords[0].y = y;
	g_ShadertoyBuffer.SunCoords[0].z = 1.0f / g_fReticleScale;
	g_ShadertoyBuffer.SunCoords[0].w = (float)(!bReticleInvisible);

	/*static float ang = 0.0f;
	ang += 6.0f * 0.01745f;
	if (ang > PI * 2.0f)
		ang -= (PI * 2.0f);*/
	static float time = 0.0f;
	time += 0.1f;
	if (time > 1.0f) time = 0.0f;
	// Convert the triangle centroid to screen coords:
	//InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
	//	(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, g_TriangleCentroid.x, g_TriangleCentroid.y, &x, &y);
	//Vector2 TriangleCentroidSC(x, y);
	g_TriangleCentroid -= 0.5f * Vector2(g_fCurInGameWidth, g_fCurInGameHeight);
	// Not sure if multiplying by preserveAspectRatioComp is necessary
	//g_TriangleCentroidSC.x *= g_ShadertoyBuffer.preserveAspectRatioComp[0];
	//g_TriangleCentroidSC.y *= g_ShadertoyBuffer.preserveAspectRatioComp[1];
	float ang = PI + atan2(g_TriangleCentroid.y, g_TriangleCentroid.x);
	g_ShadertoyBuffer.SunCoords[1].x = ang;
	g_ShadertoyBuffer.SunCoords[1].y = g_fTrianglePointerDist;
	g_ShadertoyBuffer.SunCoords[1].z = 0.1f + 0.05f * time;
	g_ShadertoyBuffer.SunCoords[1].w = (float)(!bTriangleInvisible);

	GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
	GetCraftViewMatrix(&g_ShadertoyBuffer.viewMat);
	g_ShadertoyBuffer.x0 = x0;
	g_ShadertoyBuffer.y0 = y0;
	g_ShadertoyBuffer.x1 = x1;
	g_ShadertoyBuffer.y1 = y1;
	//g_ShadertoyBuffer.iTime = 0;
	//g_ShadertoyBuffer.VRmode = bDirectSBS;
	g_ShadertoyBuffer.VRmode = g_bEnableVR ? (bDirectSBS ? 1 : 2) : 0; // 0 = non-VR, 1 = SBS, 2 = SteamVR
	g_ShadertoyBuffer.iResolution[0] = g_fCurScreenWidth;
	g_ShadertoyBuffer.iResolution[1] = g_fCurScreenHeight;
	g_ShadertoyBuffer.y_center = bExternalView ? 0.0f : g_fYCenter;
	g_ShadertoyBuffer.FOVscale = g_fFOVscale;
	
	/*/
	// DEBUG: Display light at index i:
	int i = 1;
	Vector4 xwaLight = Vector4(
		s_XwaGlobalLights[i].PositionX / 32768.0f,
		s_XwaGlobalLights[i].PositionY / 32768.0f,
		s_XwaGlobalLights[i].PositionZ / 32768.0f,
		0.0f);
	// Convert the XWA light direction into viewspace coordinates:
	Vector4 light = g_CurrentHeadingViewMatrix * xwaLight;
	
	Vector3 L(light.x, light.y, light.z);
	L *= 65536.0f;
	Vector3 p = projectMetric(L, g_viewMatrix, g_FullProjMatrixLeft);
	//log_debug("[DBG] p: %0.3f, %0.3f", p.x, p.y);
	g_ShadingSys_PSBuffer.MainLight.x = p.x;
	g_ShadingSys_PSBuffer.MainLight.y = g_bEnableVR ? -p.y : p.y;
	g_ShadingSys_PSBuffer.MainLight.z = light.z;
	resources->InitPSConstantShadingSystem(resources->_shadingSysBuffer.GetAddressOf(), &g_ShadingSys_PSBuffer);
	*/
	
	resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
	resources->InitPixelShader(g_bUseSteamVR ? resources->_externalHUDPS_VR : resources->_externalHUDPS);
	// We need this to ensure backface culling is disabled
	resources->InitRasterizerState(resources->_rasterizerState);

	context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
	if (g_bUseSteamVR)
		context->ResolveSubresource(
			resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1), 
			resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
	// Resolve the Reticle buffer
	{
		context->ResolveSubresource(resources->_ReticleBufAsInput, 0, resources->_ReticleBufMSAA, 0, BACKBUFFER_FORMAT);
		if (g_bDumpSSAOBuffers) {
			DirectX::SaveWICTextureToFile(context, resources->_ReticleBufAsInput, GUID_ContainerFormatPng,
				L"C:\\Temp\\_reticleBufAsInput.png");
		}
	}

	// Render the external HUD
	{
		// Set the new viewport (a full quad covering the full screen)
		viewport.Width  = g_fCurScreenWidth;
		viewport.Height = g_fCurScreenHeight;
		// VIEWPORT-LEFT
		if (g_bEnableVR) {
			if (g_bUseSteamVR)
				viewport.Width = (float)resources->_backbufferWidth;
			else
				viewport.Width = (float)resources->_backbufferWidth / 2.0f;
		}
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		resources->InitViewport(&viewport);

		// We don't need to clear the current vertex and pixel constant buffers.
		// Since we've just finished rendering 3D, they should contain values that
		// can be reused. So let's just overwrite the values that we need.
		g_VSCBuffer.aspect_ratio      =  g_fAspectRatio;
		g_VSCBuffer.z_override        = -1.0f;
		g_VSCBuffer.sz_override       = -1.0f;
		g_VSCBuffer.mult_z_override   = -1.0f;
		g_VSCBuffer.apply_uv_comp     =  false;
		g_VSCBuffer.bPreventTransform =  0.0f;
		g_VSCBuffer.bFullTransform    =  0.0f;
		if (g_bEnableVR)
		{
			g_VSCBuffer.viewportScale[0] = 1.0f / resources->_displayWidth;
			g_VSCBuffer.viewportScale[1] = 1.0f / resources->_displayHeight;
		}
		else
		{
			g_VSCBuffer.viewportScale[0] =  2.0f / resources->_displayWidth;
			g_VSCBuffer.viewportScale[1] = -2.0f / resources->_displayHeight;
		}

		// These are the same settings we used previously when rendering the HUD during a draw() call.
		// We use these settings to place the HUD at different depths
		g_VSCBuffer.z_override = g_fHUDDepth;
		// The Aiming HUD is now visible in external view using the exterior hook, let's put it at
		// infinity
		if (bExternalView)
			g_VSCBuffer.z_override = 65536.0f;
		if (g_bFloatingAimingHUD)
			g_VSCBuffer.bPreventTransform = 1.0f;

		// Set the left projection matrix (the viewMatrix is set at the beginning of the frame)
		g_VSMatrixCB.projEye[0] = g_FullProjMatrixLeft;
		g_VSMatrixCB.projEye[1] = g_FullProjMatrixRight;
		resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
		resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

		//resources->FillReticleVertexBuffer(g_fCurInGameWidth, g_fCurInGameHeight);
		UINT stride = sizeof(D3DTLVERTEX), offset = 0;
		resources->InitVertexBuffer(resources->_hyperspaceVertexBuffer.GetAddressOf(), &stride, &offset);
		//resources->InitVertexBuffer(resources->_reticleVertexBuffer.GetAddressOf(), &stride, &offset);
		resources->InitInputLayout(resources->_inputLayout);
		if (g_bEnableVR)
			resources->InitVertexShader(resources->_sbsVertexShader); // if (g_bEnableVR)
		else
			resources->InitVertexShader(resources->_vertexShader);

		resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		// Set the RTV:
		ID3D11RenderTargetView *rtvs[1] = {
			resources->_renderTargetViewPost.Get(), // Render to offscreenBufferPost instead of offscreenBuffer
		};
		context->OMSetRenderTargets(1, rtvs, NULL);
		// Set the SRVs:
		ID3D11ShaderResourceView *srvs[2] = {
			resources->_offscreenAsInputShaderResourceView.Get(),
			resources->_ReticleSRV.Get(),
		};
		context->PSSetShaderResources(0, 2, srvs);
		if (g_bUseSteamVR)
			context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(6, 0);

		if (!g_bEnableVR)
			goto out;

		if (g_bDumpSSAOBuffers) {
			DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferPost, GUID_ContainerFormatJpeg,
				L"C:\\Temp\\_offscreenBufPostExternalHUD.jpg");
		}

		// Second render: If VR mode is enabled, then compose the previous render with the offscreen buffer
		{
			// Reset the viewport for non-VR mode, post-proc viewport (cover the whole screen)
			viewport.TopLeftX = 0.0f;
			viewport.TopLeftY = 0.0f;
			viewport.Width    = g_fCurScreenWidth;
			viewport.Height   = g_fCurScreenHeight;
			viewport.MaxDepth = D3D11_MAX_DEPTH;
			viewport.MinDepth = D3D11_MIN_DEPTH;
			resources->InitViewport(&viewport);

			// Reset the vertex shader to regular 2D post-process
			// Set the Vertex Shader Constant buffers
			resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
				0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices

			// Set/Create the VertexBuffer and set the topology, etc
			UINT stride = sizeof(MainVertex), offset = 0;
			resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);
			resources->InitInputLayout(resources->_mainInputLayout);
			resources->InitVertexShader(g_bUseSteamVR ? resources->_mainVertexShaderVR : resources->_mainVertexShader);

			// Reset the UV limits for this shader
			GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
			g_ShadertoyBuffer.x0 = x0;
			g_ShadertoyBuffer.y0 = y0;
			g_ShadertoyBuffer.x1 = x1;
			g_ShadertoyBuffer.y1 = y1;
			g_ShadertoyBuffer.iResolution[0] = g_fCurScreenWidth;
			g_ShadertoyBuffer.iResolution[1] = g_fCurScreenHeight;
			resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
			resources->InitPixelShader(resources->_addGeomComposePS);

			// The output from the previous effect will be in offscreenBufferPost, so let's resolve it
			// to _shadertoyBuf to use it now:
			context->ResolveSubresource(resources->_shadertoyBuf, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);
			if (g_bUseSteamVR)
				context->ResolveSubresource(
					resources->_shadertoyBuf, D3D11CalcSubresource(0, 1, 1),
					resources->_offscreenBufferPost, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);

			context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
			ID3D11RenderTargetView *rtvs[1] = {
				resources->_renderTargetViewPost.Get(), // Render to offscreenBufferPost instead of offscreenBuffer
			};
			context->OMSetRenderTargets(1, rtvs, NULL);
			// Set the SRVs:
			ID3D11ShaderResourceView *srvs[2] = {
				resources->_offscreenAsInputShaderResourceView.Get(), // The current render
				resources->_shadertoySRV.Get(),	 // The effect rendered in the previous pass
			};
			context->PSSetShaderResources(0, 2, srvs);
			if (g_bUseSteamVR)
				context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
			else
				context->Draw(6, 0);
		}
	}

out:
	// Copy the result (_offscreenBufferPost) to the _offscreenBuffer so that it gets displayed
	context->CopyResource(resources->_offscreenBuffer, resources->_offscreenBufferPost);

	// Restore previous rendertarget, etc
	resources->InitInputLayout(resources->_inputLayout); // Not sure this is really needed

	this->_deviceResources->EndAnnotatedEvent();
}

inline void ProjectSpeedPoint(const Matrix4 &ViewMatrix, D3DTLVERTEX *particles, int idx)
{
	const float FOVFactor = g_ShadertoyBuffer.FOVscale;
	const float y_center = g_ShadertoyBuffer.y_center;
	//const float y_center = 0.0f;
	Vector4 P;
	P.x = particles[idx].sx;
	P.y = particles[idx].sy;
	P.z = particles[idx].sz;
	P.w = 1.0f;
	// Transform the point with the view matrix
	P = ViewMatrix * P;
	// Project to 2D
	P.x /= g_fAspectRatio; // TODO: Should this be g_fCurInGameAspectRatio?
	particles[idx].sx = FOVFactor * (P.x / P.z);
	particles[idx].sy = FOVFactor * (P.y / P.z) + y_center;
	particles[idx].sz = 0.0f; // We need to do this or the point will be clipped by DX. Setting it to 2.0 will clip it
	particles[idx].rhw = P.z; // Only used in VR to back-project (Ignored in non-VR mode)
	/*}
	else {
		// In VR, we leave the point in 3D, and we change the coordinates to match SteamVR's coord sys
		// This code won't work well until I figure out how to apply FOVscale and y_center in VR mode.
		particles[idx].sx =  P.x;
		particles[idx].sy =  P.y;
		particles[idx].sz = -P.z;
	}
	*/
}

inline void PrimarySurface::AddSpeedPoint(const Matrix4 &ViewMatrix, D3DTLVERTEX *particles,
	Vector4 Q, float zdisp, int ofs, float craft_speed)
{
	D3DTLVERTEX sample;
	// For VR we need to make the particles a little bigger or they'll be almost invisible:
	const float part_size = g_bEnableVR ? g_fSpeedShaderParticleSize * 2.0f : g_fSpeedShaderParticleSize;
	int j = ofs;
	//const float FOVFactor = g_ShadertoyBuffer.FOVscale;

	sample.sz = 0.0f;
	sample.rhw = 0.0f;
	// For VR we need a higher intensity or the effect will be almost invisible
	float gray = g_bEnableVR ? 1.0f : g_fSpeedShaderMaxIntensity * min(craft_speed, 1.0f);
	// Move the cutoff point a little above speed 0: we want the particles to disappear
	// a little before the craft stops.
	gray -= 0.1f;
	if (gray < 0.0f) gray = 0.0f;
	// The color is RRGGBB, so this value gets encoded in the blue component:
	// Disable the following line so that particles are still displayed when parked.
	sample.color = (uint32_t)(gray * 255.0f);
	// DEBUG: Do not fade speed particles
	//sample.color = 0xFFFFFFFF;
	// DEBUG

	// top
	particles[j] = sample;
	particles[j].sx = Q.x;
	particles[j].sy = Q.y - part_size;
	particles[j].sz = Q.z;
	particles[j].tu = -1.0;
	particles[j].tv =  1.0;
	ProjectSpeedPoint(ViewMatrix, particles, j);
	j++;

	particles[j] = sample;
	particles[j].sx = Q.x;
	particles[j].sy = Q.y;
	particles[j].sz = Q.z - part_size;
	particles[j].tu = -1.0;
	particles[j].tv = -1.0;
	ProjectSpeedPoint(ViewMatrix, particles, j);
	j++;

	particles[j] = sample;
	particles[j].sx = Q.x;
	particles[j].sy = Q.y;
	particles[j].sz = Q.z + zdisp;
	particles[j].tu = 1.0;
	particles[j].tv = 1.0;
	ProjectSpeedPoint(ViewMatrix, particles, j);
	j++;

	// bottom
	particles[j] = sample;
	particles[j].sx = Q.x;
	particles[j].sy = Q.y + part_size;
	particles[j].sz = Q.z;
	particles[j].tu =  1.0;
	particles[j].tv = -1.0;
	ProjectSpeedPoint(ViewMatrix, particles, j);
	j++;

	particles[j] = sample;
	particles[j].sx = Q.x;
	particles[j].sy = Q.y;
	particles[j].sz = Q.z - part_size;
	particles[j].tu = -1.0;
	particles[j].tv = -1.0;
	ProjectSpeedPoint(ViewMatrix, particles, j);
	j++;

	particles[j] = sample;
	particles[j].sx = Q.x;
	particles[j].sy = Q.y;
	particles[j].sz = Q.z + zdisp;
	particles[j].tu = 1.0;
	particles[j].tv = 1.0;
	ProjectSpeedPoint(ViewMatrix, particles, j);
	j++;


	// left
	particles[j] = sample;
	particles[j].sx = Q.x - part_size;
	particles[j].sy = Q.y;
	particles[j].sz = Q.z;
	particles[j].tu = -1.0;
	particles[j].tv =  1.0;
	ProjectSpeedPoint(ViewMatrix, particles, j);
	j++;

	particles[j] = sample;
	particles[j].sx = Q.x;
	particles[j].sy = Q.y;
	particles[j].sz = Q.z - part_size;
	particles[j].tu = -1.0;
	particles[j].tv = -1.0;
	ProjectSpeedPoint(ViewMatrix, particles, j);
	j++;

	particles[j] = sample;
	particles[j].sx = Q.x;
	particles[j].sy = Q.y;
	particles[j].sz = Q.z + zdisp;
	particles[j].tu = 1.0;
	particles[j].tv = 1.0;
	ProjectSpeedPoint(ViewMatrix, particles, j);
	j++;

	// right
	particles[j] = sample;
	particles[j].sx = Q.x + part_size;
	particles[j].sy = Q.y;
	particles[j].sz = Q.z;
	particles[j].tu = 1.0;
	particles[j].tv = -1.0;
	ProjectSpeedPoint(ViewMatrix, particles, j);
	j++;

	particles[j] = sample;
	particles[j].sx = Q.x;
	particles[j].sy = Q.y;
	particles[j].sz = Q.z - part_size;
	particles[j].tu = -1.0;
	particles[j].tv = -1.0;
	ProjectSpeedPoint(ViewMatrix, particles, j);
	j++;

	particles[j] = sample;
	particles[j].sx = Q.x;
	particles[j].sy = Q.y;
	particles[j].sz = Q.z + zdisp;
	particles[j].tu = 1.0;
	particles[j].tv = 1.0;
	ProjectSpeedPoint(ViewMatrix, particles, j);
	j++;

}

void PrimarySurface::RenderSpeedEffect()
{
	this->_deviceResources->BeginAnnotatedEvent(L"RenderSpeedEffect");

	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	bool bDirectSBS = (g_bEnableVR && !g_bUseSteamVR);
	float x0, y0, x1, y1;
	D3D11_VIEWPORT viewport;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	int NumParticleVertices = 0, NumParticles = 0;
	const bool bExternalView = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;
	static float ZTimeDisp[MAX_SPEED_PARTICLES] = { 0 };
	float craft_speed = PlayerDataTable[*g_playerIndex].currentSpeed / g_fSpeedShaderScaleFactor;

	/*
	viewMode1 and viewMode2 both become 18 in external view and 0 in cockpit view.
	They don't change when fly-by camera is active
	log_debug("[DBG] RelatedToCamera: %d, viewMode1: %d viewMode2: %d",
		PlayerDataTable[*g_playerIndex]._RelatedToCamera_,
		PlayerDataTable[*g_playerIndex].viewMode1,
		PlayerDataTable[*g_playerIndex].viewMode2);
	*/

	Vector4 Rs, Us, Fs;
	Matrix4 ViewMatrix, HeadingMatrix = GetCurrentHeadingMatrix(Rs, Us, Fs, false);
	if (g_bEnableVR) {
		// Apply the headtracking transform to the ViewMatrix
		ViewMatrix = g_VSMatrixCB.fullViewMat;
		ViewMatrix.invert();
		if (PlayerDataTable[*g_playerIndex].gunnerTurretActive) {
			Matrix4 GunnerMatrix;
			Matrix4 S = Matrix4().scale(1, 1, -1);
			GetGunnerTurretViewMatrixSpeedEffect(&GunnerMatrix);
			ViewMatrix = S * ViewMatrix * S * GunnerMatrix;
		}
		else {
			Matrix4 S = Matrix4().scale(-1, -1, 1);
			ViewMatrix = S * ViewMatrix * S;
		}
	}
	else
		GetCockpitViewMatrixSpeedEffect(&ViewMatrix, false);

	GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
	g_ShadertoyBuffer.x0 = x0;
	g_ShadertoyBuffer.y0 = y0;
	g_ShadertoyBuffer.x1 = x1;
	g_ShadertoyBuffer.y1 = y1;
	g_ShadertoyBuffer.y_center = bExternalView ? 0.0f : g_fYCenter;
	g_ShadertoyBuffer.VRmode = bDirectSBS;
	g_ShadertoyBuffer.iResolution[0] = g_fCurScreenWidth;
	g_ShadertoyBuffer.iResolution[1] = g_fCurScreenHeight;
	//g_ShadertoyBuffer.craft_speed = craft_speed;
	// The A-Wing's max speed seems to be 270
	//log_debug("[DBG] speed: %d", PlayerDataTable[*g_playerIndex].currentSpeed);
	// g_ShadertoyBuffer.FOVscale must be set! We'll need it for this shader

	resources->InitPixelShader(resources->_speedEffectPS);
	resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);

	context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
	if (g_bUseSteamVR)
		context->ResolveSubresource(
			resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
			resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);

	// Update the position of the particles, project them to 2D and add them to the vertex buffer
	{
		// If the range is big, we also need to increase the speed of the particles. The default
		// range size is 10.0f, the following factor will accelerate the particles if the range
		// is bigger than that.
		const float SPEED_CONST = g_fSpeedShaderParticleRange / 10.0f;
		Vector4 QH, QT, RH, RT;
		for (int i = 0; i < MAX_SPEED_PARTICLES; i++) {
			float zdisp = 0.0f;
			// Transform the particles from worldspace to craftspace (using the craft's heading):
			QH = HeadingMatrix * g_SpeedParticles[i];
			// Update the position in craftspace. In this coord system,
			// Forward is always Z+, so we just have to translate points
			// along the Z axis.
			QH.z -= ZTimeDisp[i];
			// ZTimeDisp has to be updated like this to avoid the particles from moving
			// backwards when we brake
			ZTimeDisp[i] += 0.0166f * SPEED_CONST * craft_speed;
			// This is the Head-to-tail displacement along the z-axis:
			zdisp = craft_speed * g_fSpeedShaderTrailSize;

			// Transform the current particle into viewspace
			// QH is the head of the particle, QT is the tail:
			QT = QH; QT.z += zdisp;
			RH = ViewMatrix * QH; // Head
			RT = ViewMatrix * QT; // Tail
			// If the particle is behind the camera, or outside the clipping space, then
			// compute a new position for it. We need to test both the head and the tail
			// of the particle, or we'll get ugly artifacts when looking back.
			if (
				// Clip the head
				RH.z < 1.0f || RH.z > g_fSpeedShaderParticleRange ||
				RH.x < -g_fSpeedShaderParticleRange || RH.x > g_fSpeedShaderParticleRange ||
				RH.y < -g_fSpeedShaderParticleRange || RH.y > g_fSpeedShaderParticleRange ||
				// Clip the tail
				RT.z < 1.0f || RT.z > g_fSpeedShaderParticleRange ||
				RT.x < -g_fSpeedShaderParticleRange || RT.x > g_fSpeedShaderParticleRange ||
				RT.y < -g_fSpeedShaderParticleRange || RT.y > g_fSpeedShaderParticleRange
				)
			{
				// Compute a new random position for this particle
				float x = (((float)rand() / RAND_MAX) - 0.5f);
				float y = (((float)rand() / RAND_MAX) - 0.5f);
				float z = (((float)rand() / RAND_MAX) - 0.5f);
				// We multiply x and y by 0.5 so that most of the particles appear in the direction
				// of travel... or that's the idea anyway
				g_SpeedParticles[i].x = x * g_fSpeedShaderParticleRange * 0.5f;
				g_SpeedParticles[i].y = y * g_fSpeedShaderParticleRange * 0.5f;
				g_SpeedParticles[i].z = z * g_fSpeedShaderParticleRange;
				ZTimeDisp[i] = 0.0f;
			}
			else
			{
				// Transform the current point with the camera matrix, project it and
				// add it to the vertex buffer
				if (NumParticles < g_iSpeedShaderMaxParticles)
				{
					AddSpeedPoint(ViewMatrix, g_SpeedParticles2D, QH, zdisp, NumParticleVertices, craft_speed);
					NumParticleVertices += 12;
					NumParticles++;
				}
			}
		}

		D3D11_MAPPED_SUBRESOURCE map;
		HRESULT hr = context->Map(resources->_speedParticlesVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
		if (SUCCEEDED(hr))
		{
			size_t length = sizeof(D3DTLVERTEX) * NumParticleVertices;
			memcpy(map.pData, g_SpeedParticles2D, length);
			context->Unmap(resources->_speedParticlesVertexBuffer, 0);
		}

		UINT stride = sizeof(D3DTLVERTEX), offset = 0;
		resources->InitVertexBuffer(resources->_speedParticlesVertexBuffer.GetAddressOf(), &stride, &offset);
		resources->InitInputLayout(resources->_inputLayout);
		resources->InitVertexShader(g_bUseSteamVR ? resources->_speedEffectVS_VR : resources->_speedEffectVS);
		resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	// First render: Render the speed effect
	// input: None
	// output: renderTargetViewPost
	{
		if (g_bEnableVR) 
		{
			// This should be the same viewport used in the Execute() function
			// Set the new viewport (a full quad covering the full screen)
			// VIEWPORT-LEFT
			if (g_bUseSteamVR)
				viewport.Width = (float)resources->_backbufferWidth;
			else
				viewport.Width = (float)resources->_backbufferWidth / 2.0f;
			viewport.Height   = g_fCurScreenHeight;
			viewport.TopLeftX = 0.0f;
			viewport.TopLeftY = 0.0f;
			viewport.MinDepth = D3D11_MIN_DEPTH;
			viewport.MaxDepth = D3D11_MAX_DEPTH;
			resources->InitViewport(&viewport);
		}
		else
			resources->InitViewport(&g_nonVRViewport);

		// We don't need to clear the current vertex and pixel constant buffers.
		// Since we've just finished rendering 3D, they should contain values that
		// can be reused. So let's just overwrite the values that we need.
		g_VSCBuffer.aspect_ratio		=  g_bEnableVR ? g_fAspectRatio : g_fCurInGameAspectRatio;
		g_VSCBuffer.z_override			= -1.0f;
		g_VSCBuffer.sz_override			= -1.0f;
		g_VSCBuffer.mult_z_override		= -1.0f;
		g_VSCBuffer.apply_uv_comp       =  false;
		g_VSCBuffer.bPreventTransform	=  0.0f;
		if (g_bEnableVR)
		{
			g_VSCBuffer.bFullTransform = 1.0f; // Setting bFullTransform tells the VS to use VR projection matrices
			g_VSCBuffer.viewportScale[0] = 1.0f / resources->_displayWidth;
			g_VSCBuffer.viewportScale[1] = 1.0f / resources->_displayHeight;
		}
		else
		{
			g_VSCBuffer.bFullTransform = 0.0f; // Setting bFullTransform tells the VS to use VR projection matrices
			g_VSCBuffer.viewportScale[0] =  2.0f / resources->_displayWidth;
			g_VSCBuffer.viewportScale[1] = -2.0f / resources->_displayHeight;
		}

		// Set the left projection matrix (the viewMatrix is set at the beginning of the frame)
		g_VSMatrixCB.projEye[0] = g_FullProjMatrixLeft;
		g_VSMatrixCB.projEye[1] = g_FullProjMatrixRight;
		resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
		resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

		context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		// Set the RTV:
		ID3D11RenderTargetView *rtvs[1] = {
			resources->_renderTargetViewPost.Get(), // Render to offscreenBufferPost instead of offscreenBuffer
		};
		context->OMSetRenderTargets(1, rtvs, NULL);
		if (g_bUseSteamVR)
			context->DrawInstanced(NumParticleVertices, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(NumParticleVertices, 0);

		if (g_bEnableVR && !g_bUseSteamVR) {
			// VIEWPORT-RIGHT
			viewport.Width = (float)resources->_backbufferWidth / 2.0f;
			viewport.TopLeftX = (float)viewport.Width;
			viewport.Height = (float)resources->_backbufferHeight;
			viewport.TopLeftY = 0.0f;
			viewport.MinDepth = D3D11_MIN_DEPTH;
			viewport.MaxDepth = D3D11_MAX_DEPTH;
			resources->InitViewport(&viewport);
			// Set the right projection matrix
			g_VSMatrixCB.projEye[0] = g_FullProjMatrixRight;
			resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

			context->OMSetRenderTargets(1, resources->_renderTargetViewPost.GetAddressOf(), NULL);
			context->Draw(NumParticleVertices, 0);
		}
	}

	// Second render: compose the cockpit over the previous effect
	{
		// Reset the viewport for non-VR mode, post-proc viewport (cover the whole screen)
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width    = g_fCurScreenWidth;
		viewport.Height   = g_fCurScreenHeight;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		resources->InitViewport(&viewport);

		// Reset the vertex shader to regular 2D post-process
		// Set the Vertex Shader Constant buffers
		resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
			0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices

		// Set/Create the VertexBuffer and set the topology, etc
		UINT stride = sizeof(MainVertex), offset = 0;
		resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);
		resources->InitInputLayout(resources->_mainInputLayout);
		resources->InitVertexShader(g_bUseSteamVR ? resources->_mainVertexShaderVR : resources->_mainVertexShader);

		// Reset the UV limits for this shader
		GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
		g_ShadertoyBuffer.x0 = x0;
		g_ShadertoyBuffer.y0 = y0;
		g_ShadertoyBuffer.x1 = x1;
		g_ShadertoyBuffer.y1 = y1;
		g_ShadertoyBuffer.iResolution[0] = g_fCurScreenWidth;
		g_ShadertoyBuffer.iResolution[1] = g_fCurScreenHeight;
		resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);

		resources->InitPixelShader(g_bUseSteamVR ? resources->_speedEffectComposePS_VR : resources->_speedEffectComposePS);
		// Clear all the render target views
		ID3D11RenderTargetView *rtvs_null[5] = {
			NULL, // Main RTV
			NULL, // Bloom
			NULL, // Depth
			NULL, // Norm Buf
			NULL, // SSAO Mask
		};
		context->OMSetRenderTargets(5, rtvs_null, NULL);

		// DEBUG
		/*
		if (g_iPresentCounter == CAPTURE_FRAME)
		{
			DirectX::SaveWICTextureToFile(context, resources->_shadertoyAuxBuf, GUID_ContainerFormatJpeg,
				L"C:\\Temp\\_shadertoyAuxBuf.jpg");
			DirectX::SaveWICTextureToFile(context, resources->_shadertoyBuf, GUID_ContainerFormatJpeg,
				L"C:\\Temp\\_shadertoyBuf.jpg");
			DirectX::SaveWICTextureToFile(context, resources->_offscreenBuffer, GUID_ContainerFormatJpeg,
				L"C:\\Temp\\_offscreenBuf-1.jpg");
			DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferPost, GUID_ContainerFormatJpeg,
				L"C:\\Temp\\_offscreenBufferPost-1.jpg");
		}
		*/
		// DEBUG

		// The output from the previous effect will be in offscreenBufferPost, so let's resolve it
		// to _shadertoyBuf to use it now:
		context->ResolveSubresource(resources->_shadertoyBuf, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);
		if (g_bUseSteamVR)
			context->ResolveSubresource(
				resources->_shadertoyBuf, D3D11CalcSubresource(0, 1, 1),
				resources->_offscreenBufferPost, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);

		context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		ID3D11RenderTargetView *rtvs[1] = {
			resources->_renderTargetViewPost.Get(), // Render to offscreenBufferPost instead of offscreenBuffer
		};
		context->OMSetRenderTargets(1, rtvs, NULL);
		// Set the SRVs:
		ID3D11ShaderResourceView *srvs[3] = {
			resources->_offscreenAsInputShaderResourceView.Get(), // The current render
			resources->_shadertoySRV.Get(),	 // The effect rendered in the previous pass
			resources->_depthBufSRV.Get(),   // The depth buffer
		};
		context->PSSetShaderResources(0, 3, srvs);
		if (g_bUseSteamVR)
			context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(6, 0);
	}

	// Copy the result (_offscreenBufferPost) to the _offscreenBuffer so that it gets displayed
	context->CopyResource(resources->_offscreenBuffer, resources->_offscreenBufferPost);

	// Restore previous rendertarget, etc
	resources->InitInputLayout(resources->_inputLayout); // Not sure this is really needed

	this->_deviceResources->EndAnnotatedEvent();
}

inline D3DCOLOR PrimarySurface::EncodeNormal(Vector3 N)
{
	N.normalize();
	uint32_t x = (uint32_t)(255.0f * (0.5f * N.x + 0.5f));
	uint32_t y = (uint32_t)(255.0f * (0.5f * N.y + 0.5f));
	uint32_t z = (uint32_t)(255.0f * (0.5f * N.z + 0.5f));
	D3DCOLOR col = (x << 24) | (y << 16) | (z << 8);
	return col;
}

/*
 * Adds geometry and returns the number of vertices added
 */
inline int PrimarySurface::AddGeometry(const Matrix4 &ViewMatrix, D3DTLVERTEX *particles,
	Vector4 Q, float zdisp, int ofs)
{
	D3DTLVERTEX sample;
	const float part_size = 0.025f;
	int j = ofs;

	sample.sz = 0.0f;
	sample.rhw = 0.0f;
	// AARRGGBB
	sample.color = 0xFFFFFF00;

	// y- is down
	// Fun fact: in external view, the origin is exactly on top of the aiming reticle

	// top-left
	particles[j] = sample;
	particles[j].sx = Q.x - part_size;
	particles[j].sy = Q.y + part_size;
	particles[j].sz = Q.z;
	particles[j].tu = -1.0;
	particles[j].tv =  1.0;
	particles[j].specular = EncodeNormal(Vector3(0.0f, 0.0f, -1.0f));
	ProjectSpeedPoint(ViewMatrix, particles, j);
	j++;

	// top-right
	particles[j] = sample;
	particles[j].sx = Q.x + part_size;
	particles[j].sy = Q.y + part_size;
	particles[j].sz = Q.z;
	particles[j].tu = 1.0;
	particles[j].tv = 1.0;
	particles[j].specular = EncodeNormal(Vector3(0.0f, 0.0f, -1.0f));
	ProjectSpeedPoint(ViewMatrix, particles, j);
	j++;

	// center
	particles[j] = sample;
	particles[j].sx = Q.x;
	particles[j].sy = Q.y;
	particles[j].sz = Q.z;
	particles[j].tu = 0.0;
	particles[j].tv = 0.0;
	particles[j].specular = EncodeNormal(Vector3(0.0f, 0.0f, -1.0f));
	ProjectSpeedPoint(ViewMatrix, particles, j);
	j++;

	return j - ofs;
}

/*
 * Computes a matrix that can be used to render OBJs modeled in Blender over the OPTs.
 * OBJs have to be pre-scaled by (1.64,1.64,-1.64). It also returns the current
 * HeadingMatrix and the current CockpitCamera matrix.
 *
 * Computes a matrix that transforms points in Modelspace -- as in the framework
 * used to model the OPTs -- into Viewspace 3D.
 *
 * This Viewspace 3D coord sys is *not* directly compatible with backprojected 3D 
 * -- as in the coord sys created during (SBS)VertexShader backprojection -- but
 * a further Viewspace 3D --> 2D projection that compensates for FOVscale and y_center
 * will project into XWA 2D (see AddGeometryVertexShader).
 *
 * In other words, things modeled in Blender over the OPT can be added to the current
 * cockpit using this matrix.
 *
 * The transform chain is:
 *
 * -POV --> CockpitCamera --> -CockpitTranslation
 *
 * In Matrix form:
 *
 * Result = -CockpitTranslation * CockpitCamera * (-POV)
 *
 * In other words:
 *
 * "Translate POV to origin, apply current cockpit camera, apply current cockpit translation"
 *
 * After applying this matrix, do the following to project to XWA 2D:
 *
 * P.x /= sm_aspect_ratio;
 * P.x = FOVscale * (P.x / P.z);
 * P.y = FOVscale * (P.y / P.z) + y_center;
 *
 * This function is used in RenderAdditionalGeometry() and RenderShadowMapOBJ().
 */
Matrix4 PrimarySurface::ComputeAddGeomViewMatrix(Matrix4 *HeadingMatrix, Matrix4 *CockpitMatrix)
{
	Vector4 Rs, Us, Fs, T;
	Matrix4 ViewMatrix, Translation;
	*HeadingMatrix = GetCurrentHeadingMatrix(Rs, Us, Fs, false);
	GetCockpitViewMatrixSpeedEffect(CockpitMatrix, false);
	ViewMatrix = *CockpitMatrix;

	// Apply the roll in VR mode:
	if (g_bEnableVR)
		ViewMatrix = g_VSMatrixCB.viewMat * ViewMatrix;

	// Translate the model so that the origin is at the POV and then scale it
	Matrix4 POVTrans;
	g_ShadowMapVSCBuffer.POV.x = -(*g_POV_X) / g_ShadowMapping.POV_XY_FACTOR;
	g_ShadowMapVSCBuffer.POV.y = -(*g_POV_Z) / g_ShadowMapping.POV_XY_FACTOR;
	g_ShadowMapVSCBuffer.POV.z =  (*g_POV_Y) / g_ShadowMapping.POV_Z_FACTOR; // For some reason, depth is inverted w.r.t POV_Y0
	POVTrans.translate(g_ShadowMapVSCBuffer.POV.x, g_ShadowMapVSCBuffer.POV.y, g_ShadowMapVSCBuffer.POV.z);
	ViewMatrix = ViewMatrix * POVTrans;

	if (g_bDumpSSAOBuffers) {
		log_debug("[DBG] [SHW] X/W HP: %d, %d, %d",
			CraftDefinitionTable[0].LaserEndingHardpoint[0],
			CraftDefinitionTable[0].LaserEndingHardpoint[1],
			CraftDefinitionTable[0].LaserEndingHardpoint[2]);
		log_debug("[DBG] [SHW] Y/W HP: %d, %d, %d",
			CraftDefinitionTable[1].LaserEndingHardpoint[0],
			CraftDefinitionTable[1].LaserEndingHardpoint[1],
			CraftDefinitionTable[1].LaserEndingHardpoint[2]);

		//log_debug("[DBG] object->y: %d", (*objects)[PlayerDataTable[*g_playerIndex].objectIndex].y);
		//log_debug("[DBG] objects: 0x%x, g_objectPtr: 0x%x", objects, *g_objectPtr);
		//log_debug("[DBG] objectIndex: %d, g_FlightSurfaceHeight: %d, cameraY: %d",
			//PlayerDataTable[*g_playerIndex].objectIndex,
			//objects[PlayerDataTable[*g_playerIndex].objectIndex].y, 
			//g_FlightSurfaceHeight, // Not relevant
			//PlayerDataTable[*g_playerIndex].cameraY); // This looks like the absolute pos of the camera

		//log_debug("[DBG] [SHW] POV0: %0.3f, %0.3f, %0.3f", (float)*g_POV_X0, (float)*g_POV_Z0, (float)*g_POV_Y0);
		log_debug("[DBG] [SHW] POV (Original): %0.3f, %0.3f, %0.3f", *g_POV_X, *g_POV_Z, *g_POV_Y);
		log_debug("[DBG] [SHW] Using POV: %0.3f, %0.3f, %0.3f",
			g_ShadowMapVSCBuffer.POV.x, g_ShadowMapVSCBuffer.POV.y, g_ShadowMapVSCBuffer.POV.z);
	}

	// Add the CockpitRef translation
	T.x = PlayerDataTable[*g_playerIndex].Camera.ShakeX * g_fCockpitTranslationScale;
	T.y = PlayerDataTable[*g_playerIndex].Camera.ShakeY * g_fCockpitTranslationScale;
	T.z = PlayerDataTable[*g_playerIndex].Camera.ShakeZ * g_fCockpitTranslationScale;

	T.w = 0.0f;
	T = *HeadingMatrix * T; // The heading matrix is needed to convert the translation into the correct frame
	Translation.translate(-T.x, -T.y, -T.z);
	ViewMatrix = ViewMatrix * Translation;

	if (g_bEnableVR)
	{
		Matrix4 S = Matrix4().scale(-1, -1, 1);
		Matrix4 trackingMat = g_VSMatrixCB.fullViewMat;

		// Get the translation vector
		const float *m = trackingMat.get();
		Vector4 t(m[12], m[13], m[14], 1.0f);
		Matrix4 T = Matrix4().translate(-t.x, -t.y, t.z);

		// Zero-out the translation component
		Vector4 Zero(0, 0, 0, 1);
		trackingMat.setColumn(3, Zero);

		trackingMat.invert();
		ViewMatrix = S * trackingMat * S * T * ViewMatrix;
	}
	return ViewMatrix;
}

void PrimarySurface::RenderAdditionalGeometry()
{
	this->_deviceResources->BeginAnnotatedEvent(L"RenderAdditionalGeometry");

	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	float x0, y0, x1, y1;
	D3D11_VIEWPORT viewport;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	int NumParticleVertices = 0, NumParticles = 0;
	const bool bExternalView = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;
	Matrix4 HeadingMatrix, CockpitMatrix;

	Matrix4 ViewMatrix = ComputeAddGeomViewMatrix(&HeadingMatrix, &CockpitMatrix);
	
	GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
	g_ShadertoyBuffer.x0 = x0;
	g_ShadertoyBuffer.y0 = y0;
	g_ShadertoyBuffer.x1 = x1;
	g_ShadertoyBuffer.y1 = y1;
	g_ShadertoyBuffer.y_center = bExternalView ? 0.0f : g_fYCenter;
	g_ShadertoyBuffer.iResolution[0] = g_fCurScreenWidth;
	g_ShadertoyBuffer.iResolution[1] = g_fCurScreenHeight;

	resources->InitPixelShader(resources->_addGeomPS);
	resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);

	context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
	if (g_bUseSteamVR)
		context->ResolveSubresource(
			resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
			resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);

	// Update the position of the particles, project them to 2D and add them to the vertex buffer
	/*
	{
		Vector4 QH, QT, RH, RT;
		for (int i = 0; i < 1; i++) {
			// Transform the particles from worldspace to craftspace (using the craft's heading):
			//QH = HeadingMatrix * g_SpeedParticles[i];
			QH.x = 0.0f;
			QH.y = 0.0f;
			QH.z = 1.0f;

			// Transform the current particle into viewspace
			RH = ViewMatrix * QH;

			// Clip the geometry
			if (RH.z > 0.1f && RH.z < g_fSpeedShaderParticleRange &&
				RH.x > -g_fSpeedShaderParticleRange && RH.x < g_fSpeedShaderParticleRange &&
				RH.y > -g_fSpeedShaderParticleRange && RH.y < g_fSpeedShaderParticleRange
			   )
			{
				// Transform the current point with the camera matrix, project it and
				// add it to the vertex buffer
				if (NumParticleVertices < MAX_SPEED_PARTICLES)
					NumParticleVertices += AddGeometry(ViewMatrix, g_SpeedParticles2D, QH, 0.0f, NumParticleVertices);
			}
		}

		D3D11_MAPPED_SUBRESOURCE map;
		HRESULT hr = context->Map(resources->_speedParticlesVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
		if (SUCCEEDED(hr))
		{
			size_t length = sizeof(D3DTLVERTEX) * NumParticleVertices;
			memcpy(map.pData, g_SpeedParticles2D, length);
			context->Unmap(resources->_speedParticlesVertexBuffer, 0);
		}

		UINT stride = sizeof(D3DTLVERTEX), offset = 0;
		resources->InitVertexBuffer(resources->_speedParticlesVertexBuffer.GetAddressOf(), &stride, &offset);
		resources->InitInputLayout(resources->_inputLayout);
		resources->InitVertexShader(resources->_addGeomVS);
		resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	*/

	// Render the Shadow Map OBJ as additional geometry
	UINT stride = sizeof(D3DTLVERTEX), offset = 0;
	resources->InitVertexBuffer(resources->_shadowVertexBuffer.GetAddressOf(), &stride, &offset);
	resources->InitIndexBuffer(resources->_shadowIndexBuffer.Get(), false);
	resources->InitInputLayout(resources->_inputLayout);
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitVertexShader(resources->_addGeomVS);

	// Set the ViewMatrix
	g_ShadowMapVSCBuffer.Camera = ViewMatrix;
	g_ShadowMapVSCBuffer.sm_aspect_ratio = g_fCurInGameAspectRatio;
	resources->InitVSConstantBufferShadowMap(resources->_shadowMappingVSConstantBuffer.GetAddressOf(), &g_ShadowMapVSCBuffer);
	resources->InitVSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);

	// First render: Render the additional geometry
	// input: None
	// output: renderTargetViewPost
	{
		if (g_bEnableVR)
		{
			// This should be the same viewport used in the Execute() function
			// Set the new viewport (a full quad covering the full screen)
			// VIEWPORT-LEFT
			if (g_bUseSteamVR)
				viewport.Width = (float)resources->_backbufferWidth;
			else
				viewport.Width = (float)resources->_backbufferWidth / 2.0f;
			viewport.Height = g_fCurScreenHeight;
			viewport.TopLeftX = 0.0f;
			viewport.TopLeftY = 0.0f;
			viewport.MinDepth = D3D11_MIN_DEPTH;
			viewport.MaxDepth = D3D11_MAX_DEPTH;
			resources->InitViewport(&viewport);
		}
		else
			resources->InitViewport(&g_nonVRViewport);

		// We don't need to clear the current vertex and pixel constant buffers.
		// Since we've just finished rendering 3D, they should contain values that
		// can be reused. So let's just overwrite the values that we need.
		g_VSCBuffer.aspect_ratio		=  g_fAspectRatio;
		g_VSCBuffer.z_override			= -1.0f;
		g_VSCBuffer.sz_override			= -1.0f;
		g_VSCBuffer.mult_z_override		= -1.0f;
		g_VSCBuffer.apply_uv_comp       =  false;
		g_VSCBuffer.bPreventTransform	=  0.0f;
		if (g_bEnableVR)
		{
			g_VSCBuffer.bFullTransform   = 1.0f; // Setting bFullTransform tells the VS to use VR projection matrices
			g_VSCBuffer.viewportScale[0] = 1.0f / resources->_displayWidth;
			g_VSCBuffer.viewportScale[1] = 1.0f / resources->_displayHeight;
		}
		else
		{
			g_VSCBuffer.bFullTransform   =  0.0f; // Setting bFullTransform tells the VS to use VR projection matrices
			g_VSCBuffer.viewportScale[0] =  2.0f / resources->_displayWidth;
			g_VSCBuffer.viewportScale[1] = -2.0f / resources->_displayHeight;
		}

		// Set the left projection matrix (the viewMatrix is set at the beginning of the frame)
		g_VSMatrixCB.projEye[0] = g_FullProjMatrixLeft;
		g_VSMatrixCB.projEye[1] = g_FullProjMatrixRight;
		resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
		resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

		context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		// Set the RTV:
		ID3D11RenderTargetView *rtvs[1] = {
			resources->_renderTargetViewPost.Get(), // Render to offscreenBufferPost instead of offscreenBuffer
		};
		context->OMSetRenderTargets(1, rtvs, NULL);
		if (g_bUseSteamVR)
			context->DrawIndexedInstanced(g_ShadowMapping.NumIndices, 2, 0, 0, 0); // Draw OBJ // if (g_bUseSteamVR)
		else
			context->DrawIndexed(g_ShadowMapping.NumIndices, 0, 0);

		// Render the additional geometry on the right eye
		if (g_bEnableVR && !g_bUseSteamVR) {
			// VIEWPORT-RIGHT
			viewport.Width = (float)resources->_backbufferWidth / 2.0f;
			viewport.TopLeftX = (float)viewport.Width;
			viewport.Height = (float)resources->_backbufferHeight;
			viewport.TopLeftY = 0.0f;
			viewport.MinDepth = D3D11_MIN_DEPTH;
			viewport.MaxDepth = D3D11_MAX_DEPTH;
			resources->InitViewport(&viewport);
			// Set the right projection matrix
			g_VSMatrixCB.projEye[0] = g_FullProjMatrixRight;
			resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

			if (g_bUseSteamVR)
				context->OMSetRenderTargets(1, resources->_renderTargetViewPostR.GetAddressOf(), NULL);
			else
				context->OMSetRenderTargets(1, resources->_renderTargetViewPost.GetAddressOf(), NULL);
			context->DrawIndexed(g_ShadowMapping.NumIndices, 0, 0); // Draw OBJ
		}
	}

	/*
	if (g_bDumpSSAOBuffers)
	{
		DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferPost, GUID_ContainerFormatPng, L"C:\\Temp\\_addGeom.png");
	}
	*/

	// Second render: compose
	{
		// Reset the viewport for non-VR mode, post-proc viewport (cover the whole screen)
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width    = g_fCurScreenWidth;
		viewport.Height   = g_fCurScreenHeight;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		resources->InitViewport(&viewport);

		// Reset the vertex shader to regular 2D post-process
		// Set the Vertex Shader Constant buffers
		resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
			0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices

		// Set/Create the VertexBuffer and set the topology, etc
		UINT stride = sizeof(MainVertex), offset = 0;
		resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);
		resources->InitInputLayout(resources->_mainInputLayout);
		resources->InitVertexShader(g_bUseSteamVR ? resources->_mainVertexShaderVR : resources->_mainVertexShader);

		// Reset the UV limits for this shader
		GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
		g_ShadertoyBuffer.x0 = x0;
		g_ShadertoyBuffer.y0 = y0;
		g_ShadertoyBuffer.x1 = x1;
		g_ShadertoyBuffer.y1 = y1;
		g_ShadertoyBuffer.iResolution[0] = g_fCurScreenWidth;
		g_ShadertoyBuffer.iResolution[1] = g_fCurScreenHeight;
		resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);

		resources->InitPixelShader(resources->_addGeomComposePS);
		// Clear all the render target views
		ID3D11RenderTargetView *rtvs_null[5] = {
			NULL, // Main RTV
			NULL, // Bloom
			NULL, // Depth
			NULL, // Norm Buf
			NULL, // SSAO Mask
		};
		context->OMSetRenderTargets(5, rtvs_null, NULL);

		// DEBUG
		/*
		if (g_iPresentCounter == CAPTURE_FRAME)
		{
			DirectX::SaveWICTextureToFile(context, resources->_shadertoyAuxBuf, GUID_ContainerFormatJpeg,
				L"C:\\Temp\\_shadertoyAuxBuf.jpg");
			DirectX::SaveWICTextureToFile(context, resources->_shadertoyBuf, GUID_ContainerFormatJpeg,
				L"C:\\Temp\\_shadertoyBuf.jpg");
			DirectX::SaveWICTextureToFile(context, resources->_offscreenBuffer, GUID_ContainerFormatJpeg,
				L"C:\\Temp\\_offscreenBuf-1.jpg");
			DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferPost, GUID_ContainerFormatJpeg,
				L"C:\\Temp\\_offscreenBufferPost-1.jpg");
		}
		*/
		// DEBUG

		// The output from the previous effect will be in offscreenBufferPost, so let's resolve it
		// to _shadertoyBuf to use it now:
		context->ResolveSubresource(resources->_shadertoyBuf, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);
		if (g_bUseSteamVR)
			context->ResolveSubresource(
				resources->_shadertoyBuf, D3D11CalcSubresource(0, 1, 1),
				resources->_offscreenBufferPost, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);

		context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		ID3D11RenderTargetView *rtvs[1] = {
			resources->_renderTargetViewPost.Get(), // Render to offscreenBufferPost instead of offscreenBuffer
		};
		context->OMSetRenderTargets(1, rtvs, NULL);
		// Set the SRVs:
		ID3D11ShaderResourceView *srvs[2] = {
			resources->_offscreenAsInputShaderResourceView.Get(), // The current render
			resources->_shadertoySRV.Get(),	 // The effect rendered in the previous pass
			//resources->_depthBufSRV.Get(),   // The depth buffer
		};
		context->PSSetShaderResources(0, 2, srvs);
		if (g_bUseSteamVR)
			context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(6, 0);
	}

	// Copy the result (_offscreenBufferPost) to the _offscreenBuffer so that it gets displayed
	context->CopyResource(resources->_offscreenBuffer, resources->_offscreenBufferPost);

	// Restore previous rendertarget, etc
	resources->InitInputLayout(resources->_inputLayout); // Not sure this is really needed

	this->_deviceResources->EndAnnotatedEvent();
}

/*
 * Computes the Parallel Projection Matrix from the point of view of the current light.
 * Since this is a parallel projection, the result is a Rotation Matrix that aligns the
 * ViewSpace coordinates so that the origin is now the current light looking at the
 * previous ViewSpace origin.
 */
Matrix4 ComputeLightViewMatrix(int idx, Matrix4 &Heading, bool invert)
{
	Matrix4 L;
	L.identity();

	Vector4 xwaLight = Vector4(
		s_XwaGlobalLights[idx].PositionX / 32768.0f,
		s_XwaGlobalLights[idx].PositionY / 32768.0f,
		s_XwaGlobalLights[idx].PositionZ / 32768.0f,
		0.0f);

	xwaLight = Heading * xwaLight;
	xwaLight.normalize();
	//log_debug("[DBG] [SHW] xwaLight: %0.3f, %0.3f, %0.3f", xwaLight.x, xwaLight.y, xwaLight.z);

	// Rotate the light vector so that it lies in the X-Z plane
	//Vector4 temp;
	Matrix4 rotX, rotY, rot;
	float pitch = atan2(xwaLight.y, xwaLight.z);
	rotX.rotateX(pitch / DEG2RAD);
	//temp = rotX * xwaLight;
	// temp lies now in the X-Z plane, so Y is always zero:
	//log_debug("[DBG] [SHW] temp: %0.3f, %0.3f, %0.3f", temp.x, temp.y, temp.z);

	// Rotate the vector around the Y axis so that x --> 0
	rotY.rotateY(-asin(xwaLight.x) / DEG2RAD);
	//temp = rotY * temp;
	rot = rotY * rotX;
	//temp = rot * xwaLight;
	// x and y should now be 0, with z = 1 all the time:
	//log_debug("[DBG] [SHW] temp: %0.3f, %0.3f, %0.3f", temp.x, temp.y, temp.z);

	// It is now easy to build a TBN matrix:
	Vector4 R(1.0f, 0.0f, 0.0f, 0.0f);
	Vector4 U(0.0f, 1.0f, 0.0f, 0.0f);
	Vector4 F(0.0f, 0.0f, -1.0f, 0.0f); // We use -1 here so that the light points towards the origin

	// Invert the rotation chain:
	rot.transpose();
	// Invert the TBN system
	R = rot * R;
	U = rot * U;
	F = rot * F;
	// Build a TBN matrix in ViewSpace
	if (!invert)
		L.set(
			R.x, U.x, F.x, 0,
			R.y, U.y, F.y, 0,
			R.z, U.z, F.z, 0,
			0, 0, 0, 1
		);
	else
		L.set(
			R.x, R.y, R.z, 0,
			U.x, U.y, U.z, 0,
			F.x, F.y, F.z, 0,
			0, 0, 0, 1
		);

	return L;
}

void IncreaseSMZFactor(float Delta) {
	g_ShadowMapVSCBuffer.sm_z_factor += Delta;
	log_debug("[DBG] [SHW] sm_z_factor: %0.3f, FOVscale: %0.3f, FOVDist: %0.3f",
		g_ShadowMapVSCBuffer.sm_z_factor, g_ShadowMapVSCBuffer.sm_FOVscale, *g_fRawFOVDist);
}

/*
 * WE CANNOT DELETE THIS FUNCTION YET.
 * Tagging using the FlightGroup info does not work during film playback, so we need
 * to use this version for that case.
 *
 * For each Sun Centroid stored in the previous frame, check if they match any of
 * XWA's lights. If there's a match, we have found the global sun and we can stop
 * computing shadows from the other lights.
 * For each light that doesn't match, if the light isn't near the edge of the screen,
 * then we know that this light doesn't correspond to a sun, so we can stop computing
 * shadows for that light.
 */
void PrimarySurface::OldTagXWALights()
{
	int NumTagged = 0;
	// Get the screen limits, we'll need them to tell when a light is visible on the screen
	float x0, y0, x1, y1;
	bool bExternal = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;
	// Don't bother tagging lights if we're parked in the hangar.
	// Don't tag anything in external view if the y_center hasn't been fixed
	// A few lines below, we use projectMetric() and that uses y_center.
	//if (*g_playerInHangar || (bExternal && !g_bYCenterHasBeenFixed))
	if (*g_playerInHangar || bExternal)
		return;
#undef RT_SIDE_LIGHTS
#ifdef RT_SIDE_LIGHTS
	return;
#endif
	GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1, true);

	// Check all the lights to see if they match any sun centroid
	for (int LightIdx = 0; LightIdx < *s_XwaGlobalLightsCount; LightIdx++)
	{
		// Skip lights that have been tagged. If we have tagged everything, then
		// we can stop tagging lights in future frames
		if (g_XWALightInfo[LightIdx].bTagged) {
			NumTagged++;
			if (NumTagged >= *s_XwaGlobalLightsCount) {
				log_debug("[DBG] [SHW] All lights have been tagged. Will set bAllLightsTagged to stop tagging lights");
				g_ShadowMapping.bAllLightsTagged = true;
				return;
			}
			continue;
		}

		Vector4 xwaLight = Vector4(
			s_XwaGlobalLights[LightIdx].PositionX / 32768.0f,
			s_XwaGlobalLights[LightIdx].PositionY / 32768.0f,
			s_XwaGlobalLights[LightIdx].PositionZ / 32768.0f,
			0.0f);
		// Convert the XWA light direction into viewspace coordinates:
		Vector4 light = g_CurrentHeadingViewMatrix * xwaLight;

		// Only test lights in front of the camera:
		if (light.z <= 0.0f)
			continue;

		// Project this light to screen coords to see if it's visible
		Vector3 L(light.x, light.y, light.z);
		L *= 65536.0f;
		Vector3 p = projectMetric(L, g_viewMatrix, g_FullProjMatrixLeft, true);
		// p is now in post-proc UV coords. If it's within (x0,y0)-(x1,y1) then this
		// light is visible on the screen
		/*if (p.x < x0 + 0.05f || p.x > x1 - 0.05f ||
			p.y < y0 + 0.05f || p.y > y1 - 0.05f)
			continue;*/
		bool Visible = x0 + 0.05f < p.x && p.x < x1 - 0.05f && 
			           y0 + 0.05f < p.y && p.y < y1 - 0.05f;
		if (!Visible)
			continue;

		// If we reach this point, it means this light is visible on the screen
		for (int CentroidIdx = 0; CentroidIdx < g_iNumSunCentroids; CentroidIdx++)
		{
			float X, Y, Z;
			Vector3 c, SunCentroid = g_SunCentroids[CentroidIdx];

			// Compensate point...
			X = SunCentroid.x;
			//Y = SunCentroid.y + bExternal ? 0.0f : g_ShadertoyBuffer.y_center;
			Y = SunCentroid.y + g_ShadertoyBuffer.y_center;
			Z = SunCentroid.z;

			c.set(X, Y, Z);
			c.normalize();
			light.normalize();
			// Here we're taking the dot product between the light's direction and the compensated
			// centroid. Note that a centroid c = [0,0,0] is directly in the middle of the screen.
			// So the vertical FOV should help us predict if the compensated centroid is visible
			// in the screen or not.
			float dot = c.x*light.x + c.y*light.y + c.z*light.z;
			/*log_debug("[DBG] [SHW] sun vector: [%0.3f, %0.3f, %0.3f], Centroid: [%0.3f, %0.3f, %0.3f]",
				c.x, c.y, c.z, X, Y, Z);
			log_debug("[DBG] [SHW] light: [%0.3f, %0.3f, %0.3f], dot: %0.3f, SUN: %d",
				light.x, light.y, light.z, dot, dot > 0.98f);*/

			if (dot > 0.98f)
			{
				// Associate an XWA light to this texture and stop checking
				log_debug("[DBG] [SHW] Sun Found: Light %d, dot: %0.3f", LightIdx, dot);
				log_debug("[DBG] [SHW] centr: [%0.3f, %0.3f, %0.3f], light: [%0.3f, %0.3f, %0.3f]",
					c.x, c.y, c.z, light.x, light.y, light.z);

				if (g_ShadowMapping.bMultipleSuns) 
				{
					// There may be missions with multiple suns, so, let's tag this one as a sun
					// but let's keep tagging.
					g_XWALightInfo[LightIdx].bIsSun = true;
					g_XWALightInfo[LightIdx].bTagged = true;
					break;
				}
				else {
					// At this point, I probably only want one sun casting shadows. So, as soon as we find our sun, we
					// can stop tagging and shut down all the other lights as shadow casters.
					for (int j = 0; j < *s_XwaGlobalLightsCount; j++)
					{
						g_XWALightInfo[j].bIsSun = (j == LightIdx); // LightIdx is the current light being tagged
						g_XWALightInfo[j].bTagged = true;
					}
					g_ShadowMapping.bAllLightsTagged = true; // We found the global sun, stop tagging lights
					return;
				}

			}
		}

		// If we reach this point, the light is visible and it has been tested against all
		// centroids. If the light was not tagged, then it's not a sun
		if (!g_XWALightInfo[LightIdx].bTagged && NumTagged < *s_XwaGlobalLightsCount - 1) // Keep at least one light as shadow caster
		{
			// If we reach this point, then the light hasn't been tagged, it's clearly visible 
			// on the screen and it's not a sun
			g_XWALightInfo[LightIdx].bTagged = true;
			g_XWALightInfo[LightIdx].bIsSun = false;
			log_debug("[DBG] [SHW] Light: %d is *NOT* a Sun", LightIdx);
			log_debug("[DBG] [SHW] light: %0.3f, %0.3f, %0.3f", light.x, light.y, light.z);
			log_debug("[DBG] [SHW] p: %0.3f, %0.3f", p.x, p.y);
			log_debug("[DBG] [SHW] limits: (%0.3f, %0.3f)-(%0.3f, %0.3f)", x0, y0, x1, y1);
			log_debug("[DBG] [SHW] g_bCustomFOVApplied: %d, Frame: %d, VR mode: %d",
				g_bCustomFOVApplied, g_iPresentCounter, g_bEnableVR);
			ShowMatrix4(g_CurrentHeadingViewMatrix, "g_CurrentHeadingViewMatrix");
		}
	} // for LightIdx
}

/*
 * Tag the lights by looking at the Flight Groups and checking the GroupId-ImageId.
 */
void PrimarySurface::TagXWALights()
{
	int numSuns = 0;
	const int numLights = *s_XwaGlobalLightsCount;
	// When the player is in the hangar, the current region is maxRegion + 1 (apparently).
	// So, in that case, we're defaulting to the first region instead. I'm not sure that this
	// is correct.
	const int curRegion = *g_playerInHangar ? 0 : PlayerDataTable[*g_playerIndex].currentRegion;

	constexpr int MAX_FLIGHT_GROUPS = 192;
	constexpr int MAX_PLANET_IDS    = 104;
	constexpr int MAX_MODEL_IDX     = 557;
	constexpr int CraftId_183_9001_1100_ResData_Backdrop = 183;
	const XwaMission* mission = *(XwaMission**)0x09EB8E0;

	log_debug("[DBG] [FG] ------------------------------");
	log_debug("[DBG] [FG] Tagging Lights. curRegion: %d", curRegion);

	// Clear all previous tags so that we can start from scratch.
	for (int i = 0; i < numLights; i++)
	{
		g_XWALightInfo[i].bIsSun = false;
		g_XWALightInfo[i].bTagged = false;
	}

	// From:
	// https://github.com/JeremyAnsel/xwa_hooks/blob/master/xwa_hook_backdrops/hook_backdrops/backdrops.cpp
	// Starfield Zenith (Up) is Z+ (StarPoints[0].Z = 1)
	// GlobalCargoIndex is the same as shadow (for instance 5)
	// Cargo = "0.0" (string)
	// SpecialCargo = "0.94" (string). This is the scale.

	// Nadir is Z-, all other fields are equal to the Zenith
	// North (Fwd) is Y-, SpecialCargo is "1.053"

	int maxRegion = 0;
	for (int FGIdx = 0; FGIdx < MAX_FLIGHT_GROUPS; FGIdx++)
	{
		if (g_ShadowMapping.bAllLightsTagged)
			break;

		const int CraftId  = mission->FlightGroups[FGIdx].CraftId;
		const int PlanetId = mission->FlightGroups[FGIdx].PlanetId;

		if (CraftId == CraftId_183_9001_1100_ResData_Backdrop && PlanetId < MAX_PLANET_IDS)
		{
			const short SX     = mission->FlightGroups[FGIdx].StartPoints->X;
			const short SY     = mission->FlightGroups[FGIdx].StartPoints->Y;
			const short SZ     = mission->FlightGroups[FGIdx].StartPoints->Z;
			const int   region = mission->FlightGroups[FGIdx].StartPointRegions[0];

			maxRegion = max(maxRegion, region);
			// By looking here:
			// https://github.com/MikeG621/Platform/blob/master/Xwa/FlightGroup.cs
			// and Yogeme, it turns out that GlobalCargo is mapped to the "shadow" field
			// when this is a backdrop -- and sometimes this field can be negative, hence
			// the use of max(0, X) below:
			int shadow = (int)mission->FlightGroups[FGIdx].GlobalCargoIndex;
			const char* spCargo = mission->FlightGroups[FGIdx].SpecialCargo;
			if (shadow == 255) shadow = 0;

			// The scale of the backdrops is pre-determined. The first two backdrops are
			// scale = 0.895 (and it appears that "shadow" is always 5 in this case).
			// The rest of the backdrops are scale 1.055 and "shadow" can be any number
			// See Yogeme, XwaForm.cs, search for "0.895"

			const int ModelIndex = g_XwaPlanets[PlanetId].ModelIndex;
			const uint8_t BackdropFlags = g_XwaPlanets[PlanetId].BackdropFlags;

			/*log_debug("[DBG] [FG] FGIdx: %d, PlanetId: %d-%d, region: %d, ModelIndex: %d",
				FGIdx, PlanetId, shadow, region, ModelIndex);*/
			if (region == curRegion && ModelIndex < MAX_MODEL_IDX)
			{
				const int GroupId = g_ExeObjectsTable[ModelIndex].DataIndex1;
				const int ImageId = g_ExeObjectsTable[ModelIndex].DataIndex2;

				Vector3 S = Vector3((float)SX, (float)-SY, (float)SZ);
				S = S.normalize();

				// These are the GroupIds that correspond to suns:
				if (9001 <= GroupId && GroupId <= 9010)
				{
					log_debug("[DBG] [FG] [%s], CraftId: %d, PlanetId: %d, S:[%0.3f, %0.3f, %0.3f]",
						mission->FlightGroups[FGIdx].Name, CraftId, PlanetId, S.x, S.y, S.z);
					log_debug("[DBG] [FG]    region: %d, curRegion: %d, Group-Id: %d-%d",
						region, curRegion, GroupId, ImageId);

					// Now check the lights in this region to find a match
					for (int LightIdx = 0; LightIdx < numLights; LightIdx++)
					{
						if (!g_XWALightInfo[LightIdx].bTagged)
						{
							Vector3 L = Vector3(
								s_XwaGlobalLights[LightIdx].PositionX / 32768.0f,
								s_XwaGlobalLights[LightIdx].PositionY / 32768.0f,
								s_XwaGlobalLights[LightIdx].PositionZ / 32768.0f);
							L = L.normalize();
							float dot = L.dot(S);

							log_debug("[DBG] [FG] light: %d: [%0.3f, %0.3f, %0.3f], dot: %0.3f, %s",
								LightIdx, L.x, L.y, L.z, dot, (dot > 0.975f) ? "SUN" : "");
							if (dot > 0.975f)
							{
								// There may be missions with multiple suns, so, let's tag this one as a sun
								// but let's keep tagging.
								g_XWALightInfo[LightIdx].bIsSun = true;
								g_XWALightInfo[LightIdx].bTagged = true;
								numSuns++;
							}
						}
					}
				}
				// Starfields tend to have ModelIndex == 0
				else if (ModelIndex == 0 && GroupId < 0)
				{
					Vector3 S = Vector3((float)SX, (float)SY, (float)SZ);
					S.normalize();
					log_debug("[DBG] [FG] Backdrop. region: %d, PlanetId: %d-%d, S:[%0.3f, %0.3f, %0.3f], scale: %s",
						region, PlanetId, shadow, S.x, S.y, S.z, spCargo);

					// Let's see if this backdrop is contained in Planet2.dat:
					const auto& it = g_BackdropIdToGroupId.find(PlanetId);
					if (it != g_BackdropIdToGroupId.end())
					{
						// This backdrop can be a planet, nebula or starfield
						int groupId = it->second;
						int key = MakeKeyFromGroupIdImageId(groupId, shadow);
						// Check if this backdrop is a starfield
						if (g_StarfieldGroupIdImageIdMap.find(key) != g_StarfieldGroupIdImageIdMap.end())
						{
							log_debug("[DBG] [FG]    STARFIELD --> GroupId-ImageId: %d-%d", groupId, shadow);
							const auto& it = g_GroupIdImageIdToTextureMap.find(key);
							if (it != g_GroupIdImageIdToTextureMap.end())
							{
								Direct3DTexture* tex = (Direct3DTexture*)it->second;
								log_debug("[DBG] [FG]        Corresponding Texture Found: 0x%x, 0x%x",
									tex, tex->_textureView.Get());
							}
							else
								log_debug("[DBG] [FG]        NO Corresponding Texture Found!!!");
						}
						else
						{
							// This backdrop is a planet
							log_debug("[DBG] [FG]    PLANET/NEBULA --> GroupId-ImageId: %d-%d", groupId, shadow);
						}
					}
					else
					{
						log_debug("[DBG] [FG]    UNCLASSIFIED");
					}
				}
				else
				{
					// NEBULA.DAT
					// 7001-100  --> 64-0
					// 7002-200  --> 65-0
					// ...
					// 7011-1100 --> 73-0

					// GALAXY.DAT
					// 8001-100  --> 74-0
					// ...
					// 8007-700  --> 80-0
					// ...
					// 8010-1000 --> 83-0
					log_debug("[DBG] [FG] UNK: GroupId-ImageId: %d-%d, PlanetId: %d",
						GroupId, ImageId, PlanetId);
				}
			}
		}
	}
	// The hangar appears to be maxRegion + 1, which means there's never an entry
	// for it in the FGs.
	log_debug("[DBG] [FG] maxRegion: %d", maxRegion);

	// Finish tagging all the other lights
	for (int i = 0; i < numLights; i++)
	{
		g_XWALightInfo[i].bTagged = true;
	}
	g_ShadowMapping.bAllLightsTagged = true;

	// Disable multiple suns if necessary
	if (!g_ShadowMapping.bMultipleSuns && numSuns > 1)
	{
		for (int i = numLights - 1; i >= 0; i--)
		{
			if (g_XWALightInfo[i].bIsSun)
			{
				g_XWALightInfo[i].bIsSun = false;
				numSuns--;
				if (numSuns == 1)
					break;
			}
		}
	}

	// Check how many suns we have left after all the tagging and leave at least one
	numSuns = 0;
	for (int i = 0; i < numLights; i++)
	{
		if (g_XWALightInfo[i].bIsSun)
		{
			log_debug("[DBG] [FG] Light: %d is a SUN", i);
			numSuns++;
		}
	}

	if (numSuns == 0)
	{
		log_debug("[DBG] [FG] WARNING: No suns after tagging! Enabling first light");
		g_XWALightInfo[0].bIsSun = true;
	}
	log_debug("[DBG] [FG] ------------------------------");
}

/*
 * Calls TagXWALights and then fades the lights if necessary.
 */
void PrimarySurface::TagAndFadeXWALights()
{
	auto &resources = this->_deviceResources;
	auto &device = resources->_d3dDevice;
	auto &context = resources->_d3dDeviceContext;
	Matrix4 HeadingMatrix, CockpitMatrix;
	Matrix4 T, Ry, Rx, S;

	// Tag the XWA global lights unless everything has been tagged
	// Looks like the very first frame cannot be used to tag anything: the lights are probably not in
	// the right places and the FOV hasn't been computed, so let's wait until the FOV has been computed
	// to tag anything and we've finished rendering a few frames
	if (g_bCustomFOVApplied && !g_ShadowMapping.bAllLightsTagged && g_iPresentCounter > 5)
	{
		if (!(*viewingFilmState))
			TagXWALights();
		else
		{
			// When viewing a film, the lights are apparently not in the right position (!)
			OldTagXWALights();
		}
	}

	// Display debug information on g_XWALightInfo (are the lights tagged, are they suns?)
	if (g_bDumpSSAOBuffers) {
		log_debug("[DBG] [SHW] RealVFOV: %0.3f, RealHFOV: %0.3f", g_fRealVertFOV / DEG2RAD, g_fRealHorzFOV / DEG2RAD);
		for (int j = 0; j < *s_XwaGlobalLightsCount; j++)
		{
			log_debug("[DBG] [SHW] light[%d], bIsSun: %d, bTagged: %d, black_level: %0.3f",
				j, g_XWALightInfo[j].bIsSun, g_XWALightInfo[j].bTagged, g_ShadowMapVSCBuffer.sm_black_levels[j]);
		}
	}

	// Turn lights on or off
	for (int j = 0; j < *s_XwaGlobalLightsCount; j++) 
	{
		// Instantaneous:
		//g_ShadowMapVSCBuffer.sm_black_levels[j] = g_XWALightInfo[j].bIsSun ? g_ShadowMapping.black_level : 1.0f;

		// Soft transition:
		if (g_ShadowMapVSCBuffer.sm_black_levels[j] >= 0.95f)
			continue;

		// If this light has been tagged and isn't a sun, then fade it!
		if (g_XWALightInfo[j].bTagged && !g_XWALightInfo[j].bIsSun) {
			//g_XWALightInfo[j].fadeout += 0.01f;
			g_ShadowMapVSCBuffer.sm_black_levels[j] += 0.02f;
			if (g_ShadowMapVSCBuffer.sm_black_levels[j] > 0.95f)
				log_debug("[DBG] [SHW] Light %d FADED", j);
		}
	}

	// We do not render the shadowmap in this path if the D3dRendererHook is enabled. See
	// XwaD3dRendererHook:RenderShadowMap()
}

/*
 * In VR mode, the centroid of the sun texture is a new vertex in metric
 * 3D space. We need to project it to the vertex buffer that is used to
 * render the flare (the hyperspace vertex buffer, so it's placed at
 * ~22km away). This projection yields a uv-coordinate in this distant
 * vertex buffer. Then we need to project this distant point to post-proc
 * screen coordinates. The screen-space coords (u,v) have (0,0) at the
 * center of the screen. The output u is further multiplied by the aspect
 * ratio to put the flare exactly on top of the sun.
 * This function projects the 3D centroid to post-proc coords.
 */
void PrimarySurface::ProjectCentroidToPostProc(Vector3 Centroid, float *u, float *v) {
	/*
	 * The following circus happens because the vertex buffer we're using to render the flare
	 * is at ~22km away. We want it there, so that it's at "infinity". So we need to trace a ray
	 * from the camera through the Centroid and then extend that until it intersects the vertex
	 * buffer at that depth. Then we compute the UV coords of the intersection and use that for
	 * the shader. This should avoid visual artifacts in SteamVR because we're using an actual 3D
	 * canvas (the vertex buffer at ~21km) that gets transformed with the regular projection matrices.
	 */
	float U0, U1, U2;
	float V0, V1, V2;
	Vector3 v0, v1, v2, P, orig = Vector3(0.0f, 0.0f, 0.0f), dir;
	float t, tu, tv;
	// The depth of the hyperspace buffer is: 
	float hb_rhw_depth = 0.000863f;
	//g_ShadertoyBuffer.iResolution[1] *= g_fAspectRatio;

	/*
	// Back-project to the depth of the vertex buffer we're using here:
	backProject(QL.x, QL.y, hb_rhw_depth, &QL);
	backProject(QR.x, QR.y, hb_rhw_depth, &QR);
	log_debug("[DBG] Centroid: %0.3f, %0.3f, %0.3f --> Q: %0.3f, %0.3f, %0.3f",
		Centroid.x, Centroid.y, Centroid.z, QL.x, QL.y, QL.z);
	*/

	// Convert the centroid into a direction coming from the origin:
	dir = Centroid;
	dir.normalize();

	// Test the first triangle that makes up the hyperspace VB for an intersection:
	backProjectMetric(0.0f, 0.0f, hb_rhw_depth, &v0);
	backProjectMetric(g_fCurInGameWidth, 0.0f, hb_rhw_depth, &v1);
	backProjectMetric(0.0f, g_fCurInGameHeight, hb_rhw_depth, &v2);
	if (rayTriangleIntersect(orig, dir, v0, v1, v2, t, P, tu, tv, 0)) {
		U0 = 0.0f; U1 = 1.0f; U2 = 0.0f;
		V0 = 0.0f; V1 = 0.0f; V2 = 1.0f;
	}
	else 
	{
		// Test the second triangle that makes up the hyperspace VB for an intersection:
		backProjectMetric(g_fCurInGameWidth, 0.0f, hb_rhw_depth, &v0);
		backProjectMetric(g_fCurInGameWidth, g_fCurInGameHeight, hb_rhw_depth, &v1);
		backProjectMetric(0.0f, g_fCurInGameHeight, hb_rhw_depth, &v2);
		rayTriangleIntersect(orig, dir, v0, v1, v2, t, P, tu, tv, 0);
		U0 = 1.0f; U1 = 1.0f; U2 = 0.0f;
		V0 = 0.0f; V1 = 1.0f; V2 = 1.0f;
	}
	// Interpolate the texture UV using the barycentric (tu, tv) coords:
	*u = tu * U0 + tv * U1 + (1.0f - tu - tv) * U2;
	*v = tu * V0 + tv * V1 + (1.0f - tu - tv) * V2;
	// Change the range to -1..1:
	*u = 2.0f * *u - 1.0f;
	*v = 2.0f * *v - 1.0f;
	// Apply the aspect ratio
	*u *= g_fCurScreenWidth / g_fCurScreenHeight;
}

void PrimarySurface::RenderSunFlare()
{
	this->_deviceResources->BeginAnnotatedEvent(L"RenderSunFlare");
	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	bool bDirectSBS = (g_bEnableVR && !g_bUseSteamVR);
	float x0, y0, x1, y1;
	D3D11_VIEWPORT viewport;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	static float iTime = 0.0f;
	Vector3 Centroid, QL[MAX_SUN_FLARES], QR[MAX_SUN_FLARES];
	Vector2 Q[MAX_SUN_FLARES];
	const bool bExternalView = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;

	iTime += 0.01f;
	GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
	g_ShadertoyBuffer.x0 = x0;
	g_ShadertoyBuffer.y0 = y0;
	g_ShadertoyBuffer.x1 = x1;
	g_ShadertoyBuffer.y1 = y1;
	g_ShadertoyBuffer.iTime = iTime;
	g_ShadertoyBuffer.y_center = bExternalView ? 0.0f : g_fYCenter;
	g_ShadertoyBuffer.VRmode = g_bEnableVR;
	g_ShadertoyBuffer.iResolution[0] = g_fCurScreenWidth;
	g_ShadertoyBuffer.iResolution[1] = g_fCurScreenHeight;
	// Apply the sun colors stored during previous draw calls
	for (int i = 0; i < g_iSunFlareCount; i++)
		g_ShadertoyBuffer.SunColor[i] = g_SunColors[i];
	g_ShadertoyBuffer.SunFlareCount = g_iSunFlareCount;
	// g_ShadertoyBuffer.FOVscale must be set! We'll need it for this shader
	if (g_bEnableVR) {
		float u, v;
		if (bDirectSBS)
			g_ShadertoyBuffer.iResolution[0] = g_fCurScreenWidth / 2.0f;

		// This is a first step towards having multiple flares; but more work is needed
		// because flares are blocked depending on stuff that lies in front of them, so
		// we can't render all flares on the same go. It has to be done one by one, eye
		// by eye. So, at this point, even though we have an array of flares, it's all
		// hard-coded to only render flare 0 in the shaders.
		// Project all flares to post-proc coords in the hyperspace vertex buffer:
		for (int i = 0; i < g_ShadertoyBuffer.SunFlareCount; i++) {
			Centroid.x = g_ShadertoyBuffer.SunCoords[i].x;
			Centroid.y = g_ShadertoyBuffer.SunCoords[i].y;
			Centroid.z = g_ShadertoyBuffer.SunCoords[i].z;
			// To keep TagXwaLights() working in non-VR space even when VR is enabled,
			// the centroid is always stored in the nonVR coord sys. For VR, we need to 
			// flip the Y coord:
			if (g_bEnableVR) Centroid.y = -Centroid.y;
			/*
			ProjectCentroidToPostProc(Centroid, &u, &v);
			*/
			
			// The Sun Centroid is in in-game coords, convert to the range [-1..1]:
			u = (g_SunCentroids2D[i].x / g_fCurInGameWidth - 0.5f) * 2.0f;
			v = (g_SunCentroids2D[i].y / g_fCurInGameHeight - 0.5f) * 2.0f;
			if (bDirectSBS) {
				v *= g_fCurScreenHeight / (g_fCurScreenWidth / 2.0f);
			}
			else {
				// SteamVR mode
				if (g_steamVRWidth < g_steamVRHeight)
					v *= (float)g_steamVRHeight / (float)g_steamVRWidth;
				else
					u *= (float)g_steamVRWidth / (float)g_steamVRHeight;
			}

			// Overwrite the centroid with the new 2D coordinates for the left/right images.
			// In VR mode, these UVs are *the same* for both eyes because they are defined
			// with respect to distant the hyperspace VB. This is *not* a bug.
			g_ShadertoyBuffer.SunCoords[i].x = u;
			g_ShadertoyBuffer.SunCoords[i].y = v;

			// Also project the centroid to 2D directly -- we'll need that during the compose pass
			// to mask the flare. In VR mode, projectToInGameCoordsMetric produces post-proc
			// coords, not in-game coords
			QL[i] = projectToInGameOrPostProcCoordsMetric(Centroid, g_viewMatrix, g_FullProjMatrixLeft);
			QR[i] = projectToInGameOrPostProcCoordsMetric(Centroid, g_viewMatrix, g_FullProjMatrixRight);
			//log_debug("[DBG] QL: %0.3f, %0.3f", QL[i].x, QL[i].y);
		}
	}
	// We need this to ensure backface culling is disabled
	resources->InitRasterizerState(resources->_rasterizerState);
	// Set the shadertoy constant buffer:
	resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
	resources->InitPixelShader(g_bUseSteamVR ? resources->_sunFlareShaderPS_VR : resources->_sunFlareShaderPS);

	context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
	if (g_bUseSteamVR)
		context->ResolveSubresource(
			resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
			resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);

#ifdef GENMIPMAPS_DEBUG
	// DEBUG
	// An easy way to check if the mip maps are being generated is by calling GenerateMipMaps here and then sampling
	// a higher level in the sun flare shader.
	context->GenerateMips(resources->_offscreenAsInputShaderResourceView);
#endif

	// Render the Sun Flare
	// output: _offscreenBufferPost, _offscreenBufferPostR
	// The non-VR case produces a fully-finished image.
	// The VR case produces only the flare on these buffers. The flare must be composed on top of the image in a further step
	// The VR flare is rendered on a distant VB buffer. The uv coords in this buffer are the same regardless of the
	// view because their frame of reference is the same VB buffer
	{
		// Set the new viewport (a full quad covering the full screen)
		viewport.Width  = g_fCurScreenWidth;
		viewport.Height = g_fCurScreenHeight;
		// VIEWPORT-LEFT
		if (g_bEnableVR) {
			if (g_bUseSteamVR)
				viewport.Width = (float)resources->_backbufferWidth;
			else
				viewport.Width = (float)resources->_backbufferWidth / 2.0f;
		}
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		resources->InitViewport(&viewport);

		// We don't need to clear the current vertex and pixel constant buffers.
		// Since we've just finished rendering 3D, they should contain values that
		// can be reused. So let's just overwrite the values that we need.
		g_VSCBuffer.aspect_ratio      =  g_fAspectRatio;
		g_VSCBuffer.z_override        = -1.0f;
		g_VSCBuffer.sz_override       = -1.0f;
		g_VSCBuffer.mult_z_override   = -1.0f;
		g_VSCBuffer.apply_uv_comp     =  false;
		g_VSCBuffer.bPreventTransform =  0.0f;
		g_VSCBuffer.bFullTransform    =  0.0f;
		if (g_bEnableVR)
		{
			g_VSCBuffer.viewportScale[0] = 1.0f / resources->_displayWidth;
			g_VSCBuffer.viewportScale[1] = 1.0f / resources->_displayHeight;
		}
		else
		{
			g_VSCBuffer.viewportScale[0] =  2.0f / resources->_displayWidth;
			g_VSCBuffer.viewportScale[1] = -2.0f / resources->_displayHeight;
		}

		// Since the HUD is all rendered on a flat surface, we lose the vrparams that make the 3D object
		// and text float
		g_VSCBuffer.z_override = 65535.0f;
		g_VSCBuffer.scale_override = 1.0f;

		// Set the left projection matrix (the viewMatrix is set at the beginning of the frame)
		g_VSMatrixCB.projEye[0] = g_FullProjMatrixLeft;
		g_VSMatrixCB.projEye[1] = g_FullProjMatrixRight;
		resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
		resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

		UINT stride = sizeof(D3DTLVERTEX), offset = 0;
		resources->InitVertexBuffer(resources->_hyperspaceVertexBuffer.GetAddressOf(), &stride, &offset);
		resources->InitInputLayout(resources->_inputLayout);
		if (g_bEnableVR)
			resources->InitVertexShader(resources->_sbsVertexShader); // if (g_bEnableVR)
		else
			resources->InitVertexShader(resources->_vertexShader);
		resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		// Set the RTV:
		ID3D11RenderTargetView *rtvs[1] = {
			resources->_renderTargetViewPost.Get(), // Render to offscreenBufferPost instead of offscreenBuffer
		};
		context->OMSetRenderTargets(1, rtvs, NULL);
		// Set the SRVs:
		ID3D11ShaderResourceView *srvs[2] = {
			resources->_offscreenAsInputShaderResourceView.Get(),
			resources->_depthBufSRV.Get(),
		};
		context->PSSetShaderResources(0, 2, srvs);
		if (g_bUseSteamVR)
			context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(6, 0);

		// Render the right image
		if (g_bEnableVR && !g_bUseSteamVR)
		{
			// VIEWPORT-RIGHT
			viewport.Width = (float)resources->_backbufferWidth / 2.0f;
			viewport.TopLeftX = (float)viewport.Width;
			viewport.Height = (float)resources->_backbufferHeight;
			viewport.TopLeftY = 0.0f;
			viewport.MinDepth = D3D11_MIN_DEPTH;
			viewport.MaxDepth = D3D11_MAX_DEPTH;
			resources->InitViewport(&viewport);
			// Set the right projection matrix
			g_VSMatrixCB.projEye[0] = g_FullProjMatrixRight;
			resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

			// DirectSBS case
			context->OMSetRenderTargets(1, resources->_renderTargetViewPost.GetAddressOf(), NULL);
			// Set the SRVs:
			ID3D11ShaderResourceView *srvs[2] = {
				resources->_offscreenAsInputShaderResourceView.Get(),
				resources->_depthBufSRV.Get(),
			};
			context->PSSetShaderResources(0, 2, srvs);
			context->Draw(6, 0);
		}
	}
	
	if (g_bDumpSSAOBuffers) {
		DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferPost, GUID_ContainerFormatPng, L"C:\\Temp\\_sunFlare.png");
		if (g_bUseSteamVR)
			DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferPostR, GUID_ContainerFormatPng, L"C:\\Temp\\_sunFlareR.png");
	}

	// Post-process: compose the flare on top of the offscreen buffers
	if (g_bEnableVR)
	{
		// Reset the viewport for non-VR mode:
		viewport.TopLeftX	= 0.0f;
		viewport.TopLeftY	= 0.0f;
		viewport.Width		= g_fCurScreenWidth;
		viewport.Height		= g_fCurScreenHeight;
		viewport.MaxDepth	= D3D11_MAX_DEPTH;
		viewport.MinDepth	= D3D11_MIN_DEPTH;
		resources->InitViewport(&viewport);

		// Reset the vertex shader to regular 2D post-process
		// Set the Vertex Shader Constant buffers
		resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
			0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices

		// Set/Create the VertexBuffer and set the topology, etc
		UINT stride = sizeof(MainVertex), offset = 0;
		resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);
		resources->InitInputLayout(resources->_mainInputLayout);
		resources->InitVertexShader(g_bUseSteamVR ? resources->_mainVertexShaderVR : resources->_mainVertexShader);

		// Reset the UV limits for this shader
		GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
		g_ShadertoyBuffer.x0 = x0;
		g_ShadertoyBuffer.y0 = y0;
		g_ShadertoyBuffer.x1 = x1;
		g_ShadertoyBuffer.y1 = y1;
		g_ShadertoyBuffer.VRmode = bDirectSBS;
		g_ShadertoyBuffer.iResolution[0] = g_fCurScreenWidth;
		g_ShadertoyBuffer.iResolution[1] = g_fCurScreenHeight;
		// Project the centroid to the left/right eyes
		for (int i = 0; i < g_ShadertoyBuffer.SunFlareCount; i++) {
			//QL = projectToInGameCoords(Centroid, g_viewMatrix, g_FullProjMatrixLeft);
			g_ShadertoyBuffer.SunCoords[i].x = QL[i].x;
			g_ShadertoyBuffer.SunCoords[i].y = QL[i].y;
		}
		resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
		resources->InitPixelShader(g_bUseSteamVR ? resources->_sunFlareComposeShaderPS_VR : resources->_sunFlareComposeShaderPS);

		// The output from the previous effect will be in offscreenBufferPost, so let's resolve it
		// to _shaderToyBuf to re-use in the next step:
		context->ResolveSubresource(resources->_shadertoyBuf, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);
		if (g_bUseSteamVR) {
			context->ResolveSubresource(
				resources->_shadertoyBuf, D3D11CalcSubresource(0, 1, 1),
				resources->_offscreenBufferPost, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
		}
		context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		
		ID3D11RenderTargetView *rtvs[1] = {
			resources->_renderTargetViewPost.Get(), // Render to offscreenBufferPost instead of offscreenBuffer
		};
		context->OMSetRenderTargets(1, rtvs, NULL);
		// Set the SRVs:
		ID3D11ShaderResourceView *srvs[3] = {
			resources->_offscreenAsInputShaderResourceView.Get(),
			resources->_shadertoySRV.Get(),
			resources->_depthBufSRV.Get(),
		};
		context->PSSetShaderResources(0, 3, srvs);
		if (g_bUseSteamVR)
			context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(6, 0);

		// Post-process the right image
		if (g_bUseSteamVR) {
			//QR = projectToInGameCoords(Centroid, g_viewMatrix, g_FullProjMatrixRight);
			for (int i = 0; i < g_ShadertoyBuffer.SunFlareCount; i++) {
				g_ShadertoyBuffer.SunCoords[i].x = QR[i].x;
				g_ShadertoyBuffer.SunCoords[i].y = QR[i].y;
			}
			resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
			ID3D11RenderTargetView *rtvs[1] = {
				resources->_renderTargetViewPostR.Get(),
			};
			context->OMSetRenderTargets(1, rtvs, NULL);
			// Set the SRVs:
			ID3D11ShaderResourceView *srvs[3] = {
				resources->_offscreenAsInputShaderResourceViewR.Get(),
				resources->_shadertoySRV_R.Get(),
				resources->_depthBufSRV_R.Get(),
			};
			context->PSSetShaderResources(0, 3, srvs);
			context->Draw(6, 0);
		}
	}

	// Copy the result (_offscreenBufferPost) to the _offscreenBuffer so that it gets displayed
	context->CopyResource(resources->_offscreenBuffer, resources->_offscreenBufferPost);

	// Restore previous rendertarget, etc
	resources->InitInputLayout(resources->_inputLayout); // Not sure this is really needed

	this->_deviceResources->EndAnnotatedEvent();
}

// ****************************************************************************
// This section is the CPU version of the raytracing traversal code in
// RTCommon.h
// ****************************************************************************

#define MAX_RT_STACK 32
// TLAS leaves use the following fields for a different purpose:
#define BLASMatrixSlot rootIdx
#define BLASId rootIdx
#define BLASBaseNodeOffset numChildren

extern std::vector<Matrix4> g_TLASMatrices;

float3 cross(const float3& A, const float3& B)
{
	// A.x A.y A.z
	// B.x B.y B.z
	return float3(A.y * B.z - A.z * B.y,
                  A.z * B.x - A.x * B.z,
                  A.x * B.y - A.y * B.x);
}

float dot(const float3& A, const float3& B)
{
	return A.x * B.x + A.y * B.y + A.z * B.z;
}

// --------------------------------------------
// The vertices of the triangle are A, B and C
// --------------------------------------------
Intersection RayTriangleIntersection(Ray ray, float3 A, float3 B, float3 C)
{
	Intersection inters;
	inters.TriID = -1;

	float3 P, T, Q;
	float3 E1 = B - A;
	float3 E2 = C - A;
	P = cross(ray.dir, E2);
	float det = 1.0f / dot(E1, P);
	T = ray.origin - A;
	inters.U = dot(T, P) * det;
	Q = cross(T, E1);
	inters.V = dot(ray.dir, Q) * det;
	inters.T = dot(E2, Q) * det;

	return inters;
}

// ------------------------------------------
// Ray-Triangle intersection test
// ------------------------------------------
bool RayTriangleTest(const Intersection& inters)
{
	return (
            (inters.U >= 0.0f) &&
            (inters.V >= 0.0f) &&
            (inters.U + inters.V <= 1.0f) &&
            (inters.T > 0.0f)
           );
}

/// <summary>
/// Given a triangle defined by vertices v0, v1, v2, and a point P inside this
/// triangle. This code will compute the barycentric coordinates (u, v) for P.
/// (u, v, 1 - (u + v)) can then be used to interpolate other attributes at point P,
/// such as texture coordinates.
/// This code is also part of Direct3DDevice::rayTriangleIntersect()
/// </summary>
void ComputeBaryCoords(float3 v0, float3 v1, float3 v2, float3 P,
	float& u, float& v)
{
	float3 v0v1 = v1 - v0;
	float3 v0v2 = v2 - v0;

	// Compute u-v again to make them consistent with tex coords
	float3 N = cross(v0v1, v0v2);
	float3 C;
	// edge 1
	float3 edge1 = v2 - v1;
	float3 vp1 = P - v1;
	C = cross(edge1, vp1);
	u = dot(N, C);

	// edge 2
	float3 edge2 = v0 - v2;
	float3 vp2 = P - v2;
	C = cross(edge2, vp2);
	v = dot(N, C);

	float denom = dot(N, N);
	u /= denom;
	v /= denom;
}


// ------------------------------------------
// Checks if the ray hits the node's AABB
// ------------------------------------------
// Returns: T[0] the min distance to the box
//          T[1] the max distance to the box
// if T[1] >= T[0], then there's an intersection.
// See also: https://tavianator.com/2011/ray_box.html
float2 _BVHIntersectBox(float3 box_min, float3 box_max, const float3& start, const float3& inv_dir)
{
	const float3 diff_max = (box_max.xyz() - start) * inv_dir;
	const float3 diff_min = (box_min.xyz() - start) * inv_dir;
	float2 T;

	T[0] = min(diff_min.x, diff_max.x);
	T[1] = max(diff_min.x, diff_max.x);

	T[0] = max(T[0], min(diff_min.y, diff_max.y));
	T[1] = min(T[1], max(diff_min.y, diff_max.y));

	T[0] = max(T[0], min(diff_min.z, diff_max.z));
	T[1] = min(T[1], max(diff_min.z, diff_max.z));

	return T;
}

float2 BVHIntersectBox(const BVHNode* BVH, const float3 start, const float3 inv_dir, const int node)
{
	const float3 box_min(BVH[node].min);
	const float3 box_max(BVH[node].max);
	return _BVHIntersectBox(box_min, box_max, start, inv_dir);
}

float3 ClosestPointOnBox(const BVHNode* BVH, const float3& origin, const int node)
{
	const float3 box_min(BVH[node].min);
	const float3 box_max(BVH[node].max);

	// Clamp origin to the box, that's the closest point:
	float3 P = origin;
	if (P.x < box_min.x) P.x = box_min.x;
	if (P.y < box_min.y) P.y = box_min.y;
	if (P.z < box_min.z) P.z = box_min.z;

	if (P.x > box_max.x) P.x = box_max.x;
	if (P.y > box_max.y) P.y = box_max.y;
	if (P.z > box_max.z) P.z = box_max.z;

	return P;
}

inline float length(float3 P)
{
	return sqrt(P.x * P.x + P.y * P.y + P.z * P.z);
}

// Ray traversal, Embedded Geometry version
Intersection _TraceRaySimpleHit(BVHNode* g_BVH, Ray ray, int Offset)
{
	int stack[MAX_RT_STACK];
	int stack_top  = 0;
	int curnode    = -1;
	float3 inv_dir = 1.0f / ray.dir;
	Intersection best_inters;

	// Read the padding from the first BVHNode. It will contain the location of the root
	int root = g_BVH[Offset].rootIdx + Offset;

	stack[stack_top++] = root; // Push the root on the stack
	while (stack_top > 0) {
		// Pop a node from the stack
		curnode = stack[--stack_top];
		BVHNode node = g_BVH[curnode];
		const int TriID = node.ref;

		if (TriID == -1) {
			// This node is a box. Do the ray-box intersection test
			float2 T = BVHIntersectBox(g_BVH, ray.origin, inv_dir, curnode);

			// T[1] >= T[0] is the standard test, but lines are infinite, so it will also intersect
			// boxes behind the ray. To skip those boxes, we need to add T[1] >= 0, to make sure
			// the box is in front of the ray (for rays originating inside a box, we'll have T[0] < 0,
			// so we can't use that test).
			if (T[1] >= 0 && T[1] >= T[0])
			{
				// Ray intersects the box
				// Inner node: push the children of this node on the stack
				if (stack_top + 4 < MAX_RT_STACK) {
					// Push the valid children of this node into the stack
					for (int i = 0; i < 4; i++) {
						int child = node.children[i];
						if (child == -1) break;
						stack[stack_top++] = child + Offset;
					}
				}
			}
		}
		else
		{
			// This node is a triangle. Do the ray-triangle intersection test.
			float3 A = float3(node.min);
			float3 B = float3(node.max);
			const float* fchildren = (float*)node.children;
			const float3 C = float3(
				fchildren[0],
				fchildren[1],
				fchildren[2]);

			Intersection inters = RayTriangleIntersection(ray, A, B, C);
			if (RayTriangleTest(inters) && inters.T < best_inters.T)
			{
				inters.TriID = TriID;
				// Compute the barycentric coords for the intersection:
				float3 P = ray.origin + inters.T * ray.dir;
				ComputeBaryCoords(A, B, C, P, inters.U, inters.V);

				best_inters = inters;
				// return best_inters; // Don't terminate early, keep searching the tree until we find the best intersection
			}
		}
	}

	return best_inters;
}

// Similar to _TraceRaySimpleHit, but finds the closest point on the BVH, from the given.
Intersection ClosestHit(BVHNode* g_BVH, float3 origin, int Offset, float3& P_out,
	ac_uv_coords* coords, int contIdx)
{
	int stack[MAX_RT_STACK];
	int stack_top = 0;
	int curnode = -1;
	Intersection best_inters;

	// Read the padding from the first BVHNode. It will contain the location of the root
	int root = g_BVH[Offset].rootIdx + Offset;

	stack[stack_top++] = root; // Push the root on the stack
	while (stack_top > 0) {
		// Pop a node from the stack
		curnode = stack[--stack_top];
		BVHNode node = g_BVH[curnode];
		const int TriID = node.ref;

		if (TriID == -1)
		{
			// This node is a box. Find the closest point on the box
			float3 P = ClosestPointOnBox(g_BVH, origin, curnode);

			// If origin is inside the box, then P == origin, otherwise, P will be on the
			// surface of the box and it will be the closest point to origin.
			float dist = length(P - origin);

			// The sphere around origin intersects this box, let's recurse:
			if (dist < GLOVE_NEAR_THRESHOLD_OPTSCALE)
			{
				// Ray intersects the box
				// Inner node: push the children of this node on the stack
				if (stack_top + 4 < MAX_RT_STACK) {
					// Push the valid children of this node into the stack
					for (int i = 0; i < 4; i++) {
						int child = node.children[i];
						if (child == -1) break;
						stack[stack_top++] = child + Offset;
					}
				}
			}
		}
		else
		{
			// This node is a triangle. Find the closest point on the triangle
			float3 A = float3(node.min);
			float3 B = float3(node.max);
			const float* fchildren = (float*)node.children;
			const float3 C = float3(fchildren[0], fchildren[1], fchildren[2]);

			Vector3* O  = (Vector3*)&origin;
			Vector3* v0 = (Vector3*)&A;
			Vector3* v1 = (Vector3*)&B;
			Vector3* v2 = (Vector3*)&C;
			Vector3 P;
			float u, v;
			float dist = ClosestPointOnTriangle(*O, *v0, *v1, *v2, P, u, v, 0.001f);
			if (dist < best_inters.T)
			{
				bool match = true;
				if (coords != nullptr)
				{
					match = false;

					const float w = 1.0f - u - v;
					const int index0 = TriID * 3;

					const int t0Idx = g_vrGlovesMeshes[contIdx].texIndices[index0 + 0];
					const int t1Idx = g_vrGlovesMeshes[contIdx].texIndices[index0 + 1];
					const int t2Idx = g_vrGlovesMeshes[contIdx].texIndices[index0 + 2];

					const XwaTextureVertex bestUV0 = g_vrGlovesMeshes[contIdx].texCoords[t0Idx];
					const XwaTextureVertex bestUV1 = g_vrGlovesMeshes[contIdx].texCoords[t1Idx];
					const XwaTextureVertex bestUV2 = g_vrGlovesMeshes[contIdx].texCoords[t2Idx];

					const float texU = u * bestUV0.u + v * bestUV1.u + w * bestUV2.u;
					const float texV = u * bestUV0.v + v * bestUV1.v + w * bestUV2.v;

					for (int i = 0; i < coords->numCoords; i++)
					{
						if (coords->area[i].x0 <= texU && texU <= coords->area[i].x1 &&
							coords->area[i].y0 <= texV && texV <= coords->area[i].y1)
						{
							match = true;
							break;
						}
					}
				}

				if (match) // dist is already better than best_inters.T
				{
					best_inters.TriID = TriID;
					best_inters.T = dist;
					best_inters.U = u;
					best_inters.V = v;
					P_out = { P.x, P.y, P.z };
					// Don't terminate early, keep searching the tree until we find the best intersection
				}
			}
		}
	}

	return best_inters;
}

// This is a copy of the code in RTCommon.h, translated so that it runs in C/C++
Intersection _TLASTraceRaySimpleHit(Ray ray)
{
	int stack[MAX_RT_STACK];
	int stack_top = 0;
	int curnode = -1;
	float3 inv_dir = 1.0f / ray.dir;
	Intersection blank_inters;

	// Read the index of the root node
	BVHNode* g_TLAS = g_ACTLASTree->nodes;
	int root = g_TLAS[0].rootIdx;

	stack[stack_top++] = root; // Push the root on the stack
	while (stack_top > 0)
	{
		// Pop a node from the stack
		curnode = stack[--stack_top];
		const BVHNode node = g_TLAS[curnode];
		const int ID = node.ref;

		if (ID == -1)
		{
			// This is an inner node. Do the ray-box intersection test
			float2 T = BVHIntersectBox(g_TLAS, ray.origin, inv_dir, curnode);

			// T[1] >= T[0] is the standard test, but lines are infinite, so it will also intersect
			// boxes behind the ray. To skip those boxes, we need to add T[1] >= 0, to make sure
			// the box is in front of the ray (for rays originating inside a box, we'll have T[0] < 0,
			// so we can't use that test).
			if (T[1] >= 0 && T[1] >= T[0])
			{
				// Ray intersects the box
				// Inner node: push the children of this node on the stack
				if (stack_top + 4 < MAX_RT_STACK) {
					// Push the valid children of this node into the stack
					for (int i = 0; i < 4; i++) {
						int child = node.children[i];
						if (child == -1) break;
						stack[stack_top++] = child;
					}
				}
			}
		}
		else
		{
			// This node is a TLAS leaf. Here we need to do two things:
			// Intersect the ray with the AABB.
			// Intersect the ray with the OBB.
			// If both tests pass, then:
			// - Fetch the offset for the corresponding BLAS.
			// - Continue tracing with the proper BLAS (adding the BLAS offset to all the nodes)

			// Intersect the ray with the TLAS leaf's AABB:
			float2 T = BVHIntersectBox(g_TLAS, ray.origin, inv_dir, curnode);
			if (T[1] >= 0 && T[1] >= T[0])
			{
				// Ray intersects the box, fetch the BLAS entry and continue the search from there
				int BLASOffset = g_TLAS[curnode].BLASBaseNodeOffset;
				int blasID     = g_TLAS[curnode].BLASId;
				auto it        = g_BLASMap.find(blasID);
				if (it != g_BLASMap.end())
				{
					BLASData blasData = it->second;
					LBVH* bvh = (LBVH* )BLASGetBVH(blasData);
					if (bvh != nullptr)
					{
						// The Active Cockpit TLAS is in OPT coords and the input
						// ray is already in OPT coords: no need to use the WorldView matrices
						//Matrix4 Matrix = g_TLASMatrices[matrixSlot];

						/*
						float* fchildren = (float*)g_TLAS[curnode].children;
						float3 obb_min = float3(
							g_TLAS[curnode].min[3],
							g_TLAS[curnode].max[3],
							fchildren[3]);
						float3 obb_max = float3(
							fchildren[0],
							fchildren[1],
							fchildren[2]);
						*/
						// Transform the ray into the same coord sys as the OBB
						//Ray new_ray;
						//Vector4 temp(ray.origin.x, ray.origin.y, ray.origin.z, 1.0f);
						//temp = Matrix * temp;
						//new_ray.origin = float3(temp);

						//temp = Vector4(ray.dir.x, ray.dir.y, ray.dir.z, 0.0f);
						//temp = Matrix * temp;
						//new_ray.dir = float3(temp);

						// Before traversing the BLAS, check if the ray intersects the OBB
						//const float3 new_ray_inv_dir = { 1.0f / new_ray.dir.x, 1.0f / new_ray.dir.y, 1.0f / new_ray.dir.z };
						//float2 T2 = _BVHIntersectBox(obb_min, obb_max, new_ray.origin, new_ray_inv_dir);
						//const float3 new_ray_inv_dir = 1.0f / ray.dir;
						//float2 T2 = _BVHIntersectBox(obb_min, obb_max, ray.origin, new_ray_inv_dir);
						// The ray intersects the OBB, check the BLAS for triangle intersections
						//if (T2[1] >= 0 && T2[1] >= T2[0])
						{
							//Intersection inters = _TraceRaySimpleHit(resources, new_ray, BLASOffset);
							//log_debug("[DBG] [AC] Traversing BLAS");
							Intersection inters = _TraceRaySimpleHit(bvh->nodes, ray, 0);
							if (inters.TriID != -1)
								return inters;
						}
					}
				}
			}

		}
	}

	return blank_inters;
}

// Trace a ray and return as soon as we hit geometry.
// metric system: Y+ is up, Z+ is forward
// OPT system: Y- forward?, Z+ is up?
Intersection TLASTraceRaySimpleHit(Ray ray)
{
	float3 pos3D = ray.origin;
	// ray.origin is in the pos3D frame (Metric, Y+ is up, Z+ is forward)
	// we need to revert it into OPT coords, but the TLAS and the ray are
	// in the same coord sys. There's no need to transform anything
	pos3D.y = -pos3D.y;
	pos3D *= 40.96f;

	// pos3D and dir are now in WorldView coords. We can cast the ray
	ray.origin = pos3D;
	// Ray needs to have its direction inverted. Also Y is flipped for the same reason
	// pos3D.y is flipped (Y+ is up)
	ray.max_dist *= 40.96f;
	return _TLASTraceRaySimpleHit(ray);
}

extern std::vector<TLASLeafItem> g_ACtlasLeaves;
// ****************************************************************************

// P is in the internal metric system:
// X+: Right
// Y+: Up
// Z+: Towards the camera
Vector4 MetricToOPTCoords(Vector4 P)
{
	Vector4 Q;

	float cockpitOriginX = *g_POV_X;
	float cockpitOriginY = *g_POV_Y;
	float cockpitOriginZ = *g_POV_Z;

	// Convert to OPT scale
	P.x *= METERS_TO_OPT;
	P.y *= METERS_TO_OPT;
	P.z *= METERS_TO_OPT;

	// Swap Z-Y axes to match XWA's coord sys and move the origin to this cockpit's POV:
	Q.x = P.w * cockpitOriginX + P.x;
	Q.y = P.w * cockpitOriginY - P.z; // Y- is forwards in the XWA coord sys
	Q.z = P.w * cockpitOriginZ + P.y;
	Q.w = P.w;

	return Q;
}

// P is in the SteamVR metric system:
// X+: Right
// Y+: Up
// Z-: Forwards
Vector4 SteamVRToOPTCoords(Vector4 P)
{
	Vector4 Q;

	float cockpitOriginX = *g_POV_X;
	float cockpitOriginY = *g_POV_Y;
	float cockpitOriginZ = *g_POV_Z;

	float dispX = P.w * (cockpitOriginX + (g_pSharedDataCockpitLook->POVOffsetX * g_pSharedDataCockpitLook->povFactor));
	float dispY = P.w * (cockpitOriginY - (g_pSharedDataCockpitLook->POVOffsetZ * g_pSharedDataCockpitLook->povFactor));
	float dispZ = P.w * (cockpitOriginZ + (g_pSharedDataCockpitLook->POVOffsetY * g_pSharedDataCockpitLook->povFactor));

	// Swap Z-Y axes to match XWA's coord sys and move the origin to this cockpit's POV:
	Q.x = (P.x * METERS_TO_OPT) + dispX;
	Q.y = (P.z * METERS_TO_OPT) + dispY; // Y- is forwards in XWA and Z- is forwards in SteamVR, so we're even
	Q.z = (P.y * METERS_TO_OPT) + dispZ;
	Q.w = P.w;

	return Q;
}

// P is in the XWA coord system:
// X+: Right
// Y-: Forwards
// Z+: Up
Vector4 OPTCoordsToSteamVR(Vector4 P)
{
	Vector4 Q;

	float cockpitOriginX = *g_POV_X;
	float cockpitOriginY = *g_POV_Y;
	float cockpitOriginZ = *g_POV_Z;

	float dispX = P.w * (cockpitOriginX + (g_pSharedDataCockpitLook->POVOffsetX * g_pSharedDataCockpitLook->povFactor));
	float dispY = P.w * (cockpitOriginY - (g_pSharedDataCockpitLook->POVOffsetZ * g_pSharedDataCockpitLook->povFactor));
	float dispZ = P.w * (cockpitOriginZ + (g_pSharedDataCockpitLook->POVOffsetY * g_pSharedDataCockpitLook->povFactor));

	// Swap Z-Y axes to match XWA's coord sys and move the origin to this cockpit's POV:
	P.x = (Q.x - dispX) * OPT_TO_METERS;
	P.y = (Q.z - dispY) * OPT_TO_METERS; // Y- is forwards in XWA and Z- is forwards in SteamVR, so we're even
	P.z = (Q.y - dispZ) * OPT_TO_METERS;
	Q.w = P.w;

	return Q;
}

/// <summary>
/// Converts the controller SteamVR position to Viewspace OPT coords.
/// </summary>
void VRControllerToOPTCoords(Vector4 contOrigin[2], Vector4 contDir[2])
{
	for (int i = 0; i < 2; i++)
	{
		if (g_bUseSteamVR)
		{
			contOrigin[i] = SteamVRToOPTCoords(g_contOriginWorldSpace[i]);
			contDir[i]    = SteamVRToOPTCoords(g_contDirWorldSpace[i]);
		}
		else
		{
			contOrigin[i] = MetricToOPTCoords(g_contOriginWorldSpace[i]);
			contDir[i]    = MetricToOPTCoords(g_contDirWorldSpace[i]);
		}
	}
}

void OPTVertexToPostProcCoords(float* viewportScale, const Vector4& P, float *screenX, float *screenY)
{
	for (int i = 0; i < 4; i++)
		g_VSCBuffer.viewportScale[i] = viewportScale[i];
	float4 pos2D = TransformProjectionScreen(float3(P));

	// pos2D is now in in-game screen coords. We need to convert that to post-proc UV coords:
	UINT left   = (UINT)g_nonVRViewport.TopLeftX;
	UINT top    = (UINT)g_nonVRViewport.TopLeftY;
	UINT width  = (UINT)g_nonVRViewport.Width;
	UINT height = (UINT)g_nonVRViewport.Height;

	InGameToScreenCoords(left, top, width, height, pos2D.x, pos2D.y, screenX, screenY);
	*screenX /= g_fCurScreenWidth;
	*screenY /= g_fCurScreenHeight;
}

/*
 * pos2D = EyeProj * Headset * OPT_TO_METERS * WorldView * pos3D
 * According to my own notes, in SteamVR, the coordinate system is as follows:
 * +x is right
 * +y is up
 * -z is forward
 */
void PrimarySurface::OPTVertexToSteamVRPostProcCoords(Vector4 pos3D, Vector4 pos2D[2])
{
	const bool bGunnerTurret = (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME) ?
		PlayerDataTable[*g_playerIndex].gunnerTurretActive : false;

	EffectsRenderer* renderer = (EffectsRenderer*)g_current_renderer;
	// TODO: When pos3D comes from the Gunner Turret, then W should be the identity matrix
	const Matrix4 W = Matrix4(renderer->_CockpitConstants.transformWorldView);
	const float pos3Dw = pos3D.w;
	const Matrix4 V = g_VSMatrixCB.fullViewMat;
	Vector4 P;

	if (!bGunnerTurret)
	{
		// Transform to WorldView coords:
		P = W * pos3D;
		P *= OPT_TO_METERS;

		pos3D.x = P.x;
		pos3D.y = P.z;
		pos3D.z = P.y;
		// At this point, output.pos3D is metric, X+ is right, Y+ is up and Z- is forward (away from the camera), because
		// that's the SteamVR coord sys

		// Apply the current SteamVR headset orientation:
		// I know this is weird, but we need to set w = 0 because the translation is already
		// applied to the World through the Cockpit Look Hook. So if we apply it here again,
		// the cursor will be translated twice. So, we set w = 0 and thus avoid the translation...
		pos3D.w = 0.0f;
		pos3D = V * pos3D;
		// ... but then we need to set w = 1 back here to enable the regular projection that is
		// effected below.
		pos3D.w = pos3Dw;
	}
	else
	{
		pos3D = OPT_TO_METERS * pos3D;
		pos3D.z = -pos3D.z;
		pos3D.w = pos3Dw;
	}

	for (int eye = 0; eye < 2; eye++)
	{
		// Project:
		pos2D[eye] = g_VSMatrixCB.projEye[eye] * pos3D;
		// DirectX divides by w internally after the PixelShader output is written. We don't
		// see that division in the shader; but we have to do it explicitly here because
		// that's what actually accomplishes the 3D -> 2D projection (it's like a weighed
		// division by Z)
		pos2D[eye] *= (1.0f / pos2D[eye].w);

		// P is now in the internal DirectX coord sys: xy in (-1..1)
		// So let's transform to the range 0..1 for post-proc coords:
		pos2D[eye].x = pos2D[eye].x * 0.5f + 0.5f;
		pos2D[eye].y = pos2D[eye].y * 0.5f + 0.5f;
		// Post-proc coords are 0 on the top and 1 at the bottom, so we need to invert the Y axis:
		pos2D[eye].y = 1.0f - pos2D[eye].y;
	}
}

/*
 * Input: offscreenBuffer (resolved here)
 * Output: offscreenBufferPost
 */
void PrimarySurface::RenderLaserPointer(D3D11_VIEWPORT *lastViewport,
	ID3D11PixelShader *lastPixelShader, Direct3DTexture *lastTextureSelected,
	ID3D11Buffer *lastVertexBuffer, UINT *lastVertexBufStride, UINT *lastVertexBufOffset)
{
	EffectsRenderer* renderer = (EffectsRenderer*)g_current_renderer;
	if (!renderer->_bCockpitConstantsCaptured)
		return;

	this->_deviceResources->BeginAnnotatedEvent(L"RenderLaserPointer");

	// DEBUG: Display contOrigin and intersVR inside the cockpit. Looks nice, BTW
#ifdef DISABLED
	{
		std::string msg;
		Vector3 P;
		Vector4 Q;
		int contIdx = 0;
		int y = 50;

		// Both P and Q are in the viewspace system and they are not affected by inertia.
		Q = g_contOriginWorldSpace[contIdx];
		msg = "contOrigin[" + std::to_string(contIdx) + "]: " + std::to_string(Q.x) + ", " + std::to_string(Q.y) + ", " + std::to_string(Q.z);
		DisplayCenteredText((char *)msg.c_str(), FONT_LARGE_IDX, y, FONT_BLUE_COLOR);
		y += 25;

		P = g_LaserPointerIntersSteamVR[contIdx]; // Use this for the Gunner Turret
		msg = "intersVR[" + std::to_string(contIdx) + "]: " + std::to_string(P.x) + ", " + std::to_string(P.y) + ", " + std::to_string(P.z);
		DisplayCenteredText((char*)msg.c_str(), FONT_LARGE_IDX, y, FONT_BLUE_COLOR);
		y += 25;

		P = g_LaserPointer3DIntersection[contIdx]; // Use this for the Cockpit
		msg = "intersOPT[" + std::to_string(contIdx) + "]: " + std::to_string(P.x) + ", " + std::to_string(P.y) + ", " + std::to_string(P.z);
		DisplayCenteredText((char*)msg.c_str(), FONT_LARGE_IDX, y, FONT_BLUE_COLOR);
		y += 25;
	}
#endif

	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	float x0, y0, x1, y1;
	static float iTime = 0.0f;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	D3D11_VIEWPORT viewport{};
	// The viewport covers the *whole* screen, including areas that were not rendered during the forward pass
	// because this is a post-process effect

	const bool bGunnerTurret = (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME) ?
		PlayerDataTable[*g_playerIndex].gunnerTurretActive : false;
	const bool bInHangar = *g_playerInHangar;

	context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
	if (g_bUseSteamVR)
		context->ResolveSubresource(
			resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
			resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);

	resources->InitPixelShader(g_bUseSteamVR ? resources->_laserPointerPS_VR :resources->_laserPointerPS);

	//g_LaserPointerBuffer.DirectSBSEye = -1;
	//if (g_bEnableVR && !g_bUseSteamVR)
	//	g_LaserPointerBuffer.DirectSBSEye = 1;

	// contOriginDisplay is in OPT coords, viewspace
	Vector4 contOriginDisplay[2], contDirDisplay[2];
	if (!bGunnerTurret && !bInHangar)
		VRControllerToOPTCoords(contOriginDisplay, contDirDisplay);
	else
		for (int i = 0; i < 2; i++)
		{
			// This is the same transform chain used in IntersectVRGeometry()
			Matrix4 swap({ 1,0,0,0,  0,0,1,0,  0,1,0,0,  0,0,0,1 });
			Matrix4 Sinv = Matrix4().scale(METERS_TO_OPT);
			Matrix4 toOPT = Sinv * swap;
			g_contOriginWorldSpace[i].w = 1.0f;
			contOriginDisplay[i] = toOPT * g_contOriginWorldSpace[i];
		}

	GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);
	g_LaserPointerBuffer.x0 = x0;
	g_LaserPointerBuffer.y0 = y0;
	g_LaserPointerBuffer.x1 = x1;
	g_LaserPointerBuffer.y1 = y1;
	g_LaserPointerBuffer.iResolution[0] = g_fCurScreenWidth;
	g_LaserPointerBuffer.iResolution[1] = g_fCurScreenHeight;
	// Let's fix the aspect ratio of the laser pointer in non-VR mode:
	g_LaserPointerBuffer.lp_aspect_ratio[0] = g_bEnableVR ? 1.0f : g_MetricRecCBuffer.mr_screen_aspect_ratio;
	g_LaserPointerBuffer.lp_aspect_ratio[1] = 1.0f;
	g_LaserPointerBuffer.bDebugMode = 0;

	g_LaserPointerBuffer.bDisplayLine[0] = false;
	g_LaserPointerBuffer.bDisplayLine[1] = false;
	/*g_LaserPointerBuffer.bDisplayLine[0] = true;
	g_LaserPointerBuffer.bDisplayLine[1] = true;*/

	// TODO: Don't allow intersections with negative distances!
	Matrix4 W = XwaTransformToMatrix4(renderer->_CockpitWorldView);
	for (int contIdx = 0; contIdx < 2; contIdx++)
	{
		const bool gloveVisible = g_vrGlovesMeshes[contIdx].visible;
		if (!gloveVisible)
			g_LaserPointerBuffer.bDisplayLine[contIdx] = true;

		// Push button detection
		static bool bPrevPushButton = false;
		static int buttonState = 0;
		bool bPushButton    = (g_iBestIntersTexIdx[contIdx] != -1) && (g_fBestIntersectionDistance[contIdx] <= g_fPushButtonThreshold);
		bool bReleaseButton = (g_iBestIntersTexIdx[contIdx] == -1) || (g_fBestIntersectionDistance[contIdx] >  g_fReleaseButtonThreshold);
		bool bButtonTrigger = false;

		// Small FSM to better control button behavior. Makes it more difficult to trigger a button multiple times
		switch (buttonState)
		{
		case 0:
			if (!bPrevPushButton && bPushButton)
			{
				bButtonTrigger = true;
				buttonState = 1;
			}
			break;
		case 1:
			bButtonTrigger = false; // Only trigger the button action once
			// Only go back to "unpushed" if the distance is big enough:
			if (bReleaseButton)
				buttonState = 0;
			break;
		}

		// Detect triggers:
		//if (g_bACLastTriggerState[contIdx] && !g_bACTriggerState[contIdx]) // || bButtonTrigger) // Push-button behavior is flaky. Needs better algorithms.
		if (!g_bACLastTriggerState[contIdx] && g_bACTriggerState[contIdx])
			g_bACActionTriggered[contIdx] = true;

		// The trigger button was released. Erase the clickRegion for this controller.
		if (g_bACLastTriggerState[contIdx] && !g_bACTriggerState[contIdx])
			g_vrKeybState.clickRegions[contIdx] = { -1, -1, -1, -1 };

		g_bACLastTriggerState[contIdx] = g_bACTriggerState[contIdx];
		// Update the display
		//g_LaserPointerBuffer.TriggerState = g_bACTriggerState || (buttonState == 1);
		g_LaserPointerBuffer.TriggerState[contIdx] = g_bACTriggerState[contIdx]; // Push-button behavior is flaky. Needs better algorithms.
		bPrevPushButton = bPushButton;

		// Regular transform chain: OPT to in-game screen coords.
		/*contOriginDisplay = Vector4(0.0f, -1.0f, 0.5f, 1.0f);
		contOriginDisplay *= METERS_TO_OPT;
		contOriginDisplay.w = 1.0f;
		contOriginDisplay = W * contOriginDisplay;*/

		// contOriginDisplay is now in OPT coords. We can now use g_ACTLASTree to find the closest hit
		// OPT coord system:
		//    X+ --> right
		//    Y+ --> backwards
		//    Z+ --> up
		Ray ray;
		ray.origin   = float3(contOriginDisplay[contIdx]);
		ray.dir      = float3(contDirDisplay[contIdx]);
		ray.max_dist = RT_MAX_DIST * METERS_TO_OPT;

		Intersection inters;
		// The intersection was already found with an active element in EffectsRenderer:ApplyActiveCockpit()
		// There's no need to traverse the BVH again here
		if (g_iBestIntersTexIdx[contIdx] != -1)
		{
			inters.T     = g_fBestIntersectionDistance[contIdx];
			inters.TriID = g_iBestIntersTexIdx[contIdx];
			// Enable the following line and LASER_VR_DEBUG in LaserPointerCommon.h to display
			// the geometry behind the AC elements:
			//g_LaserPointerBuffer.bDebugMode = g_enable_ac_debug;
		}
		// I don't think we need these intersection markers when the gloves are visible
		else if (!g_vrGlovesMeshes[contIdx].visible)
		{
			//inters = TLASTraceRaySimpleHit(ray);
			// Find the ray-geometry intersection by iterating over all the BLASes
			for (const auto& leaf : g_ACtlasLeaves)
			{
				auto it = g_BLASMap.find(leaf.PrimID);
				if (it != g_BLASMap.end())
				{
					BLASData blasData = it->second;
					LBVH* bvh = (LBVH*)BLASGetBVH(blasData);
					if (bvh != nullptr)
					{
						Intersection tempInters = _TraceRaySimpleHit(bvh->nodes, ray, 0);
						if (tempInters.TriID != -1 && // There was an intersection
							tempInters.T > 0.0f &&    // It's not behind the ray's origin
							tempInters.T < inters.T)  // It's better than the best intersection so far
						{
							inters = tempInters;
						}
					}
				}
			}
		}

		if (!bGunnerTurret && !bInHangar)
			contOriginDisplay[contIdx] = W * contOriginDisplay[contIdx];
		// contOriginDisplay is now in Worldview coords
		g_LaserPointerBuffer.bContOrigin[contIdx] = (contOriginDisplay[contIdx].z > 0.01f); // Don't display the cursor if it's behind the camera

		float screenX, screenY;
		Vector4 pos2D[2];

		// Compute the correct radius of the projected cursor origin
		if (!g_bUseSteamVR)
		{
			OPTVertexToPostProcCoords(renderer->_CockpitConstants.viewportScale, contOriginDisplay[contIdx], &screenX, &screenY);
			g_LaserPointerBuffer.contOrigin[contIdx][0].x = screenX;
			g_LaserPointerBuffer.contOrigin[contIdx][0].y = screenY;
			g_LaserPointerBuffer.contOrigin[contIdx][0].z = contOriginDisplay[contIdx].z * OPT_TO_METERS;

			// Project a point 4mm to the right of contOriginDisplay to get the proper radius
			float sX1;
			Vector4 Q = contOriginDisplay[contIdx];
			Q.x += (0.004f * METERS_TO_OPT);
			OPTVertexToPostProcCoords(renderer->_CockpitConstants.viewportScale, Q, &sX1, &screenY);
			g_LaserPointerBuffer.cursor_radius[contIdx] = (sX1 - screenX);
		}
		else
		{
			// contOriginDisplay is now in ViewSpace coords (it was multiplied by W above).
			OPTVertexToSteamVRPostProcCoords(contOriginDisplay[contIdx], pos2D);
			// Left eye cursor
			g_LaserPointerBuffer.contOrigin[contIdx][0].x = pos2D[0].x;
			g_LaserPointerBuffer.contOrigin[contIdx][0].y = pos2D[0].y;
			g_LaserPointerBuffer.contOrigin[contIdx][0].z = contOriginDisplay[contIdx].z * OPT_TO_METERS;

			// Right eye cursor
			g_LaserPointerBuffer.contOrigin[contIdx][1].x = pos2D[1].x;
			g_LaserPointerBuffer.contOrigin[contIdx][1].y = pos2D[1].y;
			g_LaserPointerBuffer.contOrigin[contIdx][1].z = contOriginDisplay[contIdx].z * OPT_TO_METERS;

			// Project a point 3mm to the right of contOriginDisplay to get the proper radius
			float sX0 = pos2D[0].x;
			Vector4 Q = contOriginDisplay[contIdx];
			Q.x += (0.003f * METERS_TO_OPT);
			OPTVertexToSteamVRPostProcCoords(Q, pos2D);
			g_LaserPointerBuffer.cursor_radius[contIdx] = (pos2D[0].x - sX0);
		}

		g_fLaserIntersectionDistance[contIdx] = FLT_MAX;
		float3 P;
		if (inters.TriID != -1)
		{
			if (g_iBestIntersTexIdx[contIdx] != -1)
			{
				P = g_LaserPointer3DIntersection[contIdx];
			}
			else
			{
				P = ray.origin + inters.T * ray.dir;
			}

			g_LaserPointerBuffer.bIntersection[contIdx] = true;
			//g_LaserPointerBuffer.bIntersection[contIdx] = false;
			Vector3 O = { ray.origin.x, ray.origin.y, ray.origin.z };
			Vector3 D = { P.x, P.y, P.z };
			if (bGunnerTurret || bInHangar)
			{
				O.x = g_contOriginWorldSpace[contIdx].x;
				O.y = g_contOriginWorldSpace[contIdx].y;
				O.z = g_contOriginWorldSpace[contIdx].z;

				D.x = g_LaserPointerIntersSteamVR[contIdx].x;
				D.y = g_LaserPointerIntersSteamVR[contIdx].y;
				D.z = g_LaserPointerIntersSteamVR[contIdx].z;
			}
			g_fLaserIntersectionDistance[contIdx] = (D - O).length();
			if (bGunnerTurret || bInHangar)
				D *= OPT_TO_METERS;

			// DEBUG
#ifdef DISABLED
			{
				Vector4 pos2D[2];
				Vector4 P;

				P = W * Vector4(g_debug_v0.x, g_debug_v0.y, g_debug_v0.z, 1.0f);
				OPTVertexToSteamVRPostProcCoords(P, pos2D);
				g_LaserPointerBuffer.v0L = pos2D[0];
				g_LaserPointerBuffer.v0R = pos2D[1];

				P = W * Vector4(g_debug_v1.x, g_debug_v1.y, g_debug_v1.z, 1.0f);
				OPTVertexToSteamVRPostProcCoords(P, pos2D);
				g_LaserPointerBuffer.v1L = pos2D[0];
				g_LaserPointerBuffer.v1R = pos2D[1];

				P = W * Vector4(g_debug_v2.x, g_debug_v2.y, g_debug_v2.z, 1.0f);
				OPTVertexToSteamVRPostProcCoords(P, pos2D);
				g_LaserPointerBuffer.v2L = pos2D[0];
				g_LaserPointerBuffer.v2R = pos2D[1];
			}
#endif
		}
		else // When there's no intersection, just draw a line pointing in the direction of the ray
		{
			// ray.dir is already in OPT metric (i.e. its length is 40.96), so the following line is equivalent to:
			// 0.5f * METERS_TO_OPT * normalize(ray.dir)
			P = ray.origin + 0.5f * ray.dir;
			g_LaserPointerBuffer.bIntersection[contIdx] = false;
		}

		// If we're gripping either the throttle or the joystick, don't display the intersections -- looks weird
		if (g_contStates[contIdx].buttons[VRButtons::GRIP])
			g_LaserPointerBuffer.bIntersection[contIdx] = false;

		// Compute the projected size of the intersection point (P)
		{
			// P is in OPT coords, now we need to transform it to WorldView coords for the projection:
			// If the Gunner Turret is active, then P is in SteamVR coords, so we need to make some adjustments.
			Vector4 Q = Vector4(P.x, P.y, P.z, 1.0f);
			if (!bGunnerTurret && !bInHangar)
				Q = W * Q;

			// Don't display the intersection if it's behind the camera
			// This is not a mistake, Q is in OPT scale, but Z is depth. The multiplication by W
			// above does the axis swap for the regular cockpit, and for the Gunner Turret, we
			// make sure to do the swap in IntersectVRGeometry()
			if (Q.z > 0.01f)
			{
				if (!g_bUseSteamVR)
				{
					OPTVertexToPostProcCoords(renderer->_CockpitConstants.viewportScale, Q, &screenX, &screenY);
					if (contIdx == 0)
					{
						g_LaserPointerBuffer.intersection[contIdx][0][0] = screenX;
						g_LaserPointerBuffer.intersection[contIdx][0][1] = screenY;
						g_LaserPointerBuffer.intersection[contIdx][0][2] = Q.z * OPT_TO_METERS;
					}

					// Project a point 2.5mm to the right of Q to get the proper 3D radius
					float sX1;
					Q.x += (0.0025f * METERS_TO_OPT);
					OPTVertexToPostProcCoords(renderer->_CockpitConstants.viewportScale, Q, &sX1, &screenY);
					g_LaserPointerBuffer.inters_radius[contIdx] = (sX1 - screenX);
				}
				else
				{
					OPTVertexToSteamVRPostProcCoords(Q, pos2D);
					// Left eye
					g_LaserPointerBuffer.intersection[contIdx][0][0] = pos2D[0].x;
					g_LaserPointerBuffer.intersection[contIdx][0][1] = pos2D[0].y;
					g_LaserPointerBuffer.intersection[contIdx][0][2] = Q.z * OPT_TO_METERS;

					// Right eye
					g_LaserPointerBuffer.intersection[contIdx][1][0] = pos2D[1].x;
					g_LaserPointerBuffer.intersection[contIdx][1][1] = pos2D[1].y;
					g_LaserPointerBuffer.intersection[contIdx][1][2] = Q.z * OPT_TO_METERS;

					// Project a point 1.5mm to the right of Q to get the proper 3D radius
					float sX0 = pos2D[0].x;
					Q.x += (0.0015f * METERS_TO_OPT);
					OPTVertexToSteamVRPostProcCoords(Q, pos2D);
					g_LaserPointerBuffer.inters_radius[contIdx] = (pos2D[0].x - sX0);
				}
			}
		}

		// If there was an intersection, find the action and execute it.
		// (I don't think this code needs to be here; but I put it here with the rest of the render function)
		if (g_LaserPointerBuffer.bIntersection[contIdx] &&
			g_iBestIntersTexIdx[contIdx] > -1 &&
			g_iBestIntersTexIdx[contIdx] < g_iNumACElements)
		{
			ac_uv_coords *coords = &(g_ACElements[g_iBestIntersTexIdx[contIdx]].coords);
			// g_iBestIntersTexIdx and g_LaserPointerBuffer.uv are populated in EffectsRenderer.cpp:ApplyActiveCockpit()
			float u = g_LaserPointerBuffer.uv[contIdx][0];
			float v = g_LaserPointerBuffer.uv[contIdx][1];
			float u0 = u, v0 = v;

			// Fix negative UVs (yes, some OPTs may have negative UVs)
			while (u < 0.0f) u += 1.0f;
			while (v < 0.0f) v += 1.0f;
			// Fix UVs beyond 1
			while (u > 1.0f) u -= 1.0f;
			while (v > 1.0f) v -= 1.0f;

			for (int i = 0; i < coords->numCoords; i++)
			{
				if (coords->area[i].x0 <= u && u <= coords->area[i].x1 &&
					coords->area[i].y0 <= v && v <= coords->area[i].y1)
				{
					g_LaserPointerBuffer.bHoveringOnActiveElem[contIdx] = 1;
					if (g_bACActionTriggered[contIdx]) {
						/*
						short width  = g_ACElements[g_iBestIntersTexIdx[contIdx]].width;
						short height = g_ACElements[g_iBestIntersTexIdx[contIdx]].height;

						log_debug("[DBG} [AC] *************");
						log_debug("[DBG] [AC] g_iBestIntersTexIdx: %d", g_iBestIntersTexIdx);
						log_debug("[DBG] [AC] Texture name: %s", g_ACElements[g_iBestIntersTexIdx].name);
						log_debug("[DBG] [AC] numCoords: %d", g_ACElements[g_iBestIntersTexIdx].coords.numCoords);
						log_debug("[DBG] [AC] Running action: [%s]", coords->action_name[i]);
						log_debug("[DBG] [AC] uv coords: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
							coords->area[i].x0, coords->area[i].y0,
							coords->area[i].x1, coords->area[i].y1);
						log_debug("[DBG] [AC] laser uv (raw): (%0.3f, %0.3f)", u0, v0);
						log_debug("[DBG] [AC] laser uv: (%0.3f, %0.3f)-(%d, %d)",
							u, v, (short)(width * u), (short)(height * v));
						*/

						// Run the action itself
						ACRunAction(coords->action[i], coords->area[i], g_iBestIntersTexIdx[contIdx], contIdx);
						//log_debug("[DBG} [AC] *************");
					}
					break;
				}
			}
		}
		g_bACActionTriggered[contIdx] = false;
	}

	// If the gloves are visible, then there's no need to render the ball-and-sticks pointers:
	// RenderVRDots() will take care of rendering the intersection markers
	if (g_vrGlovesMeshes[0].visible)
		return;
	// ******************************************************

	// Render the laser pointer and intersections
	// Temporarily disable ZWrite: we won't need it for post-proc
	D3D11_DEPTH_STENCIL_DESC desc;
	ComPtr<ID3D11DepthStencilState> depthState;
	desc.DepthEnable = FALSE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	desc.StencilEnable = FALSE;
	resources->InitDepthStencilState(depthState, &desc);

	// Dump some debug info to see what's happening with the intersection
#ifdef DISABLED
	if (g_bDumpLaserPointerDebugInfo) {
		Vector3 pos3D = Vector3(g_LaserPointer3DIntersection.x, g_LaserPointer3DIntersection.y, g_LaserPointer3DIntersection.z);
		Vector3 p = projectMetric(pos3D, g_viewMatrix, g_FullProjMatrixLeft /*, NULL, NULL*/);
		bool bIntersection = g_LaserPointerBuffer.bIntersection;
		log_debug("[DBG] [AC] bIntersection: %d", bIntersection);
		if (bIntersection) {
			short width = g_iBestIntersTexIdx > -1 ? g_ACElements[g_iBestIntersTexIdx].width : 0;
			short height = g_iBestIntersTexIdx > -1 ? g_ACElements[g_iBestIntersTexIdx].height : 0;
			log_debug("[DBG] [AC] g_iBestIntersTexIdx: %d", g_iBestIntersTexIdx);
			if (g_iBestIntersTexIdx > -1)
				log_debug("[DBG] [AC] Texture: %s", g_ACElements[g_iBestIntersTexIdx].name);
			else
				log_debug("[DBG] [AC] NULL Texture name (-1 index)");
			log_debug("[DBG] [AC] intersection: (%0.3f, %0.3f, %0.3f) --> (%0.3f, %0.3f)",
				pos3D.x, pos3D.y, pos3D.z, p.x, p.y);
			log_debug("[DBG] [AC] laser uv: (%0.3f, %0.3f)-(%d, %d)",
				g_LaserPointerBuffer.uv[0], g_LaserPointerBuffer.uv[1],
				(short)(width * g_LaserPointerBuffer.uv[0]), (short)(height * g_LaserPointerBuffer.uv[1]));
		}
		//log_debug("[DBG] [AC] g_contOrigin: (%0.3f, %0.3f, %0.3f)", g_contOriginWorldSpace.x, g_contOriginWorldSpace.y, g_contOriginWorldSpace.z);
		//log_debug("[DBG] [AC] g_contDirection: (%0.3f, %0.3f, %0.3f)", g_contDirWorldSpace.x, g_contDirWorldSpace.y, g_contDirWorldSpace.z);
		log_debug("[DBG] [AC] Triangle, best t: %0.3f: ", g_fBestIntersectionDistance);
		//log_debug("[DBG] [AC] v0: (%0.3f, %0.3f)", g_LaserPointerBuffer.v0[0], g_LaserPointerBuffer.v0[1]);
		//log_debug("[DBG] [AC] v1: (%0.3f, %0.3f)", g_LaserPointerBuffer.v1[0], g_LaserPointerBuffer.v1[1]);
		//log_debug("[DBG] [AC] v2: (%0.3f, %0.3f)", g_LaserPointerBuffer.v2[0], g_LaserPointerBuffer.v2[1]);

		/*
		FILE *file = NULL;
		fopen_s(&file, "./test-tri-inters.obj", "wt");
		fprintf(file, "# Triangle\n");
		fprintf(file, "v %0.6f %0.6f %0.6f\n", g_debug_v0.x, g_debug_v0.y, g_debug_v0.z);
		fprintf(file, "v %0.6f %0.6f %0.6f\n", g_debug_v1.x, g_debug_v1.y, g_debug_v1.z);
		fprintf(file, "v %0.6f %0.6f %0.6f\n", g_debug_v2.x, g_debug_v2.y, g_debug_v2.z);
		fprintf(file, "\n# Origin\n");
		fprintf(file, "v %0.6f %0.6f %0.6f\n", g_contOriginViewSpace.x, g_contOriginViewSpace.y, g_contOriginViewSpace.z);
		fprintf(file, "\n# Intersection\n");
		fprintf(file, "v %0.6f %0.6f %0.6f\n", g_LaserPointer3DIntersection.x, g_LaserPointer3DIntersection.y, g_LaserPointer3DIntersection.z);
		fprintf(file, "\n# Triangle\n");
		fprintf(file, "f 1 2 3\n");
		fprintf(file, "# Line\n");
		fprintf(file, "f 4 5\n");
		fclose(file);
		log_debug("[DBG] [AC] test-tri-inters.obj dumped");
		*/

		//g_bDumpLaserPointerDebugInfo = false;
	}
#endif

	{
		// Set the new viewport (a full quad covering the full screen)
		viewport.Width  = g_fCurScreenWidth;
		viewport.Height = g_fCurScreenHeight;
		// VIEWPORT-LEFT
		if (g_bEnableVR) {
			if (g_bUseSteamVR)
				viewport.Width = (float)resources->_backbufferWidth;
			else
				viewport.Width = (float)resources->_backbufferWidth / 2.0f;
		}
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		resources->InitViewport(&viewport);

		// Set the vertex buffer
		UINT stride = sizeof(MainVertex);
		UINT offset = 0;
		resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);
		resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		resources->InitInputLayout(resources->_mainInputLayout);
		
		resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(), 0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices
		resources->InitPSConstantBufferLaserPointer(resources->_laserPointerConstantBuffer.GetAddressOf(), &g_LaserPointerBuffer);
		resources->InitVertexShader(g_bUseSteamVR ? resources->_mainVertexShaderVR : resources->_mainVertexShader);

		context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		// Set the RTV:
		ID3D11RenderTargetView *rtvs[1] = {
			resources->_renderTargetViewPost.Get(), // Render to offscreenBufferPost instead of offscreenBuffer
		};
		context->OMSetRenderTargets(1, rtvs, NULL);
		// Set the SRVs:
		ID3D11ShaderResourceView* srvs[] = {
			resources->_offscreenAsInputShaderResourceView.Get(),
			resources->_depthBufSRV.Get()
		};
		context->PSSetShaderResources(0, 2, srvs);
		if (g_bUseSteamVR)
			context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
		else
			context->Draw(6, 0);
	}

	context->CopyResource(resources->_offscreenBuffer, resources->_offscreenBufferPost);

	// Restore previous rendertarget, etc
	resources->InitInputLayout(resources->_inputLayout);
	context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(),
		resources->_depthStencilViewL.Get());

	// Don't restore anything...
	// I wonder if this is causing the bug that makes the menu go black? Maybe I need to restore
	// the 2D vertex buffer here?
	//goto out;

	/*
	// Restore the original state: VertexBuffer, Shaders, Topology, Z-Buffer state, etc...
	resources->InitViewport(lastViewport);
	if (lastTextureSelected != NULL) {
		lastTextureSelected->_refCount++;
		context->PSSetShaderResources(0, 1, lastTextureSelected->_textureView.GetAddressOf());
		lastTextureSelected->_refCount--;
	}
	resources->InitInputLayout(resources->_inputLayout);
	if (g_bEnableVR)
		this->_deviceResources->InitVertexShader(resources->_sbsVertexShader); // if (g_bEnableVR)
	else
		this->_deviceResources->InitVertexShader(resources->_vertexShader);
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitPixelShader(lastPixelShader);
	resources->InitRasterizerState(resources->_rasterizerState);
	if (lastVertexBuffer != NULL)
		resources->InitVertexBuffer(&lastVertexBuffer, lastVertexBufStride, lastVertexBufOffset);
	resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
	resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
out:
*/

	this->_deviceResources->EndAnnotatedEvent();
}

/*
 * Compute the current view matrices from SteamVR or FreePIE and store them in
 * g_VSMatrixCB.viewMat, and g_VSMatrixCB.fullViewMat = rotMatrixFull.
 * This function also updates the laser pointer (this is mostly "TO-DO" at the moment)
 */
void UpdateViewMatrix()
{
	float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
	float x = 0.0f, y = 0.0f, z = 0.0f;
	static float home_yaw = 0.0f, home_pitch = 0.0f, home_roll = 0.0f;

	if (g_bUseSteamVR)
	{
		Matrix4 viewMatrixFull, rotMatrixYaw, rotMatrixPitch, rotMatrixRoll, posMatrix;
		GetSteamVRPositionalData(&yaw, &pitch, &roll, &x, &y, &z, &viewMatrixFull);
		for (int i = 0; i < 2; i++)
		{
			if (g_ACJoyEmul.joyHandIdx == i)
				// If this controller is also a joystick emulator, then the trigger is only active when
				// the controller isn't gripping the stick.
				g_bACTriggerState[i] = (g_contStates[i].buttons[VRButtons::TRIGGER] && !g_contStates[i].buttons[VRButtons::GRIP]);
			else
				g_bACTriggerState[i] = g_contStates[i].buttons[VRButtons::TRIGGER];
		}

		yaw   *= RAD_TO_DEG * g_fYawMultiplier;
		pitch *= RAD_TO_DEG * g_fPitchMultiplier;
		roll  *= RAD_TO_DEG * g_fRollMultiplier;
		//g_HMDYaw = yaw;
		//g_HMDPitch = pitch;
		//g_HMDRoll = roll;

		// DEBUG
		if (g_bSteamVRYawPitchRollFromMouseLook)
			GetFakeYawPitchRollFromMouseLook(&yaw, &pitch, &roll);
		// DEBUG

		yaw   += g_fYawOffset;
		pitch += g_fPitchOffset;

		// There is no rotation to apply since the full rotation+translation is applied in CockpitLook now.
		g_viewMatrix.identity();

		g_VSMatrixCB.viewMat     = g_viewMatrix;
		g_VSMatrixCB.fullViewMat = viewMatrixFull;

		for (int i = 0; i < 2; i++)
		{
			float disp = g_vrGlovesMeshes[i].visible ?
			             g_vrGlovesMeshes[i].forwardPmeters[VRGlovesProfile::POINT] : 0.0f;
			// Displace the origin so that it's at the tip of the pointing finger
			g_contOriginWorldSpace[i]   = g_contStates[i].pose * (Vector4(0, 0, 0, 1) + disp * g_controllerForwardVector);
			g_contOriginWorldSpace[i].w = 1.0f;
			g_contDirWorldSpace[i]      = g_contStates[i].pose * g_controllerForwardVector;
			g_contDirWorldSpace[i].w    = 0.0f;
		}

		//if (g_bResetHeadCenter)
		//{
		//	for (int i = 0; i < 2; i++)
		//	{
		//		g_contStates[i].centerYaw  = g_contStates[i].yaw;
		//		g_contStates[i].centerRoll = g_contStates[i].roll;
		//		//log_debug("[DBG] [AC] Center YPR captured for controller %d", i);
		//	}
		//}
	}
	else 
	{
		for (int i = 0; i < 2; i++)
			g_contDirWorldSpace[i] = Vector4(0, 0, 1, 0);

		// non-VR and DirectSBS modes, read the roll and position from FreePIE
		//static Vector4 headCenterPos(0, 0, 0, 0);

		// Read yaw/pitch/roll from FreePIE
		if (g_iFreePIESlot > -1 && ReadFreePIE(g_iFreePIESlot)) {
			if (g_bResetHeadCenter) {
				home_yaw   = g_FreePIEData.yaw;
				home_pitch = g_FreePIEData.pitch;
				home_roll  = g_FreePIEData.roll;
				g_bResetHeadCenter = false;
			}
			yaw   =  (g_FreePIEData.yaw   - home_yaw)   * g_fYawMultiplier;
			pitch =  (g_FreePIEData.pitch - home_pitch) * g_fPitchMultiplier;
			roll  =  (g_FreePIEData.roll  - home_roll)  * g_fRollMultiplier;
		}

		if (g_bYawPitchFromMouseOverride) {
			// If FreePIE could not be read, then get the yaw/pitch from the mouse:
			yaw   =  (float)PlayerDataTable[*g_playerIndex].MousePositionX / 32768.0f * 180.0f;
			pitch = -(float)PlayerDataTable[*g_playerIndex].MousePositionY / 32768.0f * 180.0f;
		}

		if (g_bEnableVR) {
			// Compute the rotation matrices with the current yaw, pitch, roll:
			Matrix4 rotMatrixFull, rY, rX, rZ;
			rY.rotateY(-yaw);
			rX.rotateX(-pitch);
			rZ.rotateZ(-roll);
			// This is the same transform chain we have in cockpitlook.cpp::DoRotationPitchHook()
			rotMatrixFull = rY * rX * rZ;

			g_viewMatrix.identity();
			g_VSMatrixCB.viewMat.identity();
			g_VSMatrixCB.fullViewMat = rotMatrixFull;

			// When rendering the concourse, menus, etc, we need the inverse of rotMatrixFull
			if (!g_bUseSteamVR && !g_bRendering3D)
				g_VSMatrixCB.fullViewMat.transpose();
		}
	}

	if (g_bResetHeadCenter)
		g_bResetHeadCenter = false;

	// At this point, yaw, pitch, roll contain the data read from either SteamVR or FreePIE.
}

void PrimarySurface::Add3DVisionSignature()
{
	/*
	static byte data[] =
	{
		0x4e, 0x56, 0x33, 0x44, // NVSTEREO_IMAGE_SIGNATURE         = 0x4433564e
		0x00, 0x0F, 0x00, 0x00, // Screen width * 2 = 1920*2 = 3840 = 0x00000F00
		0x38, 0x04, 0x00, 0x00, // Screen height = 1080             = 0x00000438
		0x20, 0x00, 0x00, 0x00, // dwBPP = 32                       = 0x00000020
		0x03, 0x00, 0x00, 0x00  // dwFlags = SIH_SCALE_TO_FIT       = 0x00000003
	};
	*/

	auto &resources = this->_deviceResources;
	auto &context = resources->_d3dDeviceContext;
	D3D11_MAPPED_SUBRESOURCE map;
	HRESULT hr = context->Map(resources->_vision3DStaging, 0, D3D11_MAP_WRITE, 0, &map);

	if (SUCCEEDED(hr))
	{
		uint32_t LastRow = map.RowPitch * resources->_backbufferHeight;
		
		DWORD dwWidth = resources->_backbufferWidth * 2;
		DWORD dwHeight = resources->_backbufferHeight;
		DWORD dwBPP = 32;
		DWORD dwFlags = 0x02; // Side by Side

		uint8_t *ptr;
		uint8_t signature[] = {
			0x4e, 0x56, 0x33, 0x44,	// NVSTEREO_IMAGE_SIGNATURE
			0x00, 0x00, 0x00, 0x00, // Width
			0x00, 0x00, 0x00, 0x00,	// Height
			0x00, 0x00, 0x00, 0x00,	// BPP
			0x00, 0x00, 0x00, 0x00,	// Flags
		};
		ptr = (uint8_t *)&dwWidth;
		for (int i = 0; i < 4; i++)
			signature[4 + i] = ptr[i];

		ptr = (uint8_t *)&dwHeight;
		for (int i = 0; i < 4; i++)
			signature[8 + i] = ptr[i];

		ptr = (uint8_t *)&dwBPP;
		for (int i = 0; i < 4; i++)
			signature[12 + i] = ptr[i];

		ptr = (uint8_t *)&dwFlags;
		for (int i = 0; i < 4; i++)
			signature[16 + i] = ptr[i];

		if (g_bDumpSSAOBuffers) {
			log_debug("[DBG] [3DV] Add3DVisionSignature resources->backbuffer size: %d, %d",
				resources->_backbufferWidth * 2, resources->_backbufferHeight);
			int Ofs = 0;
			log_debug("[DBG] [3DV] -----------------------------------");
			for (int j = 0; j < 5; j++, Ofs += 4)
				log_debug("[DBG] [3DV] 0x%02x, 0x%02x, 0x%02x, 0x%02x",
					signature[Ofs], signature[Ofs+1], signature[Ofs+2], signature[Ofs+3]);
			log_debug("[DBG] [3DV] ===================================");
		}

		// Debug: the following line draws a thin white line on the top of the image. I'm using
		// this to verify that the signature is being added. We need to remove this line later.
		//memset(map.pData, 0xFF, 10 * 4 * resources->_backbufferWidth);
		// Debug: the following line should write a white line on the last row of the image:
		//memset((char *)((uint32_t)map.pData + LastRow), 0xFF, 4 * resources->_backbufferWidth);

		// Add the 3D vision signature to the last row of the image
		memcpy((byte *)((uint32_t)map.pData + LastRow), signature, 20);
		context->Unmap(resources->_vision3DStaging, 0);
	}
	else {
		if (g_bDumpSSAOBuffers)
			log_debug("[DBG] [3DV] _vision3DStaging could not be mapped: 0x%x", hr);
	}
}

void PrimarySurface::RenderEdgeDetector()
{
	this->_deviceResources->BeginAnnotatedEvent(L"RenderEdgeDetector");

	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	D3D11_VIEWPORT viewport;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const bool bExternalView = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;
	D3D11_DEPTH_STENCIL_DESC desc;
	D3D11_BLEND_DESC blendDesc{};
	ComPtr<ID3D11DepthStencilState> depthState;

	DCElemSrcBox* dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[TARGET_COMP_DC_ELEM_SRC_IDX];
	if (dcElemSrcBox->bComputed) {
		float W = dcElemSrcBox->coords.x1 - dcElemSrcBox->coords.x0;
		float H = dcElemSrcBox->coords.y1 - dcElemSrcBox->coords.y0;

		// Send the UV coords down to the shader as well
		g_ShadertoyBuffer.x0 = dcElemSrcBox->coords.x0;
		g_ShadertoyBuffer.y0 = dcElemSrcBox->coords.y0;
		g_ShadertoyBuffer.x1 = dcElemSrcBox->coords.x1;
		g_ShadertoyBuffer.y1 = dcElemSrcBox->coords.y1;

		viewport.Width = g_fCurScreenWidth * W;
		viewport.Height = g_fCurScreenHeight * H;
		viewport.TopLeftX = g_fCurScreenWidth * dcElemSrcBox->coords.x0;
		viewport.TopLeftY = g_fCurScreenHeight * dcElemSrcBox->coords.y0;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		resources->InitViewport(&viewport);

		// Convert the UVs into in-game UVs for the subCMD bracket
		float x0, y0, x1, y1;
		// Convert screen coords to in-game coords:
		ScreenCoordsToInGame(g_nonVRViewport.TopLeftX, g_nonVRViewport.TopLeftY,
			g_nonVRViewport.Width, g_nonVRViewport.Height,
			viewport.TopLeftX, viewport.TopLeftY,
			&x0, &y0);
		ScreenCoordsToInGame(g_nonVRViewport.TopLeftX, g_nonVRViewport.TopLeftY,
			g_nonVRViewport.Width, g_nonVRViewport.Height,
			viewport.TopLeftX + viewport.Width, viewport.TopLeftY + viewport.Height,
			&x1, &y1);
		//log_debug("[DBG] subCMD box: (%0.3f, %0.3f)-(%0.3f, %0.3f)", x0, y0, x1, y1);
		// Convert to UVs:
		g_ShadertoyBuffer.SunCoords[1].x = x0 / g_fCurInGameWidth;
		g_ShadertoyBuffer.SunCoords[1].y = y0 / g_fCurInGameHeight;
		g_ShadertoyBuffer.SunCoords[1].z = x1 / g_fCurInGameWidth;
		g_ShadertoyBuffer.SunCoords[1].w = y1 / g_fCurInGameHeight;
		// Send the in-game resolution:
		g_ShadertoyBuffer.SunCoords[2].x = 1.0f / g_fCurInGameWidth;
		g_ShadertoyBuffer.SunCoords[2].y = 1.0f / g_fCurInGameHeight;
		// If the Radar2DRenderer is enabled, then we'll put the subCMD bracket coords in
		// SunCoords[3] and set SunCoords[3].w to 1.0. If not, then SunCoords[3].w will be set to 0.
		if (g_config.Radar2DRendererEnabled) {
			float x, y;
			static float pulse = 3.0f;
			pulse += 0.5f;
			if (pulse > 12.0f)
				pulse = 3.0f;
			//log_debug("[DBG] g_SubCMDBracket: %0.3f, %0.3f", g_SubCMDBracket.x, g_SubCMDBracket.y);
			// Convert In-game coords to post-proc UVs
			InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
				(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, g_SubCMDBracket.x, g_SubCMDBracket.y, &x, &y);
			g_ShadertoyBuffer.SunCoords[3].x = x / g_fCurScreenWidth;
			g_ShadertoyBuffer.SunCoords[3].y = y / g_fCurScreenHeight;
			g_ShadertoyBuffer.SunCoords[3].w = 1.0f;
			g_ShadertoyBuffer.iTime = pulse;
		}
		else
			g_ShadertoyBuffer.SunCoords[3].w = 0.0f;
	}
	else
	{
		this->_deviceResources->EndAnnotatedEvent();
		return;
	}

	// SunCoords[1] are the UV coords for the buffer that has the SubCMD when the 2D renderer is disabled
	// SunCoords[2].xy is the in-game resolution
	// SunCoords[3].xy is the center of the SubCMD component if the 2D renderer is enabled

	// SunColor[0].xyz is the color of the wireframe
	// SunColor[0].w is the contrast enhancer
	// SunColor[1] is the luminance vector
	g_ShadertoyBuffer.SunColor[1] = g_DCWireframeLuminance;

	// We need to set the blend state properly
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	//blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO; // This will replace the Dest alpha with the Src alpha, overwriting it
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	resources->InitBlendState(nullptr, &blendDesc);

	// Temporarily disable ZWrite
	desc.DepthEnable = FALSE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	desc.StencilEnable = FALSE;
	resources->InitDepthStencilState(depthState, &desc);

	g_ShadertoyBuffer.iResolution[0] = 1.0f / g_fCurScreenWidth;
	g_ShadertoyBuffer.iResolution[1] = 1.0f / g_fCurScreenHeight;
	// Set a default color for the wireframe
	g_ShadertoyBuffer.SunColor[0].x = 0.1f;
	g_ShadertoyBuffer.SunColor[0].y = 0.1f;
	g_ShadertoyBuffer.SunColor[0].z = 0.5f;

	// Read the IFF of the current target and use it to colorize the wireframe display
	//short currentTargetIndex = PlayerDataTable[*g_playerIndex].currentTargetIndex;
	// Jeremy says I should use this to get the target index:
	short currentTargetIndex = *g_playerInHangar ?
		PlayerDataTable[*g_playerIndex].objectIndex : PlayerDataTable[*g_playerIndex].currentTargetIndex;

	// I think I remember that when currentTargetIndex is 0, the game crashed; but Jeremy
	// just told me that currentTargetIndex can be 0 (it's -1 when no target is selected).
	// So, updating changing this code to allow target 0:
	if (currentTargetIndex >= 0) {
		if (*objects == NULL) goto nocolor;
		ObjectEntry* object = &((*objects)[currentTargetIndex]);
		if (object == NULL) goto nocolor;
		MobileObjectEntry* mobileObject = object->MobileObjectPtr;
		if (mobileObject == NULL) goto nocolor;
		if (g_bGreenAndRedForIFFColorsOnly) {
			if (IsObjectEnemy(currentTargetIndex, PlayerDataTable[*g_playerIndex].team))
				g_ShadertoyBuffer.SunColor[0] = g_DCTargetingFoe;
			else
				g_ShadertoyBuffer.SunColor[0] = g_DCTargetingFriend;
		}
		else {
			int IFF = mobileObject->IFF;
			if (IFF >= 0 && IFF <= 5)
				g_ShadertoyBuffer.SunColor[0] = g_DCTargetingIFFColors[IFF];
		}
	}
nocolor:
	// Override all of the above if the current DC file has a wireframe color set:
	if (g_DCTargetingColor.w > 0.0f) {
		g_ShadertoyBuffer.SunColor[0] = g_DCTargetingColor;
	}
	// Send the contrast data
	g_ShadertoyBuffer.SunColor[0].w = g_DCWireframeContrast;
	// Set the time
	static float time = 0.0f;
	time += 0.1f;
	if (time > 2.0f) time = 0.0f;
	g_ShadertoyBuffer.iTime = time;

	/*
	// The noise effect doesn't look that great on all cockpits, and in VR it becomes really low-res
	// I'm going to disable it, but this block can be used to re-enable it in the future.
	// Check the state of the targeted craft. If it's destroyed, then add some noise to the screen...
	static float destroyedTimer = 0.0f;
	if (currentTargetIndex > -1) {
		ObjectEntry *object = &((*objects)[currentTargetIndex]);
		if (object == NULL) goto reset;
		MobileObjectEntry *mobileObject = object->MobileObjectPtr;
		if (mobileObject == NULL) goto reset;
		CraftInstance *craftInstance = mobileObject->craftInstancePtr;
		if (craftInstance == NULL) goto reset;
		if (craftInstance->CraftState == 3 && !bExternalView) {
			destroyedTimer += 0.005f;
			destroyedTimer = min(destroyedTimer, 1.0f);
		}
		else
			destroyedTimer = 0.0f;
	}
	else {
reset:
		destroyedTimer = 0.0f;
	}
	g_ShadertoyBuffer.twirl = destroyedTimer;
	*/

	// Shut down the CMD if the target has been destroyed. Otherwise we might see some artifacts
	// due to explosions and gas.
	g_ShadertoyBuffer.twirl = 1.0f; // Display the CMD
	if (currentTargetIndex > -1) {
		if (*objects == NULL) goto nochange;
		ObjectEntry* object = &((*objects)[currentTargetIndex]);
		if (object == NULL) goto nochange;
		MobileObjectEntry* mobileObject = object->MobileObjectPtr;
		if (mobileObject == NULL) goto nochange;
		CraftInstance* craftInstance = mobileObject->craftInstancePtr;
		if (craftInstance == NULL) goto nochange;
		if (craftInstance->CraftState == 3)
			g_ShadertoyBuffer.twirl = 0.0f; // Shut down the CMD
	}
nochange:

	resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);

	resources->InitPixelShader(resources->_edgeDetectorPS);
	if (g_bDumpSSAOBuffers) {
		DirectX::SaveWICTextureToFile(context, resources->_offscreenAsInputDynCockpit, GUID_ContainerFormatJpeg,
			L"C:\\Temp\\_edgeDetectorInput.jpg");
	}

	// Apply the edge detector to the DC foreground buffer
	{
		// We don't need to clear the current vertex and pixel constant buffers.
		// Since we've just finished rendering 3D, they should contain values that
		// can be reused. So let's just overwrite the values that we need.
		g_VSCBuffer.aspect_ratio = g_fAspectRatio;
		g_VSCBuffer.z_override = -1.0f;
		g_VSCBuffer.sz_override = -1.0f;
		g_VSCBuffer.mult_z_override = -1.0f;
		g_VSCBuffer.apply_uv_comp = false;
		g_VSCBuffer.bPreventTransform = 0.0f;
		g_VSCBuffer.bFullTransform = 0.0f;
		g_VSCBuffer.viewportScale[0] = 2.0f / resources->_displayWidth;
		g_VSCBuffer.viewportScale[1] = -2.0f / resources->_displayHeight;

		// Since the HUD is all rendered on a flat surface, we lose the vrparams that make the 3D object
		// and text float
		g_VSCBuffer.z_override = 65535.0f;
		g_VSCBuffer.scale_override = 1.0f;

		// Set the left projection matrix (the viewMatrix is set at the beginning of the frame)
		g_VSMatrixCB.projEye[0] = g_FullProjMatrixLeft;
		g_VSMatrixCB.projEye[1] = g_FullProjMatrixRight;
		resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
		resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

		UINT stride = sizeof(D3DTLVERTEX), offset = 0;
		resources->InitVertexBuffer(resources->_hyperspaceVertexBuffer.GetAddressOf(), &stride, &offset);
		resources->InitInputLayout(resources->_inputLayout);
		resources->InitVertexShader(resources->_vertexShader);
		resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		// We need this to ensure backface culling is disabled
		resources->InitRasterizerState(resources->_rasterizerState);

		//context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		// Instead of clearing the RTV, we copy the DC FG buffer to the offscreenBufferPost, that way the
		// viewport only overwrites the section we're going to process.
		context->CopySubresourceRegion(resources->_offscreenBufferPost, 0, 0, 0, 0, resources->_offscreenBufferDynCockpit,0,NULL);
		// Set the RTV:
		ID3D11RenderTargetView* rtvs[1] = {
			resources->_renderTargetViewPost.Get(), // Render to offscreenBufferPost instead of offscreenBuffer
		};
		context->OMSetRenderTargets(1, rtvs, NULL);
		// Set the SRVs:
		resources->InitPSShaderResourceView(resources->_offscreenAsInputDynCockpitSRV);
		context->PSSetShaderResources(1, 1, resources->_mainDisplayTextureView.GetAddressOf());
		context->Draw(6, 0);

		if (g_bDumpSSAOBuffers) {
			DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferPost, GUID_ContainerFormatJpeg,
				L"C:\\Temp\\_edgeDetectorOutput.jpg");
		}
	}

	// Copy or resolve the result
	if (g_config.MultisamplingAntialiasingEnabled)
		context->ResolveSubresource(resources->_offscreenAsInputDynCockpit, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);
	else
		context->CopySubresourceRegion(resources->_offscreenAsInputDynCockpit, 0, 0, 0, 0, resources->_offscreenBufferPost, 0, NULL);

	// Restore previous rendertarget: this line is necessary or the 2D content won't be displayed
	// after applying this effect.
	// The reason we need this is because the original ddraw never expects another RTV other than
	// _renderTargetView, so there isn't an InitRenderTargetView() function -- there's no need since
	// this RTV is always going to be set. If we break that assumption here, things will stop working.
	context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(), NULL);

	// This method may be called multiple times per frame, because the Execute() method may be called
	// multiple times per frame, depending on how many lighting effects are rendered. For instance,
	// laser impacts or the beam effect on the current ship are apparently rendered in Execute(). If
	// multiple such effects are rendered per frame, we'll end up here multiple times too. That's fine,
	// we just need to prevent applying this effect multiple times.
	g_bEdgeEffectApplied = true;

	this->_deviceResources->EndAnnotatedEvent();
}

void PrimarySurface::RenderRTShadowMask()
{
	this->_deviceResources->BeginAnnotatedEvent(L"RenderRTShadowMask");
	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	D3D11_VIEWPORT viewport{};

	// Save the current context
	SaveContext();

	// Reset the viewport for non-VR mode:
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width    = g_fCurScreenWidth  / g_RTConstantsBuffer.RTShadowMaskSizeFactor;
	viewport.Height   = g_fCurScreenHeight / g_RTConstantsBuffer.RTShadowMaskSizeFactor;
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	resources->InitViewport(&viewport);
	SetScissoRectFullScreen();

	// Don't forget to set the lights!
	SetLights(resources, 0.0f);

	// Reset the vertex shader to regular 2D post-process
	// Set the Vertex Shader Constant buffers
	resources->InitVSConstantBuffer2D(resources->_mainShadersConstantBuffer.GetAddressOf(),
		0.0f, 1.0f, 1.0f, 1.0f, 0.0f); // Do not use 3D projection matrices

	// Set/Create the VertexBuffer and set the topology, etc
	UINT stride = sizeof(MainVertex), offset = 0;
	resources->InitVertexBuffer(resources->_postProcessVertBuffer.GetAddressOf(), &stride, &offset);
	resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	resources->InitInputLayout(resources->_mainInputLayout);

	resources->InitVertexShader(g_bUseSteamVR ? resources->_mainVertexShaderVR : resources->_mainVertexShader);
	resources->InitPixelShader(g_bUseSteamVR ? resources->_rtShadowMaskPS_VR : resources->_rtShadowMaskPS);

	// Clear all the render target views
	ID3D11RenderTargetView* rtvs_null[5] = {
		NULL, // Main RTV
		NULL, // Bloom
		NULL, // Depth
		NULL, // Norm Buf
		NULL, // SSAO Mask
	};
	context->OMSetRenderTargets(5, rtvs_null, NULL);
	context->ClearRenderTargetView(resources->_rtShadowMaskRTV, bgColor);

	// I need to Resolve by Resolves... I'm pretty sure some of these are redundant:
	context->ResolveSubresource(resources->_normBuf, 0, resources->_normBufMSAA, 0, AO_DEPTH_BUFFER_FORMAT);
	context->ResolveSubresource(resources->_depthBufAsInput, 0, resources->_depthBuf, 0, AO_DEPTH_BUFFER_FORMAT);
	if (g_bUseSteamVR) {
		context->ResolveSubresource(resources->_normBuf, D3D11CalcSubresource(0, 1, 1),
			resources->_normBufMSAA, D3D11CalcSubresource(0, 1, 1), AO_DEPTH_BUFFER_FORMAT);
		context->ResolveSubresource(resources->_depthBufAsInput, D3D11CalcSubresource(0, 1, 1),
			resources->_depthBuf, D3D11CalcSubresource(0, 1, 1), AO_DEPTH_BUFFER_FORMAT);
	}

	// Set the SRVs:
	ID3D11ShaderResourceView* srvs[2] = {
		resources->_normBufSRV.Get(),
		resources->_depthBufSRV.Get(),
	};
	context->PSSetShaderResources(0, 2, srvs);

	// Set the BVH SRVs:
	ID3D11ShaderResourceView* srvs2[] = {
		resources->_RTBvhSRV.Get(),          // 14
		resources->_RTMatricesSRV.Get(),     // 15
		resources->_RTTLASBvhSRV.Get(),      // 16
		NULL,                                // 17 Unbind the rtShadowMask set during lighting pass to avoid a D3D warning below
	};
	// Slots 14-17 are used for Raytracing buffers (BLASes, Matrices, TLAS)
	context->PSSetShaderResources(14, 4, srvs2);

	ID3D11RenderTargetView* rtvs[1] = {
		resources->_rtShadowMaskRTV.Get(),
	};
	context->OMSetRenderTargets(1, rtvs, NULL);

	if (g_bUseSteamVR)
		context->DrawInstanced(6, 2, 0, 0); // if (g_bUseSteamVR)
	else
		context->Draw(6, 0);

	// Post-process the right image
	//if (g_bUseSteamVR) {
	//	context->ClearRenderTargetView(resources->_rtShadowMaskRTV_R, bgColor);
	//	ID3D11RenderTargetView* rtvs[1] = {
	//		resources->_rtShadowMaskRTV_R.Get(),
	//	};
	//	context->OMSetRenderTargets(1, rtvs, NULL);

	//	// Set the SRVs:
	//	ID3D11ShaderResourceView* srvs[2] = {
	//		resources->_normBufSRV_R.Get(),
	//		resources->_depthBufSRV_R.Get(),
	//	};
	//	context->PSSetShaderResources(0, 2, srvs);
	//	context->Draw(6, 0);
	//}

	if (g_bDumpSSAOBuffers)
	{
		DirectX::SaveDDSTextureToFile(context, resources->_rtShadowMask, L"C:\\Temp\\_rtShadowMask.dds");
	}

	// Restore the previous context
	RestoreContext();
	this->_deviceResources->EndAnnotatedEvent();
}

HRESULT PrimarySurface::Flip(
	LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride,
	DWORD dwFlags
	)
{
	this->_deviceResources->BeginAnnotatedEvent(L"PrimarySurfaceFlip");

	static uint64_t frame, lastFrame = 0;
	static float seconds;

#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;

	if (lpDDSurfaceTargetOverride == nullptr)
	{
		str << " NULL";
	}
	else if (lpDDSurfaceTargetOverride == this->_deviceResources->_backbufferSurface)
	{
		str << " BackbufferSurface";
	}
	else if (lpDDSurfaceTargetOverride == this->_deviceResources->_frontbufferSurface)
	{
		str << " FrontbufferSurface";
	}
	else
	{
		str << " " << lpDDSurfaceTargetOverride;
	}

	if (dwFlags & DDFLIP_WAIT)
	{
		str << " WAIT";
	}

	LogText(str.str());
#endif

	auto &resources = this->_deviceResources;
	auto &context = resources->_d3dDeviceContext;
	auto &device = resources->_d3dDevice;
	this->_deviceResources->sceneRenderedEmpty = this->_deviceResources->sceneRendered == false;
	this->_deviceResources->sceneRendered = false;
	bool bHyperspaceFirstFrame = g_bHyperspaceFirstFrame; // Used to clear the shadowMap DSVs *after* they're used

	/* Display VR movies */
	if (g_bUseSteamVR && g_pSharedDataTgSmush != nullptr &&
		g_pSharedDataTgSmush->videoFrameIndex > 0)
	{
		// Create or resize the TgSmush texture. If we already created this texture and there's
		// no change in dimensions, CreateTgSmushTexture() will do nothing.
		resources->CreateTgSmushTexture(g_pSharedDataTgSmush->videoFrameWidth, g_pSharedDataTgSmush->videoFrameHeight);

		if (resources->_tgSmushTex != nullptr && g_VR2Doverlay != vr::k_ulOverlayHandleInvalid)
		{
			static int lastFrameRendered = -1;
			vr::Texture_t overlay_texture;

			// Avoid mapping the resource if the frame hasn't changed.
			if (lastFrameRendered != g_pSharedDataTgSmush->videoFrameIndex) {
				D3D11_MAPPED_SUBRESOURCE map;
				HRESULT hr = context->Map(resources->_tgSmushTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
				if (SUCCEEDED(hr)) {
					uint32_t size = g_pSharedDataTgSmush->videoFrameWidth * g_pSharedDataTgSmush->videoFrameHeight * 4;
					// Copy the data from TgSmush into this texture
					memcpy(map.pData, g_pSharedDataTgSmush->videoDataPtr, size);
					//memcpy(map.pData, g_pSharedDataTgSmush->videoDataPtr, g_pSharedDataTgSmush->videoDataLength);
					context->Unmap(resources->_tgSmushTex, 0);
				}
				lastFrameRendered = g_pSharedDataTgSmush->videoFrameIndex;
			}

			overlay_texture.eType = vr::TextureType_DirectX;
			overlay_texture.eColorSpace = vr::ColorSpace_Auto;
			overlay_texture.handle = resources->_tgSmushTex;
			// Fade compositor to black while the overlay is shown and we are not rendering the 3D scene.
			g_pVRCompositor->FadeToColor(0.1f, 0.0f, 0.0f, 0.0f, 1.0f, false);
			g_pVROverlay->SetOverlayTexture(g_VR2Doverlay, &overlay_texture);
			// Let's make movies larger than the regular 2D overlay:
			//g_pVROverlay->SetOverlayWidthInMeters(g_VR2Doverlay, 10.0f);
			g_pVROverlay->ShowOverlay(g_VR2Doverlay); // Display VR movies
		}
	}

	if (g_pSharedDataTgSmush != nullptr && g_pSharedDataTgSmush->videoFrameIndex > 0)
		// Early exit in case Flip is being called from Tgsmush to avoid doing anything unnecessary that can cause crashes.
		return DD_OK;

	if (this->_deviceResources->sceneRenderedEmpty && this->_deviceResources->_frontbufferSurface != nullptr && this->_deviceResources->_frontbufferSurface->wasBltFastCalled)
	{
		if (!this->_deviceResources->IsInConcourseHd()) {
			if (!g_bHyperspaceFirstFrame) {
				context->ClearRenderTargetView(resources->_renderTargetView, resources->clearColor);
				context->ClearRenderTargetView(resources->_shadertoyRTV, resources->clearColorRGBA);
			}
			context->ClearRenderTargetView(resources->_renderTargetViewPost, resources->clearColorRGBA);
			if (g_b3DVisionEnabled)
				context->ClearRenderTargetView(resources->_RTVvision3DPost, resources->clearColorRGBA);
			if (g_bUseSteamVR) {
				if (!g_bHyperspaceFirstFrame) {
					context->ClearRenderTargetView(resources->_renderTargetViewR, resources->clearColor);
					context->ClearRenderTargetView(resources->_shadertoyRTV_R, resources->clearColorRGBA);
					context->ClearRenderTargetView(resources->_renderTargetViewHd, resources->clearColor);
				}
				context->ClearRenderTargetView(resources->_renderTargetViewPostR, resources->clearColorRGBA);
			}
		}

		if (!g_bHyperspaceFirstFrame) {
			context->ClearDepthStencilView(resources->_depthStencilViewL, D3D11_CLEAR_DEPTH, resources->clearDepth, 0);
			if (g_bUseSteamVR)
				context->ClearDepthStencilView(resources->_depthStencilViewR, D3D11_CLEAR_DEPTH, resources->clearDepth, 0);
		}
	}

	/* Present 2D content */
	if (lpDDSurfaceTargetOverride != nullptr)
	{
		if (lpDDSurfaceTargetOverride == this->_deviceResources->_frontbufferSurface)
		{
			HRESULT hr;

			if (FAILED(hr = this->_deviceResources->_backbufferSurface->BltFast(0, 0, this->_deviceResources->_frontbufferSurface, nullptr, 0)))
			{
				this->_deviceResources->EndAnnotatedEvent();
				return hr;
			}

			this->_deviceResources->EndAnnotatedEvent();
			return this->Flip(this->_deviceResources->_backbufferSurface, 0);
		}

		/* Present 2D content */
		if (lpDDSurfaceTargetOverride == this->_deviceResources->_backbufferSurface)
		{
			bool isInConcourseHd = this->_deviceResources->IsInConcourseHd();
			g_bInTechRoom = (g_iDrawCounter > 0);
			g_iDrawCounter = 0;

			//if (g_bInTechRoom) log_debug("[DBG] IN TECH ROOM");
			// If we don't have the metric params ready by the time the Tech Room is presented,
			// then nothing will show up, so it's better to initialize the params (with default
			// values) just in case we go into the tech room before we load any mission.
			// This is only needed in VR mode:
			if (g_bEnableVR && g_bInTechRoom)
			{
				//float y_center_temp = g_MetricRecCBuffer.mr_y_center;
				*g_fRawFOVDist = 256.0f;
				*g_cachedFOVDist = *g_fRawFOVDist / 512.0f;
				*g_rawFOVDist = (uint32_t)*g_fRawFOVDist;
				//g_bYCenterHasBeenFixed = false; // Force the recomputation of YCenter when a mission loads
				g_bCustomFOVApplied = false;

				// Compute the metric scale factor conversion
				g_fOBJCurMetricScale = g_fCurInGameHeight * g_fOBJGlobalMetricMult / (SHADOW_OBJ_SCALE * 3200.0f);
				
				g_MetricRecCBuffer.mr_y_center = 0.0f;
				g_MetricRecCBuffer.mr_cur_metric_scale = g_fOBJCurMetricScale;
				g_MetricRecCBuffer.mr_aspect_ratio = g_fCurInGameAspectRatio;
				if (g_bSteamVREnabled) {
					// Compensate the distortion that happens when rendering the 3D craft over the 2D background in the tech room. 
					//g_MetricRecCBuffer.mr_aspect_ratio = ((float)g_WindowHeight / (float)g_WindowWidth) * g_fConcourseAspectRatio * ((float)g_steamVRHeight / (float)g_steamVRWidth);
					g_MetricRecCBuffer.mr_aspect_ratio = 0.5f;
					//g_MetricRecCBuffer.mr_aspect_ratio = g_fCurInGameAspectRatio * (((float)g_WindowHeight / (float)g_WindowWidth) * (4.0f / 3.0f));
				}
				g_MetricRecCBuffer.mr_z_metric_mult = g_fOBJ_Z_MetricMult;
				g_MetricRecCBuffer.mr_shadow_OBJ_scale = SHADOW_OBJ_SCALE;

				// We need to set these CBs here if the Tech Room is displayed before any mission has set the FOV params
				resources->InitVSConstantBufferMetricRec(resources->_metricRecVSConstantBuffer.GetAddressOf(), &g_MetricRecCBuffer);
				resources->InitPSConstantBufferMetricRec(resources->_metricRecPSConstantBuffer.GetAddressOf(), &g_MetricRecCBuffer);
				// Restore the previous y_center:
				//g_MetricRecCBuffer.mr_y_center = y_center_temp;
				//log_debug("[DBG] Initializing 2D Metric Params");
			}

			// Read yaw,pitch,roll from SteamVR/FreePIE and apply the rotation to the 2D content
			// on the next frame
			UpdateViewMatrix();
#ifdef DISABLED
			//if (g_bEnableVR)
			{
				float yaw, pitch, roll, x,y,z;
				Matrix3 rotMatrix;

				if (g_bUseSteamVR) {
					GetSteamVRPositionalData(&yaw, &pitch, &roll, &x, &y, &z, &rotMatrix);
					// We need to multiply the yaw and pitch in SteamVR mode to invert the rotation
					yaw   *= RAD_TO_DEG * g_fYawMultiplier * -1.0f;
					pitch *= RAD_TO_DEG * g_fPitchMultiplier * -1.0f;
					roll  *= RAD_TO_DEG * g_fRollMultiplier;
					yaw   += g_fYawOffset;
					pitch += g_fPitchOffset;
				}
				else {
					// DirectSBS case
					static float home_yaw = 0.0f, home_pitch = 0.0f, home_roll = 0.0f;
					// Read yaw/pitch/roll from FreePIE
					if (g_iFreePIESlot > -1 && ReadFreePIE(g_iFreePIESlot)) {
						if (g_bResetHeadCenter) {
							home_yaw   = g_FreePIEData.yaw;
							home_pitch = g_FreePIEData.pitch;
							home_roll  = g_FreePIEData.roll;
							g_bResetHeadCenter = false;
						}
						yaw   = (g_FreePIEData.yaw   - home_yaw)   * g_fYawMultiplier;
						pitch = (g_FreePIEData.pitch - home_pitch) * g_fPitchMultiplier;
						roll  = (g_FreePIEData.roll  - home_roll)  * g_fRollMultiplier;
					}
				}

				// DEBUG
				/*
				GetFakeYawPitchRollFromKeyboard(&yaw, &pitch, &roll);
				*/
				// DEBUG

				// Compute the full rotation
				Matrix4 rotMatrixFull, rotMatrixYaw, rotMatrixPitch, rotMatrixRoll;
				rotMatrixFull.identity();
				rotMatrixYaw.identity();   rotMatrixYaw.rotateY(g_f2DYawMul * yaw);
				rotMatrixPitch.identity(); rotMatrixPitch.rotateX(g_f2DPitchMul * pitch);
				rotMatrixRoll.identity();  rotMatrixRoll.rotateZ(g_f2DRollMul * roll);
				rotMatrixFull = rotMatrixRoll * rotMatrixPitch * rotMatrixYaw;

				g_VSMatrixCB.fullViewMat = rotMatrixFull;
			}
#endif

			if (!isInConcourseHd && this->_deviceResources->_frontbufferSurface == nullptr)
			{
				if (FAILED(this->_deviceResources->RenderMain(this->_deviceResources->_backbufferSurface->_buffer, this->_deviceResources->_displayWidth, this->_deviceResources->_displayHeight, this->_deviceResources->_displayBpp)))
				{
					this->_deviceResources->EndAnnotatedEvent();
					return DDERR_GENERIC;
				}
			}
			else if (!isInConcourseHd)
			{
				const unsigned short colorKey = 0x8080;

				int width = 640;
				int height = 480;

				int topBlack = 0;

				if (this->_deviceResources->_displayBpp == 2)
				{
					int w = this->_deviceResources->_displayWidth;
					int x = (this->_deviceResources->_displayWidth - width) / 2;
					int y = (this->_deviceResources->_displayHeight - height) / 2;
					unsigned short* buffer = (unsigned short*)this->_deviceResources->_backbufferSurface->_buffer + y * w + x;

					for (int i = 0; i < height / 2; i++)
					{
						if (buffer[i * w] != colorKey)
							break;

						topBlack++;
					}
				}
				else
				{
					const unsigned int colorKey32 = convertColorB5G6R5toB8G8R8X8(colorKey);

					int w = this->_deviceResources->_displayWidth;
					int x = (this->_deviceResources->_displayWidth - width) / 2;
					int y = (this->_deviceResources->_displayHeight - height) / 2;
					unsigned int* buffer = (unsigned int*)this->_deviceResources->_backbufferSurface->_buffer + y * w + x;

					for (int i = 0; i < height / 2; i++)
					{
						if (buffer[i * w] != colorKey32)
							break;

						topBlack++;
					}
				}

				if (topBlack < height / 2)
				{
					height -= topBlack * 2;
				}

				RECT rc;
				rc.left = (this->_deviceResources->_displayWidth - width) / 2;
				rc.top = (this->_deviceResources->_displayHeight - height) / 2;
				rc.right = rc.left + width;
				rc.bottom = rc.top + height;

				int length = width * height * this->_deviceResources->_displayBpp;
				char* buffer = new char[length];

				copySurface(buffer, width, height, this->_deviceResources->_displayBpp, this->_deviceResources->_backbufferSurface->_buffer, this->_deviceResources->_displayWidth, this->_deviceResources->_displayHeight, this->_deviceResources->_displayBpp, 0, 0, &rc, false);

				HRESULT hr = this->_deviceResources->RenderMain(buffer, width, height, this->_deviceResources->_displayBpp);

				delete[] buffer;

				if (FAILED(hr))
				{
					this->_deviceResources->EndAnnotatedEvent();
					return DDERR_GENERIC;
				}
			}

			HRESULT hr;

			/* Present Concourse, ESC Screen Menu */
			if (this->_deviceResources->_swapChain)
			{
				// If we're in the Tech Room, chances are the ship displayed has a cockpit with transparency.
				// In that case, we'd have stored all those draw calls for deferred rendering. This is where
				// we can finally execute those calls.
				RenderDeferredDrawCalls();

				// 2D path final post-processing step. Copied from ReShade's Levels.fx
				if (g_bEnableLevelsShader) {
					RenderLevels();
				}

				UINT rate = 24 * this->_deviceResources->_refreshRate.Denominator;
				UINT numerator = this->_deviceResources->_refreshRate.Numerator + this->_flipFrames;
				UINT interval = numerator / rate;
				this->_flipFrames = numerator % rate;

				interval = max(interval, 1);

				hr = DD_OK;

				interval = g_iNaturalConcourseAnimations ? interval : 1;
				if (g_iNaturalConcourseAnimations > 1)
					interval = g_iNaturalConcourseAnimations;

				for (UINT i = 0; i < interval; i++)
				{
					// In the original code the offscreenBuffer is simply resolved into the backBuffer.
					// this->_deviceResources->_d3dDeviceContext->ResolveSubresource(this->_deviceResources->_backBuffer, 0, this->_deviceresources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);

					if (!g_bDisableBarrelEffect && g_bEnableVR && !g_bUseSteamVR) {
						// Barrel effect enabled for DirectSBS mode.
						// If 3D vision is enabled, then we use a different shader that will expand _offscreenBuffer into
						// _vision3DPost.
						barrelEffect2D(i);
						if (g_b3DVisionEnabled) {
							// Resolve _vision3DPost, which is MSAA, into _vision3DNoMSAA. We can't resolve _vision3DPost directly
							// into _vision3DStaging because the flags used to create that last buffer won't allow it. So we need
							// this intermediate buffer (_vision3DNoMSAA) to help us get the data into the staging buffer.
							context->ResolveSubresource(resources->_vision3DNoMSAA, 0, resources->_vision3DPost, 0, BACKBUFFER_FORMAT);
							//context->CopyResource(resources->_vision3DStaging, resources->_vision3DNoMSAA);
							// Add the 3D vision signature to _vision3DStaging
							//Add3DVisionSignature();

							// The staging buffer now has the signature, we can copy it to the backbuffer
							D3D11_BOX box;
							box.left = 0; box.right = resources->_backbufferWidth;
							box.top = 0; box.bottom = resources->_backbufferHeight;
							box.front = 0; box.back = 1;
							context->CopySubresourceRegion(resources->_backBuffer, D3D11CalcSubresource(0, 0, 1), 0, 0, 0, resources->_vision3DNoMSAA, 0, &box);
							//context->CopySubresourceRegion(resources->_backBuffer, D3D11CalcSubresource(0, 0, 1), 0, 0, 0, resources->_vision3DStaging, 0, &box);

							// DEBUG: Just copy _vision3DPost (without the signature) to _offscreenBufferPost and resolve.
							//context->CopySubresourceRegion(resources->_offscreenBufferPost, D3D11CalcSubresource(0, 0, 1), 0, 0, 0, resources->_vision3DPost, 0, &box);
							//context->ResolveSubresource(resources->_backBuffer, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);

							if (g_bDumpSSAOBuffers) {
								log_debug("[DBG] [3DV] Dumping _vision3DNoMSAA...");
								//DirectX::SaveWICTextureToFile(context, resources->_vision3DPost, GUID_ContainerFormatPng, L"C:\\Temp\\_vision3DPost.png");
								DirectX::SaveWICTextureToFile(context, resources->_vision3DNoMSAA, GUID_ContainerFormatPng, L"C:\\Temp\\_vision3DNoMSAA.png");
								DirectX::SaveDDSTextureToFile(context, resources->_vision3DNoMSAA, L"C:\\Temp\\_vision3DNoMSAA.dds");
								//DirectX::SaveWICTextureToFile(context, resources->_vision3DStaging, GUID_ContainerFormatPng, L"C:\\Temp\\_vision3DStaging.png");
							}
						}
						else {
							context->ResolveSubresource(resources->_backBuffer, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);
						}
					}
					else {
						// In SteamVR mode this will display the left image:
						if (g_bUseSteamVR) {
							if (g_bDumpSSAOBuffers)
							{
								DirectX::SaveDDSTextureToFile(context, resources->_offscreenBufferHd, L"C:\\Temp\\_offscreenBufferHd.dds");
								DirectX::SaveDDSTextureToFile(context, resources->_steamVROverlayBuffer, L"C:\\Temp\\_steamVROverlayBuffer.dds");
							}

							if (g_VR2Doverlay != vr::k_ulOverlayHandleInvalid)
							{
								vr::Texture_t overlay_texture;
								overlay_texture.eType = vr::TextureType_DirectX;
								overlay_texture.eColorSpace = vr::ColorSpace_Auto;
								overlay_texture.handle = resources->_steamVROverlayBuffer;
								// Fade compositor to black while the overlay is shown and we are not rendering the 3D scene.
								g_pVRCompositor->FadeToColor(0.1f, 0.0f, 0.0f, 0.0f, 1.0f, false);
								g_pVROverlay->SetOverlayTexture(g_VR2Doverlay, &overlay_texture);
								g_pVROverlay->SetOverlayWidthInMeters(g_VR2Doverlay, g_fSteamVROverlayWidthInMeters);
								g_pVROverlay->ShowOverlay(g_VR2Doverlay); // Present 2D
							}

							resizeForSteamVR(0, true);
							context->ResolveSubresource(resources->_backBuffer, 0, resources->_steamVRPresentBuffer, 0, BACKBUFFER_FORMAT);
						}
						else {
							// Non-VR path
							context->ResolveSubresource(resources->_backBuffer, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
						}
					}
					
					// Enable capturing 3D objects even when rendering 2D. This happens in the Tech Library
#ifdef DBG_VR
					if (g_bStart3DCapture && !g_bDo3DCapture) {
						g_bDo3DCapture = true;
					}
					else if (g_bStart3DCapture && g_bDo3DCapture) {
						g_bDo3DCapture = false;
						g_bStart3DCapture = false;
						fclose(g_HackFile);
						g_HackFile = NULL;
					}
#endif

					// Reset the 2D draw counter -- that'll help us increase the parallax for the Tech Library
					g_iDraw2DCounter = 0;				
					SetPresentCounter(0, 0);
					if (g_bUseSteamVR) {
						// HACK ALERT:
						// We're calling UpdateViewMatrix here and then running the mouse emulation code so that
						// the mouse emulation gets refreshed at least 25fps. If we don't do this, the emulation is
						// too choppy.
						UpdateViewMatrix();
						EmulMouseWithVRControllers();

						/*vr::EVRCompositorError error = vr::VRCompositorError_None;
						vr::Texture_t leftEyeTexture = { this->_deviceResources->_offscreenBuffer.Get(), vr::TextureType_DirectX, vr::ColorSpace_Auto };
						vr::Texture_t rightEyeTexture = { this->_deviceResources->_offscreenBufferR.Get(), vr::TextureType_DirectX, vr::ColorSpace_Auto };
						error = g_pVRCompositor->Submit(vr::Eye_Left, &leftEyeTexture);
						error = g_pVRCompositor->Submit(vr::Eye_Right, &rightEyeTexture);*/
					}
					
					g_bRendering3D = false;
					g_iD3DExecuteCounter = 0; // Reset the draw call counter for the D3DRendererHook
					if (g_bDumpSSAOBuffers) {
						DirectX::SaveDDSTextureToFile(context, resources->_offscreenBuffer, L"c:\\temp\\_backgroundBuffer.dds");
						DirectX::SaveDDSTextureToFile(context, resources->_normBuf, L"C:\\Temp\\_normBuf.dds");
						DirectX::SaveDDSTextureToFile(context, resources->_depthBufAsInput, L"C:\\Temp\\_depthBuf.dds");
						DirectX::SaveDDSTextureToFile(context, resources->_depthStencilL, L"C:\\Temp\\_depthStencilL.dds");
						DirectX::SaveDDSTextureToFile(context, resources->_offscreenBufferAsInputBloomMask, L"C:\\Temp\\_bloomMask1.dds");
						DirectX::SaveDDSTextureToFile(context, resources->_offscreenBuffer, L"C:\\Temp\\_offscreenBuf.dds");
						DirectX::SaveDDSTextureToFile(context, resources->_ssaoMask, L"C:\\Temp\\_ssaoMask.dds");
						DirectX::SaveDDSTextureToFile(context, resources->_ssMask, L"C:\\Temp\\_ssMask.dds");
						log_debug("[DBG] [AO] Captured debug buffers");

						g_bDumpSSAOBuffers = false;
					}

					//if (g_bDumpOptNodes)
					//	g_bDumpOptNodes = false;

					if (g_iDelayedDumpDebugBuffers) {
						g_iDelayedDumpDebugBuffers--;
						if (g_iDelayedDumpDebugBuffers == 0)
							g_bDumpSSAOBuffers = true;
					}

					if (g_fCurInGame2DWidth < 0)
					{
						g_fCurInGame2DWidth = g_fCurInGameWidth;
						g_fCurInGame2DHeight = g_fCurInGameHeight;
					}

					//g_HyperspacePhaseFSM = HS_INIT_ST; // Resetting the hyperspace state when presenting a 2D image messes up the state
					// This is because the user can press [ESC] to display the menu while in hyperspace and that's a 2D present.
					// Present 2D
					//log_debug("[DBG] ------------------- Present 2D");
					if (FAILED(hr = this->_deviceResources->_swapChain->Present(g_iNaturalConcourseAnimations, 0)))
					{
						static bool messageShown = false;
						
						if (!messageShown)
						{
							MessageBox(nullptr, _com_error(hr).ErrorMessage(), __FUNCTION__, MB_ICONERROR);
						}

						messageShown = true;

						hr = DDERR_SURFACELOST;
						break;
					}

					if (g_bUseSteamVR) {
						if (g_bTogglePostPresentHandoff)
							g_pVRCompositor->PostPresentHandoff();
					}
				}

				// Clear the RTVs for the next frame
				{
					// We're about to switch from 3D to 2D rendering.
					// Let's clear the render target for the next iteration or we'll get multiple images during
					// the animation
					auto &context = this->_deviceResources->_d3dDeviceContext;
					float bgColor[4] = { 0, 0, 0, 0 };
					context->ClearRenderTargetView(resources->_renderTargetView, bgColor);
					if (g_bUseSteamVR) {
						context->ClearRenderTargetView(resources->_renderTargetViewR, bgColor);
						//context->ClearRenderTargetView(resources->_renderTargetViewSteamVRResize, bgColor);
					}
				}

			}
			else
			{
				hr = DD_OK;
			}

			if (this->_deviceResources->_frontbufferSurface != nullptr)
			{
				if (this->_deviceResources->_frontbufferSurface->wasBltFastCalled)
				{
					memcpy(this->_deviceResources->_frontbufferSurface->_buffer2, this->_deviceResources->_frontbufferSurface->_buffer, this->_deviceResources->_frontbufferSurface->_bufferSize);
				}

				if (g_config.HDConcourseEnabled)
				{
					ConcourseTakeScreenshot();
				}

				this->_deviceResources->_frontbufferSurface->wasBltFastCalled = false;
			}

			if (g_config.HDConcourseEnabled)
			{
				const int currentGameState = *(int*)(0x009F60E0 + 0x25FA9);
				const int updateCallback = *(int*)(0x009F60E0 + 0x25FB1 + 0x850 * currentGameState + 0x0844);
				const bool isConfigMenuGameStateUpdate = updateCallback == 0x0051D100;

				if (!isConfigMenuGameStateUpdate && this->_deviceResources->IsInConcourseHd())
				{
					if (g_bUseSteamVR)
						this->_deviceResources->_d3dDeviceContext->CopyResource(this->_deviceResources->_offscreenBufferHd, this->_deviceResources->_offscreenBufferHdBackground);
					else
						this->_deviceResources->_d3dDeviceContext->CopyResource(this->_deviceResources->_offscreenBuffer, this->_deviceResources->_offscreenBufferHdBackground);
				}
			}
			else
			{
				this->_deviceResources->_d3dDeviceContext->ClearRenderTargetView(this->_deviceResources->_renderTargetView, this->_deviceResources->clearColor);
			}

			this->_deviceResources->EndAnnotatedEvent();
			return hr;
		}
	}
	/* Present 3D content */
	else
	{
		HRESULT hr;
		auto &resources = this->_deviceResources;
		auto &context = resources->_d3dDeviceContext;
		auto &device = resources->_d3dDevice;
		const bool bExternalCamera = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;
		const bool bCockpitDisplayed = PlayerDataTable[*g_playerIndex].cockpitDisplayed;
		const int cameraObjIdx = PlayerDataTable[*g_playerIndex].Camera.CraftIndex;
		int FlyByCameraTime = PlayerDataTable[*g_playerIndex].Camera.FlyByCameraTime;

		// This moves the external camera when in the hangar:
		//static __int16 yaw = 0;
		//PlayerDataTable[*g_playerIndex].Camera.Yaw   += 40 * *mouseLook_X;
		//PlayerDataTable[*g_playerIndex].Camera.Pitch += 15 * *mouseLook_Y;
		//yaw += 600;
		//log_debug("[DBG] roll: %0.3f", PlayerDataTable[*g_playerIndex].roll / 32768.0f * 180.0f);
		/*
		if (*g_playerInHangar) 
		{
			static int yaw = 0;
			yaw += 256;
			log_debug("[DBG] yaw: %0.3f", (float)yaw / 65536.0f);
			if (bExternalCamera) {
				PlayerDataTable[*g_playerIndex].Camera.Yaw = yaw;
			}
			else {
				PlayerDataTable[*g_playerIndex].cockpitCameraYaw = yaw;
			}

			if (yaw > 65535)
				yaw -= 65535;
		}
		*/

		if (this->_deviceResources->_swapChain)
		{
			hr = DD_OK;

			// If there's a targeted object, we probably already rendered all the deferred lasers draw calls.
			// However, if the GUI is hidden, there may be some outstanding draw calls. Here we render those
			// calls in that case. If there's nothing to draw, no harm done!
			// Render any outstanding deferred calls
			RenderDeferredDrawCalls();

			// Clear the DC RTVs -- we should've done this during Execute(); but if the GUI was disabled
			// then we didn't do it!
			if (!g_bDCWasClearedOnThisFrame) {
				float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				context->ClearRenderTargetView(resources->_renderTargetViewDynCockpit, bgColor);
				context->ClearRenderTargetView(resources->_renderTargetViewDynCockpitBG, bgColor);
				context->ClearRenderTargetView(resources->_DCTextRTV, bgColor);
				g_bDCWasClearedOnThisFrame = true;
				//log_debug("[DBG] DC Clearing RTVs because GUI was off");
			}

			if (g_bDumpSSAOBuffers) {
				DirectX::SaveWICTextureToFile(context, resources->_ReticleBufMSAA, GUID_ContainerFormatPng,
					L"C:\\Temp\\_reticleBufMSAA.png");
			}

			//this->RenderBracket(); // Don't render the bracket yet, wait until all the shading has been applied
			// We can render the enhanced radar and text now; because they will go to DCTextBuf -- not directly to the screen
			if (g_config.Radar2DRendererEnabled)
				this->RenderRadar();

			if (g_config.Text2DRendererEnabled) {
				if (g_bRenderLaserIonEnergyLevels || g_bRenderThrottle)
					this->RenderSynthDCElems();
			}
			
			if (g_config.Text2DRendererEnabled) {
				/*
				short x = 250, y = 260;
				x = AddText("Hello ", 0, x, y, 0x5555FF);
				x = AddText("Weird ", 1, x, y, 0x5555FF);
				x = AddText("World ", 2, x, y, 0x5555FF);
				*/
				//AddCenteredText("Hello World", FONT_LARGE_IDX, 260, 0x5555FF);
				// The following text gets captured as part of the missile count DC element:
				//AddCenteredText("XXXXXXXXXXXXXXXXXXXXXXXX", FONT_LARGE_IDX, 17, 0x5555FF);
				this->RenderText();
			}

			// If we didn't render any GUI/HUD elements so far, then here's the place where we do some management
			// of the state that is normally done in Execute():
			// We capture the frame-so-far into shadertoyAuxBuf -- this is needed for the hyperspace effect.
			if (!g_bSwitchedToGUI) 
			{
				g_bSwitchedToGUI = true;
				// Capture the background/current-frame-so-far for the new hyperspace effect; but only if we're
				// not travelling through hyperspace:
				if (g_bHyperDebugMode || g_HyperspacePhaseFSM == HS_INIT_ST || g_HyperspacePhaseFSM == HS_POST_HYPER_EXIT_ST)
				{
					g_fLastCockpitCameraYaw   = PlayerDataTable[*g_playerIndex].MousePositionX;
					g_fLastCockpitCameraPitch = PlayerDataTable[*g_playerIndex].MousePositionY;
					g_lastCockpitXReference	  = PlayerDataTable[*g_playerIndex].Camera.ShakeX;
					g_lastCockpitYReference   = PlayerDataTable[*g_playerIndex].Camera.ShakeY;
					g_lastCockpitZReference   = PlayerDataTable[*g_playerIndex].Camera.ShakeZ;

					context->ResolveSubresource(resources->_shadertoyAuxBuf, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
					if (g_bUseSteamVR)
						context->ResolveSubresource(
							resources->_shadertoyAuxBuf, D3D11CalcSubresource(0, 1, 1),
							resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
				}
			}

			// Here we're only tagging and fading lights, the shadowmap is now rendered in EffectsRenderer::RenderShadowMap()
			// Lights should also be tagged when Raytracing is enabled, or we'll have multiple shadows
			if ((g_bRTEnabled || (g_ShadowMapping.bEnabled && g_ShadowMapping.bUseShadowOBJ)) &&
				g_HyperspacePhaseFSM == HS_INIT_ST)
			{
				TagAndFadeXWALights();
			}

			// Render the hyperspace effect if necessary
			if (g_HyperspacePhaseFSM != HS_INIT_ST)
			{
				UINT vertexBufferStride = sizeof(D3DTLVERTEX), vertexBufferOffset = 0;
				// Preconditions:
				//		shadertoyBufMSAA has a copy of the cockpit.
				//		shadertoyAuxBufMSAA has a copy of the offscreen buffer (the background, if applicable).
				// When we're in hyperspace, the cockpit should be rendered to shadertoyBufMSAA through shadertoyRTV.
				// This happens in SelectOffscreenBuffer() when we pass the relevant flags. Here, we resolve shaderToyBufMSAA
				// to shadertoyBuf. This is later used in PrimarySurface::RenderHyperspaceEffect() to compose the cockpit,
				// in the form of left/right 2D images on top of the the hyperspace effect itself.
				if (resources->_useMultisampling) {
					context->ResolveSubresource(resources->_shadertoyBuf, 0, resources->_shadertoyBufMSAA, 0, BACKBUFFER_FORMAT);
					if (g_bUseSteamVR)
						//context->ResolveSubresource(resources->_shadertoyBufR, 0, resources->_shadertoyBufMSAA_R, 0, BACKBUFFER_FORMAT);
						context->ResolveSubresource(resources->_shadertoyBuf, D3D11CalcSubresource(0, 1, 1),
							resources->_shadertoyBufMSAA, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
				}

				if (g_bDumpSSAOBuffers) {
					DirectX::SaveDDSTextureToFile(context, resources->_shadertoyBuf, L"C:\\Temp\\_shadertoyBuf.dds");
					//DirectX::SaveDDSTextureToFile(context, resources->_shadertoyAuxBuf, L"C:\\Temp\\_shadertoyAuxBuf.dds");
					if (g_bUseSteamVR)
						DirectX::SaveDDSTextureToFile(context, resources->_shadertoyBufR, L"C:\\Temp\\_shadertoyBufR.dds");
				}

				// This is the right spot to render the post-hyper-exit effect: we've captured the current offscreenBuffer into
				// shadertoyAuxBuf and we've finished rendering the cockpit/foreground too.
				RenderHyperspaceEffect(&g_nonVRViewport, resources->_pixelShaderTexture, NULL, NULL, &vertexBufferStride, &vertexBufferOffset);
			}

			// Resolve the Bloom, Normals and SSMask before the SSAO and Bloom effects.
			if (g_bReshadeEnabled)
			{
				// Resolve whatever is in the _offscreenBufferBloomMask into _offscreenBufferAsInputBloomMask, and
				// do the same for the right (SteamVR) image -- I'll worry about the details later.
				// _offscreenBufferAsInputReshade was previously resolved during Execute() -- right before any GUI is rendered
				context->ResolveSubresource(resources->_offscreenBufferAsInputBloomMask, 0,
					resources->_offscreenBufferBloomMask, 0, BLOOM_BUFFER_FORMAT);
				if (g_bUseSteamVR)
					context->ResolveSubresource(resources->_offscreenBufferAsInputBloomMask, D3D11CalcSubresource(0, 1, 1),
						resources->_offscreenBufferBloomMask, D3D11CalcSubresource(0, 1, 1), BLOOM_BUFFER_FORMAT);

				// Resolve the normals, ssaoMask and ssMask buffers if necessary
				// Notice how we don't care about the g_bDepthBufferResolved flag here, we just resolve and move on
				if (resources->_useMultisampling) {
					context->ResolveSubresource(resources->_normBuf, 0, resources->_normBufMSAA, 0, AO_DEPTH_BUFFER_FORMAT);
					context->ResolveSubresource(resources->_ssaoMask, 0, resources->_ssaoMaskMSAA, 0, AO_MASK_FORMAT);
					context->ResolveSubresource(resources->_ssMask, 0, resources->_ssMaskMSAA, 0, AO_MASK_FORMAT);
					if (g_bUseSteamVR) {
						context->ResolveSubresource(resources->_normBuf, D3D11CalcSubresource(0, 1, 1),
							resources->_normBufMSAA, D3D11CalcSubresource(0, 1, 1), AO_DEPTH_BUFFER_FORMAT);
						context->ResolveSubresource(resources->_ssaoMask, D3D11CalcSubresource(0, 1, 1), 
							resources->_ssaoMaskMSAA, D3D11CalcSubresource(0, 1, 1), AO_MASK_FORMAT);
						context->ResolveSubresource(resources->_ssMask, D3D11CalcSubresource(0, 1, 1),
							resources->_ssMaskMSAA, D3D11CalcSubresource(0, 1, 1), AO_MASK_FORMAT);
					}
				} 
				// if MSAA is not enabled, then the RTVs already populated normBuf, ssaoMask and ssMask, 
				// so nothing to do here!
			}

			if (!g_bDepthBufferResolved)
			{
				// If the depth buffer wasn't resolved during the regular Execute() then resolve it here.
				// This may happen if there is no GUI (all HUD indicators were switched off) or the
				// external camera is active.
				context->ResolveSubresource(resources->_depthBufAsInput, 0, resources->_depthBuf, 0, AO_DEPTH_BUFFER_FORMAT);
				if (g_bUseSteamVR)
					context->ResolveSubresource(resources->_depthBufAsInput, D3D11CalcSubresource(0, 1, 1),
						resources->_depthBuf, D3D11CalcSubresource(0, 1, 1), AO_DEPTH_BUFFER_FORMAT);
			}

			// Overwrite the backdrop with a texture cube:
			if (g_bUseSteamVR && g_bUseTextureCube && !g_bMapMode)
			{
				RenderSkyBoxVR(false);
				g_bBackgroundCaptured = true;
				if (g_bDumpSSAOBuffers)
					DirectX::SaveDDSTextureToFile(context, resources->_backgroundBuffer, L"c:\\temp\\_backgroundBufferAfterSkybox.dds");
			}

			if (g_bReplaceBackdrops && !g_bMapMode)
			{
				RenderSkyCylinder();
				g_bBackgroundCaptured = true;
				if (g_bDumpSSAOBuffers)
					DirectX::SaveDDSTextureToFile(context, resources->_backgroundBuffer, L"c:\\temp\\_backgroundBufferAfterSkyCylinder.dds");
			}

			if (!g_bBackgroundCaptured)
			{
				g_bBackgroundCaptured = true;

				if (!g_bMapMode)
				{
					context->CopyResource(resources->_backgroundBuffer, resources->_offscreenBuffer);
					// Wipe out the background:
					context->ClearRenderTargetView(resources->_renderTargetView, resources->clearColor);
					if (g_bUseSteamVR)
						context->ClearRenderTargetView(resources->_renderTargetViewHd, resources->clearColor);
				}

				if (g_bDumpSSAOBuffers)
					DirectX::SaveDDSTextureToFile(context, resources->_backgroundBuffer, L"c:\\temp\\_backgroundBufferP.dds");
			}

			context->ResolveSubresource(resources->_transpBufferAsInput1, 0, resources->_transpBuffer1, 0, BACKBUFFER_FORMAT);
			context->ResolveSubresource(resources->_transpBufferAsInput2, 0, resources->_transpBuffer2, 0, BACKBUFFER_FORMAT);
			if (g_bUseSteamVR)
			{
				context->ResolveSubresource(resources->_transpBufferAsInput1, D3D11CalcSubresource(0, 1, 1),
					resources->_transpBuffer1, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
				context->ResolveSubresource(resources->_transpBufferAsInput2, D3D11CalcSubresource(0, 1, 1),
					resources->_transpBuffer2, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
			}

			if (g_bDumpSSAOBuffers)
			{
				DirectX::SaveDDSTextureToFile(context, resources->_transpBufferAsInput1, L"c:\\Temp\\_transpBufferAsInput1.dds");
				DirectX::SaveDDSTextureToFile(context, resources->_transpBufferAsInput2, L"c:\\Temp\\_transpBufferAsInput2.dds");
			}

			// Render the Raytraced shadow mask
			if (g_bRTEnabled && g_bRTEnableSoftShadows)
			{
				RenderRTShadowMask();
			}

			// AO must (?) be computed before the bloom shader -- or at least output to a different buffer
			// Render the Deferred, SSAO or SSDO passes
			if (g_bAOEnabled) {
#ifdef GENMIPMAPS
				context->GenerateMips(resources->_depthBufSRV);
				if (g_bUseSteamVR)
					context->GenerateMips(resources->_depthBufSRV_R);
#endif

				// We need to set the blend state properly for SSAO
				D3D11_BLEND_DESC blendDesc{};
				blendDesc.AlphaToCoverageEnable = FALSE;
				blendDesc.IndependentBlendEnable = FALSE;
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				hr = resources->InitBlendState(nullptr, &blendDesc);

				// Temporarily disable ZWrite: we won't need it to display SSAO
				D3D11_DEPTH_STENCIL_DESC desc;
				ComPtr<ID3D11DepthStencilState> depthState;
				desc.DepthEnable = FALSE;
				desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
				desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
				desc.StencilEnable = FALSE;
				resources->InitDepthStencilState(depthState, &desc);

				context->PSSetSamplers(1, 1, &resources->_mirrorSamplerState);

				if (g_bDumpSSAOBuffers) {
					DirectX::SaveDDSTextureToFile(context, resources->_normBuf, L"C:\\Temp\\_normBuf.dds");
					DirectX::SaveDDSTextureToFile(context, resources->_depthBufAsInput, L"C:\\Temp\\_depthBuf.dds");
					DirectX::SaveDDSTextureToFile(context, resources->_depthStencilL, L"C:\\Temp\\_depthStencilL.dds");
					DirectX::SaveDDSTextureToFile(context, resources->_offscreenBufferAsInputBloomMask, L"C:\\Temp\\_bloomMask1.dds");
					DirectX::SaveWICTextureToFile(context, resources->_offscreenBuffer, GUID_ContainerFormatJpeg,
						L"C:\\Temp\\_offscreenBuf.jpg");
					if (g_bUseSteamVR)
						DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferR, GUID_ContainerFormatJpeg,
							L"C:\\Temp\\_offscreenBufR.jpg");
					DirectX::SaveDDSTextureToFile(context, resources->_ssaoMask, L"C:\\Temp\\_ssaoMask.dds");
					DirectX::SaveDDSTextureToFile(context, resources->_ssMask, L"C:\\Temp\\_ssMask.dds");
					log_debug("[DBG] [AO] Captured debug buffers");
				}

				// Input: depthBufAsInput (already resolved during Execute())
				// Output: normalsBuf
				//SmoothNormalsPass(1.0f);

				// Input: depthBuf, normBuf, randBuf
				// Output: _bloom1
				switch (g_SSAO_Type) {
					case SSO_AMBIENT:
						SSAOPass(g_fSSAOZoomFactor);
						// Resolve the bloom mask again: SSDO can modify this mask
						context->ResolveSubresource(resources->_offscreenBufferAsInputBloomMask, 0,
							resources->_offscreenBufferBloomMask, 0, BLOOM_BUFFER_FORMAT);
						if (g_bUseSteamVR)
							context->ResolveSubresource(resources->_offscreenBufferAsInputBloomMask, D3D11CalcSubresource(0, 1, 1),
								resources->_offscreenBufferBloomMask, D3D11CalcSubresource(0, 1, 1), BLOOM_BUFFER_FORMAT);
						break;
					case SSO_DIRECTIONAL:
					case SSO_BENT_NORMALS:
						SSDOPass(g_fSSAOZoomFactor, g_fSSAOZoomFactor2);
						// Resolve the bloom mask again: SSDO can modify this mask
						context->ResolveSubresource(resources->_offscreenBufferAsInputBloomMask, 0,
							resources->_offscreenBufferBloomMask, 0, BLOOM_BUFFER_FORMAT);
						if (g_bUseSteamVR)
							context->ResolveSubresource(resources->_offscreenBufferAsInputBloomMask, D3D11CalcSubresource(0, 1, 1),
								resources->_offscreenBufferBloomMask, D3D11CalcSubresource(0, 1, 1), BLOOM_BUFFER_FORMAT);
						break;
					case SSO_DEFERRED:
					case SSO_PBR:
						DeferredPass();
						// Resolve the bloom mask again: the deferred pass can modify this mask
						context->ResolveSubresource(resources->_offscreenBufferAsInputBloomMask, 0,
							resources->_offscreenBufferBloomMask, 0, BLOOM_BUFFER_FORMAT);
						if (g_bUseSteamVR)
							context->ResolveSubresource(resources->_offscreenBufferAsInputBloomMask, D3D11CalcSubresource(0, 1, 1),
								resources->_offscreenBufferBloomMask, D3D11CalcSubresource(0, 1, 1), BLOOM_BUFFER_FORMAT);
						if (g_bDumpSSAOBuffers)
						{
							DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferPost, GUID_ContainerFormatPng, L"C:\\Temp\\_sunFlare.png");
						}
						break;
				}

				// The VR dots are shadeless and should appear on top of everything else, so we render
				// them after the Deferred/SSAO pass above
				if (g_bUseSteamVR && g_bActiveCockpitEnabled)
				{
					EffectsRenderer* renderer = (EffectsRenderer*)g_current_renderer;
					renderer->RenderVRDots();
				}

				// Render the skybox after the deferred pass. This helps debug the skybox, for instance,
				// by highlighting areas designated as "Up" or "Forward" that are visible at all times.
				/*
				if (g_bUseSteamVR && g_bUseTextureCube && !g_bMapMode)
				{
					RenderSkyBox(true);
				}
				*/

				if (g_bDumpSSAOBuffers) {
					//DirectX::SaveWICTextureToFile(context, resources->_offscreenBuffer, GUID_ContainerFormatJpeg, L"C:\\Temp\\_offscreenBuffer.jpg");
					DirectX::SaveDDSTextureToFile(context, resources->_offscreenBufferAsInputBloomMask, L"C:\\Temp\\_bloomMask2.dds");
					//DirectX::SaveDDSTextureToFile(context, resources->_bentBuf, L"C:\\Temp\\_bentBuf.dds");
					//DirectX::SaveWICTextureToFile(context, resources->_bentBuf, GUID_ContainerFormatJpeg, L"C:\\Temp\\_bentBuf.jpg");
					DirectX::SaveDDSTextureToFile(context, resources->_ssaoBuf, L"C:\\Temp\\_ssaoBuf.dds");
					//DirectX::SaveWICTextureToFile(context, resources->_ssaoBufR, GUID_ContainerFormatJpeg, L"C:\\Temp\\_ssaoBufR.jpg");
					DirectX::SaveDDSTextureToFile(context, resources->_ssaoBufR, L"C:\\Temp\\_ssaoBufR.dds");
					DirectX::SaveDDSTextureToFile(context, resources->_normBuf, L"C:\\Temp\\_normBuf.dds");
					//DirectX::SaveWICTextureToFile(context, resources->_shadertoyAuxBuf, GUID_ContainerFormatJpeg, L"C:\\Temp\\_shadertoyAuxBuf.jpg");
					//DirectX::SaveWICTextureToFile(context, resources->_shadertoyAuxBuf, GUID_ContainerFormatJpeg, L"C:\\Temp\\_shadertoyAuxBuf.jpg");
					DumpHyperspaceVertexBuffer(g_fCurInGameWidth, g_fCurInGameHeight);
				}
			}

			// Render the sun flare (if applicable)
			if (g_bReshadeEnabled && g_bProceduralSuns && !g_b3DSunPresent && !g_b3DSkydomePresent &&
				g_ShadertoyBuffer.SunFlareCount > 0 && g_ShadertoyBuffer.flare_intensity > 0.01f &&
				!g_bUseSteamVR) // Temporary: sun flares don't worki well in VR mode.
			{
				// We need to set the blend state properly for Bloom, or else we might get
				// different results when brackets are rendered because they alter the 
				// blend state
				D3D11_BLEND_DESC blendDesc{};
				blendDesc.AlphaToCoverageEnable = FALSE;
				blendDesc.IndependentBlendEnable = FALSE;
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				hr = resources->InitBlendState(nullptr, &blendDesc);

				// Temporarily disable ZWrite: we won't need it for post-proc effects
				D3D11_DEPTH_STENCIL_DESC desc;
				ComPtr<ID3D11DepthStencilState> depthState;
				desc.DepthEnable = FALSE;
				desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
				desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
				desc.StencilEnable = FALSE;
				resources->InitDepthStencilState(depthState, &desc);

				RenderSunFlare();
			}

			//if (g_bDumpSSAOBuffers) {
				//DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferPost, GUID_ContainerFormatPng, L"C:\\Temp\\_offscreenBufferPost.png");
				//DirectX::SaveDDSTextureToFile(context, resources->_offscreenBufferPost, L"C:\\Temp\\_offscreenBufferPost.dds");
			//}

			// Apply FXAA
			if (g_config.FXAAEnabled)
			{
				// _offscreenBufferAsInputBloomMask is resolved earlier, before the SSAO pass because
				// SSAO uses that mask to prevent applying SSAO on bright areas

				// We need to set the blend state properly for Bloom, or else we might get
				// different results when brackets are rendered because they alter the 
				// blend state
				D3D11_BLEND_DESC blendDesc{};
				blendDesc.AlphaToCoverageEnable = FALSE;
				blendDesc.IndependentBlendEnable = FALSE;
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				hr = resources->InitBlendState(nullptr, &blendDesc);

				// Temporarily disable ZWrite: we won't need it to display Bloom
				D3D11_DEPTH_STENCIL_DESC desc;
				ComPtr<ID3D11DepthStencilState> depthState;
				desc.DepthEnable = FALSE;
				desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
				desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
				desc.StencilEnable = FALSE;
				resources->InitDepthStencilState(depthState, &desc);

				//UINT vertexBufferStride = sizeof(D3DTLVERTEX), vertexBufferOffset = 0;
				RenderFXAA();
			}

			// Render the enhanced bracket after all the shading has been applied.
			if (g_config.Radar2DRendererEnabled)
			{
				if (!g_bEnableVR)
					this->RenderBracket();
				else
					this->CacheBracketsVR();
			}

			// Draw the reticle on top of everything else
			if (g_bExternalHUDEnabled || g_bEnableVR)
			{
				// We need to set the blend state properly for Bloom, or else we might get
				// different results when brackets are rendered because they alter the 
				// blend state
				D3D11_BLEND_DESC blendDesc{};
				blendDesc.AlphaToCoverageEnable = FALSE;
				blendDesc.IndependentBlendEnable = FALSE;
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				hr = resources->InitBlendState(nullptr, &blendDesc);

				// Temporarily disable ZWrite: we won't need it to display Bloom
				D3D11_DEPTH_STENCIL_DESC desc;
				ComPtr<ID3D11DepthStencilState> depthState;
				desc.DepthEnable = FALSE;
				desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
				desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
				desc.StencilEnable = FALSE;
				resources->InitDepthStencilState(depthState, &desc);

				RenderExternalHUD();
			}

			if (g_bStarDebugEnabled)
			{
				// We need to set the blend state properly for Bloom, or else we might get
				// different results when brackets are rendered because they alter the 
				// blend state
				D3D11_BLEND_DESC blendDesc{};
				blendDesc.AlphaToCoverageEnable = FALSE;
				blendDesc.IndependentBlendEnable = FALSE;
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				hr = resources->InitBlendState(nullptr, &blendDesc);

				// Temporarily disable ZWrite: we won't need it to display Bloom
				D3D11_DEPTH_STENCIL_DESC desc;
				ComPtr<ID3D11DepthStencilState> depthState;
				desc.DepthEnable = FALSE;
				desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
				desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
				desc.StencilEnable = FALSE;
				resources->InitDepthStencilState(depthState, &desc);

				RenderStarDebug();
			}

			// Apply the speed shader
			// Adding g_bCustomFOVApplied to condition below prevents this effect from getting rendered 
			// on the first frame (sometimes it can happen and it's quite visible/ugly)
			if (g_bCustomFOVApplied && g_bEnableSpeedShader && !*g_playerInHangar && !g_bMapMode &&
				(!bExternalCamera || (bExternalCamera && *g_playerIndex == cameraObjIdx)) &&
				(g_HyperspacePhaseFSM == HS_INIT_ST || g_HyperspacePhaseFSM == HS_POST_HYPER_EXIT_ST) &&
				FlyByCameraTime == 0)
			{
				// We need to set the blend state properly for Bloom, or else we might get
				// different results when brackets are rendered because they alter the 
				// blend state
				D3D11_BLEND_DESC blendDesc{};
				blendDesc.AlphaToCoverageEnable = FALSE;
				blendDesc.IndependentBlendEnable = FALSE;
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				hr = resources->InitBlendState(nullptr, &blendDesc);

				// Temporarily disable ZWrite: we won't need it to display Bloom
				D3D11_DEPTH_STENCIL_DESC desc;
				ComPtr<ID3D11DepthStencilState> depthState;
				desc.DepthEnable = FALSE;
				desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
				desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
				desc.StencilEnable = FALSE;
				resources->InitDepthStencilState(depthState, &desc);

				context->ResolveSubresource(resources->_offscreenBufferAsInput, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
				if (g_bUseSteamVR)
					context->ResolveSubresource(
						resources->_offscreenBufferAsInput, D3D11CalcSubresource(0, 1, 1),
						resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
				RenderSpeedEffect();
			}

			// Render the additional geometry
			// Adding g_bCustomFOVApplied to condition below prevents this effect from getting rendered 
			// on the first frame (sometimes it can happen and it's quite visible/ugly
			if (g_bCustomFOVApplied && g_bEnableAdditionalGeometry)
			{
				// We need to set the blend state properly for Bloom, or else we might get
				// different results when brackets are rendered because they alter the 
				// blend state
				D3D11_BLEND_DESC blendDesc{};
				blendDesc.AlphaToCoverageEnable = FALSE;
				blendDesc.IndependentBlendEnable = FALSE;
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				hr = resources->InitBlendState(nullptr, &blendDesc);

				// Temporarily disable ZWrite: we won't need it to display Bloom
				D3D11_DEPTH_STENCIL_DESC desc;
				ComPtr<ID3D11DepthStencilState> depthState;
				desc.DepthEnable = FALSE;
				desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
				desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
				desc.StencilEnable = FALSE;
				resources->InitDepthStencilState(depthState, &desc);

				RenderAdditionalGeometry();
			}

			// Apply the Bloom effect
			if (g_bBloomEnabled) {
				// _offscreenBufferAsInputBloomMask is resolved earlier, before the SSAO pass because
				// SSAO uses that mask to prevent applying SSAO on bright areas

				// We need to set the blend state properly for Bloom, or else we might get
				// different results when brackets are rendered because they alter the 
				// blend state
				D3D11_BLEND_DESC blendDesc{};
				blendDesc.AlphaToCoverageEnable = FALSE;
				blendDesc.IndependentBlendEnable = FALSE;
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				hr = resources->InitBlendState(nullptr, &blendDesc);

				// Sometimes the bloom effect will disappear unless we set the rasterizer state here
				resources->InitRasterizerState(resources->_rasterizerState);

				// Temporarily disable ZWrite: we won't need it to display Bloom
				D3D11_DEPTH_STENCIL_DESC desc;
				ComPtr<ID3D11DepthStencilState> depthState;
				desc.DepthEnable = FALSE;
				desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
				desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
				desc.StencilEnable = FALSE;
				resources->InitDepthStencilState(depthState, &desc);

				//int HyperspacePhase = PlayerDataTable[*g_playerIndex].hyperspacePhase;
				//bool bHyperStreaks = (HyperspacePhase == 2) || (HyperspacePhase == 3);
				bool bHyperStreaks = g_HyperspacePhaseFSM == HS_HYPER_ENTER_ST ||
					g_HyperspacePhaseFSM == HS_HYPER_EXIT_ST ||
					g_HyperspacePhaseFSM == HS_POST_HYPER_EXIT_ST;

				// Initialize the accummulator buffer
				float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				context->ClearRenderTargetView(resources->_renderTargetViewBloomSum, bgColor);
				if (g_bUseSteamVR)
					context->ClearRenderTargetView(resources->_renderTargetViewBloomSumR, bgColor);

				// DEBUG
				/*if (g_bDumpSSAOBuffers) {
					log_debug("[DBG] Dumping bloom buffers");
					DirectX::SaveDDSTextureToFile(context, resources->_offscreenBufferAsInputBloomMask,
						L"c:\\temp\\_offscreenBufferBloomMask.dds");
				}*/
				// DEBUG

				{
					float fScale = 2.0f;
					// Set the _mainSamplerState on the first 2 texture slots. That way we prevent the
					// bloom effect from "bleeding" around the edge of the screen.
					context->PSSetSamplers(0, 1, resources->_mainSamplerState.GetAddressOf());
					context->PSSetSamplers(1, 1, resources->_mainSamplerState.GetAddressOf());
					for (int i = 1; i <= g_BloomConfig.iNumPasses; i++) {
						int AdditionalPasses = g_iBloomPasses[i] - 1;
						// Zoom level 2.0f with only one pass tends to show artifacts unless
						// the spread is set to 1
						BloomPyramidLevelPass(i, AdditionalPasses, fScale);
						fScale *= 2.0f;
					}
				}

				//if (!g_bAOEnabled) {
					// TODO: Check the statement below, I'm not sure it's current anymore (?)
					// If SSAO is not enabled, then we can merge the bloom buffer with the offscreen buffer
					// here. Otherwise, we'll merge it along with the SSAO buffer later.
					// Add the accumulated bloom with the offscreen buffer
					// Input: _bloomSum, _offscreenBufferAsInput
					// Output: _offscreenBuffer
					BloomBasicPass(5, 1.0f);
				//}

				// DEBUG
				/*if (g_iPresentCounter == 100 || g_bDumpBloomBuffers) {
					wchar_t filename[80];

					swprintf_s(filename, 80, L".\\_bloomMask-Final-%d.jpg", CaptureCounter);
					DirectX::SaveWICTextureToFile(context, resources->_bloomOutput1, GUID_ContainerFormatJpeg, filename);
					swprintf_s(filename, 80, L".\\_bloomMask-Final-%d.dds", CaptureCounter);
					DirectX::SaveDDSTextureToFile(context, resources->_bloomOutput1, filename);

					swprintf_s(filename, 80, L".\\_offscreenBuffer-Final-%d.jpg", CaptureCounter);
					DirectX::SaveWICTextureToFile(context, resources->_offscreenBuffer, GUID_ContainerFormatJpeg, filename);
					swprintf_s(filename, 80, L".\\_offscreenBuffer-Final-%d.dds", CaptureCounter);
					DirectX::SaveDDSTextureToFile(context, resources->_offscreenBuffer, filename);

					CaptureCounter++;
					g_bDumpBloomBuffers = false;
				}*/
				// DEBUG
			}

			// Draw the HUD
			/*
			if ((g_bDCManualActivate || bExteriorCamera) && (g_bDynCockpitEnabled || g_bReshadeEnabled) && 
				g_iHUDOffscreenCommandsRendered && resources->_bHUDVerticesReady) 
			*/
			// For some reason we have to call this or the ESC menu will disappear...
			/*
			 * If the new Text renderer and Bloom are both enabled, then the following block will cause the landing
			 * sequence to freeze _and I don't know why!_, I suspect the ResolveSubResource from the _offscreenBuffer
			 * to the _backBuffer isn't working properly; but I don't see why that should be the case. So, instead of
			 * fixing that, I'm disabling this block when the player is in the hangar, the exterior camera is enabled,
			 * and the new 2D renderer is enabled.
			 */
			//if (!(*g_playerInHangar && bExteriorCamera))
			if (!(*g_playerInHangar && bExternalCamera && g_config.Text2DRendererEnabled)) 
			{
				// If we're not in external view, then clear everything we don't want to display from the HUD
				if (g_bDynCockpitEnabled && g_bDCApplyEraseRegionCommands && !bExternalCamera)
					ClearHUDRegions();

				// DTM's Yavin map exposed a weird bug when the next if() is enabled: if the XwingCockpit.dc file
				// doesn't match the XwingCockpit.opt, then we'll erase all HUD regions; but we will not render
				// any DC elements. Pressing ESC to see the menu while flying only shows a black screen. The black
				// screen goes away if we remove the next if:
				//if (num_regions_erased < MAX_DC_REGIONS - 1)
				// I don't know why; but if we call DrawHUDVertices, or if we enable AC, the problem goes away.
				// Looks like we need the state set by DrawHUDVertices() after all...
				// ... also: we need to call DrawHUDVertices to move the HUD regions anyway (but only if there
				// are any HUD regions that weren't erased!)
				// So, we're going to call DrawHUDVertices to set the state; but skip the Draw() calls if all
				// HUD regions were erased
				//DrawHUDVertices(num_regions_erased < MAX_DC_REGIONS - 1);

				// If we jump into hyperspace while the external camera is activated, the cockpit will be displayed
				// but bExternalCamera will remain true. When this happens, we won't capture the HUD background, because
				// we don't do that when the external camera is active. This places us in a weird situation, because
				// we have the HUD text, without background and the cockpit behind. Let's avoid displaying the HUD
				// altogether in this edge case
				if (!bExternalCamera || g_HyperspacePhaseFSM == HS_INIT_ST)
					DrawHUDVertices();
			}

			// I should probably render the laser pointer before the HUD; but if I do that, then the
			// HUD gets messed up. I guess I'm destroying the state somehow
			// Render the Laser Pointer for VR
			if (g_bActiveCockpitEnabled && g_bRendering3D && !PlayerDataTable[*g_playerIndex].Camera.ExternalCamera)
			{
				UINT vertexBufferStride = sizeof(D3DTLVERTEX), vertexBufferOffset = 0;
				RenderLaserPointer(&g_nonVRViewport, resources->_pixelShaderTexture, NULL, NULL, &vertexBufferStride, &vertexBufferOffset);
			}

			// 3D path final post-processing step. Copied from ReShade's Levels.fx
			if (g_bEnableLevelsShader) {
				RenderLevels();
			}
			
			if (g_bDumpLaserPointerDebugInfo)
				g_bDumpLaserPointerDebugInfo = false;

			// In the original code, the offscreenBuffer is resolved to the backBuffer
			//this->_deviceResources->_d3dDeviceContext->ResolveSubresource(this->_deviceResources->_backBuffer, 0, this->_deviceresources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);

			if (g_bDumpSSAOBuffers && !g_bAOEnabled) {
				log_debug("[DBG] SSAO Disabled. Dumping buffers");
				// If SSAO is OFF we would still like to dump some information.
				DirectX::SaveWICTextureToFile(context, resources->_offscreenBuffer, GUID_ContainerFormatJpeg,
					L"C:\\Temp\\_offscreenBuf.jpg");
				DirectX::SaveDDSTextureToFile(context, resources->_offscreenBufferAsInputBloomMask, L"C:\\Temp\\_bloomMask1.dds");
				if (g_bUseSteamVR) {
					DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferR, GUID_ContainerFormatJpeg,
						L"C:\\Temp\\_offscreenBufR.jpg");
					DirectX::SaveDDSTextureToFile(context, resources->_offscreenBufferAsInputBloomMaskR, L"C:\\Temp\\_bloomMask1R.dds");
				}
			}

			// Final _offscreenBuffer --> _backBuffer copy. The offscreenBuffer SHOULD CONTAIN the fully-rendered image at this point.
			if (g_bEnableVR) {
				if (g_bUseSteamVR) {
					/*
					// The new Metric VR reconstruction shouldn't need any barrel effect distortion 
					if (!g_bDisableBarrelEffect) {
						// Do the barrel effect (_offscreenBuffer -> _offscreenBufferPost)
						barrelEffectSteamVR();
						context->CopyResource(resources->_offscreenBuffer, resources->_offscreenBufferPost);
						context->CopyResource(resources->_offscreenBufferR, resources->_offscreenBufferPostR);
					}
					*/
					// Resize the buffer to be presented (_offscreenBuffer -> _steamVRPresentBuffer)
					// TODO: Compensate for the difference in aspect ratio between SteamVR's window
					//       and the actual window size on the screen.
					resizeForSteamVR(0, false);
					// Resolve steamVRPresentBuffer to backBuffer so that it gets presented on the screen
					context->ResolveSubresource(resources->_backBuffer, 0, resources->_steamVRPresentBuffer, 0, BACKBUFFER_FORMAT);
				} 
				else { // Direct SBS mode
					if (!g_bDisableBarrelEffect) {
						barrelEffect3D();
						if (g_b3DVisionEnabled) {
							// Resolve _vision3DPost, which is MSAA, into _vision3DNoMSAA. We can't resolve _vision3DPost directly
							// into _vision3DStaging because the flags used to create that last buffer won't allow it. So we need
							// this intermediate buffer (_vision3DNoMSAA) to help us get the data into the staging buffer.
							context->ResolveSubresource(resources->_vision3DNoMSAA, 0, resources->_vision3DPost, 0, BACKBUFFER_FORMAT);
							/*
							context->CopyResource(resources->_vision3DStaging, resources->_vision3DNoMSAA);
							// Add the 3D vision signature to _vision3DStaging
							Add3DVisionSignature();
							*/

							// The staging buffer now has the signature, we can copy it to the backbuffer
							D3D11_BOX box;
							box.left = 0; box.right = resources->_backbufferWidth;
							box.top = 0; box.bottom = resources->_backbufferHeight;
							box.front = 0; box.back = 1;
							context->CopySubresourceRegion(resources->_backBuffer, D3D11CalcSubresource(0, 0, 1), 0, 0, 0, resources->_vision3DNoMSAA, 0, &box);
							//context->CopySubresourceRegion(resources->_backBuffer, D3D11CalcSubresource(0, 0, 1), 0, 0, 0, resources->_vision3DStaging, 0, &box);
							// DEBUG: Just copy _vision3DPost to _offscreenBufferPost and resolve.
							//context->CopySubresourceRegion(resources->_offscreenBufferPost, D3D11CalcSubresource(0, 0, 1), 0, 0, 0, resources->_vision3DPost, 0, &box);
							//context->ResolveSubresource(resources->_backBuffer, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);

							if (g_bDumpSSAOBuffers) {
								log_debug("[DBG] [3DV] Dumping _vision3DNoMSAA...");
								//DirectX::SaveWICTextureToFile(context, resources->_vision3DPost, GUID_ContainerFormatPng, L"C:\\Temp\\_vision3DPost.png");
								DirectX::SaveDDSTextureToFile(context, resources->_vision3DNoMSAA, L"C:\\Temp\\_vision3DNoMSAA.dds");
								DirectX::SaveWICTextureToFile(context, resources->_vision3DNoMSAA, GUID_ContainerFormatPng, L"C:\\Temp\\_vision3DNoMSAA.png");
								//DirectX::SaveWICTextureToFile(context, resources->_vision3DStaging, GUID_ContainerFormatPng, L"C:\\Temp\\_vision3DStaging.png");
							}
						}
						else
							context->ResolveSubresource(resources->_backBuffer, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);
					} else
						context->ResolveSubresource(resources->_backBuffer, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
				}
			}
			else { // Non-VR mode
				// D3D11CalcSubresource(0, 0, 1),
				context->ResolveSubresource(resources->_backBuffer, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
				//context->CopyResource(resources->_offscreenBufferPost, resources->_offscreenBuffer);
				//context->ResolveSubresource(resources->_backBuffer, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);
				//log_debug("[DBG] (%d) offscreenBuffer --> backBuffer", g_iPresentCounter);
				if (g_bDumpSSAOBuffers)
					DirectX::SaveDDSTextureToFile(context, resources->_backBuffer, L"C:\\Temp\\_backBuffer.dds");
			}

			if (g_bDumpSSAOBuffers)
			{
				ResetXWALightInfo();
				TagXWALights();
			}

			if (g_bDumpSSAOBuffers) 
			{
				// These values match MXvTED exactly:
				//short *POV_Y0 = (short *)(0x5BB480 + 0x238);
				//short *POV_Z0 = (short *)(0x5BB480 + 0x23A);
				//short *POV_X0 = (short *)(0x5BB480 + 0x23C);

				//*POV_X0 = 5; // This had no obvious effect

				// These are almost the same values as the above, as floats:
				// The difference is that Y has the sign inverted
				/*float *POV_X1 = (float *)(0x8B94E0 + 0x20D);
				float *POV_Y1 = (float *)(0x8B94E0 + 0x211);
				float *POV_Z1 = (float *)(0x8B94E0 + 0x215);*/

				//*POV_X1 = 5.0f; // This had no obvious effect

				// This is the same POV as the above; but Y is now -Y and it has been
				// transformed by the camera heading. So, this value changes depending
				// on the current orientation of the craft.
				// That's what Jeremy meant by "transform matrix in the MobileObject struct
				// (offset 0xC7)" <-- That's what I call the "Heading Matrix".
				// This transform matrix is *not* affected by the cockpit camera. It's just
				// the current heading.
				/*float *POV_X2 = (float *)(0x8B94E0 + 0x201);
				float *POV_Y2 = (float *)(0x8B94E0 + 0x205);
				float *POV_Z2 = (float *)(0x8B94E0 + 0x209);*/

				//log_debug("[DBG] [SHW] [POV] X0,Z0,Y0: %d, %d, %d", *g_POV_X0, *g_POV_Z0, *g_POV_Y0);
				//log_debug("[DBG] [SHW] [POV] X1,Z1,Y1: %f, %f, %f", *g_POV_X, *g_POV_Z, *g_POV_Y);
				//log_debug("[DBG] [POV] X2,Z2,Y2: %f, %f, %f", *POV_X2, *POV_Z2, *POV_Y2);

				/*
				DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferDynCockpit, GUID_ContainerFormatJpeg, L"c:\\temp\\_DC-FG-2.jpg");
				DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferDynCockpitBG, GUID_ContainerFormatJpeg, L"c:\\temp\\_DC-BG-2.jpg");
				DirectX::SaveWICTextureToFile(context, resources->_DCTextMSAA, GUID_ContainerFormatJpeg, L"c:\\temp\\_DC-Text-2.jpg");
				*/
				//log_debug("[DBG] g_playerIndex: %d, objectIndex: %d, currentTarget: %d",
				//	*g_playerIndex, PlayerDataTable[*g_playerIndex].objectIndex, PlayerDataTable[*g_playerIndex].currentTargetIndex);
				//log_debug("[DBG] mapState: %d", PlayerDataTable[*g_playerIndex].mapState);
				// Read the int value at offset 0B54 in the player struct and compare it with the player object index
				//int *pEntry = (int *)(&PlayerDataTable[*g_playerIndex] + 0x0B54);
				//log_debug("[DBG] Focus idx: %d", PlayerDataTable[*g_playerIndex].cameraFG);

				//uint32_t hud_color = (*g_XwaFlightHudColor) & 0x00FFFFFF;
				// This will change the color of the HUD
				//*g_XwaFlightHudBorderColor = 0xffffb7d2;
				//*g_XwaFlightHudColor = 0xff805c69;
				//log_debug("[DBG] hud_color, border: 0x%x, inside: 0x%x", *g_XwaFlightHudBorderColor, *g_XwaFlightHudColor);
			}

			// RESET CONTROL VARS, FRAME COUNTERS, CLEAR VECTORS, ETC
			{
				g_iDrawCounter = 0; // g_iExecBufCounter = 0;
				g_iNonZBufferCounter = 0; g_iDrawCounterAfterHUD = -1;
				g_iFloatingGUIDrawnCounter = 0;
				g_bTargetCompDrawn = false;
				g_bPrevIsFloatingGUI3DObject = false;
				g_bIsFloating3DObject = false;
				g_bStartedGUI = false;
				g_bPrevStartedGUI = false;
				g_bIsScaleableGUIElem = false;
				g_bPrevIsScaleableGUIElem = false;
				g_bScaleableHUDStarted = false;
				g_bIsTrianglePointer = false;
				g_bLastTrianglePointer = false;
				g_iHUDOffscreenCommandsRendered = 0;
				g_bSkyBoxJustFinished = false;
				g_bPrevIsPlayerObject = false;
				g_bIsPlayerObject = false;
				g_bLastFrameWasExterior = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;
				// Disable the Dynamic Cockpit whenever we're in external camera mode:
				g_bDCManualActivate = !g_bLastFrameWasExterior;
				g_bDepthBufferResolved = false;
				g_bHyperspaceEffectRenderedOnCurrentFrame = false;
				g_bSwitchedToGUI = false;
				g_ShadertoyBuffer.SunFlareCount = 0;
				g_bPrevIsTargetHighlighted = g_bIsTargetHighlighted;
				g_bIsTargetHighlighted = false;
				g_bExecuteBufferLock = false;
				g_bDCWasClearedOnThisFrame = false;
				g_bEdgeEffectApplied = false;
				g_TriangleCentroid.x = g_TriangleCentroid.y = -1.0f;
				g_iNumSunCentroids = 0; // Reset the number of sun centroids seen in this frame
				g_iReactorExplosionCount = 0;
				g_iD3DExecuteCounter = 0; // Reset the draw call counter for the D3DRendererHook
				g_bBackgroundCaptured = false;

				if (*g_playerInHangar && !g_bPrevPlayerInHangar)
				{
					// Restore the cockpit if we just landed in the hangar and the flag is set.
					if (g_bResetCockpitDamageInHangar)
						g_bResetCockpitDamage = true;
				}
				// Reset the frame counter if we just exited the hangar
				if (!(*g_playerInHangar) && g_bPrevPlayerInHangar) {
					SetPresentCounter(0, 0);
					ResetXWALightInfo();
					log_debug("[DBG] EXITED HANGAR");
				}
				g_bPrevPlayerInHangar = *g_playerInHangar;
				// Force recomputation of y_center at the beginning of each match, or after exiting the hangar
				if (g_iPresentCounter == 5)
					g_bYCenterHasBeenFixed = false;

				if (g_bTriggerReticleCapture) {
					//DisplayBox("Reticle Limits: ", g_ReticleCenterLimits);
					log_debug("Reticle Centroid: %0.3f, %0.3f", g_ReticleCentroid.x, g_ReticleCentroid.y);
					g_bTriggerReticleCapture = false;
				}
				
				// Increase the post-hyperspace-exit frames; but only when we're in the right state:
				if (g_HyperspacePhaseFSM == HS_POST_HYPER_EXIT_ST)
					g_iHyperExitPostFrames++;
				else
					g_iHyperExitPostFrames = 0;
				// If we just rendered the first hyperspace frame, then it's safe to clear the aux buffer:
				if (g_bHyperspaceFirstFrame && !g_bClearedAuxBuffer) {
					float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
					g_bClearedAuxBuffer = true;
					context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
					context->ResolveSubresource(resources->_shadertoyAuxBuf, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);
					if (g_bUseSteamVR)
						// Copy right eye subresource onto right eye just for clearing
						context->CopySubresourceRegion(resources->_shadertoyAuxBuf, D3D11CalcSubresource(0, 1, 1),
							0, 0, 0, resources->_shadertoyAuxBuf, D3D11CalcSubresource(0, 0, 1),NULL);
				}
				g_bHyperspaceFirstFrame = false;
				g_bHyperHeadSnapped = false;
				//*g_playerInHangar = 0;
				if (g_bDumpSSAOBuffers)
					g_bDumpSSAOBuffers = false;
				//if (g_bDumpOptNodes)
				//	g_bDumpOptNodes = false;

				// Reset the laser pointer intersection
				if (g_bActiveCockpitEnabled) {
					g_bPrevHoveringOnActiveElem[0] = g_LaserPointerBuffer.bHoveringOnActiveElem[0];
					g_bPrevHoveringOnActiveElem[1] = g_LaserPointerBuffer.bHoveringOnActiveElem[1];
					g_LaserPointerBuffer.bIntersection[0] = 0;
					g_LaserPointerBuffer.bIntersection[1] = 0;
					g_LaserPointerBuffer.bHoveringOnActiveElem[0] = 0;
					g_LaserPointerBuffer.bHoveringOnActiveElem[1] = 0;
					g_fBestIntersectionDistance[0] = FLT_MAX;
					g_fBestIntersectionDistance[1] = FLT_MAX;
					g_iBestIntersTexIdx[0] = -1;
					g_iBestIntersTexIdx[1] = -1;
					g_enable_ac_debug = false;
				}

				// Clear the laser list for the next frame
				if (g_bEnableLaserLights) {
					g_LaserList.clear();
					// If the headlights are ON, let's add one light right away near the cockpit's center:
					//if (g_bEnableHeadLights)
					//	g_LaserList.insert(g_HeadLightsPosition, g_HeadLightsColor);
				}

				// Select other alternate explosions if none were displayed
				if (!g_bExplosionsDisplayedOnCurrentFrame &&
					g_iPresentCounter % 300 == 0 /* Shuffle every 5 seconds (assuming 60fps) */)
				{
					for (int i = 0; i < MAX_XWA_EXPLOSIONS; i++) {
						// There's MAX_ALT_EXPLOSIONS to choose from plus the original version, so in reality, we have
						// MAX_ALT_EXPLOSIONS + 1 choices
						int rand_choice = rand() % (MAX_ALT_EXPLOSIONS + 1);
						// rand_choice goes from 0 to MAX_ALT_EXPLOSIONS, but we need it to be in the range
						// -1 .. MAX_ALT_EXPLOSIONS - 1, so we use rand_choice - 1
						g_AltExplosionSelector[i] = rand_choice - 1;
					}
				}
				g_bExplosionsDisplayedOnCurrentFrame = false;
			}

			// Apply the custom FOV
			// I tried applying these settings on DLL load, and on the first draw call in Execute(); but they didn't work.
			// Apparently I have to wait until the first frame is fully executed in order to apply the custom FOV
			if (!g_bCustomFOVApplied) {
				log_debug("[DBG] [FOV] [Flip] Applying Custom FOV.");

				switch (g_CurrentFOVType) {
				case GLOBAL_FOV:
					// Loads Focal_Length.cfg and applies the FOV using ApplyFocalLength()
					log_debug("[DBG] [FOV] [Flip] GLOBAL_FOV Loading Focal_Length.cfg");
					if (!LoadFocalLength()) {
						if (g_bEnableVR) {
							// We couldn't load the custom FOV and we're running in VR mode, let's apply
							// the current VR FOV...
							ApplyFocalLength(RealVertFOVToRawFocalLength(g_fVR_FOV));
							// ... and save it
							SaveFocalLength();
						}
						else {
							// We couldn't load the focal length and we're not running in VR mode.
							// We still need to compute the FOVscale and y_center:
							ComputeHyperFOVParams();
						}
					}
					// else: LoadFocalLength calls ApplyFocalLength, which in turn calls ComputeHyperFOVParams, which is
					// where we compute y_center and FOVscale, so we're good.
					break;
				case XWAHACKER_FOV:
					// If the current ship's DC file has a focal length, apply it:
					if (g_fCurrentShipFocalLength > 0.0f) {
						log_debug("[DBG] [FOV] [Flip] XWAHACKER_FOV Applying DC REGULAR Focal Length: %0.3f", g_fCurrentShipFocalLength);
						ApplyFocalLength(g_fCurrentShipFocalLength);
					}
					else {
						// Loads Focal_Length.cfg and applies the FOV using ApplyFocalLength()
						log_debug("[DBG] [FOV] [Flip] XWAHACKER_FOV Loading Focal_Length.cfg");
						LoadFocalLength();
					}
					break;
				case XWAHACKER_LARGE_FOV:
					// If the current ship's DC file has a focal length, apply it:
					if (g_fCurrentShipLargeFocalLength > 0.0f) {
						log_debug("[DBG] [FOV] [Flip] XWAHACKER_LARGE_FOV Applying DC LARGE Focal Length: %0.3f", g_fCurrentShipLargeFocalLength);
						ApplyFocalLength(g_fCurrentShipLargeFocalLength);
					}
					else {
						// Loads Focal_Length.cfg and applies the FOV using ApplyFocalLength()
						log_debug("[DBG] [FOV] [Flip] XWAHACKER_LARGE_FOV Loading Focal_Length.cfg");
						LoadFocalLength();
					}
					break;
				} // switch

				g_bCustomFOVApplied = true; // Becomes false in OnSizeChanged()
				ComputeHyperFOVParams();
			}

			// Reapply the MetricRec constants if necessary
			if (g_bMetricParamsNeedReapply) {
				// This probably is a good place to set the VS constant buffer:
				resources->InitVSConstantBufferMetricRec(resources->_metricRecVSConstantBuffer.GetAddressOf(), &g_MetricRecCBuffer);
				// TODO: Do we need this CB in the pixel shaders? Maybe if we replace the Shadertoy CB settings...
				resources->InitPSConstantBufferMetricRec(resources->_metricRecPSConstantBuffer.GetAddressOf(), &g_MetricRecCBuffer);
				log_debug("[DBG] Reapplied MetricRec CBs");
				g_bMetricParamsNeedReapply = false;
			}
			// Reset the Reticle Centroid *after* it has been used by ComputeHyperFOVParams
			g_ReticleCentroid.set(-1.0f, -1.0f);

//#define HYPER_OVERRIDE 1
//#ifdef HYPER_OVERRIDE
			//g_fHyperTimeOverride += 0.025f;
			if (g_bHyperDebugMode) {
				//g_fHyperTimeOverride = 1.0f;
				g_fHyperTimeOverride = 2.0f;
				if (g_fHyperTimeOverride > 2.0f) // Use 2.0 for entry, 4.0 for tunnel, 2.0 for exit, 1.5 for post-hyper-exit
					g_fHyperTimeOverride = 0.0f;
			}
			/*
			// Update the state
			if (g_fHyperTimeOverride >= 2.0f)
				g_iHyperStateOverride = 3; // HYPER_EXIT
			else
				g_iHyperStateOverride = 4; // POST_HYPER_EXIT
			*/
//#endif

			// Resolve the Dynamic Cockpit FG, BG and Text buffers.
			// This step has to be done here, after we clear the HUD regions, or we'll erase
			// the contents of these buffers for the next frame
			if (g_bDynCockpitEnabled || g_bReshadeEnabled || g_config.Text2DRendererEnabled) 
			{
				context->ResolveSubresource(_deviceResources->_offscreenAsInputDynCockpit,
					0, _deviceResources->_offscreenBufferDynCockpit, 0, BACKBUFFER_FORMAT);
				context->ResolveSubresource(_deviceResources->_offscreenAsInputDynCockpitBG,
					0, _deviceResources->_offscreenBufferDynCockpitBG, 0, BACKBUFFER_FORMAT);
				context->ResolveSubresource(_deviceResources->_DCTextAsInput,
					0, _deviceResources->_DCTextMSAA, 0, BACKBUFFER_FORMAT);

				// Apply the Edge Detector effect to the DC foreground texture
				if (g_bEdgeDetectorEnabled && g_bDynCockpitEnabled && g_bRendering3D && !g_bEdgeEffectApplied)
					RenderEdgeDetector();

				/*
				// Should this block be here? This fixes the HUD not getting cleared when the new 2D Renderer
				// Is enabled; but it breaks when it isn't
				float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				context->ClearRenderTargetView(resources->_renderTargetViewDynCockpit, bgColor);
				context->ClearRenderTargetView(resources->_renderTargetViewDynCockpitBG, bgColor);
				context->ClearRenderTargetView(resources->_DCTextRTV, bgColor);
				*/
				//log_debug("[DBG] Resolve DC RTVs.");

				/*static bool bDump = true;
				if (g_iPresentCounter == 100 && bDump) {
					DirectX::SaveWICTextureToFile(context, resources->_offscreenAsInputDynCockpit, GUID_ContainerFormatPng,
						L"C:\\Temp\\_DC-FG.png");
					DirectX::SaveWICTextureToFile(context, resources->_offscreenAsInputDynCockpitBG, GUID_ContainerFormatPng,
						L"C:\\Temp\\_DC-BG.png");
					bDump = false;
				}*/
			}

			/*
			// Clear the Shadow Map buffers
			if (bHyperspaceFirstFrame && g_ShadowMapping.Enabled) {
				context->ClearDepthStencilView(resources->_shadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
			}
			*/

#ifdef DBG_VR
			if (g_bStart3DCapture && !g_bDo3DCapture) {
				g_bDo3DCapture = true;
			}
			else if (g_bStart3DCapture && g_bDo3DCapture) {
				g_bDo3DCapture = false;
				g_bStart3DCapture = false;
				fclose(g_HackFile);
				g_HackFile = NULL;
			}
#endif

			// Submit images to SteamVR
			if (g_bUseSteamVR) {
				//if (!g_pHMD->GetTimeSinceLastVsync(&seconds, &frame))
				//	log_debug("[DBG] No Vsync info available");
				vr::EVRCompositorError error = vr::VRCompositorError_None;
				vr::Texture_t leftEyeTexture;
				vr::Texture_t rightEyeTexture;

				leftEyeTexture = { this->_deviceResources->_offscreenBuffer.Get(), vr::TextureType_DirectX, vr::ColorSpace_Auto };
				rightEyeTexture = { this->_deviceResources->_offscreenBuffer.Get(), vr::TextureType_DirectX, vr::ColorSpace_Auto };

				//log_debug("[DBG] Submit 3D");
				error = g_pVRCompositor->Submit(vr::Eye_Left, &leftEyeTexture);
				error = g_pVRCompositor->Submit(vr::Eye_Right, &rightEyeTexture);
			}

			// We're about to switch to 3D rendering.
			// * Update the hyperspace FSM if necessary
			// * Apply custom HUD colors
			if (!g_bRendering3D) {
				// We were presenting 2D content and now we're about to show 3D content. If we were in
				// hyperspace, we might need to reset the hyperspace FSM and restore any settings we
				// changed previously
				if (g_HyperspacePhaseFSM != HS_INIT_ST && PlayerDataTable[*g_playerIndex].hyperspacePhase == 0) {
					if (g_HyperspacePhaseFSM == HS_HYPER_TUNNEL_ST) {
						// Restore the previous color of the lights
						for (int i = 0; i < 2; i++) {
							memcpy(&g_LightVector[i], &g_TempLightVector[i], sizeof(Vector4));
							memcpy(&g_LightColor[i], &g_TempLightColor[i], sizeof(Vector4));
						}
					}
					g_bHyperspaceLastFrame = (g_HyperspacePhaseFSM == HS_HYPER_EXIT_ST);
					g_HyperspacePhaseFSM = HS_INIT_ST;
				}

				ApplyCustomHUDColor();

				if (g_bUseSteamVR) {
					g_pVROverlay->HideOverlay(g_VR2Doverlay);
					g_pVRCompositor->FadeToColor(0.2f, 0.0f, 0.0f, 0.0f, 0.0f, false);
				}
			}
			// Make sure the hyperspace effect is off if we're back in the hangar. This is necessary to fix
			// the Holdo bug (but more changes may be needed).
			if (*g_playerInHangar)
				g_HyperspacePhaseFSM = HS_INIT_ST;
			// We're about to show 3D content, so let's set the corresponding flag
			g_bRendering3D = true;
			SetPresentCounter(g_iPresentCounter + 1, 0);

			if (g_iDelayedDumpDebugBuffers) {
				g_iDelayedDumpDebugBuffers--;
				if (g_iDelayedDumpDebugBuffers == 0)
					g_bDumpSSAOBuffers = true;
			}

			// This is Jeremy's code:
			//if (FAILED(hr = this->_deviceResources->_swapChain->Present(g_config.VSyncEnabled ? 1 : 0, 0)))
			// For VR, we probably want to disable VSync to get as much frames a possible:
			//if (FAILED(hr = this->_deviceResources->_swapChain->Present(0, 0)))
			bool bEnableVSync = g_config.VSyncEnabled;
			if (*g_playerInHangar)
				bEnableVSync = g_config.VSyncEnabledInHangar;
			// If SteamVR is on, we do NOT want to do VSync with the monitor as well: that'll kill the performance.
			if (g_bUseSteamVR)
				bEnableVSync = false;
			log_debug_vr_reset();
			// Doing Present(1, 0) limits the framerate to 30fps, without it, it can go up to 60; but usually
			// stays around 45 in my system
			//log_debug("[DBG] ******************* PRESENT 3D");
			if (FAILED(hr = this->_deviceResources->_swapChain->Present(bEnableVSync ? 1 : 0, 0)))
			{
				static bool messageShown = false;
				log_debug("[DBG] Present 3D failed, hr: 0x%x", hr);
				if (!messageShown)
				{
					MessageBox(nullptr, _com_error(hr).ErrorMessage(), __FUNCTION__, MB_ICONERROR);
				}
				messageShown = true;
				hr = DDERR_SURFACELOST;
			}
			// We may still need the PostPresentHandoff here...
			if (g_bUseSteamVR && g_bTogglePostPresentHandoff) {
				g_pVRCompositor->PostPresentHandoff();
			}
		}
		else
		{
			hr = DD_OK;
		}
		this->_deviceResources->EndAnnotatedEvent();
		return hr;
	}

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	_deviceResources->EndAnnotatedEvent();
	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::GetAttachedSurface(
	LPDDSCAPS lpDDSCaps,
	LPDIRECTDRAWSURFACE FAR *lplpDDAttachedSurface
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	if (lpDDSCaps == nullptr)
	{
#if LOGGER
		str.str("\tDDERR_INVALIDPARAMS");
		LogText(str.str());
#endif

		return DDERR_INVALIDPARAMS;
	}

	if (lpDDSCaps->dwCaps & DDSCAPS_BACKBUFFER)
	{
		if (!this->_hasBackbufferAttached)
		{
#if LOGGER
			str.str("\tDDERR_INVALIDOBJECT");
			LogText(str.str());
#endif

			return DDERR_INVALIDOBJECT;
		}

		*lplpDDAttachedSurface = this->_backbufferSurface;
		this->_backbufferSurface->AddRef();

#if LOGGER
		str.str("");
		str << "\tBackbufferSurface\t" << *lplpDDAttachedSurface;
		LogText(str.str());
#endif

		return DD_OK;
	}

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::GetBltStatus(
	DWORD dwFlags
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::GetCaps(
	LPDDSCAPS lpDDSCaps
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::GetClipper(
	LPDIRECTDRAWCLIPPER FAR *lplpDDClipper
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::GetColorKey(
	DWORD dwFlags,
	LPDDCOLORKEY lpDDColorKey
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::GetDC(
	HDC FAR *lphDC
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::GetFlipStatus(
	DWORD dwFlags
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::GetOverlayPosition(
	LPLONG lplX,
	LPLONG lplY
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::GetPalette(
	LPDIRECTDRAWPALETTE FAR *lplpDDPalette
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::GetPixelFormat(
	LPDDPIXELFORMAT lpDDPixelFormat
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::GetSurfaceDesc(
	LPDDSURFACEDESC lpDDSurfaceDesc
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	if (lpDDSurfaceDesc == nullptr)
	{
#if LOGGER
		str.str("\tDDERR_INVALIDPARAMS");
		LogText(str.str());
#endif

		return DDERR_INVALIDPARAMS;
	}

	*lpDDSurfaceDesc = {};
	lpDDSurfaceDesc->dwSize = sizeof(DDSURFACEDESC);
	lpDDSurfaceDesc->dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH;
	lpDDSurfaceDesc->ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY;
	lpDDSurfaceDesc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	lpDDSurfaceDesc->ddpfPixelFormat.dwFlags = DDPF_RGB;
	lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount = 16;
	lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask = 0xF800;
	lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask = 0x7E0;
	lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask = 0x1F;
	lpDDSurfaceDesc->dwHeight = this->_deviceResources->_displayHeight;
	lpDDSurfaceDesc->dwWidth = this->_deviceResources->_displayWidth;
	lpDDSurfaceDesc->lPitch = this->_deviceResources->_displayWidth * 2;

#if LOGGER
	str.str("");
	str << "\t" << tostr_DDSURFACEDESC(lpDDSurfaceDesc);
	LogText(str.str());
#endif

	return DD_OK;
}

HRESULT PrimarySurface::Initialize(
	LPDIRECTDRAW lpDD,
	LPDDSURFACEDESC lpDDSurfaceDesc
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::IsLost()
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::Lock(
	LPRECT lpDestRect,
	LPDDSURFACEDESC lpDDSurfaceDesc,
	DWORD dwFlags,
	HANDLE hEvent
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::ReleaseDC(
	HDC hDC
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::Restore()
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::SetClipper(
	LPDIRECTDRAWCLIPPER lpDDClipper
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::SetColorKey(
	DWORD dwFlags,
	LPDDCOLORKEY lpDDColorKey
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::SetOverlayPosition(
	LONG lX,
	LONG lY
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::SetPalette(
	LPDIRECTDRAWPALETTE lpDDPalette
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::Unlock(
	LPVOID lpSurfaceData
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::UpdateOverlay(
	LPRECT lpSrcRect,
	LPDIRECTDRAWSURFACE lpDDDestSurface,
	LPRECT lpDestRect,
	DWORD dwFlags,
	LPDDOVERLAYFX lpDDOverlayFx
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::UpdateOverlayDisplay(
	DWORD dwFlags
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	auto& resources = this->_deviceResources;
	auto& context = resources->_d3dDeviceContext;

	/* Display VR movies in SteamVR*/
	if (g_bUseSteamVR && g_pSharedDataTgSmush != nullptr &&
		g_pSharedDataTgSmush->videoFrameIndex > 0)
	{
		// Create or resize the TgSmush texture. If we already created this texture and there's
		// no change in dimensions, CreateTgSmushTexture() will do nothing.
		resources->CreateTgSmushTexture(g_pSharedDataTgSmush->videoFrameWidth, g_pSharedDataTgSmush->videoFrameHeight);

		if (resources->_tgSmushTex != nullptr && g_VR2Doverlay != vr::k_ulOverlayHandleInvalid)
		{
			static int lastFrameRendered = -1;
			vr::Texture_t overlay_texture;

			// Avoid mapping the resource if the frame hasn't changed.
			if (lastFrameRendered != g_pSharedDataTgSmush->videoFrameIndex) {
				D3D11_MAPPED_SUBRESOURCE map;
				HRESULT hr = context->Map(resources->_tgSmushTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
				if (SUCCEEDED(hr)) {
					uint32_t size = g_pSharedDataTgSmush->videoFrameWidth * g_pSharedDataTgSmush->videoFrameHeight * 4;
					// Copy the data from TgSmush into this texture
					memcpy(map.pData, g_pSharedDataTgSmush->videoDataPtr, size);
					//memcpy(map.pData, g_pSharedDataTgSmush->videoDataPtr, g_pSharedDataTgSmush->videoDataLength);
					context->Unmap(resources->_tgSmushTex, 0);
				}
				lastFrameRendered = g_pSharedDataTgSmush->videoFrameIndex;
			}

			overlay_texture.eType = vr::TextureType_DirectX;
			overlay_texture.eColorSpace = vr::ColorSpace_Auto;
			overlay_texture.handle = resources->_tgSmushTex;
			// Fade compositor to black while the overlay is shown and we are not rendering the 3D scene.
			g_pVRCompositor->FadeToColor(0.1f, 0.0f, 0.0f, 0.0f, 1.0f, false);
			g_pVROverlay->SetOverlayTexture(g_VR2Doverlay, &overlay_texture);
			// Let's make movies larger than the regular 2D overlay:
			//g_pVROverlay->SetOverlayWidthInMeters(g_VR2Doverlay, 10.0f);
			g_pVROverlay->ShowOverlay(g_VR2Doverlay); // Movies (?)
		}
		return DD_OK;
	}		

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT PrimarySurface::UpdateOverlayZOrder(
	DWORD dwFlags,
	LPDIRECTDRAWSURFACE lpDDSReference
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

ComPtr<IDWriteTextFormat> textFormats[3];
int fontSizes[] = { 12, 16, 10 };
D2D1_POINT_2F origin;
IDWriteFactory* pDWriteFactory_;
ID2D1Factory* pD2DFactory_;
ComPtr<IDWriteTextLayout> layouts[381];
ComPtr<ID2D1SolidColorBrush> brushes[32];
int brushColors[32] = {};
bool font_initialized = false;

/*
  Computes the width of the given message. Use this function before adding messages to be
  displayed.
 */
short ComputeMsgWidth(char* str, int font_size_index) {
	int x = 0;
	if (!font_initialized)
		return x;

	unsigned char* fontWidths[] = { (unsigned char*)0x007D4C80, (unsigned char*)0x007D4D80, (unsigned char*)0x007D4E80 };
	for (int i = 0; str[i]; i++)
		x += fontWidths[font_size_index][(int)str[i]];
	return x;
}

/*
  Adds the given text to the buffer that is used by RenderText to display messages.
  This function requires Jeremy's new Text Renderer to work.

  font_size_index must be an integer in the range 0..2
		index 0: small
		index 1: regular
		index 2: smallest

  color is in 0xRRGGBB format.

  x,y must be in in-game coordinates

  returns the x coordinate after the last char rendered
 */
short DisplayText(char* str, int font_size_index, short x, short y, uint32_t color) {
	if (!font_initialized)
		return x;

	// There are 3 font sizes (see the fontSizes global definition): 12, 16, 10
	int fontSize = fontSizes[font_size_index];
	// In XwaDrawTextHook, the fontWidths are populated using DWRITE_TEXT_METRICS and other variables.
	// Here we can just use the values computed by the hook.
	// Otherwise we would need to do the following for every ASCII char:
	/*
		DWRITE_TEXT_METRICS textMetrics;
		int layoutIndex = index * 256 + (int)character;
		s_textLayouts[layoutIndex]->GetMetrics(&textMetrics);
		w = (short)textMetrics.width;
		h = (short)textMetrics.height;
	*/
	unsigned char* fontWidths[] = { (unsigned char*)0x007D4C80, (unsigned char*)0x007D4D80, (unsigned char*)0x007D4E80 };
	XwaText xwaText;

	// Common attributes for all the chars in the string
	xwaText.color = color;
	xwaText.fontSize = fontSize;

	for (int i = 0; str[i]; i++) {
		char c = str[i];
		//RenderCharHook(x, y, 0, fontSize, c, color);

		// To speed things up a little bit, I'm not calling RenderCharHook here. Instead
		// I'm inserting the text directly into g_xwa_text. However, the code below should
		// follow RenderCharHook.
		xwaText.positionX = x;
		xwaText.positionY = y;
		xwaText.textChar = c;
		g_xwa_text.push_back(xwaText);

		x += fontWidths[font_size_index][(int)c];
	}
	return x;
}

/*
  Adds centered text at the given vertical position.
  Returns the x coordinate where the next char can be placed
 */
short DisplayCenteredText(char* str, int font_size_index, short y, uint32_t color) {
	short width = ComputeMsgWidth(str, font_size_index);
	short x = (short)(g_fCurInGameWidth / 2.0f) - width / 2;
	return DisplayText(str, font_size_index, x, y, color);
}

/*
  Adds a timed message in one of the 3 available slots, overwriting and reseting
  the timer on whatever is in that slot. The hard-coded values are screen-height
  relative and seem to work fine across different resolutions.
 */
void DisplayTimedMessage(uint32_t seconds, int row, char* msg) {
	short y_pos = (short)(g_fCurInGameHeight * (0.189f + 0.05f * row));
	g_TimedMessages[row].SetMsg(msg, seconds, y_pos, FONT_LARGE_IDX, FONT_BLUE_COLOR);
}

void PrimarySurface::RenderText()
{
	static ID2D1RenderTarget* s_d2d1RenderTarget = nullptr;
	static DWORD s_displayWidth = 0;
	static DWORD s_displayHeight = 0;
	static int s_fontSizes[3] = { 12, 16, 10 };
	static ComPtr<IDWriteTextFormat> s_textFormats[3];
	static ComPtr<IDWriteTextLayout> s_textLayouts[3 * 256];
	static ComPtr<ID2D1SolidColorBrush> s_brush;
	static ComPtr<ID2D1SolidColorBrush> s_black_brush;
	static UINT s_left;
	static UINT s_top;
	static float s_scaleX;
	static float s_scaleY;
	static short s_rowSize = (short)(0.0185f * g_fCurInGameHeight);

	if (!g_PrimarySurfaceInitialized)
	{
		s_d2d1RenderTarget = nullptr;
		s_displayWidth = 0;
		s_displayHeight = 0;

		for (int index = 0; index < 3; index++)
		{
			s_textFormats[index].Release();

			for (int c = 0; c < 256; c++)
			{
				int layoutIndex = index * 256 + c;
				s_textLayouts[layoutIndex].Release();
			}
		}

		s_brush.Release();
		s_black_brush.Release();
		return;
	}

	this->_deviceResources->BeginAnnotatedEvent(L"RenderText");

	if (this->_deviceResources->_d2d1RenderTarget != s_d2d1RenderTarget || this->_deviceResources->_displayWidth != s_displayWidth || this->_deviceResources->_displayHeight != s_displayHeight)
	{
		s_d2d1RenderTarget = this->_deviceResources->_d2d1RenderTarget;
		s_displayWidth = this->_deviceResources->_displayWidth;
		s_displayHeight = this->_deviceResources->_displayHeight;

		UINT w;
		UINT h;

		if (g_config.AspectRatioPreserved)
		{
			if (this->_deviceResources->_backbufferHeight * this->_deviceResources->_displayWidth <= this->_deviceResources->_backbufferWidth * this->_deviceResources->_displayHeight)
			{
				w = this->_deviceResources->_backbufferHeight * this->_deviceResources->_displayWidth / this->_deviceResources->_displayHeight;
				h = this->_deviceResources->_backbufferHeight;
			}
			else
			{
				w = this->_deviceResources->_backbufferWidth;
				h = this->_deviceResources->_backbufferWidth * this->_deviceResources->_displayHeight / this->_deviceResources->_displayWidth;
			}
		}
		else
		{
			w = this->_deviceResources->_backbufferWidth;
			h = this->_deviceResources->_backbufferHeight;
		}

		s_left = (this->_deviceResources->_backbufferWidth - w) / 2;
		s_top = (this->_deviceResources->_backbufferHeight - h) / 2;

		s_scaleX = (float)w / (float)this->_deviceResources->_displayWidth;
		s_scaleY = (float)h / (float)this->_deviceResources->_displayHeight;

		for (int index = 0; index < 3; index++)
		{
			this->_deviceResources->_dwriteFactory->CreateTextFormat(
				g_config.TextFontFamily.c_str(),
				nullptr,
				DWRITE_FONT_WEIGHT_NORMAL,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				(float)s_fontSizes[index] * min(s_scaleX, s_scaleY),
				L"en-US",
				&s_textFormats[index]);

			for (int c = 33; c < 256; c++)
			{
				char t[2];
				t[0] = (char)c;
				t[1] = 0;

				std::wstring wtext = string_towstring(t);

				if (wtext.empty())
				{
					continue;
				}

				int layoutIndex = index * 256 + c;
				this->_deviceResources->_dwriteFactory->CreateTextLayout(
					wtext.c_str(),
					wtext.length(),
					s_textFormats[index],
					(FLOAT)this->_deviceResources->_backbufferWidth,
					(FLOAT)this->_deviceResources->_backbufferHeight,
					&s_textLayouts[layoutIndex]);
			}
		}

		this->_deviceResources->_d2d1RenderTarget->CreateSolidColorBrush(D2D1::ColorF(0), &s_brush);
		this->_deviceResources->_d2d1RenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0, 1.0f), &s_black_brush);
	}

	this->_deviceResources->_d2d1RenderTarget->SaveDrawingState(this->_deviceResources->_d2d1DrawingStateBlock);
	this->_deviceResources->_d2d1RenderTarget->BeginDraw();

	unsigned int brushColor = 0;
	s_brush->SetColor(D2D1::ColorF(brushColor));

	IDWriteTextLayout* layout = nullptr;

	if (!font_initialized)
	{
		for (int index = 0; index < 3; index++)
		{
			this->_deviceResources->_dwriteFactory->CreateTextFormat(
				g_config.TextFontFamily.c_str(),
				nullptr,
				DWRITE_FONT_WEIGHT_NORMAL,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				(float)fontSizes[index] * min(s_scaleX, s_scaleY),
				L"en-US",
				&textFormats[index]);
		}

		D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			&pD2DFactory_
		);

		DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&pDWriteFactory_)
		);

		origin = D2D1::Point2F(
			static_cast<FLOAT>(0),
			static_cast<FLOAT>(0)
		);

		for (int size_index = 0; size_index < 3; size_index++)
		{
			for (int char_index = 0; char_index < 127; char_index++)
			{
				char tm[2];
				tm[0] = char_index;
				tm[1] = 0;
				std::wstring ttext = string_towstring(tm);
				pDWriteFactory_->CreateTextLayout(
					ttext.c_str(),     
					ttext.length(),  
					textFormats[size_index],
					fontSizes[size_index] * s_scaleX,
					fontSizes[size_index] * s_scaleY,
					&layouts[size_index * 127 + char_index]
				);
			}
		}
		for (int brush_index = 0; brush_index < 32; brush_index++)
		{
			brushColors[brush_index] = -1;
		}
		font_initialized = true;
		//log_debug("[DBG] s_scaleX: %0.3f, s_scaleY: %0.3f, min: %0.3f", s_scaleX, s_scaleY, min(s_scaleX, s_scaleY));
	}

	int size_index = 0;
	for (const auto& xwaText : g_xwa_text)
	{
		if (xwaText.textChar == ' ')
		{
			continue;
		}

		IDWriteTextLayout* textLayout = nullptr;

		for (int index = 0; index < 3; index++)
		{
			if (xwaText.fontSize == s_fontSizes[index])
			{
				int layoutIndex = index * 256 + (int)(unsigned char)xwaText.textChar;
				textLayout = s_textLayouts[layoutIndex];
				break;
			}
		}

		if (!textLayout)
		{
			continue;
		}

		if (xwaText.color != brushColor)
		{
			brushColor = xwaText.color;
			s_brush->SetColor(D2D1::ColorF(brushColor));
		}

		float x = (float)s_left + (float)xwaText.positionX * s_scaleX;
		float y = (float)s_top + (float)xwaText.positionY * s_scaleY;

		this->_deviceResources->_d2d1RenderTarget->DrawTextLayout(
			D2D1_POINT_2F{ x, y },
			textLayout,
			s_brush);
	}

	// Clear the text. We'll add extra text now and render it.
	g_xwa_text.clear();
	g_xwa_text.reserve(4096);

	bool bExternalCamera = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;
	// It looks like sometimes the craftInstance table/pointer is not valid during the first few
	// frames of a new mission. I've seen some intermittent cores when playing with the GUN or
	// the MIS and then switching to other craft. So adding g_iPresentCounter > 5 seems to prevent
	// this problem, since we wait for a few frames at the beginning of the mission before we
	// try to access the CraftInstance table.
	if (g_bDynCockpitEnabled && g_bReRenderMissilesNCounterMeasures && g_iPresentCounter > 5 &&
		g_bDCApplyEraseRegionCommands && !bExternalCamera)
	{
		// Gather the data we'll need to replace the missiles and countermeasures
		CraftInstance *craftInstance = GetPlayerCraftInstanceSafe();
		if (craftInstance == NULL) goto out;

		int speed = (int)(PlayerDataTable[*g_playerIndex].currentSpeed / 2.25f);
		short throttle = (short)(100.0f * craftInstance->EngineThrottleInput / 65535.0f);
		int countL = 0, countR = 0, countermeasures = 0;
		countermeasures = craftInstance->CountermeasureAmount;
		int warheadStartIdx = craftInstance->NumberOfLasers;
		// If the secondary warheads are armed, then offset the warheadStartIdx accordingly:
		if (PlayerDataTable[*g_playerIndex].warheadArmed && PlayerDataTable[*g_playerIndex].primarySecondaryArmed)
			warheadStartIdx += 2;
		if (warheadStartIdx < 16)
			countL = craftInstance->Hardpoints[warheadStartIdx++].Count;
		if (warheadStartIdx < 16)
			countR = craftInstance->Hardpoints[warheadStartIdx++].Count;

		// The following will render a rectangle on the text buffer, which is then captured by DC. This can be used
		// to clear text right here without using more shaders.
		//D2D1_RECT_F rect = D2D1::RectF(0.0f, 100.0f, 1500.0f, 800.0f);
		//this->_deviceResources->_d2d1RenderTarget->FillRectangle(&rect, s_black_brush);

		// The following can be used to draw laser energy bars to the DC buffer: it gets captured
		// in the CMD. Coords are in the backbuffer frame (desktop resolution).
		//D2D1_RECT_F rectR = D2D1::RectF(1200.0f,1100.0f, 1800.0f,1600.0f);
		//this->_deviceResources->_d2d1RenderTarget->FillRectangle(&rectR, s_red_brush);

		// Clear the box for the missiles, countermeasures, speed & throttle, and add new 
		// text to replace the ones we're clearing here
		D2D1_RECT_F rect;
		DCElemSrcBox *dcElemSrcBox;
		float fx, fy;
		static short ofs = (short)(0.005f * g_fCurInGameHeight);

		dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[MISSILES_DC_ELEM_SRC_IDX];
		if (dcElemSrcBox->bComputed) {
			rect.left = g_fCurScreenWidth * dcElemSrcBox->coords.x0;
			rect.top = g_fCurScreenHeight * dcElemSrcBox->coords.y0;
			rect.right = g_fCurScreenWidth * dcElemSrcBox->coords.x1;
			rect.bottom = g_fCurScreenHeight * dcElemSrcBox->coords.y1;
			this->_deviceResources->_d2d1RenderTarget->FillRectangle(&rect, s_black_brush);
			
			ScreenCoordsToInGame(g_nonVRViewport.TopLeftX, g_nonVRViewport.TopLeftY,
				g_nonVRViewport.Width, g_nonVRViewport.Height,
				rect.left, rect.top, &fx, &fy);
			char buf[40];
			sprintf_s(buf, 40, "%d %d", countL, countR);
			DisplayText(buf, FONT_MEDIUM_IDX, (short)fx + ofs, (short)fy + ofs, 0xFFFFFF);
		}

		dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[NUM_CRAFTS_DC_ELEM_SRC_IDX];
		if (dcElemSrcBox->bComputed) {
			rect.left = g_fCurScreenWidth * dcElemSrcBox->coords.x0;
			rect.top = g_fCurScreenHeight * dcElemSrcBox->coords.y0;
			rect.right = g_fCurScreenWidth * dcElemSrcBox->coords.x1;
			rect.bottom = g_fCurScreenHeight * dcElemSrcBox->coords.y1;
			this->_deviceResources->_d2d1RenderTarget->FillRectangle(&rect, s_black_brush);
			ScreenCoordsToInGame(g_nonVRViewport.TopLeftX, g_nonVRViewport.TopLeftY,
				g_nonVRViewport.Width, g_nonVRViewport.Height,
				rect.left, rect.top, &fx, &fy);

			char buf[40];
			sprintf_s(buf, 40, "%d", countermeasures);
			DisplayText(buf, FONT_MEDIUM_IDX, (short)fx + ofs, (short)fy + ofs, 0xFFFFFF);
		}

		dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[SPEED_N_THROTTLE_DC_ELEM_SRC_IDX];
		if (dcElemSrcBox->bComputed) {
			rect.left = g_fCurScreenWidth * dcElemSrcBox->coords.x0;
			rect.top = g_fCurScreenHeight * dcElemSrcBox->coords.y0;
			rect.right = g_fCurScreenWidth * dcElemSrcBox->coords.x1;
			rect.bottom = g_fCurScreenHeight * dcElemSrcBox->coords.y1;
			this->_deviceResources->_d2d1RenderTarget->FillRectangle(&rect, s_black_brush);
			ScreenCoordsToInGame(g_nonVRViewport.TopLeftX, g_nonVRViewport.TopLeftY,
				g_nonVRViewport.Width, g_nonVRViewport.Height,
				rect.left, rect.top, &fx, &fy);

			char buf[40];
			sprintf_s(buf, 40, "SPD: %d", speed);
			DisplayText(buf, FONT_MEDIUM_IDX, (short)fx + ofs, (short)fy + ofs, 0xFFFFFF);
			sprintf_s(buf, 40, "THR: %d%%", throttle);
			DisplayText(buf, FONT_MEDIUM_IDX, (short)fx + ofs, (short)fy + ofs + s_rowSize, 0xFFFFFF);
		}
	}

out:
	// Add the custom text that will be displayed
	for (int i = 0; i < MAX_TIMED_MESSAGES; i++) {
		if (g_TimedMessages[i].IsExpired())
			continue;
		g_TimedMessages[i].Tick();
		DisplayCenteredText(g_TimedMessages[i].msg, g_TimedMessages[i].font_size_idx,
			g_TimedMessages[i].y, g_TimedMessages[i].color);
	}

	// Render the additional text
	size_index = 0;
	for (const auto& xwaText : g_xwa_text)
	{
		if (xwaText.textChar == ' ')
		{
			continue;
		}

		IDWriteTextLayout* textLayout = nullptr;

		for (int index = 0; index < 3; index++)
		{
			if (xwaText.fontSize == s_fontSizes[index])
			{
				int layoutIndex = index * 256 + (int)(unsigned char)xwaText.textChar;
				textLayout = s_textLayouts[layoutIndex];
				break;
			}
		}

		if (!textLayout)
		{
			continue;
		}

		if (xwaText.color != brushColor)
		{
			brushColor = xwaText.color;
			s_brush->SetColor(D2D1::ColorF(brushColor));
		}

		float x = (float)s_left + (float)xwaText.positionX * s_scaleX;
		float y = (float)s_top + (float)xwaText.positionY * s_scaleY;

		this->_deviceResources->_d2d1RenderTarget->DrawTextLayout(
			D2D1_POINT_2F{ x, y },
			textLayout,
			s_brush);
	}

	this->_deviceResources->_d2d1RenderTarget->EndDraw();
	this->_deviceResources->_d2d1RenderTarget->RestoreDrawingState(this->_deviceResources->_d2d1DrawingStateBlock);

	g_xwa_text.clear();
	g_xwa_text.reserve(4096);

	this->_deviceResources->EndAnnotatedEvent();
}

void PrimarySurface::RenderRadar()
{
	static ID2D1RenderTarget* s_d2d1RenderTarget = nullptr;
	static DWORD s_displayWidth = 0;
	static DWORD s_displayHeight = 0;
	static ComPtr<ID2D1SolidColorBrush> s_brush;
	static UINT s_left;
	static UINT s_top;
	static float s_scaleX;
	static float s_scaleY;

	if (!g_PrimarySurfaceInitialized)
	{
		s_d2d1RenderTarget = nullptr;
		s_displayWidth = 0;
		s_displayHeight = 0;

		s_brush.Release();
		return;
	}

	this->_deviceResources->BeginAnnotatedEvent(L"RenderRadar");

	if (this->_deviceResources->_d2d1RenderTarget != s_d2d1RenderTarget || this->_deviceResources->_displayWidth != s_displayWidth || this->_deviceResources->_displayHeight != s_displayHeight)
	{
		s_d2d1RenderTarget = this->_deviceResources->_d2d1RenderTarget;
		s_displayWidth = this->_deviceResources->_displayWidth;
		s_displayHeight = this->_deviceResources->_displayHeight;

		UINT w;
		UINT h;

		if (g_config.AspectRatioPreserved)
		{
			if (this->_deviceResources->_backbufferHeight * this->_deviceResources->_displayWidth <= this->_deviceResources->_backbufferWidth * this->_deviceResources->_displayHeight)
			{
				w = this->_deviceResources->_backbufferHeight * this->_deviceResources->_displayWidth / this->_deviceResources->_displayHeight;
				h = this->_deviceResources->_backbufferHeight;
			}
			else
			{
				w = this->_deviceResources->_backbufferWidth;
				h = this->_deviceResources->_backbufferWidth * this->_deviceResources->_displayHeight / this->_deviceResources->_displayWidth;
			}
		}
		else
		{
			w = this->_deviceResources->_backbufferWidth;
			h = this->_deviceResources->_backbufferHeight;
		}

		s_left = (this->_deviceResources->_backbufferWidth - w) / 2;
		s_top = (this->_deviceResources->_backbufferHeight - h) / 2;

		s_scaleX = (float)w / (float)this->_deviceResources->_displayWidth;
		s_scaleY = (float)h / (float)this->_deviceResources->_displayHeight;

		this->_deviceResources->_d2d1RenderTarget->CreateSolidColorBrush(D2D1::ColorF(0), &s_brush);
	}

	this->_deviceResources->_d2d1RenderTarget->SaveDrawingState(this->_deviceResources->_d2d1DrawingStateBlock);
	this->_deviceResources->_d2d1RenderTarget->BeginDraw();

	unsigned int brushColor = 0;
	s_brush->SetColor(D2D1::ColorF(brushColor));

	for (const auto& xwaRadar : g_xwa_radar)
	{
		unsigned short si = ((unsigned short*)0x08D9420)[xwaRadar.colorIndex];
		unsigned int esi;

		if (((bool(*)())0x0050DC50)() != 0)
		{
			unsigned short eax = si & 0x001F;
			unsigned short ecx = si & 0x7C00;
			unsigned short edx = si & 0x03E0;

			esi = (eax << 3) | (edx << 6) | (ecx << 9);
		}
		else
		{
			unsigned short eax = si & 0x001F;
			unsigned short edx = si & 0xF800;
			unsigned short ecx = si & 0x07E0;

			esi = (eax << 3) | (ecx << 5) | (edx << 8);
		}

		if (esi != brushColor)
		{
			brushColor = esi;
			s_brush->SetColor(D2D1::ColorF(brushColor));
		}

		float x = s_left + (float)xwaRadar.positionX * s_scaleX;
		float y = s_top + (float)xwaRadar.positionY * s_scaleY;

		float deltaX = 2.0f * s_scaleX;
		float deltaY = 2.0f * s_scaleY;

		this->_deviceResources->_d2d1RenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), deltaX, deltaY), s_brush);
	}

	if (g_xwa_radar_selected_positionX != -1 && g_xwa_radar_selected_positionY != -1)
	{
		s_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Yellow));

		float x = s_left + (float)g_xwa_radar_selected_positionX * s_scaleX;
		float y = s_top + (float)g_xwa_radar_selected_positionY * s_scaleY;

		float deltaX = 4.0f * s_scaleX;
		float deltaY = 4.0f * s_scaleY;

		this->_deviceResources->_d2d1RenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), deltaX, deltaY), s_brush);
	}

	this->_deviceResources->_d2d1RenderTarget->EndDraw();
	this->_deviceResources->_d2d1RenderTarget->RestoreDrawingState(this->_deviceResources->_d2d1DrawingStateBlock);

	g_xwa_radar.clear();
	g_xwa_radar_selected_positionX = -1;
	g_xwa_radar_selected_positionY = -1;

	this->_deviceResources->EndAnnotatedEvent();
}

void PrimarySurface::RenderBracket()
{
	static ID2D1RenderTarget* s_d2d1RenderTarget = nullptr;
	static DWORD s_displayWidth = 0;
	static DWORD s_displayHeight = 0;
	// It's probably not necessary to have two brushes, but I don't think it hurts either and
	// I'm doing this just in case brushes can't be shared between different RTVs
	static ComPtr<ID2D1SolidColorBrush> s_brushOffscreen, s_brushDC, s_brush;
	static UINT s_left;
	static UINT s_top;
	static float s_scaleX;
	static float s_scaleY;

	if (!g_PrimarySurfaceInitialized)
	{
		s_d2d1RenderTarget = nullptr;
		s_displayWidth = 0;
		s_displayHeight = 0;

		s_brush.Release();
		s_brushDC.Release();
		s_brushOffscreen.Release();
		return;
	}

	this->_deviceResources->BeginAnnotatedEvent(L"RenderBracket");

	if (this->_deviceResources->_d2d1RenderTarget != s_d2d1RenderTarget || this->_deviceResources->_displayWidth != s_displayWidth || this->_deviceResources->_displayHeight != s_displayHeight)
	{
		s_d2d1RenderTarget = this->_deviceResources->_d2d1RenderTarget;
		s_displayWidth = this->_deviceResources->_displayWidth;
		s_displayHeight = this->_deviceResources->_displayHeight;

		UINT w;
		UINT h;

		if (g_config.AspectRatioPreserved)
		{
			if (this->_deviceResources->_backbufferHeight * this->_deviceResources->_displayWidth <= this->_deviceResources->_backbufferWidth * this->_deviceResources->_displayHeight)
			{
				w = this->_deviceResources->_backbufferHeight * this->_deviceResources->_displayWidth / this->_deviceResources->_displayHeight;
				h = this->_deviceResources->_backbufferHeight;
			}
			else
			{
				w = this->_deviceResources->_backbufferWidth;
				h = this->_deviceResources->_backbufferWidth * this->_deviceResources->_displayHeight / this->_deviceResources->_displayWidth;
			}
		}
		else
		{
			w = this->_deviceResources->_backbufferWidth;
			h = this->_deviceResources->_backbufferHeight;
		}

		s_left = (this->_deviceResources->_backbufferWidth - w) / 2;
		s_top = (this->_deviceResources->_backbufferHeight - h) / 2;

		s_scaleX = (float)w / (float)this->_deviceResources->_displayWidth;
		s_scaleY = (float)h / (float)this->_deviceResources->_displayHeight;

		this->_deviceResources->_d2d1OffscreenRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0), &s_brushOffscreen);
		this->_deviceResources->_d2d1DCRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0), &s_brushDC);
	}

	this->_deviceResources->_d2d1OffscreenRenderTarget->SaveDrawingState(this->_deviceResources->_d2d1DrawingStateBlock);
	this->_deviceResources->_d2d1OffscreenRenderTarget->BeginDraw();

	this->_deviceResources->_d2d1DCRenderTarget->SaveDrawingState(this->_deviceResources->_d2d1DrawingStateBlock);
	this->_deviceResources->_d2d1DCRenderTarget->BeginDraw();

	unsigned int brushColor = 0;
	s_brushOffscreen->SetColor(D2D1::ColorF(brushColor));
	s_brushDC->SetColor(D2D1::ColorF(brushColor));

	for (const auto& xwaBracket : g_xwa_bracket)
	{
		unsigned short si = ((unsigned short*)0x08D9420)[xwaBracket.colorIndex];
		unsigned int esi;

		if (((bool(*)())0x0050DC50)() != 0)
		{
			unsigned short eax = si & 0x001F;
			unsigned short ecx = si & 0x7C00;
			unsigned short edx = si & 0x03E0;

			esi = (eax << 3) | (edx << 6) | (ecx << 9);
		}
		else
		{
			unsigned short eax = si & 0x001F;
			unsigned short edx = si & 0xF800;
			unsigned short ecx = si & 0x07E0;

			esi = (eax << 3) | (ecx << 5) | (edx << 8);
		}

		if (esi != brushColor)
		{
			brushColor = esi;
			s_brushOffscreen->SetColor(D2D1::ColorF(brushColor));
			s_brushDC->SetColor(D2D1::ColorF(brushColor));
		}

		float posX = s_left + (float)xwaBracket.positionX * s_scaleX;
		float posY = s_top + (float)xwaBracket.positionY * s_scaleY;
		float posW = (float)xwaBracket.width * s_scaleX;
		float posH = (float)xwaBracket.height * s_scaleY;
		float posSide = 0.125;

		float strokeWidth = 2.0f * min(s_scaleX, s_scaleY);

		bool fill = xwaBracket.width <= 4 || xwaBracket.height <= 4;
		// Select the DC RTV if this bracket was classified as belonging to
		// the Dynamic Cockpit display
		ID2D1RenderTarget *rtv = xwaBracket.DC ? this->_deviceResources->_d2d1DCRenderTarget : this->_deviceResources->_d2d1OffscreenRenderTarget;
		s_brush = xwaBracket.DC ? s_brushDC : s_brushOffscreen;

		if (fill)
		{
			rtv->FillRectangle(D2D1::RectF(posX, posY, posX + posW, posY + posH), s_brush);
		}
		else
		{
			// top left
			rtv->DrawLine(D2D1::Point2F(posX, posY), D2D1::Point2F(posX + posW * posSide, posY), s_brush, strokeWidth);
			rtv->DrawLine(D2D1::Point2F(posX, posY), D2D1::Point2F(posX, posY + posH * posSide), s_brush, strokeWidth);

			// top right
			rtv->DrawLine(D2D1::Point2F(posX + posW - posW * posSide, posY), D2D1::Point2F(posX + posW, posY), s_brush, strokeWidth);
			rtv->DrawLine(D2D1::Point2F(posX + posW, posY), D2D1::Point2F(posX + posW, posY + posH * posSide), s_brush, strokeWidth);

			// bottom left
			rtv->DrawLine(D2D1::Point2F(posX, posY + posH - posH * posSide), D2D1::Point2F(posX, posY + posH), s_brush, strokeWidth);
			rtv->DrawLine(D2D1::Point2F(posX, posY + posH), D2D1::Point2F(posX + posW * posSide, posY + posH), s_brush, strokeWidth);

			// bottom right
			rtv->DrawLine(D2D1::Point2F(posX + posW - posW * posSide, posY + posH), D2D1::Point2F(posX + posW, posY + posH), s_brush, strokeWidth);
			rtv->DrawLine(D2D1::Point2F(posX + posW, posY + posH - posH * posSide), D2D1::Point2F(posX + posW, posY + posH), s_brush, strokeWidth);
		}
	}

	this->_deviceResources->_d2d1DCRenderTarget->EndDraw();
	this->_deviceResources->_d2d1DCRenderTarget->RestoreDrawingState(this->_deviceResources->_d2d1DrawingStateBlock);

	this->_deviceResources->_d2d1OffscreenRenderTarget->EndDraw();
	this->_deviceResources->_d2d1OffscreenRenderTarget->RestoreDrawingState(this->_deviceResources->_d2d1DrawingStateBlock);

	// Need to set s_brush to NULL to avoid dangling references. Otherwise, we may get
	// crashes when exiting.
	s_brush = nullptr;
	g_xwa_bracket.clear();

	this->_deviceResources->EndAnnotatedEvent();
}

void PrimarySurface::CacheBracketsVR()
{
	const bool bExternalCamera = g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME &&
		PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;

	// Save the current projection constants
	float f0 = *(float*)0x08C1600;
	float f1 = *(float*)0x0686ACC;
	float f2 = *(float*)0x080ACF8;
	float f3 = *(float*)0x07B33C0;
	float f4 = *(float*)0x064D1AC;

	if (bExternalCamera)
	{
		// These are the constants used when rendering the cockpit. The HUD is not at the center
		// of the screen when using these:
		*(float*)0x08C1600 = g_f0x08C1600;
		*(float*)0x0686ACC = g_f0x0686ACC;
		*(float*)0x080ACF8 = g_f0x080ACF8;
		*(float*)0x07B33C0 = g_f0x07B33C0;
		*(float*)0x064D1AC = g_f0x064D1AC;
	}

	const float Zfar = *(float*)0x05B46B4;
	unsigned int brushColor = 0;
	const Vector4 screenCenter = Vector4(g_fCurInGameWidth / 2.0f, g_fCurInGameHeight / 2.0f, 0, 1);
	// Back-project the center of the screen:
	float X, Y, desiredZ = 65536.0f;
	// The following division is correct. Znear is actually greater than Zfar because the
	// depth is inverted (0 is far, 1 is close). It's also normalized.
	float Z = Zfar / (desiredZ * METERS_TO_OPT + Zfar);
	float3 C = InverseTransformProjectionScreen({ screenCenter.x, screenCenter.y, Z, Z }); // CacheBracketsVR
	C.y = -C.y;
	C.z = -C.z;

	// Let's measure the width of the stroke now:
	constexpr float STROKE_WIDTH_PX = 1.0f;
	X = screenCenter.x + STROKE_WIDTH_PX;
	Y = screenCenter.y;
	float3 U = InverseTransformProjectionScreen({ X, Y, Z, Z }); // CacheBracketsVR
	// This is the width of the stroke in OPT coordinates at the given depth
	const float strokeWidthOPT = fabs(C.x - U.x);

	for (const auto& xwaBracket : g_xwa_bracket)
	{
		unsigned short si = ((unsigned short*)0x08D9420)[xwaBracket.colorIndex];
		unsigned int esi;

		if (((bool(*)())0x0050DC50)() != 0)
		{
			unsigned short eax = si & 0x001F;
			unsigned short ecx = si & 0x7C00;
			unsigned short edx = si & 0x03E0;

			esi = (eax << 3) | (edx << 6) | (ecx << 9);
		}
		else
		{
			unsigned short eax = si & 0x001F;
			unsigned short edx = si & 0xF800;
			unsigned short ecx = si & 0x07E0;

			esi = (eax << 3) | (ecx << 5) | (edx << 8);
		}

		if (esi != brushColor)
		{
			brushColor = esi;
		}

		// Skip brackets that should be rendered in the CMD:
		//if (xwaBracket.DC)
		//	continue;

		// xwaBracket is in in-game coords.
		// xwaBracket.positionX/Y is the top-left corner, so adding halfWidth/Height gives us
		// the center of the bracket:
		X = (float)(xwaBracket.positionX + xwaBracket.width  / 2.0f);
		Y = (float)(xwaBracket.positionY + xwaBracket.height / 2.0f);
		float3 V = InverseTransformProjectionScreen({ X, Y, Z, Z }); // CacheBracketsVR
		V.y = -V.y;
		V.z = -V.z;

		// This would be the lower-right corner of the bracket. Measuring it from the center
		// of the screen also works.
		X = screenCenter.x + xwaBracket.width  / 2.0f;
		Y = screenCenter.y + xwaBracket.height / 2.0f;
		float3 W = InverseTransformProjectionScreen({ X, Y, Z, Z }); // CacheBracketsVR
		W.y = -W.y;
		W.z = -W.z;

		BracketVR bracketVR;
		bracketVR.posOPT.x = V.x;
		bracketVR.posOPT.y = V.z;
		bracketVR.posOPT.z = V.y;
		bracketVR.halfWidthOPT = fabs(W.x - C.x);
		bracketVR.strokeWidth  = strokeWidthOPT / (2.0f * bracketVR.halfWidthOPT);
		bracketVR.color.x = (float)((brushColor >> 16) & 0xFF) / 255.0f;
		bracketVR.color.y = (float)((brushColor >>  8) & 0xFF) / 255.0f;
		bracketVR.color.z = (float)((brushColor >>  0) & 0xFF) / 255.0f;
		g_bracketsVR.push_back(bracketVR);
	}
	g_xwa_bracket.clear();

	// Restore the original projection deltas
	if (bExternalCamera)
	{
		*(float*)0x08C1600 = f0;
		*(float*)0x0686ACC = f1;
		*(float*)0x080ACF8 = f2;
		*(float*)0x07B33C0 = f3;
		*(float*)0x064D1AC = f4;
	}

	// This method should only be called in VR mode:
	EffectsRenderer* renderer = (EffectsRenderer*)g_current_renderer;
	renderer->RenderVRBrackets();
}

void PrimarySurface::RenderSkyBoxVR(bool debug)
{
	if (!g_bUseSteamVR || !g_bRendering3D) return;

	const bool bExternalCamera = g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME &&
		PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;

	auto& resources = _deviceResources;
	auto& context = resources->_d3dDeviceContext;

	// DEBUG: Replace the sides of the texture map with custom data. In this case, the sides
	// will be painted different colors.
#ifdef DISABLED
	bool bFirstTime = true;
	if (bFirstTime)
	{
		D3D11_BOX box;
		box.left = 0;
		box.top = 0;
		box.front = 0;
		box.right = 1024;
		box.bottom = 1024;
		box.back = 1;
		float colors[6][4] = {
			{ 1.0f, 0.0f, 0.0f, 1.0f },
			{ 0.0f, 1.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 1.0f, 1.0f },

			{ 0.0f, 1.0f, 1.0f, 1.0f },
			{ 1.0f, 0.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 0.0f, 1.0f }
		};

		for (int i = 0; i < 6; i++)
		{
			context->ClearRenderTargetView(resources->_shadertoyRTV, colors[i]);
			context->ResolveSubresource(resources->_shadertoyBuf, 0, resources->_shadertoyBufMSAA, 0, BACKBUFFER_FORMAT);
			context->CopySubresourceRegion(resources->_textureCube, i, 0, 0, 0,
				resources->_shadertoyBuf, 0, &box);
		}
		bFirstTime = true;
	}
#endif

	const float Zfar = *(float*)0x05B46B4;
	// Save the current projection constants
	float f0 = *(float*)0x08C1600;
	float f1 = *(float*)0x0686ACC;
	float f2 = *(float*)0x080ACF8;
	float f3 = *(float*)0x07B33C0;
	float f4 = *(float*)0x064D1AC;

	if (bExternalCamera)
	{
		// These are the constants used when rendering the cockpit. The HUD is not at the center
		// of the screen when using these:
		*(float*)0x08C1600 = g_f0x08C1600;
		*(float*)0x0686ACC = g_f0x0686ACC;
		*(float*)0x080ACF8 = g_f0x080ACF8;
		*(float*)0x07B33C0 = g_f0x07B33C0;
		*(float*)0x064D1AC = g_f0x064D1AC;
	}

	//Matrix4 Heading;
	//GetHyperspaceEffectMatrix(&Heading);
	//GetCockpitViewMatrix(&Heading);
	Vector4 Rs, Us, Fs;
	// Maybe try using GetCurrentHeadingViewMatrix() instead?
	Matrix4 Heading = GetCurrentHeadingMatrix(Rs, Us, Fs, false);
	Matrix4 ViewMatrix = g_VSMatrixCB.fullViewMat; // See RenderSpeedEffect() for details
	ViewMatrix.invert();
	Matrix4 S = Matrix4().scale(-1, -1, 1);
	ViewMatrix = S * ViewMatrix * S * Heading;

	// DEBUG:
	Vector4 U = ViewMatrix * Vector4(0, 0, 1, 0);
	Vector4 F = ViewMatrix * Vector4(0, -1, 0, 0);
	g_VRGeometryCBuffer.U = float4(U.x, U.y, U.z, 0);
	g_VRGeometryCBuffer.F = float4(F.x, F.y, F.z, 0);
	//log_debug_vr("Up: %0.3f, %0.3f, %0.3f", U.x, U.y, U.z);
	//log_debug_vr("Fd: %0.3f, %0.3f, %0.3f", F.x, F.y, F.z);

	// ViewMatrix maps OPT-World coords to "Normal Mapping"/DX11 Viewspace coords:
	// X+ (Rt, OPT) maps to X+
	// Z+ (Up, OPT) maps to Y+
	// Y- (Fd, OPT) maps to Z+
	// In order to map from DX11 Viewspace coords to DX11 World coords, we need
	// to invert ViewMatrix and then rename Z+ --> Y+ and Z+ --> Y-
	Matrix4 swapScale({ 1,0,0,0,  0,0,1,0,  0,-1,0,0,  0,0,0,1 });
	// Finally, the skymap appears to be upside down, so let's rotate it around the
	// Z axis to recover the normal orientation:
	Matrix4 SkyMapRotation = Matrix4().rotateZ(180.0f);
	ViewMatrix.invert();
	g_VRGeometryCBuffer.viewMat = SkyMapRotation * swapScale * ViewMatrix;

	static bool bBracketIsCached = false;
	static BracketVR screenBracket = {};

	// Add a single bracket covering the whole screen
	g_bracketsVR.clear();
	if (!bBracketIsCached)
	{
		float W = g_fCurInGameWidth;
		float H = g_fCurInGameHeight;
		float desiredZ = 65536.0f;
		float X = W / 2.0f, Y = H / 2.0f;
		float Z = Zfar / (desiredZ * METERS_TO_OPT);
		float3 P = InverseTransformProjectionScreen({ X, Y, Z, Z });
		P.y = -P.y;
		P.z = -P.z;

		float3 Q = InverseTransformProjectionScreen({ X + W, Y + H, Z, Z });
		Q.y = -Q.y;
		Q.z = -Q.z;

		screenBracket.posOPT.x = P.x;
		screenBracket.posOPT.y = P.z;
		screenBracket.posOPT.z = P.y;
		screenBracket.halfWidthOPT = fabs(Q.x - P.x);
		screenBracket.color = Vector3(1, 1, 1);
		//if (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME) bBracketIsCached = true;
	}
	g_bracketsVR.push_back(screenBracket);

	// Restore the original projection deltas
	if (bExternalCamera)
	{
		*(float*)0x08C1600 = f0;
		*(float*)0x0686ACC = f1;
		*(float*)0x080ACF8 = f2;
		*(float*)0x07B33C0 = f3;
		*(float*)0x064D1AC = f4;
	}

	// This method should only be called in VR mode:
	EffectsRenderer* renderer = (EffectsRenderer*)g_current_renderer;
	renderer->RenderVRSkyBox(debug);
}

void PrimarySurface::RenderSkyCylinder()
{
	if (!g_bRendering3D) return;

	const bool bExternalCamera = g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME &&
		PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;

	//Matrix4 Heading;
	//GetHyperspaceEffectMatrix(&Heading);
	//GetCockpitViewMatrix(&Heading);
	Vector4 Rs, Us, Fs;
	// Maybe try using GetCurrentHeadingViewMatrix() instead?
	Matrix4 Heading = GetCurrentHeadingMatrix(Rs, Us, Fs, false);
	//Matrix4 ViewMatrix = g_VSMatrixCB.fullViewMat; // See RenderSpeedEffect() for details
	//ViewMatrix.invert();
	//Matrix4 S = Matrix4().scale(-1, -1, 1);
	//ViewMatrix = S * ViewMatrix * S * Heading;

	// Non-VR path:
	Matrix4 ViewMatrix;
	GetCockpitViewMatrixSpeedEffectOPTSys(&ViewMatrix, false);
	ViewMatrix = ViewMatrix * GetPlayerCraftMatrix(Rs, Us, Fs, -1.0f, false, false);

	// DEBUG:
	Vector4 U = ViewMatrix * Vector4(0, 0, 1, 0);
	Vector4 F = ViewMatrix * Vector4(0, -1, 0, 0);
	g_VRGeometryCBuffer.U = float4(U.x, U.y, U.z, 0);
	g_VRGeometryCBuffer.F = float4(F.x, F.y, F.z, 0);
	//log_debug_vr("Up: %0.3f, %0.3f, %0.3f", U.x, U.y, U.z);
	//log_debug_vr("Fd: %0.3f, %0.3f, %0.3f", F.x, F.y, F.z);

	// ViewMatrix maps OPT-World coords to "Normal Mapping"/DX11 Viewspace coords:
	// X+ (Rt, OPT) maps to X+
	// Z+ (Up, OPT) maps to Y+
	// Y- (Fd, OPT) maps to Z+
	// In order to map from DX11 Viewspace coords to DX11 World coords, we need
	// to invert ViewMatrix and then rename Z+ --> Y+ and Z+ --> Y-
	//Matrix4 swapScale({ 1,0,0,0,  0,0,1,0,  0,-1,0,0,  0,0,0,1 });
	//ViewMatrix.invert();
	//g_VRGeometryCBuffer.viewMat = swapScale * ViewMatrix;
	g_VRGeometryCBuffer.viewMat = ViewMatrix;

	// This method should only be called in VR mode:
	EffectsRenderer* renderer = (EffectsRenderer*)g_current_renderer;
	renderer->RenderSkyCylinder();
}

/*
 * Renders synthetic DC elements, like the Laser and Ion energy levels and the Throttle-as-a-bar
 */
void PrimarySurface::RenderSynthDCElems()
{
	static ID2D1RenderTarget* s_d2d1RenderTarget = nullptr;
	static DWORD s_displayWidth = 0;
	static DWORD s_displayHeight = 0;
	static ComPtr<ID2D1SolidColorBrush> s_gray_brush, s_content_brush;
	D2D1_RECT_F rect;
	DCElemSrcBox *dcElemSrcBox;
	bool bExternalCamera = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;
	//static UINT s_left;
	//static UINT s_top;
	//static float s_scaleX;
	//static float s_scaleY;

	if (!g_PrimarySurfaceInitialized)
	{
		s_d2d1RenderTarget = nullptr;
		s_displayWidth = 0;
		s_displayHeight = 0;

		s_gray_brush.Release();
		s_content_brush.Release();
		return;
	}

	this->_deviceResources->BeginAnnotatedEvent(L"RenderSynthDCElems");

	if (this->_deviceResources->_d2d1RenderTarget != s_d2d1RenderTarget || this->_deviceResources->_displayWidth != s_displayWidth || this->_deviceResources->_displayHeight != s_displayHeight)
	{
		s_d2d1RenderTarget = this->_deviceResources->_d2d1RenderTarget;
		s_displayWidth = this->_deviceResources->_displayWidth;
		s_displayHeight = this->_deviceResources->_displayHeight;

		UINT w;
		UINT h;

		if (g_config.AspectRatioPreserved)
		{
			if (this->_deviceResources->_backbufferHeight * this->_deviceResources->_displayWidth <= this->_deviceResources->_backbufferWidth * this->_deviceResources->_displayHeight)
			{
				w = this->_deviceResources->_backbufferHeight * this->_deviceResources->_displayWidth / this->_deviceResources->_displayHeight;
				h = this->_deviceResources->_backbufferHeight;
			}
			else
			{
				w = this->_deviceResources->_backbufferWidth;
				h = this->_deviceResources->_backbufferWidth * this->_deviceResources->_displayHeight / this->_deviceResources->_displayWidth;
			}
		}
		else
		{
			w = this->_deviceResources->_backbufferWidth;
			h = this->_deviceResources->_backbufferHeight;
		}

		//s_left = (this->_deviceResources->_backbufferWidth - w) / 2;
		//s_top = (this->_deviceResources->_backbufferHeight - h) / 2;

		//s_scaleX = (float)w / (float)this->_deviceResources->_displayWidth;
		//s_scaleY = (float)h / (float)this->_deviceResources->_displayHeight;

		this->_deviceResources->_d2d1RenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x101010, 1.0f), &s_gray_brush);
		this->_deviceResources->_d2d1RenderTarget->CreateSolidColorBrush(D2D1::ColorF(0xFF0000, 0.35f), &s_content_brush);
	}

	this->_deviceResources->_d2d1RenderTarget->SaveDrawingState(this->_deviceResources->_d2d1DrawingStateBlock);
	this->_deviceResources->_d2d1RenderTarget->BeginDraw();

	// Fetch the pointer to the current CraftInstance
	CraftInstance *craftInstance = GetPlayerCraftInstanceSafe();
	if (craftInstance == NULL) goto out;

	// The Simple HUD is activated with Alt+. and it only displays the reticle
	if (g_bRenderLaserIonEnergyLevels && !PlayerDataTable[*g_playerIndex].simpleHUD && *g_IsCMDVisible &&
		g_GameEvent.CockpitInstruments.LaserIon) 
	{
		//log_debug("[DBG] HUD visible: %d, CMD Visible: %d", PlayerDataTable[*g_playerIndex].simpleHUD, *g_IsCMDVisible);
		dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[EIGHT_LASERS_BOTH_SRC_IDX];
		if (dcElemSrcBox->bComputed) {
			rect.left = g_fCurScreenWidth * dcElemSrcBox->coords.x0;
			rect.top = g_fCurScreenHeight * dcElemSrcBox->coords.y0;
			rect.right = g_fCurScreenWidth * dcElemSrcBox->coords.x1;
			rect.bottom = g_fCurScreenHeight * dcElemSrcBox->coords.y1;
			float H = rect.bottom - rect.top, W = rect.right - rect.left;
			float BarW = W * 0.46f, BarH = H / 4.0f;
			// Fill the whole background for this element
			//this->_deviceResources->_d2d1RenderTarget->FillRectangle(&rect, s_green_brush);
			s_content_brush->SetColor(g_DCLaserColor);

			int LaserIdx = 0;
			for (int i = 0; i < craftInstance->NumberOfLasers; i++) {
				BYTE WeaponType = craftInstance->Hardpoints[i].WeaponType;
				if (WeaponType == 1 || WeaponType == 2) {
					// Render the laser and ion energy bars
					//float Energy = (float)craftInstance->Hardpoints[i].Energy / 127.0f;
					// The game displays 12 energy bars 6 bars stacked on top of each other.
					// 127 / 12 = 10.58, so we divide by 10.58 and see how many bars we should
					// display:
					float Energy = trunc((float)(craftInstance->Hardpoints[i].Energy) / 10.58f);
					float Energy0 = Energy > 6.0f ? 1.0f : Energy / 6.0f;
					float Energy1 = Energy > 6.0f ? (Energy - 6.0f) / 6.0f : 0.0f;
					D2D1_RECT_F bar;
					bar.left = (LaserIdx & 0x1) == 0x0 ? rect.left : rect.right - BarW;
					//bar.right = bar.left + Energy * BarW;

					bar.bottom = rect.bottom - (LaserIdx >> 1) * BarH;
					bar.top = bar.bottom - BarH * 0.8f;

					// Background
					bar.right = bar.left + BarW;
					this->_deviceResources->_d2d1RenderTarget->FillRectangle(&bar, s_gray_brush);

					if (WeaponType == 1)
						s_content_brush->SetColor(g_DCLaserColor);
					else
						s_content_brush->SetColor(g_DCIonColor);
					// First bar
					bar.right = bar.left + Energy0 * BarW;
					this->_deviceResources->_d2d1RenderTarget->FillRectangle(&bar, s_content_brush);
					// Second bar
					if (Energy > 6.0f) {
						bar.right = bar.left + Energy1 * BarW;
						this->_deviceResources->_d2d1RenderTarget->FillRectangle(&bar, s_content_brush);
					}
					LaserIdx++;
				}
			}

		}
	}

	if (g_bDynCockpitEnabled && g_bRenderThrottle && g_bDCApplyEraseRegionCommands &&
		!bExternalCamera && g_GameEvent.CockpitInstruments.Throttle)
	{
		dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[THROTTLE_BAR_DC_SRC_IDX];
		if (dcElemSrcBox->bComputed) {
			rect.left = g_fCurScreenWidth * dcElemSrcBox->coords.x0;
			rect.top = g_fCurScreenHeight * dcElemSrcBox->coords.y0;
			rect.right = g_fCurScreenWidth * dcElemSrcBox->coords.x1;
			rect.bottom = g_fCurScreenHeight * dcElemSrcBox->coords.y1;
			float H = rect.bottom - rect.top, W = rect.right - rect.left;

			s_content_brush->SetColor(g_DCThrottleColor);
			float throttle = craftInstance->EngineThrottleInput / 65535.0f;
			D2D1_RECT_F bar;
			bar.left = rect.left; bar.right = rect.right;
			bar.bottom = rect.bottom;
			bar.top = rect.bottom - throttle * H;

			// Background
			this->_deviceResources->_d2d1RenderTarget->FillRectangle(&rect, s_gray_brush);
			// Throttle
			this->_deviceResources->_d2d1RenderTarget->FillRectangle(&bar, s_content_brush);
		}
	}

out:
	this->_deviceResources->_d2d1RenderTarget->EndDraw();
	this->_deviceResources->_d2d1RenderTarget->RestoreDrawingState(this->_deviceResources->_d2d1DrawingStateBlock);

	this->_deviceResources->EndAnnotatedEvent();
}
