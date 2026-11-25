#pragma once
#include "raylib.h"
#include "weapon.hpp"
class Panel {
protected:
	static constexpr char DefaultPressCooldown = 20;
	char pressCooldown = 0;
	size_t choosedIndex = 0;
	void selectNext(Vector2 direction) {
		if (buttons.size() <= 1) return;
		Vector2 currentCenter = buttons[choosedIndex]->center();
		float bestScore = std::numeric_limits<float>::infinity();
		size_t bestIndex = choosedIndex;
		for(size_t i = 0; i < buttons.size(); i++) {
			if (i == choosedIndex) continue;
			Vector2 otherCenter = buttons[i]->center();
			Vector2 diff = Vector2Subtract(otherCenter, currentCenter);
			float dot = Vector2DotProduct(diff, direction);
			if (dot <= 0) continue;
			float sideways = Vector2DotProduct(diff, { -direction.y, direction.x });
			float score = dot * 3.0f + std::abs(sideways);
			if(score < bestScore) {
				bestScore = score;
				bestIndex = i;
			}
		}
		buttons[choosedIndex]->isSelected = false;
		choosedIndex = bestIndex;
		buttons[choosedIndex]->isSelected = true;
	}
public:
	const Window& window;
	int width =	640;
	int height = 480;
	IntVector2 position = { 1280 / 2 - width / 2,100 };
	Color backgroundColor = DARKBLUE;
	Color borderColor = ORANGE;
	class Button {
	public:
		const Window& window;
		IntVector2 position;
		int width;
		int height;
		Color backgroundColor;
		static constexpr Color borderColor = BROWN;
		bool isSelected = false;
		std::function<void()> onClick = nullptr;
		Vector2 center() const {
			return { static_cast<float>(width) / 2 + position.x,static_cast<float>(height) / 2 + position.y };
		}
		Button(const Window& _window,IntVector2 _position, int _width, int _height, Color _backgroundColor) :
			window(_window), position(_position), width(_width), height(_height), backgroundColor(_backgroundColor) {
		}
		virtual ~Button() = default;
		virtual void drawUnique() const {};
		void draw() const {
			Vector2 pos = { static_cast<float>(position.x),static_cast<float>(position.y) };
			Color drawingColor = backgroundColor;
			Color drawingBoarderColor = borderColor;
			float boarderThickness = 4.0f;
			if (isSelected) {
				drawingColor = ColorBrightness(drawingColor, 0.3f);
				drawingBoarderColor = ColorBrightness(drawingBoarderColor, -0.2f);
				boarderThickness *= 0.5f;
			}
			Rectangle rect = { pos.x, pos.y, static_cast<float>(width), static_cast<float>(height) };
			DrawRectangleRec(rect, drawingColor);
			DrawRectangleLinesEx(rect, boarderThickness, drawingBoarderColor);
			drawUnique();
		}
	};
	std::vector<std::unique_ptr<Button>> buttons;
	Panel(const Window& _window) : window(_window) {
	}
	virtual ~Panel() = default;
	virtual void drawUnique() const {};
	void update() {
		auto mousePointButton = [this]() {
			Vector2 mousePos = window.mousePosition();
			bool hovered = false;
			for (size_t i = 0; i < buttons.size(); i++) {
				const auto& button = buttons[i];
				if (mousePos.x >= +button->position.x && mousePos.x <= button->position.x + button->width &&
					mousePos.y >= button->position.y && mousePos.y <= button->position.y + button->height) {
					buttons[choosedIndex]->isSelected = false;
					choosedIndex = i;
					buttons[choosedIndex]->isSelected = true;
					hovered = true;
					break;
				}
			}
			return hovered;
			};
		if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
			buttons[choosedIndex]->onClick();
			return;
		}
		else if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			if (mousePointButton()) {
				buttons[choosedIndex]->onClick();
				return;
			}
		}
		if(isMouseMoving()) {
			mousePointButton();
		}
		else {
			Vector2 direction = { 0.0f, 0.0f };
			if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) direction.y -= 1.0f;
			if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) direction.y += 1.0f;
			if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) direction.x -= 1.0f;
			if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) direction.x += 1.0f;
			if ((direction.x != 0 || direction.y != 0) && buttons.size() > 1) {
				pressCooldown--;
				if (pressCooldown <= 0) {
					selectNext(direction);
					pressCooldown = DefaultPressCooldown;
				}
			}
			else pressCooldown = 0;
		}
	}
	void draw() const {
		DrawRectangle(position.x, position.y, width, height, backgroundColor);
		DrawRectangleLines(position.x, position.y, width, height, borderColor);
		drawUnique();
		for (const auto& button : buttons) {
			button->draw();
		}
	}
};
class LevelupPanel : public Panel {
public:
	static constexpr int _Width = 600;
	static constexpr int _Height = 580;
	static constexpr int TitleHeight = 64 ;
	static constexpr int PaddingLeft = 16;
	BaseItem* selectedItem = nullptr;
	class LevelupButton : public Button {
	public:
		//static constexpr int _width = 400;
		static constexpr int _height = 115;
		static constexpr Color _backgroundColor = { 150,150,150,255 };
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
		void drawUnique() const override {
			Vector2 pos = { static_cast<float>(position.x),static_cast<float>(position.y) };
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
		position.x = 1280 / 2 - width / 2;
		int buttonHeight = (height - TitleHeight) / 4;
		for (int i = 0; i < items.size(); i++) {
			if (items[i] == nullptr) break;
			IntVector2 buttonPosition = { position.x + PaddingLeft, position.y + TitleHeight + i * buttonHeight };
			auto button = std::make_unique<LevelupButton>(window, buttonPosition, i, items[i], width - PaddingLeft * 2);
			button->onClick = [this, item = button->item]() {
				selectedItem = item;
				};
			buttons.emplace_back(std::move(button));

		}
		buttons[choosedIndex]->isSelected = true;
	}
	BaseItem* takeSelectedItem() {
		return std::exchange(selectedItem, nullptr);
	}
	void drawUnique() const override {
		DrawTextEx(window.getFont(64), "    选择升级的物品", { static_cast<float>(position.x + PaddingLeft), static_cast<float>(position.y) }, 64, 0.0f, WHITE);
	}
};
class ButtonPanel : public Panel {
public:
	struct ButtonInfo {
		std::string text;
		IntVector2 position;
		int width;
		int height;
		int fontSize = 64;
		Color backgroundColor = GREEN;
		std::function<void()> onClick;
	};
private:
	class SimpleButton : public Button {
	private:
		std::string text;
		int fontSize;
	public:
		SimpleButton(const Window& _window, IntVector2 _position, int _width, int _height, Color _backgroundColor, std::string_view _text, int _fontSize) :
			Button(_window, _position, _width, _height, _backgroundColor), text(_text), fontSize(_fontSize) {
		}
		void drawUnique() const override {
			Vector2 pos = { static_cast<float>(position.x),static_cast<float>(position.y) };
			const Font& font = window.getFont(fontSize);
			DrawTextEx(font, text.c_str(), { pos.x + width / 2.0f - MeasureTextEx(font, text.c_str(), fontSize, 0.0f).x / 2.0f,pos.y + height / 2.0f - fontSize / 2.0f }, fontSize, 0.0f, BLACK);
		}
	};
public:
	ButtonPanel(const Window& _window, std::vector<ButtonInfo> buttonInfos) : Panel(_window) {
		backgroundColor = BLANK;
		borderColor = BLANK;
		position = { 0,0 };
		for (const auto& info : buttonInfos) {
			auto button = std::make_unique<SimpleButton>(window, info.position, info.width, info.height, info.backgroundColor, info.text, info.fontSize);
			button->onClick = info.onClick;
			buttons.emplace_back(std::move(button));
		}
		buttons[choosedIndex]->isSelected = true;
	}
};