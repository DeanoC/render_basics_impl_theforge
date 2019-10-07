#include <cstdint>
#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "al2o3_platform/visualdebug.h"
#include "render_basics/theforge/api.h"
#include "render_basics/buffer.h"
#include "render_basics/descriptorset.h"
#include "render_basics/framebuffer.h"
#include "render_basics/graphicsencoder.h"
#include "render_basics/pipeline.h"
#include "render_basics/rootsignature.h"
#include "visdebug.hpp"

namespace {
struct Vertex {
	Math_Vec3F pos;
	uint32_t colour;
};



RenderTF_VisualDebug *currentTarget = nullptr;

static bool CreateShaders(RenderTF_VisualDebug *vd) {
	static char const *const VertexShader = "cbuffer uniformBlockVS : register(b0, space0)\n"
																					"{\n"
																					"\tfloat4x4 worldToViewMatrix;\n"
																					"\tfloat4x4 viewToNDCMatrix;\n"
																					"\tfloat4x4 worldToNDCMatrix;\n"
																					"};\n"
																					"struct VSInput\n"
																					"{\n"
																					"\tfloat2 Position : POSITION;\n"
																					"\tfloat4 Colour   : COLOR;\n"
																					"};\n"
																					"\n"
																					"struct VSOutput {\n"
																					"\tfloat4 Position : SV_POSITION;\n"
																					"\tfloat4 Colour   : COLOR;\n"
																					"};\n"
																					"\n"
																					"VSOutput VS_main(VSInput input)\n"
																					"{\n"
																					"    VSOutput result;\n"
																					"\n"
																					"\tresult.Position = mul(worldToNDCMatrix, float4(input.Position, 0.f, 1.f));\n"
																					"\tresult.Colour = input.Colour;\n"
																					"\treturn result;\n"
																					"}";

	static char const *const FragmentShader = "struct FSInput {\n"
																						"\tfloat4 Position : SV_POSITION;\n"
																						"\tfloat4 Colour   : COLOR;\n"
																						"};\n"
																						"\n"
																						"float4 FS_main(FSInput input) : SV_Target\n"
																						"{\n"
																						"\treturn input.Colour;\n"
																						"}\n";

	static char const *const vertEntryPoint = "VS_main";
	static char const *const fragEntryPoint = "FS_main";

	VFile_Handle vfile = VFile_FromMemory(VertexShader, strlen(VertexShader) + 1, false);
	if (!vfile) {
		return false;
	}
	VFile_Handle ffile = VFile_FromMemory(FragmentShader, strlen(FragmentShader) + 1, false);
	if (!ffile) {
		VFile_Close(vfile);
		return false;
	}

	Render_ShaderObjectDesc vsod = {
			Render_ST_VERTEXSHADER,
			vfile,
			vertEntryPoint
	};
	Render_ShaderObjectDesc fsod = {
			Render_ST_FRAGMENTSHADER,
			ffile,
			fragEntryPoint
	};

	Render_ShaderObjectHandle shaderObjects[2]{};
	shaderObjects[0] = Render_ShaderObjectCreate(vd->renderer, &vsod);
	shaderObjects[1] = Render_ShaderObjectCreate(vd->renderer, &fsod);

	if (!shaderObjects[0] || !shaderObjects[1]) {
		Render_ShaderObjectDestroy(vd->renderer, shaderObjects[0]);
		Render_ShaderObjectDestroy(vd->renderer, shaderObjects[1]);
		return false;
	}

	vd->shader = Render_ShaderCreate(vd->renderer, 2, shaderObjects);

	Render_ShaderObjectDestroy(vd->renderer, shaderObjects[0]);
	Render_ShaderObjectDestroy(vd->renderer, shaderObjects[1]);

	VFile_Close(vfile);
	VFile_Close(ffile);

	return true;
}


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
	vd->renderer = target->renderer;

	vd->vertexData = CADT_VectorCreate(sizeof(Vertex));
	vd->lineIndexData = CADT_VectorCreate(sizeof(uint32_t));

	if (currentTarget != nullptr) {
		LOGWARNING("You should only have 1 framebuffer with visual debug target at any time");
	}

	CreateShaders(vd);

	Render_ShaderHandle shaders[]{vd->shader};

	Render_RootSignatureDesc rootSignatureDesc{};
	rootSignatureDesc.shaderCount = 1;
	rootSignatureDesc.shaders = shaders;
	rootSignatureDesc.staticSamplerCount = 0;
	vd->rootSignature = Render_RootSignatureCreate(vd->target->renderer, &rootSignatureDesc);
	if (!vd->rootSignature) {
		return nullptr;
	}

	Render_GraphicsPipelineDesc gfxPipeDesc{};
	TinyImageFormat colourFormats[] = {Render_FrameBufferColourFormat(vd->target)};
	gfxPipeDesc.shader = vd->shader;
	gfxPipeDesc.rootSignature = vd->rootSignature;
	gfxPipeDesc.vertexLayout = Render_GetStockVertexLayout(vd->renderer, Render_SVL_2D_COLOUR);
	gfxPipeDesc.blendState = Render_GetStockBlendState(vd->renderer, Render_SBS_OPAQUE);
	gfxPipeDesc.depthState = Render_GetStockDepthState(vd->renderer, Render_SDS_IGNORE);
	gfxPipeDesc.rasteriserState = Render_GetStockRasterisationState(vd->renderer, Render_SRS_NOCULL);
	gfxPipeDesc.colourRenderTargetCount = 1;
	gfxPipeDesc.colourFormats = colourFormats;
	gfxPipeDesc.depthStencilFormat = TinyImageFormat_UNDEFINED;
	gfxPipeDesc.sampleCount = 1;
	gfxPipeDesc.sampleQuality = 0;
	gfxPipeDesc.primitiveTopo = Render_PT_LINE_LIST;
	vd->pipeline = Render_GraphicsPipelineCreate(vd->renderer, &gfxPipeDesc);
	if (!vd->pipeline) {
		return nullptr;
	}

	Render_DescriptorSetDesc const setDesc = {
			vd->rootSignature,
			Render_DUF_PER_FRAME,
			1
	};

	vd->descriptorSet = Render_DescriptorSetCreate(vd->renderer, &setDesc);
	if (!vd->descriptorSet) {
		return nullptr;
	}

	static Render_BufferUniformDesc const ubDesc{
			sizeof(vd->uniforms),
			true
	};

	vd->uniformBuffer = Render_BufferCreateUniform(vd->renderer, &ubDesc);
	if (!vd->uniformBuffer) {
		return nullptr;
	}
	currentTarget = vd;

	vd->backup = AL2O3_VisualDebugging;
	AL2O3_VisualDebugging.Line = Line;

	return vd;
}

void RenderTF_VisualDebugDestroy(RenderTF_VisualDebug *vd) {
	if (vd->uniformBuffer) {
		Render_BufferDestroy(vd->renderer, vd->uniformBuffer);
	}
	if (vd->descriptorSet) {
		Render_DescriptorSetDestroy(vd->renderer, vd->descriptorSet);
	}
	if (vd->pipeline) {
		Render_GraphicsPipelineDestroy(vd->renderer, vd->pipeline);
	}
	if (vd->rootSignature) {
		Render_RootSignatureDestroy(vd->renderer, vd->rootSignature);
	}

	if(vd->shader) {
		Render_ShaderDestroy(vd->target->renderer, vd->shader);
	}

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

void RenderTF_VisualDebugRender(RenderTF_VisualDebug *vd, Render_GraphicsEncoderHandle encoder) {
	if (CADT_VectorSize(vd->vertexData) == 0) {
		return;
	}

	memcpy(&vd->uniforms, vd->target->debugGpuView, sizeof(Render_GpuView));

	Render_BufferUpdateDesc uniformUpdate = {
			&vd->uniforms,
			0,
			sizeof(vd->uniforms)
	};
	Render_BufferUpload(vd->uniformBuffer, &uniformUpdate);

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

	Render_BufferUpdateDesc const vertexDesc {
		CADT_VectorData(vd->vertexData),
		0,
		CADT_VectorSize(vd->vertexData) * CADT_VectorElementSize(vd->vertexData),
	};
	Render_BufferUpload(vd->gpuVertexData, &vertexDesc);

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

	Render_BufferUpdateDesc const lineIndexDesc {
			CADT_VectorData(vd->lineIndexData),
			0,
			CADT_VectorSize(vd->lineIndexData) * CADT_VectorElementSize(vd->lineIndexData),
	};
	Render_BufferUpload(vd->gpuLineIndexData, &lineIndexDesc);

	Render_DescriptorDesc params[1];
	params[0].name = "uniformBlock";
	params[0].type = Render_DT_BUFFER;
	params[0].buffer = vd->uniformBuffer;
	params[0].offset = 0;
	params[0].size = sizeof(vd->uniforms);
	Render_DescriptorUpdate(vd->descriptorSet, 0, 1, params);

	Render_GraphicsEncoderBindPipeline(encoder, vd->pipeline);
	Render_GraphicsEncoderBindDescriptorSet(encoder, vd->descriptorSet, 0);

	Render_GraphicsEncoderSetScissor(encoder, Render_FrameBufferEntireScissor(vd->target));
	Render_GraphicsEncoderSetViewport(encoder, Render_FrameBufferEntireViewport(vd->target), { 0, 1});

	Render_GraphicsEncoderBindVertexBuffer(encoder, vd->gpuVertexData, 0);

	Render_GraphicsEncoderBindIndexBuffer(encoder, vd->gpuLineIndexData, 0);
	Render_GraphicsEncoderDrawIndexed(encoder, CADT_VectorSize(vd->lineIndexData), 0, 0);

	if (CADT_VectorSize(vd->lineIndexData) > 0) {
		CADT_VectorResize(vd->lineIndexData, 0);
	}
	CADT_VectorResize(vd->vertexData, 0);

}