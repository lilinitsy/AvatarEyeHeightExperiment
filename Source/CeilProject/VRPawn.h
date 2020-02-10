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

	// Look into USkeletalMesh sockets for bone transforms -> problem doing it programmatically is have to abandon this animation methodology
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
		USkeletalMeshComponent *skeletal_mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
		USkeletalMeshComponent *sitting_mesh;

	// CONTROLLERS
	UPROPERTY(VisibleAnywhere, Category = "Motion Controllers")
		UMotionControllerComponent *left_hand;

	UPROPERTY(VisibleAnywhere, Category = "Motion Controllers")
		UMotionControllerComponent *right_hand;


	/////////////////
	/// There is a chance that I won't want to have two SkeletalMeshComponents, but just one and two animsequences
	/////////////////

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
		UAnimationAsset *standing_animation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
		UAnimationAsset *sitting_animation;

	UPROPERTY(EditAnywhere, Category = "General Parameters")
		bool seated = false; // maybe don't edit directly, call toggle_seating() for clarity?

	UPROPERTY(EditAnywhere, Category = "General Parameters")
		bool male_model = true;

	UPROPERTY(VisibleAnywhere, Category = "Information")
		float original_avatar_eyeball_height = 172.666870f;

	UPROPERTY(EditAnywhere, Category = "Information")
		float original_camera_height;

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
	void record_guess();
	void write_data_to_file(FString data);
	void scale_model_offset(float offset);
	void scale_model_adjustment(float amount);

private:
	TArray<MapData> maps;
	MapData current_map;
	MapData previous_map;
	int tick_counter = 0;
	float sum_height = 0.0f;

	void initialize_map_data();
};