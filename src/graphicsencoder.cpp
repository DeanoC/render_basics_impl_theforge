#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "tiny_imageformat/tinyimageformat_query.h"
#include "tiny_imageformat/tinyimageformat_bits.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"

AL2O3_EXTERN_C Render_GraphicsEncoderHandle Render_GraphicsEncoderCreate(Render_RendererHandle renderer,
																																				 Render_CmdPoolHandle cmdPoolHandle) {
	auto encoder = (Render_GraphicsEncoderHandle) MEMORY_CALLOC(1, sizeof(Render_GraphicsEncoder));
	if (!encoder) {
		return nullptr;
	}

	TheForge_AddCmd(cmdPoolHandle, false, &encoder->cmd);
	encoder->cmdPool = cmdPoolHandle;

	return encoder;
}

AL2O3_EXTERN_C void Render_GraphicsEncoderDestroy(Render_RendererHandle renderer, Render_GraphicsEncoderHandle encoder){
	if(!renderer || !encoder) return;

	// if this fires probably trying to free an explicit encoder like in Framebuffer
	ASSERT(encoder->cmdPool);

	TheForge_RemoveCmd(encoder->cmdPool, encoder->cmd);

	MEMORY_FREE(encoder);
}


AL2O3_EXTERN_C void Render_GraphicsEncoderBindRenderTargets(Render_GraphicsEncoderHandle encoder,
																														uint32_t count,
																														Render_RenderTargetHandle *targets,
																														bool clear,
																														bool setViewports,
																														bool setScissors) {
	if (!encoder || count == 0) {
		return;
	}

	TheForge_CmdHandle cmd = encoder->cmd;

	TheForge_LoadActionsDesc loadActions{};
	TheForge_RenderTargetHandle colourTargets[16];
	TheForge_RenderTargetHandle depthTarget = nullptr;
	uint32_t colourTargetCount = 0;

	uint32_t width = 0;
	uint32_t height = 0;

	for (uint32_t i = 0; i < count; ++i) {
		TheForge_RenderTargetHandle rth = targets[i];
		TheForge_RenderTargetDesc const *renderTargetDesc = TheForge_RenderTargetGetDesc(rth);
		if (i == 0) {
			width = renderTargetDesc->width;
			height = renderTargetDesc->height;
		}

		uint64_t formatCode = TinyImageFormat_Code(renderTargetDesc->format);
		if ((formatCode & TinyImageFormat_NAMESPACE_MASK) != TinyImageFormat_NAMESPACE_DEPTH_STENCIL) {
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
	if (setViewports) {
		TheForge_CmdSetViewport(cmd, 0.0f, 0.0f,
														(float) width, (float) height,
														0.0f, 1.0f);
	}
	if (setScissors) {
		TheForge_CmdSetScissor(cmd, 0, 0, width, height);
	}

}

AL2O3_EXTERN_C void Render_GraphicsEncoderBindVertexBuffer(Render_GraphicsEncoderHandle encoder,
																													 Render_BufferHandle vertexBuffer,
																													 uint64_t offset) {
	TheForge_CmdBindVertexBuffer(encoder->cmd, 1, &vertexBuffer->buffer, &offset);
}
AL2O3_EXTERN_C void Render_GraphicsEncoderBindVertexBuffers(Render_GraphicsEncoderHandle encoder,
																														uint32_t vertexBufferCount,
																														Render_BufferHandle *vertexBuffers,
																														uint64_t *offsets) {
	TheForge_BufferHandle buffers[16];
	for (uint32_t i = 0; i < vertexBufferCount; ++i) {
		buffers[i] = vertexBuffers[i]->buffer;
	}
	TheForge_CmdBindVertexBuffer(encoder->cmd, vertexBufferCount, buffers, offsets);
}

AL2O3_EXTERN_C void Render_GraphicsEncoderBindIndexBuffer(Render_GraphicsEncoderHandle encoder,
																													Render_BufferHandle indexBuffer,
																													uint64_t offset) {
	TheForge_CmdBindIndexBuffer(encoder->cmd, indexBuffer->buffer, offset);
}

AL2O3_EXTERN_C void Render_GraphicsEncoderSetScissor(Render_GraphicsEncoderHandle encoder, Math_Vec4U32_t rect) {
	TheForge_CmdSetScissor(encoder->cmd, rect.x, rect.y, rect.z, rect.w);
}

AL2O3_EXTERN_C void Render_GraphicsEncoderSetViewport(Render_GraphicsEncoderHandle encoder,
																											Math_Vec4F_t rect,
																											Math_Vec2F_t depth) {
	TheForge_CmdSetViewport(encoder->cmd, rect.x, rect.y, rect.z, rect.w, depth.x, depth.y);
}

AL2O3_EXTERN_C void Render_GraphicsEncoderBindDescriptors(Render_GraphicsEncoderHandle encoder,
																													Render_DescriptorBinderHandle descriptorBinder,
																													Render_RootSignatureHandle rootSignature,
																													uint32_t numDescriptors,
																													Render_DescriptorDesc *desc) {
	TheForge_DescriptorData* dd = (TheForge_DescriptorData*) STACK_ALLOC(sizeof(TheForge_DescriptorData) * numDescriptors);

	for (uint32_t i = 0; i < numDescriptors;++i) {
		dd[i].pName = desc[i].name;
		dd[i].count = 1;
		dd[i].pOffsets = &desc[i].offset;
		switch(desc[i].type) {

			case Render_DT_TEXTURE:
				dd[i].pTextures = &desc[i].texture;
				break;
			case Render_DT_SAMPLER:
				dd[i].pSamplers = &desc[i].sampler;
				break;
			case Render_DT_BUFFER:
				dd[i].pSizes = &desc[i].size;
				dd[i].pBuffers = &desc[i].buffer->buffer;
				break;
			case Render_DT_ROOT_CONSTANT:
				dd[i].pRootConstant = &desc[i].rootConstant;
				break;
		}
	}

	TheForge_CmdBindDescriptors(encoder->cmd, descriptorBinder, rootSignature, numDescriptors, dd);
}