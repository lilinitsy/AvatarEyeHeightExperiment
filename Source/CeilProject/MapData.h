// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
struct CEILPROJECT_API MapData
{
	FName name;
	FRotator rotation;				// do we want this to be TArray?
	float floor_height;				// do we want this to be TArray?
	TArray<FVector> spawn_points;
};
