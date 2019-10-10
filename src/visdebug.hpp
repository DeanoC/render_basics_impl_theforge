#pragma once

#include "al2o3_platform/visualdebug.h"
#include "render_basics/theforge/api.h"
#include "al2o3_cadt/vector.h"
#include "al2o3_os/thread.h"

struct RenderTF_PlatonicSolids;

struct RenderTF_VisualDebug {
	Render_FrameBufferHandle target;

	CADT_VectorHandle vertexData;
	CADT_VectorHandle lineIndexData;
	CADT_VectorHandle solidTriIndexData;

	uint32_t gpuVertexDataCount;
	Render_BufferHandle gpuVertexData;
	uint32_t gpuLineIndexDataCount;
	Render_BufferHandle gpuLineIndexData;
	uint32_t gpuSolidTriIndexDataCount;
	Render_BufferHandle gpuSolidTriIndexData;

	RenderTF_PlatonicSolids* platonicSolids;

	Render_RendererHandle renderer;
	Render_ShaderHandle shader;
	Render_RootSignatureHandle rootSignature;

	Render_GraphicsPipelineHandle linePipeline;
	Render_GraphicsPipelineHandle solidTriPipeline;

	Render_DescriptorSetHandle descriptorSet;
	Render_BufferHandle uniformBuffer;

	Os_Mutex_t addPrimMutex;

	union {
		Render_GpuView view;
		uint8_t spacer[UNIFORM_BUFFER_MIN_SIZE];
	} uniforms;

	AL2O3_VisualDebugging_t backup;
};

RenderTF_VisualDebug *RenderTF_VisualDebugCreate(Render_FrameBufferHandle target);
void RenderTF_VisualDebugDestroy(RenderTF_VisualDebug *vd);

void RenderTF_VisualDebugRender(RenderTF_VisualDebug *vd, Render_GraphicsEncoderHandle encoder);

bool RenderTF_PlatonicSolidsCreate(RenderTF_VisualDebug* vd);
void RenderTF_PlatonicSolidsDestroy(RenderTF_VisualDebug* vd);
void RenderTF_PlatonicSolidsRender(RenderTF_VisualDebug *vd, Render_GraphicsEncoderHandle encoder);
void RenderTF_PlatonicSolidsAddTetrahedron(RenderTF_VisualDebug* vd, Math_Mat4F transform);
void RenderTF_PlatonicSolidsAddCube(RenderTF_VisualDebug* vd, Math_Mat4F transform);
void RenderTF_PlatonicSolidsAddOctahedron(RenderTF_VisualDebug* vd, Math_Mat4F transform);
void RenderTF_PlatonicSolidsAddIcosahedron(RenderTF_VisualDebug* vd, Math_Mat4F transform);
void RenderTF_PlatonicSolidsAddDodecahedron(RenderTF_VisualDebug* vd, Math_Mat4F transform);
