#include <memory>
#include "glslang/SPIRV/GlslangToSpv.h"
#include "glslang/glslang/Public/ShaderLang.h"
#include "glslang/glslang/Include/BaseTypes.h"
#include "gft/glslang.hpp"
#include "gft/assert.hpp"
#include "gft/log.hpp"

namespace liong {

namespace glslang {

::glslang::EshTargetClientVersion TARGET_CLIENT_VER = ::glslang::EshTargetClientVersion::EShTargetVulkan_1_0;
::glslang::EShTargetLanguageVersion TARGET_LANG_VER = ::glslang::EShTargetLanguageVersion::EShTargetSpv_1_0;

void _initialize(bool silent) {
  static bool is_initialized = false;
  if (is_initialized) {
    if (!silent) {
      L_WARN("ignored redundant glslang module initialization");
    }
    return;
  }

  bool res = ::glslang::InitializeProcess();
  L_ASSERT(res, "cannot initialize glslang");
  ::glslang::Version glslang_ver = ::glslang::GetVersion();
  L_INFO("glslang version: ", glslang_ver.major, ".",
    glslang_ver.minor, ".", glslang_ver.patch);

  const char* glsl_ver_str = ::glslang::GetGlslVersionString();
  L_INFO("supported glsl version: ", glsl_ver_str);

  is_initialized = true;
}
void initialize(GlslangTarget target) {
  switch (target) {
  case L_GLSLANG_TARGET_VULKAN_1_0:
    TARGET_CLIENT_VER = ::glslang::EshTargetClientVersion::EShTargetVulkan_1_0;
    TARGET_LANG_VER = ::glslang::EShTargetLanguageVersion::EShTargetSpv_1_0;
    break;
  case L_GLSLANG_TARGET_VULKAN_1_1:
    TARGET_CLIENT_VER = ::glslang::EshTargetClientVersion::EShTargetVulkan_1_1;
    TARGET_LANG_VER = ::glslang::EShTargetLanguageVersion::EShTargetSpv_1_3;
    break;
  case L_GLSLANG_TARGET_VULKAN_1_2:
    TARGET_CLIENT_VER = ::glslang::EshTargetClientVersion::EShTargetVulkan_1_2;
    TARGET_LANG_VER = ::glslang::EShTargetLanguageVersion::EShTargetSpv_1_5;
    break;
  default:
    L_PANIC("invalid glslang target");
  }
  _initialize(false);
}
void initialize() {
  initialize(L_GLSLANG_TARGET_VULKAN_1_0);
}

std::unique_ptr<::glslang::TProgram> _create_program() {
  _initialize(true);
  return std::make_unique<::glslang::TProgram>();
}

// See `third\glslang\StandAlone\ResourceLimits.cpp`.
TBuiltInResource make_default_builtin_rsc() {
  TBuiltInResource builtin_rsc {};
  builtin_rsc.maxLights = 32;
  builtin_rsc.maxClipPlanes = 6;
  builtin_rsc.maxTextureUnits = 32;
  builtin_rsc.maxTextureCoords = 32;
  builtin_rsc.maxVertexAttribs = 64;
  builtin_rsc.maxVertexUniformComponents = 4096;
  builtin_rsc.maxVaryingFloats = 64;
  builtin_rsc.maxVertexTextureImageUnits = 32;
  builtin_rsc.maxCombinedTextureImageUnits = 80;
  builtin_rsc.maxTextureImageUnits = 32;
  builtin_rsc.maxFragmentUniformComponents = 4096;
  builtin_rsc.maxDrawBuffers = 32;
  builtin_rsc.maxVertexUniformVectors = 128;
  builtin_rsc.maxVaryingVectors = 8;
  builtin_rsc.maxFragmentUniformVectors = 16;
  builtin_rsc.maxVertexOutputVectors = 16;
  builtin_rsc.maxFragmentInputVectors = 15;
  builtin_rsc.minProgramTexelOffset = -8;
  builtin_rsc.maxProgramTexelOffset = 7;
  builtin_rsc.maxClipDistances = 8;
  builtin_rsc.maxComputeWorkGroupCountX = 65535;
  builtin_rsc.maxComputeWorkGroupCountY = 65535;
  builtin_rsc.maxComputeWorkGroupCountZ = 65535;
  builtin_rsc.maxComputeWorkGroupSizeX = 1024;
  builtin_rsc.maxComputeWorkGroupSizeY = 1024;
  builtin_rsc.maxComputeWorkGroupSizeZ = 64;
  builtin_rsc.maxComputeUniformComponents = 1024;
  builtin_rsc.maxComputeTextureImageUnits = 16;
  builtin_rsc.maxComputeImageUniforms = 8;
  builtin_rsc.maxComputeAtomicCounters = 8;
  builtin_rsc.maxComputeAtomicCounterBuffers = 1;
  builtin_rsc.maxVaryingComponents = 60;
  builtin_rsc.maxVertexOutputComponents = 64;
  builtin_rsc.maxGeometryInputComponents = 64;
  builtin_rsc.maxGeometryOutputComponents = 128;
  builtin_rsc.maxFragmentInputComponents = 128;
  builtin_rsc.maxImageUnits = 8;
  builtin_rsc.maxCombinedImageUnitsAndFragmentOutputs = 8;
  builtin_rsc.maxCombinedShaderOutputResources = 8;
  builtin_rsc.maxImageSamples = 0;
  builtin_rsc.maxVertexImageUniforms = 0;
  builtin_rsc.maxTessControlImageUniforms = 0;
  builtin_rsc.maxTessEvaluationImageUniforms = 0;
  builtin_rsc.maxGeometryImageUniforms = 0;
  builtin_rsc.maxFragmentImageUniforms = 8;
  builtin_rsc.maxCombinedImageUniforms = 8;
  builtin_rsc.maxGeometryTextureImageUnits = 16;
  builtin_rsc.maxGeometryOutputVertices = 256;
  builtin_rsc.maxGeometryTotalOutputComponents = 1024;
  builtin_rsc.maxGeometryUniformComponents = 1024;
  builtin_rsc.maxGeometryVaryingComponents = 64;
  builtin_rsc.maxTessControlInputComponents = 128;
  builtin_rsc.maxTessControlOutputComponents = 128;
  builtin_rsc.maxTessControlTextureImageUnits = 16;
  builtin_rsc.maxTessControlUniformComponents = 1024;
  builtin_rsc.maxTessControlTotalOutputComponents = 4096;
  builtin_rsc.maxTessEvaluationInputComponents = 128;
  builtin_rsc.maxTessEvaluationOutputComponents = 128;
  builtin_rsc.maxTessEvaluationTextureImageUnits = 16;
  builtin_rsc.maxTessEvaluationUniformComponents = 1024;
  builtin_rsc.maxTessPatchComponents = 120;
  builtin_rsc.maxPatchVertices = 32;
  builtin_rsc.maxTessGenLevel = 64;
  builtin_rsc.maxViewports = 16;
  builtin_rsc.maxVertexAtomicCounters = 0;
  builtin_rsc.maxTessControlAtomicCounters = 0;
  builtin_rsc.maxTessEvaluationAtomicCounters = 0;
  builtin_rsc.maxGeometryAtomicCounters = 0;
  builtin_rsc.maxFragmentAtomicCounters = 8;
  builtin_rsc.maxCombinedAtomicCounters = 8;
  builtin_rsc.maxAtomicCounterBindings = 1;
  builtin_rsc.maxVertexAtomicCounterBuffers = 0;
  builtin_rsc.maxTessControlAtomicCounterBuffers = 0;
  builtin_rsc.maxTessEvaluationAtomicCounterBuffers = 0;
  builtin_rsc.maxGeometryAtomicCounterBuffers = 0;
  builtin_rsc.maxFragmentAtomicCounterBuffers = 1;
  builtin_rsc.maxCombinedAtomicCounterBuffers = 1;
  builtin_rsc.maxAtomicCounterBufferSize = 16384;
  builtin_rsc.maxTransformFeedbackBuffers = 4;
  builtin_rsc.maxTransformFeedbackInterleavedComponents = 64;
  builtin_rsc.maxCullDistances = 8;
  builtin_rsc.maxCombinedClipAndCullDistances = 8;
  builtin_rsc.maxSamples = 4;
  builtin_rsc.maxMeshOutputVerticesNV = 256;
  builtin_rsc.maxMeshOutputPrimitivesNV = 512;
  builtin_rsc.maxMeshWorkGroupSizeX_NV = 32;
  builtin_rsc.maxMeshWorkGroupSizeY_NV = 1;
  builtin_rsc.maxMeshWorkGroupSizeZ_NV = 1;
  builtin_rsc.maxTaskWorkGroupSizeX_NV = 32;
  builtin_rsc.maxTaskWorkGroupSizeY_NV = 1;
  builtin_rsc.maxTaskWorkGroupSizeZ_NV = 1;
  builtin_rsc.maxMeshViewCountNV = 4;
  builtin_rsc.maxDualSourceDrawBuffersEXT = 1;
  builtin_rsc.limits.nonInductiveForLoops = 1;
  builtin_rsc.limits.whileLoops = 1;
  builtin_rsc.limits.doWhileLoops = 1;
  builtin_rsc.limits.generalUniformIndexing = 1;
  builtin_rsc.limits.generalAttributeMatrixVectorIndexing = 1;
  builtin_rsc.limits.generalVaryingIndexing = 1;
  builtin_rsc.limits.generalSamplerIndexing = 1;
  builtin_rsc.limits.generalVariableIndexing = 1;
  builtin_rsc.limits.generalConstantMatrixVectorIndexing = 1;
  return builtin_rsc;
}

class Glsl2Spv {
private:
  using Shader = ::glslang::TShader;
  using Program = ::glslang::TProgram;

  std::unique_ptr<Program> program;
  std::vector<std::unique_ptr<Shader>> shaders;

public:
  Glsl2Spv() :
    program(_create_program()),
    shaders() {}

  constexpr static EShMessages MSG_OPTS = (EShMessages)(
    EShMsgSpvRules |
    EShMsgVulkanRules |
    EShMsgDebugInfo);

  Glsl2Spv& with_shader(
    EShLanguage stage,
    const std::string& src,
    const std::string& entry_point
  ) {
    auto shader = std::make_unique<Shader>(stage);

    auto src_c_str = src.c_str();
    auto ver = 100;
    auto src_lang          = ::glslang::EShSourceGlsl;
    auto target_client     = ::glslang::EShClientVulkan;
    auto target_client_ver = TARGET_CLIENT_VER;
    auto target_lang       = ::glslang::EShTargetSpv;
    auto target_lang_ver   = TARGET_LANG_VER;
    shader->setStrings(&src_c_str, 1);
    shader->setEntryPoint("main");
    shader->setSourceEntryPoint(entry_point.c_str());
    shader->setEnvInput(src_lang, stage, target_client, ver);
    shader->setEnvClient(target_client, target_client_ver);
    shader->setEnvTarget(target_lang, target_lang_ver);
    // Allow auto-mapping to ease language constructs.
    shader->setAutoMapBindings(true);
    shader->setAutoMapLocations(true);

    TBuiltInResource builtin_rsc = make_default_builtin_rsc();
    ::glslang::TShader::ForbidIncluder includer;

    std::string dummy;
    if (!shader->preprocess(&builtin_rsc, ver, ENoProfile, false, false,
      MSG_OPTS, &dummy, includer)
    ) {
      panic("preprocessing failed: ", shader->getInfoLog());
    }

    if (!shader->parse(&builtin_rsc, ver, false, MSG_OPTS, includer)) {
      panic("compilation failed: ", shader->getInfoLog());
    }

    program->addShader(&*shader);
    shaders.emplace_back(std::move(shader));
    return *this;
  }
  void link() {
    if (!program->link(MSG_OPTS)) {
      panic("link failed: ", program->getInfoLog());
    }
  }
  std::vector<uint32_t> to_spv(EShLanguage stage) {
    ::spv::SpvBuildLogger logger;
    ::glslang::SpvOptions opts;
    opts.validate          = true;
    opts.generateDebugInfo = true;
    opts.optimizeSize      = true;
    opts.stripDebugInfo    = false;
    opts.disableOptimizer  = false;
    opts.disassemble       = false;

    std::vector<uint32_t> spv;
    ::glslang::GlslangToSpv(*program->getIntermediate((EShLanguage)stage), spv,
      &logger, &opts);

    std::string msg = logger.getAllMessages();
    if (msg.size() != 0) {
      L_WARN("compiler reported issues when translating glsl into spirv",
        msg);
    }

    return spv;
  }
  void reflect(size_t& ubo_size) {
    if (!program->buildReflection(MSG_OPTS)) {
      panic("reflection failed: ", program->getInfoLog());
    }
    // Uncomment this line to print reflection details:
    //program->dumpReflection();

    bool is_graph_pipe = program->getShaders(EShLangCompute).size() == 0;

    // Collect uniform block size.
    {
      uint32_t nubo = program->getNumUniformBlocks();
      L_ASSERT(nubo <= 1, "a pipeline can only bind to one uniform block");
      if (nubo != 0) {
        auto ubo = program->getUniformBlock(0);
        L_ASSERT(ubo.getBinding() == 0, "uniform block binding point must be 0");
        L_ASSERT(ubo.size != 0, "unexpected zero-sized uniform block");
        ubo_size = ubo.size;
      } else {
        ubo_size = 0;
      }
    }

    // Make sure there is exactly one fragment output.
    if (is_graph_pipe) {
      L_ASSERT(program->getNumPipeOutputs() == 1, "must have exactly one "
        "fragment output");
      const auto output = program->getPipeOutput(0);
      L_ASSERT(output.index == 0, "fragment output must be bound to location 0");
    }
  }
};

class Hlsl2Spv {
private:
  using Shader = ::glslang::TShader;
  using Program = ::glslang::TProgram;

  std::unique_ptr<Program> program;
  std::vector<std::unique_ptr<Shader>> shaders;

public:
  Hlsl2Spv() :
    program(_create_program()),
    shaders() {}

  constexpr static EShMessages MSG_OPTS = (EShMessages)(
    EShMsgSpvRules |
    EShMsgVulkanRules |
    EShMsgDebugInfo);

  Hlsl2Spv& with_shader(
    EShLanguage stage,
    const std::string& src,
    const std::string& entry_point
  ) {
    auto shader = std::make_unique<Shader>(stage);

    auto src_c_str = src.c_str();
    auto ver = 100;
    auto src_lang          = ::glslang::EShSourceHlsl;
    auto target_client     = ::glslang::EShClientVulkan;
    auto target_client_ver = TARGET_CLIENT_VER;
    auto target_lang       = ::glslang::EShTargetSpv;
    auto target_lang_ver   = TARGET_LANG_VER;
    shader->setStrings(&src_c_str, 1);
    shader->setEntryPoint("main");
    shader->setSourceEntryPoint(entry_point.c_str());
    shader->setEnvInput(src_lang, stage, target_client, ver);
    shader->setEnvClient(target_client, target_client_ver);
    shader->setEnvTarget(target_lang, target_lang_ver);
    // Allow auto-mapping to ease language constructs.
    shader->setAutoMapBindings(true);
    shader->setAutoMapLocations(true);

    TBuiltInResource builtin_rsc = make_default_builtin_rsc();
    ::glslang::TShader::ForbidIncluder includer;

    std::string dummy;
    if (!shader->preprocess(&builtin_rsc, ver, ENoProfile, false, false,
      MSG_OPTS, &dummy, includer)
    ) {
      panic("preprocessing failed: ", shader->getInfoLog());
    }

    if (!shader->parse(&builtin_rsc, ver, false, MSG_OPTS, includer)) {
      panic("compilation failed: ", shader->getInfoLog());
    }

    program->addShader(&*shader);
    shaders.emplace_back(std::move(shader));
    return *this;
  }
  void link() {
    if (!program->link(MSG_OPTS)) {
      panic("link failed: ", program->getInfoLog());
    }
  }
  std::vector<uint32_t> to_spv(EShLanguage stage) {
    ::spv::SpvBuildLogger logger;
    ::glslang::SpvOptions opts;
    opts.validate          = true;
    opts.generateDebugInfo = true;
    opts.optimizeSize      = true;
    opts.stripDebugInfo    = false;
    opts.disableOptimizer  = false;
    opts.disassemble       = false;

    std::vector<uint32_t> spv;
    ::glslang::GlslangToSpv(*program->getIntermediate((EShLanguage)stage), spv,
      &logger, &opts);

    std::string msg = logger.getAllMessages();
    if (msg.size() != 0) {
      L_WARN("compiler reported issues when translating glsl into "
        "spirv", msg);
    }

    return spv;
  }
  void reflect(size_t& ubo_size) {
    if (!program->buildReflection(MSG_OPTS)) {
      panic("reflection failed: ", program->getInfoLog());
    }
    // Uncomment this line to print reflection details:
    //program->dumpReflection();

    bool is_graph_pipe = program->getShaders(EShLangCompute).size() == 0;

    // Collect uniform block size.
    {
      uint32_t nubo = program->getNumUniformBlocks();
      L_ASSERT(nubo <= 1, "a pipeline can only bind to one uniform block");
      if (nubo != 0) {
        auto ubo = program->getUniformBlock(0);
        L_ASSERT(ubo.getBinding(), "uniform block binding point must be 0");
        L_ASSERT(ubo.size != 0, "unexpected zero-sized uniform block");
        ubo_size = ubo.size;
      } else {
        ubo_size = 0;
      }
    }

    // Make sure there is exactly one fragment output.
    if (is_graph_pipe) {
      L_ASSERT(program->getNumPipeOutputs() == 1,
        "must have exactly one fragment output");
      const auto output = program->getPipeOutput(0);
      L_ASSERT(output.index == 0,
        "fragment output must be bound to location 0");
    }
  }
};

ComputeSpirvArtifact compile_comp(
  const std::string& comp_src,
  const std::string& comp_entry_point
) {
  Glsl2Spv conv {};
  conv
    .with_shader(EShLangCompute, comp_src, comp_entry_point)
    .link();
  size_t ubo_size;
  conv.reflect(ubo_size);
  return {
    conv.to_spv(EShLangCompute),
    ubo_size
  };
}
ComputeSpirvArtifact compile_comp_hlsl(
  const std::string& comp_src,
  const std::string& comp_entry_point
) {
  Hlsl2Spv conv {};
  conv
    .with_shader(EShLangCompute, comp_src, comp_entry_point)
    .link();
  size_t ubo_size;
  conv.reflect(ubo_size);
  return {
    conv.to_spv(EShLangCompute),
    ubo_size
  };
}

GraphicsSpirvArtifact compile_graph(
  const std::string& vert_src,
  const std::string& vert_entry_point,
  const std::string& frag_src,
  const std::string& frag_entry_point
) {
  Glsl2Spv conv {};
  conv
    .with_shader(EShLangVertex, vert_src, vert_entry_point)
    .with_shader(EShLangFragment, frag_src, frag_entry_point)
    .link();
  size_t ubo_size;
  conv.reflect(ubo_size);
  return {
    conv.to_spv(EShLangVertex),
    conv.to_spv(EShLangFragment),
    ubo_size
  };
}
GraphicsSpirvArtifact compile_graph_hlsl(
  const std::string& vert_src,
  const std::string& vert_entry_point,
  const std::string& frag_src,
  const std::string& frag_entry_point
) {
  Hlsl2Spv conv {};
  conv
    .with_shader(EShLangVertex, vert_src, vert_entry_point)
    .with_shader(EShLangFragment, frag_src, frag_entry_point)
    .link();
  size_t ubo_size;
  conv.reflect(ubo_size);
  return {
    conv.to_spv(EShLangVertex),
    conv.to_spv(EShLangFragment),
    ubo_size
  };
}

} // namespace glslang

} // namespace liong
