#pragma once
#include "raylib.h"
#include <array>
#include <cmath>
struct PlayerStats {
	float maxHealth = 100.0f;
	float recovery = 0.0f;
	int armor = 0;
	float moveSpeed = 1.0f;

	float strength = 1.0f;
	float projectileSpeed = 1.0f;
	float duration = 1.0f;
	float area = 1.0f;
	
	float cooldown = 0.0f;
	int amount = 0;
	int revival = 0;
	float magnet = 0.0f;

	float luck = 0.0f;
	float growth = 0.0f;
	float greed = 0.0f;
	float curse = 0.0f;
};
enum PlayerStatus
{
	Normal,
	Invincible,
	Dead
};
class Player {
private:
	static constexpr float VELOCITY = 5.0f;
	static constexpr int MAX_INVINCIBLE_TIME = 4;
	static constexpr char MAX_MOVE_TIME = 6;
	static constexpr char ANIMATION_CHANGE = 10;
	PlayerStats stats;
	Vector2 position = {0.0f, 0.0f};
	float health;
	char moveTime = 0;
	char invincibleTime = 0;
public:
	bool towards = true; // true: right, false: left
private:
	PlayerStatus status = Normal;
	std::array<std::string, 4> animationFrames = { "1", "2", "3", "4" };
	char animationIndex = 0;
	char animationTime = 0;
	void move() {
		float dx = 0.0f;
		float dy = 0.0f;
		if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) dy -= 1.0f;
		if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) dy += 1.0f;
		if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
			dx -= 1.0f;
			towards = false;
		}
		if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
			dx += 1.0f;
			towards = true;
		}
		if(dx!=0.0f || dy!=0.0f){
			float length = std::sqrt(dx * dx + dy * dy);
			dx /= length;
			dy /= length;
			position.x += dx * stats.moveSpeed * VELOCITY;
			position.y += dy * stats.moveSpeed * VELOCITY;
			if (moveTime < MAX_MOVE_TIME) moveTime++;
			animationTime++;
			if(animationTime>ANIMATION_CHANGE){
				animationTime = 0;
				animationIndex = char(animationIndex + 1) % animationFrames.size();
			}
		}
		else {
			moveTime = 0;
			animationTime = 0;
		}
		
	}
public:
	Player() : health(stats.maxHealth) {}
	void update(float hurtDemage = 0.0f) {
		switch (status)
		{
		case Normal:
			move();
			if (hurtDemage) {
				health -= std::max(hurtDemage - stats.armor, 1.0f);
				if (health <= 0.0f)
					status = Dead;
				else {
					status = Invincible;
					invincibleTime = MAX_INVINCIBLE_TIME;
				}
			}
			break;
		case Invincible:
			move();
			if (invincibleTime > 0) {
				invincibleTime--;
			}
			else {
				status = Normal;
			}
			break;
		case Dead:
			return;
			break;
		}
	}
	std::string getAnimationFrame() const {
		return animationFrames[animationIndex];
	}
	Vector2 getPosition() const { return position; }
	PlayerStatus getStatus() const { return status; }
	float getHealth() const { return health; }
	float getMaxHealth() const { return stats.maxHealth; }
	PlayerStats& getStats() { return stats; }
};