#pragma once

#include "Settings.h"
#include "InertiaPresets.h"

namespace Inertia
{
	// Spring state for tracking inertia
	struct SpringState
	{
		// Position offset (local space)
		RE::NiPoint3 positionOffset{ 0.0f, 0.0f, 0.0f };
		RE::NiPoint3 positionVelocity{ 0.0f, 0.0f, 0.0f };
		
		// Rotation offset (euler angles in radians)
		RE::NiPoint3 rotationOffset{ 0.0f, 0.0f, 0.0f };
		RE::NiPoint3 rotationVelocity{ 0.0f, 0.0f, 0.0f };
		
		void Reset()
		{
			positionOffset = { 0.0f, 0.0f, 0.0f };
			positionVelocity = { 0.0f, 0.0f, 0.0f };
			rotationOffset = { 0.0f, 0.0f, 0.0f };
			rotationVelocity = { 0.0f, 0.0f, 0.0f };
		}
	};

	// Hand tracking for dual wielding
	enum class Hand
	{
		kRight = 0,
		kLeft = 1,
		kTotal = 2
	};

	class InertiaManager
	{
	public:
		static InertiaManager* GetSingleton()
		{
			static InertiaManager singleton;
			return &singleton;
		}

		void Update(float a_delta);
		void Reset();
		
		// Called when entering/exiting first person
		void OnEnterFirstPerson();
		void OnExitFirstPerson();
		
		// Frame-gen compatible: Apply deferred offsets (called from UpdateFirstPerson hook)
		void OnFirstPersonUpdate(RE::NiAVObject* a_firstPersonObject);

	private:
		InertiaManager() = default;
		~InertiaManager() = default;
		InertiaManager(const InertiaManager&) = delete;
		InertiaManager(InertiaManager&&) = delete;
		InertiaManager& operator=(const InertiaManager&) = delete;
		InertiaManager& operator=(InertiaManager&&) = delete;

		// Get the first person arm/hand node
		RE::NiNode* GetFirstPersonNode();
		
		// Get weapon type for a hand
		RE::WEAPON_TYPE GetWeaponTypeForHand(RE::PlayerCharacter* a_player, Hand a_hand);
		
		// Get equipped weapon's EditorID for a hand (empty string if none)
		std::string GetEquippedWeaponEditorID(RE::PlayerCharacter* a_player, Hand a_hand);
		
		// Get weapon settings using preset system (checks specific weapon first, then type)
		const WeaponInertiaSettings& GetCurrentWeaponSettings(RE::PlayerCharacter* a_player, Hand a_hand);
		
		// Check if hand has a shield
		bool HasShield(RE::PlayerCharacter* a_player, Hand a_hand);
		
		// Check if hand has a bound weapon equipped
		bool HasBoundWeapon(RE::PlayerCharacter* a_player, Hand a_hand);
		
		// Check if hand has a spell equipped (not a weapon or shield)
		bool HasSpell(RE::PlayerCharacter* a_player, Hand a_hand);
		
		// Check if hand has any weapon (including bound weapons)
		bool HasWeapon(RE::PlayerCharacter* a_player, Hand a_hand);
		
		// Detect dual wield type with priority (returns Unarmed if not dual wielding)
		WeaponType DetectDualWieldType(RE::PlayerCharacter* a_player);
		
		// Get clavicle node for a specific side (used for dual clavicle pivots 4 and 5)
		RE::NiNode* GetClavicleNode(RE::NiNode* a_fpRoot, Hand a_hand);
		
		// Update spring physics
		void UpdateSpring(SpringState& a_state, const WeaponInertiaSettings& a_settings, 
			const RE::NiPoint3& a_cameraVelocity, float a_delta, float a_multiplier);
		
		// Apply offset to node (a_hand parameter used for pivot 5 side-specific compensation)
		void ApplyOffset(RE::NiNode* a_node, const SpringState& a_state, 
			const WeaponInertiaSettings& a_settings, Hand a_hand = Hand::kRight);
		
		// Calculate camera velocity from rotation changes
		RE::NiPoint3 CalculateCameraVelocity(float a_delta);

		// Spring states - SEPARATE for camera and movement inertia
		// These are ADDITIVE - both contribute to final offset
		SpringState cameraSpring;      // Camera rotation-based inertia (single or right hand)
		SpringState movementSpring;    // Player movement-based inertia (single or right hand)
		
		// Left hand springs - used when pivot is dual clavicle mode (pivot 4 or 5, dual wield types)
		SpringState cameraSpringLeft;    // Left hand camera inertia
		SpringState movementSpringLeft;  // Left hand movement inertia
		SpringState sprintSpringLeft;    // Left hand sprint inertia
		SpringState jumpSpringLeft;      // Left hand jump inertia
		
		// Dual wield tracking
		bool isDualWieldMode{ false };     // True when using dual clavicle pivot (4 or 5)
		WeaponType currentDualWieldType{ WeaponType::Unarmed };  // Which dual wield type if any
		
		// Master enable state tracking (for spring reset on disable)
		bool wasInertiaDisabled{ false };
		
		// Previous camera rotation for velocity calculation
		float lastCameraYaw{ 0.0f };
		float lastCameraPitch{ 0.0f };
		float lastCameraRoll{ 0.0f };
		
		// Smoothed camera velocity
		RE::NiPoint3 smoothedCameraVelocity{ 0.0f, 0.0f, 0.0f };
		
		// State tracking
		bool isInFirstPerson{ false };
		bool initialized{ false };
		
		// Track which node we're modifying (for debug/logging)
		RE::NiNode* lastTargetNode{ nullptr };
		
		// Settling system - increased damping when camera stops moving
		float settlingFactor{ 0.0f };      // 0 = full spring, 1 = heavily damped (settling)
		float timeSinceMovement{ 0.0f };   // How long since camera was moving
		
		// Action blending - reduce intensity during attacks/bow draw/spells
		float actionBlendFactor{ 1.0f };   // 1 = full intensity, 0 = minimum intensity
		bool wasInAction{ false };         // Track previous action state
		
		// Equip/holster blending - smooth transition when drawing/sheathing weapon
		float equipBlendFactor{ 0.0f };    // 0 = sheathed, 1 = fully drawn
		bool wasWeaponDrawn{ false };      // Track previous weapon drawn state
		
		// Movement-based inertia - uses player input directly
		RE::NiPoint3 smoothedLocalMovement{ 0.0f, 0.0f, 0.0f };      // Smoothed local movement velocity
		
		// Sprint transition inertia state
		bool isSprinting{ false };
		bool wasSprinting{ false };
		float sprintImpulseBlendProgress{ 0.0f };  // Current blend progress (0-1)
		float sprintImpulseBlendDuration{ 0.0f };  // Total blend duration for current impulse
		RE::NiPoint3 sprintPendingPosImpulse{ 0.0f, 0.0f, 0.0f };  // Pending position impulse to blend
		RE::NiPoint3 sprintPendingRotImpulse{ 0.0f, 0.0f, 0.0f };  // Pending rotation impulse to blend
		SpringState sprintSpring;          // Sprint transition spring state
		
		// Jump/landing inertia state
		bool isInAir{ false };
		bool wasInAir{ false };
		bool didJump{ false };             // True if player jumped (vs just falling)
		float airTime{ 0.0f };             // Time spent in air (for landing impulse scaling)
		float landingCooldown{ 0.0f };     // Cooldown to prevent multiple landing impulses
		float movementAirBlend{ 1.0f };    // Blend factor for movement inertia (0 = in air, 1 = grounded)
		float cameraAirBlend{ 1.0f };      // Blend factor for camera inertia (blends to cameraInertiaAirMult when in air)
		SpringState jumpSpring;            // Jump/landing spring state
		float currentJumpStiffness{ 40.0f };  // Current spring stiffness (changes on land)
		float currentJumpDamping{ 3.0f };     // Current spring damping (changes on land)
		
		// Frame-rate independence: previous frame velocity for clamping
		RE::NiPoint3 prevCameraVelocity{ 0.0f, 0.0f, 0.0f };
		
		// Current weapon tracking for preset lookup (cached for performance)
		std::string currentWeaponEditorID;
		WeaponType currentWeaponType{ WeaponType::Unarmed };
		
		// Cached weapon settings (refreshed when weapon changes, not every frame)
		RE::FormID cachedWeaponFormID{ 0 };          // FormID of currently equipped weapon (0 if none)
		const WeaponInertiaSettings* cachedWeaponSettings{ nullptr };  // Cached settings pointer
		uint32_t cachedSettingsVersion{ 0 };         // Preset version when cache was built
		
		// Cached spine node (set on first-person enter, avoids string searches every frame)
		RE::NiNode* cachedSpineNode{ nullptr };
		
		// Frame-gen compatible deferred application (computed in Update, applied in OnFirstPersonUpdate)
		struct DeferredOffsets {
			bool hasOffsets{ false };           // True if offsets are ready to apply
			bool useDualClaviclePivot{ false }; // Which pivot mode to use
			SpringState combinedState;          // Right hand / main spine offsets
			SpringState combinedStateLeft;      // Left hand offsets (for dual clavicle)
			WeaponInertiaSettings settings;     // Copy of settings to use for application
		};
		DeferredOffsets deferredOffsets;
		
		// Get target node based on pivot point setting
		RE::NiNode* GetPivotNode(RE::NiNode* a_fpRoot, RE::PlayerCharacter* a_player);
		
		// Debug frame counter
		int debugFrameCounter{ 0 };
		
		// Helper to check if player is in an action that should reduce inertia
		bool IsPlayerInAction(RE::PlayerCharacter* a_player);
		
		// Calculate local movement velocity (relative to camera facing)
		RE::NiPoint3 CalculateLocalMovement(RE::PlayerCharacter* a_player, float a_delta);
	};

	// Install the update hook
	void Install();
}

