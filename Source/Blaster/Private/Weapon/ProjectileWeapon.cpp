// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"
void AProjectileWeapon::Fire(const FVector &HitTarget)
{
    Super::Fire(HitTarget);
    APawn* InstigatorPawn = Cast<APawn>(GetOwner());

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
    if  (MuzzleFlashSocket)
    {
        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
        FVector TargetDirection = HitTarget - SocketTransform.GetLocation();//from muzzle to target trace
        FRotator TargetRotation = TargetDirection.Rotation();

        if (ProjectileClass && InstigatorPawn)
        {
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.Owner = GetOwner(); //owner of the weapon is set to character. Now this set to weapon
            SpawnParameters.Instigator = InstigatorPawn;
            UWorld* World = GetWorld();
            if(World)
            {
                World->SpawnActor<AProjectile>(
                    ProjectileClass,
                    SocketTransform.GetLocation(),
                    TargetRotation,
                    SpawnParameters
                );
            }
        }
    }
}
