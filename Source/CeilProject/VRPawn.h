// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MapData.h"


#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CoreMinimal.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Pawn.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "MotionControllerComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h" 
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


		UPROPERTY(EditAnywhere, Category = "Instruction Audio")
		USoundWave *instruction_audio;

		UPROPERTY(EditAnywhere, Category = "Instruction Audio")
		USoundWave *stand_calib_1;

		UPROPERTY(EditAnywhere, Category = "Instruction Audio")
		USoundWave *sit_calib_1;

		UPROPERTY(EditAnywhere, Category = "Instruction Audio")
		USoundWave *commence_standing_trials_2;

		UPROPERTY(EditAnywhere, Category = "Instruction Audio")
		USoundWave *commence_sitting_trials_2;

		UPROPERTY(EditAnywhere, Category = "Instruction Audio")
		USoundWave *calibration_completed;

		UPROPERTY(EditAnywhere, Category = "Instruction Audio")
		USoundWave *look_straight_ahead;

		

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



		// Stuff related to IK here
		FTransform last_camera_position;
		FTransform body_target_position;
		FTransform body_current_position;
		float movement_thresh = 10.0f;
		float rotation_thresh = 10.0f;
		float movement_direction;
		float movement_speed;
		float alpha;

		float distance_moved(float x, float y);
		float distance_rotated(FRotator current_rotation);
		TTuple<FVector, FRotator> body_offset();
		float get_movement_direction();
		void calculate_movement();
		void reset_ik_parameters();


	private:
		// Greater map_list to save which maps are used, and then maps will be the random maps that are used for each set of n trials.
		TArray<MapData> map_list;
		TArray<MapData> maps;
		MapData current_map;
		MapData previous_map;

		// Setup variables
		int tick_counter = 0;
		float sum_height = 0.0f;
		float current_offset = 0.0f;

		bool calibrating_standing = false; // This will be swapped on first press which will make calibration the first thing that's started.

		// Saved after calibration to be used for properly scaling the model
		float original_avatar_eyeball_height;
		float original_camera_height;

		// Given default valuse that will be recalculated during calibraiton period
		float min_standing_height = 10000.0f;
		float max_standing_height = 0.0f;
		float min_sitting_height = 10000.0f;
		float max_sitting_height = 0.0f;

		// Time spent per map
		float map_time = 0.0f;

		// The total offset moved on the controllers. Reset each trial.
		float total_guessed_offset = 0.0f;
		int trial_num = 0;

		// Variables to get around oculus height tracking being broken.
		// Use floor_height* to record floor height
		bool floor_height_recorded = false;
		float floor_height = 0.0f;

		// Use begin_calibration to signify whether calibration should begin

		bool calibration_started = false;
		bool standing_calibrated = false;
		bool sitting_calibrated = false;

		bool instruction_audio_started = false;
		bool instruction_audio_finished = false;

		bool stand_calib_1_started = false;
		bool stand_calib_1_finished = false;

		bool sit_calib_1_started = false;
		bool sit_calib_1_finished = false;

		bool commence_standing_trials_2_started = false;
		bool commence_sitting_trials_2_started = false;

		// true = standing happening, false = sitting happening, randomize this at start for random groups and log it
		bool standing_trials_currently = true;
		
		float instruction_audio_time = 0.0f;
		float stand_calib_1_time = 0.0f;
		float sit_calib_1_time = 0.0f;
		float commence_standing_trials_2_time = 0.0f;


		// Instruction map hacks
		bool instruction_map_1 = true;
		bool instruction_map_2 = false;
		bool instruction_map_3 = false;
		MapData instructionlvl1;
		MapData instructionlvl2;
		MapData instructionlvl3;



		// Figure out headset motion vectors
		void write_headset_motion_data_to_file(FString rot_data, FString pos_data);
		void compute_headset_motion_information();
		FRotator min_y_camera_look = FRotator(0.0f, 0.0f, 0.0f);
		FRotator max_y_camera_look = FRotator(0.0f, 0.0f, 0.0f);
		FVector min_y_camera_look_pos = FVector(0.0f, 0.0f, 0.0f);
		FVector max_y_camera_look_pos = FVector(0.0f, 0.0f, 0.0f);

		void initialize_map_data();
		void swap_calibration();

		// Sound hack functions
		void check_audio_finished(float end_time, float &audio_time, bool &sound_finished);
};
