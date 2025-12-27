#include "Settings.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

void Settings::DetectCommunityShaders()
{
	// Check if CommunityShaders.dll is loaded
	HMODULE hModule = GetModuleHandleA("CommunityShaders.dll");
	if (hModule != nullptr) {
		communityShadersDetected = true;
		frameGenCompatMode = true;  // Auto-enable when detected
		logger::info("[FPInertia] CommunityShaders.dll detected - Frame Generation Compatible Mode auto-enabled");
	} else {
		communityShadersDetected = false;
		logger::info("[FPInertia] CommunityShaders.dll not detected");
	}
}

void WeaponInertiaSettings::Load(CSimpleIniA& a_ini, const char* a_section)
{
	// === MASTER TOGGLE ===
	enabled = a_ini.GetBoolValue(a_section, "bEnabled", enabled);
	
	// === CAMERA INERTIA ===
	stiffness = static_cast<float>(a_ini.GetDoubleValue(a_section, "fStiffness", stiffness));
	damping = static_cast<float>(a_ini.GetDoubleValue(a_section, "fDamping", damping));
	maxOffset = static_cast<float>(a_ini.GetDoubleValue(a_section, "fMaxOffset", maxOffset));
	maxRotation = static_cast<float>(a_ini.GetDoubleValue(a_section, "fMaxRotation", maxRotation));
	mass = static_cast<float>(a_ini.GetDoubleValue(a_section, "fMass", mass));
	pitchMultiplier = static_cast<float>(a_ini.GetDoubleValue(a_section, "fPitchMultiplier", pitchMultiplier));
	rollMultiplier = static_cast<float>(a_ini.GetDoubleValue(a_section, "fRollMultiplier", rollMultiplier));
	cameraPitchMult = static_cast<float>(a_ini.GetDoubleValue(a_section, "fCameraPitchMult", cameraPitchMult));
	invertCameraPitch = a_ini.GetBoolValue(a_section, "bInvertCameraPitch", invertCameraPitch);
	invertCameraYaw = a_ini.GetBoolValue(a_section, "bInvertCameraYaw", invertCameraYaw);
	// Migration: if old invertCamera was set, apply to both new toggles
	if (a_ini.GetBoolValue(a_section, "bInvertCamera", false)) {
		invertCameraPitch = true;
		invertCameraYaw = true;
	}
	
	// === MOVEMENT INERTIA ===
	movementInertiaEnabled = a_ini.GetBoolValue(a_section, "bMovementInertiaEnabled", movementInertiaEnabled);
	movementStiffness = static_cast<float>(a_ini.GetDoubleValue(a_section, "fMovementStiffness", movementStiffness));
	movementDamping = static_cast<float>(a_ini.GetDoubleValue(a_section, "fMovementDamping", movementDamping));
	movementMaxOffset = static_cast<float>(a_ini.GetDoubleValue(a_section, "fMovementMaxOffset", movementMaxOffset));
	movementMaxRotation = static_cast<float>(a_ini.GetDoubleValue(a_section, "fMovementMaxRotation", movementMaxRotation));
	movementLeftMult = static_cast<float>(a_ini.GetDoubleValue(a_section, "fMovementLeftMult", movementLeftMult));
	movementRightMult = static_cast<float>(a_ini.GetDoubleValue(a_section, "fMovementRightMult", movementRightMult));
	movementForwardMult = static_cast<float>(a_ini.GetDoubleValue(a_section, "fMovementForwardMult", movementForwardMult));
	movementBackwardMult = static_cast<float>(a_ini.GetDoubleValue(a_section, "fMovementBackwardMult", movementBackwardMult));
	invertMovementLateral = a_ini.GetBoolValue(a_section, "bInvertMovementLateral", invertMovementLateral);
	invertMovementForwardBack = a_ini.GetBoolValue(a_section, "bInvertMovementForwardBack", invertMovementForwardBack);
	// Migration: if old invertMovement was set, apply to both new toggles
	if (a_ini.GetBoolValue(a_section, "bInvertMovement", false)) {
		invertMovementLateral = true;
		invertMovementForwardBack = true;
	}
	
	// === SIMULTANEOUS CAMERA+MOVEMENT SCALING ===
	simultaneousThreshold = static_cast<float>(a_ini.GetDoubleValue(a_section, "fSimultaneousThreshold", simultaneousThreshold));
	simultaneousCameraMult = static_cast<float>(a_ini.GetDoubleValue(a_section, "fSimultaneousCameraMult", simultaneousCameraMult));
	simultaneousMovementMult = static_cast<float>(a_ini.GetDoubleValue(a_section, "fSimultaneousMovementMult", simultaneousMovementMult));
	
	// === SPRINT TRANSITION INERTIA ===
	sprintInertiaEnabled = a_ini.GetBoolValue(a_section, "bSprintInertiaEnabled", sprintInertiaEnabled);
	sprintImpulseY = static_cast<float>(a_ini.GetDoubleValue(a_section, "fSprintImpulseY", sprintImpulseY));
	sprintImpulseZ = static_cast<float>(a_ini.GetDoubleValue(a_section, "fSprintImpulseZ", sprintImpulseZ));
	sprintRotImpulse = static_cast<float>(a_ini.GetDoubleValue(a_section, "fSprintRotImpulse", sprintRotImpulse));
	sprintImpulseBlendTime = static_cast<float>(a_ini.GetDoubleValue(a_section, "fSprintImpulseBlendTime", sprintImpulseBlendTime));
	sprintStiffness = static_cast<float>(a_ini.GetDoubleValue(a_section, "fSprintStiffness", sprintStiffness));
	sprintDamping = static_cast<float>(a_ini.GetDoubleValue(a_section, "fSprintDamping", sprintDamping));
	
	// === JUMP/LANDING INERTIA ===
	jumpInertiaEnabled = a_ini.GetBoolValue(a_section, "bJumpInertiaEnabled", jumpInertiaEnabled);
	cameraInertiaAirMult = static_cast<float>(a_ini.GetDoubleValue(a_section, "fCameraInertiaAirMult", cameraInertiaAirMult));
	jumpImpulseY = static_cast<float>(a_ini.GetDoubleValue(a_section, "fJumpImpulseY", jumpImpulseY));
	jumpImpulseZ = static_cast<float>(a_ini.GetDoubleValue(a_section, "fJumpImpulseZ", jumpImpulseZ));
	jumpRotImpulse = static_cast<float>(a_ini.GetDoubleValue(a_section, "fJumpRotImpulse", jumpRotImpulse));
	jumpStiffness = static_cast<float>(a_ini.GetDoubleValue(a_section, "fJumpStiffness", jumpStiffness));
	jumpDamping = static_cast<float>(a_ini.GetDoubleValue(a_section, "fJumpDamping", jumpDamping));
	landImpulseY = static_cast<float>(a_ini.GetDoubleValue(a_section, "fLandImpulseY", landImpulseY));
	landImpulseZ = static_cast<float>(a_ini.GetDoubleValue(a_section, "fLandImpulseZ", landImpulseZ));
	landRotImpulse = static_cast<float>(a_ini.GetDoubleValue(a_section, "fLandRotImpulse", landRotImpulse));
	landStiffness = static_cast<float>(a_ini.GetDoubleValue(a_section, "fLandStiffness", landStiffness));
	landDamping = static_cast<float>(a_ini.GetDoubleValue(a_section, "fLandDamping", landDamping));
	airTimeImpulseScale = static_cast<float>(a_ini.GetDoubleValue(a_section, "fAirTimeImpulseScale", airTimeImpulseScale));
	
	// === PIVOT POINT ===
	pivotPoint = static_cast<int>(a_ini.GetLongValue(a_section, "iPivotPoint", pivotPoint));
	
	// Clamp camera values
	stiffness = std::clamp(stiffness, 10.0f, 1000.0f);
	damping = std::clamp(damping, 1.0f, 100.0f);
	maxOffset = std::clamp(maxOffset, 0.0f, 50.0f);
	maxRotation = std::clamp(maxRotation, 0.0f, 90.0f);
	mass = std::clamp(mass, 0.1f, 10.0f);
	pitchMultiplier = std::clamp(pitchMultiplier, 0.0f, 5.0f);
	rollMultiplier = std::clamp(rollMultiplier, 0.0f, 5.0f);
	cameraPitchMult = std::clamp(cameraPitchMult, 0.0f, 5.0f);
	
	// Clamp movement values
	movementStiffness = std::clamp(movementStiffness, 10.0f, 500.0f);
	movementDamping = std::clamp(movementDamping, 1.0f, 50.0f);
	movementMaxOffset = std::clamp(movementMaxOffset, 0.0f, 50.0f);
	movementMaxRotation = std::clamp(movementMaxRotation, 0.0f, 90.0f);
	movementLeftMult = std::clamp(movementLeftMult, 0.0f, 5.0f);
	movementRightMult = std::clamp(movementRightMult, 0.0f, 5.0f);
	movementForwardMult = std::clamp(movementForwardMult, 0.0f, 5.0f);
	movementBackwardMult = std::clamp(movementBackwardMult, 0.0f, 5.0f);
	
	// Clamp simultaneous blend values
	simultaneousThreshold = std::clamp(simultaneousThreshold, 0.0f, 5.0f);
	simultaneousCameraMult = std::clamp(simultaneousCameraMult, 0.0f, 2.0f);
	simultaneousMovementMult = std::clamp(simultaneousMovementMult, 0.0f, 2.0f);
	
	// Clamp sprint values
	sprintImpulseY = std::clamp(sprintImpulseY, 0.0f, 50.0f);
	sprintImpulseZ = std::clamp(sprintImpulseZ, 0.0f, 30.0f);
	sprintRotImpulse = std::clamp(sprintRotImpulse, 0.0f, 45.0f);
	sprintImpulseBlendTime = std::clamp(sprintImpulseBlendTime, 0.0f, 1.0f);
	sprintStiffness = std::clamp(sprintStiffness, 10.0f, 500.0f);
	sprintDamping = std::clamp(sprintDamping, 1.0f, 50.0f);
	
	// Clamp jump/land values
	cameraInertiaAirMult = std::clamp(cameraInertiaAirMult, 0.0f, 1.0f);
	jumpImpulseY = std::clamp(jumpImpulseY, 0.0f, 30.0f);
	jumpImpulseZ = std::clamp(jumpImpulseZ, 0.0f, 30.0f);
	jumpRotImpulse = std::clamp(jumpRotImpulse, 0.0f, 30.0f);
	jumpStiffness = std::clamp(jumpStiffness, 10.0f, 200.0f);
	jumpDamping = std::clamp(jumpDamping, 1.0f, 20.0f);
	landImpulseY = std::clamp(landImpulseY, 0.0f, 30.0f);
	landImpulseZ = std::clamp(landImpulseZ, 0.0f, 50.0f);
	landRotImpulse = std::clamp(landRotImpulse, 0.0f, 30.0f);
	landStiffness = std::clamp(landStiffness, 20.0f, 500.0f);
	landDamping = std::clamp(landDamping, 2.0f, 50.0f);
	airTimeImpulseScale = std::clamp(airTimeImpulseScale, 0.0f, 5.0f);
	
	// Clamp pivot (0=Chest, 1=RightHand, 2=LeftHand, 3=Weapon, 4=BothClavicles, 5=BothClaviclesOffset)
	pivotPoint = std::clamp(pivotPoint, 0, 5);
}

void WeaponInertiaSettings::Save(CSimpleIniA& a_ini, const char* a_section) const
{
	// === MASTER TOGGLE ===
	a_ini.SetBoolValue(a_section, "bEnabled", enabled);
	
	// === CAMERA INERTIA ===
	a_ini.SetDoubleValue(a_section, "fStiffness", stiffness);
	a_ini.SetDoubleValue(a_section, "fDamping", damping);
	a_ini.SetDoubleValue(a_section, "fMaxOffset", maxOffset);
	a_ini.SetDoubleValue(a_section, "fMaxRotation", maxRotation);
	a_ini.SetDoubleValue(a_section, "fMass", mass);
	a_ini.SetDoubleValue(a_section, "fPitchMultiplier", pitchMultiplier);
	a_ini.SetDoubleValue(a_section, "fRollMultiplier", rollMultiplier);
	a_ini.SetDoubleValue(a_section, "fCameraPitchMult", cameraPitchMult);
	a_ini.SetBoolValue(a_section, "bInvertCameraPitch", invertCameraPitch);
	a_ini.SetBoolValue(a_section, "bInvertCameraYaw", invertCameraYaw);
	
	// === MOVEMENT INERTIA ===
	a_ini.SetBoolValue(a_section, "bMovementInertiaEnabled", movementInertiaEnabled);
	a_ini.SetDoubleValue(a_section, "fMovementStiffness", movementStiffness);
	a_ini.SetDoubleValue(a_section, "fMovementDamping", movementDamping);
	a_ini.SetDoubleValue(a_section, "fMovementMaxOffset", movementMaxOffset);
	a_ini.SetDoubleValue(a_section, "fMovementMaxRotation", movementMaxRotation);
	a_ini.SetDoubleValue(a_section, "fMovementLeftMult", movementLeftMult);
	a_ini.SetDoubleValue(a_section, "fMovementRightMult", movementRightMult);
	a_ini.SetDoubleValue(a_section, "fMovementForwardMult", movementForwardMult);
	a_ini.SetDoubleValue(a_section, "fMovementBackwardMult", movementBackwardMult);
	a_ini.SetBoolValue(a_section, "bInvertMovementLateral", invertMovementLateral);
	a_ini.SetBoolValue(a_section, "bInvertMovementForwardBack", invertMovementForwardBack);
	
	// === SIMULTANEOUS CAMERA+MOVEMENT SCALING ===
	a_ini.SetDoubleValue(a_section, "fSimultaneousThreshold", simultaneousThreshold);
	a_ini.SetDoubleValue(a_section, "fSimultaneousCameraMult", simultaneousCameraMult);
	a_ini.SetDoubleValue(a_section, "fSimultaneousMovementMult", simultaneousMovementMult);
	
	// === SPRINT TRANSITION INERTIA ===
	a_ini.SetBoolValue(a_section, "bSprintInertiaEnabled", sprintInertiaEnabled);
	a_ini.SetDoubleValue(a_section, "fSprintImpulseY", sprintImpulseY);
	a_ini.SetDoubleValue(a_section, "fSprintImpulseZ", sprintImpulseZ);
	a_ini.SetDoubleValue(a_section, "fSprintRotImpulse", sprintRotImpulse);
	a_ini.SetDoubleValue(a_section, "fSprintImpulseBlendTime", sprintImpulseBlendTime);
	a_ini.SetDoubleValue(a_section, "fSprintStiffness", sprintStiffness);
	a_ini.SetDoubleValue(a_section, "fSprintDamping", sprintDamping);
	
	// === JUMP/LANDING INERTIA ===
	a_ini.SetBoolValue(a_section, "bJumpInertiaEnabled", jumpInertiaEnabled);
	a_ini.SetDoubleValue(a_section, "fCameraInertiaAirMult", cameraInertiaAirMult);
	a_ini.SetDoubleValue(a_section, "fJumpImpulseY", jumpImpulseY);
	a_ini.SetDoubleValue(a_section, "fJumpImpulseZ", jumpImpulseZ);
	a_ini.SetDoubleValue(a_section, "fJumpRotImpulse", jumpRotImpulse);
	a_ini.SetDoubleValue(a_section, "fJumpStiffness", jumpStiffness);
	a_ini.SetDoubleValue(a_section, "fJumpDamping", jumpDamping);
	a_ini.SetDoubleValue(a_section, "fLandImpulseY", landImpulseY);
	a_ini.SetDoubleValue(a_section, "fLandImpulseZ", landImpulseZ);
	a_ini.SetDoubleValue(a_section, "fLandRotImpulse", landRotImpulse);
	a_ini.SetDoubleValue(a_section, "fLandStiffness", landStiffness);
	a_ini.SetDoubleValue(a_section, "fLandDamping", landDamping);
	a_ini.SetDoubleValue(a_section, "fAirTimeImpulseScale", airTimeImpulseScale);
	
	// === PIVOT POINT ===
	a_ini.SetLongValue(a_section, "iPivotPoint", pivotPoint);
}

void Settings::Load()
{
	constexpr auto path = L"Data/SKSE/Plugins/FPInertia.ini";

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(path);

	// General settings
	enabled = ini.GetBoolValue("General", "bEnabled", true);
	enablePosition = ini.GetBoolValue("General", "bEnablePosition", true);
	enableRotation = ini.GetBoolValue("General", "bEnableRotation", true);
	requireWeaponDrawn = ini.GetBoolValue("General", "bRequireWeaponDrawn", true);
	globalIntensity = static_cast<float>(ini.GetDoubleValue("General", "fGlobalIntensity", 1.0));
	smoothingFactor = static_cast<float>(ini.GetDoubleValue("General", "fSmoothingFactor", 0.5));
	
	// Settling behavior
	settleDelay = static_cast<float>(ini.GetDoubleValue("Settling", "fSettleDelay", 0.3));
	settleSpeed = static_cast<float>(ini.GetDoubleValue("Settling", "fSettleSpeed", 2.0));
	settleDampingMult = static_cast<float>(ini.GetDoubleValue("Settling", "fSettleDampingMult", 3.0));
	
	// Clamp settling values
	settleDelay = std::clamp(settleDelay, 0.0f, 2.0f);
	settleSpeed = std::clamp(settleSpeed, 0.5f, 10.0f);
	settleDampingMult = std::clamp(settleDampingMult, 1.0f, 10.0f);
	
	// Movement inertia global settings (per-weapon settings override spring params)
	movementInertiaEnabled = ini.GetBoolValue("MovementInertia", "bEnabled", true);
	movementInertiaStrength = static_cast<float>(ini.GetDoubleValue("MovementInertia", "fStrength", 3.0));
	movementInertiaThreshold = static_cast<float>(ini.GetDoubleValue("MovementInertia", "fThreshold", 30.0));
	forwardBackInertia = ini.GetBoolValue("MovementInertia", "bForwardBackInertia", false);
	disableVanillaSway = ini.GetBoolValue("MovementInertia", "bDisableVanillaSway", false);
	
	// Clamp movement inertia global values
	movementInertiaStrength = std::clamp(movementInertiaStrength, 0.0f, 20.0f);
	movementInertiaThreshold = std::clamp(movementInertiaThreshold, 0.0f, 200.0f);
	
	// Action blending
	blendDuringAttack = ini.GetBoolValue("ActionBlend", "bBlendDuringAttack", true);
	blendDuringBowDraw = ini.GetBoolValue("ActionBlend", "bBlendDuringBowDraw", true);
	blendDuringSpellCast = ini.GetBoolValue("ActionBlend", "bBlendDuringSpellCast", true);
	actionBlendSpeed = static_cast<float>(ini.GetDoubleValue("ActionBlend", "fActionBlendSpeed", 5.0));
	actionMinIntensity = static_cast<float>(ini.GetDoubleValue("ActionBlend", "fActionMinIntensity", 0.2));
	
	// Clamp action blend values
	actionBlendSpeed = std::clamp(actionBlendSpeed, 1.0f, 20.0f);
	actionMinIntensity = std::clamp(actionMinIntensity, 0.0f, 1.0f);
	
	// Hand settings
	independentHands = ini.GetBoolValue("Hands", "bIndependentHands", true);
	leftHandMultiplier = static_cast<float>(ini.GetDoubleValue("Hands", "fLeftHandMultiplier", 1.0));
	rightHandMultiplier = static_cast<float>(ini.GetDoubleValue("Hands", "fRightHandMultiplier", 1.0));
	
	// Debug settings
	debugLogging = ini.GetBoolValue("Debug", "bDebugLogging", false);
	debugOnScreen = ini.GetBoolValue("Debug", "bDebugOnScreen", false);
	
	// Hot reload settings
	enableHotReload = ini.GetBoolValue("Debug", "bEnableHotReload", true);
	hotReloadIntervalSec = static_cast<float>(ini.GetDoubleValue("Debug", "fHotReloadInterval", 5.0));
	hotReloadIntervalSec = std::clamp(hotReloadIntervalSec, 1.0f, 60.0f);
	
	// Frame Generation Compatibility
	// Note: DetectCommunityShaders() should be called separately at plugin load time
	// Here we just load the user preference, which may be overridden by auto-detection
	bool iniFrameGenMode = ini.GetBoolValue("FrameGenCompat", "bEnabled", false);
	
	// If CommunityShaders was detected, force-enable; otherwise use INI value
	if (!communityShadersDetected) {
		frameGenCompatMode = iniFrameGenMode;
	}
	
	// Store file modification time for hot reload
	try {
		lastModifiedTime = std::filesystem::last_write_time(path);
	} catch (...) {
		// File might not exist, ignore
	}

	// Clamp general values
	globalIntensity = std::clamp(globalIntensity, 0.0f, 5.0f);
	smoothingFactor = std::clamp(smoothingFactor, 0.0f, 1.0f);
	leftHandMultiplier = std::clamp(leftHandMultiplier, 0.0f, 3.0f);
	rightHandMultiplier = std::clamp(rightHandMultiplier, 0.0f, 3.0f);

	// Set up default values per weapon type before loading
	// Use helper function to set defaults - avoids long initializer lists
	auto setWeaponDefaults = [](WeaponInertiaSettings& ws,
		float stiff, float damp, float maxOff, float maxRot, float mass,
		float pitchMult, float rollMult, float camPitchMult, bool invertCam,
		float mvStiff, float mvDamp, float mvMaxOff, float mvMaxRot,
		float mvLeft, float mvRight, bool invertMv, int pivot)
	{
		// Camera inertia
		ws.stiffness = stiff;
		ws.damping = damp;
		ws.maxOffset = maxOff;
		ws.maxRotation = maxRot;
		ws.mass = mass;
		ws.pitchMultiplier = pitchMult;
		ws.rollMultiplier = rollMult;
		ws.cameraPitchMult = camPitchMult;
		ws.invertCameraPitch = invertCam;
		ws.invertCameraYaw = invertCam;
		// Movement inertia
		ws.movementInertiaEnabled = true;
		ws.movementStiffness = mvStiff;
		ws.movementDamping = mvDamp;
		ws.movementMaxOffset = mvMaxOff;
		ws.movementMaxRotation = mvMaxRot;
		ws.movementLeftMult = mvLeft;
		ws.movementRightMult = mvRight;
		ws.movementForwardMult = 0.5f;
		ws.movementBackwardMult = 0.5f;
		ws.invertMovementLateral = invertMv;
		ws.invertMovementForwardBack = invertMv;
		ws.simultaneousThreshold = 0.5f;
		ws.simultaneousCameraMult = 1.0f;
		ws.simultaneousMovementMult = 1.0f;
		// Sprint transition inertia (disabled by default)
		ws.sprintInertiaEnabled = false;
		ws.sprintImpulseY = 8.0f;
		ws.sprintImpulseZ = 3.0f;
		ws.sprintRotImpulse = 5.0f;
		ws.sprintImpulseBlendTime = 0.1f;
		ws.sprintStiffness = 60.0f;
		ws.sprintDamping = 5.0f;
		// Jump/landing inertia (enabled by default)
		ws.jumpInertiaEnabled = true;
		ws.cameraInertiaAirMult = 0.3f;
		ws.jumpImpulseY = 4.0f;
		ws.jumpImpulseZ = 6.0f;
		ws.jumpRotImpulse = 3.0f;
		ws.jumpStiffness = 40.0f;
		ws.jumpDamping = 3.0f;
		ws.landImpulseY = 3.0f;
		ws.landImpulseZ = 10.0f;
		ws.landRotImpulse = 5.0f;
		ws.landStiffness = 120.0f;
		ws.landDamping = 10.0f;
		ws.airTimeImpulseScale = 1.5f;
		// Pivot
		ws.pivotPoint = pivot;
	};
	
	// Unarmed - very light, fast response
	setWeaponDefaults(unarmed, 200.0f, 15.0f, 5.0f, 10.0f, 0.5f, 1.0f, 1.0f, 1.0f, false,
	                  120.0f, 8.0f, 8.0f, 15.0f, 1.0f, 1.0f, false, 0);
	
	// One-hand sword - medium
	setWeaponDefaults(oneHandSword, 150.0f, 12.0f, 8.0f, 15.0f, 1.0f, 1.0f, 1.0f, 1.0f, false,
	                  80.0f, 6.0f, 12.0f, 20.0f, 1.0f, 1.0f, false, 0);
	
	// Dagger - very light
	setWeaponDefaults(oneHandDagger, 180.0f, 14.0f, 6.0f, 12.0f, 0.6f, 1.0f, 1.0f, 1.0f, false,
	                  100.0f, 7.0f, 10.0f, 18.0f, 1.0f, 1.0f, false, 0);
	
	// One-hand axe - medium-heavy
	setWeaponDefaults(oneHandAxe, 130.0f, 11.0f, 10.0f, 18.0f, 1.3f, 1.0f, 1.0f, 1.0f, false,
	                  70.0f, 5.0f, 14.0f, 22.0f, 1.0f, 1.0f, false, 0);
	
	// One-hand mace - heavy, slow
	setWeaponDefaults(oneHandMace, 120.0f, 10.0f, 12.0f, 20.0f, 1.5f, 1.0f, 1.0f, 1.0f, false,
	                  60.0f, 5.0f, 16.0f, 25.0f, 1.0f, 1.0f, false, 0);
	
	// Two-hand sword - heavy
	setWeaponDefaults(twoHandSword, 100.0f, 9.0f, 14.0f, 22.0f, 2.0f, 1.0f, 1.0f, 1.0f, false,
	                  50.0f, 4.0f, 18.0f, 28.0f, 1.0f, 1.0f, false, 0);
	
	// Two-hand axe - very heavy
	setWeaponDefaults(twoHandAxe, 90.0f, 8.0f, 16.0f, 25.0f, 2.5f, 1.0f, 1.0f, 1.0f, false,
	                  45.0f, 4.0f, 20.0f, 30.0f, 1.0f, 1.0f, false, 0);
	
	// Bow - medium, pivot at weapon for stability while aiming
	setWeaponDefaults(bow, 140.0f, 11.0f, 8.0f, 12.0f, 0.8f, 1.0f, 1.0f, 1.0f, false,
	                  90.0f, 7.0f, 10.0f, 15.0f, 1.0f, 1.0f, false, 3);
	
	// Staff - light but long
	setWeaponDefaults(staff, 160.0f, 12.0f, 10.0f, 18.0f, 0.9f, 1.0f, 1.5f, 1.0f, false,
	                  80.0f, 6.0f, 12.0f, 20.0f, 1.0f, 1.0f, false, 0);
	
	// Crossbow - heavy, stable, pivot at weapon
	setWeaponDefaults(crossbow, 110.0f, 10.0f, 10.0f, 15.0f, 1.8f, 1.0f, 1.0f, 1.0f, false,
	                  70.0f, 6.0f, 10.0f, 12.0f, 1.0f, 1.0f, false, 3);
	
	// Shield - medium-heavy
	setWeaponDefaults(shield, 120.0f, 10.0f, 10.0f, 15.0f, 1.4f, 1.0f, 1.0f, 1.0f, false,
	                  70.0f, 6.0f, 12.0f, 18.0f, 1.0f, 1.0f, false, 0);
	
	// Spell (hands only) - very light
	setWeaponDefaults(spell, 200.0f, 15.0f, 5.0f, 10.0f, 0.4f, 1.0f, 1.0f, 1.0f, false,
	                  100.0f, 8.0f, 8.0f, 12.0f, 1.0f, 1.0f, false, 0);
	
	// Dual Wield Weapons - two weapons in hands, default pivot is BothClavicles (4)
	setWeaponDefaults(dualWieldWeapons, 140.0f, 11.0f, 10.0f, 18.0f, 1.2f, 1.0f, 1.0f, 1.0f, false,
	                  75.0f, 5.0f, 14.0f, 22.0f, 1.0f, 1.0f, false, 4);
	
	// Dual Wield Magic - spells in both hands, very light, default pivot is BothClavicles (4)
	setWeaponDefaults(dualWieldMagic, 200.0f, 15.0f, 5.0f, 10.0f, 0.4f, 1.0f, 1.0f, 1.0f, false,
	                  100.0f, 8.0f, 8.0f, 12.0f, 1.0f, 1.0f, false, 4);
	
	// Spell + Weapon combo - medium weight, default pivot is BothClavicles (4)
	setWeaponDefaults(spellAndWeapon, 160.0f, 12.0f, 8.0f, 15.0f, 0.9f, 1.0f, 1.0f, 1.0f, false,
	                  85.0f, 6.0f, 11.0f, 18.0f, 1.0f, 1.0f, false, 4);

	// Check if any new fields are missing from the INI (need to write them)
	bool iniNeedsUpdate = false;
	auto checkNewField = [&ini, &iniNeedsUpdate](const char* section, const char* key) {
		if (ini.GetValue(section, key) == nullptr) {
			iniNeedsUpdate = true;
			logger::info("  INI section [{}] missing '{}' - will update INI", section, key);
		}
	};
	
	// Check for the new 'bEnabled' field in each weapon section
	const char* weaponSections[] = {
		"Unarmed", "OneHandSword", "OneHandDagger", "OneHandAxe", "OneHandMace",
		"TwoHandSword", "TwoHandAxe", "Bow", "Staff", "Crossbow", "Shield", "Spell",
		"DualWieldWeapons", "DualWieldMagic", "SpellAndWeapon"
	};
	for (const char* section : weaponSections) {
		checkNewField(section, "bEnabled");
	}
	
	// Load per-weapon settings from INI
	unarmed.Load(ini, "Unarmed");
	oneHandSword.Load(ini, "OneHandSword");
	oneHandDagger.Load(ini, "OneHandDagger");
	oneHandAxe.Load(ini, "OneHandAxe");
	oneHandMace.Load(ini, "OneHandMace");
	twoHandSword.Load(ini, "TwoHandSword");
	twoHandAxe.Load(ini, "TwoHandAxe");
	bow.Load(ini, "Bow");
	staff.Load(ini, "Staff");
	crossbow.Load(ini, "Crossbow");
	shield.Load(ini, "Shield");
	spell.Load(ini, "Spell");
	// Dual wield types
	dualWieldWeapons.Load(ini, "DualWieldWeapons");
	dualWieldMagic.Load(ini, "DualWieldMagic");
	spellAndWeapon.Load(ini, "SpellAndWeapon");
	
	// If any new fields were missing, save the INI to include them
	if (iniNeedsUpdate) {
		logger::info("Re-saving INI to update with new fields...");
		Save();
	}

	logger::info("Settings loaded:");
	logger::info("  Enabled: {}", enabled);
	logger::info("  Enable Position: {}", enablePosition);
	logger::info("  Enable Rotation: {}", enableRotation);
	logger::info("  Require Weapon Drawn: {}", requireWeaponDrawn);
	logger::info("  Global Intensity: {:.2f}", globalIntensity);
	logger::info("  Smoothing Factor: {:.2f}", smoothingFactor);
	logger::info("  Movement Inertia: {} (strength={:.2f}, threshold={:.1f})", 
		movementInertiaEnabled, movementInertiaStrength, movementInertiaThreshold);
	logger::info("  Per-weapon settings: pivot, invert, movement spring - loaded for each weapon type");
	logger::info("  Debug Logging: {}", debugLogging);
}

const WeaponInertiaSettings& Settings::GetWeaponSettings(RE::WEAPON_TYPE a_type) const
{
	switch (a_type) {
	case RE::WEAPON_TYPE::kHandToHandMelee:
		return unarmed;
	case RE::WEAPON_TYPE::kOneHandSword:
		return oneHandSword;
	case RE::WEAPON_TYPE::kOneHandDagger:
		return oneHandDagger;
	case RE::WEAPON_TYPE::kOneHandAxe:
		return oneHandAxe;
	case RE::WEAPON_TYPE::kOneHandMace:
		return oneHandMace;
	case RE::WEAPON_TYPE::kTwoHandSword:
		return twoHandSword;
	case RE::WEAPON_TYPE::kTwoHandAxe:
		return twoHandAxe;
	case RE::WEAPON_TYPE::kBow:
		return bow;
	case RE::WEAPON_TYPE::kStaff:
		return staff;
	case RE::WEAPON_TYPE::kCrossbow:
		return crossbow;
	default:
		return unarmed;
	}
}

WeaponType Settings::ToWeaponType(RE::WEAPON_TYPE a_type)
{
	switch (a_type) {
	case RE::WEAPON_TYPE::kHandToHandMelee:
		return WeaponType::Unarmed;
	case RE::WEAPON_TYPE::kOneHandSword:
		return WeaponType::OneHandSword;
	case RE::WEAPON_TYPE::kOneHandDagger:
		return WeaponType::OneHandDagger;
	case RE::WEAPON_TYPE::kOneHandAxe:
		return WeaponType::OneHandAxe;
	case RE::WEAPON_TYPE::kOneHandMace:
		return WeaponType::OneHandMace;
	case RE::WEAPON_TYPE::kTwoHandSword:
		return WeaponType::TwoHandSword;
	case RE::WEAPON_TYPE::kTwoHandAxe:
		return WeaponType::TwoHandAxe;
	case RE::WEAPON_TYPE::kBow:
		return WeaponType::Bow;
	case RE::WEAPON_TYPE::kStaff:
		return WeaponType::Staff;
	case RE::WEAPON_TYPE::kCrossbow:
		return WeaponType::Crossbow;
	default:
		return WeaponType::Unarmed;
	}
}

const WeaponInertiaSettings& Settings::GetWeaponSettings(WeaponType a_type) const
{
	switch (a_type) {
	case WeaponType::Unarmed:
		return unarmed;
	case WeaponType::OneHandSword:
		return oneHandSword;
	case WeaponType::OneHandDagger:
		return oneHandDagger;
	case WeaponType::OneHandAxe:
		return oneHandAxe;
	case WeaponType::OneHandMace:
		return oneHandMace;
	case WeaponType::TwoHandSword:
		return twoHandSword;
	case WeaponType::TwoHandAxe:
		return twoHandAxe;
	case WeaponType::Bow:
		return bow;
	case WeaponType::Staff:
		return staff;
	case WeaponType::Crossbow:
		return crossbow;
	case WeaponType::Shield:
		return shield;
	case WeaponType::Spell:
		return spell;
	case WeaponType::DualWieldWeapons:
		return dualWieldWeapons;
	case WeaponType::DualWieldMagic:
		return dualWieldMagic;
	case WeaponType::SpellAndWeapon:
		return spellAndWeapon;
	default:
		return unarmed;
	}
}

WeaponInertiaSettings& Settings::GetWeaponSettingsMutable(WeaponType a_type)
{
	switch (a_type) {
	case WeaponType::Unarmed:
		return unarmed;
	case WeaponType::OneHandSword:
		return oneHandSword;
	case WeaponType::OneHandDagger:
		return oneHandDagger;
	case WeaponType::OneHandAxe:
		return oneHandAxe;
	case WeaponType::OneHandMace:
		return oneHandMace;
	case WeaponType::TwoHandSword:
		return twoHandSword;
	case WeaponType::TwoHandAxe:
		return twoHandAxe;
	case WeaponType::Bow:
		return bow;
	case WeaponType::Staff:
		return staff;
	case WeaponType::Crossbow:
		return crossbow;
	case WeaponType::Shield:
		return shield;
	case WeaponType::Spell:
		return spell;
	case WeaponType::DualWieldWeapons:
		return dualWieldWeapons;
	case WeaponType::DualWieldMagic:
		return dualWieldMagic;
	case WeaponType::SpellAndWeapon:
		return spellAndWeapon;
	default:
		return unarmed;
	}
}

void Settings::CheckForReload(float a_deltaTime)
{
	if (!enableHotReload) {
		return;
	}
	
	timeSinceLastCheck += a_deltaTime;
	
	if (timeSinceLastCheck < hotReloadIntervalSec) {
		return;
	}
	
	timeSinceLastCheck = 0.0f;
	
	constexpr auto path = L"Data/SKSE/Plugins/FPInertia.ini";
	
	try {
		auto currentModTime = std::filesystem::last_write_time(path);
		
		if (currentModTime != lastModifiedTime) {
			logger::info("INI file changed, reloading settings...");
			Load();
			logger::info("Settings reloaded successfully");
		}
	} catch (...) {
		// File access error, ignore
	}
}

void Settings::Save()
{
	constexpr auto path = L"Data/SKSE/Plugins/FPInertia.ini";
	
	CSimpleIniA ini;
	ini.SetUnicode();
	
	// General settings
	ini.SetBoolValue("General", "bEnabled", enabled, 
		"; Master enable/disable for the entire mod");
	ini.SetBoolValue("General", "bEnablePosition", enablePosition,
		"; Enable position offset (weapon moves left/right/up/down)");
	ini.SetBoolValue("General", "bEnableRotation", enableRotation,
		"; Enable rotation offset (weapon tilts/rotates)");
	ini.SetBoolValue("General", "bRequireWeaponDrawn", requireWeaponDrawn,
		"; Only apply inertia when weapon is drawn");
	ini.SetDoubleValue("General", "fGlobalIntensity", globalIntensity,
		"; Global intensity multiplier (0.0-5.0)");
	ini.SetDoubleValue("General", "fSmoothingFactor", smoothingFactor,
		"; Camera velocity smoothing factor (0.0-1.0)");
	
	// Settling behavior
	ini.SetDoubleValue("Settling", "fSettleDelay", settleDelay,
		"; Delay before settling starts (seconds, 0.0-2.0)");
	ini.SetDoubleValue("Settling", "fSettleSpeed", settleSpeed,
		"; How fast settling increases (0.5-10.0)");
	ini.SetDoubleValue("Settling", "fSettleDampingMult", settleDampingMult,
		"; Maximum damping multiplier when fully settled (1.0-10.0)");
	
	// Movement inertia global settings
	ini.SetBoolValue("MovementInertia", "bEnabled", movementInertiaEnabled,
		"; Master enable for movement-based arm sway");
	ini.SetDoubleValue("MovementInertia", "fStrength", movementInertiaStrength,
		"; Global strength multiplier (0.0-20.0)");
	ini.SetDoubleValue("MovementInertia", "fThreshold", movementInertiaThreshold,
		"; Minimum movement speed to trigger (units/second, 0.0-200.0)");
	ini.SetBoolValue("MovementInertia", "bForwardBackInertia", forwardBackInertia,
		"; Enable arm sway for forward/backward movement (default: OFF)");
	ini.SetBoolValue("MovementInertia", "bDisableVanillaSway", disableVanillaSway,
		"; Disable vanilla walk/run arm sway via behavior graph");
	
	// Action blending
	ini.SetBoolValue("ActionBlend", "bBlendDuringAttack", blendDuringAttack,
		"; Smoothly reduce inertia during melee attacks");
	ini.SetBoolValue("ActionBlend", "bBlendDuringBowDraw", blendDuringBowDraw,
		"; Smoothly reduce inertia while drawing bow");
	ini.SetBoolValue("ActionBlend", "bBlendDuringSpellCast", blendDuringSpellCast,
		"; Smoothly reduce inertia during spell casting");
	ini.SetDoubleValue("ActionBlend", "fActionBlendSpeed", actionBlendSpeed,
		"; How fast to blend in/out (1.0-20.0)");
	ini.SetDoubleValue("ActionBlend", "fActionMinIntensity", actionMinIntensity,
		"; Minimum intensity during actions (0.0-1.0)");
	
	// Hand settings
	ini.SetBoolValue("Hands", "bIndependentHands", independentHands);
	ini.SetDoubleValue("Hands", "fLeftHandMultiplier", leftHandMultiplier);
	ini.SetDoubleValue("Hands", "fRightHandMultiplier", rightHandMultiplier);
	
	// Debug settings
	ini.SetBoolValue("Debug", "bDebugLogging", debugLogging);
	ini.SetBoolValue("Debug", "bDebugOnScreen", debugOnScreen);
	ini.SetBoolValue("Debug", "bEnableHotReload", enableHotReload);
	ini.SetDoubleValue("Debug", "fHotReloadInterval", hotReloadIntervalSec);
	
	// Frame Generation Compatibility
	ini.SetBoolValue("FrameGenCompat", "bEnabled", frameGenCompatMode,
		"; Enable two-hook frame generation compatibility (applies offsets after animations)");
	
	// Per-weapon settings
	unarmed.Save(ini, "Unarmed");
	oneHandSword.Save(ini, "OneHandSword");
	oneHandDagger.Save(ini, "OneHandDagger");
	oneHandAxe.Save(ini, "OneHandAxe");
	oneHandMace.Save(ini, "OneHandMace");
	twoHandSword.Save(ini, "TwoHandSword");
	twoHandAxe.Save(ini, "TwoHandAxe");
	bow.Save(ini, "Bow");
	staff.Save(ini, "Staff");
	crossbow.Save(ini, "Crossbow");
	shield.Save(ini, "Shield");
	spell.Save(ini, "Spell");
	// Dual wield types
	dualWieldWeapons.Save(ini, "DualWieldWeapons");
	dualWieldMagic.Save(ini, "DualWieldMagic");
	spellAndWeapon.Save(ini, "SpellAndWeapon");
	
	// Save to file
	SI_Error rc = ini.SaveFile(path);
	if (rc < 0) {
		logger::error("Failed to save settings to INI file");
	} else {
		logger::info("Settings saved to INI file");
		
		// Update modification time to prevent hot-reload from immediately reloading
		try {
			lastModifiedTime = std::filesystem::last_write_time(path);
		} catch (...) {
			// Ignore
		}
	}
}

