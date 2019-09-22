#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"


AL2O3_EXTERN_C Render_RendererHandle Render_RendererCreate(InputBasic_ContextHandle input) {
	auto renderer = (Render_Renderer*) MEMORY_CALLOC(1, sizeof(Render_Renderer));
	if(!renderer) return nullptr;

	renderer->input = input;
	// window and renderer setup
	TheForge_RendererDesc desc{
			TheForge_ST_6_0,
			TheForge_GM_SINGLE,
			TheForge_D3D_FL_12_0,
	};

	renderer->renderer = TheForge_RendererCreate("TMP", &desc);
	if (!renderer->renderer) {
		LOGERROR("TheForge_RendererCreate failed");

		return nullptr;
	}
	renderer->shaderCompiler = ShaderCompiler_Create();
	if (!renderer->shaderCompiler) {
		LOGERROR("ShaderCompiler_Create failed");
		return nullptr;
	}
#ifndef NDEBUG
	ShaderCompiler_SetOptimizationLevel(renderer->shaderCompiler, ShaderCompiler_OPT_None);
#endif

	// change from platform default to vulkan if using the vulkan backend
	if(TheForge_GetRendererApi(renderer->renderer) == TheForge_API_VULKAN) {
		ShaderCompiler_SetOutput(renderer->shaderCompiler, ShaderCompiler_OT_SPIRV, 13);
	}

	TheForge_QueueDesc queueDesc{};
	queueDesc.type = TheForge_CP_DIRECT;
	TheForge_AddQueue(renderer->renderer, &queueDesc, &renderer->graphicsQueue);
	TheForge_AddCmdPool(renderer->renderer, renderer->graphicsQueue, false, &renderer->graphicsCmdPool);
	queueDesc.type = TheForge_CP_COMPUTE;
	TheForge_AddQueue(renderer->renderer, &queueDesc, &renderer->computeQueue);
	TheForge_AddCmdPool(renderer->renderer, renderer->computeQueue, false, &renderer->computeCmdPool);
	queueDesc.type = TheForge_CP_COPY;
	TheForge_AddQueue(renderer->renderer, &queueDesc, &renderer->blitQueue);
	TheForge_AddCmdPool(renderer->renderer, renderer->blitQueue, false, &renderer->blitCmdPool);

	// init TheForge resourceloader
	TheForge_InitResourceLoaderInterface(renderer->renderer, nullptr);

	return renderer;
}

AL2O3_EXTERN_C void Render_RendererDestroy(Render_RendererHandle renderer) {
	if(!renderer) return;

	for (size_t i = 0; i < Render_SST_COUNT; ++i) {
		if (renderer->stockSamplers[i] != nullptr) {
			TheForge_RemoveSampler(renderer->renderer, renderer->stockSamplers[i]);
		}
	}

	TheForge_RemoveCmdPool(renderer->renderer, renderer->blitCmdPool);
	TheForge_RemoveQueue(renderer->blitQueue);
	TheForge_RemoveCmdPool(renderer->renderer, renderer->computeCmdPool);
	TheForge_RemoveQueue(renderer->computeQueue);
	TheForge_RemoveCmdPool(renderer->renderer, renderer->graphicsCmdPool);
	TheForge_RemoveQueue(renderer->graphicsQueue);

	ShaderCompiler_Destroy(renderer->shaderCompiler);
	TheForge_RendererDestroy(renderer->renderer);

	MEMORY_FREE(renderer);
}

AL2O3_EXTERN_C char const *Render_RendererGetBackendName(Render_RendererHandle) {
	return "TheForge";
}

AL2O3_EXTERN_C char const *Render_RendererGetGPUName(Render_RendererHandle) {
	return "UNKNOWN"; // TODO
}

AL2O3_EXTERN_C Render_QueueHandle Render_RendererGetPrimaryQueue(Render_RendererHandle ctx,
																																 Render_QueueType queueType) {
	if(!ctx) return nullptr;
	switch(queueType) {
		case Render_QT_GRAPHICS: return ctx->graphicsQueue;
		case Render_QT_COMPUTE: return ctx->computeQueue;
		case Render_QT_BLITTER: return ctx->blitQueue;
	default: return nullptr;
	}
}

AL2O3_EXTERN_C Render_CmdPoolHandle Render_RendererGetPrimaryCommandPool(Render_RendererHandle ctx,
																																				 Render_QueueType queueType) {
	if (!ctx) {
		return nullptr;
	}
	switch (queueType) {
		case Render_QT_GRAPHICS: return ctx->graphicsCmdPool;
		case Render_QT_COMPUTE: return ctx->computeCmdPool;
		case Render_QT_BLITTER: return ctx->blitCmdPool;
		default: return nullptr;
	}
}

AL2O3_EXTERN_C bool Render_RendererCanShaderReadFrom(Render_RendererHandle renderer, TinyImageFormat format) {
	return TheForge_CanShaderReadFrom(renderer->renderer, format);
}
AL2O3_EXTERN_C bool Render_RendererCanColourWriteTo(Render_RendererHandle renderer, TinyImageFormat format) {
	return TheForge_CanColorWriteTo(renderer->renderer, format);
}
AL2O3_EXTERN_C bool Render_RendererCanShaWriteTo(Render_RendererHandle renderer, TinyImageFormat format) {
	return TheForge_CanShaderWriteTo(renderer->renderer, format);
}
AL2O3_EXTERN_C void Render_QueueWaitIdle(Render_QueueHandle queue) {
	TheForge_WaitQueueIdle(queue);
}

AL2O3_EXTERN_C Render_DescriptorBinderHandle Render_DescriptorBinderCreate(Render_RendererHandle renderer,
																																					 uint32_t descCount,
																																					 Render_DescriptorBinderDesc const *descs) {
	if (!renderer || descCount == 0 || descs == NULL) {
		return nullptr;
	}
	Render_DescriptorBinderHandle db = nullptr;
	TheForge_AddDescriptorBinder(renderer->renderer, 0, descCount, (TheForge_DescriptorBinderDesc const *) descs, &db);
	return db;
}

AL2O3_EXTERN_C void Render_DescriptorBinderDestroy(Render_RendererHandle renderer,
																									 Render_DescriptorBinderHandle descBinder) {
	if (!renderer || !descBinder) {
		return;
	}

	TheForge_RemoveDescriptorBinder(renderer->renderer, descBinder);
}
