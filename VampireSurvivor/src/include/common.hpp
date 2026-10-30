#pragma once
#include "window.hpp"
#include <vector>
#include <string>
struct PlayerStats {
	float maxHealth = 1.0f;
	float recovery = 0.0f;
	int armor = 0;
	float moveSpeed = 1.0f;

	float strength = 4.0f;
	float projectileSpeed = 1.0f;
	float duration = 1.5f;
	float area = 1.0f;

	float cooldown = 1.0f;
	int amount = 1;
	int revival = 0;
	float magnet = 1.0f;

	float luck = 100.0f;
	float growth = 0.01f;
	float greed = 1.0f;
	float curse = 1.0f;
};
enum class PlayerStatsEnum {
	MaxHealth,
	Recovery,
	Armor,
	MoveSpeed,
	Strength,
	ProjectileSpeed,
	Duration,
	Area,
	Cooldown,
	Amount,
	Revival,
	Magnet,
	Luck,
	Growth,
	Greed,
	Curse,
	SizeOfEnum
};
//主动、被动物品统一基类
class BaseItem {
protected:
	const Window& window;
	const std::string_view name = "error empty name";
	const std::string_view description = "if you see this text, something goes wrong";
	char level = 0;
	char MAX_LEVEL = 8;
public:
	//注意这里的string_view
	BaseItem(const Window& _window, const std::string_view _name, const std::string_view _description) :
		window(_window), name(_name), description(_description) {
	};
	virtual ~BaseItem() = default;
	const std::string_view& getDescription() const { return description; }
	virtual const DrawFromSpriteSet& getIconTexture() const = 0;
	virtual void levelup() = 0;
	const std::string_view& getName() const { return name; }
	const char& getLevel() const { return level; }
	const Window& getWindow() const { return window; }
	bool canLevelup() const { return level < MAX_LEVEL; }
};
//武器注册池，得到所有武器的构造函数，在level中全部初始化
class BaseWeapon;
using weaponCreateFunc = std::function<std::unique_ptr<BaseWeapon>(const Window&, const PlayerStats&, const Vector2&, const bool&)>;
inline std::vector<weaponCreateFunc>& getWeaponRegisterPool() {
	static std::vector<weaponCreateFunc> pool;
	return pool;
}
inline void registerWeapon(const weaponCreateFunc& weaponCreate) {
	getWeaponRegisterPool().push_back(weaponCreate);
}
//武器更新所需要的注入函数
struct WeaponUpdateFunc
{
	std::function<Vector2()> getNearestEnemyDirection;
};
//攻击结果，默认为未造成伤害
struct AttackResult {
	float demage = 0.0f;
	bool isKilled = false;
	enum class AttackType {
		Normal,
		Critical
	} attackType = AttackType::Normal;
};