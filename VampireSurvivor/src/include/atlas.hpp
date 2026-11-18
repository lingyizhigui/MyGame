#pragma once
#include "raylib.h"
#include "raymath.h"
#include "json.hpp"
#include "window.hpp"
class Atlas {
	using json = nlohmann::json;
private:
	const Window& window;
	int tileSize;
	IntVector2 centerIndex;
	struct SpriteInfo {
		std::string name;
		IntVector2 offset;
		const DrawFromSpriteSet* drawSet;
	};
	std::vector<SpriteInfo> spriteInfos;
	std::vector<std::vector<std::vector<size_t>>> grid;
public:
	Atlas(const Window& window, std::string jsonPath) : window(window) {
		std::fstream file(jsonPath);
		json jsonData;
		file >> jsonData;
		tileSize = jsonData["tile_size"];
		for (const auto& info : jsonData["tile_types"]) {
			IntVector2 offset;
			if(info.contains("draw_offset")) {
				offset = IntVector2{
					.x = info["draw_offset"]["x"],
					.y = info["draw_offset"]["y"]
				};
			}
			else {
				offset = IntVector2{ 0, 0 };
			}
			SpriteInfo spriteInfo{
				.name = info["filename"],
				.offset = offset,
				.drawSet = &window.getTexture(info["filename"]),
			};
			spriteInfos.push_back(spriteInfo);
		}
		for(const auto& row : jsonData["tiles"]) {
			std::vector<std::vector<size_t>> gridRow;
			for(const auto& cell : row) {
				std::vector<size_t> cellContent;
				for(const auto& index : cell) {
					cellContent.push_back(index);
				}
				cellContent.shrink_to_fit();
				gridRow.push_back(cellContent);
			}
			gridRow.shrink_to_fit();
			grid.push_back(gridRow);
		}
		centerIndex = { static_cast<int>(grid[0].size() / 2), static_cast<int>(grid.size() / 2) };
	}
	void draw(Vector2 position) const {
		Vector2 leftTop = Vector2Subtract(position, window.getCenter());
		int xIndex = static_cast<int>(floor(leftTop.x / tileSize));
		int yIndex = static_cast<int>(floor(leftTop.y / tileSize));
		int columns = window.getSize().x / tileSize + 1;
		int rows = window.getSize().y / tileSize + 2;
		for (int i = 0; i < columns; i++) {
			for (int j = 0; j < rows; j++) {
				int x = xIndex + i + centerIndex.x;
				int y = yIndex + j + centerIndex.y;
				int width = static_cast<int>(grid[0].size());
				int height = static_cast<int>(grid.size());
				x = (x % width + width) % width;
				y = (y % height + height) % height;
				const auto& cell = grid[y][x];
				for (const auto& index : cell) {
					const auto& spriteInfo = spriteInfos[index];
					Rectangle drawRect = {
						(xIndex + i) * static_cast<float>(tileSize) + spriteInfo.offset.x,
						(yIndex + j) * static_cast<float>(tileSize) + spriteInfo.offset.y,
						static_cast<float>(tileSize),
						static_cast<float>(tileSize) };
					DrawTexturePro(*spriteInfo.drawSet->sprite, spriteInfo.drawSet->source, drawRect, Vector2{ 0,0 }, 0, WHITE);
				}
			}
		}
	}
};