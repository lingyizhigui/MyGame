#include "level.h"
#include "elements.h"
#include "framework.h"
#include "raylib.h"
#include "raymath.h"
#include <array>
#include <queue>
#include <iostream>
int main() {
	Window window("Brick Breaker", 60);
	GameLevel levelOne(window);
	int hsv = 0;
	
	while (!WindowShouldClose()) {
		hsv = (hsv + 2) % 360;
		window.backgroundMusic->update();
		
		if (IsKeyPressed(KEY_F)) {
			window.fps == 60 ? window.setFps(120) : window.setFps(60);
		}
		if (IsKeyPressed(KEY_B)) {
			ToggleBorderlessWindowed();
		}
		levelOne.update();

		BeginDrawing();
		//ClearBackground(RAYWHITE);
		levelOne.basicDraw();
		levelOne.drawBricks();
		levelOne.drawPaddle();
		levelOne.drawParticles();
		levelOne.drawBalls();
		levelOne.drawTexts();
		EndDrawing();
	}
	
	return 0;
}