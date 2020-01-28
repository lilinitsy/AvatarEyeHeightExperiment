// Fill out your copyright notice in the Description page of Project Settings.

#include "VRPawn.h"

#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFilemanager.h"
#include "HeadMountedDisplay.h"
#include "IXRTrackingSystem.h"
#include "Misc/FileHelper.h"
#include "UObject/ConstructorHelpers.h"
#include "XRMotionControllerBase.h"
#include "GenericPlatform/GenericPlatformMath.h"


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
	//skeletal_attachment_point->SetRelativeLocation(relative_skeletal_location);

	skeletal_mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("skeletal_mesh"));
	skeletal_mesh->SetupAttachment(skeletal_attachment_point);
	skeletal_mesh->bEditableWhenInherited = true;
	skeletal_mesh->SetMobility(EComponentMobility::Movable);

	// camera setup
	camera_attachment_point = CreateDefaultSubobject<USceneComponent>(TEXT("camera_attachment_point"));
	camera_attachment_point->SetupAttachment(vr_origin);
	camera = CreateDefaultSubobject<UCameraComponent>(TEXT("camera"));
	camera->SetupAttachment(camera_attachment_point);

	// controller setup
	// https://docs.unrealengine.com/en-US/Platforms/VR/DevelopVR/MotionController/index.html
	// https://docs.unrealengine.com/en-US/Gameplay/Input/index.html
	right_hand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("right_hand"));
	right_hand->SetRelativeLocationAndRotation(FVector::ZeroVector, FQuat::Identity);
	right_hand->SetRelativeScale3D(FVector::OneVector);
	right_hand->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	right_hand->SetupAttachment(camera);

	left_hand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("left_hand"));
	left_hand->SetRelativeLocationAndRotation(FVector::ZeroVector, FQuat::Identity);
	left_hand->SetRelativeScale3D(FVector::OneVector);
	left_hand->MotionSource = FXRMotionControllerBase::LeftHandSourceId;
	left_hand->SetupAttachment(camera);



	if (male_model)
	{
		static ConstructorHelpers::FObjectFinder<USkeletalMesh> skeletal_mesh_asset(TEXT("SkeletalMesh'/Game/Blueprints/avatar_male/male_withhead/avatar_male.avatar_male'"));
		if (skeletal_mesh_asset.Object)
		{
			skeletal_mesh->SetSkeletalMesh(skeletal_mesh_asset.Object);


		}
	}

	else
	{
		static ConstructorHelpers::FObjectFinder<USkeletalMesh> skeletal_mesh_asset(TEXT("SkeletalMesh'/Game/Blueprints/avatar_Eve/Eve_nohead/Eve_nohead.Eve_nohead'"));
		if (skeletal_mesh_asset.Object)
		{
			skeletal_mesh->SetSkeletalMesh(skeletal_mesh_asset.Object);
		}
	}


	if (seated)
	{
		skeletal_mesh->SetAnimation(sitting_animation);
	}

	else
	{
		skeletal_mesh->SetAnimation(standing_animation);
	}

	// Fill the offsets array
	offsets = fill_offset_TArray("eye-height-offsets.txt");
}


// Called when the game starts or when spawned
void AVRPawn::BeginPlay()
{
	Super::BeginPlay();
	original_camera_location = camera_attachment_point->GetComponentLocation();
}

// Called every frame
void AVRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	USkeletalMeshSocket *foot_l = (USkeletalMeshSocket*) skeletal_mesh->GetSocketByName("foot_l");
	//foot_l->


	if (tick_counter == 1000)
	{
		UE_LOG(LogTemp, Log, TEXT("Originally set camera height: %f\n"), original_camera_height);
		original_camera_height = sum_height / 1000.0f;
		UE_LOG(LogTemp, Log, TEXT("New Average Camera Height: %f\n"), original_camera_height);
	}

	else if (tick_counter < 1000)
	{
		UE_LOG(LogTemp, Log, TEXT("CALIBRATING EYE HEIGHT: TICK %d OUT OF 1000\n"), tick_counter);
		sum_height += camera->GetComponentLocation().Z - floor_height;
	}

	tick_counter++;
}

// Called to bind functionality to input
void AVRPawn::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAction("CycleOffset", IE_Released, this, &AVRPawn::cycle_offset);
	PlayerInputComponent->BindAction("RecordGuess", IE_Released, this, &AVRPawn::record_guess);
	PlayerInputComponent->BindAxis("MotionControllerRYAxis", this, &AVRPawn::set_thumbstick_y);
}


void AVRPawn::reset_hmd_origin()
{
	IHeadMountedDisplay *hmd = GEngine->XRSystem->GetHMDDevice();

	if (hmd && hmd->IsHMDEnabled())
	{
		GEngine->XRSystem->ResetOrientationAndPosition();
	}
}


void AVRPawn::scale_model_offset(float offset)
{
	float scale = (original_camera_height + offset) / original_avatar_eyeball_height;
	skeletal_mesh->SetRelativeScale3D(FVector(scale, scale, scale));
}

void AVRPawn::scale_model_adjustment(float amount)
{
	float scale = (original_camera_height + camera_attachment_point->GetRelativeTransform().GetLocation().Z + amount) / original_avatar_eyeball_height;
	skeletal_mesh->SetRelativeScale3D(FVector(scale, scale, scale));
}

// RNG for -80 to 80?
// list of 10 rooms, 11 offsets, 1 room, 1 offset, then redo
// keep track of trial n
void AVRPawn::cycle_offset()
{
	// Get offset and remove it from list
	int offset_index = FMath::RandRange(0, offsets.Num());
	float offset = offsets[offset_index];
	offsets.RemoveAt(offset_index);


	scale_model_offset(offset);

	// Move camera
	FVector camera_location = camera_attachment_point->GetComponentLocation();
	camera_attachment_point->SetWorldLocation(FVector(camera_location.X, camera_location.Y, original_camera_location.Z + offset));
	UE_LOG(LogTemp, Log, TEXT("cycle_offset: New Camera Z Position: %f\n"), camera_location.Z);
	UE_LOG(LogTemp, Log, TEXT("cycle_offset: Offset: %f\n"), offset);

	// Attempt to write offset to a file
	FString offset_string = FString::SanitizeFloat(offset) + "\t";
	write_data_to_file(offset_string);
}


void AVRPawn::write_data_to_file(FString data)
{
	FString save_directory = FPaths::ProjectDir();
	FString save_file = FString("data.txt");
	IPlatformFile& file = FPlatformFileManager::Get().GetPlatformFile();

	if (file.CreateDirectory(*save_directory))
	{
		FString absolute_file_path = save_directory + "/" + save_file;
		FFileHelper::SaveStringToFile(data, *absolute_file_path, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
	}
}


void AVRPawn::record_guess()
{
	// Do we measure it based on offset or based on true location?
	// How do we define true location while scaling?
	float guess_height = camera->GetRelativeTransform().GetLocation().Z;
	FString guess_height_string = FString::SanitizeFloat(guess_height) + "\n";
	write_data_to_file(guess_height_string);
}

void AVRPawn::set_thumbstick_y(float y)
{
	if (FGenericPlatformMath::Abs(y) > 0.1f)
	{
		float dt = GetWorld()->GetDeltaSeconds();
		float camera_movement = thumbstick_speed_scale * y * dt;
		FVector camera_location = camera_attachment_point->GetComponentLocation();
		camera_attachment_point->SetWorldLocation(FVector(camera_location.X, camera_location.Y, camera_location.Z + camera_movement));
		scale_model_adjustment(camera_movement);

		UE_LOG(LogTemp, Log, TEXT("set_thumbstick_y: Camera Z Position: %f\n"), camera_attachment_point->GetComponentLocation().Z);
		UE_LOG(LogTemp, Log, TEXT("set_thumbstick_y: Camera Movement: %f\n"), camera_movement);
	}
}


void AVRPawn::toggle_seating()
{
	// TODO
	if (!seated)
	{
		skeletal_mesh->SetActive(true);
		sitting_mesh->SetActive(false);
	}

	else
	{
		skeletal_mesh->SetActive(false);
		sitting_mesh->SetActive(true);
	}
}


TArray<float> AVRPawn::fill_offset_TArray(FString filename)
{
	TArray<float> offset_tarray;

	// Load file
	FString directory = FPaths::ProjectDir();
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
