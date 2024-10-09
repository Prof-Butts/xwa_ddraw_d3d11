#include "common.h"
#include "XwaDrawBracketHook.h"
#include "utils.h"
#include "Matrices.h"

//extern bool g_bGlobalDebugFlag; // g_bInhibitCMDBracket, g_bTargetCompDrawn;

std::vector<XwaBracket> g_xwa_bracket;

Vector2 g_SubCMDBracket;
bool g_bRenderEnhancedHUD = true;
bool g_bracketIsCurrentTarget = false;
bool g_bracketIsSubComponent = false;

int GetCurrentTargetIndex();

int TargetBoxHook(int targetIndex, int subComponent, int colorIndex)
{
	int currentTargetIndex = GetCurrentTargetIndex();

	g_bracketIsCurrentTarget = false;
	g_bracketIsSubComponent = false;

	/*log_debug("[DBG] TargetBoxHook: %d, %d, %d, curTarg: %d",
		targetIndex, subComponent, colorIndex, currentTargetIndex);*/

	// subComponent is 0xFFFF when the bracket encloses a craft
	// otherwise this is a sub-component bracket
	if (subComponent != 0xFFFF)
	{
		//colorIndex = 0x2F; // MainPal_White;
		//colorIndex = 0x32; // Blue
		g_bracketIsSubComponent = true;
	}
	else
	{
		// Not a sub-component bracket, highlight the current target:
		if (currentTargetIndex == targetIndex)
		{
			//colorIndex = 0x3D; // Green
			g_bracketIsCurrentTarget = true;
		}
	}

	// g_bracketIsCurrentTarget and g_bracketIsSubComponent are set right before we call
	// the original TargetBox(). Then TargetBox() will call DrawBracketInFlight() which
	// will call DrawBracketInFlightHook() where we'll use the global vars.
	void (*TargetBox)(int, int, int) = (void(*)(
		int targetIndex,
		int subComponent,
		int colorIndex)) 0x503A30;

	TargetBox(targetIndex, subComponent, colorIndex);

	g_bracketIsCurrentTarget = false;
	g_bracketIsSubComponent = false;

	return 0;
}

void DrawBracketInFlightHook(int A4, int A8, int AC, int A10, unsigned char A14, int A18)
{
	XwaBracket bracket;

	bracket.positionX = A4;
	bracket.positionY = A8;
	bracket.width = AC;
	bracket.height = A10;
	bracket.colorIndex = A14;
	bracket.depth = A18;
	bracket.DC = false;
	bracket.isCurrentTarget = g_bracketIsCurrentTarget;
	bracket.isSubComponent = g_bracketIsSubComponent;

	if (bracket.depth == 1)
	{
		bracket.positionX += *(short*)0x07D5244;
		bracket.positionY += *(short*)0x07CA354;
	}

	// This helps classify brackets as belonging to the DC. Unfortunately, if the game is
	// set to a low in-game resolution, like 640x480, then some brackets that should be
	// rendered on ships will be misclassified as CMD brackets too because they are too
	// small! Oh well, I can only do so much!
	if (bracket.width <= 4) {
		bracket.DC = true;
		g_SubCMDBracket.x = (float)bracket.positionX + (float)bracket.width / 2.0f;
		g_SubCMDBracket.y = (float)bracket.positionY + (float)bracket.height / 2.0f;
		// g_SubCMDBracket is in in-game coordinates
		//log_debug("[DBG] g_SubCMDBracket: %0.3f, %0.3f", g_SubCMDBracket.x, g_SubCMDBracket.y);
	}

	//log_debug("[DBG] bracket.width:%d", bracket.width);
	// Looks like CMD brackets always have a width of 4, so that may help in sending them to
	// the DynamicCockpit FG RTV

	//log_debug("[DBG] Bracket Hook");
	/*
		Here's how this hook works:

		[11120][DBG] PRESENT *******************
		[11120][DBG] Bracket Hook
		[11120][DBG](1) Execute() <-- Execute() start
		[11120][DBG](2) Execute() <-- Execute() exit
		[11120][DBG] PRESENT *******************

		In other words: the whole bracket hook is executed *once* before we even start
		rendering anything inside Execute(). How am I supposed to keep track of the state?
		The only way I can tell which brackets should belong to the DC CMD is by looking
		at their size; but there's some danger that they will be misclassified.
	*/

	g_xwa_bracket.push_back(bracket);
}

void DrawBracketMapHook(int A4, int A8, int AC, int A10, unsigned char A14)
{
	XwaBracket bracket;

	bracket.positionX = A4;
	bracket.positionY = A8;
	bracket.width = AC;
	bracket.height = A10;
	bracket.colorIndex = A14;
	bracket.depth = 0;
	bracket.DC = false;

	g_xwa_bracket.push_back(bracket);
}
