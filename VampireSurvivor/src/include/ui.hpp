#pragma once
#include "raylib.h"
#include "weapon.hpp"
class Panel {
public:
	const Window& window;
	int width =	640;
	int height = 480;
	int TopMargin = 100;
	int LeftMargin = 1280 / 2 - width / 2;
	Color backgroundColor = DARKBLUE;
	Color borderColor = ORANGE;
	class Button {
	public:
		const Window& window;
		IntVector2 position;
		int width;
		int height;
		Color backgroundColor = RAYWHITE;
		Color borderColor = BLACK;
		bool isHovered = false;
		Button(const Window& _window,IntVector2 _position, int _width, int _height, Color _backgroundColor) :
			window(_window), position(_position), width(_width), height(_height), backgroundColor(_backgroundColor) {
		}
		virtual ~Button() = default;
		virtual void drawUnique(int _topMargin, int _leftMargin) const {};
		void draw(int _topMargin,int _leftMargin) const {
			Vector2 pos = { static_cast<float>(_leftMargin + position.x),static_cast<float>(_topMargin + position.y) };
			Color drawingColor = backgroundColor;
			if (!isHovered) drawingColor.r -= 100;
			Rectangle rect = { pos.x, pos.y, static_cast<float>(width), static_cast<float>(height) };
			DrawRectangleRec(rect, drawingColor);
			DrawRectangleLinesEx(rect, 1.0, borderColor);
			drawUnique(_topMargin, _leftMargin);
		}
	};
	std::vector<std::unique_ptr<Button>> buttons;
	Panel(const Window& _window) : window(_window) {
	}
	virtual ~Panel() = default;
	virtual void drawUnique() const {};
	void draw() const {
		DrawRectangle(LeftMargin, TopMargin, width, height, backgroundColor);
		DrawRectangleLines(LeftMargin, TopMargin, width, height, borderColor);
		drawUnique();
		for (const auto& button : buttons) {
			button->draw(TopMargin, LeftMargin);
		}
	}
};
class LevelupPanel : public Panel {
public:
	static constexpr int _Width = 600;
	static constexpr int _Height = 580;
	static constexpr int TitleHeight = 64 ;
	static constexpr int PaddingLeft = 16;
	char choosedIndex = 0;
	static constexpr char DefaultPressCooldown = 20;
	char pressCooldown = 0;
	class LevelupButton : public Button {
	public:
		//static constexpr int _width = 400;
		static constexpr int _height = 115;
		static constexpr Color _backgroundColor = { 100,100,100,255 };
		static constexpr int _paddingTop = 8;
		static constexpr int _paddingLeft = 8;
		static constexpr int iconSize = 64;
		static constexpr int fontSize = 25;
		char index;
		BaseItem* item;
		const std::string_view description;
		std::string descriptionResized;
		const DrawFromSpriteSet& texture;
		LevelupButton(const Window& _window, IntVector2 _position, char _index, BaseItem* _item, int _width) :
			Button(_window, _position, _width, _height, _backgroundColor), index(_index), item(_item),
			description(item->getDescription()), texture(item->getIconTexture()){
			descriptionResized = specificWidthString(description.data(), static_cast<float>(width - _paddingLeft * 2), fontSize, window);
		}
		void drawUnique(int _topMargin, int _leftMargin) const override {
			Vector2 pos = { static_cast<float>(_leftMargin + position.x),static_cast<float>(_topMargin + position.y) };
			//图标和右边的名字
			Rectangle iconRect = { pos.x + _paddingLeft,pos.y + _paddingTop,static_cast<float>(iconSize),static_cast<float>(iconSize) };
			DrawRectangleRounded(iconRect, 0.2f, 0, DARKGRAY);
			DrawTexturePro(*texture.sprite, texture.source,
				iconRect, { 0.0f,0.0f }, 0.0f, WHITE);
			DrawRectangleRoundedLinesEx(iconRect, 0.2f, 0, 2.0f, ORANGE);
			DrawTextEx(window.getFont(fontSize), item->getName().data(), 
				{ pos.x + _paddingLeft + iconSize + _paddingLeft, pos.y + _paddingTop }, 
				fontSize, 0.0f, BLACK);
			//等级文字
			int paddingMiddle = width / 2;
			std::string levelText = std::format("等级：{}", item->getLevel() + 1);
			if (item->getLevel() == 0) levelText = "新物品！";
			DrawTextEx(window.getFont(fontSize), levelText.c_str(),
				{ pos.x + _paddingLeft + iconSize + paddingMiddle, pos.y + _paddingTop },
				fontSize, 0.0f, BLACK);
			//描述文字
			DrawTextEx(window.getFont(fontSize), description.data(),
				{ pos.x + _paddingLeft, pos.y + _paddingTop + iconSize + _paddingTop },
				fontSize, 0.0f, BLACK);
		}
	};
	LevelupPanel(std::array<BaseItem*, 4> items) : Panel(items[0]->getWindow()) {
		width = _Width;
		height = _Height;
		LeftMargin = 1280 / 2 - width / 2;
		int buttonHeight = (height - TitleHeight) / 4;
		for (int i = 0; i < items.size(); i++) {
			if (items[i] == nullptr) break;
			buttons.emplace_back(std::make_unique<LevelupButton>(window, IntVector2(PaddingLeft, TitleHeight + i * buttonHeight), i, items[i], width - PaddingLeft * 2));
		}
		buttons[choosedIndex]->isHovered = true;
	}
	void drawUnique() const override {
		DrawTextEx(window.getFont(64), "    选择升级的物品", { static_cast<float>(LeftMargin + PaddingLeft), static_cast<float>(TopMargin) }, 64, 0.0f, WHITE);
	}
	BaseItem* update() {
		if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_SPACE)){
			LevelupButton* choosedButton = static_cast<LevelupButton*>(buttons[choosedIndex].get());
			return choosedButton->item;
		}
		char dy = 0;
		if(IsKeyDown(KEY_UP)||IsKeyDown(KEY_W)) dy -= 1;
		if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) dy += 1;
		if(dy!=0 && buttons.size()>1){
			pressCooldown--;
			if (pressCooldown <= 0) {
				buttons[choosedIndex]->isHovered = false;
				choosedIndex = (choosedIndex + dy + static_cast<char>(buttons.size())) % static_cast<char>(buttons.size());
				buttons[choosedIndex]->isHovered = true;
				pressCooldown = DefaultPressCooldown;
			}
		}
		else pressCooldown = 0;
		return nullptr;
	}
};