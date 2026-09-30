#pragma once
#include "raylib.h"
#include "raymath.h"
#include "common.hpp"
#include "enemy.hpp"
#include "window.hpp"
#include <algorithm>
//基类
class Projectile {
public:
	Rectangle rect;
	float demage = 1;
	float pushBack = 1;
	virtual void attackDef(Enemy& enemy) = 0;
	Projectile(Rectangle _rect, float _damage, float _pushBack) :
		rect(_rect), demage(_damage), pushBack(_pushBack) {
	};
	~Projectile() = default;
	void attack(Enemy& enemy) {
		if (enemy.status != 1) return;
		attackDef(enemy);
		if (enemy.health <= 0) {
			enemy.status = 2;	//dying
			enemy.frame = 0;
			enemy.changeTime = 0;
		}
	}
};
class BaseWeapon {
protected:
	const Window& window;
	std::string name;
	char level = 1;
	char MAX_LEVEL = 8;
	int DefaultDemage = 1;
	int DefaultCooldown = 60;
	int cooldown = DefaultCooldown;
	int coolTime = 0;
	std::vector<std::unique_ptr<Projectile>> projectiles;
	const PlayerStats& stats;
	const Vector2& playerPosition;
	const bool& playerTowards;
public:
	BaseWeapon(const Window& _window, const std::string& _name, const PlayerStats& _stats, const Vector2& _playerPosition, const bool& _playerTowards) :
		window(_window), name(_name), stats(_stats), playerPosition(_playerPosition), playerTowards(_playerTowards) {
	};
	virtual ~BaseWeapon() = default;
	virtual const std::vector<std::unique_ptr<Projectile>>& update() = 0;
	virtual void levelup() = 0;
	virtual void draw() = 0;
};
//神鞭
class WeaponWhip : public BaseWeapon {
private:
	int defultWidth = 256;
	int defultHeight = 128;
	static constexpr int partTimeGenerate = 8;
	int projectileAmount = 1;
	//神鞭粒子
	class WhipProjectile : public Projectile
	{
	public:
		static constexpr int DefaultPushBack = 48;
		constexpr static int MaxExistTime = 15;
		int existTime = MaxExistTime;
		bool rotated = false;	//false: front, true: back
		bool towards = true;	//true: right, false: left
		WhipProjectile(Rectangle _rect, float _damage,bool _rotated,bool _towards) :
			Projectile(_rect, _damage, DefaultPushBack), 
			rotated(_rotated), towards(_towards) {
		};
		void attackDef(Enemy& enemy) override {
			if (existTime != MaxExistTime / 2) return;	//只造成一次伤害
			enemy.health -= demage;
			Vector2 direction = Vector2Scale(enemy.direction, -pushBack);
			enemy.position = Vector2Add(enemy.position, direction);
		}
	};
public:
	WeaponWhip(const Window& _window, const PlayerStats& _stats, const Vector2& _playerPosition, const bool& _playerTowards) : 
		BaseWeapon(_window, "Whip", _stats, _playerPosition, _playerTowards) {
		DefaultDemage = 10;
		DefaultCooldown = 60;
		cooldown = std::max(5, static_cast<int>(DefaultCooldown * stats.cooldown));
		projectileAmount = 1 + stats.amount;
	};
	const std::vector<std::unique_ptr<Projectile>>& update() override {
		//冷却阶段
		if (coolTime > 0) {
			coolTime--;
			if (coolTime == 0) {
				coolTime = projectileAmount * -partTimeGenerate;//注意用负数表示发射阶段
			}
		}
		//发射阶段
		else {
			coolTime++;
			int projectileIndex = (coolTime - projectileAmount * -partTimeGenerate) / partTimeGenerate;
			if (coolTime % partTimeGenerate == 0) {
				bool rotated = projectileIndex % 2 == 0;
				Rectangle dest = {playerPosition.x, playerPosition.y, defultWidth * stats.area, defultHeight * stats.area};
				if (!rotated && playerTowards) dest.y -= defultHeight * stats.area;
				else if (!rotated && !playerTowards) dest.x -= defultWidth * stats.area, dest.y -= defultHeight * stats.area;
				else if (rotated && playerTowards) dest.x -= defultWidth * stats.area;
				else;
				dest.y -= float((projectileIndex + 1) / 2) * 64;
				float damage = DefaultDemage * stats.strength;
				projectiles.push_back(std::make_unique<WhipProjectile>(dest, damage, rotated, playerTowards));
			}
			if (coolTime == 0) {
				coolTime = cooldown;
			}
		}
		//飞射物更新
		for (auto& projectilePtr : projectiles) {
			WhipProjectile* whipProjectilePtr = static_cast<WhipProjectile*>(projectilePtr.get());
			whipProjectilePtr->existTime--;
		}
		std::erase_if(projectiles, [](const std::unique_ptr<Projectile>& projectilePtr) {
			WhipProjectile* whipProjectilePtr = static_cast<WhipProjectile*>(projectilePtr.get());
			return whipProjectilePtr->existTime <= 0;
			});
		return projectiles;
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
		cooldown = std::max(5, static_cast<int>(DefaultCooldown * stats.cooldown));
	};
	void draw() override {
		for(auto& projectilePtr : projectiles){
			WhipProjectile* projectile = static_cast<WhipProjectile*>(projectilePtr.get());
			Rectangle source = window.getTexture("whip_effect").source;
			Rectangle dest = projectile->rect;
			if (projectile->rotated) {
				source.height = -source.height;
				source.width = -source.width;
			}
			if (!projectile->towards) {
				source.width = -source.width;
			}
			float progress = 1.0f - float(projectile->existTime) / float(projectile->MaxExistTime);
			float drawingHeight = dest.height * (progress * 0.5f + 0.5f);
			float drawingWidth = dest.width * (progress * 0.5f + 0.5f);
			DrawTexturePro(*window.getTexture("whip_effect").sprite, source,
				{ dest.x + dest.width / 2.0f,dest.y + dest.height / 2.0f,drawingWidth,drawingHeight },
				{ drawingWidth / 2.0f, drawingHeight / 2.0f }, 0.0f, WHITE);
		}
	};
};
