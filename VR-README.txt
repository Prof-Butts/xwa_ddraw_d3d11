xwa_vr_ddraw_d3d11, v 2.4.2

Please see READ-THIS-FIRST.txt for additional configuration steps for this
version of ddraw.

Apr 28, 2023 by Blue Max and m0rgg.

Small fixes to Raytraced soft shadows. Fixes to the cockpit look hook that
prevent wrong camera angles in the hangar.

Apr 18, 2023 by Blue Max and m0rgg.

Small fixes to the HD Concourse. Fixes to the Dynamic Cockpit (wrong beam weapon
and shields display for shipds without those systems -- thanks to LPhoenix for
being so dilligent and finding all these bugs). Disable Embree (unfortunately it
causes random lockups in the briefing room, more investigation is needed to
figure out the root cause).

Apr 10, 2023 by Blue Max and m0rgg.

Merge Jeremy's HD Concourse code into this ddraw. This feature needs a new hook
which is still under development and new HD resource files. Other minor fixes
to RT, VR and the Dynamic Cockpit.

Apr 2, 2023 by Blue Max and m0rgg.

Various fixes. Holograms are no longer transparent when travelling through
hyperspace. Wireframe CMD colors should be correct now. Damage textures can now
be scaled and they can be randomized. See the Special Effects Readme and the
AssaultGunboat.mat files for details.

Mar 23, 2023 by Blue Max and m0rgg.

Various fixes. Random crashes on various missions (longer than 10 mins) should
not occur anymore. Mission briefing should not crash anymore when Raytracing is
enabled. Temporary fix to the dark screen seen occasionally in TFTC when the Sun
goes off-screen.

Mar 4, 2023 by Blue Max and m0rgg.

Added support for Embree for Raytracing. To enable it, add the following to
SSAO.cfg:

	raytracing_enable_embree = 1

The file embree3.dll must be placed in the XWA root directory or Embree won't
work.

Fixes to the keyboard (to address problems regarding responsiveness). Fixes to
the wireframe CMD code to prevent random crashes. Restore Ctrl+Alt+G to its
previous function: toggle greebles. Fixes to normal mapping.

Feb 1, 2023 by Blue Max and m0rgg.

Soft ray-traced shadows. These shadows are rendered at sub-resolution and then
blurred, yielding both more pleasing shadows and better performance. Minor fixes
to the engine glows (they tended to disappear under certain angles).

Jan 25, 2023 by Blue Max and m0rgg.

Raytracing is here. This is real-time, screen-space, raytraced shadows are now
available during regular missions, along with a new shader mode that properly
implements PBR materials. Also minor fixes to the Gimbal Lock Fix code.
See "Special Effects Readme.docx" for details on how to enable this feature.

Jan 3, 2023 by Blue Max and m0rgg.

Minor fixes to the Gimbal Lock Fix code. Added a new setting to
GimbalLockFix.cfg:

	enable_rudder = 1

Set it to 0 to disable the joystick rudder.

Jan 1, 2023 by Blue Max and m0rgg.

Gimbal Lock Fix v1.0 (experimental). To toggle this fix, press Ctrl+Alt+G. This
fix is still not yet complete, but it's stable enough. To disable it, open
GimbalLockFix.cfg and set:

	enable_gimbal_lock_fix = 0.
	
Open GimbalLockFix.cfg to see more options.

Dec 9, 2022 by Blue Max and m0rgg.

New instance events for .mat files that can be used to display textures (like
engine glow textures) when ships have reached certain speed or throttle values:

	DisplayIfSpeedGE = X

Displays the current texture if this ship's speed is greater-than-or-equal to
X MGLT.

	DisplayIfThrottleGE = Y

Displays the current texture if this ship's throttle is greater-than-or-equal to
Y%. Throttle is specified in the range 0..100.

	DisplayIfMissionSetSpeedGE = Z

When designing a mission, it's possible to specify an optional Mission Set
Speed for ships as they make their way through different waypoints. This setting
displays the current texture if this ship's throttle is 100% and its Mission Set
Speed is greater-than-or-equal to Z MGLT.

Nov 5, 2022 by Blue Max and m0rgg.

Improvements to the depth buffer by Jeremy. This brings the functionality of the
Effects ddraw on par with Jeremy's v1.3.29 ddraw. The default settings for the
depth parameters are:

ProjectionParameterA = 32.0
ProjectionParameterB = 256.0
ProjectionParameterC = 0.33

These settings can be modified in ddraw.cfg

Nov 1, 2022 by Blue Max and m0rgg.

Fixes (courtesy of m0rgg) for the crashes that happen when playing certain
cutscenes in the game. This requires the latest TgSmush from Jeremy to work
properly.

Oct 19, 2022 by Blue Max and m0rgg.

Raytracing is now supported in the Tech Room. Press Ctrl+S while in the Tech
Room to toggle the effect, or add "raytracing_enabled_in_tech_room = 0" in
SSAO.cfg to disable it. Fixes to Dynamic Cockpit elements when MSAA is on.

Sep 20, 2022 by Blue Max and m0rgg.

Additional randomization of the Shields Down and Beam overlay effects. Minor
fixes to cutscenes (the bottom rows were previously cut off). Probable fix for
the CreateFile2 error (this ddraw may be compatible with Win7). Support for the
GreenAndRedForIFFColorsOnly setting in hook_tourmultiplayer.cfg.

Sep 18, 2022 by Blue Max and m0rgg.

The Shields Down and Beam overlay effects are now randomized. Screen overlay
layers now render bloom. Fix the wireframe display for missions without
backgrounds.

Sep 10, 2022 by Blue Max and m0rgg.

Custcenes can now be displayed in VR headsets. The latest TgSmush is needed for
this feature.

Aug 31, 2022 by Blue Max and m0rgg.

Even more new Instance Events/Animations. Some animations can now be layered
with screen/multiply blending modes. Minor bug fixes.

Aug 10, 2022 by Blue Max and m0rgg.

Introduced new Instance Events/Animations. Fixes to normal mapping and
animations. Added support for Levels.fx. The Diegetic Joystick can now be
toggled globally. Fixes to explosion lights.

July 14, 2022, by blue_max and m0rgg.

Fixes to DirectSBS and SteamVR modes for proper memory usage. Added explosion
lights, Expanded laser light limit to 16. Added directional lights. Any texture
can cast illumination now. Added new Interdiction effect. See the docx for
details.

June 13, 2022, by blue_max and m0rgg.

Various bug fixes including MSAA causing white edges around objects, lighting
artifacts in the hangar and other fixes by Jeremy. The hangar no longer applies
inertia while the external camera is active. To re-enable this effect just set
the following in CockpitLook.cfg:

	enable_external_inertia_in_hangar = 1

April 6, 2022 by blue_max and m0rgg.

New shaders, visual effects and Virtual Reality implementation of ddraw.dll.
Based on Jeremy Ansel's implementation with some features from Reimar's
ddraw.dll.

This library works with the Hook_XWACockpitLook.dll hook (see 
Xwa_Hook_CockpitLook_Readme.txt for instructions on how to configure
this hook). ddraw.dll implements the VR graphics and the hook attaches
the headset to the in-game camera. This library currently supports SteamVR
and Direct SBS modes. For Direct SBS you'll need a third-party solution to
see SBS content on your headset (like Trinus, Virtual Desktop, iVRy, VRidge,
etc).

*** EPILEPSY RISK WARNING (FLASHING LIGHTS) ***

This version comes with "laser lights" enabled. Whenever a laser is fired,
you'll see a short flash illuminating the cockpit and surrounding areas.
For a small percentage of people, these lights might cause epileptic seizures
or blackouts. If you are photosensitive or have a (family) history of epilepsy
you might want to disable these lights before playing the game. The laser
lights can be disabled in-game by pressing Ctrl+D or you can open SSAO.cfg
and find the "enable_laser_lights" parameter and set it to 0.

*** Easy Installation/Configuration ***

Please use the official Effects installer from xwaupgrade.com/downloads

*** SteamVR Configuration/Troubleshooting ***

For configuration and troubleshooting, see this thread:

https://www.xwaupgrade.com/phpBB3/viewtopic.php?f=36&t=12827

This mod will try to set the FOV automatically from your headset's parameters.
However, you may also change the FOV by pressing the following keys:

    Ctrl+Shift+Alt + Right/Left: Adjust FOV.
	
The FOV settings are saved automatically. If you want to reset the FOV, delete the 
FocalLength.cfg.

*** Features ***

* Normal Mapping.

Real Normal Mapping is finally here. The XWAU team is busy upgrading major ships
to support this feature, but in the meantime, it can be activated by adding the
following to the .mat files:

[TEX000AA]
NormalMap = Effects\YourShip.dat-X-Y

ZIP files are also supported.

* Full 6dof support by m0rgg.

Thanks to m0rgg's genius, we now have 6dof in the hangar and gunner turrets!

* Enhanced VR support in 2D areas by m0rgg.

Mission loading screens no longer will block tracking in VR modes and the transition
between 3D and 2D modes is now smooth.

* Diegetic Cockpit.

The cockpit can now respond to inputs provided by the user, providing an additional
level of immersion.

* Enhanced 3D rendering pipeline, thanks to Jeremy.

Jeremy finally figured out how to offload most of the 3D work to the GPU, thus
unlocking a huge performance gain when compared to ddraw 1.1.6.3. Going forward,
"ddraw 1.0" will no longer be maintained and new features will only appear in
"ddraw 2.0".

* Pose-Corrected Head Tracking by m0rgg.

m0rgg has hooked XWA to make full use of 6dof while in the cockpit. Thanks to
him, the gimbal lock problem is now gone from the cockpit. SteamVR is now using
these new hooks to provide even better support for head tracking. If these new
hooks don't work properly you can disable them and go back to the regular
tracking by adding the following to CockpitLook.cfg:

pose_corrected_headtracking = 0

* Experimental support for nVidia's 3DVision.

To enable 3DVision, open the included 3DVision.cfg file and set:

enable_3D_vision = 1

3DVision only works in full screen, so you can set force_fullscreen = 1 to go
full screen automatically. This mode needs more testing, so don't be surprised
if there are still some bugs in this area.

* Side-loaded images can now be stored in ZIP files.

ddraw can now side-load images from ZIP files instead of using DAT files. The
game still needs DAT files, but animations, greebles, Dynamic Cockpit textures
and alternate explosions can now be loaded from ZIP files continaing PNG, JPG
and GIF formats. A DAT-to-ZIP converter is also provided for convenience.

To use ZIP files, just continue using the regular settings in .dc and .mat files
but change the extension from .dat to .zip. You must also convert your DAT into
a ZIP file manually or by using the included DATtoZIPConverterGUI.exe tool.

* Positional Tracking is now jitter-free.

Positional Tracking jitter in VR is now gone, thanks to m0rgg. m0rgg implemented
a new callback inside the CockpitLook hook that gets rid of the jittering people
have experienced in VR. Thanks m0rgg!

* Alternate Explosions.

Each one of the default explosions can have up to 5 variants that are shuffled
randomly every 5 seconds. To enable them, open the material file for each
explosion and add the following:

	alt_frames_1 = <Path>\<DatFile>.dat-<GroupId1>
	alt_frames_2 = <Path>\<DatFile>.dat-<GroupId2>
	alt_frames_3 = <Path>\<DatFile>.dat-<GroupId3>
	alt_frames_4 = <Path>\<DatFile>.dat-<GroupId4>
	ds2_frames = <Path>\<DatFile>.dat-<GroupId5>

The ds2_frames setting specifies explosion versions that will be displayed in
the DS2 mission -- no other versions will be displayed in that case. If you want
to debug each version, you can force it by adding "force_alt_explosion = <-1..4>"
to SSAO.cfg. -1 selects the original version, 0..4 selects the alternate
versions.

* Automatic Greebler/Detail Mapping

The Automatic Greebler. This new effect will automatically add greebles using
external textures when the camera gets close enough to a surface. See
Greebles.dat and the sample material files (greeble textures by Vince T and JC).
See Special_Effects_Readme.docx for details and the sample .mat files for
some examples on how to use this feature.

* Warning Lights and Cannon Ready Events.

The warning lights in the reticle can now be placed in the Dynamic Cockpit using
events. Similarly, new events have been added that get triggered when individual
laser/ion cannons are ready to fire. See AnimatedTextures-Rules.txt for more
details.

* New Animations and Events.

Textures can now be animated and the animations can be triggered by events, like
targeting craft or receiving damage. Cockpit damage is also finally enabled. See
AnimatedTextures-Rules.txt for more details.

* New procedural shaders for Lava and Explosions.

The lava in the Pilot Proving Grounds is now generated procedurally. Explosions
can also be generated procedurally, in mixed mode or just the original
animations from the DAT files (set ExplosionBlendMode to 2, 1, 0 respectively).
Both are controlled through new material files.

* New materials and shaders for the Death Star mission.

The reactor core explosion has been replaced with Duke's explosion shader from:

https://www.shadertoy.com/view/lsySzd

The Death Star now has materials and other visual fixes (the lights will no
longer be turned off when the core is destroyed).

* Dynamic Cockpit.

The DC has additional areas to capture text messages and other text elements
from the CMD.

* TrackIR reload.

The CockpitLook hook will now try to reload TrackIR during gameplay. If that
fails, Alt+T can be used to reload TrackIR manually.

* Holographic Displays.

The Dynamic Cockpit now supports holographic displays.

* Improved Laser Lights.

UV coords can now be used to place lights on the laser textures.

* Fully-metric and distortion-free SteamVR mode.

Previous releases of this mod had a bug that caused distortion for some SteamVR
users. This has been fixed now.

Individual cockpits may have the wrong scale (either too small or too large).
This is something that has to be fixed on a per-cockpit basis.

* SteamVR Performance Improvements.

Thanks to Marcos Orallo (m0rgg), the performance of the SteamVR mode is now
almost on-par with the DirectSBS mode. Several performance improvements have
been made to the SteamVR code in both ddraw.dll and the Cockpit Look Hook.

* Head Tracking in the Hangar and Concourse.

Previous releases disabled head tracking in the hangar and Concourse screens.
It is now possible to look around in these areas, but positional tracking is still
not enabled.

* Cockpit Shadows/Shadow Mapping.

This release has support for cockpit shadows. However, this is still a bit
experimental and requires additional steps to be configured properly. See the
following thread for more details:

https://www.xwaupgrade.com/phpBB3/viewtopic.php?f=9&t=12650

* Reticle placement in VR.

The way the reticle is rendered by the game will cause it to become distorted in
VR mode. To prevent this, I've added code that captures the reticle and resizes
it. Sometimes this code may not work properly. To reset the reticle, you can
press Ctrl+Shift+Alt + Left Arrow followed by Ctrl+Shift+Alt + Right Arrow while
looking straight ahead. This will cause the VR FOV to be resized, but it will
also recapture the reticle.

New Keys added for Reticle Placement:

* Ctrl + Left/Right Arrow: Change the scale of the reticle.
* Shift + Left/Right Arrow: Change the depth of the reticle. The reticle is
  placed at infinity by default, so you'll probably have to modify VRParams.cfg
  to place it manually at a different depth to see any effect with these keys.

* SteamVR Mirror Window.

I've made some effort to try and display a reasonable image in the SteamVR mirror
window, but sometimes it may look a bit squashed. See this thread for instructions
on how to configure the SteamVR Mirror Window Manually:

https://www.xwaupgrade.com/phpBB3/viewtopic.php?f=36&t=12827

* Wireframe targeting computer mode.

The targeting computer can now display a wireframe model instead of a solid
object. The color of the object can be used to identify friends, foes or neutral
ships.

* POV Offsets.

The POV can now be modified temporarily in either VR or regular modes. The format
is the same as in MXvTED. See POVOffsets.cfg for more information.

* Holographic Text Boxes.

The text boxes (and probably other HUD/DC elements) can now be displayed as
holograms floating inside the cockpit in 3D. Cockpits need to be enabled to use
this feature, so this is still a WIP.

* Headlights Shader.

During the Death Star mission, the lights will go out. I've added a new shader
that turns on the ship's headlights by pressing Ctrl+H.

* Speed Effect.

To enhance the illusion of motion, I've added small debris that zoom past the
view when the current ship is moving.

* Integration with XWA's light system.

I've finally integrated XWA's light system with the new shaders. No need to
specify custom lights in SSAO.cfg anymore!

* Procedurally-generated Suns and Flares.

The default sun backdrops have now been replaced with procedurally-generated
animations and coronas. The colors of the suns can be customized too. Take a look
at the Materials folder and look for dat-90* files. I've also added a new flare
shader. I suggest that you disable XWA's flares; but you may combine both if you
want. The sun flare shader is based on the following shaders:

	https://www.shadertoy.com/view/4sX3Rs
	https://www.shadertoy.com/view/XdfXRX
	https://www.shadertoy.com/view/Xlc3D2

* External View Inertia

I've extended the cockpit inertia effect to work on the external view as well.
Feel free to use this along with Jeremy's external hook so that you can get the
full HUD in external view.

* New Text Renderer.

Jeremy recently added support to use any Windows font inside the game, and this
release also supports that feature (see DDraw.cfg for details). Feel free to use
your Aurebesh font in the game!

WARNING: This text renderer is known to affect performance and this is more
notorious in VR. This renderer can be disabled in DDraw.cfg. We're working on
improving the performance of this effect.

* Custom per-craft Reticles.

This version supports Jeremy's hook_reticle in regular and VR modes. For the case
of VR, custom reticles should be placed starting in group 12000 image 5001, since
all the lower-numbered images are already part of the standard reticle.

* Enhanced Radar and Bracket Visibility.

Yet another improvement made by Jeremy that was integrated into this release:
the dots in the left/right sensors and the brackets that frame the currently-
selected target have enhanced visibility in non-VR mode. VR users can use the
new "bracket_strength" setting in Bloom.cfg to make the brackets glow.

* Cockpit Inertia

This feature simulates cockpit momentum or inertia: the cockpit will lag slightly
when turning. This helps improve 3D/depth perception even when not using VR. For
people that are prone to motion sickness, the effect can be disabled with Ctrl+I
or by disabling it in CockpitLook.cfg.

* Enhanced SSDO/Bloom shaders (WIP).

The SSDO shaders have been updated to increase their performance and include proper
specular highlights, fake normal mapping, bent normals, contact shadows and 1-bounce
indirect lighting. However, SSDO is still work-in-progress and only one light is supported 
at this moment.

The bloom effect has also been updated to be compatible with MSAA.

* MSAA/FXAA Support.

This release includes support for MSAA and FXAA. The support for MSAA isn't 100%
finished; but it should remove a lot of "jaggies". I've added a new setting in
ddraw.cfg to optionally select the number of samples to fine-tune the performance.

I've also added support for FXAA. FXAA is a faster alternative to MSAA; but it
tends to dim some details. Both FXAA and MSAA can be combined to remove all jaggies.
FXAA can be enabled in ddraw.cfg; or it can be toggled by pressing Ctrl+Alt+F.

The original FXAA code comes from reinder and can be seen here:
https://www.shadertoy.com/view/ls3GWS

* D3D hook support.

This version is compatible with Jeremy Ansel's 1.3.8 ddraw.dll and supports
the latest hook_d3d.dll to improve the performance of the game. If you're using 
this hook, please make sure that you set "IsHookD3DEnabled = 1" in hook_d3d.cfg. 

* Active Cockpit.

You can now point-and-click on cockpit buttons to issue keyboard commands. I've
added a small circular cursor that can be used to click on active surfaces in
select cockpits. To click on a button, press the [Space Bar]. The main audience
for this feature is people with head tracking hardware; but it can also be used
with mouse look. At this point, only the A-Wing, Y-Wing and RZ-2 A-Wing have been
enabled to use this feature.

This feature is disabled by default because only a handful of cockpits support it.
If you want to use it, go to Active_Cockpit.cfg and enable it there.

* Dynamic Laser Lights.

Lasers will now emit light that will dynamically illuminate the objects around them.
You can configure the intensity of this effect (or disable it altogether) in SSAO.cfg.
Look for the "enable_laser_lights" and "laser_light_intensity" settings.

*** EPILEPSY RISK WARNING ***

These lights will cause a very brief flash in the cockpit. If you are photosensitive
or are at risk of epileptic seizures, you might want to disable this feature.

* New Dynamic/Active Cockpit Project OPTs.

This release includes DTM's new A-Wing and Y-Wing cockpits based on DarkSaber's
cockpits that fully support Dynamic and Active Cockpit (DC/AC) elements. See 
AwingDynamicCockpit_06-02-20.zip and YwingDynamicCockpit_06-02-20.zip for more
details.

* New Shading Model with support for Specular shading and Materials.

Thanks to Jeremy Ansel, we now have a hook that sends smooth normal data from XWA
to ddraw (hook_normals.txt). This information can be used to implement a new shading
model that will eventually replace the previous SSAO/SSDO shaders. In addition to
updated SSAO/SSDO shaders, I've added a new mode called "Deferred Shading". 

The three shader modes (Deferred, SSAO, SSDO) support fake normal mapping, specular
shading, fake HDR look and the ability to specify per-texture, per-OPT materials. Not 
all OPTs have a  material definition file, so more will be added in the future; but 
you can add material files yourself. See the Materials folder for examples.

* Multiplayer support.

This library has been tested in Multiplayer and it seems to work OK. However, the
cockpit look hook is disabled in MP to avoid synchronization issues.

* Enhanced 6dof support for VR and 5dof for TrackIR users.

This release works with a new version of the CockpitLookHook that enables better
positional tracking. You can now lean and move around the cockpit, or even walk
in the larger bridges. Use CockpitLookHook 1.0.0 or above (included in this package).

WARNING: Previous releases had positional tracking settings in VRParams.cfg. I've
moved these parameters to the CockpitLookHook.cfg file. However, the settings for the
cockpit roll are still in VRParams.cfg because this is still a hack that is applied
in ddraw.dll.

* Reimar's Joystick Emulation and X-Z axes swap.

This version ships with Reimar's joystick emulation code (used with permission from
him). I've also added a new setting in DDraw.cfg to swap the X-Z axes so that the
X axis now performs a roll and the Z axis executes the yaw -- no additional software
is needed. This setting is disabled by default.

* New High-Resolution, cockpit-look-enabled Hyperspace Effect.

This release has a new high-resolution hyperspace effect and a new cockpit look
hook. Now you can enable mouse look/head tracking and look around while travelling
through hyperspace. If you're using head tracking or VR, your head won't snap anymore
when the jump is activated. I've also fixed the hyperspace effect for the gunner's
turret in the YT-1300. See Hyperspace.cfg for more details.
I still need to fix how this effect is displayed as I believe the effect's FOV doesn't
match the game's FOV. In practical terms this means that sometimes the effect may not
align perfectly with the screen's center. I'll try to fix this in later releases. You
can either ignore this, or go to Hyperspace.cfg and try to tweak it manually in the
meantime.
Many thanks to keiranhalcyon7 for pointing out details, and for helping in the
implementation of the effect itself.

* Screen-Space Directional Occlusion (SSDO) shader + Bent Normals:

This release includes the first version of the SSDO shader. This shader is
similar to SSAO; but it uses the light direction when computing the shading,
and it can optionally compute diffuse reflections at one bounce (indirect
shading). In addition to this, SSDO is using Bent Normals to compute shading
which has the nice side effect of producing soft contact shadows. This
implementation is not 100% correct and it hasn't been optimized for performance
yet. I'll keep improving this effect in future releases. See SSAO.cfg to
enable and configure SSDO.

By default, the Indirect Lighting step of SSDO is disabled. If you want to
enable it you'll have to set "enable_indirect_ssdo = 1" in SSAO.cfg. The reason
this is disabled is that this is a subtle effect that only has a subtle effect
in the final image; but it has a significant performance impact. I'll keep
working on improving this effect in future releases.

If you activate the indirect lighting, I suggest you also run the algorithm
at sub-resolution (set ssao_buffer_scale_divisor=2) minimize the performance
impact. Since SSDO uses 90% the same infrastructure as SSAO, I've placed the
configuration in SSAO.cfg because both implementations share a lot of details.

I was also able to include Fake Normal Mapping in this release. Depending on the
resolution of the textures, this effect can increase the detail of the models; but
it may also look a bit pixelated at close range. In future releases I'll try
to provide real Normal Mapping which will pave the way to use higher-resolution
textures to solve this problem.

For SSDO, I'm using two colored lights and a colored ambient light level. These
settings can be specified through SSAO.cfg. I've tried to match the main light's
position with the in-game Sun; but sometimes the game seems to put the Sun in the
opposite direction. I'm also not 100% sure I've matched the in-game rotation with
these external lights perfectly, so you may see some inconsistent lighting every
now and then. I'll try to fix this in future releases.

Known issues:
* Halos: Objects in the foreground will always obscure objects in the background
         no matter how far they are. If the background is white, this will show as
		 a gray or black halo around the objects in the foreground.
* Position of the Lights: The lights may not always match the position of light
		 sources, like the in-game Sun.
* Grainy Normal Mapping: If the original texture has a low resolution, this may cause
         the surfaces to look a bit bumpy or pixelated at close range.
* Performance: Indirect lighting causes a noticeable performance penalty. Try running
         with ssao_buffer_scale_divisor=2 to improve the performance.

Keyboard shortcuts:

Ctrl + Alt + O will toggle the SSAO/SSDO effect. The shader is selected in SSAO.cfg.
Ctrl + Alt + P will toggle Indirect Lighting when SSDO is selected.
Ctrl + Alt + N will toggle Normal Mapping.

Many thanks to Pascal Gilcher for all the code and advice shared.

* Native Screen-Space Ambient Occlusion (SSAO) shader:

This release includes the second version of the SSAO shader. The halos around
objects have been removed and the blur is now depth-aware. The scale parameter
is now in meters so it makes more sense and there's been some performance and
memory improvements. To enable it set ssao_enabled to 1 in SSAO.cfg. You can
toggle the effect with Ctrl + Alt + O and display it in debug mode with 
Ctrl + Alt + D (helps fine-tune the effect). See SSAO.cfg for more details and
configuration settings. This is the second release of this shader; but there are
still some known artifacts:

* If you fly too close to a big object, some polygons may flip and create
  edges that SSAO will make even more evident. This is a current limitation
  of the game and it may or may not have a fix.
* SSAO is not applied to the edges of the screen/Overlapping objects may cause
  SSAO to fade in the overlapping areas. These are both known limitations of
  this effect.

I'm using a lot of code and techniques from Pascal Glicher's MXAO (with his
permission) and also some code from José María Méndez's implementation from the
following link:

https://www.gamedev.net/articles/programming/graphics/a-simple-and-practical-approach-to-ssao-r2753/

* Native Bloom Shader:

This release also includes a built-in native bloom shader. This shader is
implemented in a pyramid fashion to gain speed without sacrificing quality.
It's also highly configurable. Each level in the pyramid can have its
own spread and strength. In addition to this, several visual elements can have
their own bloom strength. See bloom.cfg for more details. I've also included
several examples of bloom configurations. This shader is only applied to specific
textures (lasers, engine glow, explosions, etc) and to the illumination textures
for each OPT. This shader is compatible with ReShade and both can be applied at
the same time. If you've used ReShade to apply bloom you'll notice that in this
implementation, white objects are no longer bloomed and ISDs won't be blindingly
bright anymore. Lasers and Engine Glow are also handled separately to enhance them
properly. See Bloom.cfg for more details.

* Dynamic Cockpit:

This version contains a preview of the Dynamic Cockpit project. You don't 
need any VR hardware to use this feature; but the cockpit elements are currently
a bit small to read without VR. I've modified some textures made by the XWAUP team
and I'm distributing them with their permission. If you modify any textures, please
respect the original author and ask for permission before distributing your work.
Go to xwaupgrade.com for more details.

If you're here just for the Dynamic Cockpit, then extract this package to your
XWA installation directory. There are more instructions in the dynamic_cockpit.cfg
and a few examples in the DynamicCockpit sub-directory. You can also visit
xwaupgrade.com for more details. No need to read any further as the rest of this
file deals with the setup for VR users.

NOTE: This version of ddraw.dll is compatible with the 32-bit mode hook; but
	  some textures may become a little dull when running with it. I've added
	  the following parameters to ddraw.cfg to enhance these textures:
	  
		EnhanceLasers = 1
		EnhanceIlluminationTextures = 1
		EnhanceEngineGlow = 1
		EnhanceExplosions = 1

NOTE: If you've used previous versions of this mod, then you should be aware
      that some of the settings in vrparams.cfg are now metric and they have
	  changed names. If you're using vrparams from a previous version, you'll
	  have to update. The settings that changed are used to set the HUD at the
	  proper depth. These settings are probably not going to change anymore.
NOTE: This release is compatible with SteamVR and DirectSBS modes.
NOTE: The new time hook from Jeremy Ansel removes the framerate limit in XWA.
      This improves the performance in SteamVR a little bit; but it vastly
	  improves the performance in DirectSBS mode (in DirectSBS mode it's now
	  possible to achieve 100+fps consistently). This hook is included in
	  this package (with Jeremy's permission) or you may download it from
	  xwaupgrade.com. Either way, make sure you install it as it!
 
More information at xwaupgrade.com -- search for the "X-Wing Alliance VR Mod
Released" and "Head Tracking Hook for XWA" forum threads.

*** Requirements ***

- You'll need a Joystick to play the game.
- Either the GOG or Steam versions of X-Wing Alliance (other versions may also work)
  Yes, the GOG version can be played with SteamVR and you don't even need to add it
  to Steam. Just run SteamVR first and then the GOG version.
- The X-Wing Alliance Upgradde (XWAU) craft pack 1.6 (from xwaupgrade.com)
- DirectX 11
- a CPU with SSE2 support
- VR Headset or other SBS-compatible solution (this version now supports smartphones
  through Google Cardboard + Third party software).

Optional:
- Software that maps the VR orientation to mouse motion (like Trinus, FreePIE,
  or OpenTrack).
- SteamVR.

*** List of Known Supported Hardware ***

DirectSBS mode:
	PSVR through Trinus PSVR.
	A number of smartphones using Google Cardboard (or similar) through Trinus Cardboard
	or similar solutions.

Supported through SteamVR:
	Oculus Rift CV1, DK2
	HTC Vive
	Pimax 5K Plus
	PSVR (through Trinus PSVR or iVRy)
	Smartphones (through iVRy and vRidge -- unconfirmed)
	Other hardware is likely to be supported through SteamVR.

*** Configuration ***

This game reads a configuration file called vrparams.cfg. The VR experience can be
further tweaked by editing this file. Open this file to see further instructions and
parameters.

The cockpit look hook reads a configuration file called cockpitlook.cfg. This file must
be present too.

Several parameters can be configured in SSAO.cfg, Bloom.cfg, Dynamic_Cockpit.cfg,
Active_Cockpit.cfg and POVOffsets.cfg.

*** Usage ***

This VR mod adds a few new keyboard mappings to customize the VR experience:

General Configuration:
Period/Dot key: This key has been overloaded to reset the seated position in SteamVR.
                Look straight ahead and press it twice to fully reset the view.
				(you only need to press it twice because the first time, the cockpit will
				 disappear, disabling head tracking).
Ctrl + Alt + S: Save current VR parameters to vrparams.cfg
Ctrl + Alt + L: Load VR parameters from vrparams.cfg
Ctrl + Alt + R: Reset VR parameters to defaults.

Post-Processing Effects:
Ctrl + Alt + A: Toggle the Bloom effect (only works if this effect was enabled through Bloom.cfg)
                when the game starts).
Ctrl + Alt + O: Toggle the SSAO/SSDO effect (only works if this effect was enabled through SSAO.cfg)
                when the game starts).
Ctrl + Alt + D: Display the SSAO/SSDO effect in debug mode. This helps get a clear view of what each
			    parameter in SSAO does, and helps fine-tune the effect. When SSDO is enabled, it will
				show the obscurance buffer. If SSDO + Indirect Lighting is selected, it will show the
				Indirect Lighting contribution map.
Ctrl + Alt + P: Toggles Indirect Lighting when SSDO is selected.
Ctrl + Alt + N: Toggles Normal Mapping.

Window Scale:
Ctrl + Z: Toggle Zoom-Out Mode (allows you to see all the corners of the screen)
Ctrl + Alt + Shift + (+/-): Adjust Zoom-Out Mode/Concourse/Menu scale

Lens Distortion Adjustment for the DirectSBS mode:
Ctrl + Alt + B: Toggle the Lens Correction Effect ("Barrel Distortion Effect").
Ctrl + Alt + Left/Right: Adjust K1 Lens Distortion Coefficient (fine-tunes the effect)
Ctrl + Alt + Up/Down:	 Adjust K2 Lens Distortion Coefficient (bigger effect, start here)

GUI Depth Adjustment:
Ctrl + Shift + Up Arrow, Down Arrow: Adjust Targeting Computer GUI Depth
Ctrl + Shift + Left Arrow, Right Arrow: Adjust Text Depth -- This should be above the Targeting Computer Depth.
Shift + Left Arrow, Right Arrow: Adjust Aiming HUD Depth.

Fake Positional Tracking (in addition to SteamVR's positional tracking):
Left/Right Arrows: 		Lean left, right.
Up/Down Arrows: 	  	Boost up or crouch down.
Shift+Up/Down Arrows:	Lean forwards/backwards.

*** Known Issues/Limitations ***

- The cockpit shake will no longer work if using the latest CockpitLookHook to support
  enhanced positional tracking.
- Cockpit Look is disabled in Multiplayer to avoid sync issues.
- Cockpit roll is still a hack that is only implemented in VR. TrackIR users still have
  only 5dof.
- Targeting craft will cause a drop in performance. The bigger or more complex the craft,
  the bigger the drop. This is a limitation with the game as it still has to render the
  additional geometry in the CPU.
- Pressing Alt-Tab while in flight may cause crashes. I've tested this with the original
  ddraw.dll and it crashes as well (it's only a bit harder to do).
- The key shortcuts added in this mod become unresponsive in the hangar.
- Head tracking is disabled when the cockpit is not displayed or when using a 
  turret.
- Occasional stuttering when lots of objects are present in the screen. See the performance
  section for tips.
- If you're using Trinus and FreePIE, chances are that your view will be backwards when you
  jump in the cockpit. Press the dot key twice to recalibrate the view.

*** License ***

xwa_vr_ddraw_d3d11 is licensed under the MIT license (see VR-LICENSE.txt)

*** Version History ***

Version 1.1.5, Dec 8, 2020.
New procedural shaders for lava and explosions. New material files for the Death
Star. Additional Dynamic Cockpit areas. Holographic DC elements. TrackIR reload.
Bug fixes.

Version 1.1.4, Aug 10, 2020.
Metric SteamVR reconstruction. Cockpit Shadows. Speed Effect. Wireframe CMD.
Head tracking enabled for the hangar and Concourse. Several minor bug fixes.

Version 1.1.3, May 1, 2020.
Integrated XWA's native lights into the new shaders. Support for Jeremy's new
text renderer (feel free to download an Aurebesh font and use it in the game!)
Fixed the Hyperspace effect's center. This version is on par with Jeremy's
1.3.10 ddraw. New procedurally-generated Suns and new flares. External View
Inertia.

Version 1.1.2, Mar 29, 2020.
Updated SSDO shaders and normal mapping. MSAA/FXAA support. The bloom shader
has been improved to remove jaggies. Several bloom profiles are also provided.
This version is on par with Jeremy's 1.3.8 and supports the D3D hook for improved
performance. First release of the Active Cockpit. Laser lights now emit light.

Version 1.1.1, Feb 23, 2020.
First release of the New Shading Model and updated SSAO/SSDO shaders with a new
Deferred shading mode. The FOV can also be adjusted in real time to fix the 
visual distortion in VR. This release can now be played in Multiplayer mode. 
Also added easy installation scripts.

Version 1.1.0, Jan 29, 2020.
Enhanced 6dof support via the new CockpitLookHook. Reimar's Joystick Emulation
with optional X-Z axes swap. Fixes to the Dynamic Cockpit. Increased the number 
of DC elements per texture to 12. Updated the code to use the latest OpenVR/SteamVR
release from  Jan 2020. Fixed SSAO/SSDO shadows in the sky domes when using DTM's
planetary maps. Minor fixes to the hyperspace effect.

Version 1.0.10, Dec 20, 2019.
New hyperspace effect and cockpit look hook to enable mouse look while jumping
through hyperspace. See Hyperspace.cfg for more details.

Version 1.0.9, Nov 10, 2019.
First release of the SSDO shader + Bent Normals. This implementation includes
diffuse reflections at one bounce and contact shadows. The SSAO shader has also
been improved, and Fake Normal Mapping for both shaders was also added. The SSDO
implementation is not 100% correct and it has not been optimized, so there are
some known artifacts that will be fixed in future releases.

Version 1.0.8, Oct 14, 2019.
Second version of the native SSAO shader with depth-aware blur. Most artifacts
have been removed and the performance has been improved a little bit. Also added
plumbing for future effects.

Version 1.0.7, Oct 5, 2019
First version of the native SSAO shader.

Version 1.0.6, Sep 23, 2019.
Fixed artifacts in Win7 and re-enabled compatiblity with ReShade. The Native Bloom
shader has also been improved for speed and quality. Several other fixes are included
in this release (including some fixes to the Dynamic Cockpit) to increase stability.

Version 1.0.5, Sep 15, 2019.
Fixed a few bugs in the Dynamic Cockpit and implemented a pyramidal Bloom shader
to increase performance without sacrificing quality. Fixed the 25fps Concourse
bug.

Version 1.0.4, Sep 2, 2019.
Dynamic Cockpit Preview 4. Fixed several artifacts and improved the performance of
ddraw.dll. First preview of the native bloom shader. Added compatibility with the
32-bit mode hook by Jeremy Ansel.

Version 1.0.3, Aug 15, 2019.
Dynamic Cockpit Preview 3. The HUD element placement for the dynamic cockpit has
been simplified. HUD elements are now specified in independent configuration files
and they work regardless of screen/in-game resolution. Added the option to move and
scale HUD regions (this is mostly for people with 3-monitor setups).
This version is still not compatible with the 32-bit mode hook by Jeremy Ansel.

Version 1.0.2, Aug 3, 2019.
Dynamic Cockpit preview 2. The background of the HUD is now displayed (it was transparent
in the previous version). Increased the number of HUD slots to 8 per cover texture.
Added optional background color for each HUD element in the cockpit. Removed unnecessary
textures used in the previous release.
This version is not compatible with the 32-bit mode hook by Jeremy Ansel.

Version 1.0.1, July 31, 2019.
Dynamic Cockpit preview. Some minor bug fixes and code cleanup.
This version is probably not compatible with the 32-bit mode hook by Jeremy Ansel.

Version 1.0.0, July 18, 2019.
Re-enabled DirectSBS mode. The aiming reticle can now be fixed to the cockpit or
it can float around following the lasers (this makes it easier to aim). An optional
offset can now be added to the yaw/pitch through cockpitlook.cfg. Improved performance
due to some code refactoring. The new time hook also removes the 60fps frame limit.
The HUD can now be fixed in space, allowing the user to see its full extent by looking
around. The text in the HUD can now be amplified by leaning forward. Also some code
refactoring to enable future projects.

Version 0.9.9. June 18, 2019.
Added experimental support for 6dof. Roll seems to work fine; but the positional
tracking needs more testing. Fixed ghosting effect in Pimax. The parameters in
vrparams are now all in meters and the reconstruction is also fully metric.

Version 0.9.8, June 10, 2019:
Fixed the 2D content (menus and Concourse) to be SteamVR compliant. Added support for
Pimax. Fixed minor issues that may cause eye strain during flight. Vive, Oculus and
Pimax headsets are seemingly supported; but I still consider this an experimental
build.

Version 0.9.7, June 7, 2019:
Fixed the 3D engine to be SteamVR compliant. Moved stereoscopy computation to the
GPU. Support for SteamVR is still experimental.

Version 0.9.6, May 29, 2019:
Rebased all the code on Jeremy Ansel's latest-and-greatest (so the mod has better
performance now). General code cleanup and minor bug fixing. Added support for FreePIE
and SteamVR (through an external hook).

Version 0.9.5, May 13, 2019:
Fixed the "offscreenBuffer invalid parameter" bug that occurred at startup on some
video cards. Fixed some minor bugs and code cleanup. Extended the brightness option
to the Concourse and Menus so that ReShade + Bloom can be used without affecting
these elements. Note that the "Text_brightness" cfg option is now just "brightness"

Version 0.9.4, May 9, 2019:
Changed internal parameters to use IPD in cms. Zoom-Out mode now scales down only the HUD.
Some other minor fixes (scale cannot be decreased below 0.2, etc)

Version 0.9.3, May 7, 2019:
Minor fixes and code cleanup. The text can now be dimmed through vrparams.cfg to prevent
ReShade's Bloom effect on these elements.

Version 0.9.2, May 5, 2019:
Fixed the Tech Library. Fixed the multiple images when displaying animations in the concourse
or when going into a menu. Fixed black-out screen when in the hangar during a campaign. Minor
performance boost and other minor fixes.

Version 0.9.1, May 2, 2019:
Added code to estimate the aspect ratio. Enabled lean forward/backward.

Version 0.9.0, Apr 30, 2019:
First BETA public release. There are some known artifacts; but the game is stable and
playable. I recommend the Quick Skirmish mode; but the campaign can be played too (although
sometimes the screen momentarily goes black when launching from the hangar).

*** Credits ***

Jeremy Ansel's xwa_ddraw_d3d11 and various hooks (used with permission from the author)
https://github.com/JeremyAnsel/xwa_ddraw_d3d11

Darksaber: I've slightly modified several textures by DarkSaber to enable the
Dynamic Cockpit. I'm distributing these textures with permission from the author.

DTM: DC/AC-enabled A-Wing and Y-Wing cockpits (based on Darksaber's previous work).

BlueSkyDefender's Polynomial Barrel Distortion: (used with permission from the author)
https://github.com/BlueSkyDefender/Depth3D/blob/master/Shaders/Polynomial_Barrel_Distortion_for_HMDs.fx
https://github.com/crosire/reshade-shaders/blob/master/Shaders/Depth3D.fx

Justagai's Hook_XWACockpitLook.dll: this new hook allowed me to map FreePIE and SteamVR tracking
directly to the in-game's camera -- no more mouse look!

All the guys behind the X-Wing Alliance Upgrade project: xwaupgrade.com

Song Ho Ahn's Vector and Matrix math (used with permission from the author).

Pascal Gilcher for the Bloom, SSAO and Fake Normal Mapping code, plus technical details
and general advice when implementing shaders.

José María Méndez for the SSAO implementation.

keiranhalcyon7 for his help in the implementation of the new hyperspace effect and FOV adjustment.

The collective mind at Shadertoy.com for various implementation details for the hyperspace
effect. See Hyperspace.cfg for more credits.

Musk's Lens Flare, modified by Icecool, with some additions from Yusef28's shader
See:

	https://www.shadertoy.com/view/4sX3Rs
	https://www.shadertoy.com/view/XdfXRX
	https://www.shadertoy.com/view/Xlc3D2

Reimar for graciously giving permission to use his Joystick Emulation code.

The original FXAA code comes from reinder and can be seen here:
https://www.shadertoy.com/view/ls3GWS

The procedural explosions are a simplified version of Duke's Volumetric
Explosions from:

https://www.shadertoy.com/view/lsySzd

Marcos Orallo (m0rgg) for his invaluable help in improving the performance of
the SteamVR mode and for removing that annoying jittering in the positional
tracking code.

Jason Winzenried (MamiyaOtaru) for the performance improvements to the 2D font
rendering code.

Post-Processing effects and VR Extension by Leo Reyes (blue_max/Prof_Butts,
Reddit), 2020.
