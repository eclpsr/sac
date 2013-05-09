#pragma once

#include "System.h"
#include <vector>

struct DebuggingComponent {
};

#define theDebuggingSystem DebuggingSystem::GetInstance()
#define DEBUGGING(e) theDebuggingSystem.Get(e)

UPDATABLE_SYSTEM(Debugging)

    public:
        void toggle();
    private:
        bool enable;
        std::map<std::string, Entity> debugEntities;
        std::vector<Entity> renderStatsEntities;

        Entity fps, entityCount, systems;
        Entity fpsLabel, entityCountLabel;
};
