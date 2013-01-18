/*
    This file is part of sac.

    @author Soupe au Caillou - Pierre-Eric Pelloux-Prayer
    @author Soupe au Caillou - Gautier Pelloux-Prayer

    Heriswap is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    Heriswap is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with sac.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "System.h"

struct CameraDepComponent {
    Vector2 screenScalePosition;
    Vector2 screenScaleSize;
    int cameraIndex;
};

#define theCameraDepSystem CameraDepSystem::GetInstance()
#define CAMERA_DEP(e) theCameraDepSystem.Get(e)

UPDATABLE_SYSTEM(CameraDep)

};
