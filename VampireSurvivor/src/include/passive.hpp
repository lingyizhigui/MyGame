#pragma once
#include "raylib.h"
#include "raymath.h"
#include "player.hpp"
#include "window.hpp"
#include <string>
//主动、被动统一基类见common.hpp
class PassiveItem: public BaseItem {
public:
	PlayerStatsEnum statsType;
	struct PassiveDef {
		std::string_view name;
		std::string_view description;
		std::function<void(PlayerStats&)> applyEffect;
	};
private:
	PlayerStats& stats;
	static const PassiveDef& getDef(PlayerStatsEnum _statsType){
		static const std::array<PassiveDef, static_cast<int>(PlayerStatsEnum::SizeOfEnum)> passiveDefs = {
			PassiveDef{"maxHealth","增加最大生命值", [](PlayerStats& stats){ stats.maxHealth *= 1.2f; }},
			PassiveDef{"recovery","增加生命回复速度", [](PlayerStats& stats){ stats.recovery += 0.2f; }},
			PassiveDef{"armor","减少受到的伤害", [](PlayerStats& stats){ stats.armor += 1; }},
			PassiveDef{"moveSpeed","增加移动速度", [](PlayerStats& stats){ stats.moveSpeed += 0.1f; }},
			PassiveDef{"strength","增加武器伤害", [](PlayerStats& stats){ stats.strength += 0.1f; }},
			PassiveDef{"projectileSpeed","增加投射物速度", [](PlayerStats& stats){ stats.projectileSpeed += 0.1f; }},
			PassiveDef{"duration","增加投射物持续时间", [](PlayerStats& stats){ stats.duration += 0.1f; }},
			PassiveDef{"area","增加投射物范围", [](PlayerStats& stats){ stats.area += 0.1f; }},
			PassiveDef{"cooldown","减少武器冷却时间", [](PlayerStats& stats){ stats.cooldown -= 0.08f; }},
			PassiveDef{"amount","增加投射物数量", [](PlayerStats& stats){ stats.amount += 1; }},
			PassiveDef{"revival","增加复活次数", [](PlayerStats& stats){ stats.revival += 1; }},
			PassiveDef{"magnet","增加拾取范围", [](PlayerStats& stats){ stats.magnet += 0.2f; }},
			PassiveDef{"luck","增加幸运", [](PlayerStats& stats){ stats.luck *= 1.1f; }},
			PassiveDef{"growth","增加经验获取速率", [](PlayerStats& stats){ stats.growth += 0.1f; }},
			PassiveDef{"greed","增加金币掉落", [](PlayerStats& stats){ stats.greed += 0.1f; }},
			PassiveDef{"curse","诅咒：增加敌人数量", [](PlayerStats& stats){ stats.curse += 0.1f; }}
		};
		return passiveDefs[static_cast<int>(_statsType)];
	}
	static const std::string_view& statsToSting(PlayerStatsEnum _statsType) {
		return getDef(_statsType).name;
	}
	static const std::string_view& statsToDescription(PlayerStatsEnum _statsType) {
		return getDef(_statsType).description;
	}
public:
	static const std::string_view getStaticName(PlayerStatsEnum _statsType) { return getDef(_statsType).name; }
	PassiveItem(const Window& _window, PlayerStatsEnum _statsType, PlayerStats& _stats) :
		BaseItem(_window, statsToSting(_statsType), statsToDescription(_statsType)), 
		statsType(_statsType), stats(_stats) {
		MAX_LEVEL = 5;
		if (statsType == PlayerStatsEnum::Revival||statsType==PlayerStatsEnum::Amount) 
			MAX_LEVEL = 2;
	};
	const DrawFromSpriteSet& getIconTexture() const override {
		return window.getTexture(std::string(statsToSting(statsType)));
	}
	void levelup() override {
		if (level < MAX_LEVEL) {
			level++;
			getDef(statsType).applyEffect(stats);
		}
	}
};
//特殊item，升级选项中的鸡腿与金币
class levelupItem : public BaseItem {
private:
	Player& player;
public:
	enum class Type {
		Chicken,
		CoinBag
	} type;
	struct LevelupDef {
		std::string_view name;
		std::string_view description;
		std::function<void(Player&)> applyEffect;
	};
	static const LevelupDef& getDef(Type _type) {
		static const std::array<LevelupDef, 2> levelupDefs = {
			LevelupDef{"chicken","回复30生命值", [](Player& player) { player.Heal(30.0f); }},
			LevelupDef{"large coin bag","获得25金币", [](Player& player) { player.gainCoin(25); }}
		};
		return levelupDefs[static_cast<int>(_type)];
	}
	levelupItem(const Window& _window, Type _type, Player& _player) :
		BaseItem(_window, getDef(_type).name, getDef(_type).description),
		type(_type), player(_player) {
	};
	const DrawFromSpriteSet& getIconTexture() const override {
		return window.getTexture(std::string(getDef(type).name));
	}
	void levelup() override {
		getDef(type).applyEffect(player);
	}
};
