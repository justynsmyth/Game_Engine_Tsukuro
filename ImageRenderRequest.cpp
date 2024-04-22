#include "ImageRenderRequest.h"

ImageRenderRequest::ImageRenderRequest() {
	image = "";
	x = 0;
	y = 0;
	sorting_order = 0;
	color = { 255, 255, 255, 255 };
	scale_y = 0;
	scale_x = 0;
	rotation = 0;
}
ImageRenderRequest::ImageRenderRequest(std::string image, SDL_Color color,
	float x, float y,
	int sorting_order, float rotation,
	float scale_x, float scale_y,
	float pivot_x, float pivot_y) :
	image(image), color(color), x(x), y(y), sorting_order(sorting_order),
	rotation(rotation), scale_x(scale_x), scale_y(scale_y), pivot_x(pivot_x), pivot_y(pivot_y)
{}
