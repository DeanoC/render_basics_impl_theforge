#pragma once

#include "al2o3_platform/visualdebug.h"
#include "render_basics/theforge/api.h"
#include "al2o3_cadt/vector.h"

struct RenderTF_VisualDebug {
	Render_FrameBufferHandle target;

	CADT_VectorHandle vertexData;
	CADT_VectorHandle lineIndexData;

	uint32_t gpuVertexDataCount;
	Render_BufferHandle gpuVertexData;
	uint32_t gpuLineIndexDataCount;
	Render_BufferHandle gpuLineIndexData;

	Render_RendererHandle renderer;
	Render_ShaderHandle shader;
	Render_RootSignatureHandle rootSignature;
	Render_GraphicsPipelineHandle pipeline;
	Render_DescriptorSetHandle descriptorSet;
	Render_BufferHandle uniformBuffer;

	union {;
		Render_GpuView view;
		uint8_t spacer[UNIFORM_BUFFER_MIN_SIZE];
	} uniforms;

	AL2O3_VisualDebugging_t backup;
};

RenderTF_VisualDebug *RenderTF_VisualDebugCreate(Render_FrameBufferHandle target);
void RenderTF_VisualDebugDestroy(RenderTF_VisualDebug *vd);

void RenderTF_VisualDebugRender(RenderTF_VisualDebug *vd, Render_GraphicsEncoderHandle encoder);