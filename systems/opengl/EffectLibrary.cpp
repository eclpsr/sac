#include "EffectLibrary.h"

#include "shaders/default_fs.h"
#ifdef USE_VBO
#include "shaders/default_vbo_vs.h"
#define VERTEX_SHADER_ARRAY ___assets_default_vbo_vs
#define VERTEX_SHADER_SIZE ___assets_default_vbo_vs_len
#else
#include "shaders/default_vs.h"
#define VERTEX_SHADER_ARRAY ___assets_default_vs
#define VERTEX_SHADER_SIZE ___assets_default_vs_len
#endif

#ifdef WINDOWS
#include "base/Log.h"
#else
#include <glog/logging.h>
#endif

static GLuint compileShader(const std::string& assetName, GLuint type, const FileBuffer& fb) {
    VLOG(1) << "Compiling " << ((type == GL_VERTEX_SHADER) ? "vertex" : "fragment") << " shader...";

    GLuint shader = glCreateShader(type);
    GL_OPERATION(glShaderSource(shader, 1, (const char**)&fb.data, &fb.size))
    GL_OPERATION(glCompileShader(shader))

    GLint logLength;
    GL_OPERATION(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength))
    if (logLength > 1) {
        char *log = new char[logLength];
        GL_OPERATION(glGetShaderInfoLog(shader, logLength, &logLength, log))
        LOG(FATAL) << "GL shader error: " << log;
        delete[] log;
    }

   if (!glIsShader(shader)) {
        LOG(ERROR) << "Weird; " << shader << "d is not a shader";
   }
    return shader;
}

static Shader buildShaderFromFileBuffer(const std::string& vsName, const FileBuffer& fragmentFb) {
    Shader out;
    VLOG(1) << "building shader ...";
    out.program = glCreateProgram();
    check_GL_errors("glCreateProgram");

    FileBuffer vertexFb;
    vertexFb.data = VERTEX_SHADER_ARRAY;
    vertexFb.size = VERTEX_SHADER_SIZE;
    GLuint vs = compileShader(vsName, GL_VERTEX_SHADER, vertexFb);

    GLuint fs = compileShader("unknown.fs", GL_FRAGMENT_SHADER, fragmentFb);

    GL_OPERATION(glAttachShader(out.program, vs))
    GL_OPERATION(glAttachShader(out.program, fs))
    VLOG(2) << "Binding GLSL attribs";
    GL_OPERATION(glBindAttribLocation(out.program, EffectLibrary::ATTRIB_VERTEX, "aPosition"))
    GL_OPERATION(glBindAttribLocation(out.program, EffectLibrary::ATTRIB_UV, "aTexCoord"))
    GL_OPERATION(glBindAttribLocation(out.program, EffectLibrary::ATTRIB_POS_ROT, "aPosRot"))

    VLOG(2) << "Linking GLSL program";
    GL_OPERATION(glLinkProgram(out.program))

    GLint logLength;
    glGetProgramiv(out.program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1) {
        char *log = new char[logLength];
        glGetProgramInfoLog(out.program, logLength, &logLength, log);
        LOG(FATAL) << "GL shader program error: '" << log << "'";
        
        delete[] log;
    }

    out.uniformMatrix = glGetUniformLocation(out.program, "uMvp");
    out.uniformColorSampler = glGetUniformLocation(out.program, "tex0");
    out.uniformAlphaSampler = glGetUniformLocation(out.program, "tex1");
    out.uniformColor= glGetUniformLocation(out.program, "vColor");
    out.uniformCamera = glGetUniformLocation(out.program, "uCamera");
#ifdef USE_VBO
    out.uniformUVScaleOffset = glGetUniformLocation(out.program, "uvScaleOffset");
    out.uniformRotation = glGetUniformLocation(out.program, "uRotation");
    out.uniformScaleZ = glGetUniformLocation(out.program, "uScaleZ");
#endif

    glDeleteShader(vs);
    glDeleteShader(fs);

    return out;
}

static Shader buildShaderFromAsset(AssetAPI* assetAPI, const std::string& vsName, const std::string& fsName) {
    VLOG(1) << "Compiling shaders: " << vsName << '/' << fsName;
    FileBuffer fragmentFb = assetAPI->loadAsset(fsName);
    Shader shader = buildShaderFromFileBuffer(vsName, fragmentFb);
    delete[] fragmentFb.data;
    return shader;
}

void EffectLibrary::init(AssetAPI* pAssetAPI) {
    NamedAssetLibrary<Shader, EffectRef, FileBuffer>::init(pAssetAPI);
    FileBuffer fb;
    fb.data = ___assets_default_fs;
    fb.size = ___assets_default_fs_len;
    registerDataSource(load(DEFAULT_FRAGMENT), fb);
}

bool EffectLibrary::doLoad(const std::string& assetName, Shader& out, const EffectRef& ref) {
    LOG_IF(FATAL, assetAPI == 0) << "Unitialized assetAPI member";

    std::map<EffectRef, FileBuffer>::iterator it = dataSource.find(ref);
    if (it == dataSource.end()) {
        VLOG(1) << "loadShader: '" << assetName << "' from file";
        out = buildShaderFromAsset(assetAPI, "default.vs", assetName);
    } else {
        const FileBuffer& fb = it->second;
        VLOG(1) << "loadShader: '" << assetName << "' from InMemoryShader (" << fb.size << ')';
        out = buildShaderFromFileBuffer("default.vs", fb);
    }

    return true;
}

void EffectLibrary::doUnload(const std::string& name, const Shader& in) {
    LOG(WARNING) << "TODO";
}

void EffectLibrary::doReload(const std::string& name, const EffectRef& ref) {
    Shader& info = ref2asset[ref];
    doLoad(name, ref2asset[ref], ref);
}