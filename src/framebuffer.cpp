#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "gfx_theforge/theforge.h"
#include "tiny_imageformat/tinyimageformat_query.h"
#include "gfx_imgui_al2o3_theforge_bindings/bindings.h"

#include "render_basics/theforge/handlemanager.h"
#include "render_basics/framebuffer.h"
#include "render_basics/graphicsencoder.h"
#include "render_basics/view.h"
#include "visdebug.hpp"

AL2O3_EXTERN_C Render_FrameBufferHandle Render_FrameBufferCreate(
		Render_RendererHandle renderer,
		Render_FrameBufferDesc const *desc) {
	ASSERT(desc->frameBufferWidth);
	ASSERT(desc->frameBufferHeight);
	ASSERT(Render_QueueHandleIsValid(desc->queue));
	ASSERT(desc->platformHandle);

	auto tfrenderer = (TheForge_RendererHandle) renderer->renderer;

	Render_FrameBufferHandle fbHandle = Render_FrameBufferHandleAlloc();
	Render_FrameBuffer* fb = Render_FrameBufferHandleToPtr(fbHandle);

	fb->renderer = renderer;
	fb->commandPool = renderer->graphicsCmdPool;
	fb->presentQueue = desc->queue;
	fb->frameBufferCount = renderer->maxFramesAhead;
	fb->platformHandle = desc->platformHandle;

	fb->renderCompleteFences = (TheForge_FenceHandle *) MEMORY_CALLOC(fb->frameBufferCount, sizeof(TheForge_FenceHandle));
	fb->renderCompleteSemaphores =
			(TheForge_SemaphoreHandle *) MEMORY_CALLOC(fb->frameBufferCount, sizeof(TheForge_SemaphoreHandle));

	for (uint32_t i = 0; i < fb->frameBufferCount; ++i) {
		TheForge_AddFence(tfrenderer, &fb->renderCompleteFences[i]);
		TheForge_AddSemaphore(tfrenderer, &fb->renderCompleteSemaphores[i]);
	}
	TheForge_AddSemaphore(tfrenderer, &fb->imageAcquiredSemaphore);

	TheForge_AddCmd_n( fb->commandPool, false, fb->frameBufferCount, &fb->frameCmds);

	TheForge_QueueHandle qs[] = {Render_QueueHandleToPtr(desc->queue)->queue};
	TheForge_SwapChainDesc swapChainDesc;
	swapChainDesc.window = desc->platformHandle;
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
	swapChainDesc.colorClearValue = {0, 0, 0, 1};
	TheForge_AddSwapChain(tfrenderer, &swapChainDesc, &fb->swapChain);

	fb->colourBufferFormat = swapChainDesc.colorFormat;
	fb->entireViewport.x = 0.0f;
	fb->entireScissor.x = 0;
	fb->entireViewport.y = 0.0f;
	fb->entireScissor.y = 0;
	fb->entireScissor.z = desc->frameBufferWidth;
	fb->entireScissor.w = desc->frameBufferHeight;
	fb->entireViewport.z = (float) desc->frameBufferWidth;
	fb->entireViewport.w = (float) desc->frameBufferHeight;

	if (desc->visualDebugTarget) {
		fb->visualDebug = RenderTF_VisualDebugCreate(renderer, fbHandle);
		fb->debugGpuView = (Render_GpuView *) MEMORY_CALLOC(1, sizeof(Render_GpuView));
	}

	if (desc->embeddedImgui) {
		// insure they have been created
		auto samplerHandle = Render_GetStockSampler(renderer, Render_SST_LINEAR);
		auto blendHandle = Render_GetStockBlendState(renderer, Render_SBS_PORTER_DUFF);
		auto depthHandle = Render_GetStockDepthState(renderer, Render_SDS_IGNORE);
		auto rasterHandle = Render_GetStockRasterisationState(renderer, Render_SRS_NOCULL);
		auto vertexHandle = Render_GetStockVertexLayout(renderer, Render_SVL_2D_COLOUR_UV);

		ImguiBindings_Shared shared = {
				Render_SamplerHandleToPtr(samplerHandle)->sampler,
				Render_BlendStateHandleToPtr(blendHandle)->state,
				Render_DepthStateHandleToPtr(depthHandle)->state,
				Render_RasteriserStateHandleToPtr(rasterHandle)->state,
				(TheForge_VertexLayout const *)vertexHandle
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
																	desc->frameBufferHeight);
		}
	}

	fb->currentColourTarget = Render_TextureHandleAlloc();
	fb->graphicsEncoder = Render_GraphicsEncoderHandleAlloc();
	return fbHandle;
}


AL2O3_EXTERN_C void Render_FrameBufferDestroy(Render_RendererHandle renderer, Render_FrameBufferHandle handle) {

	Render_FrameBuffer* frameBuffer = Render_FrameBufferHandleToPtr(handle);

	TheForge_WaitForFences(renderer->renderer, frameBuffer->frameBufferCount, frameBuffer->renderCompleteFences);

	TheForge_WaitQueueIdle(Render_QueueHandleToPtr(frameBuffer->presentQueue)->queue);

	Render_GraphicsEncoderHandleRelease(frameBuffer->graphicsEncoder);
	Render_TextureHandleRelease(frameBuffer->currentColourTarget);

	if (frameBuffer->visualDebug) {
		MEMORY_FREE(frameBuffer->debugGpuView);
		RenderTF_VisualDebugDestroy(frameBuffer->visualDebug);
	}

	if (frameBuffer->imguiBindings) {
		ImguiBindings_Destroy(frameBuffer->imguiBindings);
	}

	if (frameBuffer->swapChain) {
		TheForge_RemoveSwapChain(renderer->renderer, frameBuffer->swapChain);
	}

	TheForge_RemoveCmd_n(frameBuffer->commandPool, frameBuffer->frameBufferCount, frameBuffer->frameCmds);

	TheForge_RemoveSemaphore(renderer->renderer, frameBuffer->imageAcquiredSemaphore);

	for (uint32_t i = 0; i < frameBuffer->frameBufferCount; ++i) {
		TheForge_RemoveFence(renderer->renderer, frameBuffer->renderCompleteFences[i]);
		TheForge_RemoveSemaphore(renderer->renderer, frameBuffer->renderCompleteSemaphores[i]);
	}


	MEMORY_FREE(frameBuffer->renderCompleteFences);
	MEMORY_FREE(frameBuffer->renderCompleteSemaphores);

	Render_FrameBufferHandleRelease(handle);

}
AL2O3_EXTERN_C void Render_FrameBufferResize(Render_FrameBufferHandle handle, uint32_t width, uint32_t height) {

	TheForge_FlushResourceUpdates();

	Render_FrameBuffer* frameBuffer = Render_FrameBufferHandleToPtr(handle);

	Render_QueueWaitIdle(frameBuffer->presentQueue);

	if(frameBuffer->swapChain) {
		TheForge_RemoveSwapChain(frameBuffer->renderer->renderer, frameBuffer->swapChain);
		frameBuffer->swapChain = nullptr;
	}

	TheForge_QueueHandle qs[] = { Render_QueueHandleToPtr(frameBuffer->presentQueue)->queue };
	TheForge_SwapChainDesc swapChainDesc;

	swapChainDesc.window = frameBuffer->platformHandle;
	swapChainDesc.presentQueueCount = 1;
	swapChainDesc.pPresentQueues = qs;
	swapChainDesc.width = width;
	swapChainDesc.height = height;
	swapChainDesc.imageCount = frameBuffer->frameBufferCount;
	swapChainDesc.sampleCount = TheForge_SC_1;
	swapChainDesc.sampleQuality = 0;
	swapChainDesc.colorFormat = frameBuffer->colourBufferFormat;
	swapChainDesc.enableVsync = false;
	swapChainDesc.colorClearValue = {0, 0, 0, 1};
	TheForge_AddSwapChain(frameBuffer->renderer->renderer, &swapChainDesc, &frameBuffer->swapChain);

	frameBuffer->entireScissor.z = width;
	frameBuffer->entireScissor.w = height;
	frameBuffer->entireViewport.z = (float) width;
	frameBuffer->entireViewport.w = (float) height;

	if (frameBuffer->imguiBindings) {
		ImguiBindings_SetWindowSize(frameBuffer->imguiBindings, width, height);
	}
}

AL2O3_EXTERN_C void Render_FrameBufferNewFrame(Render_FrameBufferHandle handle) {

	Render_FrameBuffer* frameBuffer = Render_FrameBufferHandleToPtr(handle);

	auto renderer = (TheForge_RendererHandle) frameBuffer->renderer->renderer;

	uint32_t frameIndex;
	TheForge_AcquireNextImage(renderer, frameBuffer->swapChain, frameBuffer->imageAcquiredSemaphore, nullptr, &frameIndex);
	Render_RendererSetFrameIndex(frameBuffer->renderer, frameIndex);

	TheForge_SemaphoreHandle renderCompleteSemaphore = frameBuffer->renderCompleteSemaphores[frameIndex];
	TheForge_FenceHandle renderCompleteFence = frameBuffer->renderCompleteFences[frameIndex];

	// Stall if CPU is running "Swap Chain Buffer Count" frames ahead of GPU
	TheForge_FenceStatus fenceStatus;
	TheForge_GetFenceStatus(renderer, renderCompleteFence, &fenceStatus);
	if (fenceStatus == TheForge_FS_INCOMPLETE) {
		TheForge_WaitForFences(renderer, 1, &renderCompleteFence);
	}

	Render_Texture *tex = Render_TextureHandleToPtr(frameBuffer->currentColourTarget);
	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(frameBuffer->graphicsEncoder);

	tex->renderTarget = TheForge_SwapChainGetRenderTarget(frameBuffer->swapChain, frameIndex);
	tex->texture = TheForge_RenderTargetGetTexture(tex->renderTarget);
	encoder->cmd = frameBuffer->frameCmds[frameIndex];
	encoder->view = Render_View{};

	TheForge_BeginCmd(encoder->cmd);

	// insert write barrier for render target if we are more the N frames ahead
	Render_TextureTransitionType const textureTransitions[] = { Render_TTT_RENDER_TARGET};
	Render_TextureHandle textures[] = { frameBuffer->currentColourTarget };

	Render_GraphicsEncoderTransition(frameBuffer->graphicsEncoder,
			0, nullptr, nullptr,
																	 1, textures, textureTransitions);

	if (frameBuffer->visualDebug || frameBuffer->imguiBindings) {
		// ensure the buffer is cleared if we used visual debug or imgui bindings
		Render_GraphicsEncoderBindRenderTargets(frameBuffer->graphicsEncoder,
																						1,
																						textures,
																						true,
																						true,
																						true);
	}
}

AL2O3_EXTERN_C void Render_FrameBufferPresent(Render_FrameBufferHandle handle) {
	Render_FrameBuffer* frameBuffer = Render_FrameBufferHandleToPtr(handle);

	auto frameIndex = frameBuffer->renderer->frameIndex;

	if (frameBuffer->visualDebug || frameBuffer->imguiBindings) {
		Render_TextureHandle renderTargets[] = { frameBuffer->currentColourTarget};
		Render_GraphicsEncoderBindRenderTargets(frameBuffer->graphicsEncoder,
																						1,
																						renderTargets,
																						false,
																						true,
																						true);
	}

	if (frameBuffer->visualDebug) {
		RenderTF_VisualDebugRender(frameBuffer->visualDebug, frameBuffer->graphicsEncoder);
	}

	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(frameBuffer->graphicsEncoder);

	if (frameBuffer->imguiBindings) {
		ImguiBindings_Render(frameBuffer->imguiBindings, encoder->cmd);
	}

	Render_GraphicsEncoderBindRenderTargets(frameBuffer->graphicsEncoder, 0, nullptr, false, false, false);

	Render_TextureHandle textures[] = {frameBuffer->currentColourTarget};
	Render_TextureTransitionType textureTransitions[] = {Render_TTT_PRESENT};
	Render_GraphicsEncoderTransition(frameBuffer->graphicsEncoder, 0, nullptr, nullptr, 1, textures, textureTransitions);

	TheForge_EndCmd(encoder->cmd);

	Render_Queue* queue = Render_QueueHandleToPtr(frameBuffer->presentQueue);
	TheForge_QueueSubmit(queue->queue,
											 1,
											 &encoder->cmd,
											 frameBuffer->renderCompleteFences[frameIndex],
											 1,
											 &frameBuffer->imageAcquiredSemaphore,
											 1,
											 &frameBuffer->renderCompleteSemaphores[frameIndex]);

	TheForge_QueuePresent(queue->queue,
												frameBuffer->swapChain,
												frameIndex,
												1,
												&frameBuffer->renderCompleteSemaphores[frameIndex]);

}

AL2O3_EXTERN_C void Render_FrameBufferUpdate(Render_FrameBufferHandle handle,
																						 uint32_t width,
																						 uint32_t height,
																						 double deltaMS) {
	Render_FrameBuffer* frameBuffer = Render_FrameBufferHandleToPtr(handle);

	if (!frameBuffer->imguiBindings) {
		return;
	}

	ImguiBindings_UpdateInput(frameBuffer->imguiBindings, deltaMS);
}
AL2O3_EXTERN_C Render_TextureHandle Render_FrameBufferColourTarget(Render_FrameBufferHandle handle) {
	Render_FrameBuffer* frameBuffer = Render_FrameBufferHandleToPtr(handle);
	return frameBuffer->currentColourTarget;
}


AL2O3_EXTERN_C Render_GraphicsEncoderHandle Render_FrameBufferGraphicsEncoder(Render_FrameBufferHandle handle) {
	Render_FrameBuffer* frameBuffer = Render_FrameBufferHandleToPtr(handle);
	return frameBuffer->graphicsEncoder;
}

AL2O3_EXTERN_C TinyImageFormat Render_FrameBufferColourFormat(Render_FrameBufferHandle handle) {
	Render_FrameBuffer* frameBuffer = Render_FrameBufferHandleToPtr(handle);
	return frameBuffer->colourBufferFormat;
}


AL2O3_EXTERN_C void Render_SetFrameBufferDebugView(Render_FrameBufferHandle handle, Render_View const *view) {
	Render_FrameBuffer* frameBuffer = Render_FrameBufferHandleToPtr(handle);

	frameBuffer->debugGpuView->worldToViewMatrix =	Math_LookAtMat4F(view->position, view->lookAt, view->upVector);

	float const f = 1.0f / tanf(view->perspectiveFOV / 2.0f);
	frameBuffer->debugGpuView->viewToNDCMatrix = {
			f / view->perspectiveAspectWoverH, 0.0f, 0.0f, 0.0f,
			0.0f, f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 0.0f, view->nearOffset, 0.0f
	};

	frameBuffer->debugGpuView->worldToNDCMatrix = Math_MultiplyMat4F(frameBuffer->debugGpuView->worldToViewMatrix, frameBuffer->debugGpuView->viewToNDCMatrix);
}

AL2O3_EXTERN_C Math_Vec4F Render_FrameBufferEntireViewport(Render_FrameBufferHandle handle) {
	Render_FrameBuffer* frameBuffer = Render_FrameBufferHandleToPtr(handle);

	ASSERT(frameBuffer->entireViewport.z > 0);
	ASSERT(frameBuffer->entireViewport.w > 0);
	return frameBuffer->entireViewport;
}
AL2O3_EXTERN_C Math_Vec4U32 Render_FrameBufferEntireScissor(Render_FrameBufferHandle handle) {
	Render_FrameBuffer* frameBuffer = Render_FrameBufferHandleToPtr(handle);

	ASSERT(frameBuffer->entireScissor.z > 0);
	ASSERT(frameBuffer->entireScissor.w > 0);
	return frameBuffer->entireScissor;
}

AL2O3_EXTERN_C float const* Render_FrameBufferImguiScaleOffsetMatrix(Render_FrameBufferHandle handle) {
	Render_FrameBuffer* frameBuffer = Render_FrameBufferHandleToPtr(handle);

	if (frameBuffer->imguiBindings) {
		return ImguiBindings_GetScaleOffsetMatrix(frameBuffer->imguiBindings);
	} else {
		return nullptr;
	}
}

AL2O3_EXTERN_C void Render_FrameBufferDescribeROPLayout(Render_FrameBufferHandle handle, Render_ROPLayout* out) {
	ASSERT(out);

	Render_FrameBuffer* frameBuffer = Render_FrameBufferHandleToPtr(handle);
	out->depthFormat = TinyImageFormat_UNDEFINED;
	out->depthType = Render_RDT_Z;

	out->colourFormatCount = 1;
	out->colourFormats[0] = frameBuffer->colourBufferFormat;
	out->colourTypes[0] = Render_RCT_RGB_LDR; // TODO HDR + dest alpha?

}