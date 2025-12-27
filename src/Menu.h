#pragma once

#include "SKSEMenuFramework.h"
#include "Settings.h"
#include "InertiaPresets.h"

// Menu integration with SKSE Menu Framework
namespace Menu
{
	// Register with SKSE Menu Framework
	void Register();
	
	// Main render callback for SKSE Menu Framework
	void __stdcall Render();
	
	// Internal state for menu
	namespace State
	{
		// Editing state
		inline bool initialized{ false };
		inline bool hasUnsavedChanges{ false };
		
		// Preset profile management
		inline int selectedPresetIndex{ 0 };
		inline char newPresetName[128]{ "" };
		inline bool showCreatePresetPopup{ false };
		inline bool showDeletePresetConfirm{ false };
		inline std::vector<std::string> cachedPresetList;
		inline bool presetListAttemptedRefresh{ false };  // Prevents repeated refresh attempts
		
		// Selected weapon type for per-weapon settings
		inline int selectedWeaponTypeIndex{ 0 };
		
		// Collapsible section states
		inline bool generalExpanded{ true };
		inline bool settlingExpanded{ false };
		inline bool movementExpanded{ false };
		inline bool actionBlendExpanded{ false };
		inline bool handsExpanded{ false };
		inline bool debugExpanded{ false };
		inline bool weaponSettingsExpanded{ true };
		inline bool specificWeaponExpanded{ false };
		
		// Specific weapon preset management
		inline char newWeaponEditorID[256]{ "" };
		inline int selectedSpecificWeaponIndex{ -1 };
		
		// Copy to preset dialog state
		inline bool showCopyToPresetPopup{ false };
		inline int copyTargetPresetIndex{ 0 };
		inline std::string copySourceWeaponType;
		inline std::string copySourcePreset;
		
		// Native EditorID Fix detection
		inline bool nativeEditorIDFixChecked{ false };
		inline bool nativeEditorIDFixInstalled{ true };  // Assume true until checked
	}
	
	// Weapon type info for dropdown (supports both standard and custom types)
	struct WeaponTypeEntry
	{
		std::string displayName;
		std::string internalName;  // INI section name or custom type name
		WeaponType type;           // Enum for standard types (ignored for custom)
		bool isCustomType;         // True if this is a keyword-based custom type
		
		WeaponTypeEntry(const char* display, const char* internal, WeaponType t)
			: displayName(display), internalName(internal), type(t), isCustomType(false) {}
		
		WeaponTypeEntry(const std::string& customTypeName)
			: displayName(customTypeName), internalName(customTypeName), type(WeaponType::Unarmed), isCustomType(true) {}
	};
	
	// Get array of weapon types for dropdown (refreshes to include custom types)
	const std::vector<WeaponTypeEntry>& GetWeaponTypes();
	void RefreshWeaponTypesList();  // Rebuild list including custom types
	
	// Get mutable settings for a weapon type (from presets)
	WeaponInertiaSettings& GetWeaponSettingsForEditing(WeaponType a_type);
	
	// Draw helper functions
	void DrawHeader();
	void DrawPresetSelector();  // Preset profile dropdown at top
	void DrawGeneralSettings();
	void DrawSettlingSettings();
	void DrawMovementInertiaSettings();
	void DrawActionBlendSettings();
	void DrawHandsSettings();
	void DrawDebugSettings();
	void DrawWeaponTypeSettings();
	void DrawSpecificWeaponSettings();
	void DrawWeaponInertiaEditor(WeaponInertiaSettings& settings, const char* label);
	void DrawSaveLoadButtons();
	
	// Refresh cached preset list
	void RefreshPresetList();
	
	// Helper for sliders with tooltips
	bool SliderFloatWithTooltip(const char* label, float* value, float min, float max, const char* format, const char* tooltip);
	bool CheckboxWithTooltip(const char* label, bool* value, const char* tooltip);
	bool SliderIntWithTooltip(const char* label, int* value, int min, int max, const char* format, const char* tooltip);
}

