#pragma once
#ifndef RENDERER_H
#define RENDERER_H

#include <string>
#include <iostream>
#include <algorithm>
#include "ThirdParty/glm-0.9.9.8/glm/glm.hpp"
#ifdef  __APPLE__
	#include "framework/SDL2.framework/Headers/SDL.h"
	#include "framework/SDL2_image.framework/Headers/SDL_image.h"
	#include "framework/SDL2_ttf.framework/Headers/SDL_ttf.h"
#elif _WIN32
	#include "ThirdParty/SDL/include/SDL.h"
	#include "ThirdParty/SDL_image/include/SDL_image.h"
	#include "ThirdParty/SDL_ttf/include/SDL_ttf.h"
#else
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_image.h>
	#include <SDL2/SDL_ttf.h>
#endif
#include "ThirdParty/rapidjson/include/rapidjson/document.h"
#include "glad/glad.h"
#include "openvr-master/headers/openvr.h"
#include "TextRenderRequest.h"
#include "ImageRenderRequest.h"
#include "UIRenderRequest.h"
#include "DrawPixelRequest.h"
#include "Actor.h"
#include "GameManager.h"
#include "Header.h"
#include "AudioDB.h"
#include "Input.h"
#include <algorithm>

class openvr_utils;

struct TextData {
	std::string text_content;
	int font_size;
	SDL_Color font_color;
	int x;
	int y;
};

struct CompareImageRequests {
	bool operator()(const ImageRenderRequest& a, const ImageRenderRequest& b) const {
		return a.sorting_order < b.sorting_order;
	}
};

struct CompareUIRequests {
	bool operator()(const UIRenderRequest& a, const UIRenderRequest& b) const {
		return a.sorting_order < b.sorting_order;
	}
};


class Renderer
{
public:

	Renderer(rapidjson::Document& renderConfig, rapidjson::Document& doc, AudioDB* audioDB, GameManager* game, openvr_utils* Open_VR);

	~Renderer() {
		//if (hudFont != NULL) {
		//	TTF_CloseFont(hudFont);
		//}
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		TTF_Quit();
		SDL_Quit();
	}

	SDL_Window* getWindow() {
		return window;
	}

	SDL_Renderer* getRenderer() {
		return renderer;
	}

	void ProcessInput();

	void RenderText(TTF_Font* font, const std::string& text, int x, int y, SDL_Color& color);

	void renderScene(std::vector<std::shared_ptr<Actor>>);

	void CheckFont(const std::string& fontName);

	static void LuaDrawText(const std::string str_content, float x, float y, const std::string& font_name, float font_size, float r, float g, float b, float a);
	static void LuaDrawUI(const std::string image_name, float x, float y);
	static void LuaDrawUIEx(const std::string image_name, float x, float y, float r, float g, float b, float a, float sorting_order);
	static void LuaDraw(const std::string image_name, float x, float y);
	static void LuaDrawEx(const std::string image_name,
		float x, float y,
		float rotation_degrees,
		float scale_x, float scale_y,
		float pivot_x, float pivot_y,
		float r, float g, float b, float a,
		float sorting_order);
	static void LuaDrawPixel(float x, float y, float r, float g, float b, float a);


	static void SetCameraPosition(float x, float y);
	static float GetCameraPositionX();
	static float GetCameraPositionY();
	static void SetZoomFactor(float zoom_factor);
	static float GetZoomFactor();


	void static addTextRequest(TextRenderRequest& text_request);


	TTF_Font* GetFont(const std::string& font_name, int font_size);

	static SDL_Texture* getTexture(const std::string& imageName);

	static inline SDL_Renderer* renderer;

	bool isOpenGL();
private:
	bool initGL(openvr_utils* Open_VR);
	void printProgramLog(GLuint program);
	void printShaderLog(GLuint shader);
	void RenderTextRequest(TextRenderRequest& text_request);
	void RenderImageRequest(ImageRenderRequest& image_request);
	void RenderUIRequest(UIRenderRequest& ui_request);
	void RenderPixelRequest(DrawPixelRequest& pixel_request);

	bool OpenGLActivated;
	// HUD and Images
	std::unordered_map<std::string, std::unordered_map<int, TTF_Font*>> fonts;
	static inline std::unordered_map < std::string, SDL_Texture*> textures;


	GameManager* gameManager;
	AudioDB* audioDB;
	SDL_Window* window;

	int window_w;
	int window_h;
	static inline float zoom_factor;
	bool x_scale_flip_on_movement;
	static inline glm::vec2 cameraPosition;

	Uint8 clear_color_r;
	Uint8 clear_color_g;
	Uint8 clear_color_b;

	static inline std::vector<ImageRenderRequest> image_render_requests;
	static inline std::vector<TextRenderRequest> text_render_requests;
	static inline std::vector<UIRenderRequest> ui_render_requests;
	static inline std::vector<DrawPixelRequest> pixel_render_requests;

	GLuint gProgramID = 0;
	GLint gVertexPos2DLocation = -1;
	GLuint gVBO = 0;
	GLuint gIBO = 0;

};
#endif