#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "render_basics/api.h"
#include "render_basics/theforge/api.h"
#include "gfx_theforge/theforge.h"
#include "gfx_shadercompiler/compiler.h"

typedef struct Render_Renderer {
	TheForge_RendererHandle renderer;
	TheForge_QueueHandle graphicsQueue;
	TheForge_CmdPoolHandle cmdPool;

	ShaderCompiler_ContextHandle shaderCompiler;

} Render_Renderer;

AL2O3_EXTERN_C Render_RendererHandle Render_RendererCreate() {
	auto renderer = (Render_Renderer*) MEMORY_CALLOC(1, sizeof(Render_Renderer));
	if(!renderer) return nullptr;

	// window and renderer setup
	TheForge_RendererDesc desc{
			TheForge_ST_6_0,
			TheForge_GM_SINGLE,
			TheForge_D3D_FL_12_0,
	};

	renderer->renderer = TheForge_RendererCreate("TMP", &desc);
	if (!renderer->renderer) {
		LOGERROR("TheForge_RendererCreate failed");

		return false;
	}
	renderer->shaderCompiler = ShaderCompiler_Create();
	if (!renderer->shaderCompiler) {
		LOGERROR("ShaderCompiler_Create failed");
		return false;
	}
#ifndef NDEBUG
	ShaderCompiler_SetOptimizationLevel(renderer->shaderCompiler, ShaderCompiler_OPT_None);
#endif

	// change from platform default to vulkan if using the vulkan backend
	if(TheForge_GetRendererApi(renderer->renderer) == TheForge_API_VULKAN) {
		ShaderCompiler_SetOutput(renderer->shaderCompiler, ShaderCompiler_OT_SPIRV, 13);
	}

	// create basic graphics queues fences etc.
	TheForge_QueueDesc queueDesc{};
	queueDesc.type = TheForge_CP_DIRECT;
	TheForge_AddQueue(renderer->renderer, &queueDesc, &renderer->graphicsQueue);
	TheForge_AddCmdPool(renderer->renderer, renderer->graphicsQueue, false, &renderer->cmdPool);


	// init TheForge resourceloader
	TheForge_InitResourceLoaderInterface(renderer->renderer, nullptr);

	return renderer;
}

AL2O3_EXTERN_C void Render_RendererDestroy(Render_RendererHandle renderer) {
	if(!renderer) return;

	TheForge_RemoveCmdPool(renderer->renderer, renderer->cmdPool);
	TheForge_RemoveQueue(renderer->graphicsQueue);

	ShaderCompiler_Destroy(renderer->shaderCompiler);
	TheForge_RendererDestroy(renderer->renderer);

	MEMORY_FREE(renderer);
}

AL2O3_EXTERN_C char const * const Render_RendererGetBackendName(Render_RendererHandle handle) {
	return "TheForge";
}

AL2O3_EXTERN_C char const * const Render_RendererGetGPUName(Render_RendererHandle handle) {
	return "UNKNOWN"; // TODO
}
