#pragma once
#include "raylib.h"
#include "raymath.h"
#include "player.hpp"
#include "window.hpp"
#include <string>
//主动、被动统一基类见common.hpp
class PassiveItem: public BaseItem {
public:
	PlayerStatsEnum statsType;
	struct PassiveDef {
		std::string_view name;
		std::string_view description;
		std::function<void(PlayerStats&)> applyEffect;
	};
private:
	PlayerStats& stats;
	static const PassiveDef& getDef(PlayerStatsEnum _statsType){
		static const std::array<PassiveDef, static_cast<int>(PlayerStatsEnum::SizeOfEnum)> passiveDefs = {
			PassiveDef{"maxHealth","增加最大生命值", [](PlayerStats& stats){ stats.maxHealth *= 0.2f; }},
			PassiveDef{"recovery","增加生命回复速度", [](PlayerStats& stats){ stats.recovery += 0.2f; }},
			PassiveDef{"armor","减少受到的伤害", [](PlayerStats& stats){ stats.armor += 1; }},
			PassiveDef{"moveSpeed","增加移动速度", [](PlayerStats& stats){ stats.moveSpeed += 0.1f; }},
			PassiveDef{"strength","增加武器伤害", [](PlayerStats& stats){ stats.strength += 0.1f; }},
			PassiveDef{"projectileSpeed","增加投射物速度", [](PlayerStats& stats){ stats.projectileSpeed += 0.1f; }},
			PassiveDef{"duration","增加投射物持续时间", [](PlayerStats& stats){ stats.duration += 0.1f; }},
			PassiveDef{"area","增加投射物范围", [](PlayerStats& stats){ stats.area += 0.1f; }},
			PassiveDef{"cooldown","减少武器冷却时间", [](PlayerStats& stats){ stats.cooldown -= 0.08f; }},
			PassiveDef{"amount","增加投射物数量", [](PlayerStats& stats){ stats.amount += 1; }},
			PassiveDef{"revival","增加复活次数", [](PlayerStats& stats){ stats.revival += 1; }},
			PassiveDef{"magnet","增加拾取范围", [](PlayerStats& stats){ stats.magnet += 0.2f; }},
			PassiveDef{"luck","增加幸运", [](PlayerStats& stats){ stats.luck *= 1.1f; }},
			PassiveDef{"growth","增加经验获取速率", [](PlayerStats& stats){ stats.growth += 0.1f; }},
			PassiveDef{"greed","增加金币掉落", [](PlayerStats& stats){ stats.greed += 0.1f; }},
			PassiveDef{"curse","诅咒：增加敌人数量", [](PlayerStats& stats){ stats.curse += 0.1f; }}
		};
		return passiveDefs[static_cast<int>(_statsType)];
	}
	const std::string_view& statsToSting(PlayerStatsEnum _statsType) const {
		return getDef(_statsType).name;
	}
	const std::string_view& statsToDescription(PlayerStatsEnum _statsType) const {
		return getDef(_statsType).description;
	}
public:
	static const std::string_view getStaticName(PlayerStatsEnum _statsType) { return getDef(_statsType).name; }
	PassiveItem(const Window& _window, PlayerStatsEnum _statsType, PlayerStats& _stats) :
		BaseItem(_window, statsToSting(_statsType), statsToDescription(_statsType)), 
		statsType(_statsType), stats(_stats) {
		MAX_LEVEL = 5;
		if (statsType == PlayerStatsEnum::Revival||statsType==PlayerStatsEnum::Amount) 
			MAX_LEVEL = 2;
		getDef(statsType).applyEffect(stats);
	};
	const DrawFromSpriteSet& getIconTexture() const override {
		return window.getTexture(std::string(statsToSting(statsType)));
	}
	void levelup() override {
		if (level < MAX_LEVEL) {
			level++;
			getDef(statsType).applyEffect(stats);
		}
	}
};
//特殊item，升级选项中的鸡腿与金币
class levelupItem : public BaseItem {
private:
	Player& player;
public:
	enum class Type {
		Chicken,
		CoinBag
	} type;
	struct LevelupDef {
		std::string_view name;
		std::string_view description;
		std::function<void(Player&)> applyEffect;
	};
	static const LevelupDef& getDef(Type _type) {
		static const std::array<LevelupDef, 2> levelupDefs = {
			LevelupDef{"chicken","回复30生命值", [](Player& player) { player.Heal(30.0f); }},
			LevelupDef{"large coin bag","获得25金币", [](Player& player) { player.gainCoin(25); }}
		};
		return levelupDefs[static_cast<int>(_type)];
	}
	levelupItem(const Window& _window, Type _type, Player& _player) :
		BaseItem(_window, getDef(_type).name, getDef(_type).description),
		type(_type), player(_player) {
	};
	const DrawFromSpriteSet& getIconTexture() const override {
		return window.getTexture(std::string(getDef(type).name));
	}
	void levelup() override {
		getDef(type).applyEffect(player);
	}
};
//拾取物基类
struct PickupResult {
	int playerLevelupCount = 0;
};
class Pickup {
protected:
	static constexpr float MAGNET_SPEED = 10.0f;
	static constexpr int FLOAT_TIME = 30;
	std::string textureName;
	Vector2 position;
	float collisionRadius;
	Vector2 displaySize = { 16.0f, 16.0f };
	enum class state {
		lying,
		collecting,
		collected
	} currentState = state::lying;
	int floatTime = FLOAT_TIME;//向外浮动时间
public:
	Pickup(const std::string& _textureName, const Vector2& _position, float _collisionRadius) :
		textureName(_textureName), position(_position), collisionRadius(_collisionRadius) {
	};
	bool isAlive() const {
		return currentState != state::collected;
	}
	virtual ~Pickup() = default;
	virtual PickupResult applyEffect(Player&) = 0;
	virtual bool isColliding(const Player&) const = 0;
	PickupResult update(Player& player) {
		if(isColliding(player)) {
			return applyEffect(player);
		}
		return {};
	};
	virtual void draw(const Window& window) const = 0;
};
//经验宝石
class ExperienceGem : public Pickup {
private:
	int expAmount;
	enum class GemType {
		Blue,
		Green,
		Red
	} gemType = GemType::Blue;
public:
	ExperienceGem(Vector2 _position, int _expAmount) :
		Pickup("experience_gem_blue", _position, 16.0f),
		expAmount(_expAmount) {
		displaySize = { 32.0f, 32.0f };
		if (expAmount > 10) {
			gemType = GemType::Red;
			textureName = "experience_gem_red";
		}
		else if (expAmount > 2) {
			gemType = GemType::Green;
			textureName = "experience_gem_green";
		}
	}
	bool isColliding(const Player& player) const override {
		if (currentState == state::lying) {
			Vector2 playerPos = player.getPosition();
			float distance = Vector2Distance(player.getPosition(), position);
			return distance < (collisionRadius + player.getPickupRadius());
		}
		else return true;
	}
	virtual PickupResult applyEffect(Player& player) override {
		PickupResult result;
		if(currentState == state::lying) {
			currentState = state::collecting;
		}
		else if(currentState == state::collecting) {
			if(floatTime > 0) {
				floatTime--;
				Vector2 direction = Vector2Normalize(player.getPosition() - position);
				float velocity;
				int index = FLOAT_TIME - floatTime;
				if (index * 3 < FLOAT_TIME)
					velocity = MAGNET_SPEED * -(index * 3.0f) / FLOAT_TIME;
				else if (index * 3 < FLOAT_TIME * 2)
					velocity = MAGNET_SPEED * (index * 3.0f - FLOAT_TIME * 2.0f) / FLOAT_TIME;
				else {
					velocity = MAGNET_SPEED * (index * 3.0f - FLOAT_TIME * 2.0f) / FLOAT_TIME;
					velocity = std::min(velocity, Vector2Distance(player.getPosition(), position));//防止过冲，只有第三阶段需要
				}
				position += Vector2Scale(direction, velocity);
			}
			else {
				float distance = Vector2Distance(player.getPosition(), position);
				if(distance<collisionRadius){
					int levels = player.gainExp(expAmount);
					currentState = state::collected;
					//这里真正操作返回值
					result.playerLevelupCount = levels;
					return result;
				}
				else {
					Vector2 direction = Vector2Normalize(player.getPosition() - position);
					float pace = std::min(MAGNET_SPEED, distance);
					position += Vector2Scale(direction, pace);
				}
			}
		}
		else if(currentState == state::collected) {
			// Do nothing, already collected
		}
		return result;
	}
	void draw(const Window& window) const override {
		if (currentState != state::collected) {
			auto drawSet = window.getTexture(textureName);
			Rectangle source=drawSet.source;
			Texture2D* texture = drawSet.sprite;
			DrawTexturePro(*texture, source,
				{ position.x, position.y, displaySize.x, displaySize.y },
				{ displaySize.x / 2.0f, displaySize.y },
				0.0f, WHITE);
		}
	}
};
