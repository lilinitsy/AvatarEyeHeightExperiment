// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/BoxComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "LevelStreamingActor.generated.h"


UCLASS()
class CEILPROJECT_API ALevelStreamingActor : public AActor
{
	GENERATED_BODY()
	
public:	
	UPROPERTY(EditAnywhere, Category = "Volumes")
		UBoxComponent *overlap_volume;

	UPROPERTY(EditAnywhere, Category = "General Parameters")
		FName level_to_load;

	// Sets default values for this actor's properties
	ALevelStreamingActor();
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
		void OverlapBegins(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;	
};
