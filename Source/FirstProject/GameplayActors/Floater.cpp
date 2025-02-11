// Fill out your copyright notice in the Description page of Project Settings.


#include "Floater.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AFloater::AFloater()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CustomStaticMesh"));

	InitialLocation = FVector(0.0f, 0.0f, 0.0f);
	PlacedLocation = FVector(0.0f, 0.0f, 0.0f);
	WorldOrigin = FVector(0.0f, 0.0f, 0.0f);
	InitialDirection = FVector(0.0f, 0.0f, 0.0f);

	bInitializeFloaterLocations = false;
	bShouldFloat = false;

	RunningTime = 0.0f;

	A = 0.0f;
	B = 0.0f;
	C = 0.0f;
	D = 0.0f;


	//InitialForce = FVector(200000.0f, 0.0f, 0.0f);
	//InitialTorque = FVector(200000.0f, 0.0f, 0.0f);
}

// Called when the game starts or when spawned
void AFloater::BeginPlay()
{
	Super::BeginPlay();

	float InitialX = FMath::FRandRange(-500.0f, 500.0f);
	float InitialY = FMath::FRandRange(-500.0f, 500.0f);
	float InitialZ = FMath::FRandRange(0.0f, 500.0f);
	InitialLocation.X = InitialX;
	InitialLocation.Y = InitialY;
	InitialLocation.Z = InitialZ;

	//InitialLocation *= 200.0f;



	PlacedLocation = GetActorLocation();

	if (bInitializeFloaterLocations)
	{
		SetActorLocation(InitialLocation);
	}

	BaseZLocation = PlacedLocation.Z;

	//FHitResult HitResult;
	//FVector LocalOffset = FVector(200.0f, 0.0f, 0.0f);


	//FRotator Rotation = FRotator(0.0f, 0.0f, 0.0f);

	//AddActorLocalRotation(Rotation);
	//AddActorLocalOffset(LocalOffset, true, &HitResult);
	//AddActorWorldOffset(LocalOffset, true, &HitResult);


	//StaticMesh->AddForce(InitialForce);
	//StaticMesh->AddTorqueInRadians(InitialTorque);






}

// Called every frame
void AFloater::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShouldFloat) {
		FVector NewLocation = GetActorLocation();

		NewLocation.Z = BaseZLocation + A * FMath::Sin(B * RunningTime - C) + D;



		SetActorLocation(NewLocation);
		RunningTime += DeltaTime;

		/*
		FHitResult HitResult;
		AddActorLocalOffset(InitialDirection, true, &HitResult);

		FVector HitLocation = HitResult.Location;
		//UE_LOG(LogTemp, Warning, TEXT("Hit Location: X = %f, Y = %f, Z = %f"), HitLocation.X, HitLocation.Y, HitLocation.Z);
		*/
	}


	//FRotator Rotation = FRotator(0.0f, 1.0f, 0.0f);
	//AddActorLocalRotation(Rotation);
	//AddActorWorldRotation(Rotation);


}

