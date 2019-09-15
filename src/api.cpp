#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "tiny_imageformat/tinyimageformat_query.h"
#include "tiny_imageformat/tinyimageformat_bits.h"

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

AL2O3_EXTERN_C Render_QueueHandle Render_RendererGetPrimaryQueue(Render_RendererHandle ctx, Render_GraphicsQueueType queueType) {
	if(!ctx) return nullptr;
	switch(queueType) {
		case Render_GQT_GRAPHICS: return ctx->graphicsQueue;
		case Render_GQT_COMPUTE: return ctx->computeQueue;
		case Render_GQT_BLITTER: return ctx->blitQueue;
	default: return nullptr;
	}
}

AL2O3_EXTERN_C Render_CmdPoolHandle Render_RendererGetPrimaryCommandPool(Render_RendererHandle ctx, Render_GraphicsQueueType queueType) {
	if(!ctx) return nullptr;
	switch(queueType) {
		case Render_GQT_GRAPHICS: return ctx->graphicsCmdPool;
		case Render_GQT_COMPUTE: return ctx->computeCmdPool;
		case Render_GQT_BLITTER: return ctx->blitCmdPool;
	default: return nullptr;
	}
}

AL2O3_EXTERN_C void Render_CmdBindRenderTargets(Render_CmdHandle cmd, uint32_t count, Render_RenderTargetHandle* targets, bool clear, bool setViewports, bool setScissors) {
	if(!cmd || count == 0) return;

	TheForge_LoadActionsDesc loadActions{};
	TheForge_RenderTargetHandle colourTargets[16];
	TheForge_RenderTargetHandle depthTarget = nullptr;
	uint32_t colourTargetCount = 0;

	uint32_t width = 0;
	uint32_t height = 0;

	for(uint32_t i = 0; i < count; ++i) {
		TheForge_RenderTargetHandle rth = targets[i];
		TheForge_RenderTargetDesc const *renderTargetDesc = TheForge_RenderTargetGetDesc(rth);
		if(i == 0) {
			width = renderTargetDesc->width;
			height = renderTargetDesc->height;
		}

		uint64_t formatCode = TinyImageFormat_Code(renderTargetDesc->format);
		if((formatCode & TinyImageFormat_NAMESPACE_MASK) != TinyImageFormat_NAMESPACE_DEPTH_STENCIL) {
			colourTargets[colourTargetCount++] = rth;
			loadActions.loadActionsColor[i] = TheForge_LA_CLEAR;
			loadActions.clearColorValues[i] = renderTargetDesc->clearValue;
		} else {
			ASSERT(depthTarget == nullptr);
			depthTarget = rth;
			loadActions.loadActionDepth = TheForge_LA_CLEAR;
			loadActions.clearDepth = renderTargetDesc->clearValue;
		}
	}

	TheForge_CmdBindRenderTargets(cmd,
																colourTargetCount,
																colourTargets,
																depthTarget,
																&loadActions,
																nullptr, nullptr,
																-1, -1);
	if(setViewports) {
		TheForge_CmdSetViewport(cmd, 0.0f, 0.0f,
														(float) width, (float) height,
														0.0f, 1.0f);
	}
	if(setScissors) {
		TheForge_CmdSetScissor(cmd, 0, 0, width, height);
	}

}