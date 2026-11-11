#pragma once
#include "raylib.h"
#include "raymath.h"
#include "player.hpp"
#include "window.hpp"
#include <string>
struct PickupResult {
	int playerLevelupCount = 0;
	int timeFreezeTime = 0;
	int fireBreathTime = 0;
	bool gemMagnet = false;
	bool rosaryKill = false;
};
//实体对象基类，拥有位置和贴图
class WorldObject {
protected:
	std::string_view textureName;
	Vector2 position;
	Vector2 displaySize = { 16.0f, 16.0f };
public:
	WorldObject(const std::string_view _textureName, const Vector2& _position) :
		textureName(_textureName), position(_position) {
	};
	void moveTo(const Vector2& _position) {
		position = _position;
	}
};
//拾取物基类
class FloatPickup: public WorldObject {
protected:
	static constexpr float MAGNET_SPEED = 10.0f;
	static constexpr int FLOAT_TIME = 30;
	float collisionRadius = displaySize.x * 0.7f;
	enum class state {
		lying,
		collecting,
		collected
	} currentState = state::lying;
	int floatTime = FLOAT_TIME;//向外浮动时间
public:
	FloatPickup(const std::string_view _textureName, const Vector2& _position) :
		WorldObject(_textureName, _position) {
	};
	bool isAlive() const {
		return currentState != state::collected;
	}
	bool isLying() const {
		return currentState == state::lying;
	}
	virtual ~FloatPickup() = default;
	virtual PickupResult applyEffect(Player&) = 0;
	virtual bool canBeMagnet(const Player&) const = 0;
	PickupResult update(Player& player) {
		if (currentState == state::collected)
			return {};
		else if (currentState == state::collecting)
			//collecting必然是浮动中，以下是动画处理
			if (floatTime > 0) {
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
				if (distance < collisionRadius) {
					//拾取物被拾取
					currentState = state::collected;
					return applyEffect(player);
				}
				else {
					Vector2 direction = Vector2Normalize(player.getPosition() - position);
					float pace = std::min(MAGNET_SPEED, distance);
					position += Vector2Scale(direction, pace);
				}
			}
		else {
			float distance = Vector2Distance(player.getPosition(), position);
			if(canBeMagnet(player)) {
				if(distance < (collisionRadius + player.getPickupRadius()))
					currentState = state::collecting;
			}
			else {
				//如果不能吸引，踩到的时候也会强制拾取（基本用于鸡腿）
				if(distance<collisionRadius) {
					currentState = state::collected;
					return applyEffect(player);
				}
			}
		}
		return {};
	};
	void draw(const Window& window) const {
		if (currentState != state::collected) {
			auto& drawSet = window.getTexture(std::string{ textureName });
			Rectangle source = drawSet.source;
			Texture2D* texture = drawSet.sprite;
			DrawTexturePro(*texture, source,
				{ position.x, position.y, displaySize.x, displaySize.y },
				{ displaySize.x / 2.0f, displaySize.y / 2.0f },
				0.0f, WHITE);
		}
	}
};
//经验宝石
class ExperienceGem : public FloatPickup {
private:
	int expAmount;
	enum class GemType {
		Blue,
		Green,
		Red
	} gemType = GemType::Blue;
public:
	ExperienceGem(Vector2 _position, int _expAmount) :
		FloatPickup("experience_gem_blue", _position),
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
	PickupResult applyEffect(Player& player) override {
		PickupResult result;
		int levels = player.gainExp(expAmount);
		result.playerLevelupCount = levels;
		return result;

	}
	bool canBeMagnet(const Player&) const override {
		return true;
	}
	void beMagnet() {
		currentState = state::collecting;	//目前只有经验宝石有磁铁吸引
	}
};
//大多数拾取物
class Pickup : public FloatPickup {
public:
	enum class Type {
		Coin,
		CoinBag,
		RichCoinBag,
		Chicken,
		TheWorld,
		Rosary,
		FireBreath,
		Magnet,
		SizeOfEnum
	} type;
private:
	struct Def
	{
		std::string_view name;
		std::string_view textureName;
		std::function<PickupResult(Player&)> applyEffect;
	};
	static const Def& getDef(Type _type) {
		static const std::array<Def, static_cast<int>(Type::SizeOfEnum)> defs = {
			Def{"金币","GoldCoin", [](Player& player) { player.gainCoin(1); return PickupResult{}; }},
			Def{"钱袋","CoinBag", [](Player& player) { player.gainCoin(5); return PickupResult{}; }},
			Def{"大钱袋","RichCoinBag", [](Player& player) { player.gainCoin(25); return PickupResult{}; }},
			Def{"鸡腿","RoastChicken", [](Player& player) { player.Heal(30.0f); return PickupResult{}; }},
			Def{"【世界】","TheWorld", [](Player& player) { return PickupResult{.timeFreezeTime = 300}; }},
			Def{"十字架","Rosary", [](Player& player) {return PickupResult{.rosaryKill = true}; }},
			Def{"火焰喷射","FireBreath", [](Player& player) { return PickupResult{.fireBreathTime = 300}; }},
			Def{"磁铁","GemMagnet", [](Player& player) { return PickupResult{.gemMagnet = true}; }}
		};
		return defs[static_cast<int>(_type)];
	}
public:
	Pickup(Type _type, Vector2 _position) :
		FloatPickup(getDef(_type).textureName, _position),
		type(_type) {
		displaySize = { 64.0f, 64.0f };
	}
	PickupResult applyEffect(Player& player) override {
		return getDef(type).applyEffect(player);
	}
	bool canBeMagnet(const Player& player) const override {
		if(type != Type::Chicken) {
			return true;
		}
		else {
			if (player.getLostHealth())
				return true;
			else
				return false;
		}
	}
};