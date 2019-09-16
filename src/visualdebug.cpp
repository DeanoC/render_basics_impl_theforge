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
	Math_Vec4F_t colour;
};

RenderTF_VisualDebug *currentTarget = nullptr;

void Line(float p0[3], float p1[3], float colour[4]) {
	if (currentTarget == nullptr) {
		LOGERROR("RenderTF_VisualDebug Line callback still hooked up after destruction!");
		return;
	}
	Vertex v0 = {
			{p0[0], p0[1], p0[2]},
			{colour[0], colour[1], colour[2], colour[3]}
	};

	Vertex v1 = {
			{p1[0], p1[1], p1[2]},
			{colour[0], colour[1], colour[2], colour[3]}
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
		Render_BufferCreateVertex(vd->target->renderer, &vbDesc);
	}

	TheForge_BufferUpdateDesc loadDesc{};
	loadDesc.buffer = vd->gpuVertexData;
	loadDesc.pData = CADT_VectorData(vd->vertexData);
	loadDesc.mSrcOffset = 0;
	loadDesc.mDstOffset = 0;
	loadDesc.mSize = CADT_VectorSize(vd->vertexData) * CADT_VectorElementSize(vd->vertexData);

	TheForge_UpdateBuffer(&loadDesc, false);

	CADT_VectorResize(vd->lineIndexData, 0);
	CADT_VectorResize(vd->vertexData, 0);
}