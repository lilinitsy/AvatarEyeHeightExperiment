// Fill out your copyright notice in the Description page of Project Settings.

#include "VRPawn.h"

#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFilemanager.h"
#include "HeadMountedDisplay.h"
#include "IXRTrackingSystem.h"
#include "Kismet/GameplayStatics.h"
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

	// Set properties specific to model gender
	if (male_model)
	{
		original_avatar_standing_eyeball_height = 173.267804f;
		original_avatar_sitting_eyeball_height = 130.1741255;
		original_foot_size = 34.0f;
	}

	else
	{
		// set same properties for female model
		original_avatar_standing_eyeball_height = 170.497841f;
		original_avatar_sitting_eyeball_height = 130.1741105f;
		original_foot_size = 30.0f;
	}

	// Set properties specific to seated or not seated
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

	// Flood the guess with ridiculously large value
	for (int i = 0; i < 3; i++)
	{
		guesses[i] = 999999.0f;
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


	// Write out the standing height values
	if (tick_counter == 500 && calibrating_standing)
	{
		original_standing_camera_height = sum_height / 500.0f;
		original_camera_height = original_standing_camera_height;
		UE_LOG(LogTemp, Log, TEXT("New Standing Camera Height: %f\n"), original_standing_camera_height);
		UE_LOG(LogTemp, Log, TEXT("MIN STANDING HEIGHT: %f\n"), min_standing_height);
		UE_LOG(LogTemp, Log, TEXT("MAX STANDING HEIGHT: %f\n"), max_standing_height);
		FString data = "Standing Eye Height: " + FString::SanitizeFloat(original_standing_camera_height) + "\n";
		data += "Min Standing Eye Height: " + FString::SanitizeFloat(min_standing_height);
		data += "Max Standing Eye Height: " + FString::SanitizeFloat(max_standing_height);
		write_data_to_file(data);
		tick_counter++;
	}

	// Write out the sitting height values
	else if (tick_counter == 500 && !calibrating_standing)
	{
		original_sitting_camera_height = sum_height / 500.0f;
		UE_LOG(LogTemp, Log, TEXT("New Sitting Camera Height: %f\n"), original_sitting_camera_height);
		UE_LOG(LogTemp, Log, TEXT("MIN SITTING HEIGHT: %f\n"), min_sitting_height);
		UE_LOG(LogTemp, Log, TEXT("MAX SITTING HEIGHT: %f\n"), max_sitting_height);
		FString data = "Sitting Eye Height: " + FString::SanitizeFloat(original_sitting_camera_height) + "\n";
		data += "Min Sitting Eye Height: " + FString::SanitizeFloat(min_sitting_height) + "\n";
		data += "Max Sitting Eye Height: " + FString::SanitizeFloat(max_sitting_height) + "\n";
		write_data_to_file(data);
		tick_counter++;
	}

	else if (tick_counter < 500)
	{
		if (calibrating_standing && camera->GetRelativeTransform().GetLocation().Z < min_standing_height && tick_counter > 0)
		{
			min_standing_height = camera->GetRelativeTransform().GetLocation().Z;
			UE_LOG(LogTemp, Log, TEXT("CALIBRATING STANDING EYE HEIGHT: TICK %d OUT OF 500 MINHEIGHT: %f\n"), tick_counter, min_standing_height);
		}

		if (calibrating_standing && camera->GetRelativeTransform().GetLocation().Z > max_standing_height && tick_counter > 0)
		{
			max_standing_height = camera->GetRelativeTransform().GetLocation().Z;
			UE_LOG(LogTemp, Log, TEXT("CALIBRATING STANDING EYE HEIGHT: TICK %d OUT OF 500 MAXHEIGHT: %f\n"), tick_counter, max_standing_height);
		}

		if (!calibrating_standing && camera->GetRelativeTransform().GetLocation().Z < min_sitting_height && tick_counter > 0)
		{
			min_sitting_height = camera->GetRelativeTransform().GetLocation().Z;
			UE_LOG(LogTemp, Log, TEXT("CALIBRATING SITTING EYE HEIGHT: TICK %d OUT OF 500 MINHEIGHT: %f\n"), tick_counter, min_sitting_height);
		}

		if (!calibrating_standing && camera->GetRelativeTransform().GetLocation().Z > max_sitting_height && tick_counter > 0)
		{
			max_sitting_height = camera->GetRelativeTransform().GetLocation().Z;
			UE_LOG(LogTemp, Log, TEXT("CALIBRATING SITTING EYE HEIGHT: TICK %d OUT OF 500 MAXHEIGHT: %f\n"), tick_counter, max_sitting_height);
		}

		sum_height += camera->GetRelativeTransform().GetLocation().Z;
		tick_counter++;
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
	if (camera->GetForwardVector().Z > -0.15f && camera->GetForwardVector().Z < 0.25f)
	{
		guesses[guess_counter] = camera->GetRelativeTransform().GetLocation().Z;
		guess_counter++;
		if (guesses[2] != 9999.0f)
		{
			// Record the everything for this trial and write to file
			FString guess_height_string;
			float avg_guess_height = 0.0f;
			for (int i = 0; i < 3; i++)
			{
				guess_height_string += FString::SanitizeFloat(guesses[i]) + "\t";
				avg_guess_height += guesses[i];
			}
			avg_guess_height /= 3.0f;
			guess_height_string += FString::SanitizeFloat(avg_guess_height) + "\t";
			FString current_map_string = current_map.name.ToString() + "\t";
			FString offset_string = FString::SanitizeFloat(current_offset) + "\t";
			FString camera_height_string = FString::SanitizeFloat(original_camera_height) + "\t";
			FString data_string = current_map_string + offset_string + guess_height_string + camera_height_string + "\n";
			write_data_to_file(data_string);

			// Fade camera to black
			UGameplayStatics::GetPlayerController(GetWorld(), 0)->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 2.0f, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f), false, false);

			// Pick a new random map
			int map_index = FMath::RandRange(0, maps.Num() - 1);
			previous_map = current_map;
			while (maps[map_index].name == previous_map.name)
			{
				map_index = FMath::RandRange(0, maps.Num() - 1);
			}
			current_map = maps[map_index];
		
			// Set the vr_origin so the player will spawn at the right location no matter where they're standing
			FVector origin_camera_difference = vr_origin->GetComponentLocation() - camera->GetComponentLocation();
			vr_origin->SetWorldLocation(FVector(
				current_map.spawn_points[0].X + origin_camera_difference.X,
				current_map.spawn_points[0].Y + origin_camera_difference.Y,
				current_map.spawn_points[0].Z));

			// Get random offset
			float offset = FMath::RandRange(-80.0f, 80.0f);
			current_offset = offset;
			scale_model_offset(offset);

			// Move camera height by offset
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

			// Unload all levels except the current map
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

			for (int i = 0; i < 3; i++)
			{
				guess_counter = 0;
				guesses[i] = 9999.0f;
			}
		}
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
	sum_height = 0.0f;
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
	office.spawn_points.Add(FVector(100.0f, -1199.595703f, 192.002228f));

	MapData realistic_room;
	realistic_room.name = "Room";
	realistic_room.rotation = FRotator(0.0f, 0.0f, 180.0f);
	realistic_room.floor_height = -0.75f; // Actual floor height is reported as 0, but this floor is thinner and doesn't touch the foot without this offset
	realistic_room.spawn_points.Add(FVector(-106.195107f, 3177.424316f, -0.75f));

	MapData scifi_bunk;
	scifi_bunk.name = "Scifi_Bunk";
	scifi_bunk.rotation = FRotator(0.0f, 0.0f, 180.0f);
	scifi_bunk.floor_height = -12821.8f;
	scifi_bunk.spawn_points.Add(FVector(-61.999939f, -191.999573, -12821.8f));
	
	MapData scifi_hallway;
	scifi_hallway.name = "SifiW";
	scifi_hallway.rotation = FRotator(0.0f, 0.0f, 0.0f);
	scifi_hallway.floor_height = -3512.0f;
	scifi_hallway.spawn_points.Add(FVector(405.0, -165.0f, -3512.0f));

	MapData sun_temple;
	sun_temple.name = "SunTemple";
	sun_temple.rotation = FRotator(0.0f, 0.0f, 0.0f);
	sun_temple.floor_height = 29.5f;
	sun_temple.spawn_points.Add(FVector(25.398819f, 22351.001953f, 29.5f));

	MapData lightroom_day;
	lightroom_day.name = "Lightroom_day";
	lightroom_day.rotation = FRotator(0.0f, 0.0f, 0.0f);
	lightroom_day.floor_height = 36.0f;
	lightroom_day.spawn_points.Add(FVector(0.0f, 0.0f, 36.0f));

	MapData lightroom_night;
	lightroom_night.name = "Lightroom_night";
	lightroom_night.rotation = FRotator(0.0f, 0.0f, 0.0f);
	lightroom_night.floor_height = 36.0f;
	lightroom_night.spawn_points.Add(FVector(0.0f, 0.0f, 36.0f));

	MapData berlin_flat;
	berlin_flat.name = "xoio_berlinflat";
	berlin_flat.rotation = FRotator(0.0f, 0.0f, 0.0f);
	berlin_flat.floor_height = 0.0f;
	berlin_flat.spawn_points.Add(FVector(86.0f, -78.0f, 0.0f));

	MapData zen_walkway_wood;
	zen_walkway_wood.name = "Zen_vis";
	zen_walkway_wood.rotation = FRotator(0.0f, 0.0f, 0.0f);
	zen_walkway_wood.floor_height = 12.75f;
	zen_walkway_wood.spawn_points.Add(FVector(402.5f, 315.0f, 12.75f));


	maps.Add(office);
	maps.Add(realistic_room);
	maps.Add(scifi_bunk);
	maps.Add(scifi_hallway);
	maps.Add(sun_temple);
	maps.Add(lightroom_day);
	maps.Add(lightroom_night);
	maps.Add(berlin_flat);
}
