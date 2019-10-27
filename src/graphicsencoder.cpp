#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "tiny_imageformat/tinyimageformat_query.h"
#include "tiny_imageformat/tinyimageformat_bits.h"

#include "render_basics/theforge/api.h"
#include "render_basics/theforge/handlemanager.h"
#include "render_basics/api.h"
#include "render_basics/graphicsencoder.h"

AL2O3_EXTERN_C Render_GraphicsEncoderHandle Render_GraphicsEncoderCreate(Render_RendererHandle renderer) {

	Render_GraphicsEncoderHandle handle = Render_GraphicsEncoderHandleAlloc();
	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(handle);
	TheForge_AddCmd(renderer->graphicsCmdPool, false, &encoder->cmd);
	return handle;

}

AL2O3_EXTERN_C void Render_GraphicsEncoderDestroy(Render_RendererHandle renderer,
																									Render_GraphicsEncoderHandle handle) {

	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(handle);
	TheForge_RemoveCmd(renderer->graphicsCmdPool, encoder->cmd);
	Render_GraphicsEncoderHandleRelease(handle);

}


AL2O3_EXTERN_C void Render_GraphicsEncoderBindRenderTargets(Render_GraphicsEncoderHandle handle,
																														uint32_t count,
																														Render_TextureHandle *targets,
																														bool clear,
																														bool setViewports,
																														bool setScissors) {
	if (count == 0) {
		return;
	}

	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(handle);

	TheForge_LoadActionsDesc loadActions{};
	TheForge_RenderTargetHandle colourTargets[16];
	TheForge_RenderTargetHandle depthTarget = nullptr;
	uint32_t colourTargetCount = 0;

	uint32_t width = 0;
	uint32_t height = 0;

	{
		for (uint32_t i = 0; i < count; ++i) {
			Render_Texture* texture = Render_TextureHandleToPtr(targets[i]);

			if (texture->renderTarget == nullptr) {
				LOGERROR("Texture without ROP_WRITE capability is being bound as a render target");
			}
			TheForge_RenderTargetHandle rth = texture->renderTarget;
			TheForge_RenderTargetDesc const *renderTargetDesc = TheForge_RenderTargetGetDesc(rth);
			if (i == 0) {
				width = renderTargetDesc->width;
				height = renderTargetDesc->height;
			}

			uint64_t formatCode = TinyImageFormat_Code(renderTargetDesc->format);
			if ((formatCode & TinyImageFormat_NAMESPACE_MASK) != TinyImageFormat_NAMESPACE_DEPTH_STENCIL) {
				colourTargets[colourTargetCount++] = rth;
				loadActions.loadActionsColor[i] = clear ? TheForge_LA_CLEAR : TheForge_LA_DONTCARE;
				loadActions.clearColorValues[i] = renderTargetDesc->clearValue;
			} else {
				ASSERT(depthTarget == nullptr);
				depthTarget = rth;
				loadActions.loadActionDepth = clear ? TheForge_LA_CLEAR : TheForge_LA_DONTCARE;
				loadActions.clearDepth = renderTargetDesc->clearValue;
			}
		}
	}

	TheForge_CmdBindRenderTargets(encoder->cmd,
																colourTargetCount,
																colourTargets,
																depthTarget,
																&loadActions,
																nullptr, nullptr,
																-1, -1);
	if (setViewports) {
		TheForge_CmdSetViewport(encoder->cmd, 0.0f, 0.0f,
														(float) width, (float) height,
														0.0f, 1.0f);
	}
	if (setScissors) {
		TheForge_CmdSetScissor(encoder->cmd, 0, 0, width, height);
	}

}

AL2O3_EXTERN_C void Render_GraphicsEncoderBindVertexBuffer(Render_GraphicsEncoderHandle handle,
																													 Render_BufferHandle vertexBufferHandle,
																													 uint64_t offset) {

	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(handle);
	Render_Buffer* buffer = Render_BufferHandleToPtr(vertexBufferHandle);

	uint64_t actualOffset = offset;
	if(buffer->frequentlyUpdated) {
		uint32_t const frameIndex = Render_RendererGetFrameIndex(buffer->renderer);
		actualOffset += (frameIndex * buffer->size);
	}

	TheForge_CmdBindVertexBuffer(encoder->cmd, 1, &buffer->buffer, &actualOffset);
}
AL2O3_EXTERN_C void Render_GraphicsEncoderBindVertexBuffers(Render_GraphicsEncoderHandle handle,
																														uint32_t vertexBufferCount,
																														Render_BufferHandle *vertexBuffers,
																														uint64_t *offsets) {
	if(vertexBufferCount == 0 || vertexBufferCount >=16) {
		return;
	}

	TheForge_BufferHandle buffers[16];
	uint64_t actualOffsets[16];

	for (uint32_t i = 0; i < vertexBufferCount; ++i) {
		Render_Buffer const* buf = (Render_Buffer const*) Render_BufferHandleToPtr(vertexBuffers[i]);
		buffers[i] = buf->buffer;
		actualOffsets[i] = 0;
		if(buf->frequentlyUpdated) {
			uint32_t const frameIndex = Render_RendererGetFrameIndex(buf->renderer);
			actualOffsets[i] += (frameIndex * buf->size);
		}
		if(offsets) {
			actualOffsets[i] += offsets[i];
		}
	}

	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(handle);
	TheForge_CmdBindVertexBuffer(encoder->cmd, vertexBufferCount, buffers, actualOffsets);

}

AL2O3_EXTERN_C void Render_GraphicsEncoderBindIndexBuffer(Render_GraphicsEncoderHandle handle,
																													Render_BufferHandle indexBufferHandle,
																													uint64_t offset) {
	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(handle);
	Render_Buffer* buffer = Render_BufferHandleToPtr(indexBufferHandle);

	uint64_t actualOffset = offset;
	if(buffer->frequentlyUpdated) {
		uint32_t const frameIndex = Render_RendererGetFrameIndex(buffer->renderer);
		actualOffset += (frameIndex * buffer->size);
	}

	TheForge_CmdBindIndexBuffer(encoder->cmd, buffer->buffer, actualOffset);

}

AL2O3_EXTERN_C void Render_GraphicsEncoderSetScissor(Render_GraphicsEncoderHandle handle, Math_Vec4U32 rect) {
	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(handle);

	TheForge_CmdSetScissor(encoder->cmd, rect.x, rect.y, rect.z, rect.w);

}

AL2O3_EXTERN_C void Render_GraphicsEncoderSetViewport(Render_GraphicsEncoderHandle handle,
																											Math_Vec4F rect,
																											Math_Vec2F depth) {
	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(handle);

	TheForge_CmdSetViewport(encoder->cmd, rect.x, rect.y, rect.z, rect.w, depth.x, depth.y);
}

AL2O3_EXTERN_C void Render_GraphicsEncoderBindPipeline(Render_GraphicsEncoderHandle handle,
																											 Render_PipelineHandle pipelineHandle) {
	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(handle);

	Render_Pipeline* pipeline = Render_PipelineHandleToPtr(pipelineHandle);

	TheForge_CmdBindPipeline(encoder->cmd, pipeline->pipeline);

}

AL2O3_EXTERN_C void Render_GraphicsEncoderBindDescriptorSet(Render_GraphicsEncoderHandle handle,
																														Render_DescriptorSetHandle setHandle,
																														uint32_t setIndex) {
	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(handle);

	Render_DescriptorSet* set = Render_DescriptorSetHandleToPtr(setHandle);

	TheForge_CmdBindDescriptorSet(encoder->cmd, set->setIndexOffset + setIndex, set->descriptorSet);

}


AL2O3_EXTERN_C void Render_GraphicsEncoderDraw(Render_GraphicsEncoderHandle handle,
																							 uint32_t vertexCount,
																							 uint32_t firstVertex) {
	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(handle);
	TheForge_CmdDraw(encoder->cmd, vertexCount, firstVertex);
}

AL2O3_EXTERN_C void Render_GraphicsEncoderDrawInstanced(Render_GraphicsEncoderHandle handle,
																												uint32_t vertexCount,
																												uint32_t firstVertex,
																												uint32_t instanceCount,
																												uint32_t firstInstance) {
	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(handle);
	TheForge_CmdDrawInstanced(encoder->cmd, vertexCount, firstVertex, instanceCount, firstInstance);
}
AL2O3_EXTERN_C void Render_GraphicsEncoderDrawIndexed(Render_GraphicsEncoderHandle handle,
																											uint32_t indexCount,
																											uint32_t firstIndex,
																											uint32_t firstVertex) {
	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(handle);
	TheForge_CmdDrawIndexed(encoder->cmd, indexCount, firstIndex, firstVertex);
}
AL2O3_EXTERN_C void Render_GraphicsEncoderDrawIndexedInstanced(Render_GraphicsEncoderHandle handle,
																															 uint32_t indexCount,
																															 uint32_t firstIndex,
																															 uint32_t instanceCount,
																															 uint32_t firstVertex,
																															 uint32_t firstInstance) {
	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(handle);
	TheForge_CmdDrawIndexedInstanced(encoder->cmd, indexCount, firstIndex, instanceCount, firstVertex, firstInstance);
}

AL2O3_EXTERN_C void Render_GraphicsEncoderTransition(Render_GraphicsEncoderHandle handle,
																										 uint32_t numBuffers,
																										 Render_BufferHandle const *buffers,
																										 Render_BufferTransitionType const *bufferTransitions,
																										 uint32_t numTextures,
																										 Render_TextureHandle const *textures,
																										 Render_TextureTransitionType const *textureTransitions) {
	if (!numBuffers && !numTextures) {
		return;
	}

	Render_GraphicsEncoder* encoder = Render_GraphicsEncoderHandleToPtr(handle);

	auto bufferBarriers = (TheForge_BufferBarrier *) STACK_ALLOC(sizeof(TheForge_BufferBarrier) * numBuffers);

	for (uint32_t i = 0; i < numBuffers; ++i) {
		bufferBarriers[i].buffer = Render_BufferHandleToPtr(buffers[i])->buffer;
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
		textureBarriers[i].texture = Render_TextureHandleToPtr(textures[i])->texture;
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
				case RENDER_TTT_SHADER_ACCESS: newState |= TheForge_RS_SHADER_RESOURCE;
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