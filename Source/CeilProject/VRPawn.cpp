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
#include "Kismet/GameplayStatics.h"

// Sets default values
AVRPawn::AVRPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	vr_origin = CreateDefaultSubobject<USceneComponent>(TEXT("vr_origin"));
	vr_origin->SetupAttachment(RootComponent);

	// skeletal meshes
	skeletal_attachment_point = CreateDefaultSubobject<USceneComponent>(TEXT("skeletal_attachment_point"));
	skeletal_attachment_point->SetupAttachment(vr_origin);

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
		original_avatar_standing_eyeball_height = 173.267804f;
		// original_avatar_sitting_eyeball_height = ...;
		original_foot_size = 34.0f;
	}

	else
	{
		// set same properties for female model
	}

	if (seated)
	{
		original_avatar_eyeball_height = original_avatar_sitting_eyeball_height;
		original_camera_height = original_sitting_camera_height;
		skeletal_mesh->SetAnimation(sitting_animation);
		skeletal_mesh->PlayAnimation(sitting_animation, true);
	}

	else
	{
		original_avatar_eyeball_height = original_avatar_standing_eyeball_height;
		original_camera_height = original_standing_camera_height;
		skeletal_mesh->SetAnimation(standing_animation);
		skeletal_mesh->PlayAnimation(standing_animation, true);
	}

	initialize_map_data();
}


// Called when the game starts or when spawned
void AVRPawn::BeginPlay()
{
	Super::BeginPlay();

	original_camera_location = camera_attachment_point->GetRelativeTransform().GetLocation();
}

// Called every frame
void AVRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (tick_counter == 1000 && calibrating_standing)
	{
		UE_LOG(LogTemp, Log, TEXT("Originally set camera height: %f\n"), original_standing_camera_height);
		original_standing_camera_height = sum_height / 1000.0f;
		original_camera_height = original_standing_camera_height;
		UE_LOG(LogTemp, Log, TEXT("New Average Camera Height: %f\n"), original_standing_camera_height);
		FString data = "Standing Eye Height: " + FString::SanitizeFloat(original_standing_camera_height) + "\n";
		write_data_to_file(data);
		tick_counter++;
	}

	else if (tick_counter == 1000 && !calibrating_standing)
	{
		original_sitting_camera_height = sum_height / 1000.0f;
		FString data = "Sitting Eye Height: " + FString::SanitizeFloat(original_sitting_camera_height) + "\n";
		write_data_to_file(data);
		tick_counter++;
	}

	else if (tick_counter < 1000)
	{
		UE_LOG(LogTemp, Log, TEXT("CALIBRATING STANDING EYE HEIGHT: TICK %d OUT OF 1000\n"), tick_counter);
		sum_height += camera->GetRelativeTransform().GetLocation().Z;
		tick_counter++;
	}

	skeletal_attachment_point->SetRelativeRotation(FRotator(0.0f, camera->GetComponentRotation().Yaw - 90.0f, 0.0f));
}

// Called to bind functionality to input
void AVRPawn::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAction("CycleOffset", IE_Released, this, &AVRPawn::cycle_offset);
	PlayerInputComponent->BindAction("SwapCalibration", IE_Released, this, &AVRPawn::swap_calibration);
	PlayerInputComponent->BindAction("ToggleSeating", IE_Released, this, &AVRPawn::toggle_seating);
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
	skeletal_mesh->SetRelativeScale3D(FVector(scale, scale * (foot_size / original_foot_size), scale));
}

void AVRPawn::scale_model_adjustment(float amount)
{
	float scale = (original_camera_height + camera_attachment_point->GetRelativeTransform().GetLocation().Z + amount) / original_avatar_eyeball_height;
	skeletal_mesh->SetRelativeScale3D(FVector(scale, scale * (foot_size / original_foot_size), scale));
}


void AVRPawn::cycle_offset()
{
	if (camera->GetForwardVector().Z > -0.1f && camera->GetForwardVector().Z < 0.2f)
	{
		float guess_height = camera->GetRelativeTransform().GetLocation().Z;
		FString guess_height_string = FString::SanitizeFloat(guess_height) + "\t";
		FString current_map_string = current_map.name.ToString() + "\t";
		FString offset_string = FString::SanitizeFloat(current_offset) + "\t";
		FString data_string = current_map_string + offset_string + guess_height_string + "\n";
		write_data_to_file(data_string);

		int map_index = FMath::RandRange(0, maps.Num() - 1);
		previous_map = current_map;
		while (maps[map_index].name == previous_map.name)
		{
			map_index = FMath::RandRange(0, maps.Num() - 1);
		}
		current_map = maps[map_index];

		for (int i = 0; i < maps.Num(); i++)
		{
			FLatentActionInfo latent_action_info;
			latent_action_info.CallbackTarget = this;
			latent_action_info.UUID = i;
			latent_action_info.Linkage = 0;

			if (maps[i].name == current_map.name)
			{
				UGameplayStatics::LoadStreamLevel(this, maps[i].name, true, true, latent_action_info);
			}

			else
			{
				UGameplayStatics::UnloadStreamLevel(this, maps[i].name, latent_action_info);
			}
		}


		FVector origin_camera_difference = vr_origin->GetComponentLocation() - camera->GetComponentLocation();
		vr_origin->SetWorldLocation(FVector(
			current_map.spawn_points[0].X + origin_camera_difference.X,
			current_map.spawn_points[0].Y + origin_camera_difference.Y,
			current_map.spawn_points[0].Z));

		float offset = FMath::RandRange(-80.0f, 80.0f);
		current_offset = offset;
		scale_model_offset(offset);

		// Move camera height offset
		camera_attachment_point->SetRelativeLocation(FVector(0.0f, 0.0f, original_camera_location.Z + offset));

		// move skeletal mesh to line eyeball up with camera
		FVector camera_forward = camera->GetForwardVector();
		FVector middle_eye_position = skeletal_mesh->GetSocketLocation("cc_base_m_eye");
		FVector skeletal_position = skeletal_mesh->GetComponentLocation();
		FVector skeletal_attachment_eye_difference = middle_eye_position - skeletal_position;
		skeletal_attachment_point->SetRelativeRotation(FRotator(0.0f, camera->GetComponentRotation().Yaw - 90.0f, 0.0f));
		skeletal_attachment_point->SetWorldLocation(FVector(
			current_map.spawn_points[0].X - skeletal_attachment_eye_difference.X * camera_forward.X,
			current_map.spawn_points[0].Y - skeletal_attachment_eye_difference.Y * camera_forward.Y,
			current_map.spawn_points[0].Z));
	}
	UE_LOG(LogTemp, Log, TEXT("camera Forward (x, y, z): (%f, %f, %f)\n"), camera->GetForwardVector().X, camera->GetForwardVector().Y, camera->GetForwardVector().Z);

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


void AVRPawn::swap_calibration()
{
	calibrating_standing = !calibrating_standing;
	tick_counter = 0;
}

void AVRPawn::set_thumbstick_y(float y)
{
	if (FGenericPlatformMath::Abs(y) > 0.1f)
	{
		float dt = GetWorld()->GetDeltaSeconds();
		float camera_movement = thumbstick_speed_scale * y * y * y* dt;
		FVector camera_location = camera_attachment_point->GetComponentLocation();
		camera_attachment_point->SetWorldLocation(FVector(camera_location.X, camera_location.Y, camera_location.Z + camera_movement));
		scale_model_adjustment(camera_movement);
	}
}


void AVRPawn::toggle_seating()
{
	seated = !seated;

	if (seated)
	{
		original_avatar_eyeball_height = original_avatar_sitting_eyeball_height;
		original_camera_height = original_sitting_camera_height;
		skeletal_mesh->SetAnimation(sitting_animation);
		skeletal_mesh->PlayAnimation(sitting_animation, true);
	}

	else
	{
		original_avatar_eyeball_height = original_avatar_standing_eyeball_height;
		original_camera_height = original_standing_camera_height;
		skeletal_mesh->SetAnimation(standing_animation);
		skeletal_mesh->PlayAnimation(standing_animation, true);
	}


}


void AVRPawn::initialize_map_data()
{
	MapData office;
	office.name = "BlueprintOffice";
	office.rotation = FRotator(0.0f, 0.0f, 0.0f);
	office.floor_height = 200.0f;
	office.spawn_points.Add(FVector(2430.0f, -390.0f, 200.0f));

	MapData realistic_room;
	realistic_room.name = "Room";
	realistic_room.rotation = FRotator(0.0f, 0.0f, 180.0f);
	realistic_room.floor_height = -0.75f; // Actual floor height is reported as 0, but this floor is thinner and doesn't touch the foot without this offset
	realistic_room.spawn_points.Add(FVector(-90.0f, 2930.0f, -0.75f));

	MapData scifi_bunk;
	scifi_bunk.name = "Scifi_Bunk";
	scifi_bunk.rotation = FRotator(0.0f, 0.0f, 180.0f);
	scifi_bunk.floor_height = -12821.8f;
	scifi_bunk.spawn_points.Add(FVector(315.0f, -99.0f, -12821.8f));
	
	MapData scifi_hallway;
	scifi_hallway.name = "SifiW";
	scifi_hallway.rotation = FRotator(0.0f, 0.0f, 0.0f);
	scifi_hallway.floor_height = -3512.0f;
	scifi_hallway.spawn_points.Add(FVector(580.0f, -1143.0f, -3512.0f));

	MapData sun_temple;
	sun_temple.name = "SunTemple";
	sun_temple.rotation = FRotator(0.0f, 0.0f, 0.0f);
	sun_temple.floor_height = 29.5f;
	sun_temple.spawn_points.Add(FVector(5.0f, 24040.0f, 29.5f));

	maps.Add(office);
	maps.Add(realistic_room);
	maps.Add(scifi_bunk);
	maps.Add(scifi_hallway);
	maps.Add(sun_temple);
}