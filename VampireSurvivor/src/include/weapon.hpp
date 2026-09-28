#pragma once
#include "raylib.h"
#include "raymath.h"
#include "common.hpp"
#include "enemy.hpp"
#include "window.hpp"
#include <algorithm>
class BaseWeapon {
protected:
	const Window& window;
	std::string name;
	char level = 1;
	char MAX_LEVEL = 8;
	float demage = 1.0f;
	int DefaultDemage = 1;
	int cooldown = 60;
	int DefaultCooldown = 60;
	int coolTime = 0;
	//int pushBack = 10;
	int DefaultPushBack = 10;
	const PlayerStats& stats;
	const Vector2& playerPosition;
	const bool& playerTowards;
public:
	BaseWeapon(const Window& _window, const std::string& _name, const PlayerStats& _stats, const Vector2& _playerPosition, const bool& _playerTowards) :
		window(_window), name(_name), stats(_stats), playerPosition(_playerPosition), playerTowards(_playerTowards) {
	};
	virtual ~BaseWeapon() = default;
	virtual void update(std::vector<Enemy>&) = 0;
	virtual void levelup() = 0;
	virtual void draw() = 0;
};
class WeaponWhip : public BaseWeapon {
private:
	int defultWidth = 256;
	int defultHeight = 128;
	int projectileAmount = 1;
	constexpr static int maxExistTime = 10;
	struct Projectile
	{
		int exsistTime = maxExistTime;
		Rectangle rect;
		bool rotated = false;	//false: front, true: back
		bool towards = true;	//true: right, false: left
	};
	std::vector<Projectile> projectiles;
public:
	WeaponWhip(const Window& _window, const PlayerStats& _stats, const Vector2& _playerPosition, const bool& _playerTowards) : 
		BaseWeapon(_window, "Whip", _stats, _playerPosition, _playerTowards) {
		DefaultDemage = 10;
		demage = DefaultDemage * stats.strength;
		DefaultCooldown = 60;
		cooldown = std::max(5, static_cast<int>(DefaultCooldown * stats.cooldown));
		projectileAmount = 1 + stats.amount;
		DefaultPushBack = 48;

	};
	void update(std::vector<Enemy>& enemies) override {
		if (coolTime > 0) {
			coolTime--;
			if (coolTime == 0) {
				coolTime = projectileAmount * -4;
			}
		}
		else {
			coolTime++;
			int projectileIndex = (coolTime - projectileAmount * -4) / 4;
			if (coolTime % 4 == 0) {
				bool rotated = projectileIndex % 2 == 0;
				Rectangle dest = {playerPosition.x, playerPosition.y, defultWidth * stats.area, defultHeight * stats.area};
				if (!rotated && playerTowards) dest.y -= defultHeight * stats.area;
				else if (!rotated && !playerTowards) dest.x -= defultWidth * stats.area, dest.y -= defultHeight * stats.area;
				else if (rotated && playerTowards) dest.x -= defultWidth * stats.area;
				else;
				dest.y -= float((projectileIndex + 1) / 2) * 64;
				projectiles.push_back({
					maxExistTime, // exsistTime
					dest, // dest
					rotated, // rotated
					playerTowards // towards
					});
			}
			if (coolTime == 0) {
				coolTime = cooldown;
			}
		}
		for (Projectile& projectile : projectiles) {
			projectile.exsistTime--;
			if(projectile.exsistTime <= 0){
				projectile = projectiles.back();
				projectiles.pop_back();
				continue;
			}
			if (projectile.exsistTime != maxExistTime/2) continue;
			for(auto& enemy : enemies){
				if(enemy.status != 1) continue;
				Rectangle enemyRect = { enemy.position.x - enemy.def->width / 2.0f,
					enemy.position.y - enemy.def->height,
					enemy.def->width,enemy.def->height };
				if(CheckCollisionRecs(projectile.rect, enemyRect)){
					enemy.health -= demage;
					Vector2 direction = Vector2Subtract(enemy.position, playerPosition);
					direction = Vector2Normalize(direction) * DefaultPushBack;
					enemy.position = Vector2Add(enemy.position, direction);
					if(enemy.health <= 0){
						enemy.status = 2;	//dying
						enemy.frame = 0;
						enemy.changeTime = 0;
					}
				}
			}
		}
	};
	void levelup() override {
		switch (level)
		{
		case 1:
			projectileAmount++;
			break;
		case 2:
			defultWidth += 32;
			defultHeight += 16;
			break;
		case 3:
			DefaultDemage += 5;
			DefaultCooldown -= 10;
			break;
		case 4:
			projectileAmount++;
			break;
		case 5:
			defultWidth += 32;
			defultHeight += 16;
			break;
		case 6:
			DefaultDemage += 5;
			DefaultCooldown -= 10;
			break;
		case 7:
			DefaultDemage += 5;
			break;
		}
		level++;
		coolTime = 0;
		demage = DefaultDemage * stats.strength;
		cooldown = std::max(5, static_cast<int>(DefaultCooldown * stats.cooldown));
	};
	void draw() override {
		for(auto& projectile : projectiles){
			Rectangle source = window.getTexture("whip_effect").source;
			Rectangle dest = projectile.rect;
			if (projectile.rotated) {
				source.height = -source.height;
				source.width = -source.width;
			}
			if (!projectile.towards) {
				source.width = -source.width;
			}
			float progress = 1.0f - float(projectile.exsistTime) / float(maxExistTime);
			float drawingHeight = dest.height * (progress * 0.5f + 0.5f);
			float drawingWidth = dest.width * (progress * 0.5f + 0.5f);
			DrawTexturePro(*window.getTexture("whip_effect").sprite, source,
				{ dest.x + dest.width / 2.0f,dest.y + dest.height / 2.0f,drawingWidth,drawingHeight },
				{ drawingWidth / 2.0f, drawingHeight / 2.0f }, 0.0f, WHITE);
		}
	};
};
