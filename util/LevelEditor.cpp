/*
 This file is part of libsac.

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
 along with Heriswap.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "LevelEditor.h"
#include "IntersectionUtil.h"
#include "../base/EntityManager.h"
#include "../base/TouchInputManager.h"
#include "../base/PlacementHelper.h"    
#include "../systems/TransformationSystem.h"
#include "../systems/RenderingSystem.h"
#include <GL/glfw.h>

struct LevelEditor::LevelEditorDatas {
    Entity over;
    Entity selected;
    
    Vector2 lastMouseOverPosition;
    Vector2 selectedOriginalPos;

    unsigned activeCameraIndex;
};

static void select(Entity e) {
    RENDERING(e)->effectRef = theRenderingSystem.loadEffectFile("selected.fs");
}
static void deselect(Entity e) {
    RENDERING(e)->effectRef = DefaultEffectRef;
}

LevelEditor::LevelEditor() {
    datas = new LevelEditorDatas();
    datas->activeCameraIndex = 0;
}

void LevelEditor::tick(float dt) {
    Vector2 position;
    int x, y;
    glfwGetMousePos(&x, &y);
    Vector2 windowPos(x / (float)PlacementHelper::WindowWidth - 0.5, 0.5 - y / (float)PlacementHelper::WindowHeight);
    for (unsigned i=0; i<theRenderingSystem.cameras.size(); i++) {
        const RenderingSystem::Camera& cam = theRenderingSystem.cameras[i];
        if (IntersectionUtil::pointRectangle(windowPos, cam.screenPosition, cam.screenSize) && cam.enable) {
            position = cam.worldPosition + windowPos * cam.worldSize;
            break;
        }
    }

    if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_1) == GLFW_RELEASE) { 
        if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_2) == GLFW_RELEASE) {
            if (datas->selected)
                datas->selectedOriginalPos = TRANSFORM(datas->selected)->position;
            if (datas->over)
                RENDERING(datas->over)->effectRef = DefaultEffectRef;
            datas->lastMouseOverPosition = position;

            std::vector<Entity> entities = theRenderingSystem.RetrieveAllEntityWithComponent();
                float nearest = 10000;
                for (unsigned i=0; i<entities.size(); i++) {
                    if (entities[i] == datas->selected)
                        continue;
                    if (RENDERING(entities[i])->hide)
                        continue;
                    if (IntersectionUtil::pointRectangle(position, TRANSFORM(entities[i])->worldPosition, TRANSFORM(entities[i])->size)) {
                        float d = Vector2::DistanceSquared(position, TRANSFORM(entities[i])->worldPosition);
                        if (d < nearest) {
                            datas->over = entities[i];
                            nearest = d;
                        }
                    }
                }
                if (datas->over)
                    RENDERING(datas->over)->effectRef = theRenderingSystem.loadEffectFile("over.fs");
        } else {
            if (datas->over) {
                if (datas->selected)
                    deselect(datas->selected);
                datas->selected = datas->over;
                select(datas->selected);
                datas->over = 0;
            }
        }
    } else {
        if (datas->selected) {
            TRANSFORM(datas->selected)->position = datas->selectedOriginalPos + position - datas->lastMouseOverPosition;
        }
    }

    if (datas->selected) {
        static int prevWheel = 0;
        int wheel = glfwGetMouseWheel();
        int diff = wheel - prevWheel;
        if (diff) {
            bool shift = glfwGetKey( GLFW_KEY_LSHIFT );
            bool ctrl = glfwGetKey( GLFW_KEY_LCTRL );
            
            if (!shift && !ctrl) {
                TRANSFORM(datas->selected)->rotation += 2 * diff * dt;
            } else {
                if (shift) {
                    TRANSFORM(datas->selected)->size.X *= (1 + 1 * diff * dt); 
                }
                if (ctrl) {
                    TRANSFORM(datas->selected)->size.Y *= (1 + 1 * diff * dt); 
                }
            }
            prevWheel = wheel;
        }
    }

    // camera movement
    {
        RenderingSystem::Camera& camera = theRenderingSystem.cameras[datas->activeCameraIndex];
        float moveSpeed = glfwGetKey(GLFW_KEY_LSHIFT) ? 8 : 2.5;
        if (glfwGetKey(GLFW_KEY_LEFT)) {
            camera.worldPosition.X -= moveSpeed * dt;
        } else if (glfwGetKey(GLFW_KEY_RIGHT)) {
            camera.worldPosition.X += moveSpeed * dt;
        }
        if (glfwGetKey(GLFW_KEY_DOWN)) {
            camera.worldPosition.Y -= moveSpeed * dt;
        } else if (glfwGetKey(GLFW_KEY_UP)) {
            camera.worldPosition.Y += moveSpeed * dt;
        }
    }
    // camera switching
    {
        for (unsigned i=0; i<theRenderingSystem.cameras.size(); i++) {
            if (glfwGetKey(GLFW_KEY_KP_1 + i) && i != datas->activeCameraIndex) {
                std::cout << "new active cam: " << i << std::endl;
                theRenderingSystem.cameras[datas->activeCameraIndex].enable = false;
                datas->activeCameraIndex = i;
                theRenderingSystem.cameras[datas->activeCameraIndex].enable = true;
                break;
            }
        }
    }
}
