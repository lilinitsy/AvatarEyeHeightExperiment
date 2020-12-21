// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
struct CEILPROJECT_API MapData
{
	FName name;
	FRotator rotation;				
	float floor_height;				
	TArray<FVector> spawn_points;
};
