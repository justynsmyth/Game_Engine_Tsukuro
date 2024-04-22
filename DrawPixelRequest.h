#pragma once
#include "ThirdParty/SDL/include/SDL.h"
class DrawPixelRequest
{
public:
	SDL_Color color;
	float x;
	float y;

	DrawPixelRequest();
	DrawPixelRequest(SDL_Color color, float x, float y);

};

