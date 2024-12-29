#include "CombatComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Weapon.h"
#include "Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "PlayerController/BlasterPlayerController.h"
#include "TimerManager.h"
#include "Camera/CameraComponent.h"
#include "Sound/SoundCue.h"
#include "Character/BlasterAnimInstance.h"

UCombatComponent::UCombatComponent()
{
	AimWalkSpeed = 450.f;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	PrimaryComponentTick.Target = this;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.SetTickFunctionEnable(true);
	PrimaryComponentTick.RegisterTickFunction(GetComponentLevel());
	if (Character)
	{
		BaseWalkSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}
}
void UCombatComponent::ShotgunShellReload()
{
	if (Character && Character->HasAuthority())
	{
		UpdateShotgunAmmoValues();
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		UpdateHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::UpdateHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr)
		return;

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller; // TODO: this is BS carry this to Begin play

	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
			}
			else
			{

				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
			}
			// Calculate spread
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;

			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());
			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, -0.58f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0, DeltaTime, 30.f);
			}
			CrosshairShootFactor = FMath::FInterpTo(CrosshairShootFactor, 0, DeltaTime, 40.f);
			HUDPackage.CrosshairSpread = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor + CrosshairAimFactor + CrosshairShootFactor;
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr)
		return;
	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr)
		return;
	Character->GetWorldTimerManager().SetTimer(FireTimer,
											   this,
											   &UCombatComponent::FireTimeFinished,
											   EquippedWeapon->FireDelay);
}

void UCombatComponent::FireTimeFinished()
{
	if (EquippedWeapon == nullptr)
		return;
	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr)
		return;
	bAiming = bIsAiming;	  // since this is visual only, we can set localy for faster feedback
	ServerSetAiming(bAiming); // RPC from this client to server
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming);
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CurrentWeaponCariedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		/////We set weapon state again here because:
		// in equip weapon both setweaponstate and attach actor is replicated.
		// But it is no guarantee which one is recieved first. so we copy them in here to
		// clients. Because first we need to set weapon state and disable collisions first
		// then attach to actor
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		PlayEquipWeaponSound();
	}
}

void UCombatComponent::EquipWeapon(AWeapon *WeaponToEquip) // Only called for server
{
	if (Character == nullptr || WeaponToEquip == nullptr)
		return;

	if (CombatState != ECombatState::ECS_Unoccupied)
		return;

	DropEquippedWeapon();

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped); // since EquippedWeapon is replicated it will set for everyone
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetOwner(Character); // this is replicated by default (Actors have virtual OnRep_Owner() method)
	EquippedWeapon->UpdateHUDAmmo();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CurrentWeaponCariedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller =
		Controller == nullptr
			? Cast<ABlasterPlayerController>(Character->Controller)
			: Controller;

	if (Controller) // we can update for client in on repammo (ammo updated 5 lines up)
	{
		Controller->SetHUDCarriedAmmo(CurrentWeaponCariedAmmo);
	}
	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->EquipSound,
			Character->GetActorLocation());
	}
}

void UCombatComponent::UpdateCarriedAmmo()
{
}

void UCombatComponent::PlayEquipWeaponSound()
{
	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->EquipSound,
			Character->GetActorLocation());
	}
}
void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon && EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}
void UCombatComponent::DropEquippedWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
}
void UCombatComponent::AttachActorToRightHand(AActor *ActorToAttach)
{
	if (Character == nullptr || ActorToAttach == nullptr) return;
	const USkeletalMeshSocket *HandSocket =
		Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
}
void UCombatComponent::AttachActorToLeftHand(AActor *ActorToAttach)
{
}
void UCombatComponent::Reload()
{
	if (CurrentWeaponCariedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied)
	{
		ServerReload();
	}
}
void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}
	CombatState = ECombatState::ECS_Reloading;
	HandleReload(); // this is on server reload
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
		return;
	int ReloadAmount = AmountToReload();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CurrentWeaponCariedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller =
		Controller == nullptr
			? Cast<ABlasterPlayerController>(Character->Controller)
			: Controller;
	if (Controller) // we can update for client in on repammo (ammo updated 5 lines up)
	{
		Controller->SetHUDCarriedAmmo(CurrentWeaponCariedAmmo);
	}
	EquippedWeapon->AddAmmo(-ReloadAmount);
}
void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
		return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CurrentWeaponCariedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CurrentWeaponCariedAmmo);
	}
	EquippedWeapon->AddAmmo(-1);
	bCanFire = true;
	if (EquippedWeapon->IsFull() || CurrentWeaponCariedAmmo == 0)
	{
		JumpToShotgunEnd();
	}
}
void UCombatComponent::OnFinishReloadingAnim()
{
	if (Character == nullptr)
		return;
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload(); // this is on client reload
		break;

	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed)
		{
			Fire();
		}
	case ECombatState::ECS_ThrowingGrenade:
		if (Character && !Character->IsLocallyControlled())
		{
			Character->PlayThrowGrenadeMontage();
			AttachActorToLeftHand(EquippedWeapon);
		}
		break;
	}
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr)
	{
		return 0;
	}
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, AmountCarried);
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}

void UCombatComponent::ThrowGrenade()
{
	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
	}
	if (Character && !Character->HasAuthority()) // otherwise it will be called twice
	{
		ServerThrowGrenade();
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult &TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);
	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;
		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 150.f);
		}
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility);
	}
	if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
	{
		HUDPackage.CrosshairsColor = FLinearColor::Red;
	}
	else
	{
		HUDPackage.CrosshairsColor = FLinearColor::White;
	}
	if (bAiming && EquippedWeapon && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		HUDPackage.CrosshairsColor = FLinearColor::Transparent;
	}
}
void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if (bFireButtonPressed && EquippedWeapon)
	{
		Fire();
	}
}
void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;
		ServerFire(HitTarget); // this is calling from clients to server

		if (EquippedWeapon)
		{
			CrosshairShootFactor = 0.75f;
		}
		StartFireTimer();
	}
}
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize &TraceHitTarget)
{
	MulticastFire(TraceHitTarget); // this is calling from server to all clients
}
void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize &TraceHitTarget)
{
	// UE_LOG(LogTemp, Warning, TEXT("Fire requested"));
	if (EquippedWeapon == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("EquippedWeapon is null"));
		return;
	}
	if (Character && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
		CombatState = ECombatState::ECS_Unoccupied;
		return;
	}
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr)
	{
		return false;
	}
	if (!EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
		return true;

	return !EquippedWeapon->IsEmpty() &&
		   bCanFire &&
		   CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::JumpToShotgunEnd()
{
	// Jump to ShotgunEnd section
	UAnimInstance *AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance && Character->GetReloadMontage())
	{
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
	}
}
void UCombatComponent::ThrowGrenadeFinished() // called from animation
{
	CombatState = ECombatState::ECS_Unoccupied;
	AttachActorToRightHand(EquippedWeapon);
}
void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRiffle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CurrentWeaponCariedAmmo);
	}
	bool bJumpToShotgunEnd =
		CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon != nullptr &&
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&
		CurrentWeaponCariedAmmo == 0;
	if (bJumpToShotgunEnd)
	{
		JumpToShotgunEnd();
	}
}
