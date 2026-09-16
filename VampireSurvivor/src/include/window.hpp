#pragma once
#include "raylib.h"
#include <string>
#include <map>
#include <vector>
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
private:
	static const int WINDOW_WIDTH = 1280;
	static const int WINDOW_HEIGHT = 720;
	const std::string title;
	int fps = 60;
	std::map<std::string, Music> musicMap;
	std::map<std::string, SoundPool> soundMap;
	std::map<std::string, Texture2D> textureMap;
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
		for (auto& [name, texture] : textureMap) {
			UnloadTexture(texture);
		}
		CloseAudioDevice();
		CloseWindow();
	}
	Vector2 getCenter() const {
		return { static_cast<float>(WINDOW_WIDTH) / 2.0f, static_cast<float>(WINDOW_HEIGHT) / 2.0f };
	}
	void LoadMusicStream(const std::string& name, const std::string& path) {
		musicMap[name] = ::LoadMusicStream(path.c_str());
	}
	void loadSoundPool(const std::string& name, const std::string& path, int n) {
		soundMap.emplace(name, SoundPool(path, n));
	}
	void loadTexture(const std::string& name, const std::string& path) {
		textureMap[name] = ::LoadTexture(path.c_str());
	}
	Texture2D getTexture(const std::string& name) const {
		auto it = textureMap.find(name);
		if (it != textureMap.end()) {
			return it->second;
		}
		return Texture2D{}; // Return an empty texture if not found
	}

};