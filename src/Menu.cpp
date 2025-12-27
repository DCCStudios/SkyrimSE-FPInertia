#include "Menu.h"
#include "Inertia.h"
#include <format>

namespace Menu
{
	// Static storage for weapon types array
	static std::vector<WeaponTypeEntry> s_weaponTypes;
	static bool s_weaponTypesNeedRefresh = true;
	
	void RefreshWeaponTypesList()
	{
		s_weaponTypes.clear();
		
		// Add standard weapon types first
		s_weaponTypes.emplace_back("Unarmed", "Unarmed", WeaponType::Unarmed);
		s_weaponTypes.emplace_back("One-Hand Sword", "OneHandSword", WeaponType::OneHandSword);
		s_weaponTypes.emplace_back("One-Hand Dagger", "OneHandDagger", WeaponType::OneHandDagger);
		s_weaponTypes.emplace_back("One-Hand Axe", "OneHandAxe", WeaponType::OneHandAxe);
		s_weaponTypes.emplace_back("One-Hand Mace", "OneHandMace", WeaponType::OneHandMace);
		s_weaponTypes.emplace_back("Two-Hand Sword", "TwoHandSword", WeaponType::TwoHandSword);
		s_weaponTypes.emplace_back("Two-Hand Axe", "TwoHandAxe", WeaponType::TwoHandAxe);
		s_weaponTypes.emplace_back("Bow", "Bow", WeaponType::Bow);
		s_weaponTypes.emplace_back("Staff", "Staff", WeaponType::Staff);
		s_weaponTypes.emplace_back("Crossbow", "Crossbow", WeaponType::Crossbow);
		s_weaponTypes.emplace_back("Shield", "Shield", WeaponType::Shield);
		s_weaponTypes.emplace_back("Spell (Hands)", "Spell", WeaponType::Spell);
		
		// Add dual wield types (detected with priority over single-hand types)
		s_weaponTypes.emplace_back("Dual Wield Weapons", "DualWieldWeapons", WeaponType::DualWieldWeapons);
		s_weaponTypes.emplace_back("Dual Wield Magic", "DualWieldMagic", WeaponType::DualWieldMagic);
		s_weaponTypes.emplace_back("Spell + Weapon", "SpellAndWeapon", WeaponType::SpellAndWeapon);
		
		// Add custom keyword-based weapon types
		auto* presets = InertiaPresets::GetSingleton();
		const auto& customTypes = presets->GetCustomWeaponTypes();
		for (const auto& typeName : customTypes) {
			s_weaponTypes.emplace_back(typeName);  // Uses custom type constructor
		}
		
		s_weaponTypesNeedRefresh = false;
	}
	
	const std::vector<WeaponTypeEntry>& GetWeaponTypes()
	{
		if (s_weaponTypes.empty() || s_weaponTypesNeedRefresh) {
			RefreshWeaponTypesList();
		}
		return s_weaponTypes;
	}
	
	WeaponInertiaSettings& GetWeaponSettingsForEditing(WeaponType a_type)
	{
		return InertiaPresets::GetSingleton()->GetWeaponTypeSettingsMutable(a_type);
	}
	
	WeaponInertiaSettings& GetWeaponSettingsForEditingByEntry(const WeaponTypeEntry& entry)
	{
		auto* presets = InertiaPresets::GetSingleton();
		if (entry.isCustomType) {
			return presets->GetCustomWeaponTypeSettingsMutable(entry.internalName);
		} else {
			return presets->GetWeaponTypeSettingsMutable(entry.type);
		}
	}
	
	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			logger::warn("SKSE Menu Framework is not installed - menu will not be available");
			return;
		}
		
		SKSEMenuFramework::SetSection("FP Inertia");
		SKSEMenuFramework::AddSectionItem("Settings", Render);
		
		logger::info("Menu registered with SKSE Menu Framework");
	}
	
	bool SliderFloatWithTooltip(const char* label, float* value, float min, float max, const char* format, const char* tooltip)
	{
		bool changed = ImGui::SliderFloat(label, value, min, max, format);
		if (ImGui::IsItemHovered() && tooltip && tooltip[0]) {
			ImGui::SetTooltip("%s", tooltip);
		}
		return changed;
	}
	
	bool CheckboxWithTooltip(const char* label, bool* value, const char* tooltip)
	{
		bool changed = ImGui::Checkbox(label, value);
		if (ImGui::IsItemHovered() && tooltip && tooltip[0]) {
			ImGui::SetTooltip("%s", tooltip);
		}
		return changed;
	}
	
	bool SliderIntWithTooltip(const char* label, int* value, int min, int max, const char* format, const char* tooltip)
	{
		bool changed = ImGui::SliderInt(label, value, min, max, format);
		if (ImGui::IsItemHovered() && tooltip && tooltip[0]) {
			ImGui::SetTooltip("%s", tooltip);
		}
		return changed;
	}
	
	void RefreshPresetList()
	{
		auto* presets = InertiaPresets::GetSingleton();
		State::cachedPresetList = presets->GetAvailablePresets();
		
		// Find current active preset index
		const auto& activeName = presets->GetActivePresetName();
		State::selectedPresetIndex = 0;
		for (size_t i = 0; i < State::cachedPresetList.size(); ++i) {
			if (State::cachedPresetList[i] == activeName) {
				State::selectedPresetIndex = static_cast<int>(i);
				break;
			}
		}
		
		// Also refresh weapon types list to include any custom types
		s_weaponTypesNeedRefresh = true;
	}
	
	void __stdcall Render()
	{
		// Initialize on first render
		if (!State::initialized) {
			State::initialized = true;
			State::hasUnsavedChanges = false;
			RefreshPresetList();
		}
		
		DrawHeader();
		DrawPresetSelector();
		ImGui::Separator();
		
		DrawGeneralSettings();
		DrawSettlingSettings();
		DrawMovementInertiaSettings();
		DrawActionBlendSettings();
		DrawHandsSettings();
		DrawDebugSettings();
		
		ImGui::Separator();
		DrawWeaponTypeSettings();
		
		ImGui::Separator();
		DrawSpecificWeaponSettings();
		
		ImGui::Separator();
		DrawSaveLoadButtons();
	}
	
	void DrawHeader()
	{
		auto* settings = Settings::GetSingleton();
		
		// Check for Native EditorID Fix DLL on first frame
		if (!State::nativeEditorIDFixChecked) {
			State::nativeEditorIDFixChecked = true;
			
			// Check if NativeEditorIDFix.dll is loaded
			State::nativeEditorIDFixInstalled = (GetModuleHandleA("NativeEditorIDFix.dll") != nullptr);
			
			if (!State::nativeEditorIDFixInstalled) {
				logger::warn("[FPInertia] Native EditorID Fix not detected - weapon-specific presets may not work correctly");
			} else {
				logger::info("[FPInertia] Native EditorID Fix detected");
			}
		}
		
		// Show warning if Native EditorID Fix is not installed
		if (!State::nativeEditorIDFixInstalled) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
			ImGui::TextWrapped("WARNING: Native EditorID Fix not detected! Weapon-specific presets require this mod to work properly.");
			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Install 'Native EditorID Fix' from Nexus to enable weapon-specific presets.\nWithout it, vanilla weapons cannot be identified by their EditorID.");
			}
			ImGui::Spacing();
		}
		
		// Frame Generation Compatibility toggle
		{
			// Show Community Shaders detection status
			if (settings->communityShadersDetected) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
				ImGui::Text("Community Shaders Detected");
				ImGui::PopStyleColor();
				ImGui::SameLine();
			}
			
			// Frame Gen toggle - with warning when trying to disable if Community Shaders detected
			bool frameGenMode = settings->frameGenCompatMode;
			if (ImGui::Checkbox("Frame Gen Compatible Mode", &frameGenMode)) {
				if (!frameGenMode && settings->communityShadersDetected) {
					// User is trying to disable when Community Shaders is detected - show warning popup
					ImGui::OpenPopup("Frame Gen Warning");
				} else {
					settings->frameGenCompatMode = frameGenMode;
					State::hasUnsavedChanges = true;
				}
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Reduces ghosting when using Frame Generation (DLSS 3, FSR 3, etc.)\nby limiting how fast inertia offsets can change per frame.\nAuto-enabled when Community Shaders is detected.");
			}
			
			// Warning popup when trying to disable with Community Shaders detected
			if (ImGui::BeginPopupModal("Frame Gen Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
				ImGui::Text("Warning: Community Shaders Detected!");
				ImGui::PopStyleColor();
				ImGui::Spacing();
				ImGui::TextWrapped("Community Shaders with Frame Generation is detected.\nDisabling Frame Gen Compatible Mode may cause severe ghosting\nartifacts on your first-person arms.");
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
				
				if (ImGui::Button("Disable Anyway", ImVec2(120, 0))) {
					settings->frameGenCompatMode = false;
					State::hasUnsavedChanges = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Keep Enabled", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}
		ImGui::Spacing();
		
		ImGui::Text("FP Inertia v1.0.0");
		ImGui::SameLine();
		
		// Enabled toggle
		if (ImGui::Checkbox("Enabled", &settings->enabled)) {
			State::hasUnsavedChanges = true;
		}
		
		// Show first-person indicator
		auto* camera = RE::PlayerCamera::GetSingleton();
		bool inFirstPerson = camera && camera->IsInFirstPerson();
		
		ImGui::SameLine();
		if (inFirstPerson) {
			ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[1st Person]");
		} else {
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[3rd Person]");
		}
		
		// Show weapon drawn state
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (player) {
			auto* actorState = player->AsActorState();
			bool weaponDrawn = actorState && actorState->IsWeaponDrawn();
			
			ImGui::SameLine();
			if (weaponDrawn) {
				ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "[Weapon Drawn]");
			} else {
				ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[Sheathed]");
			}
		}
		
		// Unsaved changes indicator
		if (State::hasUnsavedChanges) {
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "(Unsaved changes - session only)");
		}
	}
	
	void DrawPresetSelector()
	{
		auto* presets = InertiaPresets::GetSingleton();
		
		ImGui::Spacing();
		
		// Preset selection section with distinct styling
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
		
		ImGui::Text("Active Preset:");
		ImGui::SameLine();
		
		// Dropdown for preset selection
		ImGui::SetNextItemWidth(200.0f);
		// Only attempt refresh once per session if list is empty (avoids repeated filesystem operations)
		if (State::cachedPresetList.empty() && !State::presetListAttemptedRefresh) {
			State::presetListAttemptedRefresh = true;
			RefreshPresetList();
		}
		
		if (!State::cachedPresetList.empty()) {
			// Build combo preview
			const char* previewValue = State::selectedPresetIndex >= 0 && 
				State::selectedPresetIndex < static_cast<int>(State::cachedPresetList.size()) 
				? State::cachedPresetList[State::selectedPresetIndex].c_str() 
				: "None";
			
			if (ImGui::BeginCombo("##PresetCombo", previewValue)) {
				for (int i = 0; i < static_cast<int>(State::cachedPresetList.size()); ++i) {
					bool isSelected = (State::selectedPresetIndex == i);
					if (ImGui::Selectable(State::cachedPresetList[i].c_str(), isSelected)) {
						if (State::selectedPresetIndex != i) {
							// Switch to new preset
							presets->SetActivePreset(State::cachedPresetList[i]);
							State::selectedPresetIndex = i;
							State::hasUnsavedChanges = false;  // Just loaded, no changes
							RE::DebugNotification(std::format("FP Inertia: Switched to preset '{}'", 
								State::cachedPresetList[i]).c_str());
						}
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Select a weapon type preset profile.\nEach preset contains settings for all weapon types.\nChanges are auto-saved when switching presets.");
			}
		} else {
			ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "No presets found");
		}
		
		// Buttons for preset management
		ImGui::SameLine();
		if (ImGui::Button("New")) {
			State::showCreatePresetPopup = true;
			memset(State::newPresetName, 0, sizeof(State::newPresetName));
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Create a new preset from current settings");
		}
		
		ImGui::SameLine();
		if (ImGui::Button("Refresh")) {
			State::presetListAttemptedRefresh = true;  // Mark as attempted to allow manual refresh
			RefreshPresetList();
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Refresh the list of available presets from disk");
		}
		
		// Only show delete if there's more than one preset
		if (State::cachedPresetList.size() > 1) {
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
			if (ImGui::Button("Delete")) {
				State::showDeletePresetConfirm = true;
			}
			ImGui::PopStyleColor(2);
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Delete a preset file (cannot delete the active preset)");
			}
		}
		
		ImGui::PopStyleColor(2);
		
		// Show current preset path info
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
			"Editing: Data/SKSE/Plugins/FPInertia/%s.json", presets->GetActivePresetName().c_str());
		
		// Create preset popup
		if (State::showCreatePresetPopup) {
			ImGui::OpenPopup("Create New Preset");
		}
		
		if (ImGui::BeginPopupModal("Create New Preset", &State::showCreatePresetPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Enter a name for the new preset:");
			ImGui::SetNextItemWidth(250.0f);
			ImGui::InputText("##NewPresetName", State::newPresetName, sizeof(State::newPresetName));
			
			ImGui::Spacing();
			
			bool validName = strlen(State::newPresetName) > 0;
			
			// Check if name already exists
			bool nameExists = false;
			for (const auto& p : State::cachedPresetList) {
				if (p == State::newPresetName) {
					nameExists = true;
					break;
				}
			}
			
			if (nameExists) {
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "A preset with this name already exists!");
				validName = false;
			}
			
			if (!validName) {
				ImGui::BeginDisabled();
			}
			if (ImGui::Button("Create", ImVec2(120, 0))) {
				presets->CreateNewPreset(State::newPresetName);
				RefreshPresetList();
				State::showCreatePresetPopup = false;
				State::hasUnsavedChanges = false;
				RE::DebugNotification(std::format("FP Inertia: Created preset '{}'", State::newPresetName).c_str());
			}
			if (!validName) {
				ImGui::EndDisabled();
			}
			
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				State::showCreatePresetPopup = false;
			}
			
			ImGui::EndPopup();
		}
		
		// Delete preset popup
		if (State::showDeletePresetConfirm) {
			ImGui::OpenPopup("Delete Preset");
		}
		
		if (ImGui::BeginPopupModal("Delete Preset", &State::showDeletePresetConfirm, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Select a preset to delete:");
			ImGui::Spacing();
			
			static int deleteIndex = -1;
			
			ImGui::SetNextItemWidth(250.0f);
			if (ImGui::BeginCombo("##DeletePresetCombo", 
				deleteIndex >= 0 && deleteIndex < static_cast<int>(State::cachedPresetList.size()) 
				? State::cachedPresetList[deleteIndex].c_str() 
				: "Select...")) {
				
				for (int i = 0; i < static_cast<int>(State::cachedPresetList.size()); ++i) {
					// Can't delete the active preset
					if (State::cachedPresetList[i] == presets->GetActivePresetName()) {
						continue;
					}
					
					bool isSelected = (deleteIndex == i);
					if (ImGui::Selectable(State::cachedPresetList[i].c_str(), isSelected)) {
						deleteIndex = i;
					}
				}
				ImGui::EndCombo();
			}
			
			ImGui::Spacing();
			
			bool canDelete = deleteIndex >= 0 && deleteIndex < static_cast<int>(State::cachedPresetList.size()) 
				&& State::cachedPresetList[deleteIndex] != presets->GetActivePresetName();
			
			if (!canDelete) {
				ImGui::BeginDisabled();
			}
			
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
			if (ImGui::Button("Delete", ImVec2(120, 0))) {
				std::string presetToDelete = State::cachedPresetList[deleteIndex];
				presets->DeletePreset(presetToDelete);
				RefreshPresetList();
				deleteIndex = -1;
				State::showDeletePresetConfirm = false;
				RE::DebugNotification(std::format("FP Inertia: Deleted preset '{}'", presetToDelete).c_str());
			}
			ImGui::PopStyleColor(2);
			
			if (!canDelete) {
				ImGui::EndDisabled();
			}
			
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				deleteIndex = -1;
				State::showDeletePresetConfirm = false;
			}
			
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), 
				"(Cannot delete the currently active preset)");
			
			ImGui::EndPopup();
		}
		
		ImGui::Spacing();
	}
	
	void DrawGeneralSettings()
	{
		auto* settings = Settings::GetSingleton();
		
		if (ImGui::CollapsingHeader("General Settings", State::generalExpanded ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
			State::generalExpanded = true;
			
			if (CheckboxWithTooltip("Enable Position Offset", &settings->enablePosition,
				"Enable positional movement (weapon moves left/right/up/down based on camera/movement)")) {
				State::hasUnsavedChanges = true;
			}
			
			if (CheckboxWithTooltip("Enable Rotation Offset", &settings->enableRotation,
				"Enable rotational movement (weapon tilts/rotates based on camera/movement)")) {
				State::hasUnsavedChanges = true;
			}
			
			if (CheckboxWithTooltip("Require Weapon Drawn", &settings->requireWeaponDrawn,
				"Only apply inertia effects when weapon is drawn")) {
				State::hasUnsavedChanges = true;
			}
			
			ImGui::Spacing();
			
			if (SliderFloatWithTooltip("Global Intensity", &settings->globalIntensity, 0.0f, 5.0f, "%.2f",
				"Master multiplier for all inertia effects\n1.0 = normal, 0.5 = half, 2.0 = double")) {
				State::hasUnsavedChanges = true;
			}
			
			if (SliderFloatWithTooltip("Smoothing Factor", &settings->smoothingFactor, 0.0f, 1.0f, "%.2f",
				"Camera velocity smoothing (0 = no smoothing, 1 = maximum)\nHigher values reduce jitter but add latency")) {
				State::hasUnsavedChanges = true;
			}
		} else {
			State::generalExpanded = false;
		}
	}
	
	void DrawSettlingSettings()
	{
		auto* settings = Settings::GetSingleton();
		
		if (ImGui::CollapsingHeader("Settling Behavior", State::settlingExpanded ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
			State::settlingExpanded = true;
			
			ImGui::TextWrapped("When the camera stops moving, the spring gradually dampens to reduce wobbling.");
			ImGui::Spacing();
			
			if (SliderFloatWithTooltip("Settle Delay", &settings->settleDelay, 0.0f, 2.0f, "%.2f sec",
				"Delay before settling starts after camera stops moving\n0 = immediate settling")) {
				State::hasUnsavedChanges = true;
			}
			
			if (SliderFloatWithTooltip("Settle Speed", &settings->settleSpeed, 0.5f, 10.0f, "%.1f",
				"How fast the damping increases once settling begins\nHigher = faster settling")) {
				State::hasUnsavedChanges = true;
			}
			
			if (SliderFloatWithTooltip("Settle Damping Mult", &settings->settleDampingMult, 1.0f, 10.0f, "%.1fx",
				"Maximum damping multiplier when fully settled\nHigher = less wobble when stopped")) {
				State::hasUnsavedChanges = true;
			}
		} else {
			State::settlingExpanded = false;
		}
	}
	
	void DrawMovementInertiaSettings()
	{
		auto* settings = Settings::GetSingleton();
		
		if (ImGui::CollapsingHeader("Movement Inertia", State::movementExpanded ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
			State::movementExpanded = true;
			
			ImGui::TextWrapped("Arm sway based on player movement. Adds to camera inertia. Per-weapon settings are in the weapon section.");
			ImGui::Spacing();
			
			if (CheckboxWithTooltip("Enable Movement Inertia (Global)", &settings->movementInertiaEnabled,
				"Master toggle for movement-based arm sway\nCan also be disabled per-weapon in weapon settings")) {
				State::hasUnsavedChanges = true;
			}
			
			if (settings->movementInertiaEnabled) {
				if (SliderFloatWithTooltip("Strength", &settings->movementInertiaStrength, 0.0f, 20.0f, "%.1f",
					"Global strength multiplier for movement inertia")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Threshold", &settings->movementInertiaThreshold, 0.0f, 200.0f, "%.0f units/sec",
					"Minimum movement speed to trigger effect\nLower = more sensitive to small movements")) {
					State::hasUnsavedChanges = true;
				}
				
				ImGui::Spacing();
				ImGui::Text("Forward/Backward Movement:");
				
				if (CheckboxWithTooltip("Enable Forward/Back Inertia", &settings->forwardBackInertia,
					"Enable arm sway for forward/backward movement\nDefault: OFF (only strafing causes sway)\nPer-weapon multipliers are in weapon settings")) {
					State::hasUnsavedChanges = true;
				}
				
				if (settings->forwardBackInertia) {
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
						"Forward/back multipliers are set per-weapon in the weapon settings section.");
				}
				
				ImGui::Spacing();
				
				if (CheckboxWithTooltip("Disable Vanilla Sway", &settings->disableVanillaSway,
					"Disable the game's built-in walk/run arm sway via behavior graph\nUseful if this mod's movement inertia feels doubled")) {
					State::hasUnsavedChanges = true;
				}
			}
		} else {
			State::movementExpanded = false;
		}
	}
	
	void DrawActionBlendSettings()
	{
		auto* settings = Settings::GetSingleton();
		
		if (ImGui::CollapsingHeader("Action Blending", State::actionBlendExpanded ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
			State::actionBlendExpanded = true;
			
			ImGui::TextWrapped("Smoothly reduce inertia during combat actions for better animation blending.");
			ImGui::Spacing();
			
			if (CheckboxWithTooltip("Blend During Attack", &settings->blendDuringAttack,
				"Reduce inertia while performing melee attacks")) {
				State::hasUnsavedChanges = true;
			}
			
			if (CheckboxWithTooltip("Blend During Bow Draw", &settings->blendDuringBowDraw,
				"Reduce inertia while drawing/firing bow")) {
				State::hasUnsavedChanges = true;
			}
			
			if (CheckboxWithTooltip("Blend During Spell Cast", &settings->blendDuringSpellCast,
				"Reduce inertia while casting spells")) {
				State::hasUnsavedChanges = true;
			}
			
			ImGui::Spacing();
			
			if (SliderFloatWithTooltip("Blend Speed", &settings->actionBlendSpeed, 1.0f, 20.0f, "%.1f",
				"How fast to blend inertia in/out during actions\nHigher = snappier transitions")) {
				State::hasUnsavedChanges = true;
			}
			
			if (SliderFloatWithTooltip("Minimum Intensity", &settings->actionMinIntensity, 0.0f, 1.0f, "%.2f",
				"Minimum inertia intensity during actions\n0 = fully disabled during actions, 1 = no reduction")) {
				State::hasUnsavedChanges = true;
			}
		} else {
			State::actionBlendExpanded = false;
		}
	}
	
	void DrawHandsSettings()
	{
		auto* settings = Settings::GetSingleton();
		
		if (ImGui::CollapsingHeader("Hand Settings", State::handsExpanded ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
			State::handsExpanded = true;
			
			if (CheckboxWithTooltip("Independent Hands", &settings->independentHands,
				"Process each hand separately (for future dual-wield support)")) {
				State::hasUnsavedChanges = true;
			}
			
			if (SliderFloatWithTooltip("Left Hand Multiplier", &settings->leftHandMultiplier, 0.0f, 3.0f, "%.2f",
				"Intensity multiplier for left hand")) {
				State::hasUnsavedChanges = true;
			}
			
			if (SliderFloatWithTooltip("Right Hand Multiplier", &settings->rightHandMultiplier, 0.0f, 3.0f, "%.2f",
				"Intensity multiplier for right hand")) {
				State::hasUnsavedChanges = true;
			}
		} else {
			State::handsExpanded = false;
		}
	}
	
	void DrawDebugSettings()
	{
		auto* settings = Settings::GetSingleton();
		
		if (ImGui::CollapsingHeader("Debug", State::debugExpanded ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
			State::debugExpanded = true;
			
			if (CheckboxWithTooltip("Debug Logging", &settings->debugLogging,
				"Enable detailed debug messages in the log file")) {
				State::hasUnsavedChanges = true;
			}
			
			if (CheckboxWithTooltip("Debug On Screen", &settings->debugOnScreen,
				"Show debug information on screen (not implemented)")) {
				State::hasUnsavedChanges = true;
			}
			
			ImGui::Spacing();
			
			if (CheckboxWithTooltip("Enable Hot Reload", &settings->enableHotReload,
				"Automatically reload INI when file changes on disk\nDisabled when using menu (menu changes take priority)")) {
				State::hasUnsavedChanges = true;
			}
			
			if (SliderFloatWithTooltip("Hot Reload Interval", &settings->hotReloadIntervalSec, 1.0f, 60.0f, "%.0f sec",
				"How often to check for INI changes")) {
				State::hasUnsavedChanges = true;
			}
			
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Text("Quick Actions:");
			
			if (ImGui::Button("Reset Springs")) {
				Inertia::InertiaManager::GetSingleton()->Reset();
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Reset all spring states to zero (stops any current motion)");
			}
		} else {
			State::debugExpanded = false;
		}
	}
	
	void DrawWeaponTypeSettings()
	{
		auto* settings = Settings::GetSingleton();
		
		if (ImGui::CollapsingHeader("Per-Weapon Type Settings", State::weaponSettingsExpanded ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
			State::weaponSettingsExpanded = true;
			
			ImGui::TextWrapped("Each weapon type has its own spring parameters, affecting how it feels in combat.");
			ImGui::Spacing();
			
			// Weapon type dropdown
			const auto& types = GetWeaponTypes();
			
			// Build label with custom type indicator
			std::string comboLabel = types[State::selectedWeaponTypeIndex].displayName;
			if (types[State::selectedWeaponTypeIndex].isCustomType) {
				comboLabel += " *";  // Asterisk indicates custom type
			}
			
			// Make dropdown narrower so equipped weapon text fits
			ImGui::SetNextItemWidth(200.0f);
			if (ImGui::BeginCombo("Weapon Type", comboLabel.c_str())) {
				bool shownCustomSeparator = false;
				
				for (int i = 0; i < static_cast<int>(types.size()); ++i) {
					// Add separator before first custom type
					if (types[i].isCustomType && !shownCustomSeparator) {
						ImGui::Separator();
						ImGui::TextDisabled("-- Custom Types --");
						shownCustomSeparator = true;
					}
					
					bool isSelected = (State::selectedWeaponTypeIndex == i);
					
					// Add asterisk for custom types in dropdown
					std::string label = types[i].displayName;
					if (types[i].isCustomType) {
						label += " *";
					}
					
					if (ImGui::Selectable(label.c_str(), isSelected)) {
						State::selectedWeaponTypeIndex = i;
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Select weapon type to edit.\n* = Custom keyword-based type from mapping files");
			}
			
			// Show current equipped weapon info for context
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* presets = InertiaPresets::GetSingleton();
			std::string equippedWeaponID;
			bool hasSpecificPreset = false;
			
			if (player) {
				auto* rightWeapon = player->GetEquippedObject(false);
				if (rightWeapon && rightWeapon->IsWeapon()) {
					auto* weapon = rightWeapon->As<RE::TESObjectWEAP>();
					if (weapon) {
						// Get EditorID (Native EditorID Fix makes this work for all forms)
						const char* rawEditorID = weapon->GetFormEditorID();
						if (rawEditorID && rawEditorID[0] != '\0') {
							equippedWeaponID = rawEditorID;
						} else {
							// Fallback - shouldn't happen with Native EditorID Fix installed
							equippedWeaponID = std::format("0x{:08X}", weapon->GetFormID());
						}
						
						hasSpecificPreset = presets->HasSpecificWeaponSettings(equippedWeaponID);
						
						ImGui::SameLine();
						if (hasSpecificPreset) {
							ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "(Equipped: %s - SPECIFIC PRESET ACTIVE)", weapon->GetName());
						} else {
							ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "(Equipped: %s)", weapon->GetName());
						}
					}
				}
			}
			
			// Quick save as specific weapon preset
			if (!equippedWeaponID.empty()) {
				if (hasSpecificPreset) {
					// Already has preset - show update button
					if (ImGui::Button("Update Specific Preset")) {
						// Copy current weapon type settings to the specific preset
						const auto& entry = types[State::selectedWeaponTypeIndex];
						const auto& currentSettings = GetWeaponSettingsForEditingByEntry(entry);
						auto& specificSettings = presets->GetOrCreateSpecificWeaponSettings(equippedWeaponID, entry.type);
						specificSettings = currentSettings;
						presets->SaveSpecificWeaponPreset(equippedWeaponID);
						RE::DebugNotification(std::format("Updated specific preset for {}", equippedWeaponID).c_str());
					}
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Update the specific preset for '%s' with current settings", equippedWeaponID.c_str());
					}
					ImGui::SameLine();
					if (ImGui::Button("Delete Specific Preset")) {
						presets->RemoveSpecificWeaponSettings(equippedWeaponID);
						RE::DebugNotification(std::format("Deleted specific preset for {}", equippedWeaponID).c_str());
					}
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Delete the specific preset - weapon will use type settings instead");
					}
				} else {
					// No preset yet - show create button
					if (ImGui::Button("Save as Specific Weapon Preset")) {
						// Determine weapon type from equipped
						WeaponType weaponType = WeaponType::Unarmed;
						auto* rightWeapon = player->GetEquippedObject(false);
						if (rightWeapon && rightWeapon->IsWeapon()) {
							auto* weapon = rightWeapon->As<RE::TESObjectWEAP>();
							if (weapon) {
								weaponType = Settings::ToWeaponType(weapon->GetWeaponType());
							}
						}
						
						// Create preset with current settings
						const auto& entry = types[State::selectedWeaponTypeIndex];
						const auto& currentSettings = GetWeaponSettingsForEditingByEntry(entry);
						auto& specificSettings = presets->GetOrCreateSpecificWeaponSettings(equippedWeaponID, weaponType);
						specificSettings = currentSettings;
						presets->SaveSpecificWeaponPreset(equippedWeaponID);
						RE::DebugNotification(std::format("Created specific preset for {}", equippedWeaponID).c_str());
					}
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Save current settings as a specific preset for '%s'\nThis will OVERRIDE weapon type settings for this weapon only", equippedWeaponID.c_str());
					}
				}
			}
			
			// Copy from currently equipped button
			if (ImGui::Button("Select Equipped Type")) {
				auto* player2 = RE::PlayerCharacter::GetSingleton();
				if (player2) {
					bool found = false;
					
					// PRIORITY 1: Check for dual wield scenarios first
					auto* rightEquip = player2->GetEquippedObject(false);  // right hand
					auto* leftEquip = player2->GetEquippedObject(true);    // left hand
					
					// Helper to check if weapon is actual weapon (not unarmed/hand-to-hand)
					auto isActualWeapon = [](RE::TESForm* obj) -> bool {
						if (!obj || !obj->IsWeapon()) {
							return false;
						}
						auto* weapon = obj->As<RE::TESObjectWEAP>();
						if (!weapon) {
							return false;
						}
						// Exclude hand-to-hand/unarmed from "actual weapons"
						return weapon->GetWeaponType() != RE::WEAPON_TYPE::kHandToHandMelee;
					};
					
					bool rightHasWeapon = isActualWeapon(rightEquip);
					bool leftHasWeapon = isActualWeapon(leftEquip);
					bool rightHasSpell = rightEquip && !rightEquip->IsWeapon() && !rightEquip->IsArmor();
					bool leftHasSpell = leftEquip && !leftEquip->IsWeapon() && !leftEquip->IsArmor();
					
					// Check for two-handed weapons - not dual wield
					bool isTwoHanded = false;
					if (rightHasWeapon) {
						auto* weapon = rightEquip->As<RE::TESObjectWEAP>();
						if (weapon) {
							RE::WEAPON_TYPE wtype = weapon->GetWeaponType();
							isTwoHanded = (wtype == RE::WEAPON_TYPE::kTwoHandAxe || 
							               wtype == RE::WEAPON_TYPE::kTwoHandSword ||
							               wtype == RE::WEAPON_TYPE::kBow ||
							               wtype == RE::WEAPON_TYPE::kCrossbow);
						}
					}
					
					WeaponType dualType = WeaponType::Unarmed;
					if (!isTwoHanded) {
						// DualWieldWeapons: weapon in both hands
						if (rightHasWeapon && leftHasWeapon) {
							dualType = WeaponType::DualWieldWeapons;
						}
						// SpellAndWeapon: spell in one hand, weapon in the other
						else if ((rightHasWeapon && leftHasSpell) || (rightHasSpell && leftHasWeapon)) {
							dualType = WeaponType::SpellAndWeapon;
						}
						// DualWieldMagic: spells in both hands (actual spell objects)
						else if (rightHasSpell && leftHasSpell) {
							dualType = WeaponType::DualWieldMagic;
						}
					}
					
					if (dualType != WeaponType::Unarmed) {
						// Found a dual wield type - find it in the dropdown
						for (int i = 0; i < static_cast<int>(types.size()); ++i) {
							if (!types[i].isCustomType && types[i].type == dualType) {
								State::selectedWeaponTypeIndex = i;
								found = true;
								break;
							}
						}
					}
					
					// PRIORITY 2: Check for custom keyword-based types on right hand weapon
					if (!found && rightHasWeapon) {
						auto* weapon = rightEquip->As<RE::TESObjectWEAP>();
						if (weapon) {
							std::string customType = presets->GetBestKeywordMatch(weapon);
							if (!customType.empty()) {
								for (int i = 0; i < static_cast<int>(types.size()); ++i) {
									if (types[i].isCustomType && types[i].internalName == customType) {
										State::selectedWeaponTypeIndex = i;
										found = true;
										break;
									}
								}
							}
						}
					}
					
					// PRIORITY 3: Fall back to standard weapon type
					if (!found && rightHasWeapon) {
						auto* weapon = rightEquip->As<RE::TESObjectWEAP>();
						if (weapon) {
							RE::WEAPON_TYPE type = weapon->GetWeaponType();
							WeaponType internalType = Settings::ToWeaponType(type);
							
							for (int i = 0; i < static_cast<int>(types.size()); ++i) {
								if (!types[i].isCustomType && types[i].type == internalType) {
									State::selectedWeaponTypeIndex = i;
									found = true;
									break;
								}
							}
						}
					}
					
					// PRIORITY 4: Check for shield in left hand
					if (!found && leftEquip && leftEquip->IsArmor()) {
						auto* armor = leftEquip->As<RE::TESObjectARMO>();
						if (armor && armor->IsShield()) {
							for (int i = 0; i < static_cast<int>(types.size()); ++i) {
								if (!types[i].isCustomType && types[i].type == WeaponType::Shield) {
									State::selectedWeaponTypeIndex = i;
									found = true;
									break;
								}
							}
						}
					}
					
					// PRIORITY 5: Check for spells (hands only)
					if (!found && (rightHasSpell || leftHasSpell)) {
						for (int i = 0; i < static_cast<int>(types.size()); ++i) {
							if (!types[i].isCustomType && types[i].type == WeaponType::Spell) {
								State::selectedWeaponTypeIndex = i;
								found = true;
								break;
							}
						}
					}
					
					// PRIORITY 6: Unarmed as fallback
					if (!found) {
						for (int i = 0; i < static_cast<int>(types.size()); ++i) {
							if (!types[i].isCustomType && types[i].type == WeaponType::Unarmed) {
								State::selectedWeaponTypeIndex = i;
								break;
							}
						}
					}
				}
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Switch to editing the currently equipped weapon type\n(Prioritizes dual wield types, then custom keyword types, then standard types)");
			}
			
			// Check if currently selected weapon type matches equipped weapon
			// Show yellow warning if they differ
			{
				auto* playerCheck = RE::PlayerCharacter::GetSingleton();
				if (playerCheck) {
					WeaponType equippedType = WeaponType::Unarmed;
					std::string equippedCustomType;
					bool foundEquipped = false;
					
					auto* rightEquipCheck = playerCheck->GetEquippedObject(false);
					auto* leftEquipCheck = playerCheck->GetEquippedObject(true);
					
					// Helper to check if weapon is actual weapon (not unarmed)
					auto isActualWeaponCheck = [](RE::TESForm* obj) -> bool {
						if (!obj || !obj->IsWeapon()) return false;
						auto* w = obj->As<RE::TESObjectWEAP>();
						return w && w->GetWeaponType() != RE::WEAPON_TYPE::kHandToHandMelee;
					};
					
					bool rightHasWeaponCheck = isActualWeaponCheck(rightEquipCheck);
					bool leftHasWeaponCheck = isActualWeaponCheck(leftEquipCheck);
					bool rightHasSpellCheck = rightEquipCheck && !rightEquipCheck->IsWeapon() && !rightEquipCheck->IsArmor();
					bool leftHasSpellCheck = leftEquipCheck && !leftEquipCheck->IsWeapon() && !leftEquipCheck->IsArmor();
					
					// Check two-handed
					bool isTwoHandedCheck = false;
					if (rightHasWeaponCheck) {
						auto* w = rightEquipCheck->As<RE::TESObjectWEAP>();
						if (w) {
							RE::WEAPON_TYPE wt = w->GetWeaponType();
							isTwoHandedCheck = (wt == RE::WEAPON_TYPE::kTwoHandAxe || wt == RE::WEAPON_TYPE::kTwoHandSword ||
							                    wt == RE::WEAPON_TYPE::kBow || wt == RE::WEAPON_TYPE::kCrossbow);
						}
					}
					
					// Priority 1: Dual wield
					if (!isTwoHandedCheck) {
						if (rightHasWeaponCheck && leftHasWeaponCheck) {
							equippedType = WeaponType::DualWieldWeapons;
							foundEquipped = true;
						} else if ((rightHasWeaponCheck && leftHasSpellCheck) || (rightHasSpellCheck && leftHasWeaponCheck)) {
							equippedType = WeaponType::SpellAndWeapon;
							foundEquipped = true;
						} else if (rightHasSpellCheck && leftHasSpellCheck) {
							equippedType = WeaponType::DualWieldMagic;
							foundEquipped = true;
						}
					}
					
					// Priority 2: Custom keyword type
					if (!foundEquipped && rightHasWeaponCheck) {
						auto* w = rightEquipCheck->As<RE::TESObjectWEAP>();
						if (w) {
							equippedCustomType = presets->GetBestKeywordMatch(w);
							if (!equippedCustomType.empty()) {
								foundEquipped = true;
							}
						}
					}
					
					// Priority 3: Standard weapon type
					if (!foundEquipped && rightHasWeaponCheck) {
						auto* w = rightEquipCheck->As<RE::TESObjectWEAP>();
						if (w) {
							equippedType = Settings::ToWeaponType(w->GetWeaponType());
							foundEquipped = true;
						}
					}
					
					// Priority 4: Shield
					if (!foundEquipped && leftEquipCheck && leftEquipCheck->IsArmor()) {
						auto* armor = leftEquipCheck->As<RE::TESObjectARMO>();
						if (armor && armor->IsShield()) {
							equippedType = WeaponType::Shield;
							foundEquipped = true;
						}
					}
					
					// Priority 5: Spell only
					if (!foundEquipped && (rightHasSpellCheck || leftHasSpellCheck)) {
						equippedType = WeaponType::Spell;
						foundEquipped = true;
					}
					
					// Compare with selected
					if (foundEquipped && State::selectedWeaponTypeIndex >= 0 && State::selectedWeaponTypeIndex < static_cast<int>(types.size())) {
						const auto& selected = types[State::selectedWeaponTypeIndex];
						bool mismatch = false;
						
						if (!equippedCustomType.empty()) {
							// Equipped has custom type - check if selected matches
							mismatch = !selected.isCustomType || selected.internalName != equippedCustomType;
						} else {
							// Equipped has standard type - check if selected matches
							mismatch = selected.isCustomType || selected.type != equippedType;
						}
						
						if (mismatch) {
							ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.0f, 1.0f), 
								"(!) Editing different type than equipped weapon");
						}
					}
				}
			}
			
			// Copy to Preset button
			ImGui::SameLine();
			if (ImGui::Button("Copy to Preset...")) {
				// Store source info for the popup
				if (State::selectedWeaponTypeIndex >= 0 && State::selectedWeaponTypeIndex < static_cast<int>(types.size())) {
					State::copySourceWeaponType = types[State::selectedWeaponTypeIndex].displayName;
					State::copySourcePreset = InertiaPresets::GetSingleton()->GetActivePresetName();
					State::copyTargetPresetIndex = 0;
					State::showCopyToPresetPopup = true;
					// Refresh preset list for the popup
					RefreshPresetList();
				}
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Copy settings for this weapon type to another preset");
			}
			
			// Copy to Preset popup modal
			if (State::showCopyToPresetPopup) {
				ImGui::OpenPopup("Copy to Preset");
			}
			
			if (ImGui::BeginPopupModal("Copy to Preset", &State::showCopyToPresetPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
				
				ImGui::Text("Copy Weapon Type Settings");
				ImGui::Separator();
				
				// Source info
				ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Source:");
				ImGui::BulletText("Preset: %s", State::copySourcePreset.c_str());
				ImGui::BulletText("Weapon Type: %s", State::copySourceWeaponType.c_str());
				
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
				
				// Target preset dropdown
				ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Target Preset:");
				if (ImGui::BeginCombo("##TargetPreset", 
					State::copyTargetPresetIndex >= 0 && State::copyTargetPresetIndex < static_cast<int>(State::cachedPresetList.size()) 
						? State::cachedPresetList[State::copyTargetPresetIndex].c_str() : "")) {
					for (int i = 0; i < static_cast<int>(State::cachedPresetList.size()); ++i) {
						bool isSelected = (State::copyTargetPresetIndex == i);
						bool isSameAsSource = (State::cachedPresetList[i] == State::copySourcePreset);
						
						// Gray out the source preset
						if (isSameAsSource) {
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
						}
						
						std::string label = State::cachedPresetList[i];
						if (isSameAsSource) {
							label += " (current)";
						}
						
						if (ImGui::Selectable(label.c_str(), isSelected)) {
							State::copyTargetPresetIndex = i;
						}
						
						if (isSameAsSource) {
							ImGui::PopStyleColor();
						}
						
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				
				ImGui::Spacing();
				
				// Warning about what will be overwritten
				std::string targetPreset = (State::copyTargetPresetIndex >= 0 && State::copyTargetPresetIndex < static_cast<int>(State::cachedPresetList.size()))
					? State::cachedPresetList[State::copyTargetPresetIndex] : "";
				
				if (!targetPreset.empty() && targetPreset != State::copySourcePreset) {
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "Warning:");
					ImGui::TextWrapped("This will OVERWRITE the '%s' settings in preset '%s' with the settings from preset '%s'.",
						State::copySourceWeaponType.c_str(), targetPreset.c_str(), State::copySourcePreset.c_str());
				} else if (targetPreset == State::copySourcePreset) {
					ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Cannot copy to the same preset!");
				}
				
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
				
				// Confirm and Cancel buttons
				bool canConfirm = !targetPreset.empty() && targetPreset != State::copySourcePreset;
				
				if (!canConfirm) {
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
				}
				
				if (ImGui::Button("Confirm Copy", ImVec2(120, 0)) && canConfirm) {
					// Get source settings
					const auto& sourceEntry = types[State::selectedWeaponTypeIndex];
					const auto& sourceSettings = GetWeaponSettingsForEditingByEntry(sourceEntry);
					
					// Temporarily switch to target preset, copy, then switch back
					std::string currentPreset = presets->GetActivePresetName();
					presets->SetActivePreset(targetPreset);
					
					// Get target settings and copy
					auto& targetSettings = GetWeaponSettingsForEditingByEntry(sourceEntry);
					targetSettings = sourceSettings;  // Copy all settings
					
					// Save to the target preset
					presets->SaveWeaponTypePresets();
					
					// Switch back to original preset
					presets->SetActivePreset(currentPreset);
					
					// Show notification
					RE::DebugNotification(std::format("Copied {} settings to preset '{}'", 
						State::copySourceWeaponType, targetPreset).c_str());
					
					logger::info("[FPInertia] Copied {} settings from '{}' to '{}'",
						State::copySourceWeaponType, currentPreset, targetPreset);
					
					State::showCopyToPresetPopup = false;
					ImGui::CloseCurrentPopup();
				}
				
				if (!canConfirm) {
					ImGui::PopStyleVar();
				}
				
				ImGui::SameLine();
				
				if (ImGui::Button("Cancel", ImVec2(120, 0))) {
					State::showCopyToPresetPopup = false;
					ImGui::CloseCurrentPopup();
				}
				
				ImGui::EndPopup();
			}
			
			ImGui::Separator();
			
			// Draw the editor for selected weapon type
			if (State::selectedWeaponTypeIndex >= 0 && State::selectedWeaponTypeIndex < static_cast<int>(types.size())) {
				const auto& entry = types[State::selectedWeaponTypeIndex];
				auto& weaponSettings = GetWeaponSettingsForEditingByEntry(entry);
				
				// Show custom type indicator
				if (entry.isCustomType) {
					ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "(Custom Keyword-Based Type)");
				}
				
				DrawWeaponInertiaEditor(weaponSettings, entry.displayName.c_str());
			}
		} else {
			State::weaponSettingsExpanded = false;
		}
	}
	
	void DrawWeaponInertiaEditor(WeaponInertiaSettings& settings, const char* label)
	{
		ImGui::PushID(label);
		
		// Master Enable/Disable Toggle
		ImGui::PushStyleColor(ImGuiCol_Text, settings.enabled ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
		if (ImGui::Checkbox("Enable Inertia for This Weapon Type", &settings.enabled)) {
			State::hasUnsavedChanges = true;
		}
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Master toggle to enable/disable ALL inertia effects for this weapon type.\nWhen disabled, no inertia will be applied when using this weapon type.");
		}
		
		// If disabled, show a warning and gray out the rest
		if (!settings.enabled) {
			ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Inertia disabled for this weapon type");
			ImGui::Spacing();
		}
		
		// Optionally dim the controls when disabled (they still work but are visually muted)
		if (!settings.enabled) {
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
		}
		
		// Camera Inertia Section
		if (ImGui::TreeNodeEx("Camera Inertia", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Responds to looking around (camera rotation)");
			
			if (SliderFloatWithTooltip("Stiffness##cam", &settings.stiffness, 10.0f, 1000.0f, "%.0f",
				"Spring stiffness - higher = faster return to center\nLighter weapons should have higher values")) {
				State::hasUnsavedChanges = true;
			}
			
			if (SliderFloatWithTooltip("Damping##cam", &settings.damping, 1.0f, 100.0f, "%.1f",
				"Damping - reduces oscillation/wobble\nHigher = less bouncy")) {
				State::hasUnsavedChanges = true;
			}
			
			if (SliderFloatWithTooltip("Max Offset##cam", &settings.maxOffset, 0.0f, 50.0f, "%.1f",
				"Maximum position offset in units\nLimits how far the weapon can move")) {
				State::hasUnsavedChanges = true;
			}
			
			if (SliderFloatWithTooltip("Max Rotation##cam", &settings.maxRotation, 0.0f, 90.0f, "%.1f deg",
				"Maximum rotation offset in degrees")) {
				State::hasUnsavedChanges = true;
			}
			
			if (SliderFloatWithTooltip("Mass##cam", &settings.mass, 0.1f, 10.0f, "%.2f",
				"Virtual mass - heavier = more inertia/lag\nDaggers: 0.5, Swords: 1.0, Battleaxes: 2.5")) {
				State::hasUnsavedChanges = true;
			}
			
			ImGui::Spacing();
			ImGui::Text("Multipliers:");
			
			if (SliderFloatWithTooltip("Pitch Mult##cam", &settings.pitchMultiplier, 0.0f, 5.0f, "%.2f",
				"Multiplier for pitch ROTATION effect (looking up/down tilt)")) {
				State::hasUnsavedChanges = true;
			}
			
			if (SliderFloatWithTooltip("Roll Mult##cam", &settings.rollMultiplier, 0.0f, 5.0f, "%.2f",
				"Multiplier for roll effect (wavy side-to-side motion)\nStaffs benefit from higher values")) {
				State::hasUnsavedChanges = true;
			}
			
			if (SliderFloatWithTooltip("Global Pitch Mult##cam", &settings.cameraPitchMult, 0.0f, 5.0f, "%.2f",
				"Global multiplier for ALL pitch effects (position + rotation)")) {
				State::hasUnsavedChanges = true;
			}
			
			if (CheckboxWithTooltip("Invert Pitch (Up/Down)##cam", &settings.invertCameraPitch,
				"Invert the pitch (up/down look) camera inertia")) {
				State::hasUnsavedChanges = true;
			}
			ImGui::SameLine();
			if (CheckboxWithTooltip("Invert Yaw (Left/Right)##cam", &settings.invertCameraYaw,
				"Invert the yaw (left/right look) camera inertia")) {
				State::hasUnsavedChanges = true;
			}
			
			ImGui::TreePop();
		}
		
		// Movement Inertia Section
		if (ImGui::TreeNodeEx("Movement Inertia", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Responds to player movement (strafing, forward/back)");
			
			// Per-weapon enable
			if (CheckboxWithTooltip("Enable##mov", &settings.movementInertiaEnabled,
				"Enable movement inertia for this weapon type\n(Also requires global movement inertia to be enabled)")) {
				State::hasUnsavedChanges = true;
			}
			
			if (settings.movementInertiaEnabled) {
				if (SliderFloatWithTooltip("Stiffness##mov", &settings.movementStiffness, 10.0f, 500.0f, "%.0f",
					"Spring stiffness for movement response")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Damping##mov", &settings.movementDamping, 1.0f, 50.0f, "%.1f",
					"Damping for movement spring")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Max Offset##mov", &settings.movementMaxOffset, 0.0f, 50.0f, "%.1f",
					"Maximum position offset from strafing")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Max Rotation##mov", &settings.movementMaxRotation, 0.0f, 90.0f, "%.1f deg",
					"Maximum rotation from strafing")) {
					State::hasUnsavedChanges = true;
				}
				
				ImGui::Spacing();
				ImGui::Text("Lateral (Left/Right) Multipliers:");
				
				if (SliderFloatWithTooltip("Left Mult##mov", &settings.movementLeftMult, 0.0f, 5.0f, "%.2f",
					"Multiplier when strafing LEFT\nSet different from Right for asymmetric feel")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Right Mult##mov", &settings.movementRightMult, 0.0f, 5.0f, "%.2f",
					"Multiplier when strafing RIGHT")) {
					State::hasUnsavedChanges = true;
				}
				
				// Show forward/back multipliers only if global forward/back is enabled
				auto* globalSettings = Settings::GetSingleton();
				if (globalSettings->forwardBackInertia) {
					ImGui::Spacing();
					ImGui::Text("Forward/Back Multipliers:");
					
					if (SliderFloatWithTooltip("Forward Mult##mov", &settings.movementForwardMult, 0.0f, 5.0f, "%.2f",
						"Multiplier when moving FORWARD\nMoves arms toward camera")) {
						State::hasUnsavedChanges = true;
					}
					
					if (SliderFloatWithTooltip("Backward Mult##mov", &settings.movementBackwardMult, 0.0f, 5.0f, "%.2f",
						"Multiplier when moving BACKWARD\nMoves arms away from camera")) {
						State::hasUnsavedChanges = true;
					}
				}
				
				ImGui::Spacing();
				
				if (CheckboxWithTooltip("Invert Lateral (L/R)##mov", &settings.invertMovementLateral,
					"Invert the left/right strafe movement inertia")) {
					State::hasUnsavedChanges = true;
				}
				ImGui::SameLine();
				if (CheckboxWithTooltip("Invert Forward/Back##mov", &settings.invertMovementForwardBack,
					"Invert the forward/backward movement inertia")) {
					State::hasUnsavedChanges = true;
				}
			}
			
			ImGui::TreePop();
		}
		
		// Simultaneous Camera+Movement Scaling Section
		if (ImGui::TreeNodeEx("Simultaneous Blend Scaling", ImGuiTreeNodeFlags_None)) {
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Scale effects when both looking AND moving at once");
			ImGui::Spacing();
			
			if (SliderFloatWithTooltip("Activation Threshold##simul", &settings.simultaneousThreshold, 0.0f, 5.0f, "%.2f",
				"How much camera AND movement inertia must BOTH be active\nbefore the scaling multipliers below take effect\n0 = always scale when any activity, higher = need more of both")) {
				State::hasUnsavedChanges = true;
			}
			
			ImGui::Spacing();
			
			if (SliderFloatWithTooltip("Camera Scale##simul", &settings.simultaneousCameraMult, 0.0f, 2.0f, "%.2fx",
				"Scale camera inertia when also moving\n1.00 = full effect, 0.50 = half, 0 = disabled when moving")) {
				State::hasUnsavedChanges = true;
			}
			
			if (SliderFloatWithTooltip("Movement Scale##simul", &settings.simultaneousMovementMult, 0.0f, 2.0f, "%.2fx",
				"Scale movement inertia when also looking around\n1.00 = full effect, 0.50 = half, 0 = disabled when looking")) {
				State::hasUnsavedChanges = true;
			}
			
			ImGui::TreePop();
		}
		
		// Sprint Transition Inertia Section
		if (ImGui::TreeNodeEx("Sprint Transition Inertia", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Momentum on sprint start/stop, then spring settles");
			
			if (CheckboxWithTooltip("Enable##sprint", &settings.sprintInertiaEnabled,
				"Enable sprint transition momentum\nApplies impulse when starting/stopping sprint,\nthen spring settles naturally")) {
				State::hasUnsavedChanges = true;
			}
			
			if (settings.sprintInertiaEnabled) {
				ImGui::Spacing();
				ImGui::Text("Impulse Strength:");
				
				if (SliderFloatWithTooltip("Y Impulse (Depth)##sprint", &settings.sprintImpulseY, 0.0f, 50.0f, "%.1f",
					"Forward/back impulse on sprint transition\nStart: arms lag behind, Stop: arms overshoot forward")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Z Impulse (Height)##sprint", &settings.sprintImpulseZ, 0.0f, 30.0f, "%.1f",
					"Vertical impulse on sprint transition\nStart: slight dip, Stop: slight rise")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Rotation Impulse##sprint", &settings.sprintRotImpulse, 0.0f, 45.0f, "%.1f deg",
					"Rotation impulse on sprint transition\nStart: tilt forward, Stop: tilt back")) {
					State::hasUnsavedChanges = true;
				}
				
				ImGui::Spacing();
				if (SliderFloatWithTooltip("Impulse Blend Time##sprint", &settings.sprintImpulseBlendTime, 0.0f, 1.0f, "%.2f sec",
					"Time to blend into impulse\n0 = instant (snappy), higher = more gradual")) {
					State::hasUnsavedChanges = true;
				}
				
				ImGui::Spacing();
				ImGui::Text("Spring Parameters:");
				
				if (SliderFloatWithTooltip("Stiffness##sprint", &settings.sprintStiffness, 10.0f, 500.0f, "%.0f",
					"Spring stiffness for settling\nHigher = faster return to neutral")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Damping##sprint", &settings.sprintDamping, 1.0f, 50.0f, "%.1f",
					"Damping for sprint spring\nHigher = less oscillation/bounce")) {
					State::hasUnsavedChanges = true;
				}
			}
			
			ImGui::TreePop();
		}
		
		// Jump/Landing Inertia Section
		if (ImGui::TreeNodeEx("Jump/Landing Inertia", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Momentum on jump and landing, scaled by air time");
			
			if (CheckboxWithTooltip("Enable##jump", &settings.jumpInertiaEnabled,
				"Enable jump/landing momentum\nArms dip on takeoff, compress on landing")) {
				State::hasUnsavedChanges = true;
			}
			
			ImGui::Spacing();
			
			if (SliderFloatWithTooltip("Camera Inertia Air Mult##air", &settings.cameraInertiaAirMult, 0.0f, 1.0f, "%.2f",
				"Camera inertia multiplier while airborne\n0 = no camera inertia in air\n1 = full camera inertia in air")) {
				State::hasUnsavedChanges = true;
			}
			
			if (settings.jumpInertiaEnabled) {
				ImGui::Spacing();
				ImGui::Text("Jump (Takeoff) Settings:");
				
				if (SliderFloatWithTooltip("Jump Y Impulse##jump", &settings.jumpImpulseY, 0.0f, 30.0f, "%.1f",
					"Forward/back impulse when jumping\nArms lag behind as you push off")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Jump Z Impulse##jump", &settings.jumpImpulseZ, 0.0f, 30.0f, "%.1f",
					"Vertical impulse when jumping\nArms dip down as you push off")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Jump Rotation##jump", &settings.jumpRotImpulse, 0.0f, 30.0f, "%.1f deg",
					"Rotation impulse when jumping")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Airborne Stiffness##jump", &settings.jumpStiffness, 10.0f, 200.0f, "%.0f",
					"Spring stiffness while in the air\nLow = floaty, retains motion longer")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Airborne Damping##jump", &settings.jumpDamping, 1.0f, 20.0f, "%.1f",
					"Damping while in the air\nLow = more bounce/oscillation in air")) {
					State::hasUnsavedChanges = true;
				}
				
				ImGui::Spacing();
				ImGui::Text("Landing Settings:");
				
				if (SliderFloatWithTooltip("Land Y Impulse##land", &settings.landImpulseY, 0.0f, 30.0f, "%.1f",
					"Forward/back impulse on landing")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Land Z Impulse##land", &settings.landImpulseZ, 0.0f, 50.0f, "%.1f",
					"Vertical impulse on landing\nArms compress down on impact")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Land Rotation##land", &settings.landRotImpulse, 0.0f, 30.0f, "%.1f deg",
					"Rotation impulse on landing")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Land Stiffness##land", &settings.landStiffness, 20.0f, 500.0f, "%.0f",
					"Spring stiffness on landing\nHigh = quick recovery from impact")) {
					State::hasUnsavedChanges = true;
				}
				
				if (SliderFloatWithTooltip("Land Damping##land", &settings.landDamping, 2.0f, 50.0f, "%.1f",
					"Damping on landing\nHigh = less bounce after landing")) {
					State::hasUnsavedChanges = true;
				}
				
				ImGui::Spacing();
				
				if (SliderFloatWithTooltip("Air Time Scale##air", &settings.airTimeImpulseScale, 0.0f, 5.0f, "%.2f",
					"How much air time affects landing impulse\n0 = no scaling, higher = bigger impact from longer falls")) {
					State::hasUnsavedChanges = true;
				}
			}
			
			ImGui::TreePop();
		}
		
		// Pivot Point Section
		if (ImGui::TreeNodeEx("Pivot Point", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Where rotation appears to pivot from");
			
			const char* pivotNames[] = { "Chest (0)", "Right Hand (1)", "Left Hand (2)", "Weapon (3)", "Both Clavicles (4)", "Both Clavicles Offset (5)" };
			const char* pivotTooltips[] = {
				"Chest - weapon swings the most, good for heavy weapons",
				"Right Hand - hand is more stable, weapon swings around it",
				"Left Hand - off-hand is more stable",
				"Weapon - weapon is most stable, body sways around it\nGood for bows/crossbows when aiming",
				"Both Clavicles - each arm has independent springs\nApplies inertia at shoulder level for natural dual wield sway\n(Dual wield types only)",
				"Both Clavicles Offset - same as Both Clavicles but with\npivot compensation to each hand position\n(Dual wield types only)"
			};
			
			int pivotIdx = std::clamp(settings.pivotPoint, 0, 5);
			if (ImGui::BeginCombo("Pivot##pivot", pivotNames[pivotIdx])) {
				for (int i = 0; i < 6; ++i) {
					bool isSelected = (settings.pivotPoint == i);
					if (ImGui::Selectable(pivotNames[i], isSelected)) {
						settings.pivotPoint = i;
						State::hasUnsavedChanges = true;
					}
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("%s", pivotTooltips[i]);
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Where the rotation appears to pivot from.\nAffects the 'feel' of weapon weight.\n'Both Clavicles' modes enable independent shoulder springs for dual wielding.");
			}
			
			ImGui::TreePop();
		}
		
		// Reset button for this weapon type
		ImGui::Spacing();
		if (ImGui::Button("Reset to Defaults")) {
			// Reset to default values
			settings.enabled = true;
			settings.stiffness = 150.0f;
			settings.damping = 12.0f;
			settings.maxOffset = 8.0f;
			settings.maxRotation = 15.0f;
			settings.mass = 1.0f;
			settings.pitchMultiplier = 1.0f;
			settings.rollMultiplier = 1.0f;
			settings.cameraPitchMult = 1.0f;
			settings.invertCameraPitch = false;
			settings.invertCameraYaw = false;
			settings.movementInertiaEnabled = true;
			settings.movementStiffness = 80.0f;
			settings.movementDamping = 6.0f;
			settings.movementMaxOffset = 12.0f;
			settings.movementMaxRotation = 20.0f;
			settings.movementLeftMult = 1.0f;
			settings.movementRightMult = 1.0f;
			settings.movementForwardMult = 0.5f;
			settings.movementBackwardMult = 0.5f;
			settings.invertMovementLateral = false;
			settings.invertMovementForwardBack = false;
			settings.simultaneousThreshold = 0.5f;
			settings.simultaneousCameraMult = 1.0f;
			settings.simultaneousMovementMult = 1.0f;
			settings.sprintInertiaEnabled = false;
			settings.sprintImpulseY = 8.0f;
			settings.sprintImpulseZ = 3.0f;
			settings.sprintRotImpulse = 5.0f;
			settings.sprintImpulseBlendTime = 0.1f;
			settings.sprintStiffness = 60.0f;
			settings.sprintDamping = 5.0f;
			settings.jumpInertiaEnabled = true;
			settings.cameraInertiaAirMult = 0.3f;
			settings.jumpImpulseY = 4.0f;
			settings.jumpImpulseZ = 6.0f;
			settings.jumpRotImpulse = 3.0f;
			settings.jumpStiffness = 40.0f;
			settings.jumpDamping = 3.0f;
			settings.landImpulseY = 3.0f;
			settings.landImpulseZ = 10.0f;
			settings.landRotImpulse = 5.0f;
			settings.landStiffness = 120.0f;
			settings.landDamping = 10.0f;
			settings.airTimeImpulseScale = 1.5f;
			settings.pivotPoint = 0;
			State::hasUnsavedChanges = true;
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Reset this weapon type to default values");
		}
		
		// Pop the dimming style if we pushed it
		if (!settings.enabled) {
			ImGui::PopStyleVar();
		}
		
		ImGui::PopID();
	}
	
	void DrawSpecificWeaponSettings()
	{
		auto* presets = InertiaPresets::GetSingleton();
		
		if (ImGui::CollapsingHeader("Specific Weapon Presets (by EditorID)", State::specificWeaponExpanded ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
			State::specificWeaponExpanded = true;
			
			ImGui::TextWrapped(
				"Create custom inertia settings for specific weapons using their EditorID. "
				"These override the weapon type settings above. Use the console command "
				"'help <weapon name>' to find EditorIDs.");
			ImGui::Spacing();
			
			// Show currently equipped weapon info
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (player) {
				auto* rightWeapon = player->GetEquippedObject(false);
				if (rightWeapon) {
					const char* rawEditorID = rightWeapon->GetFormEditorID();
					const char* name = rightWeapon->GetName();
					RE::FormID formID = rightWeapon->GetFormID();
					
					// Get EditorID (Native EditorID Fix makes this work for all forms)
					std::string weaponID;
					if (rawEditorID && rawEditorID[0] != '\0') {
						weaponID = rawEditorID;
					} else {
						// Fallback - shouldn't happen with Native EditorID Fix installed
						weaponID = std::format("0x{:08X}", formID);
					}
					
					ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), 
						"Equipped: %s (EditorID: %s)", name ? name : "Unknown", weaponID.c_str());
					
					// Button to create preset for equipped weapon
					if (!presets->HasSpecificWeaponSettings(weaponID)) {
						if (ImGui::Button("Create Preset for Equipped Weapon")) {
							// Determine weapon type
							WeaponType weaponType = WeaponType::Unarmed;
							if (rightWeapon->IsWeapon()) {
								auto* weapon = rightWeapon->As<RE::TESObjectWEAP>();
								if (weapon) {
									weaponType = Settings::ToWeaponType(weapon->GetWeaponType());
								}
							}
							
							// Create the preset (copies from weapon type settings)
							presets->GetOrCreateSpecificWeaponSettings(weaponID, weaponType);
							presets->SaveSpecificWeaponPreset(weaponID);
							State::hasUnsavedChanges = true;
						}
						if (ImGui::IsItemHovered()) {
							ImGui::SetTooltip("Create a custom preset for this specific weapon, copied from its weapon type defaults");
						}
					} else {
						ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "(Has custom preset)");
					}
				} else {
					ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No weapon equipped");
				}
			}
			
			ImGui::Spacing();
			ImGui::Separator();
			
			// Manual EditorID entry
			ImGui::Text("Create preset manually:");
			ImGui::SetNextItemWidth(200.0f);
			ImGui::InputText("EditorID", State::newWeaponEditorID, sizeof(State::newWeaponEditorID));
			ImGui::SameLine();
			
			if (ImGui::Button("Create")) {
				if (strlen(State::newWeaponEditorID) > 0) {
					if (!presets->HasSpecificWeaponSettings(State::newWeaponEditorID)) {
						presets->GetOrCreateSpecificWeaponSettings(State::newWeaponEditorID, WeaponType::Unarmed);
						presets->SaveSpecificWeaponPreset(State::newWeaponEditorID);
						State::hasUnsavedChanges = true;
					}
				}
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Create a new preset for the specified EditorID");
			}
			
			ImGui::Spacing();
			ImGui::Separator();
			
			// List existing specific weapon presets
			auto savedPresets = presets->GetSavedSpecificWeaponPresets();
			
			if (savedPresets.empty()) {
				ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No specific weapon presets created yet.");
			} else {
				ImGui::Text("Existing Presets (%zu):", savedPresets.size());
				
				ImGui::BeginChild("SpecificWeaponList", ImVec2(0, 200), true);
				
				for (size_t i = 0; i < savedPresets.size(); ++i) {
					const auto& editorID = savedPresets[i];
					bool isSelected = (State::selectedSpecificWeaponIndex == static_cast<int>(i));
					
					if (ImGui::Selectable(editorID.c_str(), isSelected)) {
						State::selectedSpecificWeaponIndex = static_cast<int>(i);
					}
				}
				
				ImGui::EndChild();
				
				// Edit/Delete buttons
				if (State::selectedSpecificWeaponIndex >= 0 && 
					State::selectedSpecificWeaponIndex < static_cast<int>(savedPresets.size())) {
					
					const std::string& selectedEditorID = savedPresets[State::selectedSpecificWeaponIndex];
					
					ImGui::Text("Selected: %s", selectedEditorID.c_str());
					
					// Get settings for editing
					auto* settings = presets->GetSpecificWeaponSettings(selectedEditorID);
					if (settings) {
						// Cast away const for editing (we mark dirty when changed)
						auto& mutableSettings = presets->GetOrCreateSpecificWeaponSettings(
							selectedEditorID, WeaponType::Unarmed);
						
						ImGui::PushID(selectedEditorID.c_str());
						DrawWeaponInertiaEditor(mutableSettings, selectedEditorID.c_str());
						ImGui::PopID();
						
						// Save button for this preset
						if (ImGui::Button("Save This Preset")) {
							presets->SaveSpecificWeaponPreset(selectedEditorID);
						}
						if (ImGui::IsItemHovered()) {
							ImGui::SetTooltip("Save changes to this specific weapon preset");
						}
						
						ImGui::SameLine();
						
						// Delete button
						if (ImGui::Button("Delete Preset")) {
							presets->RemoveSpecificWeaponSettings(selectedEditorID);
							State::selectedSpecificWeaponIndex = -1;
						}
						if (ImGui::IsItemHovered()) {
							ImGui::SetTooltip("Delete this specific weapon preset (will use weapon type settings instead)");
						}
					}
				}
			}
		} else {
			State::specificWeaponExpanded = false;
		}
	}
	
	void DrawSaveLoadButtons()
	{
		auto* settings = Settings::GetSingleton();
		auto* presets = InertiaPresets::GetSingleton();
		
		ImGui::Text("INI Configuration (Global Settings):");
		
		// Save button
		if (ImGui::Button("Save to INI")) {
			settings->Save();
			State::hasUnsavedChanges = false;
			RE::DebugNotification("FP Inertia: Settings saved to INI");
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Save global settings to FPInertia.ini\nChanges will persist after game restart");
		}
		
		ImGui::SameLine();
		
		// Reload button
		if (ImGui::Button("Reload from INI")) {
			settings->Load();
			State::hasUnsavedChanges = false;
			RE::DebugNotification("FP Inertia: Global settings reloaded from INI");
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Reload global settings from INI file\n(Weapon presets are separate - use 'Reload Presets from JSON')");
		}
		
		ImGui::SameLine();
		
		// Reset All button
		if (ImGui::Button("Reset All to Defaults")) {
			settings->Load();
			// Reset presets to INI values (ignores JSON files)
			presets->ResetToINIValues();
			State::hasUnsavedChanges = true;
			RE::DebugNotification("FP Inertia: Settings reset to INI defaults");
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Reset all settings to plugin defaults\n(Changes are not saved until you click Save)");
		}
		
		ImGui::Spacing();
		ImGui::Separator();
		
		// Show active preset info
		std::string presetLabel = std::format("Active Preset: {} (Weapon Type Settings)", presets->GetActivePresetName());
		ImGui::Text("%s", presetLabel.c_str());
		
		// Save weapon type presets to JSON
		std::string saveButtonLabel = std::format("Save to '{}.json'", presets->GetActivePresetName());
		if (ImGui::Button(saveButtonLabel.c_str())) {
			presets->SaveWeaponTypePresets();
			presets->ClearDirty();
			State::hasUnsavedChanges = false;
			RE::DebugNotification(std::format("FP Inertia: Saved preset '{}'", presets->GetActivePresetName()).c_str());
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Save current weapon type settings to:\nData/SKSE/Plugins/FPInertia/%s.json", 
				presets->GetActivePresetName().c_str());
		}
		
		ImGui::SameLine();
		
		// Reload presets from JSON
		std::string reloadButtonLabel = std::format("Reload '{}.json'", presets->GetActivePresetName());
		if (ImGui::Button(reloadButtonLabel.c_str())) {
			presets->LoadAllPresets();
			presets->ClearDirty();
			State::hasUnsavedChanges = false;
			RE::DebugNotification(std::format("FP Inertia: Reloaded preset '{}'", presets->GetActivePresetName()).c_str());
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Reload weapon type settings from:\nData/SKSE/Plugins/FPInertia/%s.json", 
				presets->GetActivePresetName().c_str());
		}
		
		// Status text
		if (State::hasUnsavedChanges || presets->IsDirty()) {
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), 
				"You have unsaved changes. Save to keep them after restart.");
		} else {
			ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), 
				"All settings saved.");
		}
		
		// Path info
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "INI: Data/SKSE/Plugins/FPInertia.ini");
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Preset: Data/SKSE/Plugins/FPInertia/%s.json", 
			presets->GetActivePresetName().c_str());
	}
}

