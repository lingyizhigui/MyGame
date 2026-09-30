#pragma once
#include "raylib.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include "json.hpp"
struct DrawFromSpriteSet{
	Rectangle source;
	Texture2D* sprite;
};
struct IntVector2 {
	int x;
	int y;
};
class SoundPool {
private:
	std::vector<Sound> soundList;
public:
	SoundPool(std::string path, int n = 4) {
		soundList.reserve(n);
		for (int i = 0; i < n; i++) {
			soundList.push_back(LoadSound(path.c_str()));
		}
	}
	void play() {
		static int index = 0;
		index = (index + 1) % soundList.size();
		PlaySound(soundList[index]);
	}
	~SoundPool() {
		for (auto& sound : soundList) {
			UnloadSound(sound);
		}
	}
};
class Window {
	using json = nlohmann::json;
private:
	static constexpr int WINDOW_WIDTH = 1280;
	static constexpr int WINDOW_HEIGHT = 720;
	const std::string title;
	int fps = 60;
	RenderTexture canvas;
	std::unordered_map<std::string, Music> musicMap;
	std::unordered_map<std::string, SoundPool> soundMap;
	std::unordered_map<std::string, Texture2D> spriteMap;
	std::unordered_map<std::string, DrawFromSpriteSet> textureDrawMap;
public:
	Window(std::string _title) :
		title(_title) {
		//SetConfigFlags(FLAG_BORDERLESS_WINDOWED_MODE| FLAG_VSYNC_HINT);
		InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, title.c_str());
		SetWindowState(FLAG_VSYNC_HINT  | FLAG_WINDOW_RESIZABLE);
		Image icon = LoadImage("./resources/icon.png");
		SetWindowIcon(icon);
		InitAudioDevice();
		SetTargetFPS(fps);
		canvas = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);
	}
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	~Window() {
		for (auto& [name, music] : musicMap) {
			UnloadMusicStream(music);
		}
		for (auto& [name, soundPool] : soundMap) {
			// SoundPool destructor will handle unloading
		}
		for (auto& [name, texture] : spriteMap) {
			UnloadTexture(texture);
		}
		UnloadRenderTexture(canvas);
		CloseAudioDevice();
		CloseWindow();
	}
	Vector2 getCenter() const {
		return { static_cast<float>(WINDOW_WIDTH) / 2.0f, static_cast<float>(WINDOW_HEIGHT) / 2.0f };
	}
	IntVector2 getCanvasSize() const {
		return { WINDOW_WIDTH,WINDOW_HEIGHT };
	}
	/*
	void LoadMusic(const std::string& name, const std::string& path) {
		musicMap[name] = LoadMusicStream(path.c_str());
	}
	*/
	void loadSoundPool(const std::string& name, const std::string& path, int n) {
		soundMap.emplace(name, SoundPool(path, n));
	}
	void loadSpriteSet(const std::string& spritePath, const std::string& jsonPath) {
		std::ifstream file(jsonPath);
		json jsonData;
		file >> jsonData;
		auto [it,_] = spriteMap.emplace(spritePath, LoadTexture(spritePath.c_str()));
		Texture2D* spritePtr = &(it->second);
		for (const auto& frame : jsonData["textures"][0]["sprites"]) {
			std::string name = frame["filename"];
			int x = frame["region"]["x"];
			int y = frame["region"]["y"];
			int w = frame["region"]["w"];
			int h = frame["region"]["h"];
			textureDrawMap.emplace(name, DrawFromSpriteSet{ { static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) }, spritePtr });
		}
	}
	void loadTemporaryTexture(const std::string& name, const std::string& path) {
		Texture2D texture = LoadTexture(path.c_str());
		spriteMap.emplace(name, texture);
		textureDrawMap.emplace(name, DrawFromSpriteSet{ { 0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height) }, &spriteMap.at(name) });
	}
	DrawFromSpriteSet getTexture(const std::string& name) const {
		auto it = textureDrawMap.find(name);
		if (it != textureDrawMap.end()) {
			return it->second;
		}
		return DrawFromSpriteSet{}; // Return an empty texture if not found
	}
	void beginDrawing() {
		BeginDrawing();
		ClearBackground(BLACK);
		BeginTextureMode(canvas);
		ClearBackground(RAYWHITE);
	}
	void endDrawing() {
		EndTextureMode();
		float sizeX = static_cast<float>(GetScreenWidth()) / WINDOW_WIDTH;
		float sizeY = static_cast<float>(GetScreenHeight()) / WINDOW_HEIGHT;
		float size = std::min(sizeX, sizeY);
		DrawTexturePro(canvas.texture, { 0.0f, 0.0f, static_cast<float>(WINDOW_WIDTH), -static_cast<float>(WINDOW_HEIGHT) },
			{ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f, WINDOW_WIDTH * size, WINDOW_HEIGHT * size},
			{ WINDOW_WIDTH*size/2.0f,  WINDOW_HEIGHT*size/2.0f }, 
			0.0f, WHITE);
		EndDrawing();
	}
};