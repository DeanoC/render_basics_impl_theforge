#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/shader.h"
#include "render_basics/theforge/handlemanager.h"

AL2O3_EXTERN_C Render_ShaderObjectHandle Render_ShaderObjectCreate(Render_RendererHandle renderer, Render_ShaderObjectDesc const * desc) {

	Render_ShaderObjectHandle handle = Render_ShaderObjectHandleAlloc();

	Render_ShaderObject* shaderObject = Render_ShaderObjectHandleToPtr(handle);
	if(!shaderObject) {
		return {Handle_InvalidDynamicHandle32};
	}

	strncpy(shaderObject->entryPoint, desc->entryPoint, 63);
	shaderObject->entryPoint[63] = 0;
	strncpy(shaderObject->name, VFile_GetName(desc->file), 63);
	shaderObject->name[63] = 0;

	ShaderCompiler_ShaderType scType = ShaderCompiler_ST_VertexShader;
	switch(desc->shaderType) {
		case Render_ST_VERTEXSHADER: scType = ShaderCompiler_ST_VertexShader;
			shaderObject->shaderType = TheForge_SS_VERT;
			break;
		case Render_ST_FRAGMENTSHADER: scType = ShaderCompiler_ST_FragmentShader;
			shaderObject->shaderType = TheForge_SS_FRAG;
			break;
		case Render_ST_COMPUTESHADER: scType = ShaderCompiler_ST_ComputeShader;
			shaderObject->shaderType = TheForge_SS_COMP;
			break;
		case Render_ST_GEOMETRYSHADER: scType = ShaderCompiler_ST_GeometryShader;
			shaderObject->shaderType = TheForge_SS_GEOM;
			break;
		case Render_ST_TESSCONTROLSHADER: scType = ShaderCompiler_ST_TessControlShader;
			shaderObject->shaderType = TheForge_SS_TESE;
			break;
		case Render_ST_TESSEVALUATIONSHADER: scType = ShaderCompiler_ST_TessEvaluationShader;
			shaderObject->shaderType = TheForge_SS_TESC;
			break;
			/*		case Render_ST_TASKSHADER:
						scType = ShaderCompiler_ST_TaskShader;
						shaderObject->shaderType = TheForge_SS_TASK;
						break;
					case Render_ST_MESHSHADER:
						scType = ShaderCompiler_ST_MeshShader;
						shaderObject->shaderType = TheForge_SS_MESH;
						break;
					*/
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
		Render_ShaderObjectHandleRelease(handle);
		return {Handle_InvalidDynamicHandle32};
	}

	return handle;
}
AL2O3_EXTERN_C Render_ShaderHandle Render_ShaderCreate(Render_RendererHandle renderer,
																											 uint32_t count,
																											 Render_ShaderObjectHandle *shaderObjects) {
#if AL2O3_PLATFORM == AL2O3_PLATFORM_APPLE_MAC
	TheForge_ShaderDesc sdesc {};
#else
	TheForge_BinaryShaderDesc sdesc{};
#endif
	for (uint32_t i = 0; i < count; ++i) {
		Render_ShaderObject* shaderObject = Render_ShaderObjectHandleToPtr(shaderObjects[i]);
		sdesc.stages = (TheForge_ShaderStage) (sdesc.stages | shaderObject->shaderType);

#if AL2O3_PLATFORM == AL2O3_PLATFORM_APPLE_MAC
		TheForge_ShaderStageDesc ssdesc {};
		ssdesc.entryPoint = shaderObject->entryPoint;
		ssdesc.code = (char const*)shaderObject->output.shader;
		ssdesc.name = shaderObject->name;
		ssdesc.macroCount = 0; // TODO
//		LOGINFO(ssdesc.code);
#else
		TheForge_BinaryShaderStageDesc ssdesc{};
		ssdesc.entryPoint = shaderObject->entryPoint;
		ssdesc.byteCode = (char const *) shaderObject->output.shader;
		ssdesc.byteCodeSize = (uint32_t) shaderObject->output.shaderSize;
#endif

		switch (shaderObject->shaderType) {
			case TheForge_SS_VERT: sdesc.vert = ssdesc;
				break;
			case TheForge_SS_FRAG: sdesc.frag = ssdesc;
				break;
			case TheForge_SS_TESC: sdesc.domain = ssdesc;
				break;
			case TheForge_SS_TESE: sdesc.hull = ssdesc;
				break;
			case TheForge_SS_GEOM: sdesc.geom = ssdesc;
				break;
			case TheForge_SS_COMP: sdesc.comp = ssdesc;
				break;
			case TheForge_SS_RAYTRACING: sdesc.comp = ssdesc;
				break;
			default: return {Handle_InvalidDynamicHandle32};
		}
	}

	Render_ShaderHandle shaderHandle = Render_ShaderHandleAlloc();
	Render_Shader* shader = Render_ShaderHandleToPtr(shaderHandle);

#if AL2O3_PLATFORM == AL2O3_PLATFORM_APPLE_MAC
	TheForge_AddShader(renderer->renderer, &sdesc, &shader->shader);
#else
	TheForge_AddShaderBinary(renderer->renderer, &sdesc, &shader->shader);
#endif

	return shaderHandle;
}

AL2O3_EXTERN_C void Render_ShaderObjectDestroy(Render_RendererHandle renderer, Render_ShaderObjectHandle handle) {
	if(!handle.handle)
	{
		return;
	}
	Render_ShaderObject* shaderObject = Render_ShaderObjectHandleToPtr(handle);
	MEMORY_FREE((void *) shaderObject->output.log);
	MEMORY_FREE((void *) shaderObject->output.shader);
	Render_ShaderObjectHandleRelease(handle);

}
AL2O3_EXTERN_C void Render_ShaderDestroy(Render_RendererHandle renderer, Render_ShaderHandle handle) {
	if (!renderer || !handle.handle) {
		return;
	}

	Render_Shader* shader = Render_ShaderHandleToPtr(handle);

	TheForge_RemoveShader(renderer->renderer, shader->shader);
	Render_ShaderHandleRelease(handle);

}

// helper for the standard vertex + fragment shader from VFile
AL2O3_EXTERN_C  Render_ShaderHandle Render_CreateShaderFromVFile(Render_RendererHandle renderer,
		VFile_Handle vertexFile,
		char const * vertexEntryPoint,
		VFile_Handle fragmentFile,
		char const * fragmentEntryPoint ) {

	Render_ShaderObjectDesc vsod = {
			Render_ST_VERTEXSHADER,
			vertexFile,
			vertexEntryPoint
	};
	Render_ShaderObjectDesc fsod = {
			Render_ST_FRAGMENTSHADER,
			fragmentFile,
			fragmentEntryPoint
	};

	Render_ShaderObjectHandle shaderObjects[2]{};
	shaderObjects[0] = Render_ShaderObjectCreate(renderer, &vsod);
	shaderObjects[1] = Render_ShaderObjectCreate(renderer, &fsod);

	if (!shaderObjects[0].handle || !shaderObjects[1].handle) {
		Render_ShaderObjectDestroy(renderer, shaderObjects[0]);
		Render_ShaderObjectDestroy(renderer, shaderObjects[1]);
		return {Handle_InvalidDynamicHandle32};
	}

	Render_ShaderHandle shader =  Render_ShaderCreate(renderer, 2, shaderObjects);

	Render_ShaderObjectDestroy(renderer, shaderObjects[0]);
	Render_ShaderObjectDestroy(renderer, shaderObjects[1]);

	return shader;
}