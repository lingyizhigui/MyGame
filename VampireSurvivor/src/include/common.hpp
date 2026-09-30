#pragma once
#include <vector>
#include <string>
struct PlayerStats {
	float maxHealth = 10000.0f;
	float recovery = 0.0f;
	int armor = 0;
	float moveSpeed = 1.0f;

	float strength = 4.0f;
	float projectileSpeed = 1.0f;
	float duration = 1.0f;
	float area = 1.0f;

	float cooldown = 1.0f;
	int amount = 2;
	int revival = 0;
	float magnet = 1.0f;

	float luck = 0.0f;
	float growth = 1.0f;
	float greed = 1.0f;
	float curse = 1.0f;
};