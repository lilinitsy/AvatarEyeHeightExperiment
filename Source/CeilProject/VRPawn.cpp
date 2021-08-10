// Fill out your copyright notice in the Description page of Project Settings.

#include "VRPawn.h"

#include "Components/InputComponent.h"
#include "Containers/UnrealString.h"
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
#include "Misc/App.h"
#include "XRMotionControllerBase.h"
#include "Kismet/KismetMathLibrary.h"

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
	//skeletal_mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);

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
	if(male_model)
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
	if(seated)
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

	//standing_trials_currently = FMath::RandRange(0, 1);

	initialize_map_data();
	map_list = part_of_group_a ? map_list_a : map_list_b;
	maps = map_list;
}


// Called when the game starts or when spawned
void AVRPawn::BeginPlay()
{
	Super::BeginPlay();

	original_camera_location = camera_attachment_point->GetRelativeTransform().GetLocation();
	body_current_position.SetLocation(skeletal_mesh->GetRelativeLocation());
	body_current_position.SetRotation(FQuat(skeletal_mesh->GetRelativeRotation()));

	// Specific for Oculus devices
	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);

	/*FVector origin_camera_difference = vr_origin->GetComponentLocation() - camera->GetComponentLocation();
	vr_origin->SetWorldLocation(FVector(
		current_map.spawn_points[0].X + origin_camera_difference.X,
		current_map.spawn_points[0].Y + origin_camera_difference.Y,
		current_map.spawn_points[0].Z));*/

	// move skeletal mesh out of view
	FVector skeletal_mesh_location = skeletal_mesh->GetComponentLocation();
	skeletal_mesh_location.Y -= 1000.0f;
	skeletal_mesh_location.Z -= 200.0f;

	skeletal_mesh->SetWorldLocation(skeletal_mesh_location);
	skeletal_mesh->SetHiddenInGame(true);

	FString group_data = "PART OF GROUP A: " + FString::FromInt(part_of_group_a) + "\n";
	write_data_to_file(group_data);
}


// Called every frame
void AVRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	compute_headset_motion_information();


	/*if (standing_calibrated && sitting_calibrated && commence_standing_trials_2_started && commence_standing_trials_2_started)
	{
		UGameplayStatics::PlaySound2D(this, calibration_completed, 5.0f);

	}*/

	if (sitting_calibrated)
	{
		skeletal_mesh->SetHiddenInGame(false);
	}


	if (calibration_started)
	{
		// TODO: Don't write out intermediate values to data file

		// Write out the standing height values
		if (tick_counter == 300 && calibrating_standing)
		{
			original_standing_camera_height = sum_height / tick_counter;
			original_camera_height = original_standing_camera_height;
			UE_LOG(LogTemp, Log, TEXT("New Standing Camera Height: %f\n"), original_standing_camera_height);
			UE_LOG(LogTemp, Log, TEXT("MIN STANDING HEIGHT: %f\n"), min_standing_height);
			UE_LOG(LogTemp, Log, TEXT("MAX STANDING HEIGHT: %f\n"), max_standing_height);
			
			FString data = "Standing Eye Height: " + FString::SanitizeFloat(original_standing_camera_height) + "\n";
			data += "Min Standing Eye Height: " + FString::SanitizeFloat(min_standing_height) + "\n";
			data += "Max Standing Eye Height: " + FString::SanitizeFloat(max_standing_height) + "\n";
			write_data_to_file(data);

			calibration_started = false;
			standing_calibrated = true;

			tick_counter++;
		}

		// Write out the sitting height values
		else if (tick_counter == 300 && !calibrating_standing)
		{
			original_sitting_camera_height = sum_height / tick_counter;
			UE_LOG(LogTemp, Log, TEXT("New Sitting Camera Height: %f\n"), original_sitting_camera_height);
			UE_LOG(LogTemp, Log, TEXT("MIN SITTING HEIGHT: %f\n"), min_sitting_height);
			UE_LOG(LogTemp, Log, TEXT("MAX SITTING HEIGHT: %f\n"), max_sitting_height);
			
			FString data = "Sitting Eye Height: " + FString::SanitizeFloat(original_sitting_camera_height) + "\n";
			data += "Min Sitting Eye Height: " + FString::SanitizeFloat(min_sitting_height) + "\n";
			data += "Max Sitting Eye Height: " + FString::SanitizeFloat(max_sitting_height) + "\n";
			write_data_to_file(data);
			
			calibration_started = false;
			sitting_calibrated = true;

			tick_counter++;
		}

		else if (tick_counter < 300)
		{
			if (calibrating_standing && camera->GetRelativeTransform().GetLocation().Z < min_standing_height && tick_counter > 0)
			{
				min_standing_height = camera->GetRelativeTransform().GetLocation().Z;
				UE_LOG(LogTemp, Log, TEXT("CALIBRATING STANDING EYE HEIGHT: TICK %d OUT OF 300 MINHEIGHT: %f\n"), tick_counter, min_standing_height);
			}

			if (calibrating_standing && camera->GetRelativeTransform().GetLocation().Z > max_standing_height && tick_counter > 0)
			{
				max_standing_height = camera->GetRelativeTransform().GetLocation().Z;
				UE_LOG(LogTemp, Log, TEXT("CALIBRATING STANDING EYE HEIGHT: TICK %d OUT OF 300 MAXHEIGHT: %f\n"), tick_counter, max_standing_height);
			}

			if (!calibrating_standing && camera->GetRelativeTransform().GetLocation().Z < min_sitting_height && tick_counter > 0)
			{
				min_sitting_height = camera->GetRelativeTransform().GetLocation().Z;
				UE_LOG(LogTemp, Log, TEXT("CALIBRATING SITTING EYE HEIGHT: TICK %d OUT OF 300 MINHEIGHT: %f\n"), tick_counter, min_sitting_height);
			}

			if (!calibrating_standing && camera->GetRelativeTransform().GetLocation().Z > max_sitting_height && tick_counter > 0)
			{
				max_sitting_height = camera->GetRelativeTransform().GetLocation().Z;
				UE_LOG(LogTemp, Log, TEXT("CALIBRATING SITTING EYE HEIGHT: TICK %d OUT OF 300 MAXHEIGHT: %f\n"), tick_counter, max_sitting_height);
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

		map_time += DeltaTime;
	}

	calculate_movement();
}

// Called to bind functionality to input
void AVRPawn::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAction("CycleOffset", IE_Released, this, &AVRPawn::cycle_offset);
	PlayerInputComponent->BindAction("SwapCalibration", IE_Released, this, &AVRPawn::swap_calibration);
	PlayerInputComponent->BindAction("ToggleSeating", IE_Released, this, &AVRPawn::toggle_seating);
	PlayerInputComponent->BindAxis("MotionControllerRYAxis", this, &AVRPawn::set_thumbstick_y);
	PlayerInputComponent->BindAxis("MotionControllerRYAxisNegative", this, &AVRPawn::set_thumbstick_y_negative);
}


void AVRPawn::reset_hmd_origin()
{
	IHeadMountedDisplay *hmd = GEngine->XRSystem->GetHMDDevice();

	if(hmd && hmd->IsHMDEnabled())
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
		//reset_ik_parameters();

		// Record the everything for this trial and write to file
		FString map_time_string = FString::SanitizeFloat(map_time);
		FString guess_height_string = FString::SanitizeFloat(total_guessed_offset + camera->GetRelativeTransform().GetLocation().Z) + "\t";
		FString nth_trial = FString::FromInt(trial_num) + "\t";
		FString current_map_string = current_map.name.ToString() + "\t";
		FString offset_string = FString::SanitizeFloat(current_offset) + "\t";
		FString original_camera_height_string = FString::SanitizeFloat(original_camera_height) + "\t";
		FString data_string = nth_trial + current_map_string + offset_string + guess_height_string + original_camera_height_string + map_time_string + "\n";
		write_data_to_file(data_string);

		// Fade camera to black
		UGameplayStatics::GetPlayerController(GetWorld(), 0)->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 2.0f, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f), false, false);

		// Make sure that maps has entries. Reset if not (have gone through one full rotation)
		if (maps.Num() == 0)
		{
			maps = map_list;
		}

		// Pick a new random map
		int map_index = FMath::RandRange(0, maps.Num() - 1);
		previous_map = current_map;
		while (maps[map_index].name == previous_map.name)
		{
			map_index = FMath::RandRange(0, maps.Num() - 1);
		}
		current_map = maps[map_index];
		maps.RemoveAt(map_index);

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


		// Reset the IK params
		last_camera_position.SetLocation(camera->GetRelativeLocation());
		body_target_position.SetLocation(camera->GetRelativeLocation());
		body_current_position.SetLocation(camera->GetRelativeLocation());

		last_camera_position.SetRotation(FQuat(camera->GetRelativeRotation()));
		body_target_position.SetRotation(FQuat(camera->GetRelativeRotation()));
		body_current_position.SetRotation(FQuat(camera->GetRelativeRotation()));

		movement_direction = 0.0f;
		movement_speed = 0.0f;
		alpha = 0.0f;

		// Unload all levels except the current map
		for (int i = 0; i < map_list.Num(); i++)
		{
			FLatentActionInfo latent_action_info;
			latent_action_info.CallbackTarget = this;
			latent_action_info.UUID = i;
			latent_action_info.Linkage = 0;

			if (map_list[i].name == current_map.name)
			{
				UGameplayStatics::LoadStreamLevel(this, map_list[i].name, true, true, latent_action_info);
			}

			else
			{
				UGameplayStatics::UnloadStreamLevel(this, map_list[i].name, latent_action_info, true);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("Trial num: %d\n"), trial_num);

		map_time = 0.0f;
		total_guessed_offset = 0.0f;
		trial_num++;
	}

	else
	{
		UE_LOG(LogTemp, Log, TEXT("User must look straight ahead to pull the trigger"));
	}
}


void AVRPawn::write_data_to_file(FString data)
{
	FString save_directory = FPaths::ProjectDir();
	FString save_file = FString("data.txt");
	IPlatformFile& file = FPlatformFileManager::Get().GetPlatformFile();

	if(file.CreateDirectory(*save_directory))
	{
		FString absolute_file_path = save_directory + "/" + save_file;
		FFileHelper::SaveStringToFile(data, *absolute_file_path, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
	}
}


// For figuring out headset motion vectors
void AVRPawn::write_headset_motion_data_to_file(FString rot_data, FString pos_data)
{
	FString save_directory = FPaths::ProjectDir();
	FString save_file = FString("headset_rotation.txt");
	IPlatformFile& file = FPlatformFileManager::Get().GetPlatformFile();

	if (file.CreateDirectory(*save_directory))
	{
		FString absolute_file_path = save_directory + "/" + save_file;
		FFileHelper::SaveStringToFile(rot_data, *absolute_file_path, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
	}

	save_file = FString("headset_position.txt");
	file = FPlatformFileManager::Get().GetPlatformFile();

	if (file.CreateDirectory(*save_directory))
	{
		FString absolute_file_path = save_directory + "/" + save_file;
		FFileHelper::SaveStringToFile(pos_data, *absolute_file_path, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
	}
}

// For figuring out headset motion vectors
void AVRPawn::compute_headset_motion_information()
{
	FRotator current_direction_vec = camera->GetComponentRotation();
	FVector current_camera_position_vec = camera->GetRelativeTransform().GetLocation();

	if (current_direction_vec.Pitch < min_y_camera_look.Pitch)
	{
		min_y_camera_look = current_direction_vec;
		min_y_camera_look_pos = current_camera_position_vec;
	}

	if (current_direction_vec.Pitch > max_y_camera_look.Pitch)
	{
		max_y_camera_look = current_direction_vec;
		max_y_camera_look_pos = current_camera_position_vec;
	}

	FString rot_data = FString::SanitizeFloat(current_direction_vec.Pitch) + ", " + FString::SanitizeFloat(current_direction_vec.Roll) + ", " + FString::SanitizeFloat(current_direction_vec.Yaw) + "\n";
	FString pos_data = FString::SanitizeFloat(current_camera_position_vec.X) + ", " + FString::SanitizeFloat(current_camera_position_vec.Y) + ", " + FString::SanitizeFloat(current_camera_position_vec.Z) + "\n";

	write_headset_motion_data_to_file(rot_data, pos_data);
}


// Caused by pressing A
void AVRPawn::swap_calibration()
{
	calibrating_standing = !calibrating_standing;
	UE_LOG(LogTemp, Log, TEXT("================\nCALIBRATION SWAPPED\n========"));
	calibration_started = true;
	sum_height = 0.0f;
	tick_counter = 0;
}


// Something broke, so have to have two versions of this function and two axis mappings :P
void AVRPawn::set_thumbstick_y(float y)
{
	if(FGenericPlatformMath::Abs(y) > 0.1f)
	{
		UE_LOG(LogTemp, Log, TEXT("Action mapping for SET_THUMBSTICK_Y selected"));
		float dt = GetWorld()->GetDeltaSeconds();
		float camera_movement = thumbstick_speed_scale * FGenericPlatformMath::Abs(y) * y * dt;
		total_guessed_offset += camera_movement;

		FVector camera_location = camera_attachment_point->GetComponentLocation();
		camera_attachment_point->SetWorldLocation(FVector(camera_location.X, camera_location.Y, camera_location.Z + camera_movement));
		scale_model_adjustment(camera_movement);
	}
}


void AVRPawn::set_thumbstick_y_negative(float y)
{
	if (FGenericPlatformMath::Abs(y) > 0.1f)
	{
		UE_LOG(LogTemp, Log, TEXT("Action mapping for thumbstick NEGATIVE selected"));
		float dt = GetWorld()->GetDeltaSeconds();
		float camera_movement = thumbstick_speed_scale * FGenericPlatformMath::Abs(y) * y * dt;
		total_guessed_offset += camera_movement;

		FVector camera_location = camera_attachment_point->GetComponentLocation();
		camera_attachment_point->SetWorldLocation(FVector(camera_location.X, camera_location.Y, camera_location.Z + camera_movement));
		scale_model_adjustment(camera_movement);
	}
}



// Caused by pressing B; might remove
void AVRPawn::toggle_seating()
{
	UE_LOG(LogTemp, Log, TEXT("MAPS BEFORE SWAP: "));
	for (int i = 0; i < map_list.Num(); i++)
	{
		UE_LOG(LogTemp, Log, TEXT("map %s\n"), *map_list[i].name.ToString());
	}

	seated = !seated;
	
	// somehow ternary breaks here
	if (map_list[0].name == map_list_a[0].name)
	{
		map_list = map_list_b;
	}
	else
	{
		map_list = map_list_a;
	}


	maps = map_list;

	UE_LOG(LogTemp, Log, TEXT("MAPS AFTER SWAP: "));
	for (int i = 0; i < map_list.Num(); i++)
	{
		UE_LOG(LogTemp, Log, TEXT("map %s\n"), *map_list[i].name.ToString());
	}

	UE_LOG(LogTemp, Log, TEXT("================\nSEATING SWAPPED\n========"));

	if(seated)
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
	office.floor_height = 100.0f;
	office.spawn_points.Add(FVector(100.0f, -1199.595703f, 100.0f));

	MapData realistic_room;
	realistic_room.name = "Room";
	realistic_room.rotation = FRotator(0.0f, 0.0f, 180.0f);
	realistic_room.floor_height = 0.0f;
	realistic_room.spawn_points.Add(FVector(-106.195107f, 3177.424316f, 0.0f));

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
	sun_temple.spawn_points.Add(FVector(-200.0f, 23084.0f, 29.5f));

	MapData sun_temple_day;
	sun_temple_day.name = "SunTempleDay";
	sun_temple_day.rotation = FRotator(0.0f, 0.0f, 0.0f);
	sun_temple_day.floor_height = 29.5f;
	sun_temple_day.spawn_points.Add(FVector(-630.0f, 22400.0f, 29.5f));

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
	zen_walkway_wood.name = "Zen_Vis";
	zen_walkway_wood.rotation = FRotator(0.0f, 0.0f, 0.0f);
	zen_walkway_wood.floor_height = 12.75f;
	zen_walkway_wood.spawn_points.Add(FVector(402.5f, 315.0f, 12.75f));

	MapData zen_walkway_stone;
	zen_walkway_stone.name = "Zen_Vis2";
	zen_walkway_stone.rotation = FRotator(0.0f, 0.0f, 0.0f);
	zen_walkway_stone.floor_height = 0.0f;
	zen_walkway_stone.spawn_points.Add(FVector(40.0f, -1170.0f, 0.0f));

	MapData elven_ruins;
	elven_ruins.name = "ElvenRuins";
	elven_ruins.rotation = FRotator(0.0f, 0.0f, 0.0f);
	elven_ruins.floor_height = 0.0f;
	elven_ruins.spawn_points.Add(FVector(-1480.0f, 270.0f, 8134.5f));

	map_list_a.Add(office); // daytime
	map_list_a.Add(sun_temple_day); // daytime
	map_list_a.Add(lightroom_night); // nighttime
	map_list_a.Add(zen_walkway_wood); // daytime
	map_list_a.Add(zen_walkway_stone);
	map_list_a.Add(berlin_flat); // weird one

	map_list_b.Add(realistic_room); // daytime
	map_list_b.Add(scifi_hallway); // nighttime
	map_list_b.Add(sun_temple); // nighttime
	map_list_b.Add(lightroom_day); // daytime
	map_list_b.Add(elven_ruins);

}

float AVRPawn::distance_moved(float x, float y)
{
	FVector2D input_xy = FVector2D(x, y);
	FVector2D camera_xy = FVector2D(last_camera_position.GetLocation().X, last_camera_position.GetLocation().Y);
	float distance = FVector2D::Distance(input_xy, camera_xy);
	return distance;
}

float AVRPawn::distance_rotated(FRotator current_rotation)
{
	float distance = FGenericPlatformMath::Abs(current_rotation.Yaw - last_camera_position.GetRotation().Rotator().Yaw);
	return distance;
}

TTuple<FVector, FRotator> AVRPawn::body_offset()
{
	FTransform camera_transform = camera->GetComponentTransform();
	float camera_transform_pitch	= camera_transform.Rotator().Yaw;
	camera_transform_pitch -= 90.0f;
	FRotator rotatorwtf			= FRotator(0.0f, camera_transform_pitch, 0.0f); // what the fuck is this?

	FVector rotatorwtf_rightvec = UKismetMathLibrary::GetRightVector(rotatorwtf);
	rotatorwtf_rightvec			= UKismetMathLibrary::Multiply_VectorInt(rotatorwtf_rightvec, -10); // where tf does -20 come from?

	FVector worldtrans_plus_rightvecscaled	= camera_transform.GetLocation() + rotatorwtf_rightvec;
	worldtrans_plus_rightvecscaled.Z	   -= original_avatar_eyeball_height;
	return TTuple<FVector, FRotator>(worldtrans_plus_rightvecscaled, rotatorwtf);
}


float AVRPawn::get_movement_direction()
{
	FRotator camera_pos_rotation = camera->GetRelativeRotation();
	FVector current_camera_forward = UKismetMathLibrary::GetForwardVector(camera_pos_rotation);
	current_camera_forward.Z = 0.0f;
	current_camera_forward.Normalize(0.0001);

	FVector current_camera_pos = camera->GetRelativeLocation();
	current_camera_pos.Z = 0.0f;
	current_camera_pos.Normalize(0.0001);

	FRotator last_camera_pos_rotation = last_camera_position.GetRotation().Rotator();
	FVector last_camera_forward = UKismetMathLibrary::GetForwardVector(last_camera_pos_rotation);
	last_camera_forward.Z = 0.0f;
	last_camera_forward.Normalize(0.0001);

	FVector last_camera_p = last_camera_position.GetLocation();
	last_camera_p.Z = 0.0f;
	last_camera_p.Normalize(0.0001);

	FVector forwards_added = current_camera_forward + last_camera_forward;
	forwards_added = UKismetMathLibrary::Multiply_VectorFloat(forwards_added, 0.5f);

	FVector positions_subtracted = last_camera_p - current_camera_pos;

	float forwards_dot_positions = FVector::DotProduct(forwards_added, positions_subtracted);
	forwards_dot_positions = UKismetMathLibrary::DegAcos(forwards_dot_positions);

	FVector forwards_cross_positions = FVector::CrossProduct(forwards_added, positions_subtracted);

	float forwards_cross_positions_result;
	if (forwards_cross_positions.Z > 0)
	{
		forwards_cross_positions_result = 1.0f;
	}

	else
	{
		forwards_cross_positions_result = -1.0f;
	}

	float dir = forwards_dot_positions * forwards_cross_positions_result;
	return dir;
}


void AVRPawn::calculate_movement()
{
	FTransform current_camera_position; // Yes, call it position while it'a a transform, fucking brilliant.
	float dist_moved;
	float dist_rotated;

	FVector camera_pos = camera->GetComponentLocation();
	float camera_yaw = camera->GetRelativeRotation().Yaw;
	current_camera_position.SetLocation(FVector(camera_pos.X, camera_pos.Y, 0.0f));
	current_camera_position.SetRotation(FQuat(FRotator(0.0f, 0.0f, camera_yaw)));

	dist_moved = distance_moved(current_camera_position.GetLocation().X, current_camera_position.GetLocation().Y);
	dist_rotated = distance_rotated(current_camera_position.GetRotation().Rotator());

	// branch condition

	if (dist_moved > movement_thresh || dist_rotated > rotation_thresh)
	{
		// body offset
		TTuple<FVector, FRotator> b_offset = body_offset();
		body_target_position.SetLocation(b_offset.Get<0>());
		body_target_position.SetRotation(FQuat(b_offset.Get<1>()));

		// movement direciton
		movement_direction = get_movement_direction();
		last_camera_position = current_camera_position;

		// movement speed
		movement_speed = (dist_moved + dist_rotated) / 2000.0f / GetWorld()->GetDeltaSeconds();

		//UE_LOG(LogTemp, Log, TEXT("(moved, rot): %f %f"), dist_moved, dist_rotated);
	}

	else
	{
		if (body_current_position.Equals(body_target_position))
		{
			movement_speed = 0.0f;
			movement_direction = 0.0f;
			alpha = 0.0f;
		}

		else
		{
			alpha = UKismetMathLibrary::FInterpTo_Constant(alpha, 1.0f, GetWorld()->GetDeltaSeconds(), movement_speed);
			body_current_position = UKismetMathLibrary::TLerp(body_current_position, body_target_position, alpha); // Test line below this

			FVector skeletal_mesh_move_loc = FVector(body_current_position.GetLocation().X, body_current_position.GetLocation().Y, skeletal_attachment_point->GetComponentLocation().Z);
			skeletal_mesh->SetWorldLocationAndRotation(skeletal_mesh_move_loc, body_current_position.GetRotation());
		}
	}


}


void AVRPawn::reset_ik_parameters()
{
	last_camera_position.SetLocation(camera->GetRelativeLocation());
	body_target_position.SetLocation(camera->GetRelativeLocation());
	body_current_position.SetLocation(camera->GetRelativeLocation());

	last_camera_position.SetRotation(FQuat(camera->GetRelativeRotation()));
	body_target_position.SetRotation(FQuat(camera->GetRelativeRotation()));
	body_current_position.SetRotation(FQuat(camera->GetRelativeRotation()));

	movement_direction = 0.0f;
	movement_speed = 0.0f;
	alpha = 0.0f;
}