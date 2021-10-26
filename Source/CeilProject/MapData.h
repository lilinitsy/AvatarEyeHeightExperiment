// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */


struct FloatPair
{
	float first;
	float second;

	FloatPair()
	{

	}

	FloatPair(float f, float s)
	{
		first = f;
		second = s;
	}

	FString to_string()
	{
		FString pairstring = "(" + FString::SanitizeFloat(first) + ", " + FString::SanitizeFloat(second) + ")";
		return pairstring;
	}
};


struct CEILPROJECT_API MapData
{
	FName name;
	FRotator rotation;				
	float floor_height;				
	TArray<FVector> spawn_points;
	TArray<FloatPair> intervals;

	MapData();
};
