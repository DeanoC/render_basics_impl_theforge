#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "al2o3_platform/visualdebug.h"
#include "gfx_theforge/resourceloader.h"
#include "render_basics/theforge/api.h"
#include "render_basics/buffer.h"
#include "visdebug.hpp"

namespace {
struct Vertex {
	Math_Vec3F_t pos;
	uint32_t colour;
};

RenderTF_VisualDebug *currentTarget = nullptr;

void Line(float p0x, float p0y, float p0z, float p1x, float p1y, float p1z, uint32_t colour) {
	if (currentTarget == nullptr) {
		LOGERROR("RenderTF_VisualDebug Line callback still hooked up after destruction!");
		return;
	}
	Vertex v0 = {
			{p0x, p0y, p0z},
			colour
	};

	Vertex v1 = {
			{p1x, p1y, p1z},
			colour
	};

	uint32_t i0 = (uint32_t) CADT_VectorPushElement(currentTarget->vertexData, &v0);
	uint32_t i1 = (uint32_t) CADT_VectorPushElement(currentTarget->vertexData, &v1);
	CADT_VectorPushElement(currentTarget->lineIndexData, &i0);
	CADT_VectorPushElement(currentTarget->lineIndexData, &i1);

}
} // end anon namespace

RenderTF_VisualDebug *RenderTF_VisualDebugCreate(Render_FrameBufferHandle target) {
	auto *vd = (RenderTF_VisualDebug *) MEMORY_CALLOC(1, sizeof(RenderTF_VisualDebug));
	if (!vd) {
		return nullptr;
	}
	vd->target = target;

	vd->vertexData = CADT_VectorCreate(sizeof(Vertex));
	vd->lineIndexData = CADT_VectorCreate(sizeof(uint32_t));

	if (currentTarget != nullptr) {
		LOGWARNING("You should only have 1 framebuffer with visual debug target at any time");
	}

	currentTarget = vd;

	vd->backup = AL2O3_VisualDebugging;
	AL2O3_VisualDebugging.Line = Line;

	return vd;
}

void RenderTF_VisualDebugDestroy(RenderTF_VisualDebug *vd) {

	if (vd->gpuLineIndexData) {
		Render_BufferDestroy(vd->target->renderer, vd->gpuLineIndexData);
		vd->gpuLineIndexData = nullptr;
	}

	if (vd->gpuVertexData) {
		Render_BufferDestroy(vd->target->renderer, vd->gpuVertexData);
		vd->gpuVertexData = nullptr;
	}

	CADT_VectorDestroy(vd->lineIndexData);
	CADT_VectorDestroy(vd->vertexData);

	AL2O3_VisualDebugging = vd->backup;
	currentTarget = nullptr;

	MEMORY_FREE(vd);
}

void RenderTF_VisualDebugRender(RenderTF_VisualDebug *vd) {
	if (CADT_VectorSize(vd->vertexData) == 0) {
		return;
	}

	if (CADT_VectorSize(vd->vertexData) > vd->gpuVertexDataCount) {
		if (vd->gpuVertexData) {
			Render_BufferDestroy(vd->target->renderer, vd->gpuVertexData);
			vd->gpuVertexData = nullptr;
		}
		Render_BufferVertexDesc vbDesc{
				(uint32_t) CADT_VectorSize(vd->vertexData),
				sizeof(Vertex),
				true
		};
		vd->gpuVertexData = Render_BufferCreateVertex(vd->target->renderer, &vbDesc);
		vd->gpuVertexDataCount = vbDesc.vertexCount;
	}

	if (CADT_VectorSize(vd->lineIndexData) > vd->gpuLineIndexDataCount) {
		if (vd->gpuLineIndexData) {
			Render_BufferDestroy(vd->target->renderer, vd->gpuLineIndexData);
			vd->gpuLineIndexData = nullptr;
		}
		Render_BufferIndexDesc ibDesc{
				(uint32_t) CADT_VectorSize(vd->lineIndexData),
				sizeof(uint32_t),
				true
		};
		vd->gpuLineIndexData = Render_BufferCreateIndex(vd->target->renderer, &ibDesc);
		vd->gpuLineIndexDataCount = ibDesc.indexCount;
	}

	TheForge_BufferUpdateDesc updateGPUVertices{};
	updateGPUVertices.buffer = vd->gpuVertexData;
	updateGPUVertices.pData = CADT_VectorData(vd->vertexData);
	updateGPUVertices.mSrcOffset = 0;
	updateGPUVertices.mDstOffset = 0;
	updateGPUVertices.mSize = CADT_VectorSize(vd->vertexData) * CADT_VectorElementSize(vd->vertexData);
	TheForge_UpdateBuffer(&updateGPUVertices, false);

	if (CADT_VectorSize(vd->lineIndexData) > 0) {

		TheForge_BufferUpdateDesc updateGPULineIndices{};
		updateGPULineIndices.buffer = vd->gpuLineIndexData;
		updateGPULineIndices.pData = CADT_VectorData(vd->lineIndexData);
		updateGPULineIndices.mSrcOffset = 0;
		updateGPULineIndices.mDstOffset = 0;
		updateGPULineIndices.mSize = CADT_VectorSize(vd->lineIndexData) * sizeof(uint32_t);

		TheForge_UpdateBuffer(&updateGPULineIndices, false);
		CADT_VectorResize(vd->lineIndexData, 0);
	}

	CADT_VectorResize(vd->vertexData, 0);
}