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
	PlayerStats stats;
	Vector2 displaySize = { 96.0f, 96.0f };
	Vector2 position = {0.0f, 0.0f};
	float health;
	char moveTime = 0;
	char invincibleTime = 0;
	struct BloodParticle {
		static constexpr float GRAVITY = 0.2f;
		Vector2 position;
		Vector2 velocity;
		int lifetime = 60;
	};
	std::vector<std::array<BloodParticle, BLOOD_PARTICLE_COUNT>> bloodParticles;
public:
	bool towards = true; // true: right, false: left
	float collisionRadius = 32.0f;
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
	Player() : health(stats.maxHealth) {};
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
	std::string getAnimationFrame() const {
		return animationFrames[animationIndex];
	}
	const Vector2& getPosition() const { return position; }
	PlayerStatus getStatus() const { return status; }
	float getHealth() const { return health; }
	float getMaxHealth() const { return stats.maxHealth; }
	const PlayerStats& getStats() const{ return stats; }
	const bool& getTowards() const { return towards; }
};