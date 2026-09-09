#pragma once
#include "raylib.h"
#include "raymath.h"
#include "framework.h"
#include <algorithm>
#include <string>
struct Ball {
	static const int MaxVelocity = 16;
	static const int FastVelocity = 6;
	Vector2 pos;
	Vector2 speed;
	std::array<Vector2, 16> tails;
	int index = 0;
	static constexpr int radius = 22;
	static constexpr int width = 44;
	static constexpr int height = 44;
	Ball(Vector2 _positon, Vector2 _speed) :pos(_positon), speed(_speed) {};
	void changeSpeed(float n) {
		speed = Vector2Scale(speed, n);
		if (Vector2Length(speed) > MaxVelocity) {
			speed = Vector2Normalize(speed);
			speed = Vector2Scale(speed, MaxVelocity);
		}
	}
	void go() {
		tails[index] = pos;
		index = (index + 1) % tails.size();
		pos = Vector2Add(pos, speed);
		if (Vector2Length(speed) > FastVelocity) {
			speed *= 0.998f;
		}
	}
	void move(Vector2 path) {
		pos = Vector2Add(pos, path);
	}
	Vector2 isCollideAsRect(const Rectangle& rect) const {
		Rectangle ballRect(pos.x - width / 2, pos.y - height / 2, width, height);
		float overlapX = std::min(rect.x + rect.width, ballRect.x + ballRect.width) - std::max(rect.x, ballRect.x);
		float overlapY = std::min(rect.y + rect.height, ballRect.y + ballRect.height) - std::max(rect.y, ballRect.y);
		if (overlapX <= 0 || overlapY <= 0) return{ 0.0f,0.0f };
		else if (overlapX < overlapY) return pos.x > rect.x + rect.width / 2 ? Vector2{ overlapX, 0.0f } : Vector2{ overlapX * -1, 0.0f };
		else return pos.y > rect.y + rect.height / 2 ? Vector2{ 0.0f,overlapY } : Vector2{ 0.0f,overlapY * -1 };
	}
	Vector2 isCollideInside(const Rectangle& rect) const {
		Rectangle ballRect(pos.x - width / 2, pos.y - height / 2, width, height);
		if (!(ballRect.x > rect.x && ballRect.x + ballRect.width < rect.x + rect.width &&
			ballRect.y > rect.y && ballRect.y + ballRect.height < rect.y + rect.height)) {
			float overX = pos.x > rect.x + rect.width / 2
				? std::min(rect.x + rect.width - (ballRect.x + ballRect.width), 0.0f)
				: std::max(rect.x - ballRect.x, 0.0f);
			float overY = pos.y > rect.y + rect.height / 2
				? std::min(rect.y + rect.height - (ballRect.y + ballRect.height), 0.0f)
				: std::max(rect.y - ballRect.y, 0.0f);
			if (std::abs(overX) > std::abs(overY)) return{ overX,0.0f };
			else return{ 0.0f,overY };
		}
		return { 0,0 };
	}
	Vector2 isCollide(const Rectangle& rect) const {
		float closestX = std::max(rect.x, std::min(rect.x + rect.width, pos.x));
		float closestY = std::max(rect.y, std::min(rect.y + rect.height, pos.y));
		float distance = Vector2Distance({ closestX,closestY }, pos);
		if (distance > radius) return { 0,0 };
		else if (distance > 0) {
			float deepth = radius - distance;
			Vector2 normal = Vector2Normalize(Vector2Subtract(pos, { closestX, closestY }));
			return Vector2Scale(normal, deepth);
		}
		else {
			float distLeft = pos.x - rect.x;
			float distRight = rect.x + rect.width - pos.x;
			float distTop = pos.y - rect.y;
			float distBottom = rect.y + rect.height - pos.y;
			float minDist = distLeft;
			Vector2 normal = { -1.0f, 0.0f };
			if (distRight < minDist) { minDist = distRight; normal = { 1.0f, 0.0f }; }
			if (distTop < minDist) { minDist = distTop;   normal = { 0.0f, -1.0f }; }
			if (distBottom < minDist) { minDist = distBottom; normal = { 0.0f, 1.0f }; }
			return Vector2Scale(normal, minDist + radius);
		}
	}
	Vector2 isCollideRotated(const Rectangle& rect, float angle) {
		float rad = angle * DEG2RAD;
		Vector2 realPos = pos;
		pos = Vector2Rotate(pos, rad * -1);
		Vector2 rotatedRect = Vector2Rotate({ rect.x + rect.width / 2,rect.y + rect.height / 2 }, rad * -1);
		Vector2 res = isCollide({ rotatedRect.x - rect.width / 2,rotatedRect.y - rect.height / 2,rect.width,rect.height });
		pos = realPos;
		return Vector2Rotate(res, rad);
	}
	void collide(const Vector2& plane, float otherVel = 0) {
		pos = Vector2Add(pos, Vector2Scale(plane, 1.1));
		float temp = Vector2DotProduct(plane, speed);
		if (temp < 0) {
			float deltaV = temp * 2 / Vector2Length(plane);
			if (otherVel > 0) deltaV -= otherVel * 0.3;
			speed = Vector2Subtract(speed, Vector2Scale(Vector2Normalize(plane), deltaV));
			if (Vector2Length(speed) > MaxVelocity) {
				speed = Vector2Normalize(speed);
				speed = Vector2Scale(speed, MaxVelocity);
			}
			if (Vector2Length(speed) > FastVelocity) {
				speed *= 0.95;
			}
		}
	}
};
struct Brick {
	Vector2 pos;
	char status = 0;
	char hp = 4;
	static constexpr char Normal = 1;
	static constexpr char Colorful = 2;
	static constexpr char Bomb = 3;
	Brick(Vector2 v = { 0.0f,0.0f }) :pos(v) {};
	void drawBreak() const{
		if (hp <= 2) {
			DrawLineEx(pos, { pos.x + 64.0f,pos.y + 64.0f }, 20, BLACK);
		}
	}
	void draw(const Window& window) const {
		std::string path = "broke_brick_" + std::to_string(hp);
		const auto& r = window.posMap.find(path)->second;
		DrawTextureRec(window.bricksImg, r, pos, WHITE);
	}
};
struct Particle {
	Vector2 pos;
	int height;
	int width;
	Vector2 velocity;
	int life = 30;
	static constexpr float gravity = 0.5f;
	Particle(Vector2 _position) {
		pos = { _position.x + GetRandomValue(-16,16),_position.y + GetRandomValue(-16,16) };
		height = GetRandomValue(1, 4);
		width = GetRandomValue(1, 4);
		int speed = GetRandomValue(40, 100) / 10;
		float angle = GetRandomValue(1, 360) * DEG2RAD;
		velocity = Vector2Rotate(Vector2(1, 0) * speed, angle);
	}
	void go() {
		velocity.y += gravity;
		pos = Vector2Add(pos, velocity);
		life--;
	}
	void draw() const{
		DrawRectangle(pos.x, pos.y, width, height, RAYWHITE);
	}
};
struct Paddle {
	Vector2 pos;
	Vector2 speed = { 0.0f,0.0f };
	float angle = 0;
	float targetAngle = 0;
	static constexpr float velocity = 2.5f;
	static constexpr int width = 208;
	static constexpr int height = 48;
	Paddle() {
		pos = { 0,0 };
	}
	Paddle(Vector2 _position) {
		pos.x = _position.x - (float)width / 2;
		pos.y = _position.y - (float)height / 2;
	}
	void update(Vector2 _position) {
		_position = { _position.x - (float)width / 2,_position.y - (float)height / 2 };
		speed = Vector2Subtract(_position, pos);
		pos = _position;
		if (speed.x > 5) {
			targetAngle = std::min(speed.x-5,60.0f);
		}
		else if (speed.x < -5) {
			targetAngle = std::max(speed.x + 5, -60.0f);
		}
		else {
			targetAngle = 0;
		}
		float change = 0;
		if (std::abs(targetAngle) > std::abs(angle)) {
			change = std::min(velocity * 2, std::abs(targetAngle - angle));
			angle = targetAngle > angle ? angle + change : angle - change;
		}
		else {
			change = std::min(velocity, std::abs(angle));
			angle = targetAngle > angle ? angle + change : angle - change;
		}
		
	}
};