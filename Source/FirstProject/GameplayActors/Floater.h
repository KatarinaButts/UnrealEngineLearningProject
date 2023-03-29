// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Floater.generated.h"

UCLASS()
class FIRSTPROJECT_API AFloater : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFloater();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "ActorMeshComponents")
	UStaticMeshComponent* StaticMesh;

	// Location used by SetActorLocation() when BeginPlay is called
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "FloaterVariables")
	FVector InitialLocation;

	// Location of the Actor when dragged in from the editor
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "FloaterVariables")
	FVector PlacedLocation;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "FloaterVariables")
	FVector WorldOrigin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloaterVariables")
	FVector InitialDirection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloaterVariables")
	bool bShouldFloat;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FloaterVariables")
	bool bInitializeFloaterLocations;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "FloaterVariables")
	FVector InitialForce;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "FloaterVariables")
	FVector InitialTorque;

private:
	float RunningTime;

	float BaseZLocation;

public:
	//Amplitude, how much we oscillate up and down
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloaterVariables")
	float A;

	//Period, 2*PI/ABS(B): if B gets bigger, the period (time bw oscillations) gets smaller
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloaterVariables")
	float B;

	//Phase Shift, C/B, where we start on the sine graph
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloaterVariables")
	float C;

	//Vertical Shift, moves the graph up and down in the z direction
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FloaterVariables")
	float D;



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
