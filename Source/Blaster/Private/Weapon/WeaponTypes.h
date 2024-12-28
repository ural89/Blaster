#pragma once
#define TRACE_LENGTH 80000.f
#define CUSTOM_DEPTH_PURPLE 250
#define CUSTOM_DEPTH_BLUE 251
#define CUSTOM_DEPTH_TAN 252

UENUM(BlueprintType) // blueprint type isjust in case we need to use in blueprint not necessary
enum class EWeaponType : uint8
{
    EWT_AssaultRiffle UMETA(DisplayName = "Assault Riffle"),
    EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
    EWT_Pistol UMETA(DisplayName = "Pistol"),
    EWT_SubmachineGun UMETA(DisplayName = "Submachine Gun"),
    EWT_Shotgun UMETA(DisplayName = "Shotgun"),
    EWT_SniperRifle UMETA(DisplayName = "Sniper Rifle"),
    EWT_GrenadeLauncher UMETA(DisplayName = "Grenade Launcher"),
    EWT_MAX UMETA(DisplayName = "DefaultMax")
};