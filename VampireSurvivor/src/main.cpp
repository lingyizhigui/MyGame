#include "raylib.h"
#include "window.hpp"
#include "player.hpp"
int main() {
	Window window("Vampire Survivor");
	window.loadTexture("background", "./resources/background.png");
	window.loadTexture("1", "./resources/1.png");
	window.loadTexture("2", "./resources/2.png");
	window.loadTexture("3", "./resources/3.png");
	window.loadTexture("4", "./resources/4.png");
	Camera2D camera({
		window.getCenter(),// offset
		{0,0},// target
		0.0f,// rotation
		1.0f// zoom
		});
	Player player;
	while(!WindowShouldClose()){
		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawFPS(0, 0);
		player.update();
		camera.target = player.getPosition();
		DrawText(TextFormat("Player: %.1f, %.1f",
			player.getPosition().x, player.getPosition().y),
			10, 30, 20, RED);
		BeginMode2D(camera);
		Vector2 leftTop = GetScreenToWorld2D({ 0,0 }, camera);
		int xStart = (int)floor(leftTop.x / 64) * 64 ;
		int yStart = (int)floor(leftTop.y / 64) * 64 ;
		int columns = window.getCenter().x * 2 / 64 + 1;
		int rows = window.getCenter().y * 2 / 64 + 1;
		for(int i=0;i<columns;i++){
			for(int j=0;j<rows;j++){
				DrawTexture(window.getTexture("background"), xStart + i * 64, yStart + j * 64, WHITE);
			}
		}
		Texture2D texture = window.getTexture(player.getAnimationFrame());
		Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
		if (!player.towards) {
			source.width = -source.width;
		}
		// 注意：DrawTextureRec 的位置参数是源矩形的左上角，不是中心
		DrawTextureRec(texture, source, { player.getPosition().x-64,player.getPosition().y - 128 }, WHITE);
		EndMode2D();
		EndDrawing();
	}
}