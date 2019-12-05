// Fill out your copyright notice in the Description page of Project Settings.
//
//
//
//
//
/* USE VRPawn INSTEAD OF VRCHARACTER*/
//
//
//
//
//


#pragma once

#include "Camera/CameraComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MotionControllerComponent.h"
#include "VRCharacter.generated.h"

// TODO: Make this a pawn
UCLASS()
class CEILPROJECT_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

	public:
		

	protected:
		// Called when the game starts or when spawned
		virtual void BeginPlay() override;

	public:

		// Sets default values for this character's properties
		AVRCharacter();

		UPROPERTY(VisibleAnywhere, Category = "Components")
		UCameraComponent *camera;

		// Origin for HMD
		UPROPERTY(VisibleAnywhere, Category = "Components")
		USceneComponent *vr_origin;

		// CONTROLLERS
		/*
		DON'T NEED MOTION CONTROLLERS FOR EYE HEIGHT EXPERIMENT, WILL MAYBE NEED THEM FOR ANOTHER
		UPROPERTY(VisibleAnywhere, Category = "Components")
		UMotionControllerComponent *left_hand;

		UPROPERTY(VisibleAnywhere, Category = "Components")
		UMotionControllerComponent *right_hand;
		*/

		UPROPERTY(VisibleAnywhere, Category = "General Parameters")
		bool seated = false;

		
		// Called every frame
		virtual void Tick(float DeltaTime) override;

		// Called to bind functionality to input
		virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

		void reset_hmd_origin();
		void toggle_seating();

	
	
};
