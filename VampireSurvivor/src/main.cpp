#include "raylib.h"
#include "window.hpp"
#include "level.hpp"
int main() {
	Window window("Vampire Survivor");
	window.loadTemporaryTexture("background", "./resources/background.png");
	window.loadTemporaryTexture("1", "./resources/1.png");
	window.loadTemporaryTexture("2", "./resources/2.png");
	window.loadTemporaryTexture("3", "./resources/3.png");
	window.loadTemporaryTexture("4", "./resources/4.png");
	window.loadTemporaryTexture("whip_effect","./resources/img/whip_effect.png");
	window.loadTemporaryTexture("garlic_effect", "./resources/img/garlic_effect.png");
	window.loadSpriteSet("./resources/img/ai_monster.png", "./resources/img/ai_monster.tpsheet");
	window.loadSpriteSet("./resources/img/tjr.png", "./resources/img/tjr.tpsheet");
	Player player;
	GameLevel level(window, player);
	while(!WindowShouldClose()){
		level.update();
		level.drawLevel();
		

	}
}