#include "AnimationSystem.h"
#include "opengl/AnimDescriptor.h"

INSTANCE_IMPL(AnimationSystem);

AnimationSystem::AnimationSystem() : ComponentSystemImpl<AnimationComponent>("Animation") {
    AnimationComponent tc;
    componentSerializer.add(new StringProperty(OFFSET(name, tc)));
    componentSerializer.add(new Property<float>(OFFSET(playbackSpeed, tc), 0.001));
    componentSerializer.add(new Property<int>(OFFSET(loopCount, tc)));
}

AnimationSystem::~AnimationSystem() {
    for(AnimIt it=animations.begin(); it!=animations.end(); ++it) {
        delete it->second;
    }
}

void AnimationSystem::DoUpdate(float dt) {
    FOR_EACH_ENTITY_COMPONENT(Animation, a, bc)
        AnimIt jt = animations.find(bc->name);
        if (jt == animations.end())
            continue;
        AnimDescriptor* anim = jt->second;

        if (bc->previousName != bc->name) {
            bc->textureIndex = 0;
            RENDERING(a)->texture = anim->textures[bc->textureIndex];
            bc->accum = 0;
            bc->previousName = bc->name;
            bc->loopCount = anim->loopCount.random();
        } else {
            if (bc->waitAccum > 0) {
                bc->waitAccum -= dt;
                if (bc->waitAccum <= 0) {
                    bc->waitAccum = 0;
                    RENDERING(a)->hide = false;
                }
            } else {
                bc->accum += dt * anim->playbackSpeed * bc->playbackSpeed;
            }

            while(bc->accum >= 1) {
                bool lastImage = (bc->textureIndex == (int)anim->textures.size() - 1);
                if (lastImage) {
                    if (bc->loopCount != 0) {
                        bc->textureIndex = 0;
                        bc->loopCount--;
                    } else if (!anim->nextAnim.empty()) {
                        if ((bc->waitAccum = anim->nextAnimWait.random()) > 0)
                            RENDERING(a)->hide = true;
                        bc->name = anim->nextAnim;
                        break;
                    }
                } else {
                    bc->textureIndex++;
                }
                RENDERING(a)->texture = anim->textures[bc->textureIndex];
                bc->accum -= 1;
            }
        }
    }
}

void AnimationSystem::registerAnim(const std::string& name, std::vector<TextureRef> textures, float playbackSpeed, Interval<int> loopCount, const std::string& nextAnim, Interval<float> nextAnimWait) {
    if (animations.find(name) == animations.end()) {
        AnimDescriptor* a = new AnimDescriptor();
        a->textures = textures;
        a->playbackSpeed = playbackSpeed;
        a->loopCount = loopCount;
        a->nextAnim = nextAnim;
        a->nextAnimWait = nextAnimWait;
        animations[name] = a;
    } else {
        LOG(ERROR) << "Animation '" << name << "' already defined";
    }
}

void AnimationSystem::registerAnim(const std::string& name, const std::string* textureNames, int count, float playbackSpeed, Interval<int> loopCount, const std::string& next, Interval<float> nextAnimWait) {
    LOG_IF(FATAL, animations.find(name) != animations.end()) << " Animation called '" << name << "' already exists (registered anim count: " << animations.size() << ")";
    std::vector<TextureRef> textures;
    for (int i=0; i<count; i++) {
        textures.push_back(theRenderingSystem.loadTextureFile(textureNames[i]));
    }
    registerAnim(name, textures, playbackSpeed, loopCount, next, nextAnimWait);
}

#ifdef INGAME_EDITORS
void AnimationSystem::addEntityPropertiesToBar(Entity entity, TwBar* bar) {
    AnimationComponent* tc = Get(entity, false);
    if (!tc) return;
    TwAddVarRW(bar, "name", TW_TYPE_STDSTRING, &tc->name, "group=Animation");
    TwAddVarRO(bar, "previousName", TW_TYPE_STDSTRING, &tc->previousName, "group=Animation");
    TwAddVarRO(bar, "accum", TW_TYPE_FLOAT, &tc->accum, "group=Animation");
    TwAddVarRW(bar, "playbackSpeed", TW_TYPE_FLOAT, &tc->playbackSpeed, "group=Animation");
    TwAddVarRW(bar, "loopCount", TW_TYPE_INT32, &tc->loopCount, "group=Animation");
    TwAddVarRO(bar, "textureIndex", TW_TYPE_INT32, &tc->textureIndex, "group=Animation");
    TwAddVarRO(bar, "waitAccum", TW_TYPE_FLOAT, &tc->waitAccum, "group=Animation");
}
#endif