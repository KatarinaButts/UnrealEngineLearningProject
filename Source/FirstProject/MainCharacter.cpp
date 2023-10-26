// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Weapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Enemy.h"
#include "MainPlayerController.h"
#include "FirstSaveGame.h"
#include "ItemStorage.h"

// Sets default values
AMainCharacter::AMainCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Create Camera Boom (pulls towards the player if there's a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 600.f;	//camera follows at this distance
	CameraBoom->bUsePawnControlRotation = true;	//rotate arm based on controller

	// Set size for collision capsule
	GetCapsuleComponent()->SetCapsuleSize(30.0f, 104.0f);

	// Create Follow Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// Attach the camera to the end of hte boom and let the boom adjust to match 
	// the controller orientation
	FollowCamera->bUsePawnControlRotation = false;

	// Set our turn rates for input
	BaseTurnRate = 65.f;
	BaseLookUpRate = 65.f;

	// Don't rotate the character with the camera
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;				// Character faces the direction of input...
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);	// ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 650.0f;
	GetCharacterMovement()->AirControl = 0.2f;

	MaxHealth = 100.f;
	Health = 65.f;
	MaxStamina = 150.f;
	Stamina = 120.f;
	StaminaForBasicAttack = 30.f;
	MaxCoins = 100;
	Coins = 0;
	RunningSpeed = 650.f;
	SprintingSpeed = 950.f;

	bShiftKeyDown = false;
	bLMBDown = false;
	bESCDown = false;

	//Initialize Enums
	MovementStatus = EMovementStatus::EMS_Normal;
	StaminaStatus = EStaminaStatus::ESS_Normal;

	StaminaDrainRate = 25.f;
	MinSprintStamina = 50.f;

	InterpSpeed = 15.f;
	bInterpToEnemy = false;

	bHasCombatTarget = false;

	bMovingForward = false;

	bMovingRight = false;

}

// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	MainPlayerController = Cast<AMainPlayerController>(GetController());


	//This is the version for starting a new game
	FString Map = GetWorld()->GetMapName();
	Map.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	if (Map != "StartingTestMap") {
		UE_LOG(LogTemp, Warning, TEXT("We started out in a map that's not StartingTestMap"));		
		LoadGameNoSwitch();
	}

	if (MainPlayerController) {
		MainPlayerController->GameModeOnly();
	}

	//Below is the version for loading a saved game:
	/*
	LoadGameNoSwitch();

	if (MainPlayerController) {
		MainPlayerController->GameModeOnly();
	}
	*/
}

// Called every frame
void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MovementStatus == EMovementStatus::EMS_Dead) return;
	if (MainPlayerController) {
		if (MainPlayerController->bPauseMenuVisible) {
			GetMesh()->bPauseAnims = true;
			return;
		}
		else {
			GetMesh()->bPauseAnims = false;
		}
	}

	float DeltaStamina = StaminaDrainRate * DeltaTime;

	CalculateSprintStamina(DeltaStamina);

	if (bInterpToEnemy && CombatTarget) {
		FRotator LookAtYaw = GetLookAtRotationYaw(CombatTarget->GetActorLocation());
		FRotator InterRotation = FMath::RInterpTo(GetActorRotation(), LookAtYaw, DeltaTime, InterpSpeed);

		SetActorRotation(InterRotation);
	}

	if (CombatTarget) {
		CombatTargetLocation = CombatTarget->GetActorLocation();
		if (MainPlayerController) {
			MainPlayerController->EnemyLocation = CombatTargetLocation;
		}
	}

}

FRotator AMainCharacter::GetLookAtRotationYaw(FVector Target) {
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Target);
	FRotator LookAtRotationYaw(0.f, LookAtRotation.Yaw, 0.f);
	return LookAtRotationYaw;
}

// Called to bind functionality to input
void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMainCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AMainCharacter::StopJumping);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMainCharacter::ShiftKeyDown);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMainCharacter::ShiftKeyUp);

	PlayerInputComponent->BindAction("LMB", IE_Pressed, this, &AMainCharacter::LMBDown);
	PlayerInputComponent->BindAction("LMB", IE_Released, this, &AMainCharacter::LMBUp);

	PlayerInputComponent->BindAction("ESC", IE_Pressed, this, &AMainCharacter::ESCDown);
	PlayerInputComponent->BindAction("ESC", IE_Released, this, &AMainCharacter::ESCUp);


	PlayerInputComponent->BindAxis("MoveForward", this, &AMainCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMainCharacter::MoveRight);


	PlayerInputComponent->BindAxis("Turn", this, &AMainCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AMainCharacter::LookUp);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMainCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMainCharacter::LookUpAtRate);
}


void AMainCharacter::MoveForward(float Value) {
	bMovingForward = false;
	if (CanMove(Value)) {
		//find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);
	
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);

		bMovingForward = true;
	}
}

void AMainCharacter::MoveRight(float Value) {
	bMovingRight = false;
	if (CanMove(Value)) {
		//find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);

		bMovingRight = true;
	}
}

void AMainCharacter::Turn(float Value) {
	if (CanMove(Value)) {
		AddControllerYawInput(Value);
	}
}

void AMainCharacter::LookUp(float Value) {
	if (CanMove(Value)) {
		AddControllerPitchInput(Value);
	}
}

bool AMainCharacter::CanMove(float Value) {
	if (MainPlayerController) {
		return (Value != 0.0f)
			&& (!bAttacking)
			&& (MovementStatus != EMovementStatus::EMS_Dead)
			&& !MainPlayerController->bPauseMenuVisible;
	}
	return false;
}

void AMainCharacter::TurnAtRate(float Rate) {
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}


void AMainCharacter::LookUpAtRate(float Rate) {
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());

}

void AMainCharacter::DecrementHealth(float Amount) {
	
}

void AMainCharacter::Die() {
	if (MovementStatus == EMovementStatus::EMS_Dead) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && CombatMontage) {
		AnimInstance->Montage_Play(CombatMontage, 1.0f);
		AnimInstance->Montage_JumpToSection(FName("Death"));
	}
	SetMovementStatus(EMovementStatus::EMS_Dead);
}

void AMainCharacter::Jump() {
	if (MainPlayerController) if (MainPlayerController->bPauseMenuVisible) return;

	if (MovementStatus != EMovementStatus::EMS_Dead) {
		ACharacter::Jump();
	}
}


void AMainCharacter::DeathEnd() {
	GetMesh()->bPauseAnims = true;
	GetMesh()->bNoSkeletonUpdate = true;
}

void AMainCharacter::IncrementCoins(int32 Amount) {
	if (Coins + Amount >= MaxCoins) {
		Coins = MaxCoins;
	}
	else {
		Coins += Amount;
	}
}

void AMainCharacter::IncrementHealth(float Amount) {
	if (Health + Amount >= MaxHealth) {
		Health = MaxHealth;
	}
	else {
		Health += Amount;
	}
}


void AMainCharacter::SetMovementStatus(EMovementStatus Status) {
	MovementStatus = Status;
	if (MovementStatus == EMovementStatus::EMS_Sprinting) {
		GetCharacterMovement()->MaxWalkSpeed = SprintingSpeed;
	}
	else {
		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
	}
}

void AMainCharacter::ShiftKeyDown() {
	bShiftKeyDown = true;
}

void AMainCharacter::ShiftKeyUp() {
	bShiftKeyDown = false;
}

void AMainCharacter::LMBDown() {
	bLMBDown = true;

	if (MovementStatus == EMovementStatus::EMS_Dead) {
		return;
	}

	if (MainPlayerController) if (MainPlayerController->bPauseMenuVisible) return;


	if (ActiveOverlappingItem) {
		AWeapon* Weapon = Cast<AWeapon>(ActiveOverlappingItem);
		if (Weapon) {
			Weapon->Equip(this);
			SetActiveOverlappingItem(nullptr);
		}
	}
	else if (EquippedWeapon) {
		Attack();
	}
}
void AMainCharacter::LMBUp() {
	bLMBDown = false;
}

void AMainCharacter::ESCDown() {
	bESCDown = true;

	if (MainPlayerController) {
		MainPlayerController->TogglePauseMenu();
	}
}

void AMainCharacter::ESCUp() {
	bESCDown = false;
}

void AMainCharacter::SetEquippedWeapon(AWeapon* WeaponToSet) {
	if (EquippedWeapon) {
		EquippedWeapon->Destroy();
	}
	EquippedWeapon = WeaponToSet;
}

void AMainCharacter::Attack() {
	if (!bAttacking && (MovementStatus != EMovementStatus::EMS_Dead) && CheckValidStaminaStatus()) {
		bAttacking = true;
		SetInterpToEnemy(true);

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && CombatMontage) {
			int Section = FMath::RandRange(0, 1);
			switch (Section) {
			case 0:
				AnimInstance->Montage_Play(CombatMontage, 1.25f);
				AnimInstance->Montage_JumpToSection(FName("Attack_1"), CombatMontage);
				CalculateAttackStamina(StaminaForBasicAttack);
				break;
			case 1:
				AnimInstance->Montage_Play(CombatMontage, 1.8f);
				AnimInstance->Montage_JumpToSection(FName("Attack_2"), CombatMontage);
				CalculateAttackStamina(StaminaForBasicAttack);
				break;
			default:
				;
			}
		}
	}
}

void AMainCharacter::AttackEnd() {
	bAttacking = false;
	SetInterpToEnemy(false);
	if (bLMBDown) {
		Attack();
	}
}


void AMainCharacter::PlaySwingSound() {
	if (EquippedWeapon->SwingSound) {
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->SwingSound);
	}
}

void AMainCharacter::CalculateSprintStamina(float StaminaUsed)
{
	switch (StaminaStatus) {
	case EStaminaStatus::ESS_Normal:
		if (bShiftKeyDown) {
			if (Stamina - StaminaUsed <= MinSprintStamina) {
				SetStaminaStatus(EStaminaStatus::ESS_BelowMinimum);
				Stamina -= StaminaUsed;
			}
			else {
				Stamina -= StaminaUsed;
			}
			if (bMovingForward || bMovingRight) {
				SetMovementStatus(EMovementStatus::EMS_Sprinting);
			}
			else {
				SetMovementStatus(EMovementStatus::EMS_Normal);
			}
		}
		else {	//shift key up
			if (Stamina + StaminaUsed >= MaxStamina) {
				Stamina = MaxStamina;
			}
			else {
				Stamina += StaminaUsed;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
		}
		break;
	case EStaminaStatus::ESS_BelowMinimum:
		if (bShiftKeyDown) {
			if (Stamina - StaminaUsed <= 0.f) {
				SetStaminaStatus(EStaminaStatus::ESS_Exhausted);
				Stamina = 0.f;
				SetMovementStatus(EMovementStatus::EMS_Normal);
			}
			else {
				Stamina -= StaminaUsed;
				if (bMovingForward || bMovingRight) {
					SetMovementStatus(EMovementStatus::EMS_Sprinting);
				}
				else {
					SetMovementStatus(EMovementStatus::EMS_Normal);
				}
			}
		}
		else {	//shift key up
			if (Stamina >= MinSprintStamina) {
				SetStaminaStatus(EStaminaStatus::ESS_Normal);
				Stamina += StaminaUsed;
			}
			else {
				Stamina += StaminaUsed;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
		}
		break;
	case EStaminaStatus::ESS_Exhausted:
		if (bShiftKeyDown) {
			Stamina = 0.f;
		}
		else {	//shift key up
			SetStaminaStatus(EStaminaStatus::ESS_ExhaustedRecovering);
			Stamina += StaminaUsed;
		}
		SetMovementStatus(EMovementStatus::EMS_Normal);
		break;
	case EStaminaStatus::ESS_ExhaustedRecovering:
		if (Stamina + StaminaUsed >= MinSprintStamina) {
			SetStaminaStatus(EStaminaStatus::ESS_Normal);
			Stamina += StaminaUsed;
		}
		else {
			Stamina += StaminaUsed;
		}
		SetMovementStatus(EMovementStatus::EMS_Normal);
		break;
	default:
		;
	}
}

void AMainCharacter::CalculateAttackStamina(float StaminaUsed)
{
	switch (StaminaStatus) {
	case EStaminaStatus::ESS_Normal:
		if (Stamina - StaminaUsed <= MinSprintStamina) {
			SetStaminaStatus(EStaminaStatus::ESS_BelowMinimum);
			Stamina -= StaminaUsed;
		}
		else {
			Stamina -= StaminaUsed;
		}
	case EStaminaStatus::ESS_BelowMinimum:
		if (Stamina - StaminaUsed <= 0.f) {
			SetStaminaStatus(EStaminaStatus::ESS_Exhausted);
			Stamina = 0.f;
		}
		else {
			Stamina -= StaminaUsed;
		}
	case EStaminaStatus::ESS_Exhausted:
			Stamina = 0.f;
	case EStaminaStatus::ESS_ExhaustedRecovering:
		break;
	default:
		;
	}

}

bool AMainCharacter::CheckValidStaminaStatus() {
	if ((StaminaStatus == EStaminaStatus::ESS_Normal) || (StaminaStatus == EStaminaStatus::ESS_BelowMinimum)) {
		return true;
	}
	return false;
}

void AMainCharacter::SetInterpToEnemy(bool Interp) {
	bInterpToEnemy = Interp;
}

float AMainCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) {
	if (Health - DamageAmount <= 0.f) {
		Health = 0.f;
		Die();
		if (DamageCauser) {
			AEnemy* Enemy = Cast<AEnemy>(DamageCauser);
			if (Enemy) {
				Enemy->bHasValidTarget = false;
			}
		}
	}
	else {
		Health -= DamageAmount;
	}
	return DamageAmount;
}

void AMainCharacter::UpdateCombatTarget() {
	TArray<AActor*> OverlappingActors;

	GetOverlappingActors(OverlappingActors, EnemyFilter);

	if (OverlappingActors.Num() == 0) {
		if (MainPlayerController) {
			MainPlayerController->RemoveEnemyHealthBar();
		}
		return;
	}

	AEnemy* ClosestEnemy = Cast<AEnemy>(OverlappingActors[0]);

	if (ClosestEnemy) {
		FVector Location = GetActorLocation();
		float MinDistance = (ClosestEnemy->GetActorLocation() - Location).Size();

		for (auto Actor : OverlappingActors) {
			AEnemy* Enemy = Cast<AEnemy>(Actor);
			if (Enemy) {
				float DistanceToActor = (Enemy->GetActorLocation() - Location).Size();
				if (DistanceToActor < MinDistance) {
					MinDistance = DistanceToActor;
					ClosestEnemy = Enemy;
				}

			}
		}

		if (MainPlayerController) {
			MainPlayerController->DisplayEnemyHealthBar();
		}
		SetCombatTarget(ClosestEnemy);
		bHasCombatTarget = true;
	}
}

void AMainCharacter::SwitchLevel(FName LevelName) {
	UWorld* World = GetWorld();
	if (World) {
		//Saves the game before attempting to switch level and load a game in case the player has not already saved the game
		SaveGame();

		FString CurrentLevel = World->GetMapName();
		CurrentLevel.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

		FName CurrentLevelName(*CurrentLevel);
		if (CurrentLevelName != LevelName) {
			UGameplayStatics::OpenLevel(World, LevelName);
		}
	}
}


void AMainCharacter::SaveGame() {
	UFirstSaveGame* SaveGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	SaveGameInstance->CharacterStats.Health = Health;
	SaveGameInstance->CharacterStats.MaxHealth = MaxHealth;
	SaveGameInstance->CharacterStats.Stamina = Stamina;
	SaveGameInstance->CharacterStats.MaxStamina = MaxStamina;
	SaveGameInstance->CharacterStats.Coins = Coins;

	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	SaveGameInstance->CharacterStats.LevelName = MapName;


	if (EquippedWeapon) {
		SaveGameInstance->CharacterStats.WeaponName = EquippedWeapon->Name;
		SaveGameInstance->CharacterStats.bHasWeaponEquipped = true;
	}

	SaveGameInstance->CharacterStats.Location = GetActorLocation();
	SaveGameInstance->CharacterStats.Rotation = GetActorRotation();

	UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->PlayerName, SaveGameInstance->UserIndex);

}

void AMainCharacter::LoadGame(bool SetPosition) {
	UFirstSaveGame* LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->PlayerName, LoadGameInstance->UserIndex));


	if (!LoadGameInstance->CharacterStats.bHasWeaponEquipped && EquippedWeapon) {
		UE_LOG(LogTemp, Warning, TEXT("We did not have an equipped weapon, but EquippedWeapon was true"))
		EquippedWeapon->Destroy();
		SetEquippedWeapon(NULL);
	}

	Health = LoadGameInstance->CharacterStats.Health;
	MaxHealth = LoadGameInstance->CharacterStats.MaxHealth;
	Stamina = LoadGameInstance->CharacterStats.Stamina;
	MaxStamina = LoadGameInstance->CharacterStats.MaxStamina;
	Coins = LoadGameInstance->CharacterStats.Coins;


	if (SetPosition) {
		SetActorLocation(LoadGameInstance->CharacterStats.Location);
		SetActorRotation(LoadGameInstance->CharacterStats.Rotation);
	}

	if (WeaponStorage && LoadGameInstance->CharacterStats.bHasWeaponEquipped) {
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if (Weapons) {
			FString WeaponName = LoadGameInstance->CharacterStats.WeaponName;
			if (Weapons->WeaponMap.Contains(WeaponName)) {
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);
					WeaponToEquip->Equip(this);
			}
		}
	}

	SetMovementStatus(EMovementStatus::EMS_Normal);
	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;

	if (LoadGameInstance->CharacterStats.LevelName != TEXT("")) {
		FName LevelName(*LoadGameInstance->CharacterStats.LevelName);
		SwitchLevel(LevelName);
	}
}

void AMainCharacter::LoadGameNoSwitch()
{
	UFirstSaveGame* LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->PlayerName, LoadGameInstance->UserIndex));


	if (!LoadGameInstance->CharacterStats.bHasWeaponEquipped && EquippedWeapon) {
		UE_LOG(LogTemp, Warning, TEXT("We did not have an equipped weapon, but EquippedWeapon was true"))
			EquippedWeapon->Destroy();
		SetEquippedWeapon(NULL);
	}

	Health = LoadGameInstance->CharacterStats.Health;
	MaxHealth = LoadGameInstance->CharacterStats.MaxHealth;
	Stamina = LoadGameInstance->CharacterStats.Stamina;
	MaxStamina = LoadGameInstance->CharacterStats.MaxStamina;
	Coins = LoadGameInstance->CharacterStats.Coins;

	if (WeaponStorage && LoadGameInstance->CharacterStats.bHasWeaponEquipped) {
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if (Weapons) {
			FString WeaponName = LoadGameInstance->CharacterStats.WeaponName;
			if (Weapons->WeaponMap.Contains(WeaponName)) {
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);
				WeaponToEquip->Equip(this);
			}
		}
	}

	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;
	SetMovementStatus(EMovementStatus::EMS_Normal);
}


/*
void AMainCharacter::ShowPickupLocations() {
	for (FVector Location : PickupLocations) {
		UKismetSystemLibrary::DrawDebugSphere(this, Location, 25.f, 8, FLinearColor::Red, 5.f, 0.5f);
	}
}
*/

