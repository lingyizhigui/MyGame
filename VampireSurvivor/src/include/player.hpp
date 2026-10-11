#pragma once
#include "raylib.h"
#include "raymath.h"
#include "common.hpp"
#include <array>
#include <cmath>
enum PlayerStatus
{
	Normal,
	Invincible,
	Dead
};
class Player {
private:
	static constexpr float VELOCITY = 5.0f;
	static constexpr int MAX_INVINCIBLE_TIME = 20;
	static constexpr char MAX_MOVE_TIME = 6;
	static constexpr char ANIMATION_CHANGE = 10;
	static constexpr int BLOOD_PARTICLE_COUNT = 128;
	static constexpr int DefaultMaxHealth = 100;
	PlayerStats stats;
	Vector2 displaySize = { 96.0f, 96.0f };
	Vector2 position = {0.0f, 0.0f};
	//一些游玩状态
	static constexpr int maxHealthBonus = 1000;
	float maxHealth;
	float health;
	float experience = 0.0f;
	unsigned int level = 1;
	char invincibleTime = 0;
	PlayerStatus status = Normal;
	//一些移动、动画状态
	char moveTime = 0;
	std::array<std::string, 4> animationFrames = { "1", "2", "3", "4" };
	char animationIndex = 0;
	char animationTime = 0;
	//受伤飙血
	struct BloodParticle {
		static constexpr float GRAVITY = 0.2f;
		Vector2 position;
		Vector2 velocity;
		int lifetime = 60;
	};
	std::vector<std::array<BloodParticle, BLOOD_PARTICLE_COUNT>> bloodParticles;
	//移动函数
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
	//经验等级函数
	int expToNextLevel() const {
		assert(level >= 1);
		int res;
		if(level<10){
			res = -5 + 10 * level;
		}
		else if(level<20){
			res = 85 + 13 * (level - 9);
		}
		else{
			res = 215 + 16 * (level - 19);
		}
		return res;
	}
	bool towards = true; // true: right, false: left
public:
	//外界需要访问的属性
	static constexpr float collisionRadius = 32.0f;
	float getPickupRadius() const { return stats.magnet * 128.0f; } //拾取范围
	std::string getAnimationFrame() const {
		return animationFrames[animationIndex];
	}
	const Vector2& getPosition() const { return position; }
	PlayerStatus getStatus() const { return status; }
	float getHealth() const { return health; }
	const PlayerStats& getStats() const { return stats; }
	const bool& getTowards() const { return towards; }
public:
	Player() : maxHealth((DefaultMaxHealth+maxHealthBonus)*stats.maxHealth), health(maxHealth) {};
	void updateBloodParticles() {
		for (auto& particleArray : bloodParticles) {
			for (auto& particle : particleArray) {
				if (particle.lifetime > 0) {
					particle.position = Vector2Add(particle.position, particle.velocity);
					particle.velocity.y += BloodParticle::GRAVITY;
					particle.lifetime--;
				}
			}
		}
		std::erase_if(
			bloodParticles,
			[](const std::array<BloodParticle, BLOOD_PARTICLE_COUNT>& particles) {
				return particles.front().lifetime <= 0;
			});
	}
	void update(float hurtDemage = 0.0f) {
		switch (status)
		{
		case Normal:
			move();
			updateBloodParticles();
			if (hurtDemage) {
				health -= std::max(hurtDemage - stats.armor, 1.0f);
				bloodParticles.push_back({});
				for (auto& particle : bloodParticles.back()) {
					particle.position = { position.x + float(GetRandomValue(-16, 16)), position.y - float(GetRandomValue(0, 128)) };
					particle.velocity = { float(GetRandomValue(-200, 200)) / 100.0f, float(GetRandomValue(-100, 0)) / 100.0f };
				}
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
			updateBloodParticles();
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
	void drawPlayer(const Window& window) const {
		Texture2D texture = *window.getTexture(getAnimationFrame()).sprite;
		Rectangle source = window.getTexture(getAnimationFrame()).source;
		if (!towards) {
			source.width = -source.width;
		}
		Color color = WHITE;
		if (status == Invincible) color = RED;
		DrawTexturePro(texture, source,
			{ position.x, position.y, displaySize.x, displaySize.y },
			{ displaySize.x / 2, displaySize.y }, 
			0.0f, color);
		for(auto& particleArray : bloodParticles){
			for(auto& particle : particleArray){
				unsigned char alpha = static_cast<unsigned char>(float(particle.lifetime) / 60.0f * 255);
				DrawRectangle(particle.position.x, particle.position.y, 4.0f, 4.0f, {255, 0, 0, alpha});
			}
		}
	}
	//血条绘制函数
	void drawHealthBar() const {
		constexpr int barWidth = 96;
		constexpr int barheight = 10;
		float x = position.x - barWidth / 2.0f;
		float y = position.y + barheight;
		DrawRectangle(x, y, barWidth, barheight, BLACK);
		float healthRatio = health / maxHealth;
		DrawRectangle(x, y, barWidth * healthRatio, barheight, GREEN);
	}
	//经验条绘制函数
	void drawExpBar(const Window& window) const {
		//1280*720已约定为显示分辨率
		constexpr int topMargin = 10;
		constexpr int boarderWidth = 1280;
		constexpr int boarderHeight = 25;
		constexpr int boarderThickness = 2;
		constexpr int barWidth = boarderWidth - 2 * boarderThickness;
		constexpr int barHeight = boarderHeight - 2 * boarderThickness;
		DrawRectangle(0, topMargin, boarderWidth, boarderHeight, ORANGE);
		DrawRectangle(boarderThickness, topMargin + boarderThickness, barWidth, barHeight, BLACK);
		float expRatio = experience / expToNextLevel();
		DrawRectangle(boarderThickness, topMargin + boarderThickness, barWidth * expRatio, barHeight, BLUE);
		std::string levelText = std::format("等级：{}  ", level);	
		DrawTextEx(window.getFont(),
			levelText.c_str(),
			{ boarderWidth - boarderThickness - MeasureTextEx(window.getFont(),levelText.c_str(),boarderHeight,3.0f).x, static_cast<float>(topMargin) },
			boarderHeight, 3.0f, WHITE);
	}
	//状态改变函数
	int getExp(int exp) {
		int changedLevel = 0;
		experience += exp * stats.growth;
		while (experience >= expToNextLevel()) {
			experience -= expToNextLevel();
			level++;
			changedLevel++;
		}
		return changedLevel;
	}
	//调试
	void levelUP() {
		level++;
	}
};