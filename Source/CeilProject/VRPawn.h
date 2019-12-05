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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
	USceneComponent *skeletal_attachment_point;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
	USkeletalMeshComponent *standing_mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes")
	USkeletalMeshComponent *sitting_mesh;

	// CONTROLLERS
	UPROPERTY(VisibleAnywhere, Category = "Motion Controllers")
	UMotionControllerComponent *left_hand;

	UPROPERTY(VisibleAnywhere, Category = "Motion Controllers")
	UMotionControllerComponent *right_hand;

	
	UPROPERTY(EditAnywhere, Category = "General Parameters")
	bool seated = false; // maybe don't edit directly, call toggle_seating() for clarity?

	UPROPERTY(EditAnywhere, Category = "General Parameters")
	bool male_model = true;

	UPROPERTY(EditAnywhere, Category = "Information")
	float z_offset = -174.0f;

	UPROPERTY(EditAnywhere, Category = "General Parameters")
	float foot_size;

	UPROPERTY(EditAnywhere, Category = "General Parameters")
	float height;
	
	UPROPERTY(VisibleAnywhere, Category = "General Parameters")
	TArray<float> offsets;

	UPROPERTY(VisibleAnywhere, Category = "General Parameters")
	float thumbstick_y;


	AVRPawn();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void reset_hmd_origin();
	void toggle_seating();
	void cycle_offset();
	void set_thumbstick_y();



	

private:
	TArray<float> fill_offset_TArray(FString path);
	
};
