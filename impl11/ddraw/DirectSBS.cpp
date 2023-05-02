#include "common.h"
#include <Windows.h>
#include <stdio.h>
#include "utils.h"
#include "commonVR.h"
#include "DirectSBS.h"
#include <headers/openvr.h>
#include "FreePIE.h"
#include "effects.h"

#define PSVR_VERT_FOV 106.53f
float g_fVR_FOV = PSVR_VERT_FOV;
bool g_bDirectSBSInitialized = false;

// Barrel Effect
BarrelPixelShaderCBuffer g_BarrelPSCBuffer;
extern float g_fLensK1, g_fLensK2, g_fLensK3;

bool InitDirectSBS()
{
	InitFreePIE();
	g_headCenter.set(0, 0, 0);

	g_EyeMatrixLeftInv.set
	(
		1.0f, 0.0f, 0.0f, g_fHalfIPD,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
	// Matrices are stored in column-major format, so we have to transpose them to
	// match the format above:
	g_EyeMatrixLeftInv.transpose();

	g_EyeMatrixRightInv.set
	(
		1.0f, 0.0f, 0.0f, -g_fHalfIPD,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
	// Matrices are stored in column-major format, so we have to transpose them to
	// match the format above:
	g_EyeMatrixRightInv.transpose();

	/*
	Matrix from Trinus PSVR, June 11, 2020:
	0.847458, 0.000000, 0.000000, 0.000000
	0.000000, 0.746269, 0.000000, 0.000000
	0.000000, 0.000000, -1.000010, -0.001000
	0.000000, 0.000000, -1.000000, 0.000000

	iVRy reported these values:

	projLeft:
	0.847458, 0.000000,  0.000000,  0.000000
	0.000000, 0.746269,  0.000000,  0.000000
	0.000000, 0.000000, -1.010101, -0.505050
	0.000000, 0.000000, -1.000000,  0.000000

	Raw data (Left eye):
	Left: -1.180000, Right: 1.180000, Top: -1.340000, Bottom: 1.340000
	Raw data (Right eye):
	Left: -1.180000, Right: 1.180000, Top: -1.340000, Bottom: 1.340000

	atan(1.34) = 53.26 deg, so that's 106.53 degrees vertical FOV
	*/
	g_projLeft.set
	(
		0.847458f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.746269f, 0.0f, 0.0f,
		0.0f, 0.0f, -1.000010f, -0.001f,
		0.0f, 0.0f, -1.0f, 0.0f
	);
	g_projLeft.transpose();
	g_projRight = g_projLeft;

	g_FullProjMatrixLeft = g_projLeft * g_EyeMatrixLeftInv;
	g_FullProjMatrixRight = g_projRight * g_EyeMatrixRightInv;

	//ShowMatrix4(g_EyeMatrixLeftInv, "g_EyeMatrixLeftInv");
	//ShowMatrix4(g_projLeft, "g_projLeft");
	//ShowMatrix4(g_EyeMatrixRightInv, "g_EyeMatrixRightInv");
	//ShowMatrix4(g_projRight, "g_projRight");

	// Set the vertical FOV to 106.53. This will be applied after the first frame is
	// rendered, in PrimarySurface::Flip
	g_fVR_FOV = PSVR_VERT_FOV;
	log_debug("[DBG] DirectSBS mode initialized");
	g_bDirectSBSInitialized = true;
	return true;
}

bool ShutDownDirectSBS() {
	ShutdownFreePIE();
	return true;
}