// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class ABlasterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

	void PlayFireMontage(bool bAiming);

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void FireButtonPressed();
	void FireButtonReleased();
	void AimButtonPressed();
	void AimButtonReleased();
	void UpdateAimOffset(float DeltaTime);
	void Jump() override;

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

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent *Combat;

	UFUNCTION(Server, Reliable)		 // this is RPC to Server from clients
	void ServerEquipButtonPressed(); // definetion of this is .._Implementation() //RPCs can take parameters but not OnRep notifiers

	float AO_Yaw;
	float AO_Pitch;

	float InterpAO_Yaw;

	FRotator LastRunningAimRotation;

	ETurningInPlace TurningInPlace;
	void UpdateTurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

public:
	void SetOverlappingWeapon(class AWeapon *Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GETAOYaw() const { return AO_Yaw; }
	FORCEINLINE float GetAOPitch() const { return AO_Pitch; }
	AWeapon *GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera;}
};
