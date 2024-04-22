#pragma once
#include <string>
#include "ThirdParty/SDL/include/SDL.h"
class ImageRenderRequest
{
public:
	std::string image;
	SDL_Color color;
	float x;
	float y;
	int sorting_order;
	float rotation;
	float scale_x;
	float scale_y;
	float pivot_x;
	float pivot_y;

	ImageRenderRequest();
	ImageRenderRequest(std::string image, SDL_Color color, float x, float y, int sorting_order, float rotation, float scale_x, float scale_y, float pivot_x, float pivot_y);

};

