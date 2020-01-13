	// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

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

	// Distance up/down between camera and z to make the camera align with the head
	UPROPERTY(EditAnywhere, Category = "Information")
	float z_offset = -174.0f;

	// Possibly will be irrelevant
	UPROPERTY(EditAnywhere, Category = "General Parameters")
	float foot_size;

	UPROPERTY(EditAnywhere, Category = "General Parameters")
	float height;

	// original_camera_location.Z represents original camera_eye_height
	UPROPERTY(VisibleAnywhere, Category = "Information")
	FVector original_camera_location;
	
	UPROPERTY(VisibleAnywhere, Category = "General Parameters")
	TArray<float> offsets;

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

	

private:
	TArray<float> fill_offset_TArray(FString path);
	

};
