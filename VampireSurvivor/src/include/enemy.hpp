#pragma once
#include "raylib.h"
#include <string>
#include <vector>
class EnemyDef {
private:
	std::string name;
	std::vector<std::string> animationFrames;
	std::vector<
	float speed;
	

public:

};
struct Enemy {
	Vector2 position;
	int health;
	bool towards;	// true: right, false: left
	bool alive;
};