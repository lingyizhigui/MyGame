#pragma once
#include "raylib.h"
#include "tinyxml2.h"
#include "json.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
struct CppMusic{
	Music m;
	CppMusic(std::string path) {
		m = LoadMusicStream(path.c_str());
		PlayMusicStream(m);
	}
	void update() const{
		UpdateMusicStream(m);
	}
	~CppMusic() {
		UnloadMusicStream(m);
	}
};
struct CppSound{
	std::vector<Sound> list;
	const int length;
	int index = 0;
	CppSound(std::string path, int n = 4) :length(n), index(0) {
		list.reserve(length);
		for (int i = 0; i < length; i++) {
			list.push_back(LoadSound(path.c_str()));
		}
	}
	void play() {
		index = (index + 1) % length;
		PlaySound(list[index]);
	}
	~CppSound() {
		for (int i = 0; i < length; i++) {
			UnloadSound(list[i]);
		}
	}
};
struct Window {
	static const int WINDOW_HEIGHT = 800;
	static const int WINDOW_WIDTH = 1280;
	int height = WINDOW_HEIGHT;
	int width = WINDOW_WIDTH;
	int fps = 60;
	std::string title = "My Game";
	std::map<std::string, Rectangle> posMap;
	Texture2D allImg;
	Texture2D bricksImg;
	std::unique_ptr<CppMusic> backgroundMusic;
	std::unique_ptr<CppSound> winSound;
	std::unique_ptr<CppSound> breakSound;

	Window(std::string _title, int _fps, int _height = WINDOW_HEIGHT, int _width = WINDOW_WIDTH) :
		title(_title), fps(_fps), height(_height), width(_width)
	{
		InitWindow(width, height, title.c_str());
		InitAudioDevice();
		SetTargetFPS(fps);
		allImg = LoadTexture("./resources/Spritesheet/spritesheet_double.png");
		bricksImg = LoadTexture("./resources/Spritesheet/bricks.png");
		backgroundMusic = std::make_unique<CppMusic>("./resources/music/The Last Encounter Medium Loop.wav");
		winSound = std::make_unique<CppSound>("./resources/music/Won!.wav",1);
		breakSound = std::make_unique<CppSound>("./resources/music/break_block_3.wav", 8);
		LoadXml();
		LoadJson();
	}
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	~Window() {
		UnloadTexture(allImg);
		UnloadTexture(bricksImg);
		CloseAudioDevice();
		CloseWindow();
	}
	void setFps(int n) {
		fps = n;
		SetTargetFPS(fps);
	}
	void LoadXml() {
		tinyxml2::XMLDocument doc;
		if (doc.LoadFile("./resources/Spritesheet/spritesheet_double.xml") != tinyxml2::XML_SUCCESS) {
			terminate();
		}
		for (tinyxml2::XMLElement* sub = (doc.RootElement()->FirstChildElement("SubTexture"));
			sub != nullptr;
			sub = sub->NextSiblingElement("SubTexture")) {
			const char* name = sub->Attribute("name");
			float x = sub->FloatAttribute("x");
			float y = sub->FloatAttribute("y");
			float w = sub->FloatAttribute("width");
			float h = sub->FloatAttribute("height");
			posMap[name] = { x,y,w,h };
		}
	}
	void LoadJson() {
		nlohmann::json json;
		std::ifstream file("./resources/Spritesheet/bricks.json");
		file >> json;
		for (auto& [name, frame_data] : json["frames"].items()) {
			// 提取 frame 子对象中的 x, y, w, h
			const auto& frame = frame_data["frame"];
			Rectangle rect;
			rect.x = frame["x"].get<int>();
			rect.y = frame["y"].get<int>();
			rect.width = frame["w"].get<int>();
			rect.height = frame["h"].get<int>();

			posMap[name] = rect;
		}

	}
};
struct TextCenter {
	const std::string text;
	int y;
	int x;
	const int fontSize;
	const Color color;
	TextCenter(std::string t, int _fontSize, Color _color)
		:text(t), y(0), x(0), fontSize(_fontSize), color(_color) {};
	TextCenter(std::string t, const Window& window,Color _color = RAYWHITE, int _fontSize = 40,  int changeHeight = 0)
		:text(t), fontSize(_fontSize),color(_color) {
		y = window.height / 2 - fontSize / 2;
		x = window.width / 2 - MeasureText(text.c_str(), fontSize) / 2;
	}
	void setFromWindow(const Window& window) {
		y = window.height / 2 - fontSize / 2;
		x = window.width / 2 - MeasureText(text.c_str(), fontSize) / 2;
	}
	void draw() const{
		DrawText(text.c_str(), x, y, fontSize, color);
	}
};