// Fill out your copyright notice in the Description page of Project Settings.

#include "VRPawn.h"

#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFilemanager.h"
#include "HeadMountedDisplay.h"
#include "IXRTrackingSystem.h"
#include "Misc/FileHelper.h"
#include "UObject/ConstructorHelpers.h"
#include "XRMotionControllerBase.h"

// Sets default values
AVRPawn::AVRPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	vr_origin = CreateDefaultSubobject<USceneComponent>(TEXT("vr_origin"));
	vr_origin->SetupAttachment(RootComponent);

	// skeletal meshes
	skeletal_attachment_point = CreateDefaultSubobject<USceneComponent>(TEXT("skeletal_attachment_point"));
	skeletal_attachment_point->SetupAttachment(vr_origin);
	FVector relative_skeletal_location = FVector(0.0f, 0.0f, z_offset);
	skeletal_attachment_point->SetRelativeLocation(relative_skeletal_location);

	standing_mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("standing_mesh"));
	standing_mesh->SetupAttachment(skeletal_attachment_point);
	standing_mesh->bEditableWhenInherited = true;
	standing_mesh->SetMobility(EComponentMobility::Movable);

	// same code for sitting mesh setup


	// camera setup
	camera = CreateDefaultSubobject<UCameraComponent>(TEXT("camera"));
	camera->SetupAttachment(vr_origin);

	// controller setup
	// https://docs.unrealengine.com/en-US/Platforms/VR/DevelopVR/MotionController/index.html
	// https://docs.unrealengine.com/en-US/Gameplay/Input/index.html
	right_hand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("right_hand"));
	right_hand->SetRelativeLocationAndRotation(FVector::ZeroVector, FQuat::Identity);
	right_hand->SetRelativeScale3D(FVector::OneVector);
	right_hand->MotionSource = FXRMotionControllerBase::RightHandSourceId;

	left_hand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("left_hand"));
	left_hand->SetRelativeLocationAndRotation(FVector::ZeroVector, FQuat::Identity);
	left_hand->SetRelativeScale3D(FVector::OneVector);
	left_hand->MotionSource = FXRMotionControllerBase::LeftHandSourceId;
	left_hand->SetupAttachment(camera);
	right_hand->SetupAttachment(camera);

	//right_hand->


	if (male_model)
	{
		static ConstructorHelpers::FObjectFinder<USkeletalMesh> standing_mesh_asset(TEXT("SkeletalMesh'/Game/Blueprints/avatar_male/male_withhead/avatar_male.avatar_male'"));
		if (standing_mesh_asset.Object)
		{
			standing_mesh->SetSkeletalMesh(standing_mesh_asset.Object);

			// set animation
		}

		/*
		static ConstructorHelpers::FObjectFinder<USkeletalMesh> sitting_mesh_asset(TEXT("SkeletalMesh'/Game/Blueprints/avatar_Eve/Eve_nohead/Eve_nohead.Eve_nohead''"));
		if (sitting_mesh_asset.Object)
		{
			sitting_mesh->SetSkeletalMesh(sitting_mesh_asset.Object);

			// set animation
		}
		*/
	}

	else
	{
		static ConstructorHelpers::FObjectFinder<USkeletalMesh> standing_mesh_asset(TEXT("SkeletalMesh'/Game/Blueprints/avatar_Eve/Eve_nohead/Eve_nohead.Eve_nohead'"));
		if (standing_mesh_asset.Object)
		{
			standing_mesh->SetSkeletalMesh(standing_mesh_asset.Object);

			// set animation
		}

		
		/*
		static ConstructorHelpers::FObjectFinder<USkeletalMesh> sitting_mesh_asset(TEXT("SkeletalMesh'/Game/Blueprints/avatar_Eve/Eve_nohead/Eve_nohead.Eve_nohead''"));
		if (sitting_mesh_asset.Object)
		{
			sitting_mesh->SetSkeletalMesh(sitting_mesh_asset.Object);

			// set animation
		}
		*/
	}

	// Fill the offsets array
	offsets = fill_offset_TArray("eye-height-offsets.txt");

	// standing_mesh->SetPosition(standing_mesh->GetPosition() - 163.7);
	/*FVector skeletal_position = standing_mesh->GetSkeletalCenterOfMass();
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("BEFORE GETSKELETALCENTEROFMASS: %f %f %f\n"), skeletal_position.X, skeletal_position.Y, skeletal_position.Z));
	skeletal_position.Z -= 163.7f;
	standing_mesh->GetSkeletalCenterOfMass() = skeletal_position;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("BEFORE GETSKELETALCENTEROFMASS: %f %f %f\n"), standing_mesh->GetSkeletalCenterOfMass().X, standing_mesh->GetSkeletalCenterOfMass().Y, standing_mesh->GetSkeletalCenterOfMass().Z));
	*/

	// same thing for sitting_mesh



	//left_hand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("left_hand"));
	//left_hand->MotionSource = EControllerHand::Left;
}


// Called when the game starts or when spawned
void AVRPawn::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AVRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FRotator Rotation = Controller->GetControlRotation();
	const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);
	UE_LOG(LogTemp, Warning, TEXT("Direction: (%f %f %f)\n"), Direction.X, Direction.Y, Direction.Z);



}

// Called to bind functionality to input
void AVRPawn::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


void AVRPawn::reset_hmd_origin()
{
	IHeadMountedDisplay *hmd = GEngine->XRSystem->GetHMDDevice();

	if (hmd && hmd->IsHMDEnabled())
	{
		GEngine->XRSystem->ResetOrientationAndPosition();
	}
}


void AVRPawn::cycle_offset()
{
	// Bind the Skeletal Mesh to the camera position / rotation
	// Camera position and orientation is dependent on vr_origin
	FVector camera_location = vr_origin->GetComponentLocation();
	FRotator camera_rotation = vr_origin->GetComponentRotation();

	skeletal_attachment_point->SetRelativeLocation(camera_location);
	skeletal_attachment_point->SetRelativeRotation(camera_rotation);

	// do smth with offsets here
}


void AVRPawn::toggle_seating()
{
	// TODO
	if (!seated)
	{
		standing_mesh->SetActive(true);
		sitting_mesh->SetActive(false);
	}

	else
	{
		standing_mesh->SetActive(false);
		sitting_mesh->SetActive(true);
	}
}


TArray<float> AVRPawn::fill_offset_TArray(FString filename)
{
	TArray<float> offset_tarray;

	// Load file
	FString directory = FPaths::GameDir(); // FPaths::Combine(FPaths::GameContentDir(), "Data");
	TArray<FString> string_offsets;
	IPlatformFile& file = FPlatformFileManager::Get().GetPlatformFile();
	if (file.CreateDirectory(*directory))
	{
		FString myFile = directory + filename;
		FFileHelper::LoadFileToStringArray(string_offsets, *myFile);
	}

	// Convert each number to a float
	for (int i = 0; i < string_offsets.Num(); i++)
	{
		const char *char_offset = TCHAR_TO_ANSI(*string_offsets[i]);
		offset_tarray.Add(atof(char_offset));
	}

	return offset_tarray;
}
