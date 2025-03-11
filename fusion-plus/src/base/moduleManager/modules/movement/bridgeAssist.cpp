#include "bridgeAssist.h"
#include <imgui/imgui.h>
#include <menu/menu.h>
#include <util/logger/logger.h>
#include <moduleManager/commonData.h>
#include <random>

inline static void send_key(WORD vk_key, bool send_down = true)
{
	static INPUT ip{ INPUT_KEYBOARD };

	ip.ki.wScan = 0;
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;
	ip.ki.wVk = vk_key;
	ip.ki.dwFlags = send_down ? 0 : KEYEVENTF_KEYUP;

	SendInput(1, &ip, sizeof(INPUT));
}

static void SendShiftKey(HWND hWnd, bool send_down) {
	if (send_down)
	{
		PostMessage(hWnd, WM_KEYDOWN, VK_SHIFT, static_cast<LPARAM>(MapVirtualKey(VK_SHIFT, 0) << 16));
	}
	else
	{
		PostMessage(hWnd, WM_KEYUP, VK_SHIFT, (static_cast<LPARAM>(MapVirtualKey(VK_SHIFT, 0)) << 16) | (static_cast<long long>(1) << 30) | (static_cast<long long>(1) << 31));
	}
}

void BridgeAssist::Update() // Thanks to Steve987321 @ https://github.com/Steve987321/toadclient for the idea / implementation.
{
	bool isSneaking = (bool)GetAsyncKeyState(VK_SHIFT);

	if (!settings::BA_Enabled || !CommonData::SanityCheck() || SDK::Minecraft->IsInGuiState())
	{
		if (!m_has_pressed_shift && isSneaking && !settings::BA_OnlyOnShift)
		{
			UnSneak();
		}

		m_has_pressed_shift = false;
		m_is_bridging = false;
		return;
	}

	if (settings::BA_IgnoreForwardsMovement && (GetAsyncKeyState(0x57) & 0x8000))
	{
		if (!m_has_pressed_shift && isSneaking && !settings::BA_OnlyOnShift)
		{
			UnSneak();
		}

		m_has_pressed_shift = false;
		m_is_bridging = false;
		return;
	}

	CEntityPlayerSP* player = SDK::Minecraft->thePlayer;
	CWorld* world = SDK::Minecraft->theWorld;

	CItemStack item = player->GetInventory().GetCurrentItem();
	if (item.GetInstance() == nullptr || item.GetItem().GetUnlocalizedName().find("tile") == std::string::npos)
	{
		bool found = false;
		if (m_is_bridging && settings::BA_AutoSwap)
		{
			std::vector<CItemStack> hotbar = player->GetInventory().GetMainInventory();
			for (int i = 0; i < 9; i++)
			{
				CItemStack stack = hotbar[i];
				if (stack.GetInstance() != nullptr && stack.GetItem().GetUnlocalizedName().find("tile") != std::string::npos)
				{
					player->GetInventory().SetCurrentItemIndex(i);
					found = true;
					break;
				}
			}
		}

		if (!found)
		{
			if (!m_has_pressed_shift && isSneaking && !settings::BA_OnlyOnShift)
			{
				UnSneak();
			}

			m_has_pressed_shift = false;
			m_is_bridging = false;
			return;
		}
	}

	if (settings::BA_OnlyOnShift)
	{
		if (!m_has_pressed_shift && isSneaking)
		{
			m_has_pressed_shift = true;
		}
	}

	if (player->GetRotationPitch() < settings::BA_PitchCheck)
	{
		if (!m_has_pressed_shift && isSneaking && !settings::BA_OnlyOnShift)
		{
			UnSneak();
		}

		m_has_pressed_shift = false;
		m_is_bridging = false;
		return;
	}

	if (settings::BA_OnlyOnShift && !m_has_pressed_shift)
	{
		m_is_bridging = false;
		return;
	}

	m_from = player->GetPos();
	m_from.y += 1;
	Vector3 hitBlockPos;

	bool res = world->rayTraceBlocks(m_from, m_direction, hitBlockPos, false, false, false);

	// the vertical diff between the player and block under the player
	auto diffY = player->GetPos().y - hitBlockPos.y;
	diffY -= 1;

	static bool jumped = false;
	if (GetAsyncKeyState(VK_SPACE) & 1)
	{
		jumped = true;
	}

	if (jumped)
	{
		UnSneak();

		// check if we are back on ground or going down
		if (player->GetMotion().y < 0.0f || diffY <= 0.0f)
			jumped = false;
		m_is_bridging = false;
		return;
	}

	if (diffY != 0 && diffY <= settings::BA_BlockCheck)
	{
		UnSneak();
		m_is_bridging = false;
		return;
	}


	m_prev = m_is_edge;
	m_is_edge = false;

	bool isFalling = std::abs(player->GetMotion().y) > 0.5f;

	if (!isFalling)
	{
		m_is_bridging = true;
		if (static_cast<int>(diffY) != 0 || res == false)
		{
			Sneak();
		}
		else
		{
			UnSneak();
		}
	}
	else
	{
		UnSneak();
	}
}

void BridgeAssist::RenderMenu()
{
	static bool renderSettings = false;

	ImGui::BeginGroup();

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.12f, 0.5));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10);

	if (ImGui::BeginChild("ba_header", ImVec2(425.f, renderSettings ? 146.f : 35.f), false))
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
		ImGui::BeginGroup();
		Menu::ToggleButton(64, ("Toggle " + this->GetName()).c_str(), ImVec2(368, 0), &settings::BA_Enabled);
		ImGui::EndGroup();
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			renderSettings = !renderSettings;
		}

		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.12f, 0.0));

		if (renderSettings)
		{
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
			ImGui::Separator();
			if (ImGui::BeginChild("ba_settings", ImVec2(425, 101), false))
			{
				Menu::Slider(65, "Block Check", ImVec2(225, 0), &settings::BA_BlockCheck, 1, 10);
				Menu::Slider(66, "Pitch Check", ImVec2(225, 0), &settings::BA_PitchCheck, 0.0f, 90.0f);
				Menu::ToggleButton(67, "Only on Shift", ImVec2(368, 0), &settings::BA_OnlyOnShift);
				Menu::ToggleButton(6801, "Ignore Forwards Movement", ImVec2(368, 0), &settings::BA_IgnoreForwardsMovement);
				Menu::ToggleButton(6802, "Auto Swap", ImVec2(368, 0), &settings::BA_AutoSwap);
			}
			ImGui::EndChild();
			ImGui::Spacing();
		}
	}
	ImGui::EndChild();

	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	ImGui::EndGroup();
}

void BridgeAssist::Sneak()
{
	m_is_edge = true;
	if (!m_prev)
	{
		if (!settings::BA_OnlyOnShift) send_key(VK_SHIFT, true);
		if (settings::BA_OnlyOnShift) SendShiftKey(Menu::HandleWindow, true);
		m_prev = true;
	}
}

void BridgeAssist::UnSneak()
{
	m_is_edge = false;
	if (m_prev)
	{
		if (!settings::BA_OnlyOnShift) send_key(VK_SHIFT, false);
		if (settings::BA_OnlyOnShift) SendShiftKey(Menu::HandleWindow, false);
		m_prev = false;
	}
}
