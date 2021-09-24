#pragma once

#include <libgpsmm.h>

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <atomic>
#include "glm/vec2.hpp"

//bool getLocation(float *lat, float *lon, float *spd, int *hrs, int *min, int *sec);

class GPSService {
public:
	GPSService();
	virtual ~GPSService();
	
	std::thread start();
	
	void stop();
	
	float getLatitude();
	
	float getLongitude();
	
	float getSpeed();
	
	int getHour();
	
	int getMinute();
	
protected:

	void loop();
	
protected:
	gpsmm gps;
	std::atomic<float> lat;
	std::atomic<float> lon;
	std::atomic<float> speed;
	std::atomic<int> hour;
	std::atomic<int> minute;
	std::atomic<bool> update;
	
};

//  Example C++17 gpsd program using libgpsmm

//  Has no actual purpose other than output some of libgpsmm struct variables of your choosing.
//  reference declarations: https://fossies.org/dox/gpsd-3.22/index.html
//
//  compiles without warning as follows:
//  g++ -std=c++17 -Wall -pedantic -pthread $(pkg-config --cflags --libs libgps) gpsd-example.cpp -o gpsd-example
//  clang++ -std=c++17 -Wall -pedantic -pthread $(pkg-config --cflags --libs libgps) gpsd-example.cpp -o gpsd-example
//  Free for the taking.
//  Version 1.84

