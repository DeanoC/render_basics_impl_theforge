#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "gfx_theforge/theforge.h"
#include "tiny_imageformat/tinyimageformat_query.h"
#include "gfx_imgui_al2o3_theforge_bindings/bindings.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/framebuffer.h"
#include "render_basics/graphicsencoder.h"
#include "visdebug.hpp"

AL2O3_EXTERN_C Render_FrameBufferHandle Render_FrameBufferCreate(
		Render_RendererHandle renderer,
		Render_FrameBufferDesc const *desc) {
	ASSERT(desc->frameBufferWidth);
	ASSERT(desc->frameBufferHeight);
	ASSERT(desc->queue);
	ASSERT(desc->commandPool);

	auto tfrenderer = (TheForge_RendererHandle) renderer->renderer;

	auto fb = (Render_FrameBuffer *) MEMORY_CALLOC(1, sizeof(Render_FrameBuffer));

	fb->renderer = renderer;
	fb->commandPool = (TheForge_CmdPoolHandle) desc->commandPool;
	fb->presentQueue = (TheForge_QueueHandle) desc->queue;
	fb->frameBufferCount = renderer->maxFramesAhead;

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

	TheForge_QueueHandle qs[] = {(TheForge_QueueHandle) desc->queue};
	TheForge_SwapChainDesc swapChainDesc;
	swapChainDesc.pWindow = &windowDesc;
	swapChainDesc.presentQueueCount = 1;
	swapChainDesc.pPresentQueues = qs;
	swapChainDesc.width = desc->frameBufferWidth;
	swapChainDesc.height = desc->frameBufferHeight;
	swapChainDesc.imageCount = fb->frameBufferCount;
	swapChainDesc.sampleCount = TheForge_SC_1;
	swapChainDesc.sampleQuality = 0;
	swapChainDesc.colorFormat = desc->colourFormat != TinyImageFormat_UNDEFINED ?
															desc->colourFormat : TinyImageFormat_B8G8R8A8_SRGB;
	swapChainDesc.enableVsync = false;
	swapChainDesc.colorClearValue = {1, 0, 0, 1};
	TheForge_AddSwapChain(tfrenderer, &swapChainDesc, &fb->swapChain);

	if (desc->depthFormat != TinyImageFormat_UNDEFINED) {
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

	fb->colourBufferFormat = swapChainDesc.colorFormat;
	fb->depthBufferFormat = desc->depthFormat;

	if (desc->visualDebugTarget) {
		fb->visualDebug = RenderTF_VisualDebugCreate(fb);
	}

	if (desc->embeddedImgui) {
		ImguiBindings_Shared shared = {
				Render_GetStockSampler(renderer, Render_SST_LINEAR),
				Render_GetStockBlendState(renderer, Render_SBS_PORTER_DUFF),
				Render_GetStockDepthState(renderer, Render_SDS_IGNORE),
				Render_GetStockRasterisationState(renderer, Render_SRS_NOCULL),
				Render_GetStockVertexLayout(renderer, Render_SVL_2D_COLOUR_UV)
		};
		fb->imguiBindings = ImguiBindings_Create(tfrenderer,
																						 renderer->shaderCompiler,
																						 renderer->input,
																						 &shared,
																						 20,
																						 fb->frameBufferCount,
																						 swapChainDesc.colorFormat,
																						 TheForge_SC_1,
																						 0);
		if (fb->imguiBindings) {
			ImguiBindings_SetWindowSize(fb->imguiBindings,
																	desc->frameBufferWidth,
																	desc->frameBufferHeight,
																	1.0f,
																	1.0f);
		}
	}
	return fb;
}


AL2O3_EXTERN_C void Render_FrameBufferDestroy(Render_RendererHandle handle, Render_FrameBufferHandle ctx) {
	if (!ctx) {
		return;
	}

	ASSERT(handle == ctx->renderer);

	auto renderer = handle->renderer;

	if (ctx->visualDebug) {
		RenderTF_VisualDebugDestroy(ctx->visualDebug);
	}

	if (ctx->imguiBindings) {
		ImguiBindings_Destroy(ctx->imguiBindings);
	}

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

AL2O3_EXTERN_C void Render_FrameBufferNewFrame(Render_FrameBufferHandle ctx) {

	auto renderer = (TheForge_RendererHandle) ctx->renderer->renderer;

	uint32_t frameIndex;
	TheForge_AcquireNextImage(renderer, ctx->swapChain, ctx->imageAcquiredSemaphore, nullptr, &frameIndex);
	Render_RendererSetFrameIndex(ctx->renderer, frameIndex);

	TheForge_SemaphoreHandle renderCompleteSemaphore = ctx->renderCompleteSemaphores[frameIndex];
	TheForge_FenceHandle renderCompleteFence = ctx->renderCompleteFences[frameIndex];

	// Stall if CPU is running "Swap Chain Buffer Count" frames ahead of GPU
	TheForge_FenceStatus fenceStatus;
	TheForge_GetFenceStatus(renderer, renderCompleteFence, &fenceStatus);
	if (fenceStatus == TheForge_FS_INCOMPLETE) {
		TheForge_WaitForFences(renderer, 1, &renderCompleteFence);
	}

	ctx->currentCmd = ctx->frameCmds[frameIndex];
	ctx->currentColourTarget = TheForge_SwapChainGetRenderTarget(ctx->swapChain, frameIndex);
	ctx->graphicsEncoder.cmd = ctx->currentCmd;
	ctx->graphicsEncoder.view = Render_View{};

	TheForge_BeginCmd(ctx->currentCmd);

	// insert write barrier for render target if we are more the N frames ahead
	Render_TextureHandle textures[] = {
			TheForge_RenderTargetGetTexture(ctx->currentColourTarget),
			ctx->depthBuffer ? TheForge_RenderTargetGetTexture(ctx->depthBuffer) : nullptr
	};
	Render_TextureTransitionType textureTransitions[] = {
			Render_TTT_RENDER_TARGET,
			(Render_TextureTransitionType) (Render_TTT_DEPTH_READ | Render_TTT_DEPTH_WRITE)};
	Render_GraphicsEncoderTransition(&ctx->graphicsEncoder, 0, nullptr, nullptr,
																	 ctx->depthBuffer ? 2 : 1, textures, textureTransitions);
}

AL2O3_EXTERN_C ImguiBindings_ContextHandle Render_FrameBufferCreateImguiBindings(
		Render_RendererHandle renderer,
		Render_FrameBufferHandle frameBuffer,
		InputBasic_ContextHandle input) {
	if (!renderer || !frameBuffer) {
		return nullptr;
	}

	return frameBuffer->imguiBindings;
}

AL2O3_EXTERN_C void Render_FrameBufferPresent(Render_FrameBufferHandle ctx) {
	if (!ctx) {
		return;
	}
	auto renderer = (TheForge_RendererHandle) ctx->renderer->renderer;
	auto frameIndex = ctx->renderer->frameIndex;

	TheForge_RenderTargetHandle renderTarget = TheForge_SwapChainGetRenderTarget(ctx->swapChain, frameIndex);

	if (ctx->visualDebug || ctx->imguiBindings) {
		Render_RenderTargetHandle renderTargets[2] = {renderTarget, ctx->depthBuffer};
		Render_GraphicsEncoderBindRenderTargets(&ctx->graphicsEncoder,
																						renderTargets[1] ? 2 : 1,
																						renderTargets,
																						true,
																						true,
																						true);
	}

	if (ctx->visualDebug) {
		RenderTF_VisualDebugRender(ctx->visualDebug);
	}

	if (ctx->imguiBindings) {
		ImguiBindings_Render(ctx->imguiBindings, ctx->currentCmd);
	}

	Render_GraphicsEncoderBindRenderTargets(&ctx->graphicsEncoder, 0, nullptr, false, false, false);

	Render_TextureHandle textures[] = {TheForge_RenderTargetGetTexture(renderTarget)};
	Render_TextureTransitionType textureTransitions[] = {Render_TTT_PRESENT};
	Render_GraphicsEncoderTransition(&ctx->graphicsEncoder, 0, nullptr, nullptr, 1, textures, textureTransitions);

	TheForge_EndCmd(ctx->currentCmd);

	TheForge_QueueSubmit(ctx->presentQueue,
											 1,
											 &ctx->currentCmd,
											 ctx->renderCompleteFences[frameIndex],
											 1,
											 &ctx->imageAcquiredSemaphore,
											 1,
											 &ctx->renderCompleteSemaphores[frameIndex]);

	TheForge_QueuePresent(ctx->presentQueue,
												ctx->swapChain,
												ctx->renderer->frameIndex,
												1,
												&ctx->renderCompleteSemaphores[frameIndex]);

}

AL2O3_EXTERN_C void Render_FrameBufferUpdate(Render_FrameBufferHandle frameBuffer,
																						 uint32_t width,
																						 uint32_t height,
																						 float backingScaleX, float backingScaleY,
																						 double deltaMS) {
	if (!frameBuffer || !frameBuffer->imguiBindings) {
		return;
	}

	ImguiBindings_SetWindowSize(frameBuffer->imguiBindings,
															width,
															height,
															backingScaleX,
															backingScaleY);

	ImguiBindings_UpdateInput(frameBuffer->imguiBindings, deltaMS);
}
AL2O3_EXTERN_C Render_RenderTargetHandle Render_FrameBufferColourTarget(Render_FrameBufferHandle frameBuffer) {
	return frameBuffer->currentColourTarget;
}

AL2O3_EXTERN_C Render_RenderTargetHandle Render_FrameBufferDepthTarget(Render_FrameBufferHandle frameBuffer) {
	return frameBuffer->depthBuffer;
}

AL2O3_EXTERN_C Render_GraphicsEncoderHandle Render_FrameBufferGraphicsEncoder(Render_FrameBufferHandle frameBuffer) {
	return &frameBuffer->graphicsEncoder;
}

AL2O3_EXTERN_C TinyImageFormat Render_FrameBufferColourFormat(Render_FrameBufferHandle ctx) {
	if (!ctx) {
		return TinyImageFormat_UNDEFINED;
	}

	return ctx->colourBufferFormat;
}

AL2O3_EXTERN_C TinyImageFormat Render_FrameBufferDepthFormat(Render_FrameBufferHandle ctx) {
	if (!ctx) {
		return TinyImageFormat_UNDEFINED;
	}
	return ctx->depthBufferFormat;
}

AL2O3_EXTERN_C float const *Render_FrameBufferImguiScaleOffsetMatrix(Render_FrameBufferHandle frameBuffer) {
	if (frameBuffer->imguiBindings) {
		return ImguiBindings_GetScaleOffsetMatrix(frameBuffer->imguiBindings);
	} else {
		return nullptr;
	}
}
