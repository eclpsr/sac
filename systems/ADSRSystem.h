#pragma once

#include "base/MathUtil.h"

#include "System.h"

enum Mode {
	Linear, 
	Quadratic
};


struct ADSRComponent {
	//ADSRComponent() : moding(Linear) {}
	bool active;

	float value;
	float activationTime;

	float idleValue;
	float attackValue;
	float attackTiming;
	float sustainValue;
	float decayTiming;
	float releaseTiming;
	Mode moding;
};
	
#define theADSRSystem ADSRSystem::GetInstance()
#define ADSR(entity) theADSRSystem.Get(entity)

UPDATABLE_SYSTEM(ADSR)
			
};

