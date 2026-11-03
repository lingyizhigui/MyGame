#pragma once
#include "raylib.h"
#include "json.hpp"
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <set>
#include <fstream>
#include <random>
struct DrawFromSpriteSet{
	Rectangle source;
	Texture2D* sprite;
};
struct ViewTransform {
	float scale;
	Vector2 offset;
};
struct IntVector2 {
	int x;
	int y;
	constexpr operator Vector2() const noexcept{
		return { static_cast<float>(x), static_cast<float>(y) };
	}
};
class SoundPool {
private:
	std::vector<Sound> soundList;
	int index = 0;
public:
	SoundPool(std::string path, int n = 4) {
		soundList.reserve(n);
		for (int i = 0; i < n; i++) {
			soundList.push_back(LoadSound(path.c_str()));
		}
	}
	void play() {
		index = (index + 1) % soundList.size();
		PlaySound(soundList[index]);
	}
	SoundPool(const SoundPool&) = delete;
	SoundPool& operator=(const SoundPool&) = delete;
	~SoundPool() {
		for (auto& sound : soundList) {
			UnloadSound(sound);
		}
	}
};
class Window {
	using json = nlohmann::json;
private:
	static constexpr int WINDOW_WIDTH = 1280;	//约定
	static constexpr int WINDOW_HEIGHT = 720;	//约定
	const std::string title;
	int fps = 60;
	std::map<int, Font> fonts;
	std::unordered_map<std::string, Music> musicMap;
	std::unordered_map<std::string, SoundPool> soundMap;
	std::unordered_map<std::string, Texture2D> spriteMap;
	std::unordered_map<std::string, DrawFromSpriteSet> textureDrawMap;	//只知道名字就能绘制贴图，无需关心具体精灵图
	//加载某大小中文字体
	Font loadCnFont(int fontSize) {
		std::set<int> codepointsSet;
		std::string usedCharacters = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()-_=+[]{};:'\",.<>/?\\|`~ "
			"等级：升级武器神鞭选择升级的物品"
			"增加最大生命值增加生命回复速度减少受到的伤害增加移动速度增加武器伤害增加投射物速度增加投射物持续时间增加投射物范围减少武器冷却时间增加投射物数量增加复活次数增加拾取范围增加幸运增加经验获取速率增加金币掉落诅咒：增加敌人数量"
			"向左右两边挥舞，攻击并击退敌人。"
			"以自己为中心的辐射状区域，伤害一切靠近的敌人"
			"旋转的锤子，向最近的敌人发射。"
			"新物品!！";
		for(const char* c= usedCharacters.c_str(); *c != '\0'; ){
			int byte = 0;
			int codepoint = GetCodepointNext(c, &byte);
			codepointsSet.insert(codepoint);
			c += byte;
		}
		std::vector<int> codepoints(codepointsSet.begin(), codepointsSet.end());
		Font cnFont = LoadFontEx(
			"./resources/fonts/SarasaUiSC-Regular.ttf",
			fontSize,
			codepoints.data(),
			static_cast<int>(codepoints.size())
		);
		SetTextureFilter(cnFont.texture, TEXTURE_FILTER_BILINEAR);	//字体用双线性插值，放大更平滑
		fonts[fontSize] = cnFont;
		return cnFont;
	}
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
		SetExitKey(KEY_NULL);
		//默认加载25字体
		loadCnFont(25);
		loadCnFont(64);
	}
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	~Window() {
		for(auto& [size, font] : fonts) {
			UnloadFont(font);
		}
		for (auto& [name, music] : musicMap) {
			UnloadMusicStream(music);
		}
		//虽然soundPool类会自动释放，但是发生在cloaseAudioDevice之后会出错，所以这里手动释放
		soundMap.clear();
		for (auto& [name, texture] : spriteMap) {
			UnloadTexture(texture);
		}
		CloseAudioDevice();
		CloseWindow();
	}
	Vector2 getCenter() const {
		return { static_cast<float>(WINDOW_WIDTH) / 2.0f, static_cast<float>(WINDOW_HEIGHT) / 2.0f };
	}
	IntVector2 getSize() const {
		return { WINDOW_WIDTH, WINDOW_HEIGHT };
	}
	const Font& getFont(int fontSize = 25) const {
		return fonts.at(fontSize);
	}
	/*
	void LoadMusic(const std::string& name, const std::string& path) {
		musicMap[name] = LoadMusicStream(path.c_str());
	}
	*/
	void loadSoundPool(const std::string& name, const std::string& path, int n) {
		soundMap.try_emplace(name, path, n);	//try_emplace避免构造临时对象
	}
	void loadSpriteSet(const std::string& spritePath, const std::string& jsonPath) {
		std::ifstream file(jsonPath);
		json jsonData;
		file >> jsonData;
		auto [it, _] = spriteMap.emplace(spritePath, LoadTexture(spritePath.c_str()));	
		//emplace返回pair<iterator,bool>，其中iterator指向新插入的元素
		//此处loadTexture无论键是否存在都会调用，且无法删除重复调用的资源，易泄露。但懒得改
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
	const DrawFromSpriteSet& getTexture(const std::string& name) const {
		auto it = textureDrawMap.find(name);
		if (it != textureDrawMap.end()) {
			return it->second;
		}
		assert(false && "Texture not found in textureDrawMap");//不能返回空的DrawFromSpriteSet，因为调用者会直接解引用sprite指针
		return {};	//release模式报错，不得不
	}
	ViewTransform getViewTransform() const {
		float sizeX = static_cast<float>(GetScreenWidth()) / WINDOW_WIDTH;
		float sizeY = static_cast<float>(GetScreenHeight()) / WINDOW_HEIGHT;
		float size = std::min(sizeX, sizeY);
		Vector2 offset = { (GetScreenWidth() - WINDOW_WIDTH * size) / 2.0f, (GetScreenHeight() - WINDOW_HEIGHT * size) / 2.0f };
		return { size, offset };
	}
	void beginDrawing() const{
		BeginDrawing();
		ClearBackground(RAYWHITE);
	}
	void endDrawing() const{
		auto [scale, offset] = getViewTransform();
		int left = static_cast<int>(std::ceil(offset.x));
		int top = static_cast<int>(std::ceil(offset.y));
		int right = static_cast<int>(std::floor(offset.x + WINDOW_WIDTH * scale));
		int bottom = static_cast<int>(std::floor(offset.y + WINDOW_HEIGHT * scale));
		DrawRectangle(0, 0, left, GetScreenHeight(), BLACK);
		DrawRectangle(0, 0, GetScreenWidth(), top, BLACK);
		DrawRectangle(right, 0,GetScreenWidth() - right, GetScreenHeight(), BLACK);
		DrawRectangle(0, bottom,GetScreenWidth(), GetScreenHeight() - bottom, BLACK);
		EndDrawing();
	}
};

//一些通用函数
inline bool bernoulli(double probability) {
	static std::mt19937 rng{ std::random_device{}() };
	bool res = std::bernoulli_distribution(probability)(rng);
	return res;
}
inline std::string specificWidthString(const std::string& str, float width, int fontSize, const Window& window) {
	std::string result;
	std::string temp;
	Font font = window.getFont(fontSize);
	for(const char* c = str.c_str(); *c != '\0'; ) {
		int byte = 0;
		GetCodepointNext(c, &byte);
		temp.append(c, byte);
		float textWidth = MeasureTextEx(font, temp.c_str(), fontSize, 0.0f).x;
		if (textWidth > width && temp.size() > byte) {//防止单个字符过宽导致死循环
			temp.resize(temp.size() - byte);
			temp.append("\n");
			result.append(temp);
			temp.clear();
			continue;
		}
		c += byte;
	}
	result.append(temp);
	return result;
}