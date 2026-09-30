#pragma once
#include "raylib.h"
#include <string>
#include <unordered_map>
#include <vector>
#include "json.hpp"
struct EnemyDef {
	std::string name;
	std::vector<std::string> moveFrames;
	std::vector<std::string> deathFrames;
	int width;
	int height;
	float maxHealth;
	float power;
	float speed;
	float collideRadius;
	float hurtRadius;
public:
	EnemyDef(std::string _name, 
		std::vector<std::string> _moveFrames, 
		std::vector<std::string> _deathFrames, 
		int _width, 
		int _height, 
		float _speed, 
		float _maxHealth, 
		float _power,
		float _collideRadius,
		float _hurtRadius) :
		name(_name),
		moveFrames(_moveFrames),
		deathFrames(_deathFrames),
		width(_width),
		height(_height),	
		maxHealth(_maxHealth),
		power(_power),
		speed(_speed),
		collideRadius(_collideRadius),
		hurtRadius(_hurtRadius)
	{
	};
};
struct Enemy {
	const EnemyDef* def;
	Vector2 position;
	Vector2 direction = {0.0f, 0.0f};
	int health;
	bool towards;	// true: right, false: left
	char status;	//0:dead, 1:alive,2:dying,
	char frame;
	char changeTime;
};
class EnemyGenerator {
private:
	//const Window& window;
	std::unordered_map<std::string, EnemyDef> enemyDefs;
public:
	EnemyGenerator(std::string jsonPath){
		nlohmann::json jsonData;
		std::fstream file(jsonPath);
		file >> jsonData;
		std::vector<std::string> moveFrames;
		std::vector<std::string> deathFrames;
		for (const auto& frame : jsonData["enemies"]) {
			std::string name = frame["name"];
			for (const auto& move : frame["move_frames"]) {
				moveFrames.push_back(move);
			}
			for (const auto& death : frame["death_frames"]) {
				deathFrames.push_back(death);
			}
			int width = frame["width"];
			int height = frame["height"];
			float health = frame["health"].is_null() ? 20.0f : frame["health"].get<float>();
			float power = frame["power"].is_null() ? 5.0f : frame["power"].get<float>();
			float speed = frame["speed"].is_null() ? 2.0f : frame["speed"].get<float>();
			float collideRadius = frame.value("collideRadius", 20.0f);
			float hurtRadius = frame.value("hurt_radius", 20.0f);
			EnemyDef def(name, moveFrames, deathFrames, width, height, speed, health, power, collideRadius, hurtRadius);
			enemyDefs.emplace(def.name, def);
			moveFrames.clear();
			deathFrames.clear();
		}
	}
	Enemy spawn(std::string _enemyName,Vector2 _positon) {
		const EnemyDef& def = enemyDefs.at(_enemyName);
		Enemy e{
			&def,
			_positon,
			{0.0f, 0.0f},
			static_cast<int>(def.maxHealth),
			true,	//towards: right
			1,	//status: alive
			0	//frame index
		};
		return e;
	}
};
