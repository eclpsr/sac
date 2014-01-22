/*
    This file is part of Soupe Au Caillou.

    @author Soupe au Caillou - Jordane Pelloux-Prayer
    @author Soupe au Caillou - Gautier Pelloux-Prayer
    @author Soupe au Caillou - Pierre-Eric Pelloux-Prayer

    Soupe Au Caillou is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    Soupe Au Caillou is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Soupe Au Caillou.  If not, see <http://www.gnu.org/licenses/>.
*/



#include "RenderingSystem.h"
#include "RenderingSystem_Private.h"
#include "CameraSystem.h"
#include "TransformationSystem.h"
#include <sstream>
#if SAC_INGAME_EDITORS
#include <AntTweakBar.h>
#include "util/LevelEditor.h"
#endif
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>



static void computeVerticesScreenPos(const std::vector<glm::vec2>& points, const glm::vec2& position, const glm::vec2& hSize, float rotation, float z, int rotateUV, glm::vec3* out);

bool firstCall;

RenderingSystem::ColorAlphaTextures RenderingSystem::chooseTextures(const InternalTexture& tex, const FramebufferRef& fbo, bool useFbo) {
    if (useFbo) {
        RenderingSystem::Framebuffer b = ref2Framebuffers[fbo];
        return std::make_pair(b.texture, b.texture);
    } else {
        return std::make_pair(tex.color, tex.alpha);
    }
}

#if SAC_DEBUG
namespace BatchFlushReason {
    enum Enum {
        NewCamera,
        NewFlags,
        NewTarget,
        NewTexture,
        NewEffect,
        NewColor,
        NewFBO,
        End,
        Full,
    };
}
struct BatchFlushInfo {
    BatchFlushInfo(const BatchFlushReason::Enum e) : reason(e) {}
    BatchFlushInfo(const BatchFlushReason::Enum e, unsigned f) : reason(e), newFlags(f) {}
    BatchFlushInfo(const BatchFlushReason::Enum e, TextureRef r) : reason(e), newTexture(r) {}
    BatchFlushInfo(const BatchFlushReason::Enum e, const Color& r) : reason(e), newColor(r) {}

    BatchFlushReason::Enum reason;
    union {
        unsigned newFlags;
        TextureRef newTexture;
        Color newColor;
    };
};
static std::vector<std::pair<BatchFlushInfo, int> > batchSizes;
static std::string enumToString(BatchFlushReason::Enum e) {
    switch (e) {
        case BatchFlushReason::NewCamera:
            return "NewCamera";
        case BatchFlushReason::NewFlags:
            return "NewFlags";
        case BatchFlushReason::NewTarget:
            return "NewTarget";
        case BatchFlushReason::NewTexture:
            return "NewTexture";
        case BatchFlushReason::NewEffect:
            return "NewEffect";
        case BatchFlushReason::NewColor:
            return "NewColor";
        case BatchFlushReason::NewFBO:
            return "NewFBO";
        case BatchFlushReason::End:
            return "End";
        case BatchFlushReason::Full:
            return "Full";
    }
    return "";
}

inline std::ostream& operator<<(std::ostream& stream, const BatchFlushInfo & v) {
    stream << enumToString(v.reason);
    switch (v.reason) {
        case BatchFlushReason::NewFlags:
            stream << " [ ";
            if (v.newFlags & EnableZWriteBit)
                stream << "EnableZWriteBit ";
            if (v.newFlags & DisableZWriteBit)
                stream << "DisableZWriteBit ";
            if (v.newFlags & EnableBlendingBit)
                stream << "EnableBlendingBit ";
            if (v.newFlags & DisableBlendingBit)
                stream << "DisableBlendingBit ";
            if (v.newFlags & EnableColorWriteBit)
                stream << "EnableColorWriteBit ";
            if (v.newFlags & DisableColorWriteBit)
                stream << "DisableColorWriteBit ";
            stream << "]";
            break;
        case BatchFlushReason::NewColor:
            stream << " [ " << v.newColor << " ]";
            break;
        case BatchFlushReason::NewTexture: {
            if (v.newTexture == InvalidTextureRef) {
                stream << " [ No Texture ]";
            } else {
                const TextureInfo* info = theRenderingSystem.textureLibrary.get(v.newTexture, false);
                if (!info) {
                    stream << " [ --- ]";
                } else {
                    if (info->atlasIndex >= 0) {
                        stream << " [ " << theRenderingSystem.atlas[info->atlasIndex].name << " ]";
                    } else {
                        stream << " [ " << theRenderingSystem.textureLibrary.ref2Name(v.newTexture) << " ]";
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    return stream;
}


#endif
static int drawBatchES2(
    const RenderingSystem::ColorAlphaTextures glref
    , const glm::vec3* vertices
    , const glm::vec2* uvs
    , const unsigned short* indices
    , int
#if SAC_USE_VBO
     batchVertexCount
#endif
    , int batchTriangleCount) {

    if (batchTriangleCount > 0) {
        GL_OPERATION(glActiveTexture(GL_TEXTURE0))
        // GL_OPERATION(glEnable(GL_TEXTURE_2D)
        GL_OPERATION(glBindTexture(GL_TEXTURE_2D, glref.first))

        // GL_OPERATION(glEnable(GL_TEXTURE_2D))
        if (firstCall) {
            // GL_OPERATION(glBindTexture(GL_TEXTURE_2D, 0))
        } else {
            GL_OPERATION(glActiveTexture(GL_TEXTURE1))
            GL_OPERATION(glBindTexture(GL_TEXTURE_2D, glref.second))
        }

#if SAC_USE_VBO
        // update vertex buffer
        GL_OPERATION(glBindBuffer(GL_ARRAY_BUFFER, theRenderingSystem.glBuffers[1]))
        GL_OPERATION(glBufferSubData(GL_ARRAY_BUFFER, 0,
            batchVertexCount * 3 * sizeof(float), vertices))
        GL_OPERATION(glEnableVertexAttribArray(EffectLibrary::ATTRIB_VERTEX))
        GL_OPERATION(glVertexAttribPointer(EffectLibrary::ATTRIB_VERTEX, 3, GL_FLOAT, 0, 3 * sizeof(float), 0))

        LOGT_EVERY_N(1000, "Use short for uv. Interleave position and uv [xxxx][yyyy][zzzz][uuvv]");
        LOGT_EVERY_N(1000, "Could also use short for position, for instance: using [x * 100] x can be in [-320, 320] range");
        // update uv buffer
        GL_OPERATION(glBindBuffer(GL_ARRAY_BUFFER, theRenderingSystem.glBuffers[2]))
        GL_OPERATION(glBufferSubData(GL_ARRAY_BUFFER, 0,
            batchVertexCount * 2 * sizeof(float), uvs))
        GL_OPERATION(glEnableVertexAttribArray(EffectLibrary::ATTRIB_UV))
        GL_OPERATION(glVertexAttribPointer(EffectLibrary::ATTRIB_UV, 2, GL_FLOAT, 0, 2 * sizeof(float), 0))
#else
        GL_OPERATION(glVertexAttribPointer(EffectLibrary::ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, vertices))
        GL_OPERATION(glEnableVertexAttribArray(EffectLibrary::ATTRIB_VERTEX))
        GL_OPERATION(glVertexAttribPointer(EffectLibrary::ATTRIB_UV, 2, GL_FLOAT, 1, 0, uvs))
        GL_OPERATION(glEnableVertexAttribArray(EffectLibrary::ATTRIB_UV))
#endif

        GL_OPERATION(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, theRenderingSystem.glBuffers[0]))
        GL_OPERATION(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
            batchTriangleCount * 3 * sizeof(unsigned short), indices))

        GL_OPERATION(glDrawElements(GL_TRIANGLES, batchTriangleCount * 3, GL_UNSIGNED_SHORT, 0))
    }

    #if SAC_OLD_HARDWARE
        //seems to solve artifacts on old graphic cards (nvidia 8600GT at least)
        glFinish();
    #endif
    return 0;
}

static inline void computeUV(RenderingSystem::RenderCommand& rc, const TextureInfo& info) {
    // Those 2 are used by RenderingSystem to display part of the texture, with different flags.
    // For instance: display a partial-but-opaque-version before the original alpha-blended one.
    // So, their default value are: offset=0,0 and size=1,1
    glm::vec2 uvModifierOffset(rc.uv[0]);
    glm::vec2 uvModifierSize(rc.uv[1]);
    const glm::vec2 uvSize (info.uv[1] - info.uv[0]);

     // If image is rotated in atlas, we need to adjust UVs
    if (info.rotateUV) {
        // #1: swap x/y start coordinates (ie: top-left point of the image)
        std::swap(uvModifierOffset.x, uvModifierOffset.y);
        // #2: swap x/y end coords (ie: bottom-right corner of the image)
        std::swap(uvModifierSize.x, uvModifierSize.y);
        // #3:
        uvModifierOffset.y = 1 - (uvModifierSize.y + uvModifierOffset.y);
        //uvModifierOffset.x = 1 - (uvModifierSize.x + uvModifierOffset.x);

    }

    // Compute UV to send to GPU
    {
        rc.uv[0] = info.uv[0] + glm::vec2(uvModifierOffset.x * uvSize.x, uvModifierOffset.y * uvSize.y);
        rc.uv[1] = rc.uv[0] + glm::vec2(uvModifierSize.x * uvSize.x, uvModifierSize.y * uvSize.y);
    }
    // Miror UV when doing horizontal miroring
    if (rc.mirrorH) {
        if (info.rotateUV)
            std::swap(rc.uv[0].y, rc.uv[1].y);
        else
            std::swap(rc.uv[0].x, rc.uv[1].x);
    }
    rc.rotateUV = info.rotateUV;
}

static inline void addRenderCommandToBatch(const RenderingSystem::RenderCommand& rc,
    glm::vec3* outVertices,
    glm::vec2* outUvs,
    unsigned short* outIndices,
    unsigned* verticesCount,
    unsigned* triangleCount) {

    // lookup shape
    const Polygon& polygon = theRenderingSystem.shapes[rc.shapeType];

    // vertices
    const std::vector<glm::vec2>& vert =
        (rc.vertices == DefaultVerticesRef) ? polygon.vertices : theRenderingSystem.dynamicVertices[rc.vertices];

    // perform world -> screen position transformation
    computeVerticesScreenPos(vert, rc.position, rc.halfSize, rc.rotation, -rc.z, (rc.shapeType == Shape::Square) & rc.rotateUV, outVertices);

    // copy indices
    for(const auto& i: polygon.indices) {
        *outIndices++ = *verticesCount + i;
    }

    // copy uvs
    *outUvs++ = glm::vec2(rc.uv[0].x, 1 - rc.uv[0].y);
    *outUvs++ = glm::vec2(rc.uv[1].x, 1 - rc.uv[0].y);
    *outUvs++ = glm::vec2(rc.uv[0].x, 1 - rc.uv[1].y);
    *outUvs++ = glm::vec2(rc.uv[1].x, 1 - rc.uv[1].y);

    *verticesCount += polygon.vertices.size();
    *triangleCount += polygon.indices.size() / 3;
}

EffectRef RenderingSystem::changeShaderProgram(EffectRef ref, bool _firstCall, bool useTexturing, const Color& color, const glm::mat4& mvp, bool colorEnabled) {
    const Shader& shader = effectRefToShader(ref, _firstCall, colorEnabled, useTexturing);
    // change active shader
    GL_OPERATION(glUseProgram(shader.program))
    // upload transform matrix (perspective + view)
    GL_OPERATION( glUniformMatrix4fv(shader.uniformMatrix, 1, GL_FALSE, glm::value_ptr(mvp)))
    // upload texture uniforms
    GL_OPERATION(glUniform1i(shader.uniformColorSampler, 0))
    GL_OPERATION(glUniform1i(shader.uniformAlphaSampler, 1))
    // upload color uniform
    GL_OPERATION(glUniform4fv(shader.uniformColor, 1, color.rgba))
    return ref;
}

void RenderingSystem::drawRenderCommands(RenderQueue& commands) {
    // Worst case scenario: 3 vertices per triangle (no shared vertices)
    static glm::vec3 vertices[MAX_BATCH_TRIANGLE_COUNT * 3];
    static glm::vec2 uvs[MAX_BATCH_TRIANGLE_COUNT * 3];
    static unsigned short indices[MAX_BATCH_TRIANGLE_COUNT * 3];

    // Rendering state
    struct {
        TransformationComponent worldPos;
        CameraComponent cameraAttr;
    } camera;
    InternalTexture boundTexture = InternalTexture::Invalid;
    FramebufferRef fboRef = DefaultFrameBufferRef;
    EffectRef currentEffect = InvalidTextureRef;
    Color currentColor(1,1,1,1);
    int currentFlags = 0;
    bool useFbo = false;

    // Batch variable
    unsigned int batchVertexCount = 0, batchTriangleCount = 0;

    // matrices
    glm::mat4 camViewPerspMatrix;

    LOGV(2, "Begin frame rendering: " << commands.count);

    // Setup initial GL state
    GL_OPERATION(glDepthMask(true))
    GL_OPERATION(glEnable(GL_DEPTH_TEST))
    GL_OPERATION(glDepthFunc(GL_GREATER))
    GL_OPERATION(glActiveTexture(GL_TEXTURE1))
    GL_OPERATION(glBindTexture(GL_TEXTURE_2D, 0))

    #if SAC_DEBUG
    batchSizes.clear();
    #endif

    #define TEX chooseTextures(boundTexture, fboRef, useFbo)

    // The idea here is to browse through the list of _ordered_ list of
    // render command to execute. We try to group (batch) them in single
    // GL commands. When a GL state change is required (new color, new
    // texture, etc), we flush (execute) the active batch, and start
    // building a new one.
    const unsigned count = commands.count;
    for (unsigned i=0; i<count; i++) {
        RenderCommand& rc = commands.commands[i];

        // HANDLE BEGIN/END FRAME MARKERS (new frame OR new camera)
        if (rc.texture == BeginFrameMarker) {
            #if SAC_DEBUG
            batchSizes.push_back(std::make_pair(BatchFlushReason::NewCamera, batchTriangleCount));
            #endif
            batchTriangleCount = batchVertexCount = drawBatchES2(TEX, vertices, uvs, indices, batchVertexCount, batchTriangleCount);

            PROFILE("Render", "begin-render-frame", InstantEvent);

            firstCall = true;
            unpackCameraAttributes(rc, &camera.worldPos, &camera.cameraAttr);
            LOGV(2, "   camera: pos=" << camera.worldPos.position.x << ',' << camera.worldPos.position.y
                << "size=" << camera.worldPos.size.x << ',' << camera.worldPos.size.y
                << " fb=" << camera.cameraAttr.fb);

            FramebufferRef fboRef = camera.cameraAttr.fb;
            if (fboRef == DefaultFrameBufferRef) {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                GL_OPERATION(glViewport(0, 0, windowW, windowH))
            } else {
                const Framebuffer& fb = ref2Framebuffers[fboRef];
                glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
                GL_OPERATION(glViewport(0, 0, fb.width, fb.height))
            }

            // setup transformation matrix (based on camera attributes)
            camViewPerspMatrix =
                glm::ortho(-camera.worldPos.size.x * 0.5f, camera.worldPos.size.x * 0.5f,
                    -camera.worldPos.size.y * 0.5f, camera.worldPos.size.y * 0.5f,
                    0.0f, 1.0f) *
                glm::rotate( glm::mat4(1.0f),
                    -camera.worldPos.rotation, glm::vec3(0, 0, 1) ) *
                glm::translate( glm::mat4(1.0f),
                    glm::vec3(-camera.worldPos.position, 0.0f));

            // setup initial GL state
            currentEffect = changeShaderProgram(DefaultEffectRef, firstCall, true, currentColor, camViewPerspMatrix);
            GL_OPERATION(glDepthMask(true))
            GL_OPERATION(glDisable(GL_BLEND))
            GL_OPERATION(glColorMask(true, true, true, true))
            if (camera.cameraAttr.clear) {
                GL_OPERATION(glClearColor(camera.cameraAttr.clearColor.r, camera.cameraAttr.clearColor.g, camera.cameraAttr.clearColor.b, camera.cameraAttr.clearColor.a))
                GL_OPERATION(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT))
            }
            currentFlags = (EnableZWriteBit | DisableBlendingBit | EnableColorWriteBit);
            continue;
        } else if (rc.texture == EndFrameMarker) {
            break;
        }

        // HANDLE RENDERING FLAGS (GL state switch)
        if (rc.flags != currentFlags) {
            #if SAC_DEBUG
            batchSizes.push_back(std::make_pair(BatchFlushInfo(BatchFlushReason::NewFlags, rc.flags), batchTriangleCount));
            #endif
            // flush batch before changing state
            batchTriangleCount = batchVertexCount = drawBatchES2(TEX, vertices, uvs, indices, batchVertexCount, batchTriangleCount);
            const bool useTexturing = (rc.texture != InvalidTextureRef);

            if (rc.flags & EnableZWriteBit) {
                GL_OPERATION(glDepthMask(true))
            } else if (rc.flags & DisableZWriteBit) {
                GL_OPERATION(glDepthMask(false))
            } if (rc.flags & EnableBlendingBit) {
                firstCall = false;
                GL_OPERATION(glEnable(GL_BLEND))
                if (currentEffect == DefaultEffectRef) {
                    currentEffect = changeShaderProgram(DefaultEffectRef, firstCall, useTexturing, currentColor, camViewPerspMatrix);
                }
            } else if (rc.flags & DisableBlendingBit) {
                 GL_OPERATION(glDisable(GL_BLEND))
            } if (rc.flags & EnableColorWriteBit) {
                GL_OPERATION(glColorMask(true, true, true, true))
                if (!(currentFlags & EnableColorWriteBit)) {
                    if (currentEffect == DefaultEffectRef) {
                        currentEffect = changeShaderProgram(DefaultEffectRef, firstCall, useTexturing, currentColor, camViewPerspMatrix);
                    }
                }
            } else if (rc.flags & DisableColorWriteBit) {
                GL_OPERATION(glColorMask(false, false, false, false))
                if (!(currentFlags & DisableColorWriteBit)) {
                    if (currentEffect == DefaultEffectRef) {
                        currentEffect = changeShaderProgram(DefaultEffectRef, firstCall, useTexturing, currentColor, camViewPerspMatrix, false);
                    }
                }
            }
            currentFlags = rc.flags;
        }
        // EFFECT HAS CHANGED ?
        if (rc.effectRef != currentEffect) {
            #if SAC_DEBUG
            batchSizes.push_back(std::make_pair(BatchFlushReason::NewEffect, batchTriangleCount));
            #endif
            // flush before changing effect
            batchTriangleCount = batchVertexCount = drawBatchES2(TEX, vertices, uvs, indices, batchVertexCount, batchTriangleCount);
            const bool useTexturing = (rc.texture != InvalidTextureRef);
            currentEffect = changeShaderProgram(rc.effectRef, firstCall, useTexturing, currentColor, camViewPerspMatrix, currentFlags & EnableColorWriteBit);
        }

        // SETUP TEXTURING
#if SAC_DEBUG
        const TextureRef rrr = rc.texture;
#endif
        if (rc.texture != InvalidTextureRef) {
            if (!rc.fbo) {
                const TextureInfo* info = textureLibrary.get(rc.texture, false);
                LOGF_IF(info == 0, "Invalid texture " << rc.texture << " : can not be found");
                if (info->atlasIndex >= 0) {
                    LOGF_IF((unsigned)info->atlasIndex >= atlas.size(), "Invalid atlas index: " << info->atlasIndex << " >= atlas count : " << atlas.size());
                    const TextureInfo* atlasInfo = textureLibrary.get(atlas[info->atlasIndex].ref, false);
                    LOGF_IF(!atlasInfo, "TextureInfo for atlas index: "
                        << info->atlasIndex << " not found (ref=" << atlas[info->atlasIndex].ref << ", name='" << atlas[info->atlasIndex].name << "')");
                    rc.glref = atlasInfo->glref;
                } else {
                    rc.glref = info->glref;
                }
                computeUV(rc, *info);
            } else {
                rc.uv[0] = glm::vec2(0, 1);
                rc.uv[1] = glm::vec2(1, 0);
                rc.rotateUV = 0;
            }
            if (rc.glref.color == 0)
                rc.glref.color = whiteTexture;
        } else {
            rc.glref = InternalTexture::Invalid;
            // hum
            if (firstCall) {
                rc.glref.color = whiteTexture;
                rc.glref.alpha = whiteTexture;
            }
            rc.uv[0] = glm::vec2(0.0f, 0.0f);
            rc.uv[1] = glm::vec2(1.0f, 1.0f);
            rc.rotateUV = 0;
        }

        // TEXTURE OR COLOR HAS CHANGED ?
        const bool condUseFbo = (useFbo != rc.fbo);
        const bool condTexture = (!rc.fbo && boundTexture != rc.glref);
        const bool condFbo = (rc.fbo && fboRef != rc.framebuffer);
        const bool condColor = (currentColor != rc.color);
        if (condUseFbo | condTexture | condFbo | condColor) {
            #if SAC_DEBUG
            if (condUseFbo) {
                batchSizes.push_back(std::make_pair(BatchFlushReason::NewTarget, batchTriangleCount));
            } else if (condTexture) {
                batchSizes.push_back(std::make_pair(BatchFlushInfo(BatchFlushReason::NewTexture, rrr), batchTriangleCount));
            } else if (condColor) {
                batchSizes.push_back(std::make_pair(BatchFlushInfo(BatchFlushReason::NewColor, rc.color), batchTriangleCount));
            } else if (condColor) {
                batchSizes.push_back(std::make_pair(BatchFlushReason::NewFBO, batchTriangleCount));
            }
            #endif
            // flush before changing texture/color
            batchTriangleCount = batchVertexCount = drawBatchES2(TEX, vertices, uvs, indices, batchVertexCount, batchTriangleCount);
            if (rc.fbo) {
                fboRef = rc.framebuffer;
                boundTexture = InternalTexture::Invalid;
            } else {
                fboRef = DefaultFrameBufferRef;
                boundTexture = rc.glref;
#if SAC_INGAME_EDITORS
                if (highLight.nonOpaque)
                    boundTexture.alpha = whiteTexture;
#endif
            }
            useFbo = rc.fbo;
            if (currentColor != rc.color) {
                currentColor = rc.color;
                currentEffect =
                    changeShaderProgram(currentEffect, firstCall, (boundTexture != InternalTexture::Invalid), currentColor, camViewPerspMatrix, currentFlags & EnableColorWriteBit);
            }
        }

        // ADD TO BATCH
        addRenderCommandToBatch(rc,
            vertices + batchVertexCount,
            uvs + batchVertexCount,
            indices + (batchTriangleCount * 3),
            &batchVertexCount, &batchTriangleCount);

        if ((batchTriangleCount + 6) >= MAX_BATCH_TRIANGLE_COUNT) {
            #if SAC_DEBUG
            batchSizes.push_back(std::make_pair(BatchFlushReason::Full, batchTriangleCount));
            #endif
            batchTriangleCount = batchVertexCount = drawBatchES2(TEX, vertices, uvs, indices, batchVertexCount, batchTriangleCount);
        }
    }
    #if SAC_DEBUG
    batchSizes.push_back(std::make_pair(BatchFlushReason::End, batchTriangleCount));
    #endif
    drawBatchES2(TEX, vertices, uvs, indices, batchVertexCount, batchTriangleCount);

    #undef TEX

    #if SAC_DEBUG
    static unsigned ______debug = 0;
    if ((++______debug % 3000) == 0) {
        ______debug = 0;
        LOGI("Render command size: " << count << ". Drawn using: " << batchSizes.size() << " batches");
        for (unsigned i=0; i<batchSizes.size(); i++) {
            const auto& p = batchSizes[i];
            LOGI("   # batch " << i << ", size: "<< p.second << ", reason: " << p.first);
        }
    }
    batchSizes.clear();
    #endif
}

void RenderingSystem::waitDrawingComplete() {
#if ! SAC_EMSCRIPTEN
    PROFILE("Renderer", "wait-drawing-donE", BeginEvent);
    int readQueue = (currentWriteQueue + 1) % 2;
    std::unique_lock<std::mutex> lock(mutexes[L_RENDER]);
    while (renderQueue[readQueue].count > 0 && frameQueueWritable)
        cond[C_RENDER_DONE].wait(lock);
    lock.unlock();
    PROFILE("Renderer", "wait-drawing-donE", EndEvent);
#endif
}

bool RenderingSystem::render() {
#if ! SAC_EMSCRIPTEN
    if (!initDone)
        return true;
    PROFILE("Renderer", "wait-frame", BeginEvent);

    std::unique_lock<std::mutex> lock(mutexes[L_QUEUE]);

    // processDelayedTextureJobs();
    
    while (!newFrameReady && frameQueueWritable) {
        cond[C_FRAME_READY].wait(lock);
    }

    bool hasFrame = newFrameReady;
    newFrameReady = false;
#endif
    int readQueue = (currentWriteQueue + 1) % 2;

    if (!frameQueueWritable) {
        LOGV(1, "Rendering disabled");
        renderQueue[readQueue].count = 0;
#if ! SAC_EMSCRIPTEN
        lock.unlock();
#endif
        return false;
    }
#if ! SAC_EMSCRIPTEN
    lock.unlock();
    PROFILE("Renderer", "wait-frame", EndEvent);
#endif
    PROFILE("Renderer", "load-textures", BeginEvent);
    processDelayedTextureJobs();
#if SAC_ENABLE_LOG && ! SAC_EMSCRIPTEN
    //float aftertexture= TimeUtil::GetTime();
#endif
    PROFILE("Renderer", "load-textures", EndEvent);
#if ! SAC_EMSCRIPTEN
    if (!mutexes[L_RENDER].try_lock()) {
        LOGV(1, "HMM Busy render lock");
        mutexes[L_RENDER].lock();
    }
#endif

    PROFILE("Renderer", "render", BeginEvent);
    if (renderQueue[readQueue].count == 0) {
        if (hasFrame)
            LOGW("Arg, nothing to render - probably a bug (queue=" << readQueue << ')');
    } else {
        RenderQueue& inQueue = renderQueue[readQueue];
        drawRenderCommands(inQueue);
        inQueue.count = 0;
    }
#if ! SAC_EMSCRIPTEN
    cond[C_RENDER_DONE].notify_all();
    mutexes[L_RENDER].unlock();
#endif
    PROFILE("Renderer", "render", EndEvent);
#if SAC_INGAME_EDITORS
    LevelEditor::lock();
    TwDraw();
    LevelEditor::unlock();
#endif
    return hasFrame;
}

static void computeVerticesScreenPos(const std::vector<glm::vec2>& points, const glm::vec2& position, const glm::vec2& hSize, float rotation, float z, int rotateUV, glm::vec3* out) {
    const auto b = out;
    for (const auto& p: points) {
        *out++ = glm::vec3(position + glm::rotate(p * (2.0f * hSize), rotation), z);
    }

    if (/*shape.supportUV &&*/ rotateUV) {
        std::swap(*(b + 1), *(b + 0)); // 0 is correct, 1 holds 0
        std::swap(*(b + 1), *(b + 2)); // 2 is correct, 1 holds 2
        std::swap(*(b + 1), *(b + 3));
    }
}

const Shader& RenderingSystem::effectRefToShader(EffectRef ref, bool firstCall, bool colorEnabled, bool hasTexture) {
    if (ref == DefaultEffectRef) {
        if (colorEnabled) {
            if (firstCall) {
                ref = defaultShaderNoAlpha;
            } else if (hasTexture) {
                ref = defaultShader;
            } else {
                ref = defaultShaderNoTexture;
            }
        } else {
            ref = defaultShaderEmpty;
        }
    }
    return *effectLibrary.get(ref, false);
}
