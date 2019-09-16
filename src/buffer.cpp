#include "al2o3_platform/platform.h"
#include "gfx_theforge/theforge.h"
#include "gfx_imgui_al2o3_theforge_bindings/bindings.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/buffer.h"

AL2O3_EXTERN_C Render_BufferHandle Render_BufferCreateVertex(Render_RendererHandle renderer,
																														 Render_BufferVertexDesc const *desc) {
	TheForge_BufferDesc const vbDesc{
			desc->vertexCount * desc->vertexSize * (desc->frequentlyUpdated ? 3 : 1),
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

	TheForge_BufferHandle buffer = nullptr;
	TheForge_AddBuffer(renderer->renderer, &vbDesc, &buffer);
	return buffer;
}

AL2O3_EXTERN_C Render_BufferHandle Render_BufferCreateIndex(Render_RendererHandle renderer,
																														Render_BufferIndexDesc const *desc) {
	TheForge_BufferDesc const ibDesc{
			desc->indexCount * desc->indexSize * (desc->frequentlyUpdated ? 3 : 1),
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

	TheForge_BufferHandle buffer = nullptr;
	TheForge_AddBuffer(renderer->renderer, &ibDesc, &buffer);
	return buffer;
}

AL2O3_EXTERN_C Render_BufferHandle Render_BufferCreateUniform(Render_RendererHandle renderer,
																															Render_BufferUniformDesc const *desc) {
	TheForge_BufferDesc const ubDesc{
			desc->size * (desc->frequentlyUpdated ? 3 : 1),
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

	TheForge_BufferHandle buffer = nullptr;
	TheForge_AddBuffer(renderer->renderer, &ubDesc, &buffer);
	return buffer;

}


AL2O3_EXTERN_C void Render_BufferDestroy(Render_RendererHandle renderer, Render_BufferHandle buffer) {
	if (!renderer) {
		return;
	}

	TheForge_RemoveBuffer(renderer->renderer, buffer);
}
