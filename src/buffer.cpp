#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "gfx_theforge/theforge.h"

#include "render_basics/theforge/api.h"
#include "render_basics/theforge/handlemanager.h"
#include "render_basics/api.h"
#include "render_basics/buffer.h"


AL2O3_EXTERN_C Render_BufferHandle Render_BufferCreateVertex(Render_RendererHandle renderer,
																														 Render_BufferVertexDesc const *desc) {
	Render_BufferHandle handle = Render_BufferHandleAlloc();
	Render_Buffer* buffer = Render_BufferHandleToPtr(handle);
	buffer->renderer = renderer;
	buffer->size = desc->vertexCount * desc->vertexSize;
	buffer->frequentlyUpdated = desc->frequentlyUpdated;
	TheForge_BufferDesc const vbDesc{
			buffer->size * (desc->frequentlyUpdated ? renderer->maxFramesAhead : 1),
			desc->frequentlyUpdated ? TheForge_RMU_CPU_TO_GPU : TheForge_RMU_GPU_ONLY,
			TheForge_BCF_NONE,
			TheForge_RS_UNDEFINED,
			TheForge_IT_UINT16,
			desc->vertexSize,
			0,
			0,
			0,
			nullptr,
			TinyImageFormat_UNDEFINED,
			TheForge_DESCRIPTOR_TYPE_VERTEX_BUFFER,
	};

	TheForge_AddBuffer(renderer->renderer, &vbDesc, &buffer->buffer);

	return handle;
}


AL2O3_EXTERN_C Render_BufferHandle Render_BufferCreateIndex(Render_RendererHandle renderer,
																														Render_BufferIndexDesc const *desc) {
	Render_BufferHandle handle = Render_BufferHandleAlloc();

	Render_Buffer* buffer = Render_BufferHandleToPtr(handle);
	buffer->renderer = renderer;
	buffer->size = desc->indexCount * desc->indexSize;
	buffer->frequentlyUpdated = desc->frequentlyUpdated;

	TheForge_BufferDesc const ibDesc{
			buffer->size * (desc->frequentlyUpdated ? renderer->maxFramesAhead : 1),
			desc->frequentlyUpdated ? TheForge_RMU_CPU_TO_GPU : TheForge_RMU_GPU_ONLY,
			TheForge_BCF_NONE,
			TheForge_RS_UNDEFINED,
			(desc->indexSize == 2) ? TheForge_IT_UINT16 : TheForge_IT_UINT32,
			0,
			0,
			0,
			0,
			nullptr,
			TinyImageFormat_UNDEFINED,
			TheForge_DESCRIPTOR_TYPE_INDEX_BUFFER,
	};

	TheForge_AddBuffer(renderer->renderer, &ibDesc, &buffer->buffer);
	return handle;
}

AL2O3_EXTERN_C Render_BufferHandle Render_BufferCreateUniform(Render_RendererHandle renderer,
																															Render_BufferUniformDesc const *desc) {
	Render_BufferHandle handle = Render_BufferHandleAlloc();

	Render_Buffer* buffer = Render_BufferHandleToPtr(handle);
	buffer->renderer = renderer;
	buffer->size = desc->size;
	buffer->frequentlyUpdated = desc->frequentlyUpdated;

	TheForge_BufferDesc const ubDesc{
			buffer->size * (desc->frequentlyUpdated ? renderer->maxFramesAhead : 1),
			desc->frequentlyUpdated ? TheForge_RMU_CPU_TO_GPU : TheForge_RMU_GPU_ONLY,
			TheForge_BCF_NO_DESCRIPTOR_VIEW_CREATION,
			TheForge_RS_UNDEFINED,
			TheForge_IT_UINT16,
			0,
			0,
			0,
			0,
			nullptr,
			TinyImageFormat_UNDEFINED,
			TheForge_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	};

	TheForge_AddBuffer(renderer->renderer, &ubDesc, &buffer->buffer);

	return handle;
}


AL2O3_EXTERN_C void Render_BufferDestroy(Render_RendererHandle renderer, Render_BufferHandle handle) {

	Render_Buffer* buffer = Render_BufferHandleToPtr(handle);
	TheForge_RemoveBuffer(renderer->renderer, buffer->buffer);
	Render_BufferHandleRelease(handle);
}

AL2O3_EXTERN_C void Render_BufferUpload(Render_BufferHandle handle, Render_BufferUpdateDesc const *update) {

	Render_Buffer* buffer = Render_BufferHandleToPtr(handle);

	uint64_t dstOffset = update->dstOffset;
	if(buffer->frequentlyUpdated) {
		uint32_t const frameIndex = Render_RendererGetFrameIndex(buffer->renderer);
		dstOffset += (frameIndex * buffer->size);
	}

	TheForge_BufferUpdateDesc const tfUpdate{
			buffer->buffer,
			update->data,
			0,
			dstOffset,
			update->size
	};

	TheForge_UpdateBuffer(&tfUpdate, false);
}
