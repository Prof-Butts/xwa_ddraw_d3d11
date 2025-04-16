#include "common.h"
#include "XwaTextureData.h"
#include "DeviceResources.h"
#include "effects.h"
#include "globals.h"
#include <WICTextureLoader.h>
#include <vector>

const char* TRIANGLE_PTR_RESNAME = "dat,13000,100,";
const char* TARGETING_COMP_RESNAME = "dat,12000,1100,";

// Maps DAT GroupId-ImageId to indices in resources->_extraTextures
std::map<std::string, int> DATImageMap;
// Maps image file names or DAT names (?) to indices in resources->_extraTextures
// Maybe we can merge DATImageMap and g_TextureMap now...
// DATImageMap used to map DAT GroupId-ImageId to ID3D11ShaderResourceView objects, but
// that kind of caused memory leaks.
std::map<std::string, int> g_TextureMap;

void ClearCachedSRVs()
{
	DATImageMap.clear();
}

void ClearGlobalTextureMap()
{
	g_TextureMap.clear();
}

void AddToGlobalTextureMap(std::string name, int index)
{
	if (index != -1)
		g_TextureMap[name] = index;
}

int QueryGlobalTextureMap(std::string name)
{
	const auto& it = g_TextureMap.find(name);
	if (it == g_TextureMap.end())
		return -1;
	return it->second;
}

int MakeKeyFromGroupIdImageId(int groupId, int imageId);

XwaTextureData::XwaTextureData()
{
	this->_width = 0;
	this->_height = 0;
	this->_deviceResources = nullptr;
	this->DATImageId = -1;
	this->DATGroupId = -1;
	this->is_Tagged = false;
	this->TagCount = 3;
	this->is_Reticle = false;
	this->is_ReticleCenter = false;
	this->is_HighlightedReticle = false;
	this->is_TrianglePointer = false;
	this->is_Text = false;
	this->is_Floating_GUI = false;
	this->is_LaserIonEnergy = false;
	this->is_GUI = false;
	this->is_TargetingComp = false;
	this->is_Laser = false;
	this->is_TurboLaser = false;
	this->is_LightTexture = false;
	this->is_Transparent = false;
	this->is_EngineGlow = false;
	this->is_Electricity = false;
	this->is_Explosion = false;
	this->is_Fire = false;
	this->is_CockpitTex = false;
	this->is_GunnerTex = false;
	this->is_Exterior = false;
	this->is_HyperspaceAnim = false;
	this->is_FlatLightEffect = false;
	this->is_LensFlare = false;
	this->is_Sun = false;
	this->is_3DSun = false;
	this->is_Debris = false;
	this->is_Trail = false;
	this->is_Spark = false;
	this->is_HitEffect = false;
	this->is_CockpitSpark = false;
	this->is_Chaff = false;
	this->is_Missile = false;
	this->is_GenericSSAOMasked = false;
	this->is_Skydome = false;
	this->is_SkydomeLight = false;
	this->is_DAT = false;
	this->is_BlastMark = false;
	this->is_DS2_Reactor_Explosion = false;
	this->is_MapIcon = false;
	this->is_StarfieldCap = false;
	this->is_Starfield = false;
	this->is_Backdrop = false;
	this->WarningLightType = NONE_WLIGHT;
	this->ActiveCockpitIdx = -1;
	this->AuxVectorIndex = -1;
	// Dynamic cockpit data
	this->DCElementIndex = -1;
	this->is_DynCockpitDst = false;
	this->is_DynCockpitAlphaOverlay = false;
	this->is_DC_HUDRegionSrc = false;
	this->is_DC_TargetCompSrc = false;
	this->is_DC_LeftSensorSrc = false;
	this->is_DC_RightSensorSrc = false;
	this->is_DC_ShieldsSrc = false;
	this->is_DC_SolidMsgSrc = false;
	this->is_DC_BorderMsgSrc = false;
	this->is_DC_LaserBoxSrc = false;
	this->is_DC_IonBoxSrc = false;
	this->is_DC_BeamBoxSrc = false;
	this->is_DC_TopLeftSrc = false;
	this->is_DC_TopRightSrc = false;

	this->bHasMaterial = false;
	// Create the default material for this texture
	this->material.Glossiness = DEFAULT_GLOSSINESS;
	this->material.Intensity = DEFAULT_SPEC_INT;
	this->material.Metallic = DEFAULT_METALLIC;

	this->GreebleTexIdx = -1;
	this->NormalMapIdx = -1;
}

void XwaTextureData::Clone(XwaTextureData* dst)
{
	if (!dst || dst == this)
	{
		return;
	}

	*dst = *this;
}

void XwaTextureData::TagTexture()
{
	const char* _cname = this->_name.c_str();

	// This flag is still used in the Direct3DDevice::Execute() path.
	this->is_Tagged = true;
	const bool bIsDAT = (stristr(_cname, "dat,") != NULL);

	// DEBUG: Remove later!
	//log_debug("[DBG] %s", surface->_cname);

	// Skip tagging lower-level mips on OPT textures
	if (!bIsDAT)
	{
		const int len = strlen(_cname);
		const int lastDigit = _cname[len - 1] - '0';
		//log_debug("[DBG] [DBG] %s::%d", surface->_cname, lastDigit);
		// OPTs with mipmaps will have multiple entries for the same texture
		// let's not bother tagging lower-level mips... This prevents loading
		// auxiliar textures (like normal maps) multiple times for lower mips
		if (lastDigit > 1)
			return;
	}

	{
		// Capture the textures
#ifdef DBG_VR

		{
			static int TexIndex = 0;
			wchar_t filename[300];
			swprintf_s(filename, 300, L"c:\\XWA-Tex-w-names-3\\img-%d.png", TexIndex);
			saveSurface(filename, (char*)textureData[0].pSysMem, surface->_width, surface->_height, bpp);

			char buf[300];
			sprintf_s(buf, 300, "c:\\XWA-Tex-w-names-3\\data-%d.txt", TexIndex);
			FILE* file;
			fopen_s(&file, buf, "wt");
			fprintf(file, "0x%x, size: %d, %d, name: '%s'\n", crc, surface->_width, surface->_height, surface->_name);
			fclose(file);

			TexIndex++;
		}
#endif

		if (strstr(_cname, TRIANGLE_PTR_RESNAME) != NULL)
			this->is_TrianglePointer = true;
		if (strstr(_cname, TARGETING_COMP_RESNAME) != NULL)
			this->is_TargetingComp = true;
		if (isInVector(_cname, Reticle_ResNames))
			this->is_Reticle = true; // Standard Reticle from Reticle_ResNames
		if (isInVector(_cname, Text_ResNames))
			this->is_Text = true;
		if (isInVector(_cname, ReticleCenter_ResNames)) {
			this->is_ReticleCenter = true; // Standard Reticle Center
			log_debug("[DBG] [RET] %s is a Regular Reticle Center", _cname);
		}
		if (isInVector(_cname, CustomReticleCenter_ResNames)) {
			this->is_ReticleCenter = true; // Custom Reticle Center
			log_debug("[DBG] [RET] %s is a Custom Reticle Center", _cname);
			// Stop tagging this texture
			this->is_Tagged = true;
			this->TagCount = 0;
		}

		/*
		 * Custom reticles are loaded in Reticle.cpp::LoadCustomReticle(). The algorithm that translates
		 * a custom reticle index into a HUD resname is in ReticleIndexToHUDSlot(). The algorithm was
		 * written before we had DATReader.dll, so maybe we can replace the algorithm for that library
		 * later. The custom reticle code puts the laserlock resname in g_sLaserLockedReticleResName, and
		 * we're using that here to tag textures.
		 */
		if (strstr(_cname, "dat,12000,600,") != NULL ||
			(g_sLaserLockedReticleResName[0] != 0 && strstr(_cname, g_sLaserLockedReticleResName) != NULL)
			)
		{
			log_debug("[DBG] [RET] %s is a LaserLock Reticle", _cname);
			this->is_HighlightedReticle = true;
		}

		// Reticle Warning Light textures
		this->WarningLightType = NONE_WLIGHT;
		if (strstr(_cname, "dat,12000,1500,") != NULL || strstr(_cname, "dat,12000,1900,") != NULL ||
			isInVector(_cname, CustomWLight_LL_ResNames))
		{
			log_debug("[DBG] [RET] %s is a LL Warning Light", _cname);
			this->WarningLightType = RETICLE_LEFT_WLIGHT;
		}
		if (strstr(_cname, "dat,12000,1600,") != NULL || strstr(_cname, "dat,12000,2000,") != NULL ||
			isInVector(_cname, CustomWLight_ML_ResNames))
		{
			log_debug("[DBG] [RET] %s is a ML Warning Light", _cname);
			this->WarningLightType = RETICLE_MID_LEFT_WLIGHT;
		}
		if (strstr(_cname, "dat,12000,1700,") != NULL || strstr(_cname, "dat,12000,2100,") != NULL ||
			isInVector(_cname, CustomWLight_MR_ResNames))
		{
			log_debug("[DBG] [RET] %s is a MR Warning Light", _cname);
			this->WarningLightType = RETICLE_MID_RIGHT_WLIGHT;
		}
		if (strstr(_cname, "dat,12000,1800,") != NULL || strstr(_cname, "dat,12000,2200,") != NULL ||
			isInVector(_cname, CustomWLight_RR_ResNames))
		{
			log_debug("[DBG] [RET] %s is a RR Warning Light", _cname);
			this->WarningLightType = RETICLE_RIGHT_WLIGHT;
		}

		if (isInVector(_cname, Floating_GUI_ResNames))
			this->is_Floating_GUI = true;
		if (isInVector(_cname, LaserIonEnergy_ResNames))
			this->is_LaserIonEnergy = true;
		if (isInVector(_cname, GUI_ResNames))
			this->is_GUI = true;

		// Catch the engine glow and mark it
		if (strstr(_cname, "dat,1000,1,") != NULL)
			this->is_EngineGlow = true;
		// Catch the flat light effect
		if (strstr(_cname, "dat,1000,2") != NULL)
			this->is_FlatLightEffect = true;
		// Catch the electricity textures and mark them
		if (isInVector(_cname, Electricity_ResNames))
			this->is_Electricity = true;
		// Catch the explosion textures and mark them
		if (isInVector(_cname, Explosion_ResNames))
			this->is_Explosion = true;
		if (isInVector(_cname, Fire_ResNames))
			this->is_Fire = true;
		// Catch the lens flare and mark it
		if (isInVector(_cname, LensFlare_ResNames))
			this->is_LensFlare = true;
		// Catch the hyperspace anim and mark it
		if (strstr(_cname, "dat,3051,") != NULL)
			this->is_HyperspaceAnim = true;
		// Catch the backdrup suns and mark them
		if (isInVector(_cname, Sun_ResNames)) {
			this->is_Sun = true;
			//g_AuxTextureVector.push_back(this); // We're no longer tracking which textures are Sun-textures
		}
		// Catch the space debris
		if (isInVector(_cname, SpaceDebris_ResNames))
			this->is_Debris = true;
		// Catch DAT files
		if (bIsDAT) {
			int GroupId, ImageId;
			this->is_DAT = true;
			// Check if this DAT image is a custom reticle
			if (GetGroupIdImageIdFromDATName(_cname, &GroupId, &ImageId)) {
				this->DATGroupId = GroupId;
				this->DATImageId = ImageId;
				if (GroupId == 12000 && ImageId > 5000) {
					this->is_Reticle = true; // Custom Reticle
					//log_debug("[DBG] CUSTOM RETICLE: %s", surface->_name);
					// This candidate may be a custom reticle center, let's tag it a few more times so that
					// we can see the current cockpit name and check this texture to see if it's a custom
					// reticle center
					if (this->TagCount > 0) {
						this->is_Tagged = false;
						this->TagCount--;
					}
					//else
					//	log_debug("[DBG] [RET] %s will not be tagged anymore", surface->_name);
				}
			}

			// Backdrops: planets, nebulae, etc.
			if ((GroupId >= 6010 && GroupId <= 6014) ||
				(GroupId >= 6020 && GroupId <= 6034) ||
				(GroupId >= 6040 && GroupId <= 6043) ||
				(GroupId >= 6060 && GroupId <= 6064) ||
				(GroupId >= 6070 && GroupId <= 6084) ||
				(GroupId >= 6090 && GroupId <= 6094) ||
				(GroupId >= 6100 && GroupId <= 6104) ||
				(GroupId >= 6110 && GroupId <= 6114) ||
				(GroupId >= 6304 && GroupId <= 6311)) // New in XWAU 2024
			{
				this->is_Backdrop = true;
				log_debug("[DBG] [CUBE] Backdrop: %s", _cname);
				g_GroupIdImageIdToTextureMap[MakeKeyFromGroupIdImageId(GroupId, ImageId)] = this;
			}

			// Starfields
			if ((GroupId == 6104 && ImageId == 0) ||

				(GroupId == 6079 && ImageId == 2) ||
				(GroupId == 6079 && ImageId == 3) ||
				(GroupId == 6079 && ImageId == 4) ||
				(GroupId == 6079 && ImageId == 5) ||
				(GroupId == 6079 && ImageId == 6) ||

				(GroupId == 6034 && ImageId == 3) ||
				(GroupId == 6034 && ImageId == 4) ||
				(GroupId == 6034 && ImageId == 5) ||
				(GroupId == 6034 && ImageId == 6) ||

				(GroupId == 6042 && ImageId == 1) ||
				(GroupId == 6042 && ImageId == 2) ||
				(GroupId == 6042 && ImageId == 3) ||
				(GroupId == 6042 && ImageId == 4) ||
				(GroupId == 6042 && ImageId == 5) ||
				(GroupId == 6042 && ImageId == 6) ||

				(GroupId == 6094 && ImageId == 1) ||
				(GroupId == 6094 && ImageId == 3) ||
				(GroupId == 6094 && ImageId == 5) ||

				(GroupId == 6083 && ImageId == 2) ||
				(GroupId == 6083 && ImageId == 5) ||

				(GroupId == 6084 && ImageId == 1) ||
				(GroupId == 6084 && ImageId == 4))
			{
				this->is_Backdrop = false;
				this->is_StarfieldCap = false;
				this->is_Starfield = true;
				log_debug("[DBG] [CUBE] Starfield: %s", _cname);
				g_GroupIdImageIdToTextureMap[MakeKeyFromGroupIdImageId(GroupId, ImageId)] = this;
			}

			// Starfield Caps
			if ((GroupId == 6083 && ImageId == 3) ||

				(GroupId == 6084 && ImageId == 2) ||
				(GroupId == 6084 && ImageId == 6) ||

				(GroupId == 6094 && ImageId == 2) ||
				(GroupId == 6094 && ImageId == 4) ||
				(GroupId == 6094 && ImageId == 6) ||

				(GroupId == 6104 && ImageId == 5))
			{
				this->is_Backdrop = false;
				this->is_Starfield = false;
				this->is_StarfieldCap = true;
				log_debug("[DBG] [CUBE] Starfield cap: %s", _cname);
				g_GroupIdImageIdToTextureMap[MakeKeyFromGroupIdImageId(GroupId, ImageId)] = this;
			}
		}
		// Catch blast marks
		if (strstr(_cname, "dat,3050,") != NULL)
			this->is_BlastMark = true;
		// Catch the trails
		if (isInVector(_cname, Trails_ResNames))
			this->is_Trail = true;
		// Catch the sparks
		if (isInVector(_cname, Sparks_ResNames))
			this->is_Spark = true;
		if (isInVector(_cname, HitEffects_ResNames))
			this->is_HitEffect = true;
		if (strstr(_cname, "dat,22007,") != NULL)
			this->is_CockpitSpark = true;
		if ((strstr(_cname, "dat,5000,") != NULL) ||
			(strstr(_cname, "dat,3006,") != NULL))
			this->is_Chaff = true;
		if (strstr(_cname, "dat,17002,") != NULL) // DSII reactor core explosion animation textures
			this->is_DS2_Reactor_Explosion = true;
		//if (strstr(surface->_name, "dat,17001,") != NULL) // DSII reactor core explosion animation textures
		//	this->is_DS2_Energy_Field = true;
		if ((strstr(_cname, "dat,14800,") != NULL) || // Gray map icons
			(strstr(_cname, "dat,14610,") != NULL) || // Green map icons
			(strstr(_cname, "dat,14620,") != NULL) || // Red map icons
			(strstr(_cname, "dat,14630,") != NULL) || // Blue map icons
			(strstr(_cname, "dat,14640,") != NULL) || // Yellow map icons
			(strstr(_cname, "dat,14650,") != NULL))   // Purple map icons
		{
			this->is_MapIcon = true;
		}

		/* Special handling for Dynamic Cockpit source HUD textures */
		if (g_bReshadeEnabled) {
			if (stristr(_cname, DC_TARGET_COMP_SRC_RESNAME) != NULL) {
				this->is_DC_TargetCompSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if ((stristr(_cname, DC_LEFT_SENSOR_SRC_RESNAME) != NULL) ||
				(stristr(_cname, DC_LEFT_SENSOR_2_SRC_RESNAME) != NULL)) {
				this->is_DC_LeftSensorSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if ((stristr(_cname, DC_RIGHT_SENSOR_SRC_RESNAME) != NULL) ||
				(stristr(_cname, DC_RIGHT_SENSOR_2_SRC_RESNAME) != NULL)) {
				this->is_DC_RightSensorSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(_cname, DC_SHIELDS_SRC_RESNAME) != NULL) {
				this->is_DC_ShieldsSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(_cname, DC_SOLID_MSG_SRC_RESNAME) != NULL) {
				this->is_DC_SolidMsgSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(_cname, DC_BORDER_MSG_SRC_RESNAME) != NULL) {
				this->is_DC_BorderMsgSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(_cname, DC_LASER_BOX_SRC_RESNAME) != NULL) {
				this->is_DC_LaserBoxSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(_cname, DC_ION_BOX_SRC_RESNAME) != NULL) {
				this->is_DC_IonBoxSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(_cname, DC_BEAM_BOX_SRC_RESNAME) != NULL) {
				this->is_DC_BeamBoxSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(_cname, DC_TOP_LEFT_SRC_RESNAME) != NULL) {
				this->is_DC_TopLeftSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(_cname, DC_TOP_RIGHT_SRC_RESNAME) != NULL) {
				this->is_DC_TopRightSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
		}
	}

	// Load the relevant MAT file for the current OPT if necessary
	OPTNameType OPTname;
	OPTname.name[0] = 0;
	{
		// Capture the OPT name
		// Sample surface->_cname's:
		// opt,FlightModels\PreybirdFighterExterior.opt,TEX00001,color,0
		// opt,FlightModels\PreybirdFighterExterior.opt,TEX00001,light,0
		// opt,FlightModels\PreybirdFighterExterior.opt,TEX00005,color-transparent,0
		// In the following example, the "_fg_" part means a skin is active:
		// opt,FlightModels\SlaveOneExterior.opt,Tex00000_fg_0_Default,color,0
		// opt,FlightModels\SlaveOneExterior.opt,Tex00000_fg_1_Default,color,0
		const char* start = strstr(_cname, "\\");
		const char* end = strstr(_cname, ".opt");
		char sFileName[180], sFileNameShort[180];
		if (start != NULL && end != NULL) {
			start += 1; // Skip the backslash
			int size = end - start;
			strncpy_s(OPTname.name, MAX_OPT_NAME, start, size);
			if (!isInVector(OPTname.name, g_OPTnames)) {
				//log_debug("[DBG] [MAT] OPT Name Captured: '%s'", OPTname.name);
				// Add the name to the list of OPTnames so that we don't try to process it again
				g_OPTnames.push_back(OPTname);
				OPTNameToMATParamsFile(OPTname.name, sFileName, 180);
				//log_debug("[DBG] [MAT] Loading file %s...", sFileName);
				LoadIndividualMATParams(OPTname.name, sFileName); // OPT material
			}
		}
		else if (bIsDAT) {
			// For DAT images, OPTname.name is the full DAT name:
			strncpy_s(OPTname.name, MAX_OPT_NAME, _cname, strlen(_cname));
			DATNameToMATParamsFile(OPTname.name, sFileName, sFileNameShort, 180);
			if (sFileName[0] != 0) {
				// Load the regular DAT material:
				if (!LoadIndividualMATParams(OPTname.name, sFileName)) {
					// If the regular DAT material failed, try loading the mat file for DAT animations:
					if (sFileNameShort[0] != 0)
						// For some reason, each frame of the explosion animations is loaded multiple times.
						// There doesn't seem to be any bad effects for this behavior, it's only a bit spammy.
						// So, here we're just muting the debug messages to prevent the spam:
						LoadIndividualMATParams(OPTname.name, sFileNameShort, false);
				}
			}
		}
	}

	// Tag Lasers, Missiles, Cockpit textures, Exterior textures, light textures
	{
		//log_debug("[DBG] [DC] name: [%s]", surface->_name);
		// Catch the laser-related textures and mark them
		if (strstr(_cname, "Laser") != NULL) {
			// Ignore "LaserBat.OPT"
			if (strstr(_cname, "LaserBat") == NULL) {
				this->is_Laser = true;
			}
		}

		// Tag all the missiles
		// Flare
		if (strstr(_cname, "Missile") != NULL || strstr(_cname, "Torpedo") != NULL ||
			strstr(_cname, "SpaceBomb") != NULL || strstr(_cname, "Pulse") != NULL ||
			strstr(_cname, "Rocket") != NULL || strstr(_cname, "Flare") != NULL) {
			if (strstr(_cname, "Boat") == NULL) {
				//log_debug("[DBG] Missile texture: %s", _cname);
				this->is_Missile = true;
			}
		}

		if (strstr(_cname, "Turbo") != NULL) {
			this->is_TurboLaser = true;
		}

		if (stristr(_cname, "Cockpit.opt") != NULL) {
			this->is_CockpitTex = true;

			// DEBUG
			// Looks like we always tag the color texture before the light texture.
			// Then we Load() the color and light textures.
			//if (strstr(surface->_name, "TEX00061") != NULL)
			//log_debug("[DBG] [AC] Tagging: %s", surface->_name);
			// DEBUG

			/*
			   Here's a funny story: you can change the craft when in the hangar. So we need to pay attention
			   to changes in the cockpit's name. One way to do this is by resetting DC when textures are freed;
			   but doing that causes problems. When hook_time.dll is used, the textures are reloaded after they
			   are freed; but with Hook_60FPS.dll, textures are not reloaded. So, in the latter case, DC is
			   disabled when exiting the hangar, because for some reason *that's* when textures are released.
			   So, the fix is to *always* pay attention to cockpit name changes and reset DC when we detect a
			   change.
			*/
			// Capture and store the name of the cockpit the very first time we see a cockpit texture
			if (this->is_CockpitTex) {
				char CockpitName[128];
				//strstr(surface->_name, "Gunner")  != NULL)  {
				// Extract the name of the cockpit and put it in CockpitName
				const char* start = strstr(_cname, "\\");
				const char* end = strstr(_cname, ".opt");
				if (start != NULL && end != NULL) {
					start += 1; // Skip the backslash
					int size = end - start;
					strncpy_s(CockpitName, 128, start, size);
					//log_debug("[DBG] Cockpit Name Captured: '%s'", CockpitName);
				}

				bool bCockpitNameChanged = !(strcmp(g_sCurrentCockpit, CockpitName) == 0);
				if (bCockpitNameChanged)
				{
					// Force the recomputation of y_center for the next cockpit
					g_bYCenterHasBeenFixed = false;
					// The cockpit name has changed, update it and reset DC
					_deviceResources->ResetDynamicCockpit(); // Resetting DC will erase the cockpit name
					strncpy_s(g_sCurrentCockpit, 128, CockpitName, 128);
					log_debug("[DBG] Cockpit Name Changed/Captured: '%s'", g_sCurrentCockpit);
					// Load the relevant DC file for the current cockpit if necessary
					char sFileName[80];
					CockpitNameToDCParamsFile(g_sCurrentCockpit, sFileName, 80);
					if (!LoadIndividualDCParams(sFileName))
						log_debug("[DBG] [DC] WARNING: Could not load DC params");

					// Load the relevant AC file for the current cockpit if necessary
					if (g_bActiveCockpitEnabled) {
						char sFileName[80];
						CockpitNameToACParamsFile(g_sCurrentCockpit, sFileName, 80);
						if (!LoadIndividualACParams(sFileName))
							log_debug("[DBG] [AC] WARNING: Could not load AC params");
					}
					// Load the cockpit Shadow geometry
					if (g_ShadowMapping.bEnabled) {
						char sFileName[80];
						snprintf(sFileName, 80, "./ShadowMapping/%s.obj", g_sCurrentCockpit);
						log_debug("[DBG] [SHW] Loading file: %s", sFileName);
						// Disable shadow mapping, if we can load a Shadow Map OBJ, then it will be re-enabled again.
						g_ShadowMapping.bUseShadowOBJ = false;
						LoadShadowOBJ(sFileName);
					}
					// Load the reticle definition file
					{
						LoadCustomReticle(g_sCurrentCockpit);
					}
					// Load the POV Offset from the current INI file
					{
						LoadPOVOffsetFromIniFile();
					}
					// Load the Holo Offset from the current INI file
					{
						LoadHoloOffsetFromIniFile();
					}
					// Load the HUD color from the current INI file
					{
						LoadHUDColorFromIniFile();
					}
					// We need to count the number of lasers and ions in this new craft, but we probably
					// can't do it now since we're still loading resources here. Instead, let's set a flag
					// to count them later.
					g_bLasersIonsNeedCounting = true;

					// Disable the Gimbal Lock Fix on the following ships:
					g_bYTSeriesShip = false;
					if (stristr(g_sCurrentCockpit, "CorellianTransport2") != NULL) {
						g_bYTSeriesShip = true;
					}
					else if (stristr(g_sCurrentCockpit, "FamilyTransport") != NULL) {
						g_bYTSeriesShip = true;
					}
					else if (stristr(g_sCurrentCockpit, "Outrider") != NULL) {
						g_bYTSeriesShip = true;
					}
					else if (stristr(g_sCurrentCockpit, "MilleniumFalcon") != NULL) {
						g_bYTSeriesShip = true;
					}
					else if (stristr(g_sCurrentCockpit, "MiniFalcon") != NULL) {
						g_bYTSeriesShip = true;
					}
				}

			}

		}

		if (strstr(_cname, "Gunner") != NULL) {
			this->is_GunnerTex = true;
		}

		if (strstr(_cname, "Exterior") != NULL) {
			this->is_Exterior = true;
		}

		// Catch light textures and mark them appropriately
		if (strstr(_cname, ",light,") != NULL)
			this->is_LightTexture = true;

		// Tag textures with transparency
		if (strstr(_cname, ",color-transparent,") != NULL)
			this->is_Transparent = true;

		//if (strstr(surface->_name, "color-transparent") != NULL) {
			//this->is_ColorTransparent = true;
			//log_debug("[DBG] [DC] ColorTransp: [%s]", surface->_name);
		//}

		/*
			Naming conventions from DTM:

			Surface_XXX_xxKm.opt = Planetary Surface
			Building_XXX.opt = Planetary Building
			Skydome_XXX_xxKm.opt = Planetary Sky
			Planet3D_XXX_xxKm.opt = Planet OPT object
			Star3D_XXX_ccKm.opt = Sun OPT object
			DeathStar3D_XXX.opt = Part of the real size Death Star (in progress)
		*/

		// Disable SSAO/SSDO for all Skydomes (This fix is specific for DTM's maps)
		if (strstr(_cname, "Cielo") != NULL ||
			strstr(_cname, "Skydome") != NULL)
		{
			g_b3DSkydomePresent = true;
			this->is_Skydome = true;
			this->is_GenericSSAOMasked = true;
			if (this->is_LightTexture) {
				this->is_SkydomeLight = true;
				this->is_LightTexture = false; // The is_SkydomeLight attribute overrides this attribute
			}
		}

		// 3D Sun is present in this scene: [opt, FlightModels\Planet3D_Sole_26Km.opt, TEX00001, color, 0]
		if (strstr(_cname, "Sole") != NULL ||
			strstr(_cname, "Star3D") != NULL) {
			g_b3DSunPresent = true;
			this->is_3DSun = true;
			//log_debug("[DBG] 3D Sun is present in this scene: [%s]", surface->_name);
		}

		{
			/* Process Dynamic Cockpit destination textures: */
			int idx = isInVector(_cname, g_DCElements, g_iNumDCElements);
			if (idx > -1) {
				// "light" and "color" textures are processed differently
				if (strstr(_cname, ",color") != NULL) {
					// This texture is a Dynamic Cockpit destination texture
					this->is_DynCockpitDst = true;
					// Make this texture "point back" to the right dc_element
					this->DCElementIndex = idx;
					// Activate this dc_element
					g_DCElements[idx].bActive = true;
					// Load the cover texture if necessary
					if (_deviceResources->dc_coverTexture[idx] == nullptr &&
						g_DCElements[idx].coverTextureName[0] != 0)
					{
						HRESULT res = S_OK;
						char* substr_dat = stristr(g_DCElements[idx].coverTextureName, ".dat");
						char* substr_zip = stristr(g_DCElements[idx].coverTextureName, ".zip");
						if (substr_dat != NULL || substr_zip != NULL) {
							// This is a DAT or a ZIP texture, load it through the DATReader or ZIPReader libraries
							char sDATZIPFileName[128];
							short GroupId, ImageId;
							bool bIsDATFile = false;
							res = E_FAIL;
							if (ParseDatZipFileNameGroupIdImageId(g_DCElements[idx].coverTextureName, sDATZIPFileName, 128, &GroupId, &ImageId)) {
								//log_debug("[DBG] [DC] Loading cover texture [%s]-%d-%d", sDATFileName, GroupId, ImageId);
								if (substr_dat != NULL)
									res = LoadDATImage(sDATZIPFileName, GroupId, ImageId, false, &(_deviceResources->dc_coverTexture[idx]));
								else
									res = LoadZIPImage(sDATZIPFileName, GroupId, ImageId, false, &(_deviceResources->dc_coverTexture[idx]));
								//if (FAILED(res)) log_debug("[DBG] [DC] *** ERROR loading cover texture");
							}
						}
						else {
							// This is a regular texture, load it as usual
							wchar_t wTexName[MAX_TEXTURE_NAME];
							size_t len = 0;
							mbstowcs_s(&len, wTexName, MAX_TEXTURE_NAME, g_DCElements[idx].coverTextureName, MAX_TEXTURE_NAME);
							res = DirectX::CreateWICTextureFromFile(_deviceResources->_d3dDevice, wTexName, NULL,
								&(_deviceResources->dc_coverTexture[idx]));
						}
						if (FAILED(res)) {
							//log_debug("[DBG] [DC] ***** Could not load cover texture [%s]: 0x%x",
							//	g_DCElements[idx].coverTextureName, res);
							_deviceResources->dc_coverTexture[idx] = nullptr;
						}
					}
				}
				else if (strstr(_cname, ",light") != NULL) {
					this->is_DynCockpitAlphaOverlay = true;
				}
			} // if (idx > -1)
		}

		if (g_bActiveCockpitEnabled) {
			if (this->is_CockpitTex && !this->is_LightTexture)
			{
				/* Process Active Cockpit destination textures: */
				int idx = isInVector(_cname, g_ACElements, g_iNumACElements);
				if (idx > -1) {
					// "Point back" into the right ac_element index:
					this->ActiveCockpitIdx = idx;
					g_ACElements[idx].bActive = true;
					//log_debug("[DBG] [AC] %s is an AC Texture, ActiveCockpitIdx: %d", surface->_name, this->ActiveCockpitIdx);
				}
			}
		}

		// Materials
		if (OPTname.name[0] != 0)
		{
			int craftIdx = FindCraftMaterial(OPTname.name);
			if (craftIdx > -1) {
				char texname[MAX_TEXNAME];
				// We need to check if this is a DAT or an OPT first
				if (bIsDAT)
				{
					texname[0] = 0; // Retrieve the default material
				}
				else
				{
					//log_debug("[DBG] [MAT] Craft Material %s found", OPTname.name);
					const char* start = strstr(_cname, ".opt");
					// Skip the ".opt," part
					start += 5;
					// Find the next comma
					const char* end = strstr(start, ",");
					int size = end - start;
					strncpy_s(texname, MAX_TEXNAME, start, size);
				}
				//log_debug("[DBG] [MAT] Looking for material for %s", texname);
				this->material = FindMaterial(craftIdx, texname);
				this->bHasMaterial = true;
				// This texture has a material associated with it, let's save it in the aux list:
				g_AuxTextureVector.push_back(this);
				this->AuxVectorIndex = g_AuxTextureVector.size() - 1;
				// If this material has an animated light map, let's load the textures now
				if (!this->material.bInstanceMaterial) {
					// Load GLOBAL event animations
					if ((this->material.AnyLightMapATCIndex() && this->is_LightTexture) ||
						(this->material.AnyTextureATCIndex() && !this->is_LightTexture))
					{
						// Load the animated textures for each valid index
						int ATCType = this->is_LightTexture ? LIGHTMAP_ATC_IDX : TEXTURE_ATC_IDX;
						// Go over each valid TextureATCIndex and load their associated animations
						for (int i = 0; i < MAX_GAME_EVT; i++)
							if (this->material.TextureATCIndices[ATCType][i] > -1)
							{
								const int ATCIndex = this->material.TextureATCIndices[ATCType][i];
								if (!g_AnimatedMaterials[ATCIndex].SequenceLoaded)
									LoadAnimatedTextures(ATCIndex);
							}
					}
				}
				else {
					// Load INSTANCE event animations
					if ((this->material.AnyInstLightMapATCIndex() && this->is_LightTexture) ||
						(this->material.AnyInstTextureATCIndex() && !this->is_LightTexture))
					{
						// Load the animated textures for each valid index
						int ATCType = this->is_LightTexture ? LIGHTMAP_ATC_IDX : TEXTURE_ATC_IDX;
						// Go over each valid InstTextureATCIndices and load their associated animations
						for (int i = 0; i < MAX_INST_EVT; i++)
						{
							int size = this->material.InstTextureATCIndices[ATCType][i].size();
							for (int j = 0; j < size; j++)
							{
								const int ATCIndex = this->material.InstTextureATCIndices[ATCType][i][j];
								if (!g_AnimatedMaterials[ATCIndex].SequenceLoaded)
									LoadAnimatedTextures(ATCIndex);
							}
						}
					}
				}

				// Load the Greeble Textures here...
				if (this->material.GreebleDataIdx != -1) {
					GreebleData* greeble_data = &(g_GreebleData[this->material.GreebleDataIdx]);
					short Width, Height;
					//log_debug("[DBG] [GRB] This material has GreebleData at index: %d", this->material.GreebleDataIdx);
					//log_debug("[DBG] [GRB] GreebleTexIndex[0]: %d, GreebleTexIndex[1]: %d",
					//	greeble_data->GreebleTexIndex[0], greeble_data->GreebleTexIndex[1]);
					// Load the Greeble Textures
					for (int i = 0; i < MAX_GREEBLE_LEVELS; i++) {
						// We need to load this texture. Textures are loaded in resources->_extraTextures
						if (greeble_data->GreebleTexIndex[i] == -1 && greeble_data->GreebleTexName[i][0] != 0) {
							// TODO: Optimization opportunity: search all the texture names in g_GreebleData and avoid loading
							// textures if we find we've already loaded them before...
							greeble_data->GreebleTexIndex[i] = LoadGreebleTexture(greeble_data->GreebleTexName[i], &Width, &Height);
							if (greeble_data->GreebleTexIndex[i] != -1) {
								if (greeble_data->greebleBlendMode[i] == GBM_UV_DISP ||
									greeble_data->greebleBlendMode[i] == GBM_UV_DISP_AND_NORMAL_MAP) {
									greeble_data->UVDispMapResolution.x = Width;
									greeble_data->UVDispMapResolution.y = Height;
									//log_debug("[DBG] [GRB] UVDispMapResolution: %0.0f, %0.0f",
									//	greeble_data->UVDispMapResolution.x, greeble_data->UVDispMapResolution.y);
								}
								//log_debug("[DBG] [GRB] Loaded Greeble Texture at index: %d", greeble_data->GreebleTexIndex[i]);
							}
						}
						// Load Lightmap Greebles
						if (greeble_data->GreebleLightMapIndex[i] == -1 && greeble_data->GreebleLightMapName[i][0] != 0) {
							// TODO: Optimization opportunity: search all the lightmap texture names in g_GreebleData and avoid loading
							// textures if we find we've already loaded them before...
							greeble_data->GreebleLightMapIndex[i] = LoadGreebleTexture(greeble_data->GreebleLightMapName[i], &Width, &Height);
							if (greeble_data->GreebleLightMapIndex[i] != -1) {
								if (greeble_data->greebleLightMapBlendMode[i] == GBM_UV_DISP ||
									greeble_data->greebleLightMapBlendMode[i] == GBM_UV_DISP_AND_NORMAL_MAP) {
									// TODO: Only one (1) UVDispMap is currently supported for regular and lightmap greebles.
									// I may need to add support for more maps later.
									greeble_data->UVDispMapResolution.x = Width;
									greeble_data->UVDispMapResolution.y = Height;
									//log_debug("[DBG] [GRB] UVDispMapResolution (Lightmap): %0.0f, %0.0f",
									//	greeble_data->UVDispMapResolution.x, greeble_data->UVDispMapResolution.y);
								}
								//log_debug("[DBG] [GRB] Loaded Lightmap Greeble Texture at index: %d", greeble_data->GreebleLightMapIndex[i]);
							}
						}
					}

				}

				// Load the alternate explosions here...
				for (int AltExpIdx = 0; AltExpIdx < MAX_ALT_EXPLOSIONS; AltExpIdx++) {
					int ATCIndex = this->material.AltExplosionIdx[AltExpIdx];
					if (ATCIndex != -1) {
						AnimatedTexControl* atc = &(g_AnimatedMaterials[ATCIndex]);
						// Make sure we only load this sequence once
						if (!atc->SequenceLoaded) {
							//log_debug("[DBG] Loading AltExplosionIdx[%d]: %d", AltExpIdx, ATCIndex);
							for (uint32_t j = 0; j < atc->Sequence.size(); j++) {
								TexSeqElem tex_seq_elem = atc->Sequence[j];
								ID3D11ShaderResourceView* texSRV = nullptr;
								if (atc->Sequence[j].ExtraTextureIndex == -1) {
									int index = -1;

									if (tex_seq_elem.IsDATImage)
										index = LoadDATImage(tex_seq_elem.texname, tex_seq_elem.GroupId,
											/* ImageId */ j, true, &texSRV);
									else if (tex_seq_elem.IsZIPImage)
										index = LoadZIPImage(tex_seq_elem.texname, tex_seq_elem.GroupId,
											/* ImageId */ j, true, &texSRV);

									if (index == -1) {
										log_debug("[DBG] [MAT] ***** Could not load DAT texture [%s-%d-%d]",
											tex_seq_elem.texname, tex_seq_elem.GroupId, j);
									}
									else {
										atc->Sequence[j].ExtraTextureIndex = index;
									}
								}
							}
							atc->SequenceLoaded = true; // Make extra-sure we only load this animation once
						}
					}
				}

				// Load the Normal Maps here...
				if (this->material.NormalMapName[0] != 0)
				{
					const std::string normalMapName = std::string(this->material.NormalMapName);
					this->NormalMapIdx = QueryGlobalTextureMap(normalMapName);
					if (this->NormalMapIdx == -1)
					{
						short Width, Height; // These variables are not used
						this->NormalMapIdx = LoadNormalMap(this->material.NormalMapName, &Width, &Height);
						AddToGlobalTextureMap(normalMapName, this->NormalMapIdx);
						log_debug("[DBG] [MAT] LoadNormal [%s], res: %d, loaded: %d",
							this->_name.c_str(),
							this->NormalMapIdx, this->material.NormalMapLoaded);
					}
					this->material.NormalMapLoaded = (this->NormalMapIdx != -1);
				}

				// DEBUG
				/*if (bIsDat) {
					log_debug("[DBG] [MAT] [%s] --> Material: %0.3f, %0.3f, %0.3f",
						surface->_name, material.Light.x, material.Light.y, material.Light.z);
				}*/
				// DEBUG
			}
			//else {
				// Material not found, use the default material (already created in the constructor)...
				//log_debug("[DBG] [MAT] Material %s not found", OPTname.name);
			//}
		}
	}
}

bool XwaTextureData::LoadShadowOBJ(const char* sFileName)
{
	FILE* file;
	int error = 0;

	try {
		error = fopen_s(&file, sFileName, "rt");
	}
	catch (...) {
		log_debug("[DBG] [SHW] Could not load file %s", sFileName);
	}

	if (error != 0) {
		log_debug("[DBG] [SHW] Error %d when loading %s", error, sFileName);
		return false;
	}

	std::vector<D3DTLVERTEX> vertices;
	std::vector<WORD> indices;
	float minx = 100000.0f, miny = 100000.0f, minz = 100000.0f;
	float maxx = -100000.0f, maxy = -100000.0f, maxz = -100000.0f;
	float rangex, rangey, rangez;

	char line[256];
	while (!feof(file)) {
		fgets(line, 256, file);
		// fgets may fail because EOF has been reached, so we need to check
		// again here.
		if (feof(file))
			break;

		if (line[0] == 'v') {
			D3DTLVERTEX v;
			float x, y, z;
			// With the D3dRendererHook, we're going to use transform matrices that work on OPT
			// coordinates, so we need to convert OBJ coords into OPT coords. To do that, we need
			// to swap Y and Z, because that's what the OPT coordinate system uses, and we need
			// to multiply by 40.96 for the same reason. In the line below, we read X, Z, Y to
			// swap the axes: THIS IS NOT A MISTAKE!
			sscanf_s(line, "v %f %f %f", &x, &z, &y);
			// And now we add the 40.96 scale factor.
			// g_ShadowMapping.shadow_map_mult_x/y/z are supposed to be either 1 or -1
			x *= g_ShadowMapping.shadow_map_mult_x * METERS_TO_OPT;
			y *= g_ShadowMapping.shadow_map_mult_y * METERS_TO_OPT;
			z *= g_ShadowMapping.shadow_map_mult_z * METERS_TO_OPT;

			v.sx = x;
			v.sy = y;
			v.sz = z;

			vertices.push_back(v);
			if (x < minx) minx = x;
			if (y < miny) miny = y;
			if (z < minz) minz = z;

			if (x > maxx) maxx = x;
			if (y > maxy) maxy = y;
			if (z > maxz) maxz = z;
		}
		else if (line[0] == 'f') {
			int i, j, k;
			sscanf_s(line, "f %d %d %d", &i, &j, &k);
			indices.push_back((WORD)(i - 1));
			indices.push_back((WORD)(j - 1));
			indices.push_back((WORD)(k - 1));
		}
	}
	fclose(file);

	rangex = maxx - minx;
	rangey = maxy - miny;
	rangez = maxz - minz;
	//g_ShadowMapVSCBuffer.OBJrange = max(max(rangex, rangey), rangez);

	log_debug("[DBG] [SHW] Loaded %d vertices, %d faces.",
		vertices.size(), indices.size() / 3);
	log_debug("[DBG] [SHW] min,max: [%0.3f, %0.3f], [%0.3f, %0.3f], [%0.3f, %0.3f]",
		minx, maxx, miny, maxy, minz, maxz);
	log_debug("[DBG] [SHW] range-x,y,z: %0.3f, %0.3f, %0.3f",
		rangex, rangey, rangez);

	g_ShadowMapping.bUseShadowOBJ = true;
	log_debug("[DBG] [SHW] Shadow Map OBJ loaded, enabling shadow mapping");
	_deviceResources->CreateShadowVertexIndexBuffers(vertices.data(), indices.data(), vertices.size(), indices.size());
	vertices.clear();
	indices.clear();

	g_OBJLimits.clear();
	// Build a box with the limits. This box can be transformed later to find the 2D limits
	// of the shadow map
	g_OBJLimits.push_back(Vector4(minx, miny, minz, 1.0f));
	g_OBJLimits.push_back(Vector4(maxx, miny, minz, 1.0f));
	g_OBJLimits.push_back(Vector4(maxx, maxy, minz, 1.0f));
	g_OBJLimits.push_back(Vector4(minx, maxy, minz, 1.0f));

	g_OBJLimits.push_back(Vector4(minx, miny, maxz, 1.0f));
	g_OBJLimits.push_back(Vector4(maxx, miny, maxz, 1.0f));
	g_OBJLimits.push_back(Vector4(maxx, maxy, maxz, 1.0f));
	g_OBJLimits.push_back(Vector4(minx, maxy, maxz, 1.0f));
	return true;
}

int XwaTextureData::LoadDATImage(const char* sDATFileName, int GroupId, int ImageId, bool cache,
	ID3D11ShaderResourceView** srv, short* Width_out, short* Height_out)
{
	short Width = 0, Height = 0;
	uint8_t Format = 0;
	uint8_t* buf = nullptr;
	int buf_len = 0;
	// Initialize the output to null/failure by default:
	HRESULT res = E_FAIL;
	auto& resources = this->_deviceResources;
	int index = -1;
	*srv = nullptr;

	if (cache)
	{
		index = GetCachedSRV(sDATFileName, GroupId, ImageId, srv);
		if (index != -1)
			return index;
	}

	if (!InitDATReader()) // This call is idempotent and does nothing when DATReader is already loaded
		return -1;

	//if (SetDATVerbosity != nullptr) SetDATVerbosity(true);

	if (!LoadDATFile(sDATFileName)) {
		log_debug("[DBG] Could not load DAT file: %s", sDATFileName);
		return -1;
	}

	if (!GetDATImageMetadata(GroupId, ImageId, &Width, &Height, &Format)) {
		log_debug("[DBG] [C++] DAT Image not found");
		return -1;
	}

	if (Width_out != nullptr) *Width_out = Width;
	if (Height_out != nullptr) *Height_out = Height;

	const bool isBc7 = (Format == 27);

	if (isBc7 && (Width % 4 == 0) && (Height % 4 == 0))
	{
		buf_len = Width * Height;
	}
	else
	{
		buf_len = Width * Height * 4;
	}

	buf = new uint8_t[buf_len];
	if (!isBc7 && g_config.FlipDATImages && ReadFlippedDATImageData != nullptr)
	{
		if (ReadFlippedDATImageData(buf, buf_len))
			res = CreateSRVFromBuffer(buf, buf_len, Width, Height, srv);
		else
			log_debug("[DBG] [C++] Failed to read flipped image data");
	}
	else
	{
		if (ReadDATImageData(buf, buf_len))
			res = CreateSRVFromBuffer(buf, buf_len, Width, Height, srv);
		else
			log_debug("[DBG] [C++] Failed to read image data");
	}

	if (buf != nullptr) delete[] buf;

	if (FAILED(res))
		return -1;

	if (cache)
	{
		index = resources->PushExtraTexture(*srv);
		AddCachedSRV(sDATFileName, GroupId, ImageId, index);
		return index;
	}
	else
	{
		return S_OK;
	}
}

int XwaTextureData::LoadZIPImage(const char* sZIPFileName, int GroupId, int ImageId, bool cache,
	ID3D11ShaderResourceView** srv, short* Width_out, short* Height_out)
{
	auto& resources = this->_deviceResources;
	uint8_t Format = 0;
	uint8_t* buf = nullptr;
	int buf_len = 0;
	// Initialize the output to null/failure by default:
	HRESULT res = E_FAIL;
	int index = -1;
	*srv = nullptr;

	if (cache)
	{
		index = GetCachedSRV(sZIPFileName, GroupId, ImageId, srv);
		if (index != -1)
			return index;
	}

	if (!InitZIPReader()) // This call is idempotent and should do nothing when ZIPReader is already loaded
		return -1;

	//if (SetZIPVerbosity != nullptr) SetZIPVerbosity(true);

	//log_debug("[DBG] [C#] Trying to load ZIP Image: [%s],%d-%d", sZIPFileName, GroupId, ImageId);
	// Unzip the file first. If the file has been unzipped already, this is a no-op
	if (!LoadZIPFile(sZIPFileName)) {
		log_debug("[DBG] Could not load ZIP file: %s", sZIPFileName);
		return -1;
	}

	char* buffer = nullptr;
	int bufferSize = 0;

	if (!GetZIPImageMetadata(GroupId, ImageId, Width_out, Height_out, &buffer, &bufferSize))
		return -1;
	//log_debug("[DBG] [C#] ZIP Image: [%s] loaded", sActualFileName);

	res = DirectX::CreateWICTextureFromMemory(resources->_d3dDevice, (uint8_t*)buffer, (size_t)bufferSize, NULL, srv);

	LocalFree((HLOCAL)buffer);

	if (FAILED(res))
		return -1;

	if (cache)
	{
		index = resources->PushExtraTexture(*srv);
		AddCachedSRV(sZIPFileName, GroupId, ImageId, index);
		return index;
	}
	else
	{
		return S_OK;
	}
}

std::string XwaTextureData::GetDATImageHash(const char* sDATZIPFileName, int GroupId, int ImageId)
{
	return std::string(sDATZIPFileName) + "-" + std::to_string(GroupId) + "-" + std::to_string(ImageId);
}

int XwaTextureData::GetCachedSRV(const char* sDATZIPFileName, int GroupId, int ImageId, ID3D11ShaderResourceView** srv)
{
	std::string hash = GetDATImageHash(sDATZIPFileName, GroupId, ImageId);
	auto it = DATImageMap.find(hash);
	if (it == DATImageMap.end()) {
		*srv = nullptr;
		//log_debug("[DBG] Cached image not found [%s]", hash);
		return -1;
	}
	auto& resources = this->_deviceResources;
	const int index = it->second;
	*srv = resources->_extraTextures[index];
	return index;
}

int XwaTextureData::AddCachedSRV(const char* sDATZIPFileName, int GroupId, int ImageId, int index)
{
	auto& resources = this->_deviceResources;
	std::string hash = GetDATImageHash(sDATZIPFileName, GroupId, ImageId);
	DATImageMap.insert(std::make_pair(hash, index));
	return index;
}

void XwaTextureData::LoadAnimatedTextures(int ATCIndex)
{
	auto& resources = this->_deviceResources;
	AnimatedTexControl* atc = &(g_AnimatedMaterials[ATCIndex]);

	for (uint32_t i = 0; i < atc->Sequence.size(); i++) {
		TexSeqElem tex_seq_elem = atc->Sequence[i];
		ID3D11ShaderResourceView* texSRV = nullptr;
		int index = -1;

		if (tex_seq_elem.IsDATImage) {
			index = LoadDATImage(tex_seq_elem.texname, tex_seq_elem.GroupId, tex_seq_elem.ImageId, true, &texSRV);
			//if (SUCCEEDED(res)) log_debug("[DBG] DAT %d-%d loaded!", tex_seq_elem.GroupId, tex_seq_elem.ImageId);
		}
		else if (tex_seq_elem.IsZIPImage) {
			index = LoadZIPImage(tex_seq_elem.texname, tex_seq_elem.GroupId, tex_seq_elem.ImageId, true, &texSRV);
			//if (SUCCEEDED(res)) log_debug("[DBG] ZIP %d-%d loaded!", tex_seq_elem.GroupId, tex_seq_elem.ImageId);
		}
		else {
			char texname[MAX_TEX_SEQ_NAME + 20];
			wchar_t wTexName[MAX_TEX_SEQ_NAME];
			size_t len = 0;
			sprintf_s(texname, MAX_TEX_SEQ_NAME + 20, "Effects\\Animations\\%s", tex_seq_elem.texname);
			mbstowcs_s(&len, wTexName, MAX_TEX_SEQ_NAME, texname, MAX_TEX_SEQ_NAME);
			//log_debug("[DBG] [MAT] Loading Light(%d) ANIMATED texture: %s for ATC index: %d, extraTexIdx: %d",
			//	this->is_LightTexture, texname, ATCIndex, resources->_extraTextures.size());

			// For some weird reason, I just *have* to do &(resources->_extraTextures[resources->_numExtraTextures])
			// with CreateWICTextureFromFile() or otherwise it. Just. Won't. Work! Most likely a ComPtr
			// issue, but it's still irritating and also makes it hard to manage a dynamic std::vector.
			// Will have to come back to this later.
			//HRESULT res = DirectX::CreateWICTextureFromFile(resources->_d3dDevice, wTexName, NULL,
			//	&(resources->_extraTextures[resources->_numExtraTextures]));
			HRESULT res = DirectX::CreateWICTextureFromFile(resources->_d3dDevice, wTexName, NULL, &texSRV);
			if (FAILED(res))
				index = -1;
		}

		if (index == -1) {
			if (tex_seq_elem.IsDATImage)
				log_debug("[DBG] [MAT] ***** Could not load animated DAT texture [%s-%d-%d]",
					tex_seq_elem.texname, tex_seq_elem.GroupId, tex_seq_elem.ImageId);
			else
				log_debug("[DBG] [MAT] ***** Could not load animated texture [%s]",
					tex_seq_elem.texname);
			atc->Sequence[i].ExtraTextureIndex = -1;
		}
		else {
			//texSRV->AddRef(); // Without this line, funny things happen
			//ComPtr<ID3D11ShaderResourceView> p = texSRV;
			//p->AddRef();
			//resources->_extraTextures.push_back(p);

			atc->Sequence[i].ExtraTextureIndex = index;

			//resources->_numExtraTextures++;
			//atc->LightMapSequence[i].ExtraTextureIndex = resources->_numExtraTextures - 1;
			//log_debug("[DBG] [MAT] Added animated texture in slot: %d",
			//	this->material.LightMapSequence[i].ExtraTextureIndex);
		}
	}

	// Mark this sequence as loaded to prevent re-loading it again later...
	atc->SequenceLoaded = true;
}

int XwaTextureData::LoadGreebleTexture(char* GreebleDATZIPGroupIdImageId, short* Width, short* Height)
{
	auto& resources = this->_deviceResources;
	ID3D11ShaderResourceView* texSRV = nullptr;
	int GroupId = -1, ImageId = -1;
	int index = -1;

	char* substr_dat = stristr(GreebleDATZIPGroupIdImageId, ".dat");
	char* substr_zip = stristr(GreebleDATZIPGroupIdImageId, ".zip");
	bool bIsDATFile = substr_dat != NULL;
	char* substr = bIsDATFile ? substr_dat : substr_zip;
	if (substr == NULL) return -1;
	// Skip the ".dat/.zip" token and terminate the string
	substr += 4;
	*substr = 0;
	// Advance to the next substring, we should now have a string of the form
	// <GroupId>-<ImageId>
	substr++;
	log_debug("[DBG] Loading GreebleTex: %s, GroupId-ImageId: %s", GreebleDATZIPGroupIdImageId, substr);
	sscanf_s(substr, "%d-%d", &GroupId, &ImageId);

	// Load the greeble texture
	if (bIsDATFile)
		index = LoadDATImage(GreebleDATZIPGroupIdImageId, GroupId, ImageId, true, &texSRV, Width, Height);
	else
		index = LoadZIPImage(GreebleDATZIPGroupIdImageId, GroupId, ImageId, true, &texSRV, Width, Height);
	if (index == -1) {
		log_debug("[DBG] Could not load greeble");
		return -1;
	}
	// Link the new texture as a greeble of the current texture
	this->GreebleTexIdx = index;
	//log_debug("[DBG] Loaded Greeble texture at index: %d", this->GreebleTexIdx);
	return this->GreebleTexIdx;
}

int XwaTextureData::LoadNormalMap(char* DATZIPGroupIdImageId, short* Width, short* Height)
{
	auto& resources = this->_deviceResources;
	ID3D11ShaderResourceView* texSRV = nullptr;
	int GroupId = -1, ImageId = -1;
	int index = -1;
	char* substr_dat = stristr(DATZIPGroupIdImageId, ".dat");
	char* substr_zip = stristr(DATZIPGroupIdImageId, ".zip");
	bool bIsDATFile = substr_dat != NULL;
	char* substr = bIsDATFile ? substr_dat : substr_zip;
	if (substr == NULL) return -1;
	// Skip the ".dat/.zip" token and terminate the string
	substr += 4;
	*substr = 0;
	// Advance to the next substring, we should now have a string of the form
	// <GroupId>-<ImageId>
	substr++;
	//log_debug("[DBG] Loading NormalMap: %s, GroupId-ImageId: %s", DATZIPGroupIdImageId, substr);
	sscanf_s(substr, "%d-%d", &GroupId, &ImageId);

	// Load the greeble texture
	if (bIsDATFile)
		index = LoadDATImage(DATZIPGroupIdImageId, GroupId, ImageId, true, &texSRV, Width, Height);
	else
		index = LoadZIPImage(DATZIPGroupIdImageId, GroupId, ImageId, true, &texSRV, Width, Height);
	if (index == -1) {
		log_debug("[DBG] Could not load NormalMap %s", DATZIPGroupIdImageId);
		return -1;
	}
	// Link the new texture as a greeble of the current texture
	this->NormalMapIdx = index;
	//log_debug("[DBG] Loaded NormalMap texture at index: %d", this->NormalMapIdx);
	return this->NormalMapIdx;
}

HRESULT XwaTextureData::CreateSRVFromBuffer(uint8_t* Buffer, int BufferLength, int Width, int Height, ID3D11ShaderResourceView** srv)
{
	auto& resources = this->_deviceResources;
	auto& context = resources->_d3dDeviceContext;
	auto& device = resources->_d3dDevice;

	HRESULT hr;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
	D3D11_SUBRESOURCE_DATA textureData = { 0 };
	ComPtr<ID3D11Texture2D> texture2D;
	*srv = NULL;

	bool isBc7 = (BufferLength == Width * Height);
	DXGI_FORMAT ColorFormat = (g_DATReaderVersion <= DAT_READER_VERSION_101 || g_config.FlipDATImages) ?
		DXGI_FORMAT_R8G8B8A8_UNORM : // Original, to be used with DATReader 1.0.1. Needs channel swizzling.
		DXGI_FORMAT_B8G8R8A8_UNORM;  // To be used with DATReader 1.0.2+. Enables Marshal.Copy(), no channel swizzling.
	desc.Width = (UINT)Width;
	desc.Height = (UINT)Height;
	desc.Format = isBc7 ? DXGI_FORMAT_BC7_UNORM : ColorFormat;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	textureData.pSysMem = (void*)Buffer;
	textureData.SysMemPitch = sizeof(uint8_t) * Width * 4;
	textureData.SysMemSlicePitch = 0;

	if (FAILED(hr = device->CreateTexture2D(&desc, &textureData, &texture2D))) {
		log_debug("[DBG] Failed when calling CreateTexture2D from Buffer, reason: 0x%x",
			device->GetDeviceRemovedReason());
		goto out;
	}

	shaderResourceViewDesc.Format = desc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	if (FAILED(hr = device->CreateShaderResourceView(texture2D, &shaderResourceViewDesc, srv))) {
		log_debug("[DBG] Failed when calling CreateShaderResourceView on texture2D, reason: 0x%x",
			device->GetDeviceRemovedReason());
		goto out;
	}

out:
	// DEBUG
	/*if (texture2D != NULL) {
		hr = DirectX::SaveDDSTextureToFile(context, texture2D, L"C:\\Temp\\_DATImage.dds");
		log_debug("[DBG] Dumped texture2D to file, hr: 0x%x", hr);
	}
	// DEBUG
	if (texture2D != nullptr) texture2D->Release();
	if (texture2DSRV != nullptr) texture2DSRV->Release();
	return NULL;*/
	return hr;
}
