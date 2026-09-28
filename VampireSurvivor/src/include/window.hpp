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
	static const int WINDOW_WIDTH = 1280;
	static const int WINDOW_HEIGHT = 720;
	const std::string title;
	int fps = 60;
	std::unordered_map<std::string, Music> musicMap;
	std::unordered_map<std::string, SoundPool> soundMap;
	std::unordered_map<std::string, Texture2D> spriteMap;
	std::unordered_map<std::string, DrawFromSpriteSet> textureDrawMap;
public:
	Window(std::string _title) :
		title(_title) {
		InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, title.c_str());
		InitAudioDevice();
		SetTargetFPS(fps);
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
		CloseAudioDevice();
		CloseWindow();
	}
	Vector2 getCenter() const {
		return { static_cast<float>(WINDOW_WIDTH) / 2.0f, static_cast<float>(WINDOW_HEIGHT) / 2.0f };
	}
	/*
	void LoadMusicStream(const std::string& name, const std::string& path) {
		musicMap[name] = ::LoadMusicStream(path.c_str());
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

};