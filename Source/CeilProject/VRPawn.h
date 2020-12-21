// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MapData.h"


#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CoreMinimal.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Pawn.h"
#include "MotionControllerComponent.h"
#include "VRPawn.generated.h"


UCLASS()
class CEILPROJECT_API AVRPawn : public APawn
{
	GENERATED_BODY()

	protected:
		// Called when the game starts or when spawned
		virtual void BeginPlay() override;

	public:

		UPROPERTY(EditAnywhere, Category = "Components")
		UCameraComponent *camera;

		// Origin for HMD
		UPROPERTY(EditAnywhere, Category = "Components")
		USceneComponent *vr_origin;

		UPROPERTY(EditAnywhere, Category = "Components")
		USceneComponent *camera_attachment_point;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
		USceneComponent *skeletal_attachment_point;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
		USkeletalMeshComponent *skeletal_mesh;

		UPROPERTY(VisibleAnywhere, Category = "Motion Controllers")
		UMotionControllerComponent *left_hand;

		UPROPERTY(VisibleAnywhere, Category = "Motion Controllers")
		UMotionControllerComponent *right_hand;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
		UAnimationAsset *standing_animation;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
		UAnimationAsset *sitting_animation;

		UPROPERTY(EditAnywhere, Category = "General Parameters")
		bool seated = false;

		UPROPERTY(EditAnywhere, Category = "General Parameters")
		bool male_model = true;

		UPROPERTY(VisibleAnywhere, Category = "Information")
		float original_avatar_standing_eyeball_height;

		UPROPERTY(VisibleAnywhere, Category = "Information")
		float original_avatar_sitting_eyeball_height;

		UPROPERTY(VisibleAnywhere, Category = "Information")
		float original_standing_camera_height;

		UPROPERTY(VisibleAnywhere, Category = "Information")
		float original_sitting_camera_height;

		// User foot size (cm)
		UPROPERTY(EditAnywhere, Category = "General Parameters")
		float foot_size;

		UPROPERTY(VisibleAnywhere, Category = "Information")
		float original_foot_size;

		// original_camera_location.Z represents original camera_eye_height
		UPROPERTY(VisibleAnywhere, Category = "Information")
		FVector original_camera_location;

		UPROPERTY(VisibleAnywhere, Category = "Motion Parameters")
		float thumbstick_y;

		UPROPERTY(EditAnywhere, Category = "Motion Parameters")
		float thumbstick_speed_scale;

		AVRPawn();
		virtual void Tick(float DeltaTime) override;
		virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
		void reset_hmd_origin();
		void toggle_seating();
		void set_thumbstick_y(float y);
		void cycle_offset();
		void write_data_to_file(FString data);
		void scale_model_offset(float offset);
		void scale_model_adjustment(float amount);

	private:
		TArray<MapData> map_list;
		TArray<MapData> maps;
		MapData current_map;
		MapData previous_map;

		int tick_counter = 0;
		float sum_height = 0.0f;
		float current_offset = 0.0f;

		bool calibrating_standing = true;

		float original_avatar_eyeball_height;
		float original_camera_height;
		float min_standing_height = 10000.0f;
		float max_standing_height = 0.0f;
		float min_sitting_height = 10000.0f;
		float max_sitting_height = 0.0f;

		int guess_counter = 0;
		float map_time = 0.0f;
		float total_guessed_offset = 0.0f;

		int trial_num = 0;

		void initialize_map_data();
		void swap_calibration();
};
