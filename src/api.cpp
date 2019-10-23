#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "al2o3_handle/handle.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/theforge/handlemanager.h"

AL2O3_EXTERN_C Render_HandleManagerTheForge* g_Render_HandleManagerTheForge = nullptr;
static uint32_t g_RendererCount = 0;

static void CreateHandleManager() {
	g_Render_HandleManagerTheForge = (Render_HandleManagerTheForge*) MEMORY_CALLOC(1, sizeof(Render_HandleManagerTheForge));
	ASSERT(g_Render_HandleManagerTheForge);
	Render_HandleManagerTheForge* hm = g_Render_HandleManagerTheForge;

	// objects that have low number use a fixed handle manager
	// objects where lots of objects are possible use a dynamic allocator
	// fixed handle manager might be faster to access (due to no locks)

	// high volume objects - dynamic large blocks
	hm->buffers = Handle_Manager32Create(sizeof(Render_Buffer), 1024, 256, true);
	hm->rootSignatures = Handle_Manager32Create(sizeof(Render_RootSignature), 1024, 256, true);
	hm->textures = Handle_Manager32Create(sizeof(Render_Texture), 1024, 256, true);
	hm->pipelines = Handle_Manager32Create(sizeof(Render_Pipeline), 1024, 256, true);
	hm->descriptorSets = Handle_Manager32Create(sizeof(Render_DescriptorSet), 1024, 256, true);

	// medium volume
	hm->rasteriserStates = Handle_Manager32Create(sizeof(Render_RasteriserState), 256, 8, true);
	hm->blendStates = Handle_Manager32Create(sizeof(Render_BlendState), 256, 8, true);
	hm->depthStates = Handle_Manager32Create(sizeof(Render_DepthState), 256, 8, true);
	hm->samplers = Handle_Manager32Create(sizeof(Render_Sampler), 256, 8, true);
	hm->shaderObjects = Handle_Manager32Create(sizeof(Render_ShaderObject), 128, 8, true);
	hm->shaders = Handle_Manager32Create(sizeof(Render_Shader), 64, 8, true);

	// low volume
	hm->frameBuffers = Handle_Manager32Create(sizeof(Render_FrameBuffer), 16, 4, true);
	hm->blitEncoders = Handle_Manager32Create(sizeof(Render_BlitEncoder), 16, 4, true);
	hm->computeEncoders = Handle_Manager32Create(sizeof(Render_ComputeEncoder), 16, 4, true);
	hm->graphicsEncoders = Handle_Manager32Create(sizeof(Render_GraphicsEncoder), 16, 4, true);
	hm->queues = Handle_Manager32Create(sizeof(Render_Queue), 8, 4, true);

}

static void DestroyHandleManager() {
	ASSERT(g_RendererCount == 0);

	Render_HandleManagerTheForge* hm = g_Render_HandleManagerTheForge;

	Handle_Manager32Destroy(hm->frameBuffers);
	Handle_Manager32Destroy(hm->blendStates);
	Handle_Manager32Destroy(hm->blitEncoders);
	Handle_Manager32Destroy(hm->buffers);
	Handle_Manager32Destroy(hm->computeEncoders);
	Handle_Manager32Destroy(hm->depthStates);
	Handle_Manager32Destroy(hm->descriptorSets);
	Handle_Manager32Destroy(hm->graphicsEncoders);
	Handle_Manager32Destroy(hm->queues);
	Handle_Manager32Destroy(hm->pipelines);
	Handle_Manager32Destroy(hm->rasteriserStates);
	Handle_Manager32Destroy(hm->rootSignatures);
	Handle_Manager32Destroy(hm->samplers);
	Handle_Manager32Destroy(hm->shaderObjects);
	Handle_Manager32Destroy(hm->shaders);
	Handle_Manager32Destroy(hm->textures);

	MEMORY_FREE(hm);
	g_Render_HandleManagerTheForge = nullptr;
}

AL2O3_EXTERN_C Render_RendererHandle Render_RendererCreate(InputBasic_ContextHandle input) {
	if(g_Render_HandleManagerTheForge == nullptr) {
		ASSERT(g_RendererCount == 0);
		CreateHandleManager();
	}

	auto renderer = (Render_Renderer *) MEMORY_CALLOC(1, sizeof(Render_Renderer));
	if (!renderer) {
		return nullptr;
	}

	renderer->input = input;
	renderer->maxFramesAhead = 2; // TODO fix MacOs bug if 3

	// window and renderer setup
	TheForge_RendererDesc desc{
			TheForge_ST_6_0,
			TheForge_GM_SINGLE,
			TheForge_D3D_FL_11_0,
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

	TheForge_QueueHandle gfxQ;
	queueDesc.type = TheForge_CP_DIRECT;
	TheForge_AddQueue(renderer->renderer, &queueDesc, &gfxQ);

	TheForge_QueueHandle computeQ;
	queueDesc.type = TheForge_CP_COMPUTE;
	TheForge_AddQueue(renderer->renderer, &queueDesc, &computeQ);

	TheForge_QueueHandle blitQ;
	queueDesc.type = TheForge_CP_COPY;
	TheForge_AddQueue(renderer->renderer, &queueDesc, &blitQ);

	TheForge_AddCmdPool(renderer->renderer, gfxQ, false, &renderer->graphicsCmdPool);
	TheForge_AddCmdPool(renderer->renderer, computeQ, false, &renderer->computeCmdPool);
	TheForge_AddCmdPool(renderer->renderer, blitQ, false, &renderer->blitCmdPool);

	renderer->graphicsQueue = Render_QueueHandleAlloc();
	renderer->computeQueue = Render_QueueHandleAlloc();
	renderer->blitQueue = Render_QueueHandleAlloc();

	ASSERT(sizeof(Render_Queue) == sizeof(TheForge_QueueHandle));
	Render_QueueHandleToPtr(renderer->graphicsQueue)->queue = gfxQ;
	Render_QueueHandleToPtr(renderer->computeQueue)->queue = computeQ;
	Render_QueueHandleToPtr(renderer->blitQueue)->queue = blitQ;

	// init TheForge resourceloader
	TheForge_InitResourceLoaderInterface(renderer->renderer, nullptr);

	g_RendererCount++;

	return renderer;
}

AL2O3_EXTERN_C void Render_RendererDestroy(Render_RendererHandle renderer) {
	if(!renderer) return;

	// idle everything
	TheForge_FlushResourceUpdates();
	TheForge_WaitQueueIdle(Render_QueueHandleToPtr(renderer->graphicsQueue)->queue);
	TheForge_WaitQueueIdle(Render_QueueHandleToPtr(renderer->computeQueue)->queue);
	TheForge_WaitQueueIdle(Render_QueueHandleToPtr(renderer->blitQueue)->queue);

	// remove any stocks that have been allocator
	for (auto i = 0u; i < Render_SBS_COUNT; ++i) {
		if (Render_BlendStateHandleIsValid(renderer->stockBlendState[i])) {
			Render_BlendState* bs = Render_BlendStateHandleToPtr(renderer->stockBlendState[i]);
			TheForge_RemoveBlendState(renderer->renderer, bs->state);
			Render_BlendStateHandleRelease(renderer->stockBlendState[i]);
		}
	}
	for (auto i = 0u; i < Render_SDS_COUNT; ++i) {
		if (Render_DepthStateHandleIsValid(renderer->stockDepthState[i])) {
			Render_DepthState* ds = Render_DepthStateHandleToPtr(renderer->stockDepthState[i]);
			TheForge_RemoveDepthState(renderer->renderer, ds->state);
			Render_DepthStateHandleRelease(renderer->stockDepthState[i]);
		}
	}
	for (auto i = 0u; i < Render_SRS_COUNT; ++i) {
		if (Render_RasteriserStateHandleIsValid(renderer->stockRasteriserState[i])) {
			Render_RasteriserState* rs = Render_RasteriserStateHandleToPtr(renderer->stockRasteriserState[i]);
			TheForge_RemoveRasterizerState(renderer->renderer, rs->state);
			Render_RasteriserStateHandleRelease(renderer->stockRasteriserState[i]);
		}
	}
	for (size_t i = 0; i < Render_SST_COUNT; ++i) {
		if (Render_SamplerHandleIsValid(renderer->stockSamplers[i])) {
			Render_Sampler* s = Render_SamplerHandleToPtr(renderer->stockSamplers[i]);
			TheForge_RemoveSampler(renderer->renderer, s->sampler);
			Render_SamplerHandleRelease(renderer->stockSamplers[i]);
		}
	}

	// stock vertex layouts are static and don't need releasing

	TheForge_RemoveQueue(Render_QueueHandleToPtr(renderer->graphicsQueue)->queue);
	TheForge_RemoveQueue(Render_QueueHandleToPtr(renderer->computeQueue)->queue);
	TheForge_RemoveQueue(Render_QueueHandleToPtr(renderer->blitQueue)->queue);
	Render_QueueHandleRelease(renderer->graphicsQueue);
	Render_QueueHandleRelease(renderer->computeQueue);
	Render_QueueHandleRelease(renderer->blitQueue);

	TheForge_RemoveCmdPool(renderer->renderer, renderer->blitCmdPool);
	TheForge_RemoveCmdPool(renderer->renderer, renderer->computeCmdPool);
	TheForge_RemoveCmdPool(renderer->renderer, renderer->graphicsCmdPool);

	ShaderCompiler_Destroy(renderer->shaderCompiler);

	TheForge_RemoveResourceLoaderInterface(renderer->renderer);
	TheForge_RendererDestroy(renderer->renderer);

	MEMORY_FREE(renderer);
	g_RendererCount--;

	if(g_RendererCount == 0) {
		DestroyHandleManager();
	}
}

AL2O3_EXTERN_C char const *Render_RendererGetBackendName(Render_RendererHandle) {
	return "TheForge";
}

AL2O3_EXTERN_C char const *Render_RendererGetGPUName(Render_RendererHandle) {
	return "UNKNOWN"; // TODO
}

AL2O3_EXTERN_C Render_QueueHandle Render_RendererGetPrimaryQueue(Render_RendererHandle ctx,
																																 Render_QueueType queueType) {
	if(!ctx) return { 0 };
	switch(queueType) {
		case Render_QT_GRAPHICS: return ctx->graphicsQueue;
		case Render_QT_COMPUTE: return ctx->computeQueue;
		case Render_QT_BLITTER: return ctx->blitQueue;
		default: return { 0 };
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
AL2O3_EXTERN_C void Render_QueueWaitIdle(Render_QueueHandle handle) {
	TheForge_QueueHandle q = Render_QueueHandleToPtr(handle)->queue;
	TheForge_WaitQueueIdle(q);
}

AL2O3_EXTERN_C void Render_RendererSetFrameIndex(Render_RendererHandle renderer, uint32_t newFrameIndex) {
	renderer->frameIndex = newFrameIndex;
}

AL2O3_EXTERN_C uint32_t Render_RendererGetFrameIndex(Render_RendererHandle renderer) {
	return renderer->frameIndex;
}
AL2O3_EXTERN_C void Render_RendererStartGpuCapture(Render_RendererHandle renderer, char const* filename) {
	TheForge_CaptureTraceStart(renderer->renderer, filename);
}
AL2O3_EXTERN_C void Render_RendererEndGpuCapture(Render_RendererHandle renderer) {
	TheForge_CaptureTraceEnd(renderer->renderer);
}
