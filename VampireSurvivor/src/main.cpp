#include "raylib.h"
#include "window.hpp"
#include "level.hpp"
int main() {
	Window window("Vampire Survivor");
	window.loadTemporaryTexture("skull_32x32", "./resources/skull_32x32.png");
	window.loadTemporaryTexture("coin_32x32", "./resources/coin_32x32.png");
	window.loadTemporaryTexture("1", "./resources/1.png");
	window.loadTemporaryTexture("2", "./resources/2.png");
	window.loadTemporaryTexture("3", "./resources/3.png");
	window.loadTemporaryTexture("4", "./resources/4.png");
	window.loadSpriteSet("./resources/img/weapon_effect.png", "./resources/img/weapon_effect.tpsheet");
	window.loadSpriteSet("./resources/img/ai_monster.png", "./resources/img/ai_monster.tpsheet");
	window.loadSpriteSet("./resources/img/tjr.png", "./resources/img/tjr.tpsheet");
	window.loadSpriteSet("./resources/img/pick_up.png", "./resources/img/pick_up.tpsheet");
	window.loadSpriteSet("./resources/img/icons.png", "./resources/img/icons.tpsheet");
	window.loadSpriteSet("./resources/img/forest_atlas.png", "./resources/img/forest_atlas.tpsheet");
	Player player;
	GameLevel level(window, player);
	while(!WindowShouldClose()){
		level.update();
		if(IsKeyPressed(KEY_F11)) {
			ToggleBorderlessWindowed();
		}
		window.beginDrawing();
		level.drawLevel();
		window.endDrawing();
	}
}