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
	//默认碰撞检测，粗略矩形即为实际碰撞。可重写
	virtual bool checkCollision(const Enemy& enemy) const {
		return CheckCollisionCircleRec(enemy.position, enemy.def->hurtRadius, rect);
	}
	//默认攻击函数，击退敌人并造成伤害，可重写
	virtual AttackResult attackDef(Enemy& enemy) {
		Vector2 direction = Vector2Scale(enemy.direction, -pushBack);
		enemy.position = Vector2Add(enemy.position, direction);
		return enemy.takeDemage(demage);
	};
	AttackResult attack(Enemy& enemy) {
		if (!checkCollision(enemy)) return AttackResult{};
		if (enemy.status != Enemy::Status::alive) return AttackResult{};
		if (attackedEnemy.find(enemy.id) != attackedEnemy.end()) {
			if (attackedEnemy[enemy.id] > 0) {
				return AttackResult{};
			}
		}
		attackedEnemy[enemy.id] = DefaultAttackCooldown;
		return attackDef(enemy);
	}
};
//武器基类
class BaseWeapon : public BaseItem {
protected:
	int DefaultDemage = 1;
	int DefaultCooldown = 60;
	int cooldown() { return std::max(5, static_cast<int>(DefaultCooldown * stats.cooldown)); }
	int coolTime = 0;
	std::vector<std::unique_ptr<Projectile>> projectiles;
	const PlayerStats& stats;
	const Vector2& playerPosition;
	const bool& playerTowards;
	//连发管理类，可选
	class FireManager {
	private:
		int fireInterval;
		int fireCount;
		int coolDown;
		int time = 0;
	public:
		FireManager(int _fireInterval, int _fireCount, int _coolDown) :
			fireInterval(_fireInterval), fireCount(_fireCount), coolDown(_coolDown) {
		};
		std::optional<int> get() {
			if (time >= fireCount * fireInterval) {
				time = -coolDown;
				return std::nullopt;
			}
			else if (time < 0) {
				time++;
				return -1;
			}
			else if (time % fireInterval == 0) {
				int index = time / fireInterval;
				time++;
				return index;
			}
			else {
				time++;
				return -1;
			}
		}
		void reset() {
			time = 0;
		}
		void update(int _fireCount, int _coolDown) {
			fireCount = _fireCount;
			coolDown = _coolDown;
			time = std::max(time, -coolDown);	//本轮更新冷却缩减
		}
	};
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
	virtual const std::vector<std::unique_ptr<Projectile>>& update(const WeaponUpdateFunc&) = 0;
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
	static constexpr int AttackInterval = 8;
	int DefaultProjectileAmount = 1;
	int projectileAmount() { return DefaultProjectileAmount + stats.amount; }
	FireManager fireManager;
	//神鞭粒子
	class WhipProjectile : public Projectile
	{
	public:
		static constexpr int DefaultPushBack = 48;
		static constexpr int MaxExistTime = 15;
		int existTime = MaxExistTime;
		bool rotated = false;	//false: front, true: back
		bool towards = true;	//true: right, false: left
		WhipProjectile(Rectangle _rect, float _damage, bool _rotated, bool _towards) :
			Projectile(_rect, _damage, DefaultPushBack, MaxExistTime),
			rotated(_rotated), towards(_towards) {
		};
	};
public:
	WeaponWhip(const Window& _window, const PlayerStats& _stats, const Vector2& _playerPosition, const bool& _playerTowards) :
		BaseWeapon(_window, Name, _stats, _playerPosition, _playerTowards,
			"向左右两边挥舞，攻击并击退敌人。"
		), fireManager(AttackInterval, projectileAmount(), cooldown()) {
		DefaultDemage = 10;
		DefaultCooldown = 60;
	};
	const std::vector<std::unique_ptr<Projectile>>& update(const WeaponUpdateFunc&) override {
		auto result = fireManager.get();
		if (result) {
			if (*result >= 0) {
				bool rotated = (*result) % 2 != 0;
				static constexpr float xOffset = 0.15;
				static constexpr float yOffset = 0.10;
				Rectangle dest = { playerPosition.x - defaultWidth * stats.area * xOffset, 
					playerPosition.y - defaultHeight * stats.area * yOffset, 
					defaultWidth * stats.area, 
					defaultHeight * stats.area };
				if (!rotated && playerTowards) 
					dest.y -= defaultHeight * stats.area - 2 * defaultHeight * stats.area * yOffset;
				else if (!rotated && !playerTowards) 
					dest.x -= defaultWidth * stats.area - 2 * defaultWidth * stats.area * xOffset, 
					dest.y -= defaultHeight * stats.area - 2 * defaultHeight * stats.area * yOffset;
				else if (rotated && playerTowards) 
					dest.x -= defaultWidth * stats.area - 2 * defaultWidth * stats.area * xOffset;
				else;
				dest.y -= float((*result) / 2) * 64;
				float damage = DefaultDemage * stats.strength;
				projectiles.push_back(std::make_unique<WhipProjectile>(dest, damage, rotated, playerTowards));
			}
		}
		else {
			fireManager.update(projectileAmount(), cooldown());
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
		switch (level)
		{
		case 0:
			break;
		case 1:
			DefaultProjectileAmount++;
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
			DefaultProjectileAmount++;
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
		fireManager.update(projectileAmount(), cooldown());
		fireManager.reset();
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
	};
public:
	WeaponGarlic(const Window& _window, const PlayerStats& _stats, const Vector2& _playerPosition, const bool& _playerTowards) :
		BaseWeapon(_window, Name, _stats, _playerPosition, _playerTowards,
			"以自己为中心的辐射状区域，伤害一切靠近的敌人"
		) {
		DefaultDemage = 5;
		DefaultCooldown = 30;
		projectiles.reserve(1);
		float radius = defaultRadius * stats.area;
		Rectangle rect = { playerPosition.x - radius, playerPosition.y - radius, radius * 2.0f, radius * 2.0f };
		projectiles.emplace_back(std::make_unique<GarlicProjectile>(rect, DefaultDemage * stats.strength, cooldown(), radius));
	};
	const std::vector<std::unique_ptr<Projectile>>& update(const WeaponUpdateFunc&) override {
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
//回旋锤子
class WeaponHammer : public BaseWeapon {
public:
	static constexpr std::string_view Name = "Hammer";
	inline static bool registered = registerThis<WeaponHammer>();
private:
	static constexpr int AttackInterval = 10;
	static constexpr int DefaultSpeed = 6;
	static constexpr IntVector2 DefaultSize = { 65, 104 };
	static constexpr int DefaultCollideRadius = 32;
	static constexpr int TotalExistTime = 120;
	static constexpr int UniformTime = 50;
	static constexpr int AccelerateTime = (TotalExistTime - 2 * UniformTime) / 2;
	int DefaultCount = 1;
	int projectileCount() { return DefaultCount + stats.amount; }
	float speed() { return DefaultSpeed * stats.projectileSpeed; }
	Vector2 size() { return Vector2Scale(DefaultSize, stats.area); }
	float collideRadius() { return DefaultCollideRadius * stats.area; }
	FireManager fireManager;
	//回旋锤粒子
	class HammerProjectile : public Projectile
	{
	public:
		static constexpr int DefaultPushBack = 16;
		static constexpr int AttackCooldown = 60;
		int time = 0;
		int angle = 0;	//角度制
		const Vector2 CenterOffset;
		Vector2 velocity;
		float collideRadius;
		HammerProjectile(Rectangle _rect, float _damage, Vector2 _velocity, float _collideRadius) :
			Projectile(_rect, _damage, DefaultPushBack, AttackCooldown),
			CenterOffset(_rect.width / 4.0f, _rect.height / 4.0f), velocity(_velocity), collideRadius(_collideRadius)
		{
		};
		bool checkCollision(const Enemy& enemy) const override {
			Vector2 center = Vector2(rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f);
			Vector2 hitCenter = center + Vector2Rotate(CenterOffset, angle * DEG2RAD);
			return CheckCollisionCircles(enemy.position, enemy.def->hurtRadius, hitCenter, collideRadius);
		}
	};
public:
	WeaponHammer(const Window& _window, const PlayerStats& _stats, const Vector2& _playerPosition, const bool& _playerTowards) :
		BaseWeapon(_window, Name, _stats, _playerPosition, _playerTowards,
			"旋转的锤子，向最近的敌人发射。"
		), fireManager(AttackInterval, projectileCount(), cooldown()) {
		DefaultDemage = 8;
		DefaultCooldown = 60;
	}
	const std::vector<std::unique_ptr<Projectile>>& update(const WeaponUpdateFunc& funcs) override {
		auto result = fireManager.get();
		if (result) {
			if (*result >= 0) {
				Vector2 direction = funcs.getNearestEnemyDirection();
				if (Vector2Length(direction) == 0.0f) {
					direction = { playerTowards ? 1.0f : -1.0f, 0.0f };
				}
				direction = Vector2Normalize(direction);
				Vector2 velocity = Vector2Scale(direction, speed());
				auto size = this->size();
				Rectangle rect = { playerPosition.x - size.x / 2.0f, playerPosition.y - size.y / 2.0f, size.x, size.y };
				float damage = DefaultDemage * stats.strength;
				projectiles.push_back(std::make_unique<HammerProjectile>(rect, damage, velocity, collideRadius()));
			}
		}
		else {
			fireManager.update(projectileCount(), cooldown());
		}
		//飞射物更新
		for (auto& projectilePtr : projectiles) {
			HammerProjectile* hammerProjectilePtr = static_cast<HammerProjectile*>(projectilePtr.get());
			hammerProjectilePtr->angle += 8;
			hammerProjectilePtr->time++;
			if (hammerProjectilePtr->time < UniformTime) {
				hammerProjectilePtr->rect.x += hammerProjectilePtr->velocity.x;
				hammerProjectilePtr->rect.y += hammerProjectilePtr->velocity.y;
			}
			else if (hammerProjectilePtr->time < AccelerateTime + UniformTime) {
				float ratio = float(UniformTime+AccelerateTime - hammerProjectilePtr->time) / float(AccelerateTime);
				hammerProjectilePtr->rect.x += hammerProjectilePtr->velocity.x * ratio;
				hammerProjectilePtr->rect.y += hammerProjectilePtr->velocity.y * ratio;
			}
			//这里反向
			else if(hammerProjectilePtr->time == AccelerateTime + UniformTime) {
				hammerProjectilePtr->velocity *= -1;
				hammerProjectilePtr->attackedEnemy.clear();	//清空攻击过的敌人
			}
			else if (hammerProjectilePtr->time < 2 * AccelerateTime + UniformTime) {
				float ratio = float(hammerProjectilePtr->time - UniformTime - AccelerateTime) / float(AccelerateTime);
				hammerProjectilePtr->rect.x += hammerProjectilePtr->velocity.x * ratio;
				hammerProjectilePtr->rect.y += hammerProjectilePtr->velocity.y * ratio;
			}
			else {
				hammerProjectilePtr->rect.x += hammerProjectilePtr->velocity.x;
				hammerProjectilePtr->rect.y += hammerProjectilePtr->velocity.y;
			}
			for(auto& [_,attackedTime] : hammerProjectilePtr->attackedEnemy) {
				attackedTime = std::max(0, attackedTime - 1);
			}
		}
		std::erase_if(projectiles, [this](const std::unique_ptr<Projectile>& projectilePtr) {
			HammerProjectile* hammerProjectilePtr = static_cast<HammerProjectile*>(projectilePtr.get());
			return hammerProjectilePtr->time >= TotalExistTime * stats.duration;
			});
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
			DefaultCount++;
			break;
		case 3:
			DefaultDemage += 5;
			break;
		case 4:
			DefaultCount++;
			break;
		case 5:
			DefaultDemage += 5;
			break;
		case 6:
			DefaultCount++;
			break;
		case 7:
			DefaultDemage += 5;
			break;
		}
		level++;
		fireManager.update(projectileCount(), cooldown());
		fireManager.reset();
	}
	void draw() const override {
		for(auto& projectilePtr : projectiles){
			HammerProjectile* projectile = static_cast<HammerProjectile*>(projectilePtr.get());
			Rectangle source = window.getTexture("hammer_effect").source;
			Rectangle dest = projectile->rect;
			DrawTexturePro(*window.getTexture("hammer_effect").sprite, source,
				{ dest.x + dest.width / 2.0f, dest.y + dest.height / 2.0f, dest.width, dest.height },
				{ dest.width / 2.0f, dest.height / 2.0f }, projectile->angle, WHITE);
		}
	}
};
//激光
class WeaponLaser : public BaseWeapon {
public:
	static constexpr std::string_view Name = "Laser";
	inline static bool registered = registerThis<WeaponLaser>();
private:
	static constexpr int AttackInterval = 1;
	static constexpr IntVector2 DefaultSize = { 58, 71 };
	static constexpr Vector2 DisplaySize = { DefaultSize.x * 1.0f, DefaultSize.y * 1.0f };
	static constexpr int ExsistTime = 24;
	static constexpr int ChargingNeedTime = 300;
	Vector2 lastPosition;
	int chargeTime = 0;
	bool charged = true;
	int DefaultCount = 1;
	int projectileCount() { return DefaultCount + stats.amount; }
	Vector2 size() { return Vector2Scale(DisplaySize, stats.area); }
	FireManager fireManager;
	class LaserProjectile : public Projectile {
	public:
		static constexpr int DefaultPushBack = 5;
		static constexpr int AttackCooldown = 15;
		const Vector2 size;
		int textureNum;
		const bool charged;
		const bool towards;
		int time = 0;
		LaserProjectile(Rectangle _rect, float _damage, Vector2 _size, int _textureNum, bool _charged, bool _towards) :
			Projectile(_rect, _damage, DefaultPushBack, AttackCooldown),
			size(_size), textureNum(_textureNum), charged(_charged), towards(_towards) {
		};
	};
public:
	WeaponLaser(const Window& _window, const PlayerStats& _stats, const Vector2& _playerPosition, const bool& _playerTowards) :
		BaseWeapon(_window, Name, _stats, _playerPosition, _playerTowards,
			"发射一束激光，持续伤害沿途的敌人。"
		), fireManager(AttackInterval, projectileCount(), cooldown()) {
		DefaultDemage = 3;
		DefaultCooldown = 90;
		lastPosition = playerPosition;
	}
	const std::vector<std::unique_ptr<Projectile>>& update(const WeaponUpdateFunc&) override {
		chargeTime++;
		//更新先前的投射物位置，防止本帧生成的错误偏移
		for (auto& projectilePtr : projectiles) {
			LaserProjectile* laserProjectilePtr = static_cast<LaserProjectile*>(projectilePtr.get());
			Vector2 posChange = Vector2Subtract(playerPosition, lastPosition);
			laserProjectilePtr->rect.x += posChange.x;
			laserProjectilePtr->rect.y += posChange.y;
		}
		auto result = fireManager.get();
		if (result) {
			if (*result == 0 && chargeTime >= ChargingNeedTime) {
				charged = true;
			}
			if (*result >= 0) {
				int textureNum = static_cast<int>(std::ceil(window.getCenter().x / size().x));
				bool towards = *result % 2 == 0;//偶数向右，奇数向左
				int totalRows;
				float rowsTemp = towards ? std::ceil(projectileCount() / 2.0f) : std::floor(projectileCount() / 2.0f);
				totalRows = static_cast<int>(rowsTemp);
				float top = playerPosition.y - size().y * totalRows / 2.0f;
				float y = top + size().y * std::floor((*result) / 2.0f);
				float x = playerPosition.x;
				x -= towards ? 0 : size().x * textureNum;
				float damage = DefaultDemage * stats.strength;
				if(charged) {
					damage *= 2;
				}
				projectiles.push_back(std::make_unique<LaserProjectile>(
					Rectangle{ x, y, size().x * textureNum, size().y }, damage, size(), textureNum, charged, towards));
			}
		}
		else {
			if(charged) {
				chargeTime = 0;
				charged = false;
			}
			fireManager.update(projectileCount(), cooldown());
		}
		//飞射物更新
		for(auto& projectilePtr : projectiles) {
			LaserProjectile* laserProjectilePtr = static_cast<LaserProjectile*>(projectilePtr.get());
			laserProjectilePtr->time++;
			for(auto& [_,attackedTime] : laserProjectilePtr->attackedEnemy) {
				attackedTime = std::max(0, attackedTime - 1);
			}
		}
		std::erase_if(projectiles, [this](const std::unique_ptr<Projectile>& projectilePtr) {
			LaserProjectile* laserProjectilePtr = static_cast<LaserProjectile*>(projectilePtr.get());
			return laserProjectilePtr->time >= ExsistTime * stats.duration;
			});
		lastPosition = playerPosition;
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
			DefaultCount++;
			break;
		case 3:
			DefaultDemage += 5;
			break;
		case 4:
			DefaultCount++;
			break;
		case 5:
			DefaultDemage += 5;
			break;
		case 6:
			DefaultCount++;
			break;
		case 7:
			DefaultDemage += 5;
			break;
		}
		level++;
		fireManager.update(projectileCount(), cooldown());
		fireManager.reset();
	}
	void draw() const override {
		for(auto& projectilePtr : projectiles) {
			LaserProjectile* projectile = static_cast<LaserProjectile*>(projectilePtr.get());
			std::string textureName = projectile->charged ? "laser_charged_0" : "laser_normal_0";
			const DrawFromSpriteSet& texture = window.getTexture(textureName);
			Rectangle source = texture.source;
			Rectangle dest = { projectile->rect.x, projectile->rect.y, projectile->size.x, projectile->size.y };
			if (!projectile->towards) {
				source.width = -source.width;
				dest.x += projectile->size.x * (projectile->textureNum - 1);
			}
			DrawTexturePro(*texture.sprite, source, dest, { 0,0 }, 0.0f, WHITE);
			for(int n = 1;n<projectile->textureNum;n++){
				if(projectile->charged) {
					textureName = "laser_charged_" + std::to_string(GetRandomValue(1, 3));
				}
				else {
					textureName = "laser_normal_1";
				}
				const DrawFromSpriteSet& texture = window.getTexture(textureName);
				Rectangle source = texture.source;
				Rectangle dest = { projectile->rect.x + projectile->size.x * n, projectile->rect.y, projectile->size.x, projectile->size.y };
				if(!projectile->towards) {
					source.width = -source.width;
					dest.x = projectile->rect.x + projectile->size.x * (projectile->textureNum - 1 - n);
				}
				DrawTexturePro(*texture.sprite, source, dest, { 0,0 }, 0.0f, WHITE);
			}
		}
	}
};