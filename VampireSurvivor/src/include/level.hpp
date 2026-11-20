#pragma once
#include "raylib.h"
#include "player.hpp"
#include "enemy.hpp"
#include "weapon.hpp"
#include "passive.hpp"
#include "pickup.hpp"
#include "window.hpp"
#include "ui.hpp"
#include "atlas.hpp"
#include <deque>
struct SpawnTimerData {
	int waveTimeSecond;
	std::string enemyName;
	int targetAmount;
	int defaultIntervalFrame;
};
static const std::vector<SpawnTimerData> spawnTimerData = {
	{ 60, "small_bat", 15,60 },
	{ 120, "reed_slime", 30,60 },
	{ 180, "copper_scarab", 50,60 },
	{ 240, "lantern_wisp", 50,60 },
	{ 300, "thorn_shroom", 30,60 },
	{ 360, "bone_hound", 50,60 },
	{ 420, "basalt_golem", 50,60 }
};
struct DemageNumber {
	static constexpr int Duration = 40;
	static constexpr int EmergenceTime = 10;
	static constexpr int fontSize = 25;
	int time = 0;
	Vector2 position;
	int demage;
	AttackResult::AttackType type;
	DemageNumber(const Vector2& _position, int _demage, AttackResult::AttackType _type) : 
		position(_position), demage(_demage), type(_type) {
	};
};
class GameLevel {
	static constexpr int ENEMY_WAVE_COUNT = 20;
	const Window& window;
	EnemyGenerator generator;
	Player& player;

	Atlas atlas;
	int gameTime = 0;
	uint64_t enemyIdCounter = 0;
	std::vector<Enemy> enemyList;
	int existingEnemyCount = 0;
	int killedEnemyCount = 0;
	const float& goldGain = player.getCoin();
	std::vector<std::unique_ptr<BaseWeapon>> allWeapons;
	std::vector<BaseWeapon*> ownedWeapons;
	std::vector<std::unique_ptr<PassiveItem>> allPassives;
	std::vector<PassiveItem*> ownedPassives;
	std::array<std::unique_ptr<BaseItem>, 2> lastLevelupItems = {
		std::make_unique<levelupItem>(window, levelupItem::Type::Chicken, player),
		std::make_unique<levelupItem>(window, levelupItem::Type::CoinBag, player)
	};
	std::vector<std::unique_ptr<FloatPickup>> pickupList;
	static constexpr int MAX_LIGHT_SOURCE_COUNT = 8;
	std::deque<std::unique_ptr<Enemy>> lightSources;
	std::unique_ptr<Panel> panel = nullptr;
	std::vector<DemageNumber> demageNumbers;
	static constexpr int GRID_SIZE = 128;
	enum class GameState {
		Playing,
		paused,
		choosing,
		GameOver
	} gameState = GameState::Playing;
	//Camera相关
	Camera2D camera;
	Camera2D getUICamera() const {
		auto [scale, offset] = window.getViewTransform();
		Camera2D uiCamera = {
			.offset = offset,
			.target = {0.0f, 0.0f},
			.rotation = 0.0f,
			.zoom = scale
		};
		return uiCamera;
	}
	Camera2D getGameCamera() const {
		auto [scale, offset] = window.getViewTransform();
		Camera2D gameCamera = camera;
		gameCamera.offset.x = gameCamera.offset.x * scale + offset.x;
		gameCamera.offset.y = gameCamera.offset.y * scale + offset.y;
		gameCamera.zoom *= scale;
		return gameCamera;
	}
	//碰撞网格
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
	//检测是否在屏幕内
	bool isInScreen(const Enemy& enemy) const {
		return
			enemy.position.x > player.getPosition().x - window.getCenter().x - enemy.def->width / 2.0f &&
			enemy.position.x < player.getPosition().x + window.getCenter().x + enemy.def->width / 2.0f &&
			enemy.position.y > player.getPosition().y - window.getCenter().y &&
			enemy.position.y < player.getPosition().y + window.getCenter().y + enemy.def->height;
	}
	//怪物死亡事件
	void onEnemyDeath(const Enemy& enemy) {
		if(enemy.attackType == Enemy::AttackType::Normal){
			existingEnemyCount--;
			killedEnemyCount++;
			int expDrop = enemy.def->expDrop;
			pickupList.emplace_back(std::make_unique<ExperienceGem>(enemy.position, expDrop));//掉落经验宝石
		}
		else if(enemy.attackType == Enemy::AttackType::LightSource) {
			auto type = Pickup::getRandomType(player.getLevel(), player.getStats().luck);
			pickupList.emplace_back(std::make_unique<Pickup>(type, enemy.position));
		}
	}
	//升级抽取
	std::array<BaseItem*, 4> selectLevelup() {
		//高幸运有机会获得第四个选项
		char itemCount = 3;
		double chanceToGetFourth = 1.0 - 1.0 / player.getStats().luck;
		if (bernoulli(chanceToGetFourth)) itemCount += 1;
		constexpr int MaxWeaponCount = 6;
		constexpr int MaxPassiveCount = 6;
		std::array<BaseItem*, 4> forPanel = { nullptr };
		std::vector<BaseItem*> canLevelupWeapons;
		std::vector<BaseItem*> canLevelupPassives;
		std::vector<BaseItem*> canLevelupOwns;
		for (const auto& weapon : allWeapons) {
			if (weapon->canLevelup()) {
				canLevelupWeapons.push_back(weapon.get());
				if (weapon->getLevel() > 0) {
					canLevelupOwns.push_back(weapon.get());
				}
			}
		}
		for (const auto& passive : allPassives) {
			if (passive->canLevelup()) {
				canLevelupPassives.push_back(passive.get());
				if (passive->getLevel() > 0) {
					canLevelupOwns.push_back(passive.get());
				}
			}
		}
		if (ownedWeapons.size() >= MaxWeaponCount) {
			std::erase_if(canLevelupWeapons, [](BaseItem* item) { return item->getLevel() == 0; });
		}
		if (ownedPassives.size() >= MaxPassiveCount) {
			std::erase_if(canLevelupPassives, [](BaseItem* item) { return item->getLevel() == 0; });
		}
		//如果无可升级物品
		if(canLevelupWeapons.empty() && canLevelupPassives.empty()) {
			forPanel[0] = lastLevelupItems[0].get();
			forPanel[1] = lastLevelupItems[1].get();
			return forPanel;
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
	//武器更新注入函数
	//获得最近的敌人相对位置
	int nearestLastCheckTime = 0;
	Vector2 nearestDirection = { 0.0f, 0.0f };
	Vector2 getNearestEnemyDirection() {
		if(gameTime - nearestLastCheckTime < 5) {
			return nearestDirection;
		}
		nearestDirection = { 0.0f, 0.0f };
		nearestLastCheckTime = gameTime;
		float nearestDistance = std::numeric_limits<float>::infinity();
		for (const Enemy& enemy : enemyList) {
			if (enemy.status != Enemy::Status::alive) continue;
			float distance = Vector2DistanceSqr(player.getPosition(), enemy.position);
			if (distance < nearestDistance) {
				nearestDistance = distance;
				nearestDirection = Vector2Subtract(enemy.position, player.getPosition());
			}
		}
		return nearestDirection;
	}
	//获得玩家移动方向，向量长度为移动距离
	Vector2 lastPlayerPosition = { 0.0f, 0.0f };
	Vector2 getPlayerTowards() {
		Vector2 towards = Vector2Subtract(player.getPosition(), lastPlayerPosition);
		if(towards == Vector2{0.0f, 0.0f}) {
			towards = player.getTowards() ? Vector2{1e-6f, 0.0f} : Vector2{-1e-6f, 0.0f};
		}
		lastPlayerPosition = player.getPosition();
		return towards;
	}
	const WeaponUpdateFunc weaponUpdateFuncs = {
	.getNearestEnemyDirection = [this]() {return getNearestEnemyDirection(); },
	.getPlayerTowards = [this]() {return getPlayerTowards(); }
	};
	//掉落物拾取结果处理
	HiddenWeaponFireBreath fireBreath{ window, player.getStats(), player.getPosition(), player.getTowards() };
	int timeFreezeTime = 0;
	int levelupCount = 0;
	void onPickup(const PickupResult& result) {
		if (result.playerLevelupCount > 0) {
			levelupCount += result.playerLevelupCount;
		}
		if(result.timeFreezeTime > 0) {
			timeFreezeTime += result.timeFreezeTime;
		}
		if(result.fireBreathTime > 0) {
			fireBreath.getFireBreath(result.fireBreathTime);
		}
		if(result.rosaryKill) {
			for (Enemy& enemy : enemyList) {
				if (enemy.status == Enemy::Status::alive && isInScreen(enemy)) {
					enemy.takeDemage(enemy.health + 1.0f);
					onEnemyDeath(enemy);
					demageNumbers.emplace_back(enemy.getCenter(), 999, AttackResult::AttackType::Normal);
				}
			}
		}
		if(result.gemMagnet) {
			for (auto& pickup : pickupList) {
				if (pickup->isLying() == false) continue;
				auto gem = dynamic_cast<ExperienceGem*>(pickup.get());
				if(gem != nullptr) {
					gem->beMagnet();
				}
			}
		}
	}
	//屏幕周围位置获取
	//矩形版
	Vector2 outScreenRandomPositionRect() {
		static constexpr int area = 64;
		static constexpr float ratio = 0.2f;
		int gameWidth = window.getSize().x;
		int gameHeight = window.getSize().y;
		int r = GetRandomValue(0, (gameWidth + gameHeight) * (2 + 4 * ratio));
		float xPadding = gameWidth * (0.5f + ratio);
		float yPadding = gameHeight * (0.5f + ratio);
		float x;
		float y;
		if (r < yPadding * 2) {
			x = player.getPosition().x - xPadding + GetRandomValue(-area, area);
			y = player.getPosition().y - yPadding + r;
		}
		else if (r < yPadding * 2 + xPadding * 2) {
			x = player.getPosition().x - xPadding + r - yPadding * 2;
			y = player.getPosition().y + yPadding + GetRandomValue(-area, area);
		}
		else if (r < yPadding * 4 + xPadding * 2) {
			x = player.getPosition().x + xPadding + GetRandomValue(-area, area);
			y = player.getPosition().y - yPadding + r - yPadding * 2 - xPadding * 2;
		}
		else {
			x = player.getPosition().x - xPadding + r - yPadding * 4 - xPadding * 2;
			y = player.getPosition().y - yPadding + GetRandomValue(-area, area);
		}
		return { x, y };
	}
	//椭圆版
	Vector2 outScreenRandomPositionEllipse() {
		float ratio = 2.0f;	//为根号2(1.414)时是最小面积外切椭圆
		ratio *= randomNormal(1.0f, 0.15f, 0.25f);
		float a = window.getSize().x /2.0f * ratio;
		float b = window.getSize().y /2.0f * ratio;
		float theta = GetRandomValue(0, 3590) * PI / 1800.0f;
		float x = player.getPosition().x + a * cos(theta);
		float y = player.getPosition().y + b * sin(theta);
		return { x, y };
	}
public:
	GameLevel(const Window& _window, Player& _player) : 
		window(_window), atlas(_window, "./resources/forest_map.json"), generator("./resources/enemies.json"), player(_player),
		camera({
			window.getCenter(),// offset
			{0,0},// target
			0.0f,// rotation
			1.0f// zoom
			}){
		enemyList.reserve(100);
		for(int i=0;i<collideGrid.size();i++){
			for(int j=0;j<collideGrid[i].size();j++){
				collideGrid[i][j].PosLeftTop = { float((j - 1) * GRID_SIZE), float((i - 1) * GRID_SIZE) };
			}
		}
		//初始化全部武器，主动和被动
		auto& weaponGen = getWeaponRegisterPool();
		for(auto& createFunc: weaponGen) {
			allWeapons.emplace_back(createFunc(window, player.getStats(), player.getPosition(), player.getTowards()));
			//测试时期
			if(allWeapons.back()->getName() == "Whip") {
				allWeapons.back()->levelup();
				ownedWeapons.push_back(allWeapons.back().get());
			}
		}
		for(int i=0;i<static_cast<int>(PlayerStatsEnum::SizeOfEnum);i++){
			PlayerStatsEnum kind = static_cast<PlayerStatsEnum>(i);
			allPassives.emplace_back(std::make_unique<PassiveItem>(window, kind, player.getStats()));
			//测试，初始化为一级
			//allPassives.back()->levelup();
			//ownedPassives.push_back(allPassives.back().get());
		}
	}
	void update() {
		if (GameState::GameOver == gameState) {
			return;
		}
		if (gameState == GameState::paused) {
			if (IsKeyPressed(KEY_ESCAPE)) {
				gameState = GameState::Playing;
			}
			return;
		}
		if (gameState == GameState::choosing) {
			LevelupPanel* levelupPanel = static_cast<LevelupPanel*>(panel.get());
			BaseItem* item = levelupPanel->update();
			if (item != nullptr) {
				if (item->getLevel() == 0) {
					if (auto* weapon = dynamic_cast<BaseWeapon*>(item)) {
						ownedWeapons.push_back(weapon);
					}
					else if (auto* passive = dynamic_cast<PassiveItem*>(item)) {
						ownedPassives.push_back(passive);
					}
				}
				item->levelup();
				panel.reset();
				levelupCount--;
				if (levelupCount == 0) {
					gameState = GameState::Playing;
				}
				else {
					panel = std::make_unique<LevelupPanel>(selectLevelup());
				}
			}
			return;
		}
		//以下为Playing状态
		if (IsKeyPressed(KEY_ESCAPE)) {
			gameState = GameState::paused;
			return;
		}
		gameTime++;
		//调试
		if (IsKeyPressed(KEY_U)) {
			//player.getStats().growth *= 1000.0f;
			pickupList.emplace_back(std::make_unique<Pickup>(Pickup::Type::FireBreath, Vector2(0, 0)));
		}
		if (IsKeyPressed(KEY_I)) {
			pickupList.emplace_back(std::make_unique<Pickup>(Pickup::Type::Rosary, Vector2(0, 0)));
		}
		if (IsKeyPressed(KEY_P)) {
			pickupList.emplace_back(std::make_unique<Pickup>(Pickup::Type::Magnet, Vector2(0, 0)));
		}
		//光源更新，每秒生成
		if (gameTime % 60 == 0) {
			if (lightSources.size() < MAX_LIGHT_SOURCE_COUNT) {
				for (int i = 0; i < MAX_LIGHT_SOURCE_COUNT - lightSources.size(); i++) {
					Vector2 pos = outScreenRandomPositionEllipse();
					lightSources.emplace_back(std::make_unique<Enemy>(generateLightSourceEnemy(pos)));
				}
			}
			else {
				int n = 0;
				for (; n < lightSources.size(); n++) {
					if (lightSources[n]->status == Enemy::Status::alive && !isInScreen(*lightSources[n])) {
						break;
					}
				}
				if(n != lightSources.size()) {
					lightSources[n]->status = Enemy::Status::dead;	//后面马上删除
				}
				Vector2 pos = outScreenRandomPositionEllipse();
				lightSources.emplace_back(std::make_unique<Enemy>(generateLightSourceEnemy(pos)));
			}
		}
		std::erase_if(lightSources, [](const std::unique_ptr<Enemy>& ptr) {
			return ptr->status == Enemy::Status::dead;
			});
		//怪物波次生成
		const SpawnTimerData* currentWave = nullptr;
		for(const auto& wave: spawnTimerData) {
			if(gameTime / 60 < wave.waveTimeSecond) {
				currentWave = &wave;
				break;
			}
		}
		assert(currentWave != nullptr);
		int interval = static_cast<int>(currentWave->defaultIntervalFrame * player.getStats().curse);
		if (gameTime % interval == 0) {
			int spawnCount = currentWave->targetAmount - existingEnemyCount;
			spawnCount = std::max(1, spawnCount);
			for (int i = 0; i < spawnCount; i++) {
				Vector2 pos = outScreenRandomPositionEllipse();
				enemyList.push_back(generator.spawn(currentWave->enemyName, pos, enemyIdCounter++));
				existingEnemyCount++;
			}
		}
		//怪物前进、更新动画
		std::erase_if(enemyList, [](const Enemy& enemy) { return enemy.status == Enemy::Status::dead; });
		for (Enemy& enemy : enemyList) {
			enemy.update(player.getPosition());
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
		//掉落物更新
		PickupResult result;
		size_t size = pickupList.size();//过程中会新增元素（念珠击杀敌人后掉落经验宝石）
		for (size_t i = 0; i < size; i++) {
			const PickupResult res = pickupList[i]->update(player);
			onPickup(res);
		}
		if (timeFreezeTime > 0) {
			timeFreezeTime--;
			for (Enemy& enemy : enemyList) {
				if (isInScreen(enemy))
					enemy.specialStatus = Enemy::SpecialStatus::frozen;
			}
		}
		std::erase_if(pickupList, [](const std::unique_ptr<FloatPickup>& pickup) { return !pickup->isAlive(); });
		//玩家更新，检查受伤与自身更新
		float hurtDemage = 0.0f;
		for (int i = 0; i < collideGrid.size(); i++) {
			for (int j = 0; j < collideGrid[i].size(); j++) {
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
		//在碰撞格子中加入光源
		for(const auto& lightSource : lightSources) {
			if (lightSource->status != Enemy::Status::alive) continue;
			Vector2 screenPos = GetWorldToScreen2D(lightSource->position, camera);
			int x = static_cast<int>(std::floor(screenPos.x) / GRID_SIZE) + 1;
			int y = static_cast<int>(std::floor(screenPos.y) / GRID_SIZE) + 1;
			if(x >= 0 && x < collideGrid[0].size() && y >= 0 && y < collideGrid.size()) {
				collideGrid[y][x].enemies.push_back(lightSource.get());
			}
		}
		//武器攻击
		BaseWeapon* hiddenFireBreath = &fireBreath;
		ownedWeapons.push_back(hiddenFireBreath);//临时加入fireBreath
		for (auto& weapon : ownedWeapons) {
			auto& projectiles = weapon->update(weaponUpdateFuncs);
			for (auto& projectile : projectiles) {
				Vector2 projectilePosition = GetWorldToScreen2D({ projectile->rect.x,projectile->rect.y }, camera);//2D坐标转换
				Rectangle projectileRect = { projectilePosition.x,projectilePosition.y,projectile->rect.width,projectile->rect.height };
				forNearGrids(projectileRect,
					[&projectile, this](Enemy& enemy) {
						if (enemy.status != Enemy::Status::alive) return;
						AttackResult result = projectile->attack(enemy);
						if (result.isKilled) {
							onEnemyDeath(enemy);
						}
						if (result.demage > 0) {
							demageNumbers.emplace_back(enemy.getCenter(), static_cast<int>(result.demage), result.attackType);
						}
					});
			}
		}
		ownedWeapons.pop_back();//移除fireBreath
		//按y坐标排序格子，保证绘制顺序正确
		for(auto& row : collideGrid) {
			for(auto& cell : row) {
				std::sort(cell.enemies.begin(), cell.enemies.end(), [](const Enemy* a, const Enemy* b) {
					return a->position.y < b->position.y;
					});
			}
		}
		//伤害数字更新
		for (auto& demageNumber : demageNumbers) {
			demageNumber.time++;
		}
		std::erase_if(demageNumbers, [](const DemageNumber& demageNumber) { return demageNumber.time > DemageNumber::Duration; });
		//最后处理升级
		if (levelupCount > 0 && panel == nullptr) {
			panel = std::make_unique<LevelupPanel>(selectLevelup());
			gameState = GameState::choosing;
		}
	}
	void drawLevel() const {
		BeginMode2D(getGameCamera());
		//绘制地图
		atlas.draw(player.getPosition());
		//draw pickups
		for(const auto& pickup : pickupList){
			pickup->draw(window);
		}
		//绘制格子中的实体
		for (int i = 0; i < collideGrid.size(); i++) {
			for (const auto& cells : collideGrid[i]) {
				for (const auto& enemyPtr : cells.enemies) {
					const Enemy& enemy = *enemyPtr;
					if (enemy.status == Enemy::Status::dead) continue;
					Texture2D texture;
					Rectangle source;
					if (enemy.status == Enemy::Status::alive) {
						texture = *window.getTexture(enemy.def->moveFrames[enemy.frame]).sprite;
						source = window.getTexture(enemy.def->moveFrames[enemy.frame]).source;
					}
					else if (enemy.status == Enemy::Status::dying) {
						texture = *window.getTexture(enemy.def->deathFrames[enemy.frame]).sprite;
						source = window.getTexture(enemy.def->deathFrames[enemy.frame]).source;
					}
					if (!enemy.towards) {
						source.width = -source.width;
					}
					float drawingHeight = enemy.def->height * 1.0f;
					float drawingWidth = enemy.def->width * 1.0f;
					Color color = WHITE;
					if (enemy.specialStatus == Enemy::SpecialStatus::frozen) {
						color = BLUE;
					}
					DrawTexturePro(texture, source,
						{ enemy.position.x , enemy.position.y,drawingWidth,drawingHeight },
						{ drawingWidth / 2.0f	 ,drawingHeight },
						0.0f,
						color);
				}
			}
		}
		//draw player
		player.drawPlayer(window);
		player.drawHealthBar();
		//draw weapons
		for (const auto& weapon : ownedWeapons) {
			weapon->draw();
		}
		fireBreath.draw();
		//伤害数字
		for (auto& demageNumber : demageNumbers) {
			Color color = WHITE;
			Color boarderColor = BLACK;
			if (demageNumber.type == AttackResult::AttackType::Critical) {
				color = RED;
			}
			float ratio;
			if(demageNumber.time < DemageNumber::EmergenceTime) {
				ratio = static_cast<float>(demageNumber.time) / DemageNumber::EmergenceTime;
			}
			else if(demageNumber.time >= DemageNumber::Duration - DemageNumber::EmergenceTime){
				ratio = static_cast<float>(DemageNumber::Duration - demageNumber.time) / DemageNumber::EmergenceTime;
			}
			else {
				ratio = 1.0f;
			}
			color.a = static_cast<unsigned char>(color.a * ratio);
			boarderColor.a = static_cast<unsigned char>(boarderColor.a * ratio);
			const Font& font = window.getFont(DemageNumber::fontSize);
			std::string textStr = std::format("{}", demageNumber.demage);
			const char* text = textStr.c_str();
			float displaySize = DemageNumber::fontSize * ratio * 1.2f;
			Vector2 textPosition = { demageNumber.position.x - MeasureTextEx(font, text, displaySize, 0.0f).x * 0.5f,
				demageNumber.position.y - displaySize * 0.5f };
			for (int i = 0; i < 3; i++) {
				for(int j = 0; j < 3; j++) {
					if(i==1 && j==1) continue;
					DrawTextEx(font, text, { textPosition.x + (i - 1) * 2.0f, textPosition.y + (j - 1) * 2.0f }, displaySize, 0.0f, boarderColor);
				}
			}
			for (int i = 0; i < 2; i++) {//文字的周边是有半透明区域的，重复会更饱满更粗体
				DrawTextEx(font, text, textPosition, displaySize, 0.0f, color);
			}
			
		}
		EndMode2D();
		//over
		//绘制固定UI
		BeginMode2D(getUICamera());
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
		//经验条
		player.drawExpBar(window);//我也不知道为啥放进player里了
		//击杀数、金币数
		{
			static constexpr int marginTop = 40;
			static constexpr int marginRight = 10;
			static constexpr int padding = 100;
			static constexpr float iconSize = 32.0f;
			const Font font = window.getFont(64);
			std::string killCountStr = std::to_string(killedEnemyCount);
			std::string goldGainStr = std::to_string(static_cast<int>(goldGain));
			const auto& killText = window.getTexture("skull_32x32");
			const auto& goldText = window.getTexture("coin_32x32");
			Rectangle target = { window.getSize().x - marginRight - iconSize, static_cast<float>(marginTop), iconSize, iconSize };
			DrawTexturePro(*goldText.sprite, goldText.source, target, { 0,0 }, 0.0f, WHITE);
			DrawTextEx(font, goldGainStr.c_str(),
				{ target.x - MeasureTextEx(font, goldGainStr.c_str(), iconSize, 0.0f).x, target.y },
				iconSize, 0.0f, WHITE);
			target.x -= static_cast<float>(padding);
			DrawTexturePro(*killText.sprite, killText.source, target, { 0,0 }, 0.0f, WHITE);
			DrawTextEx(font, killCountStr.c_str(),
				{ target.x - MeasureTextEx(font, killCountStr.c_str(), iconSize, 0.0f).x, target.y },
				iconSize, 0.0f, WHITE);
		}
		//已拥有物品
		{
			constexpr int Padding = 4;
			constexpr int MarginTop = 40;
			constexpr int IconSize = 48;
			constexpr int Collumns = 6;
			auto drawItems = [&](const auto& items, int startRow) {
				int rowCount = startRow;
				int collumnCount = 0;
				for (int i = 0; i < items.size() || i < Collumns; i++) {
					if (i > 0 && i % Collumns == 0) {
						rowCount++;
						collumnCount = 0;
					}
					Rectangle itemRect = { static_cast<float>(Padding + collumnCount * (IconSize + Padding)),
						static_cast<float>(MarginTop + rowCount * (IconSize + Padding)),
						static_cast<float>(IconSize), static_cast<float>(IconSize) };
					Color color = { 255,255,255,192 };
					DrawRectangleRec(itemRect, color);
					if (i < items.size()) {
						DrawFromSpriteSet drawSet = items[i]->getIconTexture();
						DrawTexturePro(*drawSet.sprite, drawSet.source, itemRect, { 0,0 }, 0.0f, WHITE);
					}
					collumnCount++;
				}
				return rowCount + 1; // Return the next row index
				};
			int nextRow = drawItems(ownedWeapons, 0);
			drawItems(ownedPassives, nextRow);
		}
		//绘制面板
		if(panel!=nullptr){
			panel->draw();
		}
		EndMode2D();
	}
};