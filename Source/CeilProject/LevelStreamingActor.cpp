// Fill out your copyright notice in the Description page of Project Settings.

#include "LevelStreamingActor.h"


// Sets default values
ALevelStreamingActor::ALevelStreamingActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	overlap_volume = CreateDefaultSubobject<UBoxComponent>(TEXT("overlap_volume"));
	RootComponent = overlap_volume;

    overlap_volume->OnComponentBeginOverlap.AddUniqueDynamic(this, &ALevelStreamingActor::OverlapBegins);

}

// Called when the game starts or when spawned
void ALevelStreamingActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ALevelStreamingActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ALevelStreamingActor::OverlapBegins(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	APawn* pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (OtherActor == pawn) // && level_to_load != "")
	{
		FLatentActionInfo LatentInfo;
		UE_LOG(LogTemp, Log, TEXT("Pawn name: %s\n"), *pawn->GetHumanReadableName());
		//UGameplayStatics::LoadStreamLevel(this, level_to_load, true, true, LatentInfo);
	}
}
