#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/shader.h"


AL2O3_EXTERN_C Render_ShaderObjectHandle Render_ShaderObjectCreate(Render_RendererHandle renderer, Render_ShaderObjectDesc const * desc) {

	auto shaderObject = (Render_ShaderObject*)MEMORY_CALLOC(1, sizeof(Render_ShaderObject));
	if(!shaderObject) {
		return nullptr;
	}

	ShaderCompiler_ShaderType scType = ShaderCompiler_ST_VertexShader;
	switch(desc->shaderType) {
		case Render_ST_VERTEXSHADER: scType = ShaderCompiler_ST_VertexShader; break;
		case Render_ST_FRAGMENTSHADER: scType = ShaderCompiler_ST_FragmentShader; break;
		case Render_ST_COMPUTESHADER: scType = ShaderCompiler_ST_ComputeShader; break;
		case Render_ST_GEOMETRYSHADER: scType = ShaderCompiler_ST_GeometryShader; break;
		case Render_ST_TESSCONTROLSHADER: scType = ShaderCompiler_ST_TessControlShader; break;
		case Render_ST_TESSEVALUATIONSHADER: scType = ShaderCompiler_ST_TessEvaluationShader; break;
		case Render_ST_TASKSHADER: scType = ShaderCompiler_ST_TaskShader; break;
		case Render_ST_MESHSHADER: scType = ShaderCompiler_ST_MeshShader; break;
	}

	bool vokay = ShaderCompiler_Compile(
			renderer->shaderCompiler,
			scType,
			VFile_GetName(desc->file),
			desc->entryPoint,
			desc->file,
			&shaderObject->output);

	if (shaderObject->output.log != nullptr) {
		LOGWARNING("Shader compiler : %s %s", vokay ? "warnings" : "ERROR", shaderObject->output.log);
	}
	if (!vokay) {
		MEMORY_FREE((void *) shaderObject->output.log);
		MEMORY_FREE((void *) shaderObject->output.shader);
		MEMORY_FREE(shaderObject);
		return nullptr;
	}
	return shaderObject;
}

AL2O3_EXTERN_C Render_ShaderHandle Render_ShaderCreate(Render_RendererHandle renderer, Render_ShaderDesc const * desc) {
/*
#if AL2O3_PLATFORM == AL2O3_PLATFORM_APPLE_MAC
	TheForge_ShaderDesc sdesc;
	sdesc.stages = (TheForge_ShaderStage) (TheForge_SS_FRAG | TheForge_SS_VERT);
	sdesc.vert.name = "ImguiBindings_VertexShader";
	sdesc.vert.code = (char *) vout.shader;
	sdesc.vert.entryPoint = vertEntryPoint;
	sdesc.vert.macroCount = 0;
	sdesc.frag.name = "ImguiBindings_FragmentShader";
	sdesc.frag.code = (char *) fout.shader;
	sdesc.frag.entryPoint = fragEntryPoint;
	sdesc.frag.macroCount = 0;
	TheForge_AddShader(ctx->renderer->renderer, &sdesc, &ctx->shader);
#else
	TheForge_BinaryShaderDesc bdesc;
	bdesc.stages = (TheForge_ShaderStage) (TheForge_SS_FRAG | TheForge_SS_VERT);
	bdesc.vert.byteCode = (char *) vout.shader;
	bdesc.vert.byteCodeSize = (uint32_t) vout.shaderSize;
	bdesc.vert.entryPoint = vertEntryPoint;
	bdesc.frag.byteCode = (char *) fout.shader;
	bdesc.frag.byteCodeSize = (uint32_t) fout.shaderSize;
	bdesc.frag.entryPoint = fragEntryPoint;
	TheForge_AddShaderBinary(ctx->renderer->renderer, &bdesc, &ctx->shader);
#endif
 */
}