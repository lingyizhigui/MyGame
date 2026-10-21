#pragma once
#include "raylib.h"
#include "player.hpp"
#include "enemy.hpp"
#include "weapon.hpp"
#include "pickup.hpp"
#include "window.hpp"
#include "ui.hpp"
#include "raymath.h"
class GameLevel {
	static constexpr int ENEMY_WAVE_COUNT = 20;
	const Window& window;
	EnemyGenerator generator;

	int gameTime = 0;
	uint64_t enemyIdCounter = 0;
	std::vector<Enemy> enemyList;
	//std::vector<Enemy*> sortedEnemyList;	//指针数组，按y排序便于绘制
	std::vector<std::unique_ptr<BaseWeapon>> allWeapons;
	std::vector<std::unique_ptr<PassiveItem>> allPassives;
	std::vector<std::unique_ptr<Pickup>> pickupList;
	std::unique_ptr<Panel> panel = nullptr;
	int levelupCount = 0;
	Player& player;
	Camera2D camera;
	static constexpr int GRID_SIZE = 128;
	enum class GameState {
		Playing,
		paused,
		choosing,
		GameOver
	} gameState = GameState::Playing;
	struct CollideGrid {
		Vector2 PosLeftTop;
		std::vector<Enemy*> enemies;
	};
	std::array<std::array<CollideGrid, 12>, 8> collideGrid;
	//遍历周围的格子
	void forNearGrids(int _x, int _y, std::function<void(Enemy&)> func,bool _skipCurrentGrid = false) {
		for (size_t i = std::max(0, _y - 1); i <= std::min(static_cast<int>(collideGrid.size() - 1), _y + 1); i++) {
			for (size_t j = std::max(0, _x - 1); j <= std::min(static_cast<int>(collideGrid[i].size() - 1), _x + 1); j++) {
				if(_skipCurrentGrid) {
					if(i==_y && j==_x) continue;
				}
				for(auto& enemyPtr : collideGrid[i][j].enemies){
					func(*enemyPtr);
				}
			}
		}
	}
	//矩形版本遍历格子
	void forNearGrids(const Rectangle& rect, std::function<void(Enemy&)> func) {
		int x0 = static_cast<int>(rect.x) / GRID_SIZE + 1;
		int y0 = static_cast<int>(rect.y) / GRID_SIZE + 1;
		int x1 = static_cast<int>(rect.x + rect.width) / GRID_SIZE + 1;
		int y1 = static_cast<int>(rect.y + rect.height) / GRID_SIZE + 1;
		for (int i = std::max(0, y0 - 1); i <= std::min(static_cast<int>(collideGrid.size() - 1), y1 + 1); i++) {
			for (int j = std::max(0, x0 - 1); j <= std::min(static_cast<int>(collideGrid[i].size() - 1), x1 + 1); j++) {
				for(auto& enemyPtr : collideGrid[i][j].enemies){
					func(*enemyPtr);
				}
			}
		}
	}
	//遍历→↘↓↙四个相邻格子
	void forRightDownGrids(int _x, int _y, std::function<void(Enemy&)> func) {
		if(_x + 1 < collideGrid[_y].size()) {
			for(auto& enemyPtr : collideGrid[_y][_x + 1].enemies){
				func(*enemyPtr);
			}
		}
		if(_y + 1 < collideGrid.size()) {
			for(int j = std::max(_x-1,0); j <= std::min(_x + 1, static_cast<int>(collideGrid[_y + 1].size()) - 1); j++) {
				for(auto& enemyPtr : collideGrid[_y + 1][j].enemies){
					func(*enemyPtr);
				}
			}
		}
	}
	//怪物死亡事件
	void onEnemyDeath(const Enemy& enemy) {
		int expDrop = enemy.def->expDrop;
		pickupList.emplace_back(std::make_unique<ExperienceGem>(enemy.position, expDrop));//掉落经验宝石
	}
	//升级抽取
	std::array<BaseItem*, 4> selectLevelup(char itemCount) {
		constexpr int MaxWeaponCount = 6;
		constexpr int MaxPassiveCount = 6;
		std::array<BaseItem*, 4> forPanel = { nullptr };
		std::vector<BaseItem*> canLevelupWeapons;
		std::vector<BaseItem*> canLevelupPassives;
		std::vector<BaseItem*> canLevelupOwns;
		std::vector<BaseItem*> ownedWeapons;
		std::vector<BaseItem*> ownedPassives;
		for (const auto& weapon : allWeapons) {
			if (weapon->canLevelup()) {
				canLevelupWeapons.push_back(weapon.get());
			}
			if (weapon->getLevel() > 0) {
				ownedWeapons.push_back(weapon.get());
			}
			if (weapon->getLevel() > 0 && weapon->canLevelup()) {
				canLevelupOwns.push_back(weapon.get());
			}
		}
		for (const auto& passive : allPassives) {
			if (passive->canLevelup()) {
				canLevelupPassives.push_back(passive.get());
			}
			if (passive->getLevel() > 0) {
				ownedPassives.push_back(passive.get());
			}
			if (passive->getLevel() > 0 && passive->canLevelup()) {
				canLevelupOwns.push_back(passive.get());
			}
		}
		if (ownedWeapons.size() >= MaxWeaponCount) {
			std::erase_if(canLevelupWeapons, [](BaseItem* item) { return item->getLevel() == 0; });
		}
		if (ownedPassives.size() >= MaxPassiveCount) {
			std::erase_if(canLevelupPassives, [](BaseItem* item) { return item->getLevel() == 0; });
		}
		for (char i = 0; i < itemCount; i++) {
			if (canLevelupPassives.empty() && canLevelupWeapons.empty()) {
				break;
			}
			//算法源自原版，前两抽优先已获得物品
			if (i == 0 || i == 1) {
				if (!canLevelupOwns.empty()) {
					double probability = 1 - 1.0 / player.getStats().luck + 0.3 * (1 + player.getLevel() % 2);
					probability = std::clamp(probability, 0.0, 1.0);
					if (bernoulli(probability)) {
						BaseItem* ownedTry = canLevelupOwns[GetRandomValue(0, canLevelupOwns.size() - 1)];
						forPanel[i] = ownedTry;
						std::erase(canLevelupOwns, ownedTry);
						std::erase(canLevelupWeapons, ownedTry);
						std::erase(canLevelupPassives, ownedTry);
						continue;
					}
				}
			}
			//如没有抽到或是后两抽，则从全部池中抽取
			BaseItem* selectedItem = nullptr;
			if (!canLevelupWeapons.empty() && (canLevelupPassives.empty() || GetRandomValue(0, 1) == 0))
				selectedItem = canLevelupWeapons[GetRandomValue(0, canLevelupWeapons.size() - 1)];
			else
				selectedItem = canLevelupPassives[GetRandomValue(0, canLevelupPassives.size() - 1)];
			forPanel[i] = selectedItem;
			std::erase(canLevelupWeapons, selectedItem);
			std::erase(canLevelupPassives, selectedItem);
			std::erase(canLevelupOwns, selectedItem);
		}
		return forPanel;
	}
public:
	GameLevel(const Window& _window, Player& _player) : window(_window), generator("./resources/enemies.json"), player(_player),
		camera({
			window.getCenter(),// offset
			{0,0},// target
			0.0f,// rotation
			1.0f// zoom
			}){
		enemyList.reserve(100);
		//weaponList.emplace_back(std::make_unique<WeaponGarlic>(window, player.getStats(), player.getPosition(), player.getTowards()));
		//weaponList.emplace_back(std::make_unique<WeaponWhip>(window, player.getStats(), player.getPosition(), player.getTowards()));
		for(int i=0;i<collideGrid.size();i++){
			for(int j=0;j<collideGrid[i].size();j++){
				collideGrid[i][j].PosLeftTop = { float((j - 1) * GRID_SIZE), float((i - 1) * GRID_SIZE) };
			}
		}
		//初始化全部武器，主动和被动
		auto& weaponGen = getWeaponRegisterPool();
		for(auto& createFunc: weaponGen) {
			allWeapons.emplace_back(createFunc(window, player.getStats(), player.getPosition(), player.getTowards()));
			allWeapons.back()->levelup();	//初始化为1级
		}
		for(int i=0;i<static_cast<int>(PlayerStatsEnum::SizeOfEnum);i++){
			PlayerStatsEnum kind = static_cast<PlayerStatsEnum>(i);
			allPassives.emplace_back(std::make_unique<PassiveItem>(window, kind, player.getStats()));
		}
	}
	void update() {
		if(GameState::GameOver == gameState) {
			return;
		}
		if(gameState == GameState::paused) {
			if(IsKeyPressed(KEY_ESCAPE)) {
				gameState = GameState::Playing;
			}
			return;
		}
		if(gameState == GameState::choosing) {
			LevelupPanel* levelupPanel = static_cast<LevelupPanel*>(panel.get());
			BaseItem* weapon = levelupPanel->update();
			if (weapon != nullptr) {
				weapon->levelup();
				panel.reset();
				gameState = GameState::Playing;
			}
			return;
		}
		if(IsKeyPressed(KEY_ESCAPE)) {
			gameState = GameState::paused;
			return;
		}
		gameTime++;
		//怪物波次生成
		if (IsKeyPressed(KEY_E)) {
			for (int enemySpawn = 0; enemySpawn < ENEMY_WAVE_COUNT; enemySpawn++) {
				static constexpr int area = 64;
				int gameWidth = window.getCanvasSize().x;
				int gameHeight = window.getCanvasSize().y;
				int r = GetRandomValue(0, gameWidth * 3 + gameHeight * 3);
				int x = 0;
				int y = 0;
				if (r < gameHeight * 1.5f) {
					x = player.getPosition().x - gameWidth + GetRandomValue(-area, area);
					y = player.getPosition().y - gameHeight + r;
				}
				else if (r < gameHeight * 1.5f + gameWidth * 1.5f) {
					x = player.getPosition().x - gameWidth + r - gameHeight * 1.5;
					y = player.getPosition().y + gameHeight + GetRandomValue(-area, area);
				}
				else if (r < gameHeight * 3 + gameWidth * 1.5f) {
					x = player.getPosition().x + gameWidth + GetRandomValue(-area, area);
					y = player.getPosition().y - gameHeight + r - gameHeight * 1.5 - gameWidth * 1.5;
				}
				else {
					x = player.getPosition().x - gameWidth + r - gameHeight * 3 - gameWidth * 1.5f;
					y = player.getPosition().y - gameHeight + GetRandomValue(-area, area);
				}
				enemyList.push_back(generator.spawn("basalt_golem", { float(x),float(y) }, enemyIdCounter++));
			}
		}
		//怪物前进、更新动画
		std::erase_if(enemyList, [](const Enemy& enemy) { return enemy.status == Enemy::Status::dead; });//删除死亡怪物
		for (Enemy& enemy : enemyList) {
			if (enemy.status == Enemy::Status::alive) {
				if (enemy.changeTime > 15) {
					enemy.changeTime = 0;
					enemy.frame = (enemy.frame + 1) % enemy.def->moveFrames.size();
				}
				else {
					enemy.changeTime++;
				}
				Vector2 direction = { player.getPosition().x - enemy.position.x, player.getPosition().y - enemy.position.y };
				direction = Vector2Normalize(direction) * enemy.def->speed;
				{
					direction = direction * GetRandomValue(90, 110) / 100.0f;
				}
				enemy.direction = direction;
				enemy.position = Vector2Add(enemy.position, direction);
				if (direction.x > 0.0f) {
					enemy.towards = true;
				}
				else if (direction.x < 0.0f) {
					enemy.towards = false;
				}
			}
			else if (enemy.status == Enemy::Status::dying) {	//dying
				if (enemy.changeTime > 2) {
					enemy.changeTime = 0;
					enemy.frame++;
				}
				else {
					enemy.changeTime++;
				}
				if (enemy.frame == enemy.def->deathFrames.size()) {
					enemy.status = Enemy::Status::dead;
				}
			}
		}
		//每帧重建碰撞格子
		for (auto& row : collideGrid) {
			for (auto& cell : row) {
				cell.enemies.clear();
			}
		}
		for (auto& enemy : enemyList) {
			if (enemy.status != Enemy::Status::alive) continue;
			Vector2 screenPos = GetWorldToScreen2D(enemy.position, camera);
			for (int i = 0; i < collideGrid.size(); i++) {
				for (int j = 0; j < collideGrid[i].size(); j++) {
					Vector2 leftTop = collideGrid[i][j].PosLeftTop;
					if (screenPos.x >= leftTop.x && screenPos.x < leftTop.x + GRID_SIZE &&
						screenPos.y >= leftTop.y && screenPos.y < leftTop.y + GRID_SIZE) {
						collideGrid[i][j].enemies.push_back(&enemy);
					}
				}
			}
		}
		//敌人碰撞
		for (auto& gridArray : collideGrid) {
			for (auto& grid : gridArray) {
				for (int i = 0; i < grid.enemies.size(); i++) {
					Enemy& enemy = *grid.enemies[i];
					if (enemy.status != Enemy::Status::alive) continue;
					auto collideWith = [&enemy](Enemy& otherEnemy) {
						if (otherEnemy.status != Enemy::Status::alive) return;
						if (CheckCollisionCircles(enemy.position, enemy.def->collideRadius, otherEnemy.position, otherEnemy.def->collideRadius)) {
							Vector2 direction = { enemy.position.x - otherEnemy.position.x, enemy.position.y - otherEnemy.position.y };
							direction = Vector2Normalize(direction);
							enemy.position = Vector2Add(enemy.position, direction * 2.0f);
							otherEnemy.position = Vector2Add(otherEnemy.position, direction * -2.0f);
						}
						};
					//本块内碰撞
					for (int j = i + 1; j < grid.enemies.size(); j++) {
						collideWith(*grid.enemies[j]);
					}
					//相邻格子碰撞
					forRightDownGrids(static_cast<int>(&grid - &gridArray[0]), static_cast<int>(&gridArray - &collideGrid[0]),
						collideWith);
				}
			}
		}
		//掉落物更新，升级事件也在这里
		PickupResult result;
		for (auto& pickup : pickupList) {
			const PickupResult res = pickup->update(player);
			if (res.playerLevelupCount > 0) {
				result.playerLevelupCount += res.playerLevelupCount;
			}
		}
		std::erase_if(pickupList, [](const std::unique_ptr<Pickup>& pickup) { return !pickup->isAlive(); });
		if(result.playerLevelupCount > 0) {
			levelupCount += result.playerLevelupCount;
			char choosenCount = 3;
			double chanceToGetFourth = 1.0 - 1.0 / player.getStats().luck;
			if (bernoulli(chanceToGetFourth)) choosenCount += 1;
			panel = std::make_unique<LevelupPanel>(selectLevelup(choosenCount));
			gameState = GameState::choosing;
		}
		//玩家更新，检查受伤与自身更新
		float hurtDemage = 0.0f;
		for(int i=0;i<collideGrid.size();i++){
			for(int j=0;j<collideGrid[i].size();j++){
				Vector2 leftTop = collideGrid[i][j].PosLeftTop;
				Vector2 playerPosition = GetWorldToScreen2D(player.getPosition(), camera);//2D坐标转换
				if (playerPosition.x >= leftTop.x && playerPosition.x < leftTop.x + GRID_SIZE &&
					playerPosition.y >= leftTop.y && playerPosition.y < leftTop.y + GRID_SIZE) {
					forNearGrids(j, i,
						[&](Enemy& enemy) {
							if (enemy.status != Enemy::Status::alive) return;
							if (CheckCollisionCircles(player.getPosition(), player.collisionRadius, enemy.position, enemy.def->collideRadius)) {
								hurtDemage = std::max(hurtDemage, enemy.def->power);
								enemy.position = Vector2Add(enemy.position, Vector2Scale(enemy.direction, -10.0f));//推开怪物
								}
							}
						);
				}
			}
		}
		player.update(hurtDemage);
		camera.target = player.getPosition();
		//武器攻击
		for(auto& weapon : allWeapons){
			if (weapon->getLevel() == 0) continue;
			auto& projectiles = weapon->update();
			for(auto& projectile : projectiles){
				Vector2 projectilePosition = GetWorldToScreen2D({ projectile->rect.x,projectile->rect.y }, camera);//2D坐标转换
				Rectangle projectileRect = { projectilePosition.x,projectilePosition.y,projectile->rect.width,projectile->rect.height };
				forNearGrids(projectileRect,
					[&projectile,this](Enemy& enemy) {
						if (enemy.status != Enemy::Status::alive) return;
						if (projectile->checkCollision(enemy)) {
							if(projectile->attack(enemy)){
								onEnemyDeath(enemy);
							}
						}
					});
			}
		}
		//按y坐标排序，保证绘制顺序正确
		std::sort(enemyList.begin(), enemyList.end(), [](const Enemy& a, const Enemy& b) {
			return a.position.y < b.position.y;
			});
	}
	void drawLevel() const {
		BeginMode2D(camera);
		//draw background
		Vector2 leftTop = GetScreenToWorld2D({ 0,0 }, camera);
		int xStart = (int)floor(leftTop.x / 64) * 64;
		int yStart = (int)floor(leftTop.y / 64) * 64;
		int columns = window.getCanvasSize().x / 64 + 1;
		int rows = window.getCanvasSize().y / 64 + 2;
		for (int i = 0; i < columns; i++) {
			for (int j = 0; j < rows; j++) {
				auto drawSet = window.getTexture("background");
				DrawTextureRec(*drawSet.sprite,
					drawSet.source,
					Vector2(xStart + i * 64, yStart + j * 64),
					WHITE);
			}
		}
		//draw pickups
		for(const auto& pickup : pickupList){
			pickup->draw(window);
		}
		//draw enemies
		for (const auto& enemy : enemyList) {
			if (enemy.status == Enemy::Status::dead) continue;
			Texture2D texture;
			Rectangle source;
			if(enemy.status==Enemy::Status::alive){
				texture = *window.getTexture(enemy.def->moveFrames[enemy.frame]).sprite;
				source = window.getTexture(enemy.def->moveFrames[enemy.frame]).source;
			}
			else if(enemy.status==Enemy::Status::dying){
				texture = *window.getTexture(enemy.def->deathFrames[enemy.frame]).sprite;
				source = window.getTexture(enemy.def->deathFrames[enemy.frame]).source;
			}
			if (!enemy.towards) {
				source.width = -source.width;
			}
			float drawingHeight = enemy.def->height * 1.0f;
			float drawingWidth = enemy.def->width * 1.0f;
			DrawTexturePro(texture, source,
				{ enemy.position.x , enemy.position.y,drawingWidth,drawingHeight },
				{ drawingWidth / 2.0f	 ,drawingHeight },
				0.0f,
				WHITE);
		}
		//draw player
		player.drawPlayer(window);
		player.drawHealthBar();
		//draw weapons
		for (const auto& weapon : allWeapons) {
			if (weapon->getLevel() == 0) continue;
			weapon->draw();
		}
		EndMode2D();
		//over
		//draw level information
		DrawFPS(0, 700);
		//游戏时钟
		{
			int seconds = gameTime / 60;
			int minutes = seconds / 60;
			seconds = seconds % 60;
			std::string time = std::format("{:02d} : {:02d}", minutes, seconds);
			const int timeTopMargin = 40;
			const int timeHeight = 64;
			DrawTextEx(window.getFont(timeHeight),
				time.c_str(),
				{ 1280 / 2.0f - MeasureTextEx(window.getFont(timeHeight), time.c_str(), timeHeight, 0.0f).x / 2.0f, static_cast<float>(timeTopMargin) },
				timeHeight, 0.0f, WHITE);
		}
		//绘制经验条
		player.drawExpBar(window);//我也不知道为啥放进player里了
		//绘制面板
		if(panel!=nullptr){
			panel->draw();
		}
	}
};