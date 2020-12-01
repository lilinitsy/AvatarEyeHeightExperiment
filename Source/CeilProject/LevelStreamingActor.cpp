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

// CHECK https://forums.unrealengine.com/development-discussion/c-gameplay-programming/12588-level-streaming-with-c

// Called when the game starts or when spawned
void ALevelStreamingActor::BeginPlay()
{
	Super::BeginPlay();

	FLatentActionInfo latent_info;
	latent_info.CallbackTarget = this;
	//latent_info.ExecutionFunction = ""
	latent_info.UUID = 1;
	latent_info.Linkage = 0;
	//UGameplayStatics::OpenLevel(this, level_to_load);
	UGameplayStatics::LoadStreamLevel(this, level_to_load, true, true, latent_info);
	UGameplayStatics::UnloadStreamLevel(this, "BlueprintOffice", latent_info, true);
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
		FLatentActionInfo latent_info;
		UE_LOG(LogTemp, Log, TEXT("Pawn name: %s\n"), *pawn->GetHumanReadableName());
		//UGameplayStatics::LoadStreamLevel(this, level_to_load, true, true, LatentInfo);
	}
}
