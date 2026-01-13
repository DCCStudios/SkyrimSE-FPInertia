#include "InertiaPresets.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include <SimpleIni.h>

namespace
{
	struct WeaponTypeInfo
	{
		WeaponType type;
		const char* name;
		const char* displayName;
	};

	const std::vector<WeaponTypeInfo> g_weaponTypes = {
		{ WeaponType::Unarmed, "Unarmed", "Unarmed" },
		{ WeaponType::OneHandSword, "OneHandSword", "One-Handed Sword" },
		{ WeaponType::OneHandDagger, "OneHandDagger", "Dagger" },
		{ WeaponType::OneHandAxe, "OneHandAxe", "One-Handed Axe" },
		{ WeaponType::OneHandMace, "OneHandMace", "Mace" },
		{ WeaponType::TwoHandSword, "TwoHandSword", "Two-Handed Sword" },
		{ WeaponType::TwoHandAxe, "TwoHandAxe", "Two-Handed Axe/Hammer" },
		{ WeaponType::Bow, "Bow", "Bow" },
		{ WeaponType::Staff, "Staff", "Staff" },
		{ WeaponType::Crossbow, "Crossbow", "Crossbow" },
		{ WeaponType::Shield, "Shield", "Shield" },
		{ WeaponType::Spell, "Spell", "Spell" },
		// Dual wield types (detected with priority)
		{ WeaponType::DualWieldWeapons, "DualWieldWeapons", "Dual Wield Weapons" },
		{ WeaponType::DualWieldMagic, "DualWieldMagic", "Dual Wield Magic" },
		{ WeaponType::SpellAndWeapon, "SpellAndWeapon", "Spell + Weapon" }
	};
}

// JSON serialization for WeaponInertiaSettings
void to_json(json& j, const WeaponInertiaSettings& s)
{
	j = json{
		// Master toggle
		{"enabled", s.enabled},
		
		// Camera inertia
		{"stiffness", s.stiffness},
		{"damping", s.damping},
		{"maxOffset", s.maxOffset},
		{"maxRotation", s.maxRotation},
		{"mass", s.mass},
		{"pitchMultiplier", s.pitchMultiplier},
		{"rollMultiplier", s.rollMultiplier},
		{"cameraPitchMult", s.cameraPitchMult},
		{"invertCameraPitch", s.invertCameraPitch},
		{"invertCameraYaw", s.invertCameraYaw},
		{"pivotPoint", s.pivotPoint},
		
		// Movement inertia
		{"movementInertiaEnabled", s.movementInertiaEnabled},
		{"movementStiffness", s.movementStiffness},
		{"movementDamping", s.movementDamping},
		{"movementMaxOffset", s.movementMaxOffset},
		{"movementMaxRotation", s.movementMaxRotation},
		{"movementLeftMult", s.movementLeftMult},
		{"movementRightMult", s.movementRightMult},
		{"movementForwardMult", s.movementForwardMult},
		{"movementBackwardMult", s.movementBackwardMult},
		{"invertMovementLateral", s.invertMovementLateral},
		{"invertMovementForwardBack", s.invertMovementForwardBack},
		
		// Simultaneous camera+movement scaling
		{"simultaneousThreshold", s.simultaneousThreshold},
		{"simultaneousCameraMult", s.simultaneousCameraMult},
		{"simultaneousMovementMult", s.simultaneousMovementMult},
		
		// Sprint inertia
		{"sprintInertiaEnabled", s.sprintInertiaEnabled},
		{"sprintImpulseY", s.sprintImpulseY},
		{"sprintImpulseZ", s.sprintImpulseZ},
		{"sprintRotImpulse", s.sprintRotImpulse},
		{"sprintImpulseBlendTime", s.sprintImpulseBlendTime},
		{"sprintStiffness", s.sprintStiffness},
		{"sprintDamping", s.sprintDamping},
		
		// Jump inertia
		{"jumpInertiaEnabled", s.jumpInertiaEnabled},
		{"cameraInertiaAirMult", s.cameraInertiaAirMult},
		{"jumpImpulseY", s.jumpImpulseY},
		{"jumpImpulseZ", s.jumpImpulseZ},
		{"jumpRotImpulse", s.jumpRotImpulse},
		{"fallImpulseY", s.fallImpulseY},
		{"fallImpulseZ", s.fallImpulseZ},
		{"fallRotImpulse", s.fallRotImpulse},
		{"jumpStiffness", s.jumpStiffness},
		{"jumpDamping", s.jumpDamping},
		{"landImpulseY", s.landImpulseY},
		{"landImpulseZ", s.landImpulseZ},
		{"landRotImpulse", s.landRotImpulse},
		{"landStiffness", s.landStiffness},
		{"landDamping", s.landDamping},
		{"airTimeImpulseScale", s.airTimeImpulseScale},
		
		// Stance multipliers
		{"enableStanceMultipliers", s.enableStanceMultipliers},
		{"stanceMultipliers", s.stanceMultipliers},
		{"stanceInvertCamera", s.stanceInvertCamera},
		{"stanceInvertMovement", s.stanceInvertMovement}
	};
}

void from_json(const json& j, WeaponInertiaSettings& s)
{
	// Master toggle
	if (j.contains("enabled")) j.at("enabled").get_to(s.enabled);
	
	// Camera inertia
	if (j.contains("stiffness")) j.at("stiffness").get_to(s.stiffness);
	if (j.contains("damping")) j.at("damping").get_to(s.damping);
	if (j.contains("maxOffset")) j.at("maxOffset").get_to(s.maxOffset);
	if (j.contains("maxRotation")) j.at("maxRotation").get_to(s.maxRotation);
	if (j.contains("mass")) j.at("mass").get_to(s.mass);
	if (j.contains("pitchMultiplier")) j.at("pitchMultiplier").get_to(s.pitchMultiplier);
	if (j.contains("rollMultiplier")) j.at("rollMultiplier").get_to(s.rollMultiplier);
	if (j.contains("cameraPitchMult")) j.at("cameraPitchMult").get_to(s.cameraPitchMult);
	if (j.contains("invertCameraPitch")) j.at("invertCameraPitch").get_to(s.invertCameraPitch);
	if (j.contains("invertCameraYaw")) j.at("invertCameraYaw").get_to(s.invertCameraYaw);
	// Migration: if old invertCamera was set, apply to both new toggles
	if (j.contains("invertCamera") && j.at("invertCamera").get<bool>()) {
		s.invertCameraPitch = true;
		s.invertCameraYaw = true;
	}
	if (j.contains("pivotPoint")) j.at("pivotPoint").get_to(s.pivotPoint);
	
	// Movement inertia
	if (j.contains("movementInertiaEnabled")) j.at("movementInertiaEnabled").get_to(s.movementInertiaEnabled);
	if (j.contains("movementStiffness")) j.at("movementStiffness").get_to(s.movementStiffness);
	if (j.contains("movementDamping")) j.at("movementDamping").get_to(s.movementDamping);
	if (j.contains("movementMaxOffset")) j.at("movementMaxOffset").get_to(s.movementMaxOffset);
	if (j.contains("movementMaxRotation")) j.at("movementMaxRotation").get_to(s.movementMaxRotation);
	if (j.contains("movementLeftMult")) j.at("movementLeftMult").get_to(s.movementLeftMult);
	if (j.contains("movementRightMult")) j.at("movementRightMult").get_to(s.movementRightMult);
	if (j.contains("movementForwardMult")) j.at("movementForwardMult").get_to(s.movementForwardMult);
	if (j.contains("movementBackwardMult")) j.at("movementBackwardMult").get_to(s.movementBackwardMult);
	if (j.contains("invertMovementLateral")) j.at("invertMovementLateral").get_to(s.invertMovementLateral);
	if (j.contains("invertMovementForwardBack")) j.at("invertMovementForwardBack").get_to(s.invertMovementForwardBack);
	// Migration: if old invertMovement was set, apply to both new toggles
	if (j.contains("invertMovement") && j.at("invertMovement").get<bool>()) {
		s.invertMovementLateral = true;
		s.invertMovementForwardBack = true;
	}
	
	// Simultaneous camera+movement scaling
	if (j.contains("simultaneousThreshold")) j.at("simultaneousThreshold").get_to(s.simultaneousThreshold);
	if (j.contains("simultaneousCameraMult")) j.at("simultaneousCameraMult").get_to(s.simultaneousCameraMult);
	if (j.contains("simultaneousMovementMult")) j.at("simultaneousMovementMult").get_to(s.simultaneousMovementMult);
	
	// Sprint inertia
	if (j.contains("sprintInertiaEnabled")) j.at("sprintInertiaEnabled").get_to(s.sprintInertiaEnabled);
	if (j.contains("sprintImpulseY")) j.at("sprintImpulseY").get_to(s.sprintImpulseY);
	if (j.contains("sprintImpulseZ")) j.at("sprintImpulseZ").get_to(s.sprintImpulseZ);
	if (j.contains("sprintRotImpulse")) j.at("sprintRotImpulse").get_to(s.sprintRotImpulse);
	if (j.contains("sprintImpulseBlendTime")) j.at("sprintImpulseBlendTime").get_to(s.sprintImpulseBlendTime);
	if (j.contains("sprintStiffness")) j.at("sprintStiffness").get_to(s.sprintStiffness);
	if (j.contains("sprintDamping")) j.at("sprintDamping").get_to(s.sprintDamping);
	
	// Jump inertia
	if (j.contains("jumpInertiaEnabled")) j.at("jumpInertiaEnabled").get_to(s.jumpInertiaEnabled);
	if (j.contains("cameraInertiaAirMult")) j.at("cameraInertiaAirMult").get_to(s.cameraInertiaAirMult);
	if (j.contains("jumpImpulseY")) j.at("jumpImpulseY").get_to(s.jumpImpulseY);
	if (j.contains("jumpImpulseZ")) j.at("jumpImpulseZ").get_to(s.jumpImpulseZ);
	if (j.contains("jumpRotImpulse")) j.at("jumpRotImpulse").get_to(s.jumpRotImpulse);
	if (j.contains("fallImpulseY")) j.at("fallImpulseY").get_to(s.fallImpulseY);
	if (j.contains("fallImpulseZ")) j.at("fallImpulseZ").get_to(s.fallImpulseZ);
	if (j.contains("fallRotImpulse")) j.at("fallRotImpulse").get_to(s.fallRotImpulse);
	if (j.contains("jumpStiffness")) j.at("jumpStiffness").get_to(s.jumpStiffness);
	if (j.contains("jumpDamping")) j.at("jumpDamping").get_to(s.jumpDamping);
	if (j.contains("landImpulseY")) j.at("landImpulseY").get_to(s.landImpulseY);
	if (j.contains("landImpulseZ")) j.at("landImpulseZ").get_to(s.landImpulseZ);
	if (j.contains("landRotImpulse")) j.at("landRotImpulse").get_to(s.landRotImpulse);
	if (j.contains("landStiffness")) j.at("landStiffness").get_to(s.landStiffness);
	if (j.contains("landDamping")) j.at("landDamping").get_to(s.landDamping);
	if (j.contains("airTimeImpulseScale")) j.at("airTimeImpulseScale").get_to(s.airTimeImpulseScale);
	
	// Stance multipliers
	if (j.contains("enableStanceMultipliers")) j.at("enableStanceMultipliers").get_to(s.enableStanceMultipliers);
	if (j.contains("stanceMultipliers")) j.at("stanceMultipliers").get_to(s.stanceMultipliers);
	if (j.contains("stanceInvertCamera")) j.at("stanceInvertCamera").get_to(s.stanceInvertCamera);
	if (j.contains("stanceInvertMovement")) j.at("stanceInvertMovement").get_to(s.stanceInvertMovement);
}

const char* InertiaPresets::GetWeaponTypeName(WeaponType a_type)
{
	for (const auto& info : g_weaponTypes) {
		if (info.type == a_type) {
			return info.name;
		}
	}
	return "Unknown";
}

const char* InertiaPresets::GetWeaponTypeDisplayName(WeaponType a_type)
{
	for (const auto& info : g_weaponTypes) {
		if (info.type == a_type) {
			return info.displayName;
		}
	}
	return "Unknown";
}

WeaponType InertiaPresets::ParseWeaponTypeName(const std::string& a_name)
{
	for (const auto& info : g_weaponTypes) {
		if (a_name == info.name) {
			return info.type;
		}
	}
	return WeaponType::Unarmed;
}

void InertiaPresets::Init()
{
	EnsurePresetFolderExists();
	
	// Load keyword mappings first (so we know what custom types exist)
	LoadKeywordMappings();
	
	// Load which preset was active last session
	LoadActivePresetSetting();
	
	// Try to load existing presets for the active preset
	LoadAllPresets();
	
	// If no weapon type presets were loaded, initialize with defaults from INI
	if (weaponTypeSettings.empty()) {
		InitializeDefaultSettings();
		// Save the default preset
		SaveWeaponTypePresets();
	}
	
	// Ensure custom weapon types from mappings are in the preset
	EnsureCustomTypesInPreset();
	
	logger::info("InertiaPresets initialized with preset '{}', {} weapon types, {} custom types, and {} specific weapons",
		activePresetName, weaponTypeSettings.size(), customWeaponTypeSettings.size(), specificWeaponSettings.size());
}

void InertiaPresets::InitializeDefaultSettings()
{
	auto* settings = Settings::GetSingleton();
	
	// Copy current weapon settings from Settings singleton (loaded from INI)
	std::unique_lock lock(presetMutex);
	
	weaponTypeSettings[WeaponTypeKey{WeaponType::Unarmed}] = settings->unarmed;
	weaponTypeSettings[WeaponTypeKey{WeaponType::OneHandSword}] = settings->oneHandSword;
	weaponTypeSettings[WeaponTypeKey{WeaponType::OneHandDagger}] = settings->oneHandDagger;
	weaponTypeSettings[WeaponTypeKey{WeaponType::OneHandAxe}] = settings->oneHandAxe;
	weaponTypeSettings[WeaponTypeKey{WeaponType::OneHandMace}] = settings->oneHandMace;
	weaponTypeSettings[WeaponTypeKey{WeaponType::TwoHandSword}] = settings->twoHandSword;
	weaponTypeSettings[WeaponTypeKey{WeaponType::TwoHandAxe}] = settings->twoHandAxe;
	weaponTypeSettings[WeaponTypeKey{WeaponType::Bow}] = settings->bow;
	weaponTypeSettings[WeaponTypeKey{WeaponType::Staff}] = settings->staff;
	weaponTypeSettings[WeaponTypeKey{WeaponType::Crossbow}] = settings->crossbow;
	weaponTypeSettings[WeaponTypeKey{WeaponType::Shield}] = settings->shield;
	weaponTypeSettings[WeaponTypeKey{WeaponType::Spell}] = settings->spell;
	// Dual wield types
	weaponTypeSettings[WeaponTypeKey{WeaponType::DualWieldWeapons}] = settings->dualWieldWeapons;
	weaponTypeSettings[WeaponTypeKey{WeaponType::DualWieldMagic}] = settings->dualWieldMagic;
	weaponTypeSettings[WeaponTypeKey{WeaponType::SpellAndWeapon}] = settings->spellAndWeapon;
	
	logger::info("Initialized default weapon type settings from INI");
}

void InertiaPresets::ResetToINIValues()
{
	// Clear existing presets
	{
		std::unique_lock lock(presetMutex);
		weaponTypeSettings.clear();
		specificWeaponSettings.clear();
	}
	
	// Re-copy from INI
	InitializeDefaultSettings();
	isDirty = true;
	
	logger::info("Reset all presets to INI values");
}

std::filesystem::path InertiaPresets::GetPresetFolderPath() const
{
	return std::filesystem::path("Data/SKSE/Plugins/FPInertia");
}

std::filesystem::path InertiaPresets::GetWeaponTypePresetsPath() const
{
	return GetPresetPath(activePresetName);
}

std::filesystem::path InertiaPresets::GetPresetPath(const std::string& a_presetName) const
{
	return GetPresetFolderPath() / (a_presetName + ".json");
}

std::filesystem::path InertiaPresets::GetSpecificWeaponPresetPath(const std::string& a_editorID) const
{
	// Sanitize EditorID for filename
	std::string filename = a_editorID;
	for (char& c : filename) {
		if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
			c = '_';
		}
	}
	return GetPresetFolderPath() / "Weapons" / (filename + ".json");
}

void InertiaPresets::EnsurePresetFolderExists()
{
	auto path = GetPresetFolderPath();
	if (!std::filesystem::exists(path)) {
		std::filesystem::create_directories(path);
		logger::info("Created preset folder: {}", path.string());
	}
	
	auto weaponsPath = path / "Weapons";
	if (!std::filesystem::exists(weaponsPath)) {
		std::filesystem::create_directories(weaponsPath);
	}
	
	auto mappingsPath = GetKeywordMappingsFolderPath();
	if (!std::filesystem::exists(mappingsPath)) {
		std::filesystem::create_directories(mappingsPath);
		logger::info("Created keyword mappings folder: {}", mappingsPath.string());
	}
}

std::filesystem::path InertiaPresets::GetKeywordMappingsFolderPath() const
{
	return GetPresetFolderPath() / "WeaponTypeMappings";
}

const WeaponInertiaSettings& InertiaPresets::GetWeaponSettings(const std::string& a_editorID, WeaponType a_type) const
{
	std::shared_lock lock(presetMutex);
	
	// Check for specific weapon preset first
	if (!a_editorID.empty()) {
		SpecificWeaponKey specificKey{ a_editorID };
		auto it = specificWeaponSettings.find(specificKey);
		if (it != specificWeaponSettings.end()) {
			return it->second;
		}
	}
	
	// Fall back to weapon type preset
	WeaponTypeKey typeKey{ a_type };
	auto it = weaponTypeSettings.find(typeKey);
	if (it != weaponTypeSettings.end()) {
		return it->second;
	}
	
	return defaultSettings;
}

const WeaponInertiaSettings& InertiaPresets::GetWeaponSettingsWithKeywords(const std::string& a_editorID, RE::TESObjectWEAP* a_weapon, WeaponType a_type) const
{
	// Priority 1: Specific weapon by EditorID
	if (!a_editorID.empty()) {
		std::shared_lock lock(presetMutex);
		SpecificWeaponKey specificKey{ a_editorID };
		auto it = specificWeaponSettings.find(specificKey);
		if (it != specificWeaponSettings.end()) {
			return it->second;
		}
	}
	
	// Priority 2: Custom keyword-based weapon type
	if (a_weapon && !keywordMappings.empty()) {
		std::string customType = GetBestKeywordMatch(a_weapon);
		if (!customType.empty()) {
			std::shared_lock lock(presetMutex);
			auto it = customWeaponTypeSettings.find(customType);
			if (it != customWeaponTypeSettings.end()) {
				return it->second;
			}
		}
	}
	
	// Priority 3: Standard weapon type preset
	{
		std::shared_lock lock(presetMutex);
		WeaponTypeKey typeKey{ a_type };
		auto it = weaponTypeSettings.find(typeKey);
		if (it != weaponTypeSettings.end()) {
			return it->second;
		}
	}
	
	return defaultSettings;
}

WeaponInertiaSettings& InertiaPresets::GetWeaponSettingsMutable(const std::string& a_editorID, WeaponType a_type)
{
	std::shared_lock lock(presetMutex);
	
	// Check for specific weapon preset first
	if (!a_editorID.empty()) {
		SpecificWeaponKey specificKey{ a_editorID };
		auto it = specificWeaponSettings.find(specificKey);
		if (it != specificWeaponSettings.end()) {
			lock.unlock();
			std::unique_lock writeLock(presetMutex);
			// Note: Don't mark dirty here - let the caller mark dirty when values actually change
			return specificWeaponSettings[specificKey];
		}
	}
	
	// Fall back to weapon type preset
	WeaponTypeKey typeKey{ a_type };
	auto it = weaponTypeSettings.find(typeKey);
	if (it != weaponTypeSettings.end()) {
		lock.unlock();
		std::unique_lock writeLock(presetMutex);
		// Note: Don't mark dirty here - let the caller mark dirty when values actually change
		return weaponTypeSettings[typeKey];
	}
	
	lock.unlock();
	std::unique_lock writeLock(presetMutex);
	// Note: Don't mark dirty here - let the caller mark dirty when values actually change
	return weaponTypeSettings[typeKey];
}

const WeaponInertiaSettings& InertiaPresets::GetWeaponTypeSettings(WeaponType a_type) const
{
	std::shared_lock lock(presetMutex);
	
	WeaponTypeKey key{ a_type };
	auto it = weaponTypeSettings.find(key);
	if (it != weaponTypeSettings.end()) {
		return it->second;
	}
	
	return defaultSettings;
}

WeaponInertiaSettings& InertiaPresets::GetWeaponTypeSettingsMutable(WeaponType a_type)
{
	std::unique_lock lock(presetMutex);
	
	WeaponTypeKey key{ a_type };
	// Note: Don't mark dirty here - let the caller mark dirty when values actually change
	return weaponTypeSettings[key];
}

const WeaponInertiaSettings* InertiaPresets::GetSpecificWeaponSettings(const std::string& a_editorID) const
{
	std::shared_lock lock(presetMutex);
	
	SpecificWeaponKey key{ a_editorID };
	auto it = specificWeaponSettings.find(key);
	if (it != specificWeaponSettings.end()) {
		return &it->second;
	}
	
	return nullptr;
}

WeaponInertiaSettings& InertiaPresets::GetOrCreateSpecificWeaponSettings(const std::string& a_editorID, WeaponType a_baseType)
{
	std::unique_lock lock(presetMutex);
	
	SpecificWeaponKey key{ a_editorID };
	auto it = specificWeaponSettings.find(key);
	if (it != specificWeaponSettings.end()) {
		// Existing preset - don't mark dirty, let caller do it when values change
		return it->second;
	}
	
	// Create new specific weapon settings, copying from weapon type as base
	WeaponTypeKey typeKey{ a_baseType };
	auto typeIt = weaponTypeSettings.find(typeKey);
	if (typeIt != weaponTypeSettings.end()) {
		specificWeaponSettings[key] = typeIt->second;
	} else {
		specificWeaponSettings[key] = WeaponInertiaSettings{};
	}
	
	// Only mark dirty when CREATING a new preset
	isDirty = true;
	return specificWeaponSettings[key];
}

bool InertiaPresets::HasSpecificWeaponSettings(const std::string& a_editorID) const
{
	std::shared_lock lock(presetMutex);
	SpecificWeaponKey key{ a_editorID };
	return specificWeaponSettings.find(key) != specificWeaponSettings.end();
}

void InertiaPresets::RemoveSpecificWeaponSettings(const std::string& a_editorID)
{
	std::unique_lock lock(presetMutex);
	SpecificWeaponKey key{ a_editorID };
	specificWeaponSettings.erase(key);
	isDirty = true;
	
	// Also delete the preset file
	auto path = GetSpecificWeaponPresetPath(a_editorID);
	if (std::filesystem::exists(path)) {
		std::filesystem::remove(path);
		logger::info("Deleted specific weapon preset: {}", path.string());
	}
}

void InertiaPresets::SaveWeaponTypePresets()
{
	EnsurePresetFolderExists();
	
	json j;
	
	std::shared_lock lock(presetMutex);
	
	// Save standard weapon types
	for (const auto& [key, settings] : weaponTypeSettings) {
		const char* typeName = GetWeaponTypeName(key.type);
		j[typeName] = settings;
		j[typeName]["weaponType"] = typeName;
	}
	
	// Save custom keyword-based weapon types
	for (const auto& [typeName, settings] : customWeaponTypeSettings) {
		j[typeName] = settings;
		j[typeName]["weaponType"] = typeName;
		j[typeName]["isCustomType"] = true;  // Mark as custom for identification
	}
	
	lock.unlock();
	
	auto path = GetWeaponTypePresetsPath();
	std::ofstream file(path);
	if (file.is_open()) {
		file << j.dump(4);
		file.close();
		isDirty = false;
		logger::info("Saved weapon type presets to: {}", path.string());
	} else {
		logger::error("Failed to save weapon type presets to: {}", path.string());
	}
}

void InertiaPresets::LoadWeaponTypePresets()
{
	auto path = GetWeaponTypePresetsPath();
	if (!std::filesystem::exists(path)) {
		return;
	}
	
	std::ifstream file(path);
	if (!file.is_open()) {
		logger::warn("Could not open weapon type presets: {}", path.string());
		return;
	}
	
	// Track if preset is missing any new fields (needs re-save to update)
	bool needsResave = false;
	
	try {
		json j;
		file >> j;
		file.close();
		
		std::unique_lock lock(presetMutex);
		
		for (auto& [key, value] : j.items()) {
			// Check for new fields that might be missing from older presets
			// If any are missing, we'll re-save the preset to include them
			if (!value.contains("enabled")) {
				needsResave = true;
				logger::info("  Preset '{}' missing 'enabled' field - will update preset file", key);
			}
			// Check if this is a custom keyword-based type
			bool isCustomType = value.contains("isCustomType") && value["isCustomType"].get<bool>();
			
			// Also check if the key matches a known custom type name from mappings
			if (!isCustomType) {
				isCustomType = IsCustomWeaponType(key);
			}
			
			if (isCustomType) {
				// Only load custom types if they're defined in the current mapping files
				// This ensures orphaned custom types (from removed mapping files) are ignored
				if (!IsCustomWeaponType(key)) {
					logger::info("  Skipping orphaned custom type {} (no mapping file defines it)", key);
					continue;
				}
				
				// Load as custom weapon type
				customWeaponTypeSettings[key] = value.get<WeaponInertiaSettings>();
				logger::info("  Loaded custom type {}: stiffness={:.0f}, damping={:.1f}",
					key, customWeaponTypeSettings[key].stiffness, customWeaponTypeSettings[key].damping);
			} else {
				// Load as standard weapon type
				WeaponType type = ParseWeaponTypeName(key);
				
				// Also check embedded weaponType field
				if (value.contains("weaponType")) {
					type = ParseWeaponTypeName(value["weaponType"].get<std::string>());
				}
				
				// Skip if this looks like a custom type name but isn't in our mappings
				// (e.g., preset has "OneHandRapier" but no mapping file defines it)
				if (type == WeaponType::Unarmed && key != "Unarmed") {
					// ParseWeaponTypeName returns Unarmed as default for unknown types
					// If the key isn't "Unarmed" but parsed to Unarmed, it's an unknown type
					logger::info("  Skipping unknown type {} (not a standard type and no mapping defines it)", key);
					continue;
				}
				
				WeaponTypeKey typeKey{ type };
				weaponTypeSettings[typeKey] = value.get<WeaponInertiaSettings>();
				
				logger::info("  Loaded {}: stiffness={:.0f}, damping={:.1f}",
					key, weaponTypeSettings[typeKey].stiffness, weaponTypeSettings[typeKey].damping);
			}
		}
		
		logger::info("Loaded weapon type presets from: {}", path.string());
		
		// If any new fields were missing, re-save the preset to include them
		if (needsResave) {
			lock.unlock();  // Release lock before saving (Save acquires its own lock)
			logger::info("Re-saving preset to update with new fields...");
			SaveWeaponTypePresets();
		}
	} catch (const std::exception& e) {
		logger::error("Error loading weapon type presets: {}", e.what());
	}
}

void InertiaPresets::SaveSpecificWeaponPreset(const std::string& a_editorID)
{
	EnsurePresetFolderExists();
	
	std::shared_lock lock(presetMutex);
	SpecificWeaponKey key{ a_editorID };
	auto it = specificWeaponSettings.find(key);
	if (it == specificWeaponSettings.end()) {
		logger::warn("No specific weapon settings found for: {}", a_editorID);
		return;
	}
	
	json j = it->second;
	j["editorID"] = a_editorID;
	lock.unlock();
	
	auto path = GetSpecificWeaponPresetPath(a_editorID);
	std::ofstream file(path);
	if (file.is_open()) {
		file << j.dump(4);
		file.close();
		logger::info("Saved specific weapon preset: {}", path.string());
	} else {
		logger::error("Failed to save specific weapon preset: {}", path.string());
	}
}

void InertiaPresets::LoadSpecificWeaponPreset(const std::string& a_editorID)
{
	auto path = GetSpecificWeaponPresetPath(a_editorID);
	if (!std::filesystem::exists(path)) {
		return;
	}
	
	std::ifstream file(path);
	if (!file.is_open()) {
		return;
	}
	
	try {
		json j;
		file >> j;
		file.close();
		
		std::string editorID = a_editorID;
		if (j.contains("editorID")) {
			editorID = j["editorID"].get<std::string>();
		}
		
		std::unique_lock lock(presetMutex);
		SpecificWeaponKey key{ editorID };
		specificWeaponSettings[key] = j.get<WeaponInertiaSettings>();
		
		logger::info("Loaded specific weapon preset: {}", editorID);
	} catch (const std::exception& e) {
		logger::error("Error loading specific weapon preset {}: {}", a_editorID, e.what());
	}
}

void InertiaPresets::LoadAllPresets()
{
	// Load weapon type presets
	LoadWeaponTypePresets();
	
	// Load all specific weapon presets from the Weapons folder
	auto weaponsPath = GetPresetFolderPath() / "Weapons";
	if (!std::filesystem::exists(weaponsPath)) {
		return;
	}
	
	for (const auto& entry : std::filesystem::directory_iterator(weaponsPath)) {
		if (entry.is_regular_file() && entry.path().extension() == ".json") {
			std::string editorID = entry.path().stem().string();
			LoadSpecificWeaponPreset(editorID);
		}
	}
}

std::vector<std::string> InertiaPresets::GetSavedSpecificWeaponPresets() const
{
	std::vector<std::string> presets;
	
	auto weaponsPath = GetPresetFolderPath() / "Weapons";
	if (!std::filesystem::exists(weaponsPath)) {
		return presets;
	}
	
	for (const auto& entry : std::filesystem::directory_iterator(weaponsPath)) {
		if (entry.is_regular_file() && entry.path().extension() == ".json") {
			presets.push_back(entry.path().stem().string());
		}
	}
	
	return presets;
}

std::vector<std::string> InertiaPresets::GetAvailablePresets() const
{
	std::vector<std::string> presets;
	
	auto folderPath = GetPresetFolderPath();
	if (!std::filesystem::exists(folderPath)) {
		return presets;
	}
	
	for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
		if (entry.is_regular_file() && entry.path().extension() == ".json") {
			// Exclude files in subfolders (like Weapons/) - only get direct files
			presets.push_back(entry.path().stem().string());
		}
	}
	
	// Sort alphabetically for consistent ordering
	std::sort(presets.begin(), presets.end());
	
	return presets;
}

void InertiaPresets::SetActivePreset(const std::string& a_name)
{
	if (a_name == activePresetName) {
		return;  // Already active
	}
	
	// Check if preset file exists
	auto path = GetPresetPath(a_name);
	if (!std::filesystem::exists(path)) {
		logger::warn("Preset file does not exist: {}", path.string());
		return;
	}
	
	// Clear current settings
	{
		std::unique_lock lock(presetMutex);
		weaponTypeSettings.clear();
		customWeaponTypeSettings.clear();
	}
	
	// Switch to new preset
	activePresetName = a_name;
	
	// Load the new preset
	LoadWeaponTypePresets();
	
	// If loading failed, initialize with defaults
	if (weaponTypeSettings.empty()) {
		InitializeDefaultSettings();
	}
	
	// Ensure custom types are in the preset
	EnsureCustomTypesInPreset();
	
	// Save the active preset setting to INI
	SaveActivePresetSetting();
	
	isDirty = false;  // We just loaded, so no unsaved changes
	
	// Increment version so InertiaManager knows to refresh its cached settings
	settingsVersion++;
	
	logger::info("Switched to preset: {} (version {})", a_name, settingsVersion);
}

void InertiaPresets::CreateNewPreset(const std::string& a_name)
{
	if (a_name.empty()) {
		logger::warn("Cannot create preset with empty name");
		return;
	}
	
	// Check if preset already exists
	auto path = GetPresetPath(a_name);
	if (std::filesystem::exists(path)) {
		logger::warn("Preset already exists: {}", a_name);
		return;
	}
	
	// Save current settings to the new preset file
	json j;
	{
		std::shared_lock lock(presetMutex);
		for (const auto& [key, settings] : weaponTypeSettings) {
			const char* typeName = GetWeaponTypeName(key.type);
			j[typeName] = settings;
			j[typeName]["weaponType"] = typeName;
		}
	}
	
	std::ofstream file(path);
	if (file.is_open()) {
		file << j.dump(4);
		file.close();
		logger::info("Created new preset: {}", path.string());
	} else {
		logger::error("Failed to create preset: {}", path.string());
		return;
	}
	
	// Switch to the new preset
	activePresetName = a_name;
	SaveActivePresetSetting();
}

void InertiaPresets::DuplicatePreset(const std::string& a_sourceName, const std::string& a_newName)
{
	if (a_newName.empty()) {
		logger::warn("Cannot create preset with empty name");
		return;
	}
	
	auto sourcePath = GetPresetPath(a_sourceName);
	auto destPath = GetPresetPath(a_newName);
	
	if (!std::filesystem::exists(sourcePath)) {
		logger::warn("Source preset does not exist: {}", a_sourceName);
		return;
	}
	
	if (std::filesystem::exists(destPath)) {
		logger::warn("Destination preset already exists: {}", a_newName);
		return;
	}
	
	try {
		std::filesystem::copy_file(sourcePath, destPath);
		logger::info("Duplicated preset {} to {}", a_sourceName, a_newName);
	} catch (const std::exception& e) {
		logger::error("Failed to duplicate preset: {}", e.what());
	}
}

void InertiaPresets::DeletePreset(const std::string& a_name)
{
	// Don't allow deleting the active preset
	if (a_name == activePresetName) {
		logger::warn("Cannot delete the active preset. Switch to a different preset first.");
		return;
	}
	
	auto path = GetPresetPath(a_name);
	if (!std::filesystem::exists(path)) {
		logger::warn("Preset does not exist: {}", a_name);
		return;
	}
	
	try {
		std::filesystem::remove(path);
		logger::info("Deleted preset: {}", a_name);
	} catch (const std::exception& e) {
		logger::error("Failed to delete preset: {}", e.what());
	}
}

void InertiaPresets::RenamePreset(const std::string& a_oldName, const std::string& a_newName)
{
	if (a_newName.empty()) {
		logger::warn("Cannot rename preset to empty name");
		return;
	}
	
	auto oldPath = GetPresetPath(a_oldName);
	auto newPath = GetPresetPath(a_newName);
	
	if (!std::filesystem::exists(oldPath)) {
		logger::warn("Preset does not exist: {}", a_oldName);
		return;
	}
	
	if (std::filesystem::exists(newPath)) {
		logger::warn("Preset already exists: {}", a_newName);
		return;
	}
	
	try {
		std::filesystem::rename(oldPath, newPath);
		
		// If this was the active preset, update the name
		if (activePresetName == a_oldName) {
			activePresetName = a_newName;
			SaveActivePresetSetting();
		}
		
		logger::info("Renamed preset {} to {}", a_oldName, a_newName);
	} catch (const std::exception& e) {
		logger::error("Failed to rename preset: {}", e.what());
	}
}

void InertiaPresets::SaveActivePresetSetting()
{
	CSimpleIniA ini;
	ini.SetUnicode();
	
	auto iniPath = L"Data/SKSE/Plugins/FPInertia.ini";
	ini.LoadFile(iniPath);
	
	ini.SetValue("Presets", "sActivePreset", activePresetName.c_str(), 
		"; The currently active weapon type preset file (without .json extension)");
	
	ini.SaveFile(iniPath);
	logger::info("Saved active preset setting: {}", activePresetName);
}

void InertiaPresets::LoadActivePresetSetting()
{
	CSimpleIniA ini;
	ini.SetUnicode();
	
	auto iniPath = L"Data/SKSE/Plugins/FPInertia.ini";
	if (ini.LoadFile(iniPath) != SI_OK) {
		logger::info("No INI file found, using default preset: {}", activePresetName);
		return;
	}
	
	const char* value = ini.GetValue("Presets", "sActivePreset", "WeaponTypes");
	if (value && value[0] != '\0') {
		activePresetName = value;
		logger::info("Loaded active preset setting: {}", activePresetName);
	}
}

// ============================================================================
// Custom Keyword-Based Weapon Types
// ============================================================================

void InertiaPresets::LoadKeywordMappings()
{
	auto mappingsPath = GetKeywordMappingsFolderPath();
	if (!std::filesystem::exists(mappingsPath)) {
		logger::info("No keyword mappings folder found");
		return;
	}
	
	keywordMappings.clear();
	std::set<std::string> uniqueTypeNames;  // Use set to avoid duplicates
	
	// Scan all .txt files in the mappings folder
	for (const auto& entry : std::filesystem::directory_iterator(mappingsPath)) {
		if (!entry.is_regular_file()) continue;
		
		auto ext = entry.path().extension().string();
		// Accept .txt, .ini, or no extension
		if (ext != ".txt" && ext != ".ini" && !ext.empty()) continue;
		
		std::ifstream file(entry.path());
		if (!file.is_open()) {
			logger::warn("Could not open mapping file: {}", entry.path().string());
			continue;
		}
		
		std::string line;
		int lineNum = 0;
		while (std::getline(file, line)) {
			lineNum++;
			
			// Trim whitespace
			size_t start = line.find_first_not_of(" \t\r\n");
			if (start == std::string::npos) continue;  // Empty line
			line = line.substr(start);
			size_t end = line.find_last_not_of(" \t\r\n");
			if (end != std::string::npos) line = line.substr(0, end + 1);
			
			// Skip comments
			if (line.empty() || line[0] == ';' || line[0] == '#') continue;
			
			// Parse format: Keyword1,Keyword2=WeaponTypeName
			size_t eqPos = line.find('=');
			if (eqPos == std::string::npos) {
				logger::warn("Invalid mapping format at {}:{} - missing '='", entry.path().filename().string(), lineNum);
				continue;
			}
			
			std::string keywordsPart = line.substr(0, eqPos);
			std::string typeName = line.substr(eqPos + 1);
			
			// Trim the type name
			start = typeName.find_first_not_of(" \t");
			if (start != std::string::npos) typeName = typeName.substr(start);
			end = typeName.find_last_not_of(" \t");
			if (end != std::string::npos) typeName = typeName.substr(0, end + 1);
			
			if (typeName.empty()) {
				logger::warn("Invalid mapping format at {}:{} - empty type name", entry.path().filename().string(), lineNum);
				continue;
			}
			
			// Parse keywords (comma-separated)
			KeywordMapping mapping;
			mapping.weaponTypeName = typeName;
			
			std::stringstream ss(keywordsPart);
			std::string keyword;
			while (std::getline(ss, keyword, ',')) {
				// Trim each keyword
				start = keyword.find_first_not_of(" \t");
				if (start != std::string::npos) keyword = keyword.substr(start);
				end = keyword.find_last_not_of(" \t");
				if (end != std::string::npos) keyword = keyword.substr(0, end + 1);
				
				if (!keyword.empty()) {
					mapping.keywords.push_back(keyword);
				}
			}
			
			if (mapping.keywords.empty()) {
				logger::warn("Invalid mapping format at {}:{} - no keywords", entry.path().filename().string(), lineNum);
				continue;
			}
			
			keywordMappings.push_back(mapping);
			uniqueTypeNames.insert(typeName);
			
			logger::info("  Loaded mapping: {} keywords -> {}", mapping.keywords.size(), typeName);
		}
		
		file.close();
		logger::info("Loaded keyword mappings from: {}", entry.path().filename().string());
	}
	
	// Sort mappings by specificity (most keywords first)
	std::sort(keywordMappings.begin(), keywordMappings.end());
	
	// Build the list of custom type names
	customWeaponTypeNames.clear();
	for (const auto& name : uniqueTypeNames) {
		customWeaponTypeNames.push_back(name);
	}
	std::sort(customWeaponTypeNames.begin(), customWeaponTypeNames.end());
	
	// Pre-resolve all keyword EditorIDs to pointers for performance
	// This avoids expensive LookupByEditorID calls every frame
	ResolveKeywordPointers();
	
	logger::info("Loaded {} keyword mappings defining {} custom weapon types", 
		keywordMappings.size(), customWeaponTypeNames.size());
}

void InertiaPresets::ResolveKeywordPointers()
{
	int totalResolved = 0;
	int totalFailed = 0;
	
	for (auto& mapping : keywordMappings) {
		mapping.resolvedKeywords.clear();
		mapping.resolvedKeywords.reserve(mapping.keywords.size());
		mapping.keywordsResolved = true;
		
		for (const auto& keywordName : mapping.keywords) {
			auto* keyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(keywordName);
			if (keyword) {
				mapping.resolvedKeywords.push_back(keyword);
				totalResolved++;
			} else {
				// Keyword not found - mark mapping as unresolved (won't match anything)
				mapping.keywordsResolved = false;
				totalFailed++;
				logger::warn("  Could not resolve keyword '{}' for type '{}' - this custom type will not work",
					keywordName, mapping.weaponTypeName);
			}
		}
	}
	
	if (totalFailed > 0) {
		logger::warn("Resolved {} keywords, {} failed to resolve", totalResolved, totalFailed);
	} else if (totalResolved > 0) {
		logger::info("Pre-resolved {} keyword pointers for custom weapon types", totalResolved);
	}
}

std::string InertiaPresets::GetBestKeywordMatch(RE::TESObjectWEAP* a_weapon) const
{
	if (!a_weapon) return "";
	
	// Get the keyword form interface
	auto* keywordForm = a_weapon->As<RE::BGSKeywordForm>();
	if (!keywordForm) return "";
	
	// Check each mapping (already sorted by specificity - most keywords first)
	// Uses pre-resolved keyword pointers for performance (avoids LookupByEditorID every frame)
	for (const auto& mapping : keywordMappings) {
		// Skip mappings where keywords failed to resolve
		if (!mapping.keywordsResolved || mapping.resolvedKeywords.size() != mapping.keywords.size()) {
			continue;
		}
		
		bool allMatch = true;
		
		// Use pre-resolved keyword pointers
		for (const auto* keyword : mapping.resolvedKeywords) {
			if (!keyword || !keywordForm->HasKeyword(keyword)) {
				allMatch = false;
				break;
			}
		}
		
		if (allMatch) {
			// Found a match! Return immediately since we're sorted by specificity
			return mapping.weaponTypeName;
		}
	}
	
	return "";  // No match found
}

bool InertiaPresets::IsCustomWeaponType(const std::string& a_name) const
{
	for (const auto& typeName : customWeaponTypeNames) {
		if (typeName == a_name) return true;
	}
	return false;
}

void InertiaPresets::EnsureCustomTypesInPreset()
{
	if (customWeaponTypeNames.empty()) return;
	
	std::unique_lock lock(presetMutex);
	
	bool addedAny = false;
	for (const auto& typeName : customWeaponTypeNames) {
		if (customWeaponTypeSettings.find(typeName) == customWeaponTypeSettings.end()) {
			// Add with default settings
			customWeaponTypeSettings[typeName] = WeaponInertiaSettings{};
			addedAny = true;
			logger::info("Added default settings for custom weapon type: {}", typeName);
		}
	}
	
	// If we added any, save the preset
	if (addedAny) {
		lock.unlock();
		SaveWeaponTypePresets();
	}
}

const WeaponInertiaSettings* InertiaPresets::GetCustomWeaponTypeSettings(const std::string& a_customTypeName) const
{
	std::shared_lock lock(presetMutex);
	
	auto it = customWeaponTypeSettings.find(a_customTypeName);
	if (it != customWeaponTypeSettings.end()) {
		return &it->second;
	}
	
	return nullptr;
}

WeaponInertiaSettings& InertiaPresets::GetCustomWeaponTypeSettingsMutable(const std::string& a_customTypeName)
{
	std::unique_lock lock(presetMutex);
	
	// Create if doesn't exist
	if (customWeaponTypeSettings.find(a_customTypeName) == customWeaponTypeSettings.end()) {
		customWeaponTypeSettings[a_customTypeName] = WeaponInertiaSettings{};
	}
	
	return customWeaponTypeSettings[a_customTypeName];
}

