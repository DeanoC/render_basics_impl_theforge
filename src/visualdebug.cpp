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
	static char const *const VertexShader = "cbuffer uniformBlock : register(b0, space1)\n"
																					"{\n"
																					"\tfloat4x4 worldToViewMatrix;\n"
																					"\tfloat4x4 viewToNDCMatrix;\n"
																					"\tfloat4x4 worldToNDCMatrix;\n"
																					"};\n"
																					"struct VSInput\n"
																					"{\n"
																					"\tfloat4 Position : POSITION;\n"
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
																					"\tresult.Position = mul(worldToNDCMatrix, input.Position);\n"
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

uint32_t PickVisibleColour(uint32_t primitiveId) {
	static const uint32_t ColourTableSize = 8;
	static const uint32_t ColourTable[ColourTableSize] = {
			VISDEBUG_PACKCOLOUR(0xe6, 0x26, 0x1f, 0xFF),
			VISDEBUG_PACKCOLOUR(0xeb, 0x75, 0x32, 0xFF),
			VISDEBUG_PACKCOLOUR(0xf7, 0xd0, 0x38, 0xFF),
			VISDEBUG_PACKCOLOUR(0xa2, 0xe0, 0x48, 0xFF),
			VISDEBUG_PACKCOLOUR(0x49, 0xda, 0x9a, 0xFF),
			VISDEBUG_PACKCOLOUR(0x34, 0xbb, 0xe6, 0xFF),
			VISDEBUG_PACKCOLOUR(0x43, 0x55, 0xdb, 0xFF),
			VISDEBUG_PACKCOLOUR(0xd2, 0x3b, 0xe7, 0xFF)
	};

	primitiveId = primitiveId % ColourTableSize;
	return ColourTable[primitiveId];
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

void Lines(uint32_t lineCount, float const * verts, uint32_t colour) {
	if (currentTarget == nullptr) {
		LOGERROR("RenderTF_VisualDebug Line callback still hooked up after destruction!");
		return;
	}
	for (auto i = 0u; i < lineCount; ++i) {
		Vertex v0 = {
				{verts[0], verts[1], verts[2]},
				colour
		};
		Vertex v1 = {
				{verts[3], verts[4], verts[5]},
				colour
		};
		uint32_t i0 = (uint32_t) CADT_VectorPushElement(currentTarget->vertexData, &v0);
		uint32_t i1 = (uint32_t) CADT_VectorPushElement(currentTarget->vertexData, &v1);

		CADT_VectorPushElement(currentTarget->lineIndexData, &i0);
		CADT_VectorPushElement(currentTarget->lineIndexData, &i1);

		verts += 6;
	}
}

void LineStrip(uint32_t lineCount, float const * verts, uint32_t colour) {
	if (currentTarget == nullptr) {
		LOGERROR("RenderTF_VisualDebug Line callback still hooked up after destruction!");
		return;
	}
	Vertex lastVertex = {
			{verts[0], verts[1], verts[2]},
			colour
	};
	verts += 3;
	uint32_t lastIndex = (uint32_t) CADT_VectorPushElement(currentTarget->vertexData, &lastVertex);
	for (auto i = 0u; i < lineCount; ++i) {
		Vertex curVertex = {
				{verts[0], verts[1], verts[2]},
				colour
		};
		uint32_t curIndex = (uint32_t) CADT_VectorPushElement(currentTarget->vertexData, &curVertex);

		CADT_VectorPushElement(currentTarget->lineIndexData, &lastIndex);
		CADT_VectorPushElement(currentTarget->lineIndexData, &curIndex);

		verts += 3;
		lastIndex = curIndex;
		memcpy(&lastVertex, &curVertex, sizeof(Vertex));
	}
}

void SolidTris(uint32_t triCount, float const * verts, uint32_t colour) {
	if (currentTarget == nullptr) {
		LOGERROR("RenderTF_VisualDebug Line callback still hooked up after destruction!");
		return;
	}

	uint32_t const primId = CADT_VectorSize(currentTarget->solidTriIndexData)/3;
	for (auto i = 0u; i < triCount; ++i) {
		uint32_t col = colour;
		if(colour == 0) {
			col = PickVisibleColour(primId + i);
		}

		Vertex v0 = {
				{verts[0], verts[1], verts[2]},
				colour
		};
		Vertex v1 = {
				{verts[3], verts[4], verts[5]},
				colour
		};
		Vertex v2 = {
				{verts[6], verts[7], verts[8]},
				colour
		};

		uint32_t i0 = (uint32_t) CADT_VectorPushElement(currentTarget->vertexData, &v0);
		uint32_t i1 = (uint32_t) CADT_VectorPushElement(currentTarget->vertexData, &v1);
		uint32_t i2 = (uint32_t) CADT_VectorPushElement(currentTarget->vertexData, &v2);

		CADT_VectorPushElement(currentTarget->solidTriIndexData, &i0);
		CADT_VectorPushElement(currentTarget->solidTriIndexData, &i1);
		CADT_VectorPushElement(currentTarget->solidTriIndexData, &i2);

		verts += 9;
	}
}

void SolidQuads(uint32_t quadCount, float const * verts, uint32_t colour) {
	if (currentTarget == nullptr) {
		LOGERROR("RenderTF_VisualDebug Line callback still hooked up after destruction!");
		return;
	}
	uint32_t const primId = CADT_VectorSize(currentTarget->solidTriIndexData)/3;

	for (auto i = 0u; i < quadCount; ++i) {
		uint32_t col = colour;
		if(colour == 0) {
			col = PickVisibleColour(primId + i);
		}

		Vertex v0 = {
				{verts[0], verts[1], verts[2]},
				col
		};
		Vertex v1 = {
				{verts[3], verts[4], verts[5]},
				col
		};
		Vertex v2 = {
				{verts[6], verts[7], verts[8]},
				col
		};
		Vertex v3 = {
				{verts[9], verts[10], verts[11]},
				col
		};

		uint32_t i0 = (uint32_t) CADT_VectorPushElement(currentTarget->vertexData, &v0);
		uint32_t i1 = (uint32_t) CADT_VectorPushElement(currentTarget->vertexData, &v1);
		uint32_t i2 = (uint32_t) CADT_VectorPushElement(currentTarget->vertexData, &v2);
		uint32_t i3 = (uint32_t) CADT_VectorPushElement(currentTarget->vertexData, &v3);

		CADT_VectorPushElement(currentTarget->solidTriIndexData, &i0);
		CADT_VectorPushElement(currentTarget->solidTriIndexData, &i1);
		CADT_VectorPushElement(currentTarget->solidTriIndexData, &i2);

		CADT_VectorPushElement(currentTarget->solidTriIndexData, &i0);
		CADT_VectorPushElement(currentTarget->solidTriIndexData, &i2);
		CADT_VectorPushElement(currentTarget->solidTriIndexData, &i3);

		verts += 12;
	}
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
	vd->solidTriIndexData = CADT_VectorCreate(sizeof(uint32_t));

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

	TinyImageFormat colourFormats[] = {Render_FrameBufferColourFormat(vd->target)};

	Render_GraphicsPipelineDesc lineGfxPipeDesc{};
	lineGfxPipeDesc.shader = vd->shader;
	lineGfxPipeDesc.rootSignature = vd->rootSignature;
	lineGfxPipeDesc.vertexLayout = Render_GetStockVertexLayout(vd->renderer, Render_SVL_3D_COLOUR);
	lineGfxPipeDesc.blendState = Render_GetStockBlendState(vd->renderer, Render_SBS_PM_PORTER_DUFF);
	lineGfxPipeDesc.depthState = Render_GetStockDepthState(vd->renderer, Render_SDS_IGNORE);
	lineGfxPipeDesc.rasteriserState = Render_GetStockRasterisationState(vd->renderer, Render_SRS_NOCULL);
	lineGfxPipeDesc.colourRenderTargetCount = 1;
	lineGfxPipeDesc.colourFormats = colourFormats;
	lineGfxPipeDesc.depthStencilFormat = TinyImageFormat_UNDEFINED;
	lineGfxPipeDesc.sampleCount = 1;
	lineGfxPipeDesc.sampleQuality = 0;
	lineGfxPipeDesc.primitiveTopo = Render_PT_LINE_LIST;
	vd->linePipeline = Render_GraphicsPipelineCreate(vd->renderer, &lineGfxPipeDesc);
	if (!vd->linePipeline) {
		return nullptr;
	}

	Render_GraphicsPipelineDesc solidTriGfxPipeDesc{};
	solidTriGfxPipeDesc.shader = vd->shader;
	solidTriGfxPipeDesc.rootSignature = vd->rootSignature;
	solidTriGfxPipeDesc.vertexLayout = Render_GetStockVertexLayout(vd->renderer, Render_SVL_3D_COLOUR);
	solidTriGfxPipeDesc.blendState = Render_GetStockBlendState(vd->renderer, Render_SBS_PM_PORTER_DUFF);
	solidTriGfxPipeDesc.depthState = Render_GetStockDepthState(vd->renderer, Render_SDS_IGNORE);
	solidTriGfxPipeDesc.rasteriserState = Render_GetStockRasterisationState(vd->renderer, Render_SRS_FRONTCULL);
	solidTriGfxPipeDesc.colourRenderTargetCount = 1;
	solidTriGfxPipeDesc.colourFormats = colourFormats;
	solidTriGfxPipeDesc.depthStencilFormat = TinyImageFormat_UNDEFINED;
	solidTriGfxPipeDesc.sampleCount = 1;
	solidTriGfxPipeDesc.sampleQuality = 0;
	solidTriGfxPipeDesc.primitiveTopo = Render_PT_TRI_LIST;
	vd->solidTriPipeline = Render_GraphicsPipelineCreate(vd->renderer, &solidTriGfxPipeDesc);
	if (!vd->solidTriPipeline) {
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
	Render_DescriptorDesc params[1];
	params[0].name = "uniformBlock";
	params[0].type = Render_DT_BUFFER;
	params[0].buffer = vd->uniformBuffer;
	params[0].offset = 0;
	params[0].size = sizeof(vd->uniforms);
	Render_DescriptorPresetFrequencyUpdated(vd->descriptorSet, 0, 1, params);
	currentTarget = vd;

	vd->backup = AL2O3_VisualDebugging;
	AL2O3_VisualDebugging.Line = Line;
	AL2O3_VisualDebugging.Lines = Lines;
	AL2O3_VisualDebugging.LineStrip = LineStrip;

	AL2O3_VisualDebugging.SolidTris = SolidTris;
	AL2O3_VisualDebugging.SolidQuads = SolidQuads;

	return vd;
}

void RenderTF_VisualDebugDestroy(RenderTF_VisualDebug *vd) {
	if (vd->uniformBuffer) {
		Render_BufferDestroy(vd->renderer, vd->uniformBuffer);
	}
	if (vd->descriptorSet) {
		Render_DescriptorSetDestroy(vd->renderer, vd->descriptorSet);
	}

	if (vd->linePipeline) {
		Render_GraphicsPipelineDestroy(vd->renderer, vd->linePipeline);
	}
	if (vd->solidTriPipeline) {
		Render_GraphicsPipelineDestroy(vd->renderer, vd->solidTriPipeline);
	}

	if (vd->rootSignature) {
		Render_RootSignatureDestroy(vd->renderer, vd->rootSignature);
	}

	if(vd->shader) {
		Render_ShaderDestroy(vd->target->renderer, vd->shader);
	}

	if (vd->gpuSolidTriIndexData) {
		Render_BufferDestroy(vd->target->renderer, vd->gpuSolidTriIndexData);
		vd->gpuSolidTriIndexData = nullptr;
	}

	if (vd->gpuLineIndexData) {
		Render_BufferDestroy(vd->target->renderer, vd->gpuLineIndexData);
		vd->gpuLineIndexData = nullptr;
	}

	if (vd->gpuVertexData) {
		Render_BufferDestroy(vd->target->renderer, vd->gpuVertexData);
		vd->gpuVertexData = nullptr;
	}

	CADT_VectorDestroy(vd->solidTriIndexData);
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

	// upload the uniforms
	memcpy(&vd->uniforms, vd->target->debugGpuView, sizeof(Render_GpuView));
	Render_BufferUpdateDesc uniformUpdate = {
			&vd->uniforms,
			0,
			sizeof(vd->uniforms)
	};
	Render_BufferUpload(vd->uniformBuffer, &uniformUpdate);

	// grow the vertex buffer if required
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

	// upload the vertex data
	Render_BufferUpdateDesc const vertexDesc {
		CADT_VectorData(vd->vertexData),
		0,
		CADT_VectorSize(vd->vertexData) * CADT_VectorElementSize(vd->vertexData),
	};
	Render_BufferUpload(vd->gpuVertexData, &vertexDesc);

	// line index buffer grow if nessecary
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
	// update the line index buffer if nessecary
	if(CADT_VectorSize(vd->lineIndexData) > 0) {
		Render_BufferUpdateDesc const lineIndexDesc{
				CADT_VectorData(vd->lineIndexData),
				0,
				CADT_VectorSize(vd->lineIndexData) * CADT_VectorElementSize(vd->lineIndexData),
		};
		Render_BufferUpload(vd->gpuLineIndexData, &lineIndexDesc);
	}
	// solid tri index buffer grow if nessecary
	if (CADT_VectorSize(vd->solidTriIndexData) > vd->gpuSolidTriIndexDataCount) {
		if (vd->gpuSolidTriIndexData) {
			Render_BufferDestroy(vd->target->renderer, vd->gpuSolidTriIndexData);
			vd->gpuSolidTriIndexData = nullptr;
		}
		Render_BufferIndexDesc ibDesc{
				(uint32_t) CADT_VectorSize(vd->solidTriIndexData),
				sizeof(uint32_t),
				true
		};
		vd->gpuSolidTriIndexData = Render_BufferCreateIndex(vd->target->renderer, &ibDesc);
		vd->gpuSolidTriIndexDataCount = ibDesc.indexCount;
	}
	// update the solid tri index buffer
	if(CADT_VectorSize(vd->solidTriIndexData) > 0) {
		Render_BufferUpdateDesc const solidTriIndexDesc{
				CADT_VectorData(vd->solidTriIndexData),
				0,
				CADT_VectorSize(vd->solidTriIndexData) * CADT_VectorElementSize(vd->solidTriIndexData),
		};
		Render_BufferUpload(vd->gpuSolidTriIndexData, &solidTriIndexDesc);
	}

	Render_GraphicsEncoderBindDescriptorSet(encoder, vd->descriptorSet, 0);

	Render_GraphicsEncoderSetScissor(encoder, Render_FrameBufferEntireScissor(vd->target));
	Render_GraphicsEncoderSetViewport(encoder, Render_FrameBufferEntireViewport(vd->target), { 0, 1});

	Render_GraphicsEncoderBindVertexBuffer(encoder, vd->gpuVertexData, 0);

	if (vd->gpuSolidTriIndexDataCount) {
		Render_GraphicsEncoderBindPipeline(encoder, vd->solidTriPipeline);
		Render_GraphicsEncoderBindIndexBuffer(encoder, vd->gpuSolidTriIndexData, 0);
		Render_GraphicsEncoderDrawIndexed(encoder, CADT_VectorSize(vd->solidTriIndexData), 0, 0);
		CADT_VectorResize(vd->solidTriIndexData, 0);
	}

	if(vd->gpuLineIndexDataCount > 0) {
		Render_GraphicsEncoderBindPipeline(encoder, vd->linePipeline);
		Render_GraphicsEncoderBindIndexBuffer(encoder, vd->gpuLineIndexData, 0);
		Render_GraphicsEncoderDrawIndexed(encoder, CADT_VectorSize(vd->lineIndexData), 0, 0);
		CADT_VectorResize(vd->lineIndexData, 0);
	}

	CADT_VectorResize(vd->vertexData, 0);

}