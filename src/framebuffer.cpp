#include <render_basics/api.h>
#include <render_basics/theforge/api.h>
#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "gfx_theforge/theforge.h"
#include "tiny_imageformat/tinyimageformat_query.h"
#include "render_basics/api.h"

typedef struct Render_FrameBuffer {
	Render_Renderer* renderer;
	TheForge_CmdPoolHandle commandPool;
	TheForge_QueueHandle presentQueue;
	uint32_t frameBufferCount;

	TheForge_SwapChainHandle swapChain;
	TheForge_RenderTargetHandle depthBuffer;
	TheForge_FenceHandle *renderCompleteFences;
	TheForge_SemaphoreHandle imageAcquiredSemaphore;
	TheForge_SemaphoreHandle *renderCompleteSemaphores;
	TheForge_CmdHandle *frameCmds;

	uint32_t frameIndex;
} Render_FrameBuffer;

AL2O3_EXTERN_C Render_FrameBufferHandle Render_FrameBufferCreate(
		Render_RendererHandle handle,
		Render_FrameBufferDesc const* desc) {
	ASSERT(desc->frameBufferWidth);
	ASSERT(desc->frameBufferHeight);
	ASSERT(desc->queue);
	ASSERT(desc->commandPool);

	auto rrenderer = (Render_Renderer*)handle;
	auto tfrenderer = (TheForge_RendererHandle) rrenderer->renderer;

	auto fb = (Render_FrameBuffer *) MEMORY_CALLOC(1, sizeof(Render_FrameBuffer));

	fb->renderer = handle;
	fb->commandPool = (TheForge_CmdPoolHandle) desc->commandPool;
	fb->presentQueue = (TheForge_QueueHandle)desc->queue;
	fb->frameBufferCount = desc->frameBufferCount;

	fb->renderCompleteFences = (TheForge_FenceHandle *) MEMORY_CALLOC(fb->frameBufferCount, sizeof(TheForge_FenceHandle));
	fb->renderCompleteSemaphores =
			(TheForge_SemaphoreHandle *) MEMORY_CALLOC(fb->frameBufferCount, sizeof(TheForge_SemaphoreHandle));

	for (uint32_t i = 0; i < fb->frameBufferCount; ++i) {
		TheForge_AddFence(tfrenderer, &fb->renderCompleteFences[i]);
		TheForge_AddSemaphore(tfrenderer, &fb->renderCompleteSemaphores[i]);
	}
	TheForge_AddSemaphore(tfrenderer, &fb->imageAcquiredSemaphore);

	TheForge_AddCmd_n((TheForge_CmdPoolHandle) desc->commandPool, false, fb->frameBufferCount, &fb->frameCmds);

	TheForge_WindowsDesc windowDesc;
	memset(&windowDesc, 0, sizeof(TheForge_WindowsDesc));
	windowDesc.handle = desc->platformHandle;

	TheForge_QueueHandle qs[] = { (TheForge_QueueHandle)desc->queue };
	TheForge_SwapChainDesc swapChainDesc;
	swapChainDesc.pWindow = &windowDesc;
	swapChainDesc.presentQueueCount = 1;
	swapChainDesc.pPresentQueues = qs;
	swapChainDesc.width = desc->frameBufferWidth;
	swapChainDesc.height = desc->frameBufferHeight;
	swapChainDesc.imageCount = desc->frameBufferCount;
	swapChainDesc.sampleCount = TheForge_SC_1;
	swapChainDesc.sampleQuality = 0;
	swapChainDesc.colorFormat = desc->colourFormat != TinyImageFormat_UNDEFINED ?
															desc->colourFormat :
															TheForge_GetRecommendedSwapchainFormat(false);
	swapChainDesc.enableVsync = false;
	TheForge_AddSwapChain(tfrenderer, &swapChainDesc, &fb->swapChain);

	if(desc->depthFormat != TinyImageFormat_UNDEFINED) {
		// Add depth buffer
		TheForge_RenderTargetDesc depthRTDesc;
		depthRTDesc.arraySize = 1;
		depthRTDesc.clearValue.depth = 1.0f;
		depthRTDesc.clearValue.stencil = 0;
		depthRTDesc.depth = 1;
		depthRTDesc.format = desc->depthFormat;
		depthRTDesc.width = desc->frameBufferWidth;
		depthRTDesc.height = desc->frameBufferHeight;
		depthRTDesc.mipLevels = 1;
		depthRTDesc.sampleCount = TheForge_SC_1;
		depthRTDesc.sampleQuality = 0;
		depthRTDesc.descriptors = TheForge_DESCRIPTOR_TYPE_UNDEFINED;
		depthRTDesc.debugName = "Backing DepthBuffer";
		depthRTDesc.flags = TheForge_TCF_NONE;
		TheForge_AddRenderTarget(tfrenderer, &depthRTDesc, &fb->depthBuffer);
	}
	return fb;
}


AL2O3_EXTERN_C void Render_FrameBufferDestroy(Render_FrameBufferHandle ctx) {
	if (!ctx)
		return;

	auto renderer = (TheForge_RendererHandle) ctx->renderer->renderer;

	if (ctx->swapChain) {
		TheForge_RemoveSwapChain(renderer, ctx->swapChain);
	}
	if (ctx->depthBuffer) {
		TheForge_RemoveRenderTarget(renderer, ctx->depthBuffer);
	}

	TheForge_RemoveCmd_n(ctx->commandPool, ctx->frameBufferCount, ctx->frameCmds);

	TheForge_RemoveSemaphore(renderer, ctx->imageAcquiredSemaphore);

	for (uint32_t i = 0; i < ctx->frameBufferCount; ++i) {
		TheForge_RemoveFence(renderer, ctx->renderCompleteFences[i]);
		TheForge_RemoveSemaphore(renderer, ctx->renderCompleteSemaphores[i]);
	}

	MEMORY_FREE(ctx->renderCompleteFences);
	MEMORY_FREE(ctx->renderCompleteSemaphores);
	MEMORY_FREE(ctx);

}

AL2O3_EXTERN_C Render_CmdHandle Render_FrameBufferNewFrame(Render_FrameBufferHandle ctx,
																						Render_RenderTargetHandle *renderTarget,
																						Render_RenderTargetHandle *depthTarget) {

	auto renderer = (TheForge_RendererHandle)ctx->renderer->renderer;
	ASSERT(renderTarget != nullptr);
	ASSERT(depthTarget != nullptr);

	TheForge_AcquireNextImage(renderer, ctx->swapChain, ctx->imageAcquiredSemaphore, nullptr, &ctx->frameIndex);

	TheForge_RenderTargetHandle rt = TheForge_SwapChainGetRenderTarget(ctx->swapChain, ctx->frameIndex);
	*renderTarget = (Render_RenderTargetHandle)rt;
	*depthTarget = (Render_RenderTargetHandle)ctx->depthBuffer;

	TheForge_SemaphoreHandle renderCompleteSemaphore = ctx->renderCompleteSemaphores[ctx->frameIndex];
	TheForge_FenceHandle renderCompleteFence = ctx->renderCompleteFences[ctx->frameIndex];

	// Stall if CPU is running "Swap Chain Buffer Count" frames ahead of GPU
	TheForge_FenceStatus fenceStatus;
	TheForge_GetFenceStatus(renderer, renderCompleteFence, &fenceStatus);
	if (fenceStatus == TheForge_FS_INCOMPLETE)
		TheForge_WaitForFences(renderer, 1, &renderCompleteFence);

	TheForge_CmdHandle cmd = ctx->frameCmds[ctx->frameIndex];
	TheForge_BeginCmd(cmd);

	// insert write barrier for render target if we are more the N frames ahead
	if(ctx->depthBuffer) {
		TheForge_TextureBarrier barriers[] = {
				{TheForge_RenderTargetGetTexture(rt), TheForge_RS_RENDER_TARGET},
				{TheForge_RenderTargetGetTexture(ctx->depthBuffer), TheForge_RS_DEPTH_WRITE},
		};
		TheForge_CmdResourceBarrier(cmd, 0, nullptr, 2, barriers, false);
	} else {
		TheForge_TextureBarrier barriers[] = {
				{TheForge_RenderTargetGetTexture(rt), TheForge_RS_RENDER_TARGET},
		};
		TheForge_CmdResourceBarrier(cmd, 0, nullptr, 1, barriers, false);
	}

	return (Render_CmdHandle)cmd;
}

AL2O3_EXTERN_C void Render_FrameBufferPresent(Render_FrameBufferHandle ctx) {
	if(!ctx) return;
	auto renderer = (TheForge_RendererHandle)ctx->renderer->renderer;

	TheForge_CmdHandle cmd = ctx->frameCmds[ctx->frameIndex];

	TheForge_RenderTargetHandle renderTarget = TheForge_SwapChainGetRenderTarget(ctx->swapChain, ctx->frameIndex);

	TheForge_CmdBindRenderTargets(cmd,
																0,
																nullptr, nullptr,
																nullptr,
																nullptr, nullptr,
																-1,
																-1);

	TheForge_TextureBarrier barriers[] = {
			{TheForge_RenderTargetGetTexture(renderTarget), TheForge_RS_PRESENT},
	};

	TheForge_CmdResourceBarrier(cmd,
															0,
															nullptr,
															1,
															barriers,
															true);
	TheForge_EndCmd(cmd);

	TheForge_QueueSubmit(ctx->presentQueue,
											 1,
											 &cmd,
											 ctx->renderCompleteFences[ctx->frameIndex],
											 1,
											 &ctx->imageAcquiredSemaphore,
											 1,
											 &ctx->renderCompleteSemaphores[ctx->frameIndex]);

	TheForge_QueuePresent(ctx->presentQueue,
												ctx->swapChain,
												ctx->frameIndex,
												1,
												&ctx->renderCompleteSemaphores[ctx->frameIndex]);

}

AL2O3_EXTERN_C TinyImageFormat Render_FrameBufferColourFormat(Render_FrameBufferHandle ctx) {
	if(!ctx) return TinyImageFormat_UNDEFINED;

	TheForge_RenderTargetHandle renderTarget = TheForge_SwapChainGetRenderTarget(ctx->swapChain, ctx->frameIndex);
	TheForge_RenderTargetDesc const* desc = TheForge_RenderTargetGetDesc(renderTarget);
	ASSERT(desc);
	return desc->format;
}

AL2O3_EXTERN_C TinyImageFormat Render_FrameBufferDepthFormat(Render_FrameBufferHandle ctx) {
	if(!ctx) return TinyImageFormat_UNDEFINED;
	if(!ctx->depthBuffer) return TinyImageFormat_UNDEFINED;

	TheForge_RenderTargetDesc const* desc = TheForge_RenderTargetGetDesc(ctx->depthBuffer);
	ASSERT(desc);
	return desc->format;
}
