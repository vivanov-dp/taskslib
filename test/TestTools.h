#pragma once

#include <random>
#include <chrono>

#include "Types.h"
#include "TaskOptions.h"

using namespace TasksLib;

TaskOptions GenerateRandomOptions(std::default_random_engine& randEng) {
	std::uniform_int_distribution<unsigned int> distInt(1, INT_MAX);
	std::uniform_int_distribution<unsigned int> distBool(0, 1);
	std::uniform_int_distribution<unsigned int> distThreadTarget(0, 1);

	TaskPriority priority{ distInt(randEng) };
	bool isBlocking{ (bool)distBool(randEng) };
	TaskThreadTarget target{ static_cast<TaskThreadTarget>(distThreadTarget(randEng)) };
	std::chrono::milliseconds ms{ distInt(randEng) };

	return TaskOptions{ priority, isBlocking, target, ms };
}
