// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;

	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();
	void OnRep_ReplicatedMovement() override;
	void Elim();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

protected:
	virtual void BeginPlay() override;

	void RotateInPlace(float DeltaTime);
	void UpdateHUDHealth();
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void ReloadButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void GrenadeButtonPressed();
	void UpdateAimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimProxiesTurn();

	void FireButtonPressed();
	void FireButtonReleased();
	void PlayHitReactMontage();
	void Jump() override;

	UFUNCTION()
	void RecieveDamage(AActor *DamagedActor, float Damage, const UDamageType *DamageType,
					   class AController *InstigatorController, AActor *DamageCauser);
	void PollInit();

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent *CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent *FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent *OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon) // Replication happens only when we attach new value to OverlappingWeapon
	class AWeapon *OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(class AWeapon *LastWeapon); // OnRep_ + replicated objects name //It can have replicated object as parameter (optional)
	//!!! OnRep_ is NOT called for server (since servers do not replicate. Replication is only server to clients)!!!

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent *CombatComp;

	UFUNCTION(Server, Reliable)		 // this is RPC to Server from clients
	void ServerEquipButtonPressed(); // definetion of this is .._Implementation() //RPCs can take parameters but not OnRep notifiers

	float AO_Yaw;
	float AO_Pitch;

	float InterpAO_Yaw;

private:
	FRotator LastRunningAimRotation;

	ETurningInPlace TurningInPlace;
	void UpdateTurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = CombatComp)
	class UAnimMontage *FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = CombatComp)
	UAnimMontage *ReloadMontage;

	UPROPERTY(EditAnywhere, Category = CombatComp)
	UAnimMontage *HitReactMontage;

	UPROPERTY(EditAnywhere, Category = CombatComp)
	UAnimMontage *ElimMontage;

	UPROPERTY(EditAnywhere, Category = CombatComp)
	UAnimMontage *ThrowGrandeMontage;

	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	float CalculateSpeed();
	// Player Health
	UPROPERTY(EditAnywhere, Category = "Player Stats");
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats");
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health();

	UPROPERTY()
	class ABlasterPlayerController *BlasterPlayerController;

	bool bElimmed = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;
	void ElimTimerFInished();

	// Dissolve VFX
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent *DissolveTimeline;
	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat *DissolveCurve;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic *DynamicDissolveMaterialInstance; // Dynamic instance that we
															   // we change on runtime

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance *DissolveMaterialInstance; // instance set on the blueprint

	UPROPERTY(EditAnywhere)
	UParticleSystem *ElimBotEffect;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent *ElimBotComponent;

	UPROPERTY()
	class ABlasterPlayerState *BlasterPlayerState;

public:
	void SetOverlappingWeapon(class AWeapon *Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GETAOYaw() const { return AO_Yaw; }
	FORCEINLINE float GetAOPitch() const { return AO_Pitch; }
	AWeapon *GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent *GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return Health; }
	FORCEINLINE UCombatComponent *GetCombat() const { return CombatComp; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	ECombatState GetCombatState() const;
};