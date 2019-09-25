#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "tiny_imageformat/tinyimageformat_query.h"
#include "tiny_imageformat/tinyimageformat_bits.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/graphicsencoder.h"

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

AL2O3_EXTERN_C void Render_GraphicsEncoderDestroy(Render_RendererHandle renderer,
																									Render_GraphicsEncoderHandle encoder) {
	if (!renderer || !encoder) {
		return;
	}

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

AL2O3_EXTERN_C void Render_GraphicsEncoderSetScissor(Render_GraphicsEncoderHandle encoder, Math_Vec4U32 rect) {
	TheForge_CmdSetScissor(encoder->cmd, rect.x, rect.y, rect.z, rect.w);
}

AL2O3_EXTERN_C void Render_GraphicsEncoderSetViewport(Render_GraphicsEncoderHandle encoder,
																											Math_Vec4F rect,
																											Math_Vec2F depth) {
	TheForge_CmdSetViewport(encoder->cmd, rect.x, rect.y, rect.z, rect.w, depth.x, depth.y);
}

AL2O3_EXTERN_C void Render_GraphicsEncoderBindPipeline(Render_GraphicsEncoderHandle encoder,
																											 Render_GraphicsPipelineHandle pipeline) {
	TheForge_CmdBindPipeline(encoder->cmd, pipeline);
}

AL2O3_EXTERN_C void Render_GraphicsEncoderBindDescriptorSet(Render_GraphicsEncoderHandle encoder,
																														Render_DescriptorSetHandle set,
																														uint32_t setIndex) {
	TheForge_CmdBindDescriptorSet(encoder->cmd, set->setIndexOffset + setIndex, set->descriptorSet);
}


AL2O3_EXTERN_C void Render_GraphicsEncoderDraw(Render_GraphicsEncoderHandle encoder,
																							 uint32_t vertexCount,
																							 uint32_t firstVertex) {
	TheForge_CmdDraw(encoder->cmd, vertexCount, firstVertex);
}

AL2O3_EXTERN_C void Render_GraphicsEncoderDrawInstanced(Render_GraphicsEncoderHandle encoder,
																												uint32_t vertexCount,
																												uint32_t firstVertex,
																												uint32_t instanceCount,
																												uint32_t firstInstance) {
	TheForge_CmdDrawInstanced(encoder->cmd, vertexCount, firstVertex, instanceCount, firstInstance);
}
AL2O3_EXTERN_C void Render_GraphicsEncoderDrawIndexed(Render_GraphicsEncoderHandle encoder,
																											uint32_t indexCount,
																											uint32_t firstIndex,
																											uint32_t firstVertex) {
	TheForge_CmdDrawIndexed(encoder->cmd, indexCount, firstIndex, firstVertex);

}
AL2O3_EXTERN_C void Render_GraphicsEncoderDrawIndexedInstanced(Render_GraphicsEncoderHandle encoder,
																															 uint32_t indexCount,
																															 uint32_t firstIndex,
																															 uint32_t instanceCount,
																															 uint32_t firstVertex,
																															 uint32_t firstInstance) {
	TheForge_CmdDrawIndexedInstanced(encoder->cmd, indexCount, firstIndex, instanceCount, firstVertex, firstInstance);
}

AL2O3_EXTERN_C void Render_GraphicsEncoderTransition(Render_GraphicsEncoderHandle encoder,
																										 uint32_t numBuffers,
																										 Render_BufferHandle const *buffers,
																										 Render_BufferTransitionType const *bufferTransitions,
																										 uint32_t numTextures,
																										 Render_TextureHandle const *textures,
																										 Render_TextureTransitionType const *textureTransitions) {
	if (!encoder && !buffers && !textures && !numBuffers && !numTextures) {
		return;
	}

	auto bufferBarriers = (TheForge_BufferBarrier *) STACK_ALLOC(sizeof(TheForge_BufferBarrier) * numBuffers);

	for (uint32_t i = 0; i < numBuffers; ++i) {
		bufferBarriers[i].buffer = buffers[i]->buffer;
		uint32_t newState = 0;
		for (uint32_t j = 0x1; j < Render_BTT_MAX; j = j << 1) {
			switch ((Render_BufferTransitionType) ((uint32_t const) bufferTransitions[i] & j)) {
				case Render_BTT_VERTEX_OR_CONSTANT_BUFFER: newState |= TheForge_RS_VERTEX_AND_CONSTANT_BUFFER;
					break;
				case Render_BTT_INDEX_BUFFER: newState |= TheForge_RS_INDEX_BUFFER;
					break;
				case Render_BTT_UNORDERED_ACCESS: newState |= TheForge_RS_UNORDERED_ACCESS;
					break;
				case Render_BTT_INDIRECT_ARGUMENT: newState |= TheForge_RS_INDIRECT_ARGUMENT;
					break;
				case Render_BTT_COPY_DEST: newState |= TheForge_RS_COPY_DEST;
					break;
				case Render_BTT_COPY_SOURCE: newState |= TheForge_RS_COPY_SOURCE;
					break;

				default:
				case Render_BTT_UNDEFINED: break;
			}
		}
		bufferBarriers[i].newState = (TheForge_ResourceState) newState;
		bufferBarriers[i].split = false;
	}

	auto textureBarriers = (TheForge_TextureBarrier *) STACK_ALLOC(sizeof(TheForge_TextureBarrier) * numTextures);
	for (uint32_t i = 0; i < numTextures; ++i) {
		textureBarriers[i].texture = textures[i];
		uint32_t newState = 0;
		for (uint32_t j = 0x1; j < Render_TTT_MAX; j = j << 1) {
			switch ((Render_TextureTransitionType) ((uint32_t const) textureTransitions[i] & j)) {
				case Render_TTT_RENDER_TARGET: newState |= TheForge_RS_RENDER_TARGET;
					break;
				case Render_TTT_UNORDERED_ACCESS: newState |= TheForge_RS_UNORDERED_ACCESS;
					break;
				case Render_TTT_DEPTH_WRITE: newState |= TheForge_RS_DEPTH_WRITE;
					break;
				case Render_TTT_DEPTH_READ: newState |= TheForge_RS_DEPTH_READ;
					break;
				case Render_TTT_COPY_DEST: newState |= TheForge_RS_COPY_DEST;
					break;
				case Render_TTT_COPY_SOURCE: newState |= TheForge_RS_COPY_SOURCE;
					break;
				case Render_TTT_PRESENT: newState |= TheForge_RS_PRESENT;
					break;

				default:
				case Render_TTT_UNDEFINED:break;
			}
		}
		textureBarriers[i].newState = (TheForge_ResourceState) newState;
		textureBarriers[i].split = false;
	}

	TheForge_CmdResourceBarrier(encoder->cmd, numBuffers, bufferBarriers, numTextures, textureBarriers);
}