// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes (c) 2019

#pragma once

#include "materials.h"
#include "DeviceResources.h"
#include <map>

class TextureSurface;

enum WarningLightEnum {
	NONE_WLIGHT = 0,
	RETICLE_LEFT_WLIGHT,
	RETICLE_MID_LEFT_WLIGHT,
	RETICLE_MID_RIGHT_WLIGHT,
	RETICLE_RIGHT_WLIGHT,
	WARHEAD_RETICLE_LEFT_WLIGHT,
	WARHEAD_RETICLE_MID_LEFT_WLIGHT,
	WARHEAD_RETICLE_MID_RIGHT_WLIGHT,
	WARHEAD_RETICLE_RIGHT_WLIGHT
};

class Direct3DTexture : public IDirect3DTexture
{
public:
	// Set to true once this texture has been tagged/classified.
	bool is_Tagged;
	// We don't know if the current cockpit has custom reticles until we see the name of the cockpit.
	// However, the game loads HUD.dat before the cockpit resources. So, the custom reticles are
	// already loaded by the time we see the cockpit name. To allow us a second chance at tagging
	// custom reticles, we use this counter. Essentially any dat,12000,51XX, (or greater) resources
	// have to be tagged at least two times to allow them to be recognized as custom reticles.
	// The field below helps us tag these resources multiple times to recognize custom reticles.
	uint8_t TagCount;
	// These indices store the GroupId and ImageId of this texture (if it's a DAT image)
	int DATGroupId, DATImageId;
	// Used to tell whether the current texture is part of the aiming HUD and should not be scalled.
	// This flag is set during resource Load
	bool is_Reticle;
	// This flag is true only for the HUD textures that serve as crosshairs.
	bool is_ReticleCenter;
	// This flag is set whenever the target can be fired upon (12000,600 or ... how do I detect this when a custom reticle is used?)
	bool is_HighlightedReticle;
	// This flag is set to true if this texture is the triangle pointer
	bool is_TrianglePointer;
	// This flag is set to true if this texture is a font/text
	bool is_Text;
	// This flag is set to true if this is a floating GUI element (like the targeting computer)
	bool is_Floating_GUI;
	// This flag is set to true if this is a laser/ion energy bar or the bracket containing said energy bars.
	bool is_LaserIonEnergy;
	// This flag is set to true if this is a regular GUI element
	bool is_GUI;
	// This flag is set to true if this is the main targeting computer HUD background (center, low)
	bool is_TargetingComp;
	// True if this texture is a laser or ion and should be shadeless/bright
	bool is_Laser;
	// True if this texture is a turbolaser.
	bool is_TurboLaser;
	// True if this is an "illumination" or "light" texture
	bool is_LightTexture;
	// True if this is a texture with transparency
	bool is_Transparent;
	// True if this is an Engine Glow texture
	bool is_EngineGlow;
	// True if this is an Electricity (Sparks or Electric Arcs) texture
	bool is_Electricity;
	// True if this is an Explosion texture
	bool is_Explosion;
	// True if this is smoke coming from an explosion
	bool is_Smoke;
	// True if this texture is a cockpit texture (used with Bloom to tone down the effect inside the cockpit)
	bool is_CockpitTex;
	// True if this texture is a gunner texture
	bool is_GunnerTex;
	// True if this texture belongs to an Exterior OPT (the player's craft when external camera is on)
	bool is_Exterior;
	// True when displaying the hyperspace animation
	bool is_HyperspaceAnim;
	// True for dat,1000,2, (used for the hyperspace streak effect)
	bool is_FlatLightEffect;
	// True for lens flare effects
	bool is_LensFlare;
	// True for suns in the backdrop dat files
	bool is_Sun;
	// True for 3D suns, as in DTM's space missions
	bool is_3DSun;
	// True when this texture has been tested against an XWA light
	//int AssociatedXWALight;
	int AuxVectorIndex;
	// True for space debris (used to inhibit them for SSAO)
	bool is_Debris;
	// True for warhead trails (used to inhibit SSAO)
	bool is_Trail;
	// True for sparks (to control Bloom)
	bool is_Spark;
	// True for cockpit sparks (to control Bloom)
	bool is_CockpitSpark;
	// True for the chaff dat
	bool is_Chaff;
	// True for all sorts of missiles. Used for Bloom and SSAO
	bool is_Missile;
	// True for all textures that should not render SSAO (sets SSAO mask to 1)
	bool is_GenericSSAOMasked;
	// True for all skydome textures in DTM's plantery maps
	bool is_Skydome;
	// True for all skydome lights in DTM's planetary maps
	bool is_SkydomeLight;
	// True for all *.dat files
	bool is_DAT;
	// True if this is a blast mark
	bool is_BlastMark;
	// True if this is a reactor-core explosion texture
	bool is_DS2_Reactor_Explosion;
	// True if this is the energy field surrounding the reactor core.
	//bool is_DS2_Energy_Field;
	// The following encodes whether this is a warning light or not (and which type)
	WarningLightEnum WarningLightType;
	// True if this is an Active Cockpit texture for VR
	int ActiveCockpitIdx;

	// **** DYNAMIC COCKPIT FLAGS **** //
	// Textures in the cockpit that can be replaced with new textures
	// Index into g_DCElements that holds the Dynamic Cockpit information for this texture
	int DCElementIndex;
	// True for all textures that can be used as destinations for the dynamic cockpit.
	bool is_DynCockpitDst;
	// True for the light textures loaded in Hi-Res mode that provide additional glow
	bool is_DynCockpitAlphaOverlay;
	// True for all the source HUD textures (HUD Regions)
	bool is_DC_HUDRegionSrc;
	// True for specific DC HUD source textures
	bool is_DC_TargetCompSrc;
	bool is_DC_LeftSensorSrc;
	bool is_DC_RightSensorSrc;
	bool is_DC_ShieldsSrc;
	bool is_DC_SolidMsgSrc;
	bool is_DC_BorderMsgSrc;
	bool is_DC_LaserBoxSrc;
	bool is_DC_IonBoxSrc;
	bool is_DC_BeamBoxSrc;
	bool is_DC_TopLeftSrc;
	bool is_DC_TopRightSrc;

	// **** Materials ****
	bool bHasMaterial;
	Material material;

	// **** Greebles
	int GreebleTexIdx;

	// **** Normal Mapping
	int NormalMapIdx;
		
	Direct3DTexture(DeviceResources* deviceResources, TextureSurface* surface);

	int GetWidth();
	int GetHeight();
	bool LoadShadowOBJ(char* sFileName);
	void TagTexture();

	virtual ~Direct3DTexture();

	/*** IUnknown methods ***/
	
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj);
	
	STDMETHOD_(ULONG, AddRef)(THIS);
	
	STDMETHOD_(ULONG, Release)(THIS);

	/*** IDirect3DTexture methods ***/
	
	STDMETHOD(Initialize)(THIS_ LPDIRECT3DDEVICE, LPDIRECTDRAWSURFACE);
	
	STDMETHOD(GetHandle)(THIS_ LPDIRECT3DDEVICE, LPD3DTEXTUREHANDLE);
	
	STDMETHOD(PaletteChanged)(THIS_ DWORD, DWORD);

	std::string GetDATImageHash(char *sDATZIPFileName, int GroupId, int ImageId);
	int GetCachedSRV(char *sDATZIPFileName, int GroupId, int ImageId, ID3D11ShaderResourceView **srv);
	int AddCachedSRV(char *sDATZIPFileName, int GroupId, int ImageId, int index, ID3D11ShaderResourceView *srv);

	void LoadAnimatedTextures(int ATCIndex);

	int LoadGreebleTexture(char *GreebleDATGroupIdImageId, short *Width=nullptr, short *Height=nullptr);

	int LoadNormalMap(char * DATZIPGroupIdImageId, short * Width, short * Height);

	HRESULT CreateSRVFromBuffer(uint8_t *Buffer, int BufferLength, int Width, int Height, ID3D11ShaderResourceView **srv);

	int LoadDATImage(char *sDATFileName, int GroupId, int ImageId, bool cache,
		ID3D11ShaderResourceView **srv, short *Width_out=nullptr, short *Height_out=nullptr);

	int LoadZIPImage(char *sZIPFileName, int GroupId, int ImageId, bool cache,
		ID3D11ShaderResourceView **srv, short *Width_out=nullptr, short *Height_out=nullptr);
	
	STDMETHOD(Load)(THIS_ LPDIRECT3DTEXTURE);
	
	STDMETHOD(Unload)(THIS);

	ULONG _refCount;

	DeviceResources* _deviceResources;

	TextureSurface* _surface;
	std::string _name;

	ComPtr<ID3D11ShaderResourceView> _textureView;
};
