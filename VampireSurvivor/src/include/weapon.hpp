#pragma once
#include "raylib.h"
#include "raymath.h"
#include "common.hpp"
#include "enemy.hpp"
#include "window.hpp"
#include <algorithm>
//主动、被动统一基类见common.hpp
//被动见pickup.hpp
//投射物基类
class Projectile {
public:
	Rectangle rect;	//粗略矩形，level通过网格大致判断碰撞范围
	float demage = 1;
	float pushBack = 1;
	int DefaultAttackCooldown = 5;	//攻击冷却，单位帧
	std::unordered_map<uint64_t, int> attackedEnemy;	//记录已经攻击过的敌人，以及剩余攻击冷却
	Projectile(Rectangle _rect, float _damage, float _pushBack, int _defaultAttackCooldown) :
		rect(_rect), demage(_damage), pushBack(_pushBack), DefaultAttackCooldown(_defaultAttackCooldown) {
	};
	virtual ~Projectile() = default;
	virtual bool attackDef(Enemy& enemy) = 0;
	bool attack(Enemy& enemy) {
		if (!checkCollision(enemy)) return false;
		if (attackedEnemy.find(enemy.id) != attackedEnemy.end()) {
			if (attackedEnemy[enemy.id] > 0) {
				return false;
			}
		}
		attackedEnemy[enemy.id] = DefaultAttackCooldown;
		if (enemy.status != Enemy::Status::alive) return false;
		return attackDef(enemy);
	}
	//默认碰撞检测，粗略矩形即为实际碰撞。可重写
	virtual bool checkCollision(const Enemy& enemy) const {
		return CheckCollisionCircleRec(enemy.position, enemy.def->hurtRadius, rect);
	}
};
//武器基类
class BaseWeapon : public BaseItem {
protected:
	int DefaultDemage = 1;
	int DefaultCooldown = 60;
	int cooldown = DefaultCooldown;
	int coolTime = 0;
	std::vector<std::unique_ptr<Projectile>> projectiles;
	const PlayerStats& stats;
	const Vector2& playerPosition;
	const bool& playerTowards;
	//注册函数，注册武器构造函数到注册池
	template<typename T>
	static bool registerThis() {
		weaponCreateFunc func = [](const Window& _window, const PlayerStats& _stats, const Vector2& _playerPosition, const bool& _playerTowards) {
			return std::make_unique<T>(_window, _stats, _playerPosition, _playerTowards);
		};
		registerWeapon(func);
		return true;
	}
public:
	BaseWeapon(const Window& _window, const std::string_view _name, 
		const PlayerStats& _stats, const Vector2& _playerPosition, 
		const bool& _playerTowards, const std::string_view _description) :
		BaseItem(_window, _name, _description),
		stats(_stats), playerPosition(_playerPosition), playerTowards(_playerTowards) {
	};
	const DrawFromSpriteSet& getIconTexture() const override {
		return window.getTexture(std::string(name) + "_icon");
	}
	virtual ~BaseWeapon() = default;
	virtual const std::vector<std::unique_ptr<Projectile>>& update() = 0;
	virtual void draw() const = 0;
	
};
//神鞭
class WeaponWhip : public BaseWeapon {
public:
	static constexpr std::string_view Name = "Whip";
	inline static bool registered = registerThis<WeaponWhip>();
private:
	int defaultWidth = 256;
	int defaultHeight = 128;
	static constexpr int partTimeGenerate = 8;
	int projectileAmount = 1;
	//神鞭粒子
	class WhipProjectile : public Projectile
	{
	public:
		static constexpr int DefaultPushBack = 48;
		static constexpr int MaxExistTime = 15;
		int existTime = MaxExistTime;
		bool rotated = false;	//false: front, true: back
		bool towards = true;	//true: right, false: left
		WhipProjectile(Rectangle _rect, float _damage,bool _rotated,bool _towards) :
			Projectile(_rect, _damage, DefaultPushBack, MaxExistTime), 
			rotated(_rotated), towards(_towards) {
		};
		bool attackDef(Enemy& enemy) override {
			Vector2 direction = Vector2Scale(enemy.direction, -pushBack);
			enemy.position = Vector2Add(enemy.position, direction);
			return enemy.takeDemage(demage);
		}
	};
public:
	WeaponWhip(const Window& _window, const PlayerStats& _stats, const Vector2& _playerPosition, const bool& _playerTowards) : 
		BaseWeapon(_window, Name, _stats, _playerPosition, _playerTowards,
			"向左右两边挥舞，攻击并击退敌人。"
			) {
		DefaultDemage = 10;
		DefaultCooldown = 60;
		cooldown = std::max(5, static_cast<int>(DefaultCooldown * stats.cooldown));
		projectileAmount = 1 + stats.amount;
	};
	const std::vector<std::unique_ptr<Projectile>>& update() override {
		//先根据玩家属性更新自身
		cooldown = std::max(5, static_cast<int>(DefaultCooldown * stats.cooldown));
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
				Rectangle dest = {playerPosition.x, playerPosition.y, defaultWidth * stats.area, defaultHeight * stats.area};
				if (!rotated && playerTowards) dest.y -= defaultHeight * stats.area;
				else if (!rotated && !playerTowards) dest.x -= defaultWidth * stats.area, dest.y -= defaultHeight * stats.area;
				else if (rotated && playerTowards) dest.x -= defaultWidth * stats.area;
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
			for(auto& [_,attackedTime] : whipProjectilePtr->attackedEnemy) {
				attackedTime = std::max(0, attackedTime - 1);
			}
		}
		std::erase_if(projectiles, [](const std::unique_ptr<Projectile>& projectilePtr) {
			WhipProjectile* whipProjectilePtr = static_cast<WhipProjectile*>(projectilePtr.get());
			return whipProjectilePtr->existTime <= 0;
			});
		return projectiles;
	};
	void levelup() override {
		if (level >= MAX_LEVEL) {
			terminate();
			return;
		}
		switch (level)
		{
		case 0:
			break;
		case 1:
			projectileAmount++;
			break;
		case 2:
			defaultWidth += 32;
			defaultHeight+= 16;
			break;
		case 3:
			DefaultDemage += 5;
			DefaultCooldown -= 10;
			break;
		case 4:
			projectileAmount++;
			break;
		case 5:
			defaultWidth += 32;
			defaultHeight += 16;
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
	void draw() const override {
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
//大蒜
class WeaponGarlic : public BaseWeapon {
public:
	static constexpr std::string_view Name = "Garlic";
	inline static bool registered = registerThis<WeaponGarlic>();
private:
	int defaultRadius = 128;
	//大蒜粒子
	class GarlicProjectile : public Projectile
	{
	public:
		static constexpr int DefaultPushBack = 10;
		float radius;
		GarlicProjectile(Rectangle _rect, float _damage, int _cooldown, float _radius) :
			Projectile(_rect, _damage, DefaultPushBack, _cooldown),
			radius(_radius) {
		};
		bool checkCollision(const Enemy& enemy) const override {
			return CheckCollisionCircles(enemy.position, enemy.def->hurtRadius, { rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f }, radius);
		}
		bool attackDef(Enemy& enemy) override {
			Vector2 direction = Vector2Scale(enemy.direction, -pushBack);
			enemy.position = Vector2Add(enemy.position, direction);
			return enemy.takeDemage(demage);
		}
	};
public:
	WeaponGarlic(const Window& _window, const PlayerStats& _stats, const Vector2& _playerPosition, const bool& _playerTowards) :
		BaseWeapon(_window, "Garlic", _stats, _playerPosition, _playerTowards,
			"以自己为中心的辐射状区域，伤害一切靠近的敌人"
		) {
		DefaultDemage = 5;
		DefaultCooldown = 30;
		cooldown = std::max(5, static_cast<int>(DefaultCooldown * stats.cooldown));
		projectiles.reserve(1);
		float radius = defaultRadius * stats.area;
		Rectangle rect = { playerPosition.x - radius, playerPosition.y - radius, radius * 2.0f, radius * 2.0f };
		projectiles.emplace_back(std::make_unique<GarlicProjectile>(rect, DefaultDemage * stats.strength, cooldown, radius));
	};
	const std::vector<std::unique_ptr<Projectile>>& update() override {
		//始终只有一个大蒜粒子，更新其位置和属性
		float radius = defaultRadius * stats.area;
		Rectangle rect = { playerPosition.x - radius, playerPosition.y - radius, radius * 2.0f, radius * 2.0f };
		projectiles[0]->rect = rect;
		projectiles[0]->demage = DefaultDemage * stats.strength;
		projectiles[0]->DefaultAttackCooldown = std::max(5, static_cast<int>(DefaultCooldown * stats.cooldown));
		GarlicProjectile* garlicProjectilePtr = static_cast<GarlicProjectile*>(projectiles[0].get());
		garlicProjectilePtr->radius = radius;
		for(auto& [_,attackedTime] : projectiles[0]->attackedEnemy) {
			attackedTime = std::max(0, attackedTime - 1);
		}
		return projectiles;
	}
	void levelup() override {
		switch (level)
		{
		case 0:
			break;
		case 1:
			DefaultDemage += 5;
			break;
		case 2:
			defaultRadius += 32;
			DefaultCooldown -= 5;
			break;
		case 3:
			DefaultDemage += 5;
			break;
		case 4:
			defaultRadius += 32;
			DefaultCooldown -= 4;
			break;
		case 5:
			DefaultDemage += 5;
			break;
		case 6:
			defaultRadius += 32;
			DefaultCooldown -= 3;
			break;
		case 7:
			DefaultDemage += 5;
			break;
		}
		level++;
		cooldown = std::max(5, static_cast<int>(DefaultCooldown * stats.cooldown));
	}
	void draw() const override {
		auto& texture = window.getTexture("garlic_effect");
		Rectangle dest = projectiles[0]->rect;
		//使用加色混合模式绘制，因为是半透明素材
		BeginBlendMode(BLEND_ADDITIVE);
		DrawTexturePro(*texture.sprite, texture.source,
			{ dest.x + dest.width / 2.0f,dest.y + dest.height / 2.0f,dest.width,dest.height },
			{ dest.width / 2.0f, dest.height / 2.0f }, 0.0f, WHITE);
		EndBlendMode();
	}
};