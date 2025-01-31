#pragma once

#include <string>
#include <vector>
#include <chrono>

#include "moduleManager/moduleBase.h"

class ChestStealer : public ModuleBase
{
public:
	void Update() override;
	void RenderUpdate() override;

	void RenderMenu() override;

	bool IsEnabled() override { return settings::CS_Enabled; }
	std::string GetName() override { return Name; }
	std::string GetCategory() override { return Category; }

private:
	bool isStealing;
	std::chrono::time_point<std::chrono::steady_clock> lastStealTime;
	std::chrono::time_point<std::chrono::steady_clock> activated;
	std::vector<int> chestSlots;
	int chestSlotIndex = 0;

	void ResetSteal();

	void RenderItems(bool& isOpen);

private:
	std::string Name = "Chest Stealer";
	std::string Category = "Inventory";
};