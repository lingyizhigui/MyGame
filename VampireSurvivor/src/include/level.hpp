#pragma once
#include "raylib.h"
#include "player.hpp"
#include "enemy.hpp"
#include "weapon.hpp"
#include "window.hpp"
#include "raymath.h"
class GameLevel {
	const Window& window;
	EnemyGenerator generator;
	std::vector<Enemy> enemyList;
	std::vector<std::unique_ptr<BaseWeapon>> weaponList;
	Player& player;
	Camera2D camera;
public:
	GameLevel(const Window& _window, Player& _player) : window(_window), generator("./resources/enemies.json"), player(_player),
		camera({
			window.getCenter(),// offset
			{0,0},// target
			0.0f,// rotation
			1.0f// zoom
			})
	{
		enemyList.reserve(100);
		enemyList.push_back(generator.spawn("basalt_golem"));
		weaponList.emplace_back(std::make_unique<WeaponWhip>(window, player.getStats(), player.getPosition(), player.getTowards()));
	}
	void update() {
		if(IsKeyPressed(KEY_SPACE)){
			enemyList.push_back(generator.spawn("basalt_golem"));
		}
		player.update();
		for(auto& weapon : weaponList){
			weapon->update(enemyList);
		}
		camera.target = player.getPosition();
		for(Enemy& enemy : enemyList){
			if (enemy.status == 0) {
				enemy = enemyList.back();
				enemyList.pop_back();
				continue;
			}
			if (enemy.status == 1) {
				if(enemy.changeTime > 15){
					enemy.changeTime = 0;
					enemy.frame = (enemy.frame + 1) % enemy.def->moveFrames.size();
				}
				else{
					enemy.changeTime++;
				}
				Vector2 direction = { player.getPosition().x - enemy.position.x, player.getPosition().y - enemy.position.y };
				direction = Vector2Normalize(direction) * enemy.def->speed;
				{
					direction = direction * GetRandomValue(90, 110) / 100.0f;
				}
				enemy.position = Vector2Add(enemy.position, direction);
				if(direction.x > 0.0f){
					enemy.towards = true;
				}
				else if(direction.x < 0.0f){
					enemy.towards = false;
				}
			}
			else if(enemy.status == 2){	//dying
				if(enemy.changeTime > 2){
					enemy.changeTime = 0;
					enemy.frame++;
				}
				else{
					enemy.changeTime++;
				}
				if(enemy.frame == enemy.def->deathFrames.size()){
					enemy.status = 0;
				}
			}
		}
	}
	void drawLevel() const {
		BeginDrawing();
		ClearBackground(RAYWHITE);
		//draw level imformation
		DrawFPS(0, 0);
		DrawText(TextFormat("Player: %.1f, %.1f",
			player.getPosition().x, player.getPosition().y),
			10, 30, 20, RED);
		//
		BeginMode2D(camera);
		//draw background
		Vector2 leftTop = GetScreenToWorld2D({ 0,0 }, camera);
		int xStart = (int)floor(leftTop.x / 64) * 64;
		int yStart = (int)floor(leftTop.y / 64) * 64;
		int columns = window.getCenter().x * 2 / 64 + 1;
		int rows = window.getCenter().y * 2 / 64 + 1;
		for (int i = 0; i < columns; i++) {
			for (int j = 0; j < rows; j++) {
				auto drawSet = window.getTexture("background");
				DrawTextureRec(*drawSet.sprite,
					drawSet.source,
					Vector2(xStart + i * 64, yStart + j * 64),
					WHITE);
			}
		}
		//
		//draw enemies
		for (const auto& enemy : enemyList) {
			if (enemy.status == 0) continue;
			Texture2D texture;
			Rectangle source;
			if(enemy.status==1){
				texture = *window.getTexture(enemy.def->moveFrames[enemy.frame]).sprite;
				source = window.getTexture(enemy.def->moveFrames[enemy.frame]).source;
			}
			else if(enemy.status==2){
				texture = *window.getTexture(enemy.def->deathFrames[enemy.frame]).sprite;
				source = window.getTexture(enemy.def->deathFrames[enemy.frame]).source;
			}
			if (!enemy.towards) {
				source.width = -source.width;
			}
			float drawingHeight = enemy.def->height * 2.0f;
			float drawingWidth = enemy.def->width * 2.0f;
			DrawTexturePro(texture, source,
				{ enemy.position.x , enemy.position.y,drawingWidth,drawingHeight },
				{ drawingWidth / 2.0f	 ,drawingHeight },
				0.0f,
				WHITE);
		}
		//
		//draw player
		player.drawPlayer(window);
		//draw weapons
		for (const auto& weapon : weaponList) {
			weapon->draw();
		}
		//
		EndMode2D();
		EndDrawing();
	}
};