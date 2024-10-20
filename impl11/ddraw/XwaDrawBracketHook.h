#pragma once

#include <vector>

struct XwaBracket
{
	int positionX;
	int positionY;
	int width;
	int height;
	unsigned char colorIndex;
	int depth;
	bool DC;
	bool isCurrentTarget;
	bool isSubComponent;
};

extern std::vector<XwaBracket> g_xwa_bracket;
extern int g_currentTargetObjectId;

int TargetBoxHook(int targetIndex, int subComponent, int colorIndex);

void DrawBracketInFlightHook(int A4, int A8, int AC, int A10, unsigned char A14, int A18);

void DrawBracketMapHook(int A4, int A8, int AC, int A10, unsigned char A14);
