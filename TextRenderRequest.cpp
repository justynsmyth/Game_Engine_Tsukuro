#include "TextRenderRequest.h"


TextRenderRequest::TextRenderRequest() {
	text = "";
	font = "";
	size = 0;
	x = 0;
	y = 0;
}


TextRenderRequest::TextRenderRequest(
	std::string text, std::string font, SDL_Color color, int size, int x, int y) :
	text(text), font(font), color(color), size(size), x(x), y(y) {}