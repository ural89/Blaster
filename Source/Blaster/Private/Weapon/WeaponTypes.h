#pragma once

UENUM(BlueprintType) // blueprint type isjust in case we need to use in blueprint not necessary
enum class EWeaponType : uint8
{
    EWT_AssaultRiffle UMETA(DisplayName = "Assault Riffle"),
    EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
    EWT_MAX UMETA(DisplayName = "DefaultMax")
};