#pragma once
#include <vector>
#include <string>
struct PlayerStats {
	float maxHealth = 100.0f;
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
	float magnet = 0.0f;

	float luck = 0.0f;
	float growth = 0.0f;
	float greed = 0.0f;
	float curse = 0.0f;
};