#pragma once

#include "Settings.h"
#include <filesystem>
#include <unordered_map>
#include <shared_mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Key for weapon type preset lookup
struct WeaponTypeKey
{
	WeaponType type;
	
	bool operator==(const WeaponTypeKey& other) const
	{
		return type == other.type;
	}
};

// Hash function for WeaponTypeKey
struct WeaponTypeKeyHash
{
	std::size_t operator()(const WeaponTypeKey& key) const
	{
		return std::hash<int>()(static_cast<int>(key.type));
	}
};

// Key for specific weapon preset lookup (by EditorID)
struct SpecificWeaponKey
{
	std::string editorID;
	
	bool operator==(const SpecificWeaponKey& other) const
	{
		return editorID == other.editorID;
	}
};

// Keyword-to-weapon-type mapping for custom weapon types
struct KeywordMapping
{
	std::vector<std::string> keywords;            // Keyword EditorIDs that must ALL be present on the weapon
	std::vector<RE::BGSKeyword*> resolvedKeywords; // Resolved keyword pointers (cached at load time for performance)
	std::string weaponTypeName;                   // The custom type name (e.g., "OneHandRapier")
	bool keywordsResolved{ false };               // Whether keywords have been resolved to pointers
	
	// For sorting by specificity (more keywords = higher priority)
	bool operator<(const KeywordMapping& other) const
	{
		return keywords.size() > other.keywords.size();  // Descending order
	}
};

// Hash function for SpecificWeaponKey
struct SpecificWeaponKeyHash
{
	std::size_t operator()(const SpecificWeaponKey& key) const
	{
		return std::hash<std::string>()(key.editorID);
	}
};

// JSON serialization for WeaponInertiaSettings
void to_json(json& j, const WeaponInertiaSettings& s);
void from_json(const json& j, WeaponInertiaSettings& s);

class InertiaPresets
{
public:
	static InertiaPresets* GetSingleton()
	{
		static InertiaPresets singleton;
		return &singleton;
	}

	// Initialize the preset manager
	void Init();
	
	// Get settings for a weapon by EditorID (returns specific if exists, else type-based)
	const WeaponInertiaSettings& GetWeaponSettings(const std::string& a_editorID, WeaponType a_type) const;
	WeaponInertiaSettings& GetWeaponSettingsMutable(const std::string& a_editorID, WeaponType a_type);
	
	// Get settings for a weapon with keyword checking (full priority: specific -> keyword -> type)
	const WeaponInertiaSettings& GetWeaponSettingsWithKeywords(const std::string& a_editorID, RE::TESObjectWEAP* a_weapon, WeaponType a_type) const;
	
	// Get settings for a weapon type (fallback)
	const WeaponInertiaSettings& GetWeaponTypeSettings(WeaponType a_type) const;
	WeaponInertiaSettings& GetWeaponTypeSettingsMutable(WeaponType a_type);
	
	// Get settings for a specific weapon by EditorID
	const WeaponInertiaSettings* GetSpecificWeaponSettings(const std::string& a_editorID) const;
	WeaponInertiaSettings& GetOrCreateSpecificWeaponSettings(const std::string& a_editorID, WeaponType a_baseType);
	
	// Get settings for a custom keyword-based weapon type
	const WeaponInertiaSettings* GetCustomWeaponTypeSettings(const std::string& a_customTypeName) const;
	WeaponInertiaSettings& GetCustomWeaponTypeSettingsMutable(const std::string& a_customTypeName);
	
	// Check if specific weapon preset exists
	bool HasSpecificWeaponSettings(const std::string& a_editorID) const;
	
	// Remove specific weapon preset (will use type-based instead)
	void RemoveSpecificWeaponSettings(const std::string& a_editorID);
	
	// Preset file management
	void SaveWeaponTypePresets();
	void LoadWeaponTypePresets();
	void SaveSpecificWeaponPreset(const std::string& a_editorID);
	void LoadSpecificWeaponPreset(const std::string& a_editorID);
	void LoadAllPresets();
	
	// Reset presets to INI values (ignores JSON files)
	void ResetToINIValues();
	
	// Get all saved specific weapon preset names
	std::vector<std::string> GetSavedSpecificWeaponPresets() const;
	
	// Preset profile management (multiple weapon type preset files)
	std::vector<std::string> GetAvailablePresets() const;  // List JSON files in FPInertia folder (excluding Weapons subfolder)
	const std::string& GetActivePresetName() const { return activePresetName; }
	void SetActivePreset(const std::string& a_name);  // Switch to a different preset
	void CreateNewPreset(const std::string& a_name);  // Create new preset file from current values
	void DuplicatePreset(const std::string& a_sourceName, const std::string& a_newName);  // Copy a preset
	void DeletePreset(const std::string& a_name);  // Delete a preset file
	void RenamePreset(const std::string& a_oldName, const std::string& a_newName);  // Rename a preset
	void SaveActivePresetSetting();  // Save active preset name to INI
	void LoadActivePresetSetting();  // Load active preset name from INI
	
	// Path helpers
	std::filesystem::path GetPresetFolderPath() const;
	std::filesystem::path GetWeaponTypePresetsPath() const;  // Uses active preset name
	std::filesystem::path GetPresetPath(const std::string& a_presetName) const;  // Get path for any preset
	std::filesystem::path GetSpecificWeaponPresetPath(const std::string& a_editorID) const;
	
	// Dirty tracking
	bool IsDirty() const { return isDirty; }
	void MarkDirty() { isDirty = true; }
	void ClearDirty() { isDirty = false; }
	
	// Settings version (incremented when presets change, for cache invalidation)
	uint32_t GetSettingsVersion() const { return settingsVersion; }
	void IncrementSettingsVersion() { settingsVersion++; }
	
	// Weapon type name helpers
	static const char* GetWeaponTypeName(WeaponType a_type);
	static const char* GetWeaponTypeDisplayName(WeaponType a_type);
	static WeaponType ParseWeaponTypeName(const std::string& a_name);
	
	// Custom keyword-based weapon types
	void LoadKeywordMappings();  // Load all .txt files from WeaponTypeMappings folder
	void ResolveKeywordPointers();  // Resolve keyword EditorIDs to pointers (call after game data loaded)
	std::string GetBestKeywordMatch(RE::TESObjectWEAP* a_weapon) const;  // Find most specific keyword match
	const std::vector<std::string>& GetCustomWeaponTypes() const { return customWeaponTypeNames; }
	bool IsCustomWeaponType(const std::string& a_name) const;
	void EnsureCustomTypesInPreset();  // Add missing custom types to current preset with defaults
	std::filesystem::path GetKeywordMappingsFolderPath() const;

private:
	InertiaPresets() = default;
	~InertiaPresets() = default;
	InertiaPresets(const InertiaPresets&) = delete;
	InertiaPresets(InertiaPresets&&) = delete;
	InertiaPresets& operator=(const InertiaPresets&) = delete;
	InertiaPresets& operator=(InertiaPresets&&) = delete;

	// Per-weapon-type settings (the defaults from INI, can be modified in menu)
	std::unordered_map<WeaponTypeKey, WeaponInertiaSettings, WeaponTypeKeyHash> weaponTypeSettings;
	
	// Per-specific-weapon settings (editorID -> settings)
	std::unordered_map<SpecificWeaponKey, WeaponInertiaSettings, SpecificWeaponKeyHash> specificWeaponSettings;
	
	// Custom keyword-based weapon types (loaded from mapping files)
	std::vector<KeywordMapping> keywordMappings;  // Sorted by specificity (most keywords first)
	std::vector<std::string> customWeaponTypeNames;  // Unique list of custom type names
	
	// Custom weapon type settings (string key -> settings, for keyword-based types)
	std::unordered_map<std::string, WeaponInertiaSettings> customWeaponTypeSettings;
	
	// Default/empty settings for fallback
	WeaponInertiaSettings defaultSettings;
	
	// Track unsaved changes
	bool isDirty{ false };
	
	// Settings version counter (incremented when presets change)
	uint32_t settingsVersion{ 0 };
	
	// Active preset name (without .json extension)
	std::string activePresetName{ "WeaponTypes" };
	
	// Thread safety
	mutable std::shared_mutex presetMutex;
	
	// Ensure preset folder exists
	void EnsurePresetFolderExists();
	
	// Initialize default weapon type settings (called if no presets exist)
	void InitializeDefaultSettings();
};

