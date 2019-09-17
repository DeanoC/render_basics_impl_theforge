#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "gfx_theforge/theforge.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/buffer.h"

AL2O3_EXTERN_C Render_BufferHandle Render_BufferCreateVertex(Render_RendererHandle renderer,
																														 Render_BufferVertexDesc const *desc) {

	Render_BufferHandle buffer = (Render_BufferHandle) MEMORY_CALLOC(1, sizeof(Render_Buffer));
	if (!buffer) {
		return nullptr;
	}

	buffer->maxFrames = (desc->frequentlyUpdated ? 3 : 1);

	TheForge_BufferDesc const vbDesc{
			desc->vertexCount * desc->vertexSize * buffer->maxFrames,
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
	return buffer;
}


AL2O3_EXTERN_C Render_BufferHandle Render_BufferCreateIndex(Render_RendererHandle renderer,
																														Render_BufferIndexDesc const *desc) {
	Render_BufferHandle buffer = (Render_BufferHandle) MEMORY_CALLOC(1, sizeof(Render_Buffer));
	if (!buffer) {
		return nullptr;
	}

	buffer->maxFrames = (desc->frequentlyUpdated ? 3 : 1);

	TheForge_BufferDesc const ibDesc{
			desc->indexCount * desc->indexSize * buffer->maxFrames,
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
	return buffer;
}

AL2O3_EXTERN_C Render_BufferHandle Render_BufferCreateUniform(Render_RendererHandle renderer,
																															Render_BufferUniformDesc const *desc) {
	Render_BufferHandle buffer = (Render_BufferHandle) MEMORY_CALLOC(1, sizeof(Render_Buffer));
	if (!buffer) {
		return nullptr;
	}

	buffer->maxFrames = (desc->frequentlyUpdated ? 3 : 1);

	TheForge_BufferDesc const ubDesc{
			desc->size * buffer->maxFrames,
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
	return buffer;

}


AL2O3_EXTERN_C void Render_BufferDestroy(Render_RendererHandle renderer, Render_BufferHandle buffer) {
	if (!renderer || !buffer) {
		return;
	}

	TheForge_RemoveBuffer(renderer->renderer, buffer->buffer);
	MEMORY_FREE(buffer);
}
