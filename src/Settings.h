#pragma once

// Pivot point options for where inertia is applied
enum class PivotPoint : int
{
	kChest = 0,      // Spine2 - current behavior
	kRightHand = 1,  // Right hand/weapon hand
	kLeftHand = 2,   // Left hand/off-hand
	kWeapon = 3      // Weapon node itself
};

// Weapon type enum (extends RE::WEAPON_TYPE with additional categories)
enum class WeaponType : int
{
	Unarmed = 0,
	OneHandSword = 1,
	OneHandDagger = 2,
	OneHandAxe = 3,
	OneHandMace = 4,
	TwoHandSword = 5,
	TwoHandAxe = 6,
	Bow = 7,
	Staff = 8,
	Crossbow = 9,
	Shield = 10,
	Spell = 11,
	// Dual wield types (detected with priority)
	DualWieldWeapons = 12,
	DualWieldMagic = 13,
	SpellAndWeapon = 14
};

// Weapon type inertia settings - ALL settings are per-weapon
struct WeaponInertiaSettings
{
	// === MASTER TOGGLE ===
	bool  enabled{ true };           // Enable/disable inertia for this weapon type
	
	// === CAMERA INERTIA (responds to looking around) ===
	float stiffness{ 150.0f };       // Spring stiffness (higher = faster return)
	float damping{ 12.0f };          // Damping coefficient (higher = less oscillation)
	float maxOffset{ 8.0f };         // Maximum position offset in units
	float maxRotation{ 15.0f };      // Maximum rotation offset in degrees
	float mass{ 1.0f };              // Virtual mass for inertia feel
	float pitchMultiplier{ 1.0f };   // Multiplier for pitch ROTATION (up/down tilt)
	float rollMultiplier{ 1.0f };    // Multiplier for roll (wavy motion)
	float cameraPitchMult{ 1.0f };   // Global multiplier for ALL camera pitch effects (position + rotation)
	bool  invertCameraPitch{ false };  // Invert pitch (up/down) camera inertia
	bool  invertCameraYaw{ false };    // Invert yaw (left/right) camera inertia
	
	// === MOVEMENT INERTIA (responds to strafing) ===
	bool  movementInertiaEnabled{ true };    // Enable movement inertia for this weapon type
	float movementStiffness{ 80.0f };        // Spring stiffness for movement
	float movementDamping{ 6.0f };           // Damping for movement spring
	float movementMaxOffset{ 12.0f };        // Maximum position offset from movement
	float movementMaxRotation{ 20.0f };      // Maximum rotation from movement (degrees)
	float movementLeftMult{ 1.0f };          // Multiplier for leftward movement offset
	float movementRightMult{ 1.0f };         // Multiplier for rightward movement offset
	float movementForwardMult{ 0.5f };       // Multiplier for forward movement offset
	float movementBackwardMult{ 0.5f };      // Multiplier for backward movement offset
	bool  invertMovementLateral{ false };      // Invert lateral (left/right) movement inertia
	bool  invertMovementForwardBack{ false };  // Invert forward/back movement inertia
	
	// === SIMULTANEOUS BLEND SCALING ===
	float simultaneousThreshold{ 0.5f };      // Threshold before simultaneous scaling kicks in
	float simultaneousCameraMult{ 1.0f };     // Camera inertia multiplier when both active
	float simultaneousMovementMult{ 1.0f };   // Movement inertia multiplier when both active
	
	// === SPRINT INERTIA ===
	bool  sprintInertiaEnabled{ false };     // Enable sprint impulse inertia
	float sprintImpulseY{ 8.0f };            // Depth impulse when starting/stopping sprint
	float sprintImpulseZ{ 3.0f };            // Height impulse when starting/stopping sprint
	float sprintRotImpulse{ 5.0f };          // Rotation impulse (degrees)
	float sprintImpulseBlendTime{ 0.1f };    // Time to blend in/out sprint impulse
	float sprintStiffness{ 60.0f };          // Spring stiffness for sprint spring
	float sprintDamping{ 5.0f };             // Damping for sprint spring
	
	// === JUMP/LAND INERTIA ===
	bool  jumpInertiaEnabled{ true };        // Enable jump/land inertia
	float cameraInertiaAirMult{ 0.3f };      // Camera inertia multiplier while airborne
	float jumpImpulseY{ 4.0f };              // Y impulse when jumping
	float jumpImpulseZ{ 6.0f };              // Z impulse when jumping
	float jumpRotImpulse{ 3.0f };            // Rotation impulse when jumping (degrees)
	float jumpStiffness{ 40.0f };            // Spring stiffness while airborne
	float jumpDamping{ 3.0f };               // Damping while airborne
	float landImpulseY{ 3.0f };              // Y impulse when landing
	float landImpulseZ{ 10.0f };             // Z impulse when landing
	float landRotImpulse{ 5.0f };            // Rotation impulse when landing (degrees)
	float landStiffness{ 120.0f };           // Spring stiffness for landing recovery
	float landDamping{ 10.0f };              // Damping for landing recovery
	float airTimeImpulseScale{ 1.5f };       // Scale landing impulse by air time
	
	// === PIVOT POINT ===
	int pivotPoint{ 0 };  // 0=Chest, 1=RightHand, 2=LeftHand, 3=Weapon
	
	void Load(CSimpleIniA& a_ini, const char* a_section);
	void Save(CSimpleIniA& a_ini, const char* a_section) const;
};

class Settings
{
public:
	static Settings* GetSingleton()
	{
		static Settings singleton;
		return &singleton;
	}

	void Load();
	void Save();  // Save current settings to INI file
	void CheckForReload(float a_deltaTime);  // Call periodically to check for INI changes
	void DetectCommunityShaders();  // Check if Community Shaders is installed
	
	// Get settings for a specific weapon type
	const WeaponInertiaSettings& GetWeaponSettings(RE::WEAPON_TYPE a_type) const;
	const WeaponInertiaSettings& GetWeaponSettings(WeaponType a_type) const;
	WeaponInertiaSettings& GetWeaponSettingsMutable(WeaponType a_type);
	
	// Convert RE::WEAPON_TYPE to our WeaponType enum
	static WeaponType ToWeaponType(RE::WEAPON_TYPE a_type);

	// General settings
	bool  enabled{ true };
	bool  enablePosition{ true };     // Enable position offset
	bool  enableRotation{ true };     // Enable rotation offset
	bool  requireWeaponDrawn{ true }; // Only apply inertia when weapon is drawn
	float globalIntensity{ 1.0f };    // Global intensity multiplier
	float smoothingFactor{ 0.5f };    // Smoothing for camera velocity (0-1)
	
	// Settling behavior - spring dampens over time when camera stops
	float settleDelay{ 0.3f };        // Seconds before settling starts (0 = immediate)
	float settleSpeed{ 2.0f };        // How fast settling increases (higher = faster settle)
	float settleDampingMult{ 3.0f };  // Max damping multiplier when fully settled
	
	// Movement inertia global settings
	bool  movementInertiaEnabled{ true };     // Master enable for movement-based arm sway
	float movementInertiaStrength{ 3.0f };    // Global strength multiplier
	float movementInertiaThreshold{ 30.0f };  // Minimum movement speed (units/sec)
	bool  forwardBackInertia{ false };        // Enable inertia for forward/back movement (default: OFF)
	bool  disableVanillaSway{ false };        // Disable game's built-in walk/run arm sway
	
	// Action blending - smooth inertia reduction during certain actions
	bool  blendDuringAttack{ true };     // Reduce inertia during melee attacks
	bool  blendDuringBowDraw{ true };    // Reduce inertia while drawing bow
	bool  blendDuringSpellCast{ true };  // Reduce inertia during spell casting
	float actionBlendSpeed{ 5.0f };      // How fast to blend in/out (higher = faster)
	float actionMinIntensity{ 0.2f };    // Minimum intensity during actions (0 = fully disabled)
	
	// Different settings for each hand (for dual wielding)
	bool  independentHands{ true };   // Process each hand separately
	float leftHandMultiplier{ 1.0f }; // Multiplier for left hand inertia
	float rightHandMultiplier{ 1.0f }; // Multiplier for right hand inertia
	
	// Frame Generation compatibility
	bool communityShadersDetected{ false };
	bool frameGenCompatMode{ false };
	
	// Debug settings
	bool debugLogging{ false };
	bool debugOnScreen{ false };
	
	// Hot reload settings
	bool  enableHotReload{ true };        // Check for INI changes while game is running
	float hotReloadIntervalSec{ 5.0f };   // How often to check for changes (seconds)

	// Per-weapon-type settings (standard types)
	WeaponInertiaSettings unarmed;
	WeaponInertiaSettings oneHandSword;
	WeaponInertiaSettings oneHandDagger;
	WeaponInertiaSettings oneHandAxe;
	WeaponInertiaSettings oneHandMace;
	WeaponInertiaSettings twoHandSword;
	WeaponInertiaSettings twoHandAxe;
	WeaponInertiaSettings bow;
	WeaponInertiaSettings staff;
	WeaponInertiaSettings crossbow;
	WeaponInertiaSettings shield;     // For shield in left hand
	WeaponInertiaSettings spell;      // For spell in hand (no weapon)
	
	// Per-weapon-type settings (dual wield types)
	WeaponInertiaSettings dualWieldWeapons;
	WeaponInertiaSettings dualWieldMagic;
	WeaponInertiaSettings spellAndWeapon;

private:
	Settings() = default;
	Settings(const Settings&) = delete;
	Settings(Settings&&) = delete;
	~Settings() = default;

	Settings& operator=(const Settings&) = delete;
	Settings& operator=(Settings&&) = delete;

	// Private hot-reload tracking
	std::filesystem::file_time_type lastModifiedTime{};
	float timeSinceLastCheck{ 0.0f };
};
