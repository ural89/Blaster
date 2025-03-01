// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/BlasterHUD.h"
#include "Weapon/WeaponTypes.h"
#include "Blaster/BlasterTypes/CombatState.h"

#include "CombatComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

	friend class ABlasterCharacter;

	void EquipWeapon(class AWeapon *WeaponToEquip);
	void Reload();

	UFUNCTION(BlueprintCallable)
	void OnFinishReloadingAnim();
	void FireButtonPressed(bool bPressed);

	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	void JumpToShotgunEnd();

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	UFUNCTION(BlueprintCallable)
	void LaunchGrenade(); // this is called from animation

	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize &Target);
	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);

protected:
	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();
	void UpdateShotgunAmmoValues();

	UPROPERTY(ReplicatedUsing = OnRep_Grenades)
	int32 Grenades = 4;
	UFUNCTION()
	void OnRep_Grenades();
	UPROPERTY(EditAnywhere)
	int32 MaxGrenades = 4;
	void UpdateHUDGrenades();

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);
	void Fire();

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize &TraceHitTarget); // NetQuantize version is more effiecent over network

	UFUNCTION(NetMulticast, Reliable) // multicast RPC's will work everybody when fired from server
	void MulticastFire(const FVector_NetQuantize &TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult &TraceHitResult);

	void UpdateHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload();
	int32 AmountToReload();

	void ThrowGrenade();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;

	void DropEquippedWeapon();
	void AttachActorToRightHand(AActor *ActorToAttach);
	void AttachActorToLeftHand(AActor *ActorToAttach);
	void UpdateCarriedAmmo();
	void PlayEquipWeaponSound();
	void ReloadEmptyWeapon();
	void ShowAttachedGrenade(bool bShowGrenade);

private:
	class ABlasterCharacter *Character;
	class ABlasterPlayerController *Controller;
	class ABlasterHUD *HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon *EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed = false;

	// HUD and crosshairs

	FHUDPackage HUDPackage;
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootFactor;

	FVector HitTarget;

	// Aiming and FOV
	float DefaultFOV;

	UPROPERTY(Editanywhere, Category = Combat)
	float ZoomedFOV = 30.f;

	float CurrentFOV;

	UPROPERTY(Editanywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);

	// Automatic fire
	FTimerHandle FireTimer;
	bool bCanFire = true;

	FTimerHandle ThrowGrenadeTimer; // SInce i dont have anim for grenade i will set timer to finish
	float GrenadeThrowFinishDurationSec = 1.f;

	void StartFireTimer();
	void FireTimeFinished();

	bool CanFire();

	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CurrentWeaponCariedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType, int32> CarriedAmmoMap; // TMaps cant be replicated and
											 // this is in server! CurrentWeaponCarriedAmmo is replicated though

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 500;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 0;

	void InitializeCarriedAmmo();

	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLauncherAmmo = 0;

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues();

public:
	FORCEINLINE int32 GetGrenades() const { return Grenades; }
};