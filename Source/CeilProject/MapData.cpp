// Fill out your copyright notice in the Description page of Project Settings.

#include "MapData.h"


MapData::MapData()
{
	intervals.Add(TTuple<float, float>(-80.0f, -40.0f));
	intervals.Add(TTuple<float, float>(-40.0f, -20.0f));
	intervals.Add(TTuple<float, float>(-20.0f, 20.0f));
	intervals.Add(TTuple<float, float>(20.0f, 40.0f));
	intervals.Add(TTuple<float, float>(40.0f, 80.0f));
}