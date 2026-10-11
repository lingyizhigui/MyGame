#pragma once
#include "raylib.h"
#include "raymath.h"
#include "player.hpp"
#include "window.hpp"
#include <string>
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
	virtual ~Pickup() = default;
	virtual void applyEffect(Player&) = 0;
	virtual bool isColliding(const Player&) const = 0;
	void update(Player& player) {
		if(isColliding(player)) {
			applyEffect(player);
		}
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
	void applyEffect(Player& player) override {
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
					player.getExp(expAmount);
					currentState = state::collected;
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
