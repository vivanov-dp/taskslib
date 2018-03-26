#pragma once

#include <random>
#include <chrono>
#include <string>
#include <sstream>

#include "Types.h"
#include "TaskOptions.h"

using namespace TasksLib;

class TestWithRandom : public ::testing::Test {
public:
	TestWithRandom::TestWithRandom() {
		randEng.seed(randDev());
	}
	virtual TestWithRandom::~TestWithRandom() {}

	std::random_device randDev;
	std::default_random_engine randEng;
};


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

class ExecutableTester {
public:
	ExecutableTester(std::default_random_engine& _randEng)
		: randEng(_randEng)
		, distInt1(1, INT_MAX - 20001)
		, distInt2(1, 20000)
	{
		ResetTest();
	}

	std::default_random_engine randEng;
	std::uniform_int_distribution<unsigned int> distInt1;
	std::uniform_int_distribution<unsigned int> distInt2;
	unsigned int test;
	unsigned int testBase;
	unsigned int generated;

	void PerformTest() {
		generated = distInt2(randEng);
		test = testBase + generated;
	}
	void ResetTest() {
		test = 0;
		testBase = distInt1(randEng);
		generated = 0;
	}
};

std::string GenerateRandomString(const size_t minLen, const size_t maxLen, std::default_random_engine& randEng) {
	std::uniform_int_distribution<unsigned int> distLength(static_cast<unsigned int>(minLen), static_cast<unsigned int>(maxLen));
	std::uniform_int_distribution<unsigned short> distChar(32, 126);
	std::stringstream buff;

	for (int i = distLength(randEng); i > 0; i--) {
		buff << static_cast<char>(distChar(randEng));
	}

	return buff.str();
}
