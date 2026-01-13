#include "Inertia.h"
#include "Settings.h"

namespace Inertia
{
	// Forward declaration for WeaponEnchantmentController (from EnchantmentArtExtender)
	class WeaponEnchantmentController : public RE::ReferenceEffectController
	{
	public:
		RE::ActorMagicCaster* magicCaster;   // 08
		RE::Actor* target;                   // 10
		RE::TESEffectShader* effectShader;  // 18
		RE::BGSArtObject* artObject;        // 20
		RE::NiPointer<RE::NiAVObject> attachRoot; // 28
		RE::TESObjectWEAP* weapon;          // 30
		bool firstPerson;                   // 38
	};

	static_assert(sizeof(WeaponEnchantmentController) == 0x40);

	// Helper function to get stance name
	const char* GetStanceName(Stance a_stance)
	{
		switch (a_stance) {
		case Stance::Neutral: return "Neutral";
		case Stance::Low:     return "Low";
		case Stance::Mid:     return "Mid";
		case Stance::High:    return "High";
		default:              return "Unknown";
		}
	}
	namespace
	{
		// Constants
		constexpr float PI = 3.14159265358979323846f;
		constexpr float DEG_TO_RAD = PI / 180.0f;
		constexpr float RAD_TO_DEG = 180.0f / PI;
		
		// Flag to track if we've logged the skeleton hierarchy yet
		bool hasLoggedSkeleton = false;
		
		// Recursively log the entire node hierarchy with indentation
		void LogNodeHierarchy(RE::NiAVObject* obj, int depth = 0)
		{
			if (!obj) return;
			
			std::string indent(depth * 2, ' ');
			std::string typeName = "NiAVObject";
			
			// Determine node type
			if (obj->AsNode()) {
				typeName = "NiNode";
			} else if (obj->AsGeometry()) {
				typeName = "NiGeometry";
			}
			
			// Log this node
			logger::info("{}[{}] {}", indent, typeName, obj->name.c_str());
			
			// If it's a node, recurse into children
			auto* node = obj->AsNode();
			if (node) {
				auto& children = node->GetChildren();
				for (std::uint16_t i = 0; i < children.capacity(); ++i) {
					auto& child = children[i];
					if (child) {
						LogNodeHierarchy(child.get(), depth + 1);
					}
				}
			}
		}
		
		// Log the full first person skeleton hierarchy
		void LogFirstPersonSkeleton()
		{
			if (hasLoggedSkeleton) return;
			
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) return;
			
			auto* firstPerson3D = player->Get3D(1);
			if (!firstPerson3D) return;
			
			logger::info("========== FIRST PERSON SKELETON HIERARCHY ==========");
			LogNodeHierarchy(firstPerson3D);
			logger::info("======================================================");
			
			hasLoggedSkeleton = true;
		}
		
		// Clamp a vector component-wise
		RE::NiPoint3 ClampVector(const RE::NiPoint3& a_vec, float a_max)
		{
			return {
				std::clamp(a_vec.x, -a_max, a_max),
				std::clamp(a_vec.y, -a_max, a_max),
				std::clamp(a_vec.z, -a_max, a_max)
			};
		}
		
		// Linear interpolation for vectors
		RE::NiPoint3 LerpVector(const RE::NiPoint3& a_from, const RE::NiPoint3& a_to, float a_t)
		{
			return {
				a_from.x + (a_to.x - a_from.x) * a_t,
				a_from.y + (a_to.y - a_from.y) * a_t,
				a_from.z + (a_to.z - a_from.z) * a_t
			};
		}
		
		// Create rotation matrix from euler angles (XYZ order)
		RE::NiMatrix3 EulerToMatrix(const RE::NiPoint3& a_euler)
		{
			float cx = std::cos(a_euler.x);
			float sx = std::sin(a_euler.x);
			float cy = std::cos(a_euler.y);
			float sy = std::sin(a_euler.y);
			float cz = std::cos(a_euler.z);
			float sz = std::sin(a_euler.z);
			
			RE::NiMatrix3 result;
			result.entry[0][0] = cy * cz;
			result.entry[0][1] = -cy * sz;
			result.entry[0][2] = sy;
			result.entry[1][0] = sx * sy * cz + cx * sz;
			result.entry[1][1] = -sx * sy * sz + cx * cz;
			result.entry[1][2] = -sx * cy;
			result.entry[2][0] = -cx * sy * cz + sx * sz;
			result.entry[2][1] = cx * sy * sz + sx * cz;
			result.entry[2][2] = cx * cy;
			
			return result;
		}
	}

	RE::NiNode* InertiaManager::GetFirstPersonNode()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return nullptr;
		}
		
		// Get the first person 3D skeleton (parameter 1 = first person, 0 = third person)
		auto* firstPerson3D = player->Get3D(1);
		if (!firstPerson3D) {
			auto* settings = Settings::GetSingleton();
			if (settings->debugLogging) {
				logger::warn("[FPInertia] Could not get first person 3D model");
			}
			return nullptr;
		}
		
		return firstPerson3D->AsNode();
	}
	
	// Helper function to find a node by name in the skeleton hierarchy
	RE::NiNode* FindNode(RE::NiNode* a_root, const char* a_name)
	{
		if (!a_root || !a_name) {
			return nullptr;
		}
		
		auto* object = a_root->GetObjectByName(a_name);
		if (object) {
			return object->AsNode();
		}
		
		return nullptr;
	}
	
	// Check if a weapon type is two-handed
	bool IsTwoHandedWeapon(RE::WEAPON_TYPE a_type)
	{
		switch (a_type) {
		case RE::WEAPON_TYPE::kTwoHandSword:
		case RE::WEAPON_TYPE::kTwoHandAxe:
		case RE::WEAPON_TYPE::kBow:
		case RE::WEAPON_TYPE::kCrossbow:
			return true;
		default:
			return false;
		}
	}

	RE::WEAPON_TYPE InertiaManager::GetWeaponTypeForHand(RE::PlayerCharacter* a_player, Hand a_hand)
	{
		if (!a_player) {
			return RE::WEAPON_TYPE::kHandToHandMelee;
		}
		
		bool isLeftHand = (a_hand == Hand::kLeft);
		auto* equippedObject = a_player->GetEquippedObject(isLeftHand);
		
		if (!equippedObject) {
			return RE::WEAPON_TYPE::kHandToHandMelee;
		}
		
		// Check if it's a weapon
		if (equippedObject->IsWeapon()) {
			auto* weapon = equippedObject->As<RE::TESObjectWEAP>();
			if (weapon) {
				return weapon->GetWeaponType();
			}
		}
		
		return RE::WEAPON_TYPE::kHandToHandMelee;
	}

	bool InertiaManager::HasShield(RE::PlayerCharacter* a_player, Hand a_hand)
	{
		if (!a_player || a_hand != Hand::kLeft) {
			return false;
		}
		
		auto* equippedObject = a_player->GetEquippedObject(true);  // Left hand
		if (!equippedObject) {
			return false;
		}
		
		if (equippedObject->IsArmor()) {
			auto* armor = equippedObject->As<RE::TESObjectARMO>();
			if (armor && armor->IsShield()) {
				return true;
			}
		}
		
		return false;
	}
	
	bool InertiaManager::HasBoundWeapon(RE::PlayerCharacter* a_player, Hand a_hand)
	{
		if (!a_player) {
			return false;
		}
		
		bool isLeftHand = (a_hand == Hand::kLeft);
		auto* equippedObject = a_player->GetEquippedObject(isLeftHand);
		
		if (!equippedObject || !equippedObject->IsWeapon()) {
			return false;
		}
		
		// Check for bound weapon keyword
		auto* weapon = equippedObject->As<RE::TESObjectWEAP>();
		if (!weapon) {
			return false;
		}
		
		// Look for WeapTypeBound keyword
		static auto* boundKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("WeapTypeBound");
		if (boundKeyword) {
			auto* keywordForm = weapon->As<RE::BGSKeywordForm>();
			if (keywordForm && keywordForm->HasKeyword(boundKeyword)) {
				return true;
			}
		}
		
		return false;
	}
	
	bool InertiaManager::HasSpell(RE::PlayerCharacter* a_player, Hand a_hand)
	{
		if (!a_player) {
			return false;
		}
		
		bool isLeftHand = (a_hand == Hand::kLeft);
		auto* equippedObject = a_player->GetEquippedObject(isLeftHand);
		
		// If nothing equipped, check for actual magic in that hand
		if (!equippedObject) {
			// Check if player has an actual spell in this hand via the magic caster
			// GetEquippedObject returns nullptr for spells, but we can check IsWeaponDrawn + casting state
			// For now, return false - empty hand is not a "spell"
			return false;
		}
		
		// Check if it's a spell/magic item (not weapon and not armor)
		// Spells in Skyrim are typically SpellItem forms
		if (!equippedObject->IsWeapon() && !equippedObject->IsArmor()) {
			// This is likely a spell or other magic item
			return true;
		}
		
		return false;
	}
	
	bool InertiaManager::HasWeapon(RE::PlayerCharacter* a_player, Hand a_hand)
	{
		if (!a_player) {
			return false;
		}
		
		bool isLeftHand = (a_hand == Hand::kLeft);
		auto* equippedObject = a_player->GetEquippedObject(isLeftHand);
		
		if (!equippedObject) {
			return false;
		}
		
		// Check if it's a weapon (includes bound weapons)
		return equippedObject->IsWeapon();
	}
	
	WeaponType InertiaManager::DetectDualWieldType(RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return WeaponType::Unarmed;
		}
		
		// Get what's equipped in each hand
		auto* rightEquip = a_player->GetEquippedObject(false);  // false = right hand
		auto* leftEquip = a_player->GetEquippedObject(true);    // true = left hand
		
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
		
		// Categorize each hand (excludes unarmed from "has weapon")
		bool rightHasWeapon = isActualWeapon(rightEquip);
		bool leftHasWeapon = isActualWeapon(leftEquip);
		bool leftHasShield = HasShield(a_player, Hand::kLeft);
		
		// Check for spells (non-weapon, non-armor equipped items)
		bool rightHasSpell = rightEquip && !rightEquip->IsWeapon() && !rightEquip->IsArmor();
		bool leftHasSpell = leftEquip && !leftEquip->IsWeapon() && !leftEquip->IsArmor();
		
		// Check for two-handed weapons - not dual wield
		if (rightHasWeapon) {
			RE::WEAPON_TYPE rightWeaponType = GetWeaponTypeForHand(a_player, Hand::kRight);
			if (IsTwoHandedWeapon(rightWeaponType)) {
				return WeaponType::Unarmed;  // Not dual wield (two-handed weapon)
			}
		}
		
		// Dual Wield Weapons: weapon in both hands (includes bound weapons, staffs)
		if (rightHasWeapon && leftHasWeapon) {
			return WeaponType::DualWieldWeapons;
		}
		
		// Spell + Weapon: spell in one hand, weapon in the other (not shield)
		if ((rightHasWeapon && leftHasSpell) || (rightHasSpell && leftHasWeapon)) {
			return WeaponType::SpellAndWeapon;
		}
		
		// Dual Wield Magic: actual spells in both hands (not just empty hands)
		// Both hands must have an actual spell equipped
		if (rightHasSpell && leftHasSpell) {
			return WeaponType::DualWieldMagic;
		}
		
		// Not dual wielding (includes: unarmed, weapon+shield, weapon+nothing, just shield, etc.)
		return WeaponType::Unarmed;
	}
	
	RE::NiNode* InertiaManager::GetClavicleNode(RE::NiNode* a_fpRoot, Hand a_hand)
	{
		if (!a_fpRoot) {
			return nullptr;
		}
		
		// Try to find the appropriate clavicle node for dual clavicle pivots (4 and 5)
		// Using clavicles instead of hands for a more natural shoulder-based inertia effect
		const char* clavicleNodeName = (a_hand == Hand::kRight) 
			? "NPC R Clavicle [RClv]" 
			: "NPC L Clavicle [LClv]";
		
		RE::NiNode* clavicleNode = FindNode(a_fpRoot, clavicleNodeName);
		
		// Fallback to alternative names
		if (!clavicleNode) {
			clavicleNodeName = (a_hand == Hand::kRight) ? "RClavicle" : "LClavicle";
			clavicleNode = FindNode(a_fpRoot, clavicleNodeName);
		}
		
		return clavicleNode;
	}

	std::string InertiaManager::GetEquippedWeaponEditorID(RE::PlayerCharacter* a_player, Hand a_hand)
	{
		if (!a_player) {
			return "";
		}
		
		bool isLeftHand = (a_hand == Hand::kLeft);
		auto* equippedObject = a_player->GetEquippedObject(isLeftHand);
		
		if (!equippedObject) {
			return "";
		}
		
		// Get the EditorID of the equipped object
		// With Native EditorID Fix installed (required), this works for ALL forms including vanilla weapons
		const char* editorID = equippedObject->GetFormEditorID();
		if (editorID && editorID[0] != '\0') {
			return std::string(editorID);
		}
		
		// Fallback to FormID as hex string (shouldn't happen with Native EditorID Fix, but kept as safety)
		RE::FormID formID = equippedObject->GetFormID();
		logger::warn("[FPInertia] EditorID not available for form {:08X} - is Native EditorID Fix installed?", formID);
		return std::format("0x{:08X}", formID);
	}

	const WeaponInertiaSettings& InertiaManager::GetCurrentWeaponSettings(RE::PlayerCharacter* a_player, Hand a_hand)
	{
		// Check if equipped weapon has changed by comparing FormID
		// This avoids expensive EditorID lookups and keyword checks every frame
		bool isLeftHand = (a_hand == Hand::kLeft);
		auto* equippedObject = a_player->GetEquippedObject(isLeftHand);
		auto* equippedObjectOther = a_player->GetEquippedObject(!isLeftHand);
		
		// Get current FormIDs for both hands (for dual wield detection)
		RE::FormID currentFormID = equippedObject ? equippedObject->GetFormID() : 0;
		RE::FormID otherFormID = equippedObjectOther ? equippedObjectOther->GetFormID() : 0;
		
		// Combine FormIDs for caching (detects any equipment change)
		RE::FormID combinedFormID = currentFormID ^ (otherFormID << 16);
		
		// Check for shield in left hand
		bool isShield = false;
		if (a_hand == Hand::kLeft && equippedObject && equippedObject->IsArmor()) {
			auto* armor = equippedObject->As<RE::TESObjectARMO>();
			isShield = (armor && armor->IsShield());
		}
		
		auto* presets = InertiaPresets::GetSingleton();
		
		// Check if presets have changed (user switched preset)
		uint32_t currentVersion = presets->GetSettingsVersion();
		
		// Use cached settings if weapon hasn't changed AND presets haven't changed
		if (cachedWeaponSettings && combinedFormID == cachedWeaponFormID && 
			cachedSettingsVersion == currentVersion &&
			(currentWeaponType == WeaponType::Shield) == isShield) {
			return *cachedWeaponSettings;
		}
		
		// Weapon or presets changed - refresh cached settings
		cachedWeaponFormID = combinedFormID;
		cachedSettingsVersion = currentVersion;
		
		// PRIORITY 1: Check for dual wield types first
		WeaponType dualWieldType = DetectDualWieldType(a_player);
		if (dualWieldType != WeaponType::Unarmed) {
			// It's a dual wield scenario
			WeaponType prevType = currentWeaponType;
			currentWeaponType = dualWieldType;
			currentDualWieldType = dualWieldType;
			isDualWieldMode = true;
			
			// Log dual wield detection
			auto* settings = Settings::GetSingleton();
			if (settings->debugLogging && prevType != dualWieldType) {
				const char* dualTypeName = (dualWieldType == WeaponType::DualWieldWeapons) ? "DualWieldWeapons" :
				                           (dualWieldType == WeaponType::DualWieldMagic) ? "DualWieldMagic" :
				                           (dualWieldType == WeaponType::SpellAndWeapon) ? "SpellAndWeapon" : "Unknown";
				logger::info("[FPInertia] Detected DUAL WIELD type: {}", dualTypeName);
			}
			
			// For dual wield, we don't use specific weapon presets - just the type
			cachedWeaponSettings = &presets->GetWeaponTypeSettings(dualWieldType);
			return *cachedWeaponSettings;
		}
		
		// Not dual wielding
		isDualWieldMode = false;
		currentDualWieldType = WeaponType::Unarmed;
		
		// PRIORITY 2: Standard weapon type detection
		RE::WEAPON_TYPE reWeaponType = GetWeaponTypeForHand(a_player, a_hand);
		WeaponType weaponType = Settings::ToWeaponType(reWeaponType);
		
		// Override to shield if applicable
		if (isShield) {
			weaponType = WeaponType::Shield;
		}
		
		// Get EditorID (only on weapon change, not every frame)
		std::string editorID = GetEquippedWeaponEditorID(a_player, a_hand);
		
		// Update cached weapon info
		currentWeaponEditorID = editorID;
		currentWeaponType = weaponType;
		
		// Get the weapon for keyword checking
		RE::TESObjectWEAP* weapon = nullptr;
		if (equippedObject && equippedObject->IsWeapon()) {
			weapon = equippedObject->As<RE::TESObjectWEAP>();
		}
		
		// Use preset system with keyword support
		// Priority: specific weapon -> keyword-based type -> standard type
		cachedWeaponSettings = &presets->GetWeaponSettingsWithKeywords(editorID, weapon, weaponType);
		
		// Log weapon type detection
		auto* settings = Settings::GetSingleton();
		if (settings->debugLogging) {
			const char* typeNames[] = { "Unarmed", "OneHandSword", "OneHandDagger", "OneHandAxe", "OneHandMace",
			                            "TwoHandSword", "TwoHandAxe", "Bow", "Staff", "Crossbow", "Shield", "Spell",
			                            "DualWieldWeapons", "DualWieldMagic", "SpellAndWeapon" };
			int typeIdx = static_cast<int>(weaponType);
			const char* typeName = (typeIdx >= 0 && typeIdx < 15) ? typeNames[typeIdx] : "Unknown";
			logger::info("[FPInertia] Weapon type: {} | EditorID: {}", typeName, editorID.empty() ? "(none)" : editorID);
		}
		
		return *cachedWeaponSettings;
	}

	bool InertiaManager::IsPlayerInAction(RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return false;
		}
		
		auto* settings = Settings::GetSingleton();
		
		// Check attack state
		if (settings->blendDuringAttack) {
			auto attackState = a_player->AsActorState()->GetAttackState();
			if (attackState != RE::ATTACK_STATE_ENUM::kNone) {
				return true;
			}
		}
		
		// Check bow draw state
		if (settings->blendDuringBowDraw) {
			auto attackState = a_player->AsActorState()->GetAttackState();
			// Check if in any bow-related attack state
			if (attackState == RE::ATTACK_STATE_ENUM::kBowDraw ||
				attackState == RE::ATTACK_STATE_ENUM::kBowAttached ||
				attackState == RE::ATTACK_STATE_ENUM::kBowDrawn ||
				attackState == RE::ATTACK_STATE_ENUM::kBowReleasing ||
				attackState == RE::ATTACK_STATE_ENUM::kBowReleased ||
				attackState == RE::ATTACK_STATE_ENUM::kBowNextAttack ||
				attackState == RE::ATTACK_STATE_ENUM::kBowFollowThrough) {
				return true;
			}
		}
		
		// Check spell casting
		if (settings->blendDuringSpellCast) {
			// Check both hands for spell casting
			if (a_player->IsCasting(nullptr)) {
				return true;
			}
		}
		
		return false;
	}

	RE::NiPoint3 InertiaManager::CalculateCameraVelocity(float a_delta)
	{
		auto* camera = RE::PlayerCamera::GetSingleton();
		if (!camera || a_delta <= 0.0f) {
			return { 0.0f, 0.0f, 0.0f };
		}
		
		// Get current camera state
		auto* firstPersonState = camera->currentState.get();
		if (!firstPersonState) {
			return { 0.0f, 0.0f, 0.0f };
		}
		
		// Get camera rotation from player angles (more reliable)
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return { 0.0f, 0.0f, 0.0f };
		}
		
		float currentYaw = player->GetAngleZ();
		float currentPitch = player->GetAngleX();
		float currentRoll = 0.0f;  // Roll is typically 0 in Skyrim
		
		// Calculate angular velocity (radians per second)
		RE::NiPoint3 velocity;
		
		// Handle yaw wraparound (0 to 2*PI)
		float yawDiff = currentYaw - lastCameraYaw;
		if (yawDiff > PI) {
			yawDiff -= 2.0f * PI;
		} else if (yawDiff < -PI) {
			yawDiff += 2.0f * PI;
		}
		
		velocity.z = yawDiff / a_delta;  // Yaw velocity
		velocity.x = (currentPitch - lastCameraPitch) / a_delta;  // Pitch velocity
		velocity.y = (currentRoll - lastCameraRoll) / a_delta;  // Roll velocity
		
		// Store current values for next frame
		lastCameraYaw = currentYaw;
		lastCameraPitch = currentPitch;
		lastCameraRoll = currentRoll;
		
		// === FRAMERATE INDEPENDENCE FIX ===
		// Clamp maximum velocity to prevent extreme spikes from frame drops or sudden input
		constexpr float MAX_ANGULAR_VELOCITY = 25.0f;  // radians/sec - very fast turn
		velocity.x = std::clamp(velocity.x, -MAX_ANGULAR_VELOCITY, MAX_ANGULAR_VELOCITY);
		velocity.y = std::clamp(velocity.y, -MAX_ANGULAR_VELOCITY, MAX_ANGULAR_VELOCITY);
		velocity.z = std::clamp(velocity.z, -MAX_ANGULAR_VELOCITY, MAX_ANGULAR_VELOCITY);
		
		// Clamp velocity CHANGE per frame to prevent sudden jumps
		constexpr float MAX_VELOCITY_CHANGE_PER_SEC = 50.0f;  // radians/sec^2
		float maxChange = MAX_VELOCITY_CHANGE_PER_SEC * a_delta;
		
		RE::NiPoint3 velocityDelta = {
			velocity.x - prevCameraVelocity.x,
			velocity.y - prevCameraVelocity.y,
			velocity.z - prevCameraVelocity.z
		};
		
		velocityDelta.x = std::clamp(velocityDelta.x, -maxChange, maxChange);
		velocityDelta.y = std::clamp(velocityDelta.y, -maxChange, maxChange);
		velocityDelta.z = std::clamp(velocityDelta.z, -maxChange, maxChange);
		
		velocity.x = prevCameraVelocity.x + velocityDelta.x;
		velocity.y = prevCameraVelocity.y + velocityDelta.y;
		velocity.z = prevCameraVelocity.z + velocityDelta.z;
		
		prevCameraVelocity = velocity;
		
		return velocity;
	}

	// Update camera-based spring (responds to camera rotation)
	void InertiaManager::UpdateSpring(SpringState& a_state, const WeaponInertiaSettings& a_settings,
		const RE::NiPoint3& a_cameraVelocity, float a_delta, float a_multiplier,
		bool a_stanceInvertCamera)
	{
		auto* settings = Settings::GetSingleton();
		float intensity = settings->globalIntensity * a_multiplier;

		if (intensity <= 0.0f) {
			a_state.Reset();
			return;
		}
		
		// Separate invert factors for pitch (up/down) and yaw (left/right)
		// XOR with stance-specific override: stance invert flips the base setting
		bool effectiveInvertPitch = a_settings.invertCameraPitch ^ a_stanceInvertCamera;
		bool effectiveInvertYaw = a_settings.invertCameraYaw ^ a_stanceInvertCamera;
		float invertPitch = effectiveInvertPitch ? -1.0f : 1.0f;
		float invertYaw = effectiveInvertYaw ? -1.0f : 1.0f;
		
		// Spring-damper system for CAMERA inertia only
		float k = a_settings.stiffness;
		float m = a_settings.mass;
		
		// Apply settling - increase damping when camera stops
		float dampingMultiplier = 1.0f + (settlingFactor * (settings->settleDampingMult - 1.0f));
		float c = a_settings.damping * dampingMultiplier;
		
		// Global pitch multiplier affects ALL pitch effects (position + rotation)
		float pitchMult = a_settings.cameraPitchMult;
		
		// === FRAMERATE INDEPENDENCE: Use sub-stepping for large deltas ===
		constexpr float MAX_SUBSTEP = 0.016f;  // ~60fps equivalent
		int numSteps = static_cast<int>(std::ceil(a_delta / MAX_SUBSTEP));
		numSteps = std::clamp(numSteps, 1, 4);  // Max 4 substeps
		float stepDelta = a_delta / static_cast<float>(numSteps);
		
		// Maximum velocity and position change per substep for stability
		constexpr float MAX_POS_VELOCITY = 500.0f;   // units/sec
		constexpr float MAX_ROT_VELOCITY = 50.0f;    // radians/sec
		
		for (int step = 0; step < numSteps; ++step) {
			// Position offset - CAMERA ONLY
			if (settings->enablePosition) {
				RE::NiPoint3 targetOffset = {
					invertYaw * -a_cameraVelocity.z * 2.0f * intensity,  // Yaw -> X offset
					0.0f,
					invertPitch * a_cameraVelocity.x * 1.5f * intensity * pitchMult  // Pitch -> Z offset (global pitch mult)
				};
				
				targetOffset = ClampVector(targetOffset, a_settings.maxOffset * 2.0f);
				
				RE::NiPoint3 springForce = {
					-k * a_state.positionOffset.x - c * a_state.positionVelocity.x + targetOffset.x * k * 0.5f,
					-k * a_state.positionOffset.y - c * a_state.positionVelocity.y,
					-k * a_state.positionOffset.z - c * a_state.positionVelocity.z + targetOffset.z * k * 0.5f
				};
				
				RE::NiPoint3 acceleration = { springForce.x / m, springForce.y / m, springForce.z / m };
				
				a_state.positionVelocity.x += acceleration.x * stepDelta;
				a_state.positionVelocity.y += acceleration.y * stepDelta;
				a_state.positionVelocity.z += acceleration.z * stepDelta;
				
				// Clamp velocity for stability
				a_state.positionVelocity = ClampVector(a_state.positionVelocity, MAX_POS_VELOCITY);
				
				a_state.positionOffset.x += a_state.positionVelocity.x * stepDelta;
				a_state.positionOffset.y += a_state.positionVelocity.y * stepDelta;
				a_state.positionOffset.z += a_state.positionVelocity.z * stepDelta;
				
				a_state.positionOffset = ClampVector(a_state.positionOffset, a_settings.maxOffset);
			}
			
			// Rotation offset - CAMERA ONLY
			if (settings->enableRotation) {
				float maxRotRad = a_settings.maxRotation * DEG_TO_RAD;
				
				RE::NiPoint3 targetRotation = {
					invertPitch * a_cameraVelocity.x * 0.08f * intensity * a_settings.pitchMultiplier * pitchMult,  // Pitch rotation (both mults)
					invertYaw * -a_cameraVelocity.z * 0.06f * intensity,  // Yaw rotation
					invertYaw * -a_cameraVelocity.z * 0.04f * intensity * a_settings.rollMultiplier  // Roll (driven by yaw)
				};
				
				targetRotation = ClampVector(targetRotation, maxRotRad * 2.0f);
				
				RE::NiPoint3 rotSpringForce = {
					-k * a_state.rotationOffset.x - c * a_state.rotationVelocity.x + targetRotation.x * k * 0.5f,
					-k * a_state.rotationOffset.y - c * a_state.rotationVelocity.y + targetRotation.y * k * 0.5f,
					-k * a_state.rotationOffset.z - c * a_state.rotationVelocity.z + targetRotation.z * k * 0.5f
				};
				
				RE::NiPoint3 rotAcceleration = { rotSpringForce.x / m, rotSpringForce.y / m, rotSpringForce.z / m };
				
				a_state.rotationVelocity.x += rotAcceleration.x * stepDelta;
				a_state.rotationVelocity.y += rotAcceleration.y * stepDelta;
				a_state.rotationVelocity.z += rotAcceleration.z * stepDelta;
				
				// Clamp velocity for stability
				a_state.rotationVelocity = ClampVector(a_state.rotationVelocity, MAX_ROT_VELOCITY);
				
				a_state.rotationOffset.x += a_state.rotationVelocity.x * stepDelta;
				a_state.rotationOffset.y += a_state.rotationVelocity.y * stepDelta;
				a_state.rotationOffset.z += a_state.rotationVelocity.z * stepDelta;
				
				a_state.rotationOffset = ClampVector(a_state.rotationOffset, maxRotRad);
			}
		}
	}
	
	// Update movement-based spring (responds to player strafing and forward/back movement)
	// Uses TARGET-BASED spring: spring moves towards target position, not just returning to 0
	// Uses PER-WEAPON settings for spring parameters
	// Optional stance invert override: when true, XOR with base invert settings
	void UpdateMovementSpring(SpringState& a_state, Settings* settings, 
		const WeaponInertiaSettings& a_weaponSettings,
		const RE::NiPoint3& a_localMovement, float a_delta, float a_intensity,
		bool a_stanceInvertMovement = false)
	{
		// Check per-weapon enable AND global enable
		if (!settings->movementInertiaEnabled || !a_weaponSettings.movementInertiaEnabled || a_intensity <= 0.0f) {
			// Decay to zero when disabled
			float decayRate = std::min(1.0f, a_delta * 5.0f);
			a_state.positionOffset = LerpVector(a_state.positionOffset, {0,0,0}, decayRate);
			a_state.rotationOffset = LerpVector(a_state.rotationOffset, {0,0,0}, decayRate);
			a_state.positionVelocity = LerpVector(a_state.positionVelocity, {0,0,0}, decayRate);
			a_state.rotationVelocity = LerpVector(a_state.rotationVelocity, {0,0,0}, decayRate);
			return;
		}
		
		// Spring parameters from PER-WEAPON settings
		float k = a_weaponSettings.movementStiffness;
		float c = a_weaponSettings.movementDamping;
		float m = 1.0f;
		
		// Separate invert factors for lateral (left/right) and forward/back movement
		// XOR with stance-specific override: stance invert flips the base setting
		bool effectiveInvertLateral = a_weaponSettings.invertMovementLateral ^ a_stanceInvertMovement;
		bool effectiveInvertForwardBack = a_weaponSettings.invertMovementForwardBack ^ a_stanceInvertMovement;
		float invertLateral = effectiveInvertLateral ? -1.0f : 1.0f;
		float invertForwardBack = effectiveInvertForwardBack ? -1.0f : 1.0f;
		
		// Calculate TARGET position based on movement
		RE::NiPoint3 targetPos = { 0.0f, 0.0f, 0.0f };
		RE::NiPoint3 targetRot = { 0.0f, 0.0f, 0.0f };
		
		float lateralSpeed = std::abs(a_localMovement.x);
		float forwardSpeed = std::abs(a_localMovement.y);
		float totalHorizontalSpeed = lateralSpeed + forwardSpeed;
		
		// === LATERAL (LEFT/RIGHT) MOVEMENT ===
		if (lateralSpeed > settings->movementInertiaThreshold) {
			float strength = settings->movementInertiaStrength * a_intensity;
			float speedFactor = std::min(1.0f, lateralSpeed / 200.0f);
			
			// Calculate blend factor - reduce effect when moving diagonally
			float lateralBlend = (totalHorizontalSpeed > 0.1f) ? (lateralSpeed / totalHorizontalSpeed) : 0.0f;
			float blendedStrength = strength * lateralBlend * speedFactor * 1.5f;
			
			float normalizedInput = std::clamp(a_localMovement.x / 200.0f, -1.0f, 1.0f);
			
			// Blend multipliers based on direction
			float rightBlend = std::max(0.0f, normalizedInput);
			float leftBlend = std::max(0.0f, -normalizedInput);
			float effectiveMult = rightBlend * a_weaponSettings.movementRightMult + 
			                      leftBlend * a_weaponSettings.movementLeftMult;
			
			targetPos.x = invertLateral * -normalizedInput * effectiveMult * blendedStrength;
			targetRot.z = invertLateral * -normalizedInput * effectiveMult * blendedStrength * 0.033f;
		}
		
		// === FORWARD/BACK MOVEMENT ===
		// Uses per-weapon forward/back multipliers (only if global forward/back is enabled)
		if (settings->forwardBackInertia && forwardSpeed > settings->movementInertiaThreshold) {
			float strength = settings->movementInertiaStrength * a_intensity;
			float speedFactor = std::min(1.0f, forwardSpeed / 200.0f);
			
			// Calculate blend factor - reduce effect when moving diagonally
			float forwardBlend = (totalHorizontalSpeed > 0.1f) ? (forwardSpeed / totalHorizontalSpeed) : 0.0f;
			float blendedStrength = strength * forwardBlend * speedFactor;
			
			float normalizedInput = std::clamp(a_localMovement.y / 200.0f, -1.0f, 1.0f);
			
			// Use per-weapon forward/back multipliers
			float forwardBlendMult = std::max(0.0f, normalizedInput);
			float backwardBlendMult = std::max(0.0f, -normalizedInput);
			float effectiveMult = forwardBlendMult * a_weaponSettings.movementForwardMult + 
			                      backwardBlendMult * a_weaponSettings.movementBackwardMult;
			
			// Forward -> arms move toward camera (negative Y in local space)
			// Backward -> arms move away from camera (positive Y in local space)
			targetPos.y = invertForwardBack * -normalizedInput * effectiveMult * blendedStrength;
			// Slight Z adjustment for natural feel
			targetPos.z += invertForwardBack * -normalizedInput * effectiveMult * blendedStrength * 0.3f;
		}
		
		// Clamp targets using per-weapon max values
		targetPos = ClampVector(targetPos, a_weaponSettings.movementMaxOffset);
		float maxRotRad = a_weaponSettings.movementMaxRotation * DEG_TO_RAD;
		targetRot = ClampVector(targetRot, maxRotRad);
		
		// === FRAMERATE INDEPENDENCE: Sub-stepping ===
		constexpr float MAX_SUBSTEP = 0.016f;
		int numSteps = static_cast<int>(std::ceil(a_delta / MAX_SUBSTEP));
		numSteps = std::clamp(numSteps, 1, 4);
		float stepDelta = a_delta / static_cast<float>(numSteps);
		
		constexpr float MAX_POS_VELOCITY = 300.0f;
		constexpr float MAX_ROT_VELOCITY = 30.0f;
		
		for (int step = 0; step < numSteps; ++step) {
			// Spring physics for position: F = -k * (current - target) - c * velocity
			RE::NiPoint3 posSpringForce = {
				-k * (a_state.positionOffset.x - targetPos.x) - c * a_state.positionVelocity.x,
				-k * (a_state.positionOffset.y - targetPos.y) - c * a_state.positionVelocity.y,
				-k * (a_state.positionOffset.z - targetPos.z) - c * a_state.positionVelocity.z
			};
			
			a_state.positionVelocity.x += (posSpringForce.x / m) * stepDelta;
			a_state.positionVelocity.y += (posSpringForce.y / m) * stepDelta;
			a_state.positionVelocity.z += (posSpringForce.z / m) * stepDelta;
			
			a_state.positionVelocity = ClampVector(a_state.positionVelocity, MAX_POS_VELOCITY);
			
			a_state.positionOffset.x += a_state.positionVelocity.x * stepDelta;
			a_state.positionOffset.y += a_state.positionVelocity.y * stepDelta;
			a_state.positionOffset.z += a_state.positionVelocity.z * stepDelta;
			
			a_state.positionOffset = ClampVector(a_state.positionOffset, a_weaponSettings.movementMaxOffset);
			
			// Spring physics for rotation
			RE::NiPoint3 rotSpringForce = {
				-k * (a_state.rotationOffset.x - targetRot.x) - c * a_state.rotationVelocity.x,
				-k * (a_state.rotationOffset.y - targetRot.y) - c * a_state.rotationVelocity.y,
				-k * (a_state.rotationOffset.z - targetRot.z) - c * a_state.rotationVelocity.z
			};
			
			a_state.rotationVelocity.x += (rotSpringForce.x / m) * stepDelta;
			a_state.rotationVelocity.y += (rotSpringForce.y / m) * stepDelta;
			a_state.rotationVelocity.z += (rotSpringForce.z / m) * stepDelta;
			
			a_state.rotationVelocity = ClampVector(a_state.rotationVelocity, MAX_ROT_VELOCITY);
			
			a_state.rotationOffset.x += a_state.rotationVelocity.x * stepDelta;
			a_state.rotationOffset.y += a_state.rotationVelocity.y * stepDelta;
			a_state.rotationOffset.z += a_state.rotationVelocity.z * stepDelta;
			
			a_state.rotationOffset = ClampVector(a_state.rotationOffset, maxRotRad);
		}
	}
	
	// Update sprint transition inertia
	// Applies an impulse on sprint state change, then spring settles naturally
	void UpdateSprintSpring(SpringState& a_state, const WeaponInertiaSettings& a_weaponSettings,
		bool a_isSprinting, bool a_wasSprinting,
		float& a_blendProgress, float& a_blendDuration,
		RE::NiPoint3& a_pendingPosImpulse, RE::NiPoint3& a_pendingRotImpulse,
		float a_delta)
	{
		if (!a_weaponSettings.sprintInertiaEnabled) {
			// Quickly decay to zero when disabled
			float decayRate = std::min(1.0f, a_delta * 10.0f);
			a_state.positionOffset = LerpVector(a_state.positionOffset, {0,0,0}, decayRate);
			a_state.rotationOffset = LerpVector(a_state.rotationOffset, {0,0,0}, decayRate);
			a_state.positionVelocity = LerpVector(a_state.positionVelocity, {0,0,0}, decayRate);
			a_state.rotationVelocity = LerpVector(a_state.rotationVelocity, {0,0,0}, decayRate);
			a_blendProgress = 0.0f;
			a_pendingPosImpulse = {0,0,0};
			a_pendingRotImpulse = {0,0,0};
			return;
		}
		
		constexpr float DEG_TO_RAD_LOCAL = 3.14159265358979323846f / 180.0f;
		
		// Detect sprint state transitions and set up impulse
		if (a_isSprinting != a_wasSprinting) {
			// Calculate the total impulse to apply
			if (a_isSprinting) {
				// Starting sprint - arms lag behind
				a_pendingPosImpulse = {
					0.0f,
					a_weaponSettings.sprintImpulseY * 20.0f,
					-a_weaponSettings.sprintImpulseZ * 15.0f
				};
				a_pendingRotImpulse = {
					a_weaponSettings.sprintRotImpulse * DEG_TO_RAD_LOCAL * 10.0f,
					0.0f,
					0.0f
				};
			} else {
				// Stopping sprint - arms overshoot forward
				a_pendingPosImpulse = {
					0.0f,
					-a_weaponSettings.sprintImpulseY * 25.0f,
					a_weaponSettings.sprintImpulseZ * 12.0f
				};
				a_pendingRotImpulse = {
					-a_weaponSettings.sprintRotImpulse * DEG_TO_RAD_LOCAL * 8.0f,
					0.0f,
					0.0f
				};
			}
			
			// Set up blend
			a_blendDuration = a_weaponSettings.sprintImpulseBlendTime;
			a_blendProgress = 0.0f;
			
			// If blend time is 0, apply instantly
			if (a_blendDuration <= 0.001f) {
				a_state.positionVelocity.x += a_pendingPosImpulse.x;
				a_state.positionVelocity.y += a_pendingPosImpulse.y;
				a_state.positionVelocity.z += a_pendingPosImpulse.z;
				a_state.rotationVelocity.x += a_pendingRotImpulse.x;
				a_state.rotationVelocity.y += a_pendingRotImpulse.y;
				a_state.rotationVelocity.z += a_pendingRotImpulse.z;
				a_pendingPosImpulse = {0,0,0};
				a_pendingRotImpulse = {0,0,0};
				a_blendProgress = 1.0f;
			}
		}
		
		// Apply pending impulse gradually if blending
		if (a_blendProgress < 1.0f && a_blendDuration > 0.001f) {
			float prevProgress = a_blendProgress;
			a_blendProgress = std::min(1.0f, a_blendProgress + a_delta / a_blendDuration);
			
			// Apply the portion of impulse for this frame
			float deltaProgress = a_blendProgress - prevProgress;
			a_state.positionVelocity.x += a_pendingPosImpulse.x * deltaProgress;
			a_state.positionVelocity.y += a_pendingPosImpulse.y * deltaProgress;
			a_state.positionVelocity.z += a_pendingPosImpulse.z * deltaProgress;
			a_state.rotationVelocity.x += a_pendingRotImpulse.x * deltaProgress;
			a_state.rotationVelocity.y += a_pendingRotImpulse.y * deltaProgress;
			a_state.rotationVelocity.z += a_pendingRotImpulse.z * deltaProgress;
			
			// Clear pending when done
			if (a_blendProgress >= 1.0f) {
				a_pendingPosImpulse = {0,0,0};
				a_pendingRotImpulse = {0,0,0};
			}
		}
		
		// Spring physics to settle back to zero (target is always neutral)
		float k = a_weaponSettings.sprintStiffness;
		float c = a_weaponSettings.sprintDamping;
		float m = 1.0f;
		
		// Sub-stepping for stability
		constexpr float MAX_SUBSTEP = 0.016f;
		int numSteps = static_cast<int>(std::ceil(a_delta / MAX_SUBSTEP));
		numSteps = std::clamp(numSteps, 1, 4);
		float stepDelta = a_delta / static_cast<float>(numSteps);
		
		constexpr float MAX_POS_VELOCITY = 400.0f;
		constexpr float MAX_ROT_VELOCITY = 40.0f;
		
		for (int step = 0; step < numSteps; ++step) {
			// Position spring: F = -k * position - c * velocity (target is 0,0,0)
			RE::NiPoint3 posForce = {
				-k * a_state.positionOffset.x - c * a_state.positionVelocity.x,
				-k * a_state.positionOffset.y - c * a_state.positionVelocity.y,
				-k * a_state.positionOffset.z - c * a_state.positionVelocity.z
			};
			
			a_state.positionVelocity.x += (posForce.x / m) * stepDelta;
			a_state.positionVelocity.y += (posForce.y / m) * stepDelta;
			a_state.positionVelocity.z += (posForce.z / m) * stepDelta;
			
			a_state.positionVelocity = ClampVector(a_state.positionVelocity, MAX_POS_VELOCITY);
			
			a_state.positionOffset.x += a_state.positionVelocity.x * stepDelta;
			a_state.positionOffset.y += a_state.positionVelocity.y * stepDelta;
			a_state.positionOffset.z += a_state.positionVelocity.z * stepDelta;
			
			// Clamp max offset
			a_state.positionOffset = ClampVector(a_state.positionOffset, 30.0f);
			
			// Rotation spring
			RE::NiPoint3 rotForce = {
				-k * a_state.rotationOffset.x - c * a_state.rotationVelocity.x,
				-k * a_state.rotationOffset.y - c * a_state.rotationVelocity.y,
				-k * a_state.rotationOffset.z - c * a_state.rotationVelocity.z
			};
			
			a_state.rotationVelocity.x += (rotForce.x / m) * stepDelta;
			a_state.rotationVelocity.y += (rotForce.y / m) * stepDelta;
			a_state.rotationVelocity.z += (rotForce.z / m) * stepDelta;
			
			a_state.rotationVelocity = ClampVector(a_state.rotationVelocity, MAX_ROT_VELOCITY);
			
			a_state.rotationOffset.x += a_state.rotationVelocity.x * stepDelta;
			a_state.rotationOffset.y += a_state.rotationVelocity.y * stepDelta;
			a_state.rotationOffset.z += a_state.rotationVelocity.z * stepDelta;
			
			// Clamp max rotation (45 degrees in radians)
			a_state.rotationOffset = ClampVector(a_state.rotationOffset, 0.785f);
		}
	}
	
	// Update jump/landing inertia
	// Applies impulse on jump start, uses low stiffness while airborne,
	// then applies landing impulse with high stiffness scaled by air time
	// Blends smoothly between jump and landing states if landing before jump settles
	void UpdateJumpSpring(SpringState& a_state, const WeaponInertiaSettings& a_weaponSettings,
		bool a_isInAir, bool a_wasInAir, bool a_didJump, float a_airTime, bool a_landingDetected,
		float& a_currentStiffness, float& a_currentDamping, float a_delta)
	{
		if (!a_weaponSettings.jumpInertiaEnabled) {
			// Quickly decay to zero when disabled
			float decayRate = std::min(1.0f, a_delta * 10.0f);
			a_state.positionOffset = LerpVector(a_state.positionOffset, {0,0,0}, decayRate);
			a_state.rotationOffset = LerpVector(a_state.rotationOffset, {0,0,0}, decayRate);
			a_state.positionVelocity = LerpVector(a_state.positionVelocity, {0,0,0}, decayRate);
			a_state.rotationVelocity = LerpVector(a_state.rotationVelocity, {0,0,0}, decayRate);
			return;
		}
		
		constexpr float DEG_TO_RAD_LOCAL = 3.14159265358979323846f / 180.0f;
		
		// Detect state transitions
		if (a_isInAir && !a_wasInAir) {
			// Just left the ground (jump or fall)
			// Use airborne spring parameters
			a_currentStiffness = a_weaponSettings.jumpStiffness;
			a_currentDamping = a_weaponSettings.jumpDamping;
			
			if (a_didJump) {
				// Player actually jumped - apply full jump impulse
				// Arms dip down and back as player pushes off
				a_state.positionVelocity.y += a_weaponSettings.jumpImpulseY * 15.0f;
				a_state.positionVelocity.z -= a_weaponSettings.jumpImpulseZ * 20.0f;
				a_state.rotationVelocity.x += a_weaponSettings.jumpRotImpulse * DEG_TO_RAD_LOCAL * 8.0f;
			} else {
				// Player fell off a ledge - apply gentler fall impulse
				// Slight downward/forward motion as arms follow momentum
				a_state.positionVelocity.y += a_weaponSettings.fallImpulseY * 15.0f;
				a_state.positionVelocity.z -= a_weaponSettings.fallImpulseZ * 20.0f;
				a_state.rotationVelocity.x += a_weaponSettings.fallRotImpulse * DEG_TO_RAD_LOCAL * 8.0f;
			}
		}
		else if (a_landingDetected) {
			// Just landed (detected via SBF_ReadyStart or state transition with cooldown)
			// Switch to landing spring parameters (tighter)
			a_currentStiffness = a_weaponSettings.landStiffness;
			a_currentDamping = a_weaponSettings.landDamping;
			
			// Calculate how "settled" the spring currently is (0 = max energy, 1 = fully settled)
			// This allows smooth blending if landing before jump spring has settled
			float posEnergy = std::sqrt(
				a_state.positionOffset.x * a_state.positionOffset.x +
				a_state.positionOffset.y * a_state.positionOffset.y +
				a_state.positionOffset.z * a_state.positionOffset.z);
			float velEnergy = std::sqrt(
				a_state.positionVelocity.x * a_state.positionVelocity.x +
				a_state.positionVelocity.y * a_state.positionVelocity.y +
				a_state.positionVelocity.z * a_state.positionVelocity.z);
			
			// Normalize energy (rough estimate based on typical max values)
			float normalizedEnergy = std::clamp((posEnergy / 20.0f) + (velEnergy / 200.0f), 0.0f, 1.0f);
			
			// Settled factor: 1 when spring is idle, 0 when spring has max energy
			float settledFactor = 1.0f - normalizedEnergy;
			
			// Calculate landing impulse scale based on air time
			// Minimum 0.3 sec to register as significant landing, caps at ~2 sec
			float normalizedAirTime = std::clamp((a_airTime - 0.1f) / 1.5f, 0.0f, 1.0f);
			
			// If player didn't jump (just fell), use a different base scale
			float baseScale = a_didJump ? 1.0f : 0.7f;
			
			// Apply air time scaling with configurable exponent
			float impulseScale = baseScale * (0.3f + normalizedAirTime * a_weaponSettings.airTimeImpulseScale);
			
			// Blend landing impulse based on how settled the spring is
			// If spring has lots of energy (unsettled), reduce landing impulse to avoid stacking
			// Minimum 30% of landing impulse always applies to ensure landing feels responsive
			float blendedScale = impulseScale * (0.3f + 0.7f * settledFactor);
			
			// If spring is unsettled, also dampen existing velocity to transition smoothly
			// This prevents the jump spring momentum from fighting the landing impulse
			if (normalizedEnergy > 0.3f) {
				float dampFactor = 0.5f + 0.5f * settledFactor; // 0.5 to 1.0
				a_state.positionVelocity.x *= dampFactor;
				a_state.positionVelocity.y *= dampFactor;
				a_state.positionVelocity.z *= dampFactor;
				a_state.rotationVelocity.x *= dampFactor;
				a_state.rotationVelocity.y *= dampFactor;
				a_state.rotationVelocity.z *= dampFactor;
			}
			
			// Landing impulse - arms compress down then bounce back
			a_state.positionVelocity.y -= a_weaponSettings.landImpulseY * blendedScale * 20.0f;
			a_state.positionVelocity.z -= a_weaponSettings.landImpulseZ * blendedScale * 25.0f;
			a_state.rotationVelocity.x += a_weaponSettings.landRotImpulse * DEG_TO_RAD_LOCAL * blendedScale * 10.0f;
		}
		
		// Spring physics with current stiffness/damping
		float k = a_currentStiffness;
		float c = a_currentDamping;
		float m = 1.0f;
		
		// Sub-stepping for stability
		constexpr float MAX_SUBSTEP = 0.016f;
		int numSteps = static_cast<int>(std::ceil(a_delta / MAX_SUBSTEP));
		numSteps = std::clamp(numSteps, 1, 4);
		float stepDelta = a_delta / static_cast<float>(numSteps);
		
		constexpr float MAX_POS_VELOCITY = 500.0f;
		constexpr float MAX_ROT_VELOCITY = 50.0f;
		
		for (int step = 0; step < numSteps; ++step) {
			// Position spring: F = -k * position - c * velocity (target is 0,0,0)
			RE::NiPoint3 posForce = {
				-k * a_state.positionOffset.x - c * a_state.positionVelocity.x,
				-k * a_state.positionOffset.y - c * a_state.positionVelocity.y,
				-k * a_state.positionOffset.z - c * a_state.positionVelocity.z
			};
			
			a_state.positionVelocity.x += (posForce.x / m) * stepDelta;
			a_state.positionVelocity.y += (posForce.y / m) * stepDelta;
			a_state.positionVelocity.z += (posForce.z / m) * stepDelta;
			
			a_state.positionVelocity = ClampVector(a_state.positionVelocity, MAX_POS_VELOCITY);
			
			a_state.positionOffset.x += a_state.positionVelocity.x * stepDelta;
			a_state.positionOffset.y += a_state.positionVelocity.y * stepDelta;
			a_state.positionOffset.z += a_state.positionVelocity.z * stepDelta;
			
			// Clamp max offset
			a_state.positionOffset = ClampVector(a_state.positionOffset, 40.0f);
			
			// Rotation spring
			RE::NiPoint3 rotForce = {
				-k * a_state.rotationOffset.x - c * a_state.rotationVelocity.x,
				-k * a_state.rotationOffset.y - c * a_state.rotationVelocity.y,
				-k * a_state.rotationOffset.z - c * a_state.rotationVelocity.z
			};
			
			a_state.rotationVelocity.x += (rotForce.x / m) * stepDelta;
			a_state.rotationVelocity.y += (rotForce.y / m) * stepDelta;
			a_state.rotationVelocity.z += (rotForce.z / m) * stepDelta;
			
			a_state.rotationVelocity = ClampVector(a_state.rotationVelocity, MAX_ROT_VELOCITY);
			
			a_state.rotationOffset.x += a_state.rotationVelocity.x * stepDelta;
			a_state.rotationOffset.y += a_state.rotationVelocity.y * stepDelta;
			a_state.rotationOffset.z += a_state.rotationVelocity.z * stepDelta;
			
			// Clamp max rotation
			a_state.rotationOffset = ClampVector(a_state.rotationOffset, 0.785f);
		}
	}

	RE::NiPoint3 InertiaManager::CalculateLocalMovement(RE::PlayerCharacter* a_player, float a_delta)
	{
		if (!a_player || a_delta <= 0.0f) {
			return { 0.0f, 0.0f, 0.0f };
		}
		
		// Use player input directly - much more reliable than position tracking
		// This gives us the raw input values already in local (camera-relative) space
		auto* playerControls = RE::PlayerControls::GetSingleton();
		if (!playerControls) {
			return { 0.0f, 0.0f, 0.0f };
		}
		
		// moveInputVec is already in local space:
		// X = left/right input (-1 = left, +1 = right)
		// Y = forward/back input (-1 = back, +1 = forward)
		RE::NiPoint2 inputVec = playerControls->data.moveInputVec;
		
		// Scale input to approximate velocity (input is -1 to 1, scale to reasonable speed)
		// Normal walk speed is ~100-150 units/sec, run is ~300+
		constexpr float INPUT_TO_VELOCITY = 200.0f;
		
		RE::NiPoint3 localVelocity = {
			inputVec.x * INPUT_TO_VELOCITY,   // Local right (strafe)
			inputVec.y * INPUT_TO_VELOCITY,   // Local forward
			0.0f                               // No vertical from input
		};
		
		// Smooth the movement input to reduce jitter
		constexpr float MOVEMENT_SMOOTHING = 0.3f;
		smoothedLocalMovement = LerpVector(smoothedLocalMovement, localVelocity, 1.0f - MOVEMENT_SMOOTHING);
		
		return smoothedLocalMovement;
	}

	void InertiaManager::ApplyOffset(RE::NiNode* a_node, const SpringState& a_state,
		const WeaponInertiaSettings& a_settings, [[maybe_unused]] Hand a_hand)
	{
		if (!a_node) {
			return;
		}
		
		auto* settings = Settings::GetSingleton();
		
		// Get the offsets from spring state
		const RE::NiPoint3& positionOffset = a_state.positionOffset;
		const RE::NiPoint3& rotationOffset = a_state.rotationOffset;
		
		// Apply position offset (ADDITIVE from both camera and movement springs)
		if (settings->enablePosition) {
			a_node->local.translate.x += positionOffset.x;
			a_node->local.translate.y += positionOffset.y;
			a_node->local.translate.z += positionOffset.z;
		}
		
		// Apply rotation offset with pivot compensation
		if (settings->enableRotation && 
			(std::abs(rotationOffset.x) > 0.0001f ||
			 std::abs(rotationOffset.y) > 0.0001f ||
			 std::abs(rotationOffset.z) > 0.0001f)) {
			
			// Apply pivot compensation using PER-WEAPON pivot point
			// This adds a translation to make it APPEAR that rotation is around a different point
			int weaponPivot = a_settings.pivotPoint;
			if (weaponPivot != 0) {  // 0 = Chest, no compensation needed
				// Pivot offset is the approximate distance from spine to the pivot point
				float pivotDistance = 0.0f;
				switch (weaponPivot) {
				case 1:  // Right hand
				case 2:  // Left hand
					pivotDistance = 35.0f;
					break;
				case 3:  // Weapon
					pivotDistance = 50.0f;
					break;
				case 4:  // Both Clavicles (no compensation, applied at clavicle level)
					pivotDistance = 0.0f;
					break;
				case 5:  // Both Clavicles with Offset - compensate based on which side
					// Distance from clavicle to approximate hand position (arm length)
					// Similar to pivots 1/2 for consistent feel
					pivotDistance = 35.0f;
					break;
				default:
					break;
				}
				
				// Yaw rotation (around Z) causes X displacement at the pivot
				float yawCompensation = -pivotDistance * rotationOffset.z;
				// Pitch rotation (around X) causes Y displacement at the pivot
				float pitchCompensation = -pivotDistance * rotationOffset.x * 0.5f;
				
				a_node->local.translate.x += yawCompensation;
				a_node->local.translate.y += pitchCompensation;
			}
			
			RE::NiMatrix3 offsetRotation = EulerToMatrix(rotationOffset);
			a_node->local.rotate = a_node->local.rotate * offsetRotation;
		}
	}
	
	// Get the spine node for applying inertia (ALWAYS the spine)
	// Pivot point setting affects the MATH, not which node we modify
	// Uses caching to avoid expensive string-based node searches every frame
	RE::NiNode* InertiaManager::GetPivotNode(RE::NiNode* a_fpRoot, RE::PlayerCharacter* a_player)
	{
		// Return cached node if valid (pointer still valid and part of current skeleton)
		// We verify the cache is still valid by checking if the node's parent chain leads to our root
		if (cachedSpineNode) {
			// Quick validation: check the node still exists and has a valid name
			// If the skeleton was rebuilt, the pointer would be invalid
			// Using try-catch would be overkill; the skeleton rarely changes
			return cachedSpineNode;
		}
		
		// ALWAYS apply inertia to the spine - this affects the whole FP arms
		// The pivot point setting changes how the rotation is CALCULATED, not WHERE it's applied
		RE::NiNode* spineNode = FindNode(a_fpRoot, "NPC Spine2 [Spn2]");
		if (!spineNode) spineNode = FindNode(a_fpRoot, "NPC Spine1 [Spn1]");
		if (!spineNode) spineNode = FindNode(a_fpRoot, "NPC Spine [Spn0]");
		if (!spineNode) spineNode = FindNode(a_fpRoot, "Spine2");
		if (!spineNode) spineNode = FindNode(a_fpRoot, "Spine1");
		
		// Cache the result
		cachedSpineNode = spineNode;
		
		return spineNode;
	}

	void InertiaManager::Update(float a_delta)
	{
		auto* settings = Settings::GetSingleton();
		if (!settings->enabled) {
			return;
		}
		
		// Skip updates when any menu is open to prevent flickering/glitches
		// Consolidated early-exit: GameIsPaused covers most pause menus
		auto* ui = RE::UI::GetSingleton();
		if (ui && (ui->GameIsPaused() || ui->numPausesGame > 0)) {
			return;
		}
		
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}
		
		auto* camera = RE::PlayerCamera::GetSingleton();
		if (!camera) {
			return;
		}
		
		// Only process in first person
		if (!camera->IsInFirstPerson()) {
			if (isInFirstPerson) {
				OnExitFirstPerson();
			}
			return;
		}
		
		// Log skeleton hierarchy once for debugging node structure
		if (!hasLoggedSkeleton && settings->debugLogging) {
			LogFirstPersonSkeleton();
		}
		
		// Track weapon drawn state for blend
		auto* actorState = player->AsActorState();
		bool isWeaponDrawn = actorState && actorState->IsWeaponDrawn();
		
		// If weapon drawn requirement is disabled, always treat as drawn
		if (!settings->requireWeaponDrawn) {
			isWeaponDrawn = true;
		}
		
		// Update equip blend factor (smooth transition when drawing/sheathing)
		// Cap delta time to prevent instant blends from frame rate spikes or loading
		constexpr float MAX_BLEND_DELTA = 0.05f;  // 50ms max
		float cappedDelta = std::min(a_delta, MAX_BLEND_DELTA);
		
		constexpr float EQUIP_BLEND_IN_SPEED = 3.0f;   // Speed when drawing weapon (blend in)
		constexpr float EQUIP_BLEND_OUT_SPEED = 4.0f;  // Speed when sheathing weapon (blend out, slightly faster)
		float equipBlendTarget = isWeaponDrawn ? 1.0f : 0.0f;
		
		// Use appropriate speed based on direction
		float blendSpeed = (equipBlendFactor < equipBlendTarget) ? EQUIP_BLEND_IN_SPEED : EQUIP_BLEND_OUT_SPEED;
		float equipBlendDelta = blendSpeed * cappedDelta;
		
		float prevEquipBlendFactor = equipBlendFactor;
		
		if (equipBlendFactor < equipBlendTarget) {
			equipBlendFactor = std::min(equipBlendTarget, equipBlendFactor + equipBlendDelta);
		} else if (equipBlendFactor > equipBlendTarget) {
			equipBlendFactor = std::max(equipBlendTarget, equipBlendFactor - equipBlendDelta);
		}
		
		// Log equip state changes (only when debug logging enabled)
		if (settings->debugLogging) {
			if (isWeaponDrawn != wasWeaponDrawn) {
				logger::info("[FPInertia] Equip state change: drawn={} -> {}, equipBlendFactor={:.3f}",
					wasWeaponDrawn, isWeaponDrawn, equipBlendFactor);
			}
			
			// Log blend progress while blending (every few frames to avoid spam)
			bool isBlending = std::abs(equipBlendFactor - equipBlendTarget) > 0.001f;
			static int blendLogCounter = 0;
			if (isBlending) {
				blendLogCounter++;
				if (blendLogCounter % 10 == 0) {  // Log every 10 frames while blending
					logger::info("[FPInertia] Equip blend: {:.3f} -> {:.3f} (target={:.1f}, speed={:.1f}, delta={:.4f})",
						prevEquipBlendFactor, equipBlendFactor, equipBlendTarget, blendSpeed, cappedDelta);
				}
			} else {
				blendLogCounter = 0;
			}
		}
		
		// If fully sheathed and blend is complete, reset springs
		if (equipBlendFactor <= 0.001f && !isWeaponDrawn) {
			if (isInFirstPerson && lastTargetNode) {
				logger::info("[FPInertia] Fully sheathed, resetting springs");
				Reset();
				isInFirstPerson = false;
			}
			wasWeaponDrawn = isWeaponDrawn;
			return;
		}
		
		wasWeaponDrawn = isWeaponDrawn;
		
		if (!isInFirstPerson) {
			OnEnterFirstPerson();
		}
		
		// Increment debug frame counter
		debugFrameCounter++;
		
		// *** VANILLA SWAY CANCELLATION VIA BEHAVIOR GRAPH ***
		// If enabled, set the Direction behavior variable to 0.
		// This forces the locomotion system to always use forward animations,
		// eliminating the strafe sway. Our custom movement inertia takes over.
		if (settings->disableVanillaSway) {
			// Set Direction to 0 (forward) to prevent strafe animation blending
			player->SetGraphVariableFloat("Direction", 0.0f);
		}
		
		// Get the first person skeleton root
		auto* fpNode = GetFirstPersonNode();
		if (!fpNode) {
			// Log this error once per session
			static bool loggedFpNodeError = false;
			if (!loggedFpNodeError) {
				logger::error("[FPInertia] ERROR: GetFirstPersonNode returned nullptr!");
				loggedFpNodeError = true;
			}
			return;
		}
		
		// Calculate camera velocity
		RE::NiPoint3 rawCameraVelocity = CalculateCameraVelocity(a_delta);
		
		// Smooth the camera velocity to reduce jitter
		float smoothing = settings->smoothingFactor;
		smoothedCameraVelocity = LerpVector(smoothedCameraVelocity, rawCameraVelocity, 1.0f - smoothing);
		
		// Skip first frame to initialize camera tracking
		if (!initialized) {
			initialized = true;
			settlingFactor = 0.0f;
			timeSinceMovement = 0.0f;
			logger::info("[FPInertia] First frame initialized, starting inertia updates");
			return;
		}
		
		// Calculate settling factor based on camera movement
		// When camera is moving, reset settling. When stopped, gradually increase damping.
		float cameraSpeed = std::sqrt(
			smoothedCameraVelocity.x * smoothedCameraVelocity.x +
			smoothedCameraVelocity.y * smoothedCameraVelocity.y +
			smoothedCameraVelocity.z * smoothedCameraVelocity.z
		);
		
		constexpr float MOVEMENT_THRESHOLD = 0.1f; // Minimum velocity to count as "moving"
		
		if (cameraSpeed > MOVEMENT_THRESHOLD) {
			// Camera is moving - reset settling
			timeSinceMovement = 0.0f;
			settlingFactor = 0.0f;
		} else {
			// Camera stopped - start settling after delay
			timeSinceMovement += a_delta;
			
			if (timeSinceMovement > settings->settleDelay) {
				// Gradually increase settling factor
				float settleTime = timeSinceMovement - settings->settleDelay;
				settlingFactor = std::min(1.0f, settleTime * settings->settleSpeed);
			}
		}
		
		// Action blending - reduce intensity during attacks/bow draw/spells
		bool inAction = IsPlayerInAction(player);
		float targetBlend = inAction ? settings->actionMinIntensity : 1.0f;
		
		// Smooth blend towards target
		if (actionBlendFactor < targetBlend) {
			actionBlendFactor = std::min(targetBlend, actionBlendFactor + a_delta * settings->actionBlendSpeed);
		} else if (actionBlendFactor > targetBlend) {
			actionBlendFactor = std::max(targetBlend, actionBlendFactor - a_delta * settings->actionBlendSpeed);
		}
		
		// Get weapon types for each hand
		RE::WEAPON_TYPE rightWeaponType = GetWeaponTypeForHand(player, Hand::kRight);
		bool isTwoHanded = IsTwoHandedWeapon(rightWeaponType);
		
		// Get settings based on weapon type (uses preset system - checks EditorID first, then type)
		const WeaponInertiaSettings& primarySettings = GetCurrentWeaponSettings(player, Hand::kRight);
		
		// *** CHECK MASTER ENABLE ***
		// If inertia is disabled for this weapon type, reset springs and exit early
		if (!primarySettings.enabled) {
			// Reset springs so there's no lingering offset when switching to an enabled type
			if (!wasInertiaDisabled) {
				cameraSpring.Reset();
				movementSpring.Reset();
				cameraSpringLeft.Reset();
				movementSpringLeft.Reset();
				sprintSpring.Reset();
				sprintSpringLeft.Reset();
				jumpSpring.Reset();
				jumpSpringLeft.Reset();
				wasInertiaDisabled = true;
				if (settings->debugLogging) {
					logger::info("[FPInertia] Inertia disabled for current weapon type - springs reset");
				}
			}
			return;
		} else {
			wasInertiaDisabled = false;
		}
		
		// *** CALCULATE PLAYER MOVEMENT FIRST ***
		// This must happen before spring updates so movement data is available
		CalculateLocalMovement(player, a_delta);
		
		// *** CHECK SPRINT STATE ***
		wasSprinting = isSprinting;
		isSprinting = player->AsActorState()->IsSprinting();
		
		// *** CHECK JUMP/AIR STATE ***
		wasInAir = isInAir;
		
		// Check if player is in the air (jumping or falling)
		bool currentlyInAir = player->IsInMidair();
		
		// Update landing cooldown
		if (landingCooldown > 0.0f) {
			landingCooldown -= a_delta;
		}
		
		// Track air time for landing impulse scaling
		if (currentlyInAir && !wasInAir) {
			// Just left ground - reset air time
			// Detect if player jumped vs fell by checking behavior graph "bInJumpState"
			// This is set by the game when the player actually presses jump
			bool inJumpState = false;
			player->GetGraphVariableBool("bInJumpState", inJumpState);
			didJump = inJumpState;
			airTime = 0.0f;
		}
		
		if (currentlyInAir) {
			airTime += a_delta;
		}
		
		// Check for landing via behavior graph state (more reliable than IsInMidair transition)
		// SBF_ReadyStart triggers when the landing animation begins
		bool landingDetected = false;
		if (!currentlyInAir && wasInAir && landingCooldown <= 0.0f) {
			// Also check for SBF_ReadyStart for more accurate landing detection
			bool readyStart = false;
			if (player->GetGraphVariableBool("SBF_ReadyStart", readyStart) && readyStart) {
				landingDetected = true;
				landingCooldown = 0.25f;  // Prevent multiple triggers
			} else {
				// Fallback: if we just transitioned from air to ground, count it as landing
				landingDetected = true;
				landingCooldown = 0.25f;
			}
		}
		
		isInAir = currentlyInAir;
		
		// Blend movement inertia out while in air, back in when grounded
		constexpr float AIR_BLEND_SPEED = 8.0f;  // How fast to blend in/out
		float airBlendTarget = isInAir ? 0.0f : 1.0f;
		float airBlendDelta = AIR_BLEND_SPEED * a_delta;
		if (movementAirBlend < airBlendTarget) {
			movementAirBlend = std::min(movementAirBlend + airBlendDelta, airBlendTarget);
		} else if (movementAirBlend > airBlendTarget) {
			movementAirBlend = std::max(movementAirBlend - airBlendDelta, airBlendTarget);
		}
		
		// Blend camera inertia to configurable multiplier while in air
		float cameraAirTarget = isInAir ? primarySettings.cameraInertiaAirMult : 1.0f;
		if (cameraAirBlend < cameraAirTarget) {
			cameraAirBlend = std::min(cameraAirBlend + airBlendDelta, cameraAirTarget);
		} else if (cameraAirBlend > cameraAirTarget) {
			cameraAirBlend = std::max(cameraAirBlend - airBlendDelta, cameraAirTarget);
		}
		
		// *** DETECT CURRENT STANCE ***
		// Get current stance from stance mods (Stances NG, Dynamic Weapon Movesets)
		// This affects the global intensity multiplier for all inertia
		previousStance = currentStance;
		currentStance = GetCurrentStance();
		
		// Get stance multiplier from per-weapon settings
		float stanceMultiplier = primarySettings.stanceMultipliers[static_cast<size_t>(currentStance)];

		// Get stance-specific invert overrides (when true, XOR with base invert settings)
		bool stanceInvertCamera = primarySettings.stanceInvertCamera[static_cast<size_t>(currentStance)];
		bool stanceInvertMovement = primarySettings.stanceInvertMovement[static_cast<size_t>(currentStance)];
		
		// Log stance changes (debug only)
		if (settings->debugLogging && currentStance != previousStance) {
			logger::info("[FPInertia] Stance changed: {} -> {} (mult: {:.2f}, invertCam: {}, invertMov: {})",
				GetStanceName(previousStance), GetStanceName(currentStance), stanceMultiplier,
				stanceInvertCamera ? "yes" : "no", stanceInvertMovement ? "yes" : "no");
		}
		
		// *** UPDATE CAMERA SPRING ***
		// Responds to camera rotation (looking around) - uses per-weapon settings
		// Multiply intensity by equipBlendFactor so springs decay when weapon is sheathed
		// Also apply stance multiplier for per-stance intensity adjustment
		// This prevents built-up spring state from suddenly appearing when drawing a weapon
		float cameraIntensity = actionBlendFactor * equipBlendFactor * stanceMultiplier;
		UpdateSpring(cameraSpring, primarySettings, smoothedCameraVelocity, a_delta, cameraIntensity, stanceInvertCamera);
		
		// *** UPDATE MOVEMENT SPRING (SEPARATE) ***
		// Responds to player strafing - uses per-weapon movement spring settings
		// Also uses equipBlendFactor to decay when weapon is sheathed
		// Also apply stance multiplier for per-stance intensity adjustment
		float movementIntensity = actionBlendFactor * equipBlendFactor * stanceMultiplier;
		UpdateMovementSpring(movementSpring, settings, primarySettings, smoothedLocalMovement, a_delta, movementIntensity, stanceInvertMovement);
		
		// *** UPDATE LEFT HAND SPRINGS (for dual clavicle pivot modes) ***
		// Only use clavicle pivots (4 or 5) if we're actually in dual wield mode
		// If a non-dual-wield type has pivot 4/5 set (by mistake or from config), force it back to Chest (0)
		bool useDualClaviclePivot = (primarySettings.pivotPoint == 4 || primarySettings.pivotPoint == 5) && isDualWieldMode;
		
		// Log dual wield settings being applied (once per frame batch)
		static bool loggedDualWieldSettings = false;
		if (isDualWieldMode && !loggedDualWieldSettings && settings->debugLogging) {
			const char* dualTypeName = (currentDualWieldType == WeaponType::DualWieldWeapons) ? "DualWieldWeapons" :
			                           (currentDualWieldType == WeaponType::DualWieldMagic) ? "DualWieldMagic" :
			                           (currentDualWieldType == WeaponType::SpellAndWeapon) ? "SpellAndWeapon" : "Unknown";
			logger::info("[FPInertia] Applying {} settings: pivot={}, stiffness={:.0f}, damping={:.1f}, useDualClavicle={}",
				dualTypeName, primarySettings.pivotPoint, primarySettings.stiffness, primarySettings.damping,
				useDualClaviclePivot ? "true" : "false");
			loggedDualWieldSettings = true;
		} else if (!isDualWieldMode) {
			loggedDualWieldSettings = false;
		}
		
		// Log warning if pivot 4 is set but not in dual wield mode (only once per weapon change)
		static bool loggedPivotFallback = false;
		if ((primarySettings.pivotPoint == 4 || primarySettings.pivotPoint == 5) && !isDualWieldMode) {
			if (!loggedPivotFallback && settings->debugLogging) {
				logger::warn("[FPInertia] Dual clavicle pivot ({}) is set but not in dual wield mode - falling back to Chest (0)", primarySettings.pivotPoint);
				loggedPivotFallback = true;
			}
		} else {
			loggedPivotFallback = false;
		}
		
		if (useDualClaviclePivot) {
			// Update left hand springs independently
			// They use the same input but maintain separate state for natural asymmetry
			UpdateSpring(cameraSpringLeft, primarySettings, smoothedCameraVelocity, a_delta, cameraIntensity, stanceInvertCamera);
			UpdateMovementSpring(movementSpringLeft, settings, primarySettings, smoothedLocalMovement, a_delta, movementIntensity, stanceInvertMovement);
		}
		
		// Log spring intensities and output periodically while blending (debug only)
		if (settings->debugLogging) {
			static int springLogCounter = 0;
			springLogCounter++;
			if (springLogCounter % 30 == 0 && (equipBlendFactor > 0.001f && equipBlendFactor < 0.999f)) {
				logger::info("[FPInertia] Spring intensity: action={:.3f}, equip={:.3f}, camera={:.3f}, movement={:.3f}",
					actionBlendFactor, equipBlendFactor, cameraIntensity, movementIntensity);
				logger::info("[FPInertia] Movement spring: pos=({:.3f},{:.3f},{:.3f}), vel=({:.3f},{:.3f},{:.3f})",
					movementSpring.positionOffset.x, movementSpring.positionOffset.y, movementSpring.positionOffset.z,
					movementSpring.positionVelocity.x, movementSpring.positionVelocity.y, movementSpring.positionVelocity.z);
			}
		}
		
		// *** UPDATE SPRINT SPRING ***
		// Applies impulse on sprint transitions, then spring settles
		UpdateSprintSpring(sprintSpring, primarySettings, isSprinting, wasSprinting,
			sprintImpulseBlendProgress, sprintImpulseBlendDuration,
			sprintPendingPosImpulse, sprintPendingRotImpulse, a_delta);
		
		// *** UPDATE JUMP SPRING ***
		// Applies impulse on jump and landing with air time scaling
		UpdateJumpSpring(jumpSpring, primarySettings, isInAir, wasInAir, didJump, airTime, landingDetected,
			currentJumpStiffness, currentJumpDamping, a_delta);
		
		// Update left hand sprint and jump springs for dual clavicle pivot mode
		if (useDualClaviclePivot) {
			// Left hand sprint spring (uses same state tracking as right hand)
			float sprintBlendProgressLeft = sprintImpulseBlendProgress;  // Share progress but maintain separate spring state
			float sprintBlendDurationLeft = sprintImpulseBlendDuration;
			RE::NiPoint3 sprintPosImpulseLeft = sprintPendingPosImpulse;
			RE::NiPoint3 sprintRotImpulseLeft = sprintPendingRotImpulse;
			UpdateSprintSpring(sprintSpringLeft, primarySettings, isSprinting, wasSprinting,
				sprintBlendProgressLeft, sprintBlendDurationLeft,
				sprintPosImpulseLeft, sprintRotImpulseLeft, a_delta);
			
			// Left hand jump spring
			float jumpStiffnessLeft = currentJumpStiffness;
			float jumpDampingLeft = currentJumpDamping;
			UpdateJumpSpring(jumpSpringLeft, primarySettings, isInAir, wasInAir, didJump, airTime, landingDetected,
				jumpStiffnessLeft, jumpDampingLeft, a_delta);
		}
		
		// *** COMBINE SPRINGS ADDITIVELY ***
		// Camera (blended by air mult), movement (blended out in air), sprint, and jump inertia all contribute
		// Movement inertia is blended out while in air to prevent ground-based sway during jumps
		// Camera inertia is dampened while in air based on cameraInertiaAirMult setting
		// Both camera and movement inertia are also blended by equipBlendFactor during equip/holster
		
		// Check if both camera AND movement are active simultaneously
		// Apply per-weapon scaling multipliers when both are active, with smooth blend
		float camMagnitude = std::sqrt(
			cameraSpring.positionOffset.x * cameraSpring.positionOffset.x +
			cameraSpring.positionOffset.z * cameraSpring.positionOffset.z +
			cameraSpring.rotationOffset.x * cameraSpring.rotationOffset.x +
			cameraSpring.rotationOffset.z * cameraSpring.rotationOffset.z);
		float movMagnitude = std::sqrt(
			movementSpring.positionOffset.x * movementSpring.positionOffset.x +
			movementSpring.positionOffset.y * movementSpring.positionOffset.y +
			movementSpring.rotationOffset.z * movementSpring.rotationOffset.z);
		
		// Calculate smooth blend factors for simultaneous scaling
		// Blend range: how far above threshold before full scaling applies
		float activeThreshold = primarySettings.simultaneousThreshold;
		constexpr float BLEND_RANGE = 1.0f;  // Units above threshold for full blend
		
		// Calculate how far above threshold each is (0 = at/below threshold, 1 = fully above blend range)
		float camAboveThreshold = std::clamp((camMagnitude - activeThreshold) / BLEND_RANGE, 0.0f, 1.0f);
		float movAboveThreshold = std::clamp((movMagnitude - activeThreshold) / BLEND_RANGE, 0.0f, 1.0f);
		
		// Both must be above threshold - use minimum as the limiting factor
		// This means scaling only applies when BOTH are active, and blends based on the lesser one
		float simultaneousBlend = camAboveThreshold * movAboveThreshold;
		
		// Smoothstep for more natural feel (ease in/out)
		simultaneousBlend = simultaneousBlend * simultaneousBlend * (3.0f - 2.0f * simultaneousBlend);
		
		// Interpolate from 1.0 (no scaling) toward target mult based on blend
		float camSimultaneousMult = 1.0f + (primarySettings.simultaneousCameraMult - 1.0f) * simultaneousBlend;
		float movSimultaneousMult = 1.0f + (primarySettings.simultaneousMovementMult - 1.0f) * simultaneousBlend;
		
		SpringState combinedState;
		combinedState.positionOffset = {
			(cameraSpring.positionOffset.x * cameraAirBlend * equipBlendFactor * camSimultaneousMult) + (movementSpring.positionOffset.x * movementAirBlend * equipBlendFactor * movSimultaneousMult) + sprintSpring.positionOffset.x + jumpSpring.positionOffset.x,
			(cameraSpring.positionOffset.y * cameraAirBlend * equipBlendFactor * camSimultaneousMult) + (movementSpring.positionOffset.y * movementAirBlend * equipBlendFactor * movSimultaneousMult) + sprintSpring.positionOffset.y + jumpSpring.positionOffset.y,
			(cameraSpring.positionOffset.z * cameraAirBlend * equipBlendFactor * camSimultaneousMult) + (movementSpring.positionOffset.z * movementAirBlend * equipBlendFactor * movSimultaneousMult) + sprintSpring.positionOffset.z + jumpSpring.positionOffset.z
		};
		combinedState.rotationOffset = {
			(cameraSpring.rotationOffset.x * cameraAirBlend * equipBlendFactor * camSimultaneousMult) + (movementSpring.rotationOffset.x * movementAirBlend * equipBlendFactor * movSimultaneousMult) + sprintSpring.rotationOffset.x + jumpSpring.rotationOffset.x,
			(cameraSpring.rotationOffset.y * cameraAirBlend * equipBlendFactor * camSimultaneousMult) + (movementSpring.rotationOffset.y * movementAirBlend * equipBlendFactor * movSimultaneousMult) + sprintSpring.rotationOffset.y + jumpSpring.rotationOffset.y,
			(cameraSpring.rotationOffset.z * cameraAirBlend * equipBlendFactor * camSimultaneousMult) + (movementSpring.rotationOffset.z * movementAirBlend * equipBlendFactor * movSimultaneousMult) + sprintSpring.rotationOffset.z + jumpSpring.rotationOffset.z
		};
		
		// Combine left hand springs for dual clavicle pivot mode
		SpringState combinedStateLeft;
		if (useDualClaviclePivot) {
			combinedStateLeft.positionOffset = {
				(cameraSpringLeft.positionOffset.x * cameraAirBlend * equipBlendFactor * camSimultaneousMult) + (movementSpringLeft.positionOffset.x * movementAirBlend * equipBlendFactor * movSimultaneousMult) + sprintSpringLeft.positionOffset.x + jumpSpringLeft.positionOffset.x,
				(cameraSpringLeft.positionOffset.y * cameraAirBlend * equipBlendFactor * camSimultaneousMult) + (movementSpringLeft.positionOffset.y * movementAirBlend * equipBlendFactor * movSimultaneousMult) + sprintSpringLeft.positionOffset.y + jumpSpringLeft.positionOffset.y,
				(cameraSpringLeft.positionOffset.z * cameraAirBlend * equipBlendFactor * camSimultaneousMult) + (movementSpringLeft.positionOffset.z * movementAirBlend * equipBlendFactor * movSimultaneousMult) + sprintSpringLeft.positionOffset.z + jumpSpringLeft.positionOffset.z
			};
			combinedStateLeft.rotationOffset = {
				(cameraSpringLeft.rotationOffset.x * cameraAirBlend * equipBlendFactor * camSimultaneousMult) + (movementSpringLeft.rotationOffset.x * movementAirBlend * equipBlendFactor * movSimultaneousMult) + sprintSpringLeft.rotationOffset.x + jumpSpringLeft.rotationOffset.x,
				(cameraSpringLeft.rotationOffset.y * cameraAirBlend * equipBlendFactor * camSimultaneousMult) + (movementSpringLeft.rotationOffset.y * movementAirBlend * equipBlendFactor * movSimultaneousMult) + sprintSpringLeft.rotationOffset.y + jumpSpringLeft.rotationOffset.y,
				(cameraSpringLeft.rotationOffset.z * cameraAirBlend * equipBlendFactor * camSimultaneousMult) + (movementSpringLeft.rotationOffset.z * movementAirBlend * equipBlendFactor * movSimultaneousMult) + sprintSpringLeft.rotationOffset.z + jumpSpringLeft.rotationOffset.z
			};
		}
		
		// *** STORE OFFSETS FOR DEFERRED APPLICATION ***
		// Frame-gen compatible: Store computed offsets for application in UpdateFirstPerson hook
		// This ensures offsets are applied AFTER the game's animation system updates
		
		deferredOffsets.hasOffsets = true;
		deferredOffsets.useDualClaviclePivot = useDualClaviclePivot;
		deferredOffsets.combinedState = combinedState;
		deferredOffsets.combinedStateLeft = combinedStateLeft;
		deferredOffsets.settings = primarySettings;
		
		// Debug logging (offset computation, not application)
		if (settings->debugLogging && debugFrameCounter == 1) {
			if (useDualClaviclePivot) {
				const char* pivotModeName = (primarySettings.pivotPoint == 5) ? "BothClaviclesOffset" : "BothClavicles";
				logger::info("[FPInertia] {} pivot mode active for {}", pivotModeName, isDualWieldMode ? "dual wield" : "other");
			} else {
				const char* pivotNames[] = { "Chest", "RightHand", "LeftHand", "Weapon", "BothClavicles", "BothClaviclesOffset" };
				int pivotIdx = std::clamp(primarySettings.pivotPoint, 0, 5);
				logger::info("[FPInertia] Weapon pivot: {} | invertCamPitch/Yaw: {}/{} | invertMovLat/FB: {}/{}", 
					pivotNames[pivotIdx],
					primarySettings.invertCameraPitch ? "yes" : "no",
					primarySettings.invertCameraYaw ? "yes" : "no",
					primarySettings.invertMovementLateral ? "yes" : "no",
					primarySettings.invertMovementForwardBack ? "yes" : "no");
			}
		}
		
		// Debug logging
		if (settings->debugLogging) {
			if (debugFrameCounter % 30 == 0) {  // Log every 30 frames (~0.5 sec at 60fps)
				const char* nodeType = useDualClaviclePivot ? (primarySettings.pivotPoint == 5 ? "BothClaviclesOffset" : "BothClavicles") : (isTwoHanded ? "Spine" : "Root");
				logger::info("[FPInertia] Node: {} | Delta: {:.4f}s | CamVel: ({:.2f}, {:.2f}, {:.2f}) | PosOff: ({:.3f}, {:.3f}, {:.3f}) | RotOff: ({:.2f}, {:.2f}, {:.2f}) deg",
					nodeType, a_delta,
					smoothedCameraVelocity.x, smoothedCameraVelocity.y, smoothedCameraVelocity.z,
					combinedState.positionOffset.x, combinedState.positionOffset.y, combinedState.positionOffset.z,
					combinedState.rotationOffset.x * RAD_TO_DEG, combinedState.rotationOffset.y * RAD_TO_DEG, combinedState.rotationOffset.z * RAD_TO_DEG);
			}
		}
	}
	
	// Frame-gen compatible: Apply deferred offsets AFTER animations have updated
	// Called from UpdateFirstPerson hook which runs after the game's animation system
	void InertiaManager::OnFirstPersonUpdate(RE::NiAVObject* a_firstPersonObject)
	{
		if (!a_firstPersonObject || !deferredOffsets.hasOffsets) {
			return;
		}
		
		// Skip if game is paused
		auto* ui = RE::UI::GetSingleton();
		if (ui && (ui->GameIsPaused() || ui->numPausesGame > 0)) {
			return;
		}
		
		auto* fpNode = a_firstPersonObject->AsNode();
		if (!fpNode) {
			return;
		}
		
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}
		
		const auto& combinedState = deferredOffsets.combinedState;
		const auto& combinedStateLeft = deferredOffsets.combinedStateLeft;
		const auto& primarySettings = deferredOffsets.settings;

		auto* settings = Settings::GetSingleton();
		
		// Check if offsets are significant enough to apply
		// Skip entirely if offsets are negligible - this preserves particle effects at idle
		float posMag = std::sqrt(
			combinedState.positionOffset.x * combinedState.positionOffset.x +
			combinedState.positionOffset.y * combinedState.positionOffset.y +
			combinedState.positionOffset.z * combinedState.positionOffset.z);
		float rotMag = std::sqrt(
			combinedState.rotationOffset.x * combinedState.rotationOffset.x +
			combinedState.rotationOffset.y * combinedState.rotationOffset.y +
			combinedState.rotationOffset.z * combinedState.rotationOffset.z);
		
		constexpr float MIN_OFFSET_THRESHOLD = 0.01f;
		constexpr float MIN_ROT_THRESHOLD = 0.0001f;
		
		if (posMag < MIN_OFFSET_THRESHOLD && rotMag < MIN_ROT_THRESHOLD) {
			// Offsets are negligible, skip modification to preserve effects
			deferredOffsets.hasOffsets = false;
			return;
		}
		
		// Apply inertia to spine/clavicle local transforms only
		// The engine's Update() will handle propagation (called after this in the hook)
		// This ensures correct motion vectors since we modify BEFORE the engine's Update()

		if (deferredOffsets.useDualClaviclePivot) {
			// Dual clavicle pivot (4 or 5): Apply to both clavicle nodes independently
			RE::NiNode* rightClavicleNode = GetClavicleNode(fpNode, Hand::kRight);
			RE::NiNode* leftClavicleNode = GetClavicleNode(fpNode, Hand::kLeft);

			if (rightClavicleNode) {
				ApplyOffset(rightClavicleNode, combinedState, primarySettings, Hand::kRight);
				lastTargetNode = rightClavicleNode;
			}

			if (leftClavicleNode) {
				ApplyOffset(leftClavicleNode, combinedStateLeft, primarySettings, Hand::kLeft);
			}
		} else {
			// Standard pivot: Apply to spine node (or other single node)
			RE::NiNode* targetNode = GetPivotNode(fpNode, player);

			if (!targetNode) {
				deferredOffsets.hasOffsets = false;
				return;
			}

			lastTargetNode = targetNode;
			logger::info("[FPInertia] Applying skeleton inertia - Node: '{}', Position: ({:.3f}, {:.3f}, {:.3f}), Rotation: ({:.3f}, {:.3f}, {:.3f})",
				targetNode->name.c_str(),
				combinedState.positionOffset.x, combinedState.positionOffset.y, combinedState.positionOffset.z,
				combinedState.rotationOffset.x, combinedState.rotationOffset.y, combinedState.rotationOffset.z);
			ApplyOffset(targetNode, combinedState, primarySettings);
		}
		
		// Selective transform propagation per FP Inertia reference implementation:
		// - Skip ONLY: MagicEffectsNode (exact match), Particle, -Emitter nodes
		// - Do NOT skip NPC R MagicNode [RMag] - it's a skeleton bone for crossbow bolts
		// - Update all skeleton bones, equipment, and geometry children
		// - Use NiAVObject* not NiNode* to include geometry objects (weapons, Arrow:0, etc.)
		RE::NiUpdateData updateData{};
		
		std::function<void(RE::NiAVObject*)> selectivePropagation = [&](RE::NiAVObject* obj) {
			if (!obj) return;
			
			// Update this object's world transform
			obj->UpdateWorldData(&updateData);
			
			// Recurse into children if this is a node
			auto* node = obj->AsNode();
			if (!node) return;
			
			auto& children = node->GetChildren();
			for (std::uint16_t i = 0; i < children.capacity(); ++i) {
				auto& child = children[i];
				if (!child) continue;
				
				std::string_view name = child->name.c_str();
				
				// Skip ONLY these problematic effect containers:
				// - MagicEffectsNode (exact match) - spell effects container
				// - Nodes containing "Particle" - particle systems
				// - Nodes ending in "-Emitter" - particle emitters
				bool shouldSkip = (name == "MagicEffectsNode") ||
				                  (name.find("Particle") != std::string_view::npos) ||
				                  (name.length() > 8 && name.substr(name.length() - 8) == "-Emitter");
				
				if (shouldSkip) {
					// Skip this node - let game handle it to preserve effects
					continue;
				}
				
				// Update all other nodes:
				// - Skeleton bones (NPC *, CME *)
				// - Equipment (WEAPON, SHIELD, AnimObject*)
				// - NPC R MagicNode [RMag] - skeleton attachment for crossbow bolts
				// - Geometry children (Arrow:0, weapon meshes, etc.)
				selectivePropagation(child.get());
			}
		};
		
		// Apply selective propagation from the target node
		if (deferredOffsets.useDualClaviclePivot) {
			RE::NiNode* rightClavicleNode = GetClavicleNode(fpNode, Hand::kRight);
			RE::NiNode* leftClavicleNode = GetClavicleNode(fpNode, Hand::kLeft);
			if (rightClavicleNode) selectivePropagation(rightClavicleNode);
			if (leftClavicleNode) selectivePropagation(leftClavicleNode);
		} else if (lastTargetNode) {
			selectivePropagation(lastTargetNode);
		}

		// Log the final skeleton inertia values for reference
		logger::info("[FPInertia] FINAL: Skeleton inertia applied - Position: ({:.3f}, {:.3f}, {:.3f}), Rotation: ({:.3f}, {:.3f}, {:.3f})",
			combinedState.positionOffset.x, combinedState.positionOffset.y, combinedState.positionOffset.z,
			combinedState.rotationOffset.x, combinedState.rotationOffset.y, combinedState.rotationOffset.z);

		// Log first successful update
	static bool loggedFirstUpdate = false;
		if (!loggedFirstUpdate && lastTargetNode) {
			const char* nodeType = deferredOffsets.useDualClaviclePivot ?
				(primarySettings.pivotPoint == 5 ? "BothClaviclesOffset" : "BothClavicles") : "Spine";
			logger::info("[FPInertia] First FP update hook application! Target: {} ({})",
				nodeType, lastTargetNode->name.c_str());
			loggedFirstUpdate = true;
		}
		
		// Clear the deferred offsets (they've been applied)
		deferredOffsets.hasOffsets = false;
	}

	void InertiaManager::Reset()
	{
		// Just reset our state - game's animation system will reset transforms naturally
		cameraSpring.Reset();
		movementSpring.Reset();
		sprintSpring.Reset();
		smoothedCameraVelocity = { 0.0f, 0.0f, 0.0f };
		prevCameraVelocity = { 0.0f, 0.0f, 0.0f };
		initialized = false;
		lastTargetNode = nullptr;
		settlingFactor = 0.0f;
		timeSinceMovement = 0.0f;
		actionBlendFactor = 1.0f;
		wasInAction = false;
		
		// Reset equip blend
		equipBlendFactor = 0.0f;
		wasWeaponDrawn = false;
		
		// Reset movement tracking
		smoothedLocalMovement = { 0.0f, 0.0f, 0.0f };
		
		// Reset sprint state
		isSprinting = false;
		wasSprinting = false;
		sprintImpulseBlendProgress = 0.0f;
		sprintImpulseBlendDuration = 0.0f;
		sprintPendingPosImpulse = {0,0,0};
		sprintPendingRotImpulse = {0,0,0};
		
		// Reset jump state
		isInAir = false;
		wasInAir = false;
		didJump = false;
		airTime = 0.0f;
		landingCooldown = 0.0f;
		movementAirBlend = 1.0f;
		cameraAirBlend = 1.0f;
		jumpSpring.Reset();
		currentJumpStiffness = 40.0f;
		currentJumpDamping = 3.0f;
		
		// Reset left hand springs (for dual clavicle pivot)
		cameraSpringLeft.Reset();
		movementSpringLeft.Reset();
		sprintSpringLeft.Reset();
		jumpSpringLeft.Reset();
		isDualWieldMode = false;
		currentDualWieldType = WeaponType::Unarmed;
		
		// Reset master enable tracking
		wasInertiaDisabled = false;
		
		// Reset cached values (will be refreshed on next update)
		cachedWeaponFormID = 0;
		cachedWeaponSettings = nullptr;
		cachedSettingsVersion = 0;
		cachedSpineNode = nullptr;
		
		// Reset deferred offsets
		deferredOffsets.hasOffsets = false;
	}

	void InertiaManager::OnEnterFirstPerson()
	{
		isInFirstPerson = true;
		cameraSpring.Reset();
		movementSpring.Reset();
		sprintSpring.Reset();
		smoothedCameraVelocity = { 0.0f, 0.0f, 0.0f };
		prevCameraVelocity = { 0.0f, 0.0f, 0.0f };
		initialized = false;
		lastTargetNode = nullptr;
		debugFrameCounter = 0;
		hasLoggedSkeleton = false;  // Reset so we log fresh skeleton on next enter
		settlingFactor = 0.0f;
		timeSinceMovement = 0.0f;
		actionBlendFactor = 1.0f;
		wasInAction = false;
		
		// Reset movement tracking
		smoothedLocalMovement = { 0.0f, 0.0f, 0.0f };
		
		// Reset sprint state
		isSprinting = false;
		wasSprinting = false;
		sprintImpulseBlendProgress = 0.0f;
		sprintImpulseBlendDuration = 0.0f;
		sprintPendingPosImpulse = {0,0,0};
		sprintPendingRotImpulse = {0,0,0};
		
		// Reset jump state
		isInAir = false;
		wasInAir = false;
		didJump = false;
		airTime = 0.0f;
		landingCooldown = 0.0f;
		movementAirBlend = 1.0f;
		cameraAirBlend = 1.0f;
		jumpSpring.Reset();
		currentJumpStiffness = 40.0f;
		currentJumpDamping = 3.0f;
		
		// Reset left hand springs (for dual clavicle pivot)
		cameraSpringLeft.Reset();
		movementSpringLeft.Reset();
		sprintSpringLeft.Reset();
		jumpSpringLeft.Reset();
		isDualWieldMode = false;
		currentDualWieldType = WeaponType::Unarmed;

		auto* settings = Settings::GetSingleton();
		logger::info("[FPInertia] Entered first person - inertia system active");
		if (settings->debugLogging) {
			logger::info("[FPInertia] Settings: enabled={}, pos={}, rot={}, intensity={:.2f}",
				settings->enabled, settings->enablePosition, settings->enableRotation,
				settings->globalIntensity);
			logger::info("[FPInertia] Per-weapon settings loaded (pivot, invert, spring params)");
		}
	}

	void InertiaManager::OnExitFirstPerson()
	{
		isInFirstPerson = false;
		
		// Just reset state - game handles transform cleanup naturally
		cameraSpring.Reset();
		movementSpring.Reset();
		sprintSpring.Reset();
		smoothedCameraVelocity = { 0.0f, 0.0f, 0.0f };
		prevCameraVelocity = { 0.0f, 0.0f, 0.0f };
		initialized = false;
		lastTargetNode = nullptr;
		
		// Reset movement tracking
		smoothedLocalMovement = { 0.0f, 0.0f, 0.0f };
		
		// Reset sprint state
		isSprinting = false;
		wasSprinting = false;
		sprintImpulseBlendProgress = 0.0f;
		sprintImpulseBlendDuration = 0.0f;
		sprintPendingPosImpulse = {0,0,0};
		sprintPendingRotImpulse = {0,0,0};
		
		// Reset jump state
		isInAir = false;
		wasInAir = false;
		didJump = false;
		airTime = 0.0f;
		landingCooldown = 0.0f;
		movementAirBlend = 1.0f;
		cameraAirBlend = 1.0f;
		jumpSpring.Reset();
		currentJumpStiffness = 40.0f;
		currentJumpDamping = 3.0f;
		
		// Reset left hand springs (for dual clavicle pivot)
		cameraSpringLeft.Reset();
		movementSpringLeft.Reset();
		sprintSpringLeft.Reset();
		jumpSpringLeft.Reset();
		isDualWieldMode = false;
		currentDualWieldType = WeaponType::Unarmed;
		
		// Reset cached values (skeleton may change)
		cachedWeaponFormID = 0;
		cachedWeaponSettings = nullptr;
		cachedSettingsVersion = 0;
		cachedSpineNode = nullptr;
		
		// Reset deferred offsets
		deferredOffsets.hasOffsets = false;
		
		logger::info("[FPInertia] Exited first person - inertia system deactivated");
	}

	// === STANCE DETECTION IMPLEMENTATION ===
	
	void InertiaManager::InitStances()
	{
		auto* settings = Settings::GetSingleton();
		if (!settings->enableStanceSupport) {
			stancesInitialized = true;
			return;
		}
		
		// Only initialize after a save has been loaded
		if (!saveLoaded) {
			logger::debug("[FPInertia] Stances init deferred - waiting for save load");
			return;
		}
		
		// If already initialized and any stance forms found, don't re-init
		bool hasStancesNG = stanceHighEffect || stanceMidEffect || stanceLowEffect;
		bool hasDWM = dwmHighPerk || dwmMidPerk || dwmLowPerk;
		if (stancesInitialized && (hasStancesNG || hasDWM)) {
			return;
		}
		
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			logger::error("[FPInertia] TESDataHandler not available for stance mod lookup");
			return;
		}
		
		// ========================
		// Stances NG detection (magic effects)
		// ========================
		// According to mod documentation:
		// - Bear Stance (High): FormID 0x803
		// - Wolf Stance (Mid): FormID 0x805  
		// - Hawk Stance (Low): FormID 0x806
		
		constexpr RE::FormID kBearStanceFormID = 0x803;  // High
		constexpr RE::FormID kWolfStanceFormID = 0x805;  // Mid
		constexpr RE::FormID kHawkStanceFormID = 0x806;  // Low
		constexpr const char* kStancesNGPlugin = "StancesNG.esp";
		
		// Look up magic effects by FormID
		auto* highForm = dataHandler->LookupForm(kBearStanceFormID, kStancesNGPlugin);
		auto* midForm = dataHandler->LookupForm(kWolfStanceFormID, kStancesNGPlugin);
		auto* lowForm = dataHandler->LookupForm(kHawkStanceFormID, kStancesNGPlugin);
		
		// Cast to EffectSetting (magic effect)
		if (highForm) {
			stanceHighEffect = highForm->As<RE::EffectSetting>();
		}
		if (midForm) {
			stanceMidEffect = midForm->As<RE::EffectSetting>();
		}
		if (lowForm) {
			stanceLowEffect = lowForm->As<RE::EffectSetting>();
		}
		
		// ========================
		// Dynamic Weapon Movesets detection (perks)
		// ========================
		// FormIDs from "Stances - Dynamic Weapon Movesets SE.esp":
		// - High Stance: 0x42518
		// - Mid Stance: 0x42519
		// - Low Stance: 0x4251A
		
		constexpr RE::FormID kDWMHighPerkFormID = 0x42518;
		constexpr RE::FormID kDWMMidPerkFormID = 0x42519;
		constexpr RE::FormID kDWMLowPerkFormID = 0x4251A;
		constexpr const char* kDWMPlugin = "Stances - Dynamic Weapon Movesets SE.esp";
		
		auto* dwmHighForm = dataHandler->LookupForm(kDWMHighPerkFormID, kDWMPlugin);
		auto* dwmMidForm = dataHandler->LookupForm(kDWMMidPerkFormID, kDWMPlugin);
		auto* dwmLowForm = dataHandler->LookupForm(kDWMLowPerkFormID, kDWMPlugin);
		
		// Cast to BGSPerk
		if (dwmHighForm) {
			dwmHighPerk = dwmHighForm->As<RE::BGSPerk>();
		}
		if (dwmMidForm) {
			dwmMidPerk = dwmMidForm->As<RE::BGSPerk>();
		}
		if (dwmLowForm) {
			dwmLowPerk = dwmLowForm->As<RE::BGSPerk>();
		}
		
		stancesInitialized = true;
		
		// Log Stances NG results
		logger::info("[FPInertia] Stances NG magic effect detection (from {}):", kStancesNGPlugin);
		logger::info("[FPInertia]   Bear/High (0x{:03X}): {} (FormID: 0x{:08X})", 
			kBearStanceFormID,
			stanceHighEffect ? "Found" : "Not found",
			stanceHighEffect ? stanceHighEffect->GetFormID() : 0);
		logger::info("[FPInertia]   Wolf/Mid (0x{:03X}): {} (FormID: 0x{:08X})", 
			kWolfStanceFormID,
			stanceMidEffect ? "Found" : "Not found",
			stanceMidEffect ? stanceMidEffect->GetFormID() : 0);
		logger::info("[FPInertia]   Hawk/Low (0x{:03X}): {} (FormID: 0x{:08X})", 
			kHawkStanceFormID,
			stanceLowEffect ? "Found" : "Not found",
			stanceLowEffect ? stanceLowEffect->GetFormID() : 0);
		
		// Log Dynamic Weapon Movesets results
		logger::info("[FPInertia] Dynamic Weapon Movesets perk detection (from {}):", kDWMPlugin);
		logger::info("[FPInertia]   High (0x{:05X}): {} (FormID: 0x{:08X})", 
			kDWMHighPerkFormID,
			dwmHighPerk ? "Found" : "Not found",
			dwmHighPerk ? dwmHighPerk->GetFormID() : 0);
		logger::info("[FPInertia]   Mid (0x{:05X}): {} (FormID: 0x{:08X})", 
			kDWMMidPerkFormID,
			dwmMidPerk ? "Found" : "Not found",
			dwmMidPerk ? dwmMidPerk->GetFormID() : 0);
		logger::info("[FPInertia]   Low (0x{:05X}): {} (FormID: 0x{:08X})", 
			kDWMLowPerkFormID,
			dwmLowPerk ? "Found" : "Not found",
			dwmLowPerk ? dwmLowPerk->GetFormID() : 0);
		
		bool foundStancesNG = stanceHighEffect || stanceMidEffect || stanceLowEffect;
		bool foundDWM = dwmHighPerk || dwmMidPerk || dwmLowPerk;
		
		// Update settings to reflect detection
		settings->stancesNGInstalled = foundStancesNG || foundDWM;
		
		if (!foundStancesNG && !foundDWM) {
			logger::info("[FPInertia] No stance mods detected - stance multipliers will not be applied");
		} else {
			if (foundStancesNG) {
				logger::info("[FPInertia] Stances NG stance detection active");
			}
			if (foundDWM) {
				logger::info("[FPInertia] Dynamic Weapon Movesets stance detection active");
			}
		}
	}
	
	bool InertiaManager::IsStancesAvailable() const
	{
		auto* settings = Settings::GetSingleton();
		return settings->stancesNGInstalled && settings->enableStanceSupport;
	}
	
	Stance InertiaManager::GetCurrentStance() const
	{
		auto* settings = Settings::GetSingleton();
		if (!settings->enableStanceSupport) {
			return Stance::Neutral;
		}
		
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return Stance::Neutral;
		}
		
		// Re-try initialization if no stance forms were found (save may not have been loaded)
		bool hasStancesNG = stanceHighEffect || stanceMidEffect || stanceLowEffect;
		bool hasDWM = dwmHighPerk || dwmMidPerk || dwmLowPerk;
		if (!hasStancesNG && !hasDWM) {
			const_cast<InertiaManager*>(this)->InitStances();
		}
		
		// ========================
		// Check Stances NG magic effects (primary method)
		// ========================
		auto* magicTarget = player->AsMagicTarget();
		if (magicTarget) {
			if (stanceHighEffect && magicTarget->HasMagicEffect(stanceHighEffect)) {
				return Stance::High;
			}
			if (stanceMidEffect && magicTarget->HasMagicEffect(stanceMidEffect)) {
				return Stance::Mid;
			}
			if (stanceLowEffect && magicTarget->HasMagicEffect(stanceLowEffect)) {
				return Stance::Low;
			}
		}
		
		// ========================
		// Check Dynamic Weapon Movesets perks
		// ========================
		if (dwmHighPerk && player->HasPerk(dwmHighPerk)) {
			return Stance::High;
		}
		if (dwmMidPerk && player->HasPerk(dwmMidPerk)) {
			return Stance::Mid;
		}
		if (dwmLowPerk && player->HasPerk(dwmLowPerk)) {
			return Stance::Low;
		}
		
		return Stance::Neutral;
	}
	
	void InertiaManager::OnSaveLoaded()
	{
		logger::info("[FPInertia] Save loaded - initializing stance detection");
		saveLoaded = true;
		
		// Reset stance detection state for re-initialization
		stanceHighEffect = nullptr;
		stanceMidEffect = nullptr;
		stanceLowEffect = nullptr;
		dwmHighPerk = nullptr;
		dwmMidPerk = nullptr;
		dwmLowPerk = nullptr;
		stancesInitialized = false;
		
		InitStances();
	}



	// Main update hook - calculates spring physics and stores deferred offsets
	namespace Hook
	{
		class MainUpdateHook
		{
		public:
			static void Install()
			{
				auto& trampoline = SKSE::GetTrampoline();
				
				REL::Relocation<std::uintptr_t> hook{ RELOCATION_ID(35565, 36564) };  // Main update
				
				logger::info("[FPInertia] Main update hook address base: {:X}", hook.address());
				logger::info("[FPInertia] Main update offset: SE={:X}, AE={:X}", 0x748, 0xC26);
				
				_originalUpdate = trampoline.write_call<5>(hook.address() + RELOCATION_OFFSET(0x748, 0xC26), OnUpdate);
				
				logger::info("[FPInertia] Installed main update hook successfully");
			}

		private:
			static void OnUpdate()
			{
				// Call original first - this ensures game state is updated before we modify it
				_originalUpdate();
				
				// Rate limit updates to prevent issues with frame generation
				// Frame gen can cause our hook to be called multiple times per actual frame
				static auto lastUpdateTime = std::chrono::steady_clock::now();
				auto currentTime = std::chrono::steady_clock::now();
				float timeSinceLastUpdate = std::chrono::duration<float>(currentTime - lastUpdateTime).count();
				
				// Minimum 5ms between updates (200fps cap) to avoid frame gen issues
				constexpr float MIN_UPDATE_INTERVAL = 0.005f;
				if (timeSinceLastUpdate < MIN_UPDATE_INTERVAL) {
					return; // Skip this update, too soon after the last one
				}
				
				// Use the actual time since last update as delta
				float delta = timeSinceLastUpdate;
				lastUpdateTime = currentTime;
				
				// Clamp to reasonable values
				delta = std::clamp(delta, MIN_UPDATE_INTERVAL, 0.1f);
				
				// Check for INI hot reload
				Settings::GetSingleton()->CheckForReload(delta);
				
				// Update inertia - calculates spring physics and stores deferred offsets
				InertiaManager::GetSingleton()->Update(delta);
			}

			static inline REL::Relocation<decltype(OnUpdate)> _originalUpdate;
		};
		
		// UpdateFirstPerson hook - applies deferred offsets AFTER animations
		// This hook runs after the game's animation system, making it frame-gen compatible
		// Same hook point used by ImprovedCameraSE-NG
		class UpdateFirstPersonHook
		{
		public:
			static void Install()
			{
				auto& trampoline = SKSE::GetTrampoline();
				
				// Hook UpdateFirstPerson at REL::RelocationID(39446, 40522) + 0xD7
				// This is after the game's animation update, so our changes persist
				REL::Relocation<std::uintptr_t> hook{ RELOCATION_ID(39446, 40522) };
				
				logger::info("[FPInertia] UpdateFirstPerson hook address base: {:X}", hook.address());
				logger::info("[FPInertia] UpdateFirstPerson offset: 0xD7");
				
				_originalFunc = trampoline.write_call<5>(hook.address() + 0xD7, OnFirstPersonUpdate);
				
				logger::info("[FPInertia] Installed UpdateFirstPerson hook successfully (frame-gen compatible)");
			}
			
		private:
			static void OnFirstPersonUpdate(RE::NiAVObject* a_firstPersonObject, RE::NiUpdateData* a_updateData)
			{
				// CORRECT HOOK ORDER (per ImprovedCameraSE-NG):
				// 1. Apply our local transform modifications BEFORE calling original
				// 2. Call original - engine's Update() will propagate with our offsets
				// 3. No need to call Update() ourselves - engine handles everything including previousWorld
				
				// Apply our deferred offsets BEFORE the engine's update
				// This ensures the engine's Update() propagates our modified local transforms
				// and correctly handles previousWorld for motion vectors
				InertiaManager::GetSingleton()->OnFirstPersonUpdate(a_firstPersonObject);
				
				// Now call the original - engine propagates transforms with our modifications
				_originalFunc(a_firstPersonObject, a_updateData);
			}
			
			static inline REL::Relocation<decltype(OnFirstPersonUpdate)> _originalFunc;
		};
	}

	void Install()
	{
		// Allocate trampoline space for both hooks
		SKSE::GetTrampoline().create(128);
		
		Hook::MainUpdateHook::Install();
		Hook::UpdateFirstPersonHook::Install();
		logger::info("FP Inertia system installed (frame-gen compatible)");
	}
}

