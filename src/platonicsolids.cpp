#include "al2o3_platform/platform.h"
#include "al2o3_platform/utf8.h"
#include "al2o3_cadt/vector.h"
#include "render_basics/buffer.h"
#include "render_basics/pipeline.h"
#include "render_basics/framebuffer.h"
#include "render_basics/graphicsencoder.h"
#include "visdebug.hpp"
#include "render_basics/theforge/handlemanager.h"

struct Solid {
	uint32_t startVertex;
	uint32_t vertexCount;
	CADT_VectorHandle instanceData;
	uint32_t gpuCount;
	Render_BufferHandle gpuInstanceData;
};

enum class SolidType {
	Tetrahedron,
	Cube,
	Octahedron,
	Icosahedron,
	Dodecahedron,

	COUNT
};

struct RenderTF_PlatonicSolids {
	Render_BufferHandle gpuVertexData;
	Render_ShaderHandle shader;
	Render_PipelineHandle pipeline;

	Solid solids[(int)SolidType::COUNT];
};

namespace {
struct Vertex {
	Math_Vec3F pos;
	Math_Vec3F normal;
	uint32_t colour;
};

struct Instance {
	Math_Vec4F row0;
	Math_Vec4F row1;
	Math_Vec4F row2;
};

static Render_ShaderHandle CreateShaders(RenderTF_VisualDebug *vd) {
	static char const *const VertexShader = "cbuffer uniformBlock : register(b0, space1)\n"
																					"{\n"
																					"\tfloat4x4 worldToViewMatrix;\n"
																					"\tfloat4x4 viewToNDCMatrix;\n"
																					"\tfloat4x4 worldToNDCMatrix;\n"
																					"};\n"
																					"struct VSInput\n"
																					"{\n"
																					"\tfloat4 Position : POSITION;\n"
																					"\tfloat3 Normal   : NORMAL;\n"
																					"\tfloat4 Colour   : COLOR;\n"
																					"};\n"
																					"\n"
																					"struct InstanceInput\n"
																					"{\n"
																					"\tfloat4 localToWorldMatrixRow0 : INSTANCEROW0;\n"
																					"\tfloat4 localToWorldMatrixRow1 : INSTANCEROW1;\n"
																					"\tfloat4 localToWorldMatrixRow2 : INSTANCEROW2;\n"
																					"};\n"
																					"\n"
																					"struct VSOutput {\n"
																					"\tfloat4 Position : SV_POSITION;\n"
																					"\tfloat4 Colour   : COLOR;\n"
																					"};\n"
																					"\n"
																					"VSOutput VS_main(VSInput input, InstanceInput instance)\n"
																					"{\n"
																					"    VSOutput result;\n"
																					"\n"
																				 "\tfloat4x4 localToWorldMatrix;\n"
																					"\tlocalToWorldMatrix[0] = instance.localToWorldMatrixRow0;\n"
																					"\tlocalToWorldMatrix[1] = instance.localToWorldMatrixRow1;\n"
																					"\tlocalToWorldMatrix[2] = instance.localToWorldMatrixRow2;\n"
																					"\tlocalToWorldMatrix[3] = float4(0,0,0,1);\n"
																					"\tfloat4 pos = mul(localToWorldMatrix, input.Position);\n"
																					"\tresult.Position = mul(worldToNDCMatrix, pos);\n"
																					"\tresult.Colour = float4((input.Normal*0.5)+float3(0.5,0.5,0.5),1);\n//input.Colour;\n"
																					"\treturn result;\n"
																					"}\n";

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

	VFile_Handle vfile = VFile_FromMemory(VertexShader, utf8size(VertexShader) + 1, false);
	if (!vfile) {
		return {0};
	}
	VFile_Handle ffile = VFile_FromMemory(FragmentShader, utf8size(FragmentShader) + 1, false);
	if (!ffile) {
		VFile_Close(vfile);
		return {0};
	}
	auto shader = Render_CreateShaderFromVFile(vd->renderer, vfile, "VS_main", ffile, "FS_main");

	VFile_Close(vfile);
	VFile_Close(ffile);

	return shader;
}

uint32_t CreateTetrahedon(CADT_VectorHandle outPos) {
	ASSERT(CADT_VectorElementSize(outPos) == sizeof(Vertex));

	static const uint32_t NumVertices = 4;
	static const uint32_t NumFaces = 4;
	float const pos[NumVertices * 3] = {
			 1,  1,  1,
			 1, -1,  1,
			-1,  1, -1,
			-1, -1,  1,
	};
	uint32_t const faces[NumFaces * 3] = {
			1, 2, 0,
			2, 3, 0,
			3, 1, 0,
			2, 1, 3,
	};
	for (uint32_t i = 0u; i < NumFaces; ++i) {
		Math_Vec3F v0 = Math_FromVec3F(pos + (faces[(i*3) + 0] * 3));
		Math_Vec3F v1 = Math_FromVec3F(pos + (faces[(i*3) + 1] * 3));
		Math_Vec3F v2 = Math_FromVec3F(pos + (faces[(i*3) + 2] * 3));
		Math_Vec3F e0 = Math_NormaliseVec3F(Math_SubVec3F(v1, v0));
		Math_Vec3F e1 = Math_NormaliseVec3F(Math_SubVec3F(v2, v0));
		Math_Vec3F n = Math_CrossVec3F(e0, e1);

		Vertex vertex;

		vertex.colour = 0xFFFFFFFF;
		memcpy(&vertex.normal, &n, sizeof(float) * 3);

		memcpy(&vertex.pos, &v0, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v1, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v2, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);

	}

	return NumFaces * 3;
}

uint32_t CreateCube(CADT_VectorHandle outPos) {
	ASSERT(CADT_VectorElementSize(outPos) == sizeof(Vertex));

	static const uint32_t NumVertices = 8;
	static const uint32_t NumFaces = 6;
	static const float pos[NumVertices * 3] = {
			-1,  1, -1,
			-1, -1, -1,
			 1, -1, -1,
			 1,  1, -1,

			-1,  1,  1,
			-1, -1,  1,
			 1, -1,  1,
			 1,  1,  1,
	};

	uint32_t const faces[NumFaces * 4] = {
			0, 1, 2, 3,
			7, 6, 5, 4,
			4, 0, 3, 7,
			5, 6, 2, 1,
			5, 1, 0, 4,
			2, 6, 7, 3
	};

	for (uint32_t i = 0u; i < NumFaces; ++i) {
		Math_Vec3F v0 = Math_FromVec3F(pos + (faces[(i*4) + 0] * 3));
		Math_Vec3F v1 = Math_FromVec3F(pos + (faces[(i*4) + 1] * 3));
		Math_Vec3F v2 = Math_FromVec3F(pos + (faces[(i*4) + 2] * 3));
		Math_Vec3F v3 = Math_FromVec3F(pos + (faces[(i*4) + 3] * 3));

		Math_Vec3F e0 = Math_NormaliseVec3F(Math_SubVec3F(v1, v0));
		Math_Vec3F e1 = Math_NormaliseVec3F(Math_SubVec3F(v2, v0));
		Math_Vec3F n = Math_CrossVec3F(e0, e1);

		Vertex vertex;

		vertex.colour = 0xFFFFFFFF;
		memcpy(&vertex.normal, &n, sizeof(float) * 3);

		memcpy(&vertex.pos, &v0, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v1, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v2, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);

		memcpy(&vertex.pos, &v2, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v3, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v0, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);

	}

	return NumFaces * 6;
}

uint32_t CreateOctahedron(CADT_VectorHandle outPos) {

	ASSERT(CADT_VectorElementSize(outPos) == sizeof(Vertex));

	static const uint32_t NumVertices = 6;
	static const uint32_t NumFaces = 8;
	float const pos[NumVertices * 3] = {
			-1,  0,  0,
			 1,  0,  0,
			 0, -1,  0,
			 0,  1,  0,
			 0,  0, -1,
			 0,  0,  1,
	};
	uint32_t const faces[NumFaces * 3] = {
			0, 3, 5,
			0, 5, 2,
			4, 3, 0,
			4, 0, 2,
			5, 3, 1,
			5, 1, 2,
			4, 1, 3,
			4, 2, 1,
	};
	for (uint32_t i = 0u; i < NumFaces; ++i) {
		Math_Vec3F v0 = Math_FromVec3F(pos + (faces[(i*3) + 0] * 3));
		Math_Vec3F v1 = Math_FromVec3F(pos + (faces[(i*3) + 1] * 3));
		Math_Vec3F v2 = Math_FromVec3F(pos + (faces[(i*3) + 2] * 3));
		Math_Vec3F e0 = Math_NormaliseVec3F(Math_SubVec3F(v1, v0));
		Math_Vec3F e1 = Math_NormaliseVec3F(Math_SubVec3F(v2, v0));
		Math_Vec3F n = Math_CrossVec3F(e0, e1);

		Vertex vertex;

		vertex.colour = 0xFFFFFFFF;
		memcpy(&vertex.normal, &n, sizeof(float) * 3);

		memcpy(&vertex.pos, &v0, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v1, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v2, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);

	}

	return NumFaces * 3;
}

uint32_t CreateIcosahedron(CADT_VectorHandle outPos) {

	ASSERT(CADT_VectorElementSize(outPos) == sizeof(Vertex));

	// Phi - the square root of 5 plus 1 divided by 2
	float const phi = 1.6810f;//(1.0 + sqrt(5.0)) * 0.5;
	float const p = 2.0f / (2.0f * phi);
	
	static const uint32_t NumVertices = 12;
	static const uint32_t NumFaces = 20;
	float const pos[NumVertices * 3] = {
			-p,  1,  0,
			 p,  1,  0,
			 0,  p, -1,
			 0,  p,  1,
			-1,  0,  p,
			 1,  0,  p,

			 1,  0, -p,
		  -1,  0, -p,
			 0, -p,  1,
			 0, -p, -1,
			 p, -1,  0,
			-p, -1,  0
	};

	uint32_t const faces[NumFaces * 3] = {
			2, 1, 0,
			2, 0, 7,
			2, 9, 6,
			2, 6, 1,
			2, 7, 9,
			1, 5, 3,
			1, 6, 5,
			1, 3, 0,
			0, 3, 4,
			0, 4, 7,
			3, 8, 4,
			3, 5, 8,
			8, 5, 10,
			8, 10,11,
			4, 8, 11,
			4, 11,7,
			5, 6, 10,
			9, 7, 11,
			9, 10,6,
			9, 11,10,
	};

	for (uint32_t i = 0u; i < NumFaces; ++i) {
		Math_Vec3F v0 = Math_FromVec3F(pos + (faces[(i*3) + 0] * 3));
		Math_Vec3F v1 = Math_FromVec3F(pos + (faces[(i*3) + 1] * 3));
		Math_Vec3F v2 = Math_FromVec3F(pos + (faces[(i*3) + 2] * 3));
		Math_Vec3F e0 = Math_NormaliseVec3F(Math_SubVec3F(v1, v0));
		Math_Vec3F e1 = Math_NormaliseVec3F(Math_SubVec3F(v2, v0));
		Math_Vec3F n = Math_CrossVec3F(e0, e1);

		Vertex vertex;

		vertex.colour = 0xFFFFFFFF;
		memcpy(&vertex.normal, &n, sizeof(float) * 3);

		memcpy(&vertex.pos, &v0, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v1, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v2, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);

	}

	return NumFaces * 3;
}


uint32_t CreateDodecahedron(CADT_VectorHandle outPos) {
	ASSERT(CADT_VectorElementSize(outPos) == sizeof(Vertex));

	static const float a = sqrtf(2.0f / (3.0f + sqrtf(5.0f)));
	static const float b = 1.0f + sqrtf(6.0f / (3.0f + sqrtf(5.0f)) -
			2.0f + 2.0f * sqrtf(2.0f / (3.0f + sqrtf(5.0f))));

	static const uint32_t NumVertices = 20;
	static const uint32_t NumFaces = 12;
	static const float pos[NumVertices * 3] = {
			-a,  0,  b,
			 a,  0,  b,
			-1, -1, -1,
			-1, -1,  1,
			-1,  1, -1,
			-1,  1,  1,
			 1, -1, -1,
			 1, -1,  1,
			 1,  1, -1,
			 1,  1,  1,
			 b,  a,  0,
			 b, -a,  0,
			-b,  a,  0,
			-b, -a,  0,
			-a,  0, -b,
			 a,  0, -b,
			 0,  b,  a,
			 0,  b, -a,
			 0, -b,  a,
			 0, -b, -a,
	};

	static uint32_t const faces[NumFaces * 5] = {
			0, 1, 9, 16, 5,
			1, 0, 3, 18, 7,
			1, 7, 11, 10, 9,
			11, 7, 18, 19, 6,
			8, 17, 16, 9, 10,
			2, 14, 15, 6, 19,
			2, 13, 12, 4, 14,
			2, 19, 18, 3, 13,
			3, 0, 5, 12, 13,
			6, 15, 8, 10, 11,
			4, 17, 8, 15, 14,
			4, 12, 5, 16, 17,
	};

	for (uint32_t i = 0u; i < NumFaces; ++i) {
		Math_Vec3F v0 = Math_FromVec3F(pos + (faces[(i*5) + 0] * 3));
		Math_Vec3F v1 = Math_FromVec3F(pos + (faces[(i*5) + 1] * 3));
		Math_Vec3F v2 = Math_FromVec3F(pos + (faces[(i*5) + 2] * 3));
		Math_Vec3F v3 = Math_FromVec3F(pos + (faces[(i*5) + 3] * 3));
		Math_Vec3F v4 = Math_FromVec3F(pos + (faces[(i*5) + 4] * 3));

		Math_Vec3F e0 = Math_NormaliseVec3F(Math_SubVec3F(v1, v0));
		Math_Vec3F e1 = Math_NormaliseVec3F(Math_SubVec3F(v2, v0));
		Math_Vec3F n = Math_CrossVec3F(e0, e1);

		Vertex vertex;

		vertex.colour = 0xFFFFFFFF;
		memcpy(&vertex.normal, &n, sizeof(float) * 3);

		memcpy(&vertex.pos, &v0, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v1, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v2, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);

		memcpy(&vertex.pos, &v0, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v2, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v3, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);

		memcpy(&vertex.pos, &v0, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v3, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
		memcpy(&vertex.pos, &v4, sizeof(float) * 3);
		CADT_VectorPushElement(outPos, &vertex);
	}

	return NumFaces * 9;
}

} // end anon namespac
bool RenderTF_PlatonicSolidsCreate(RenderTF_VisualDebug *vd) {

	vd->platonicSolids = (RenderTF_PlatonicSolids *) MEMORY_CALLOC(1, sizeof(RenderTF_PlatonicSolids));
	if (!vd->platonicSolids) {
		return false;
	}
	RenderTF_PlatonicSolids *ps = vd->platonicSolids;

	ps->shader = CreateShaders(vd);
	if(!Render_ShaderHandleIsValid(ps->shader)) {
		RenderTF_PlatonicSolidsDestroy(vd);
		return false;
	}
	static Render_VertexLayout vertexLayout{
			6,
			{
					{TheForge_SS_POSITION, 8, "POSITION", TinyImageFormat_R32G32B32_SFLOAT, 0, 0, 0, TheForge_VAR_VERTEX},
					{TheForge_SS_NORMAL, 9, "NORMAL", TinyImageFormat_R32G32B32_SFLOAT, 0, 1, sizeof(float) * 3, TheForge_VAR_VERTEX},
					{TheForge_SS_COLOR, 5, "COLOR", TinyImageFormat_R8G8B8A8_UNORM, 0, 2, sizeof(float) * 6, TheForge_VAR_VERTEX},

					{TheForge_SS_TEXCOORD0, 12, "INSTANCEROW", TinyImageFormat_R32G32B32A32_SFLOAT, 1, 0, 0, TheForge_VAR_INSTANCE },
					{TheForge_SS_TEXCOORD1, 12, "INSTANCEROW", TinyImageFormat_R32G32B32A32_SFLOAT, 1, 1, sizeof(float)*4, TheForge_VAR_INSTANCE },
					{TheForge_SS_TEXCOORD2, 12, "INSTANCEROW", TinyImageFormat_R32G32B32A32_SFLOAT, 1, 2, sizeof(float)*8, TheForge_VAR_INSTANCE }
			}
	};
	TinyImageFormat colourFormats[] = {Render_FrameBufferColourFormat(vd->target)};


	Render_GraphicsPipelineDesc platonicPipeDesc{};
	platonicPipeDesc.shader = ps->shader;
	platonicPipeDesc.rootSignature = vd->rootSignature;
	platonicPipeDesc.vertexLayout = (Render_VertexLayoutHandle) &vertexLayout;
	platonicPipeDesc.blendState = Render_GetStockBlendState(vd->renderer, Render_SBS_OPAQUE);
	platonicPipeDesc.depthState = Render_GetStockDepthState(vd->renderer, Render_SDS_IGNORE);
	platonicPipeDesc.rasteriserState = Render_GetStockRasterisationState(vd->renderer, Render_SRS_BACKCULL);
	platonicPipeDesc.colourRenderTargetCount = 1;
	platonicPipeDesc.colourFormats = colourFormats;
	platonicPipeDesc.depthStencilFormat = TinyImageFormat_UNDEFINED;
	platonicPipeDesc.sampleCount = 1;
	platonicPipeDesc.sampleQuality = 0;
	platonicPipeDesc.primitiveTopo = Render_PT_TRI_LIST;
	ps->pipeline = Render_GraphicsPipelineCreate(vd->renderer, &platonicPipeDesc);
	if (!Render_PipelineHandleIsValid(ps->pipeline)) {
		RenderTF_PlatonicSolidsDestroy(vd);
		return false;
	}

	// todo use temp allocator
	CADT_VectorHandle vertices = CADT_VectorCreate(sizeof(Vertex));

	uint32_t curIndex = 0;

	ps->solids[(int)SolidType::Tetrahedron].startVertex = curIndex;
	ps->solids[(int)SolidType::Tetrahedron].vertexCount = CreateTetrahedon(vertices);
	curIndex += ps->solids[(int)SolidType::Tetrahedron].vertexCount;

	ps->solids[(int)SolidType::Cube].startVertex = curIndex;
	ps->solids[(int)SolidType::Cube].vertexCount = CreateCube(vertices);
	curIndex += ps->solids[(int)SolidType::Cube].vertexCount;

	ps->solids[(int)SolidType::Octahedron].startVertex = curIndex;
	ps->solids[(int)SolidType::Octahedron].vertexCount = CreateOctahedron(vertices);
	curIndex += ps->solids[(int)SolidType::Octahedron].vertexCount;

	ps->solids[(int)SolidType::Icosahedron].startVertex = curIndex;
	ps->solids[(int)SolidType::Icosahedron].vertexCount = CreateIcosahedron(vertices);
	curIndex += ps->solids[(int)SolidType::Icosahedron].vertexCount;

	ps->solids[(int)SolidType::Dodecahedron].startVertex = curIndex;
	ps->solids[(int)SolidType::Dodecahedron].vertexCount = CreateDodecahedron(vertices);
	curIndex += ps->solids[(int)SolidType::Dodecahedron].vertexCount;

	Render_BufferVertexDesc vbDesc{
			(uint32_t) CADT_VectorSize(vertices),
			sizeof(Vertex),
			false
	};

	ps->gpuVertexData = Render_BufferCreateVertex(vd->renderer, &vbDesc);
	if (!Render_BufferHandleIsValid(ps->gpuVertexData)) {
		RenderTF_PlatonicSolidsDestroy(vd);
		return false;
	}

	// upload the vertex data
	Render_BufferUpdateDesc vertexUpdate = {
			CADT_VectorData(vertices),
			0,
			sizeof(Vertex) * CADT_VectorSize(vertices)
	};
	Render_BufferUpload(ps->gpuVertexData, &vertexUpdate);

	CADT_VectorDestroy(vertices);

	for(auto i=0;i < (int)SolidType::COUNT;++i) {
		ps->solids[i].instanceData = CADT_VectorCreate(sizeof(Instance));
	}

	return true;
}

void RenderTF_PlatonicSolidsDestroy(RenderTF_VisualDebug *vd) {
	RenderTF_PlatonicSolids *ps = vd->platonicSolids;

	for(auto i=0;i < (int)SolidType::COUNT;++i) {
		if(Render_BufferHandleIsValid(ps->solids[i].gpuInstanceData)) {
			Render_BufferDestroy(vd->renderer, ps->solids[i].gpuInstanceData);
		}
		CADT_VectorDestroy(ps->solids[i].instanceData);
	}

	if (Render_ShaderHandleIsValid(ps->shader)) {
		Render_ShaderDestroy(vd->renderer, ps->shader);
	}

	if (Render_PipelineHandleIsValid(ps->pipeline)) {
		Render_PipelineDestroy(vd->renderer, ps->pipeline);
	}

	if (Render_BufferHandleIsValid(ps->gpuVertexData)) {
		Render_BufferDestroy(vd->renderer, ps->gpuVertexData);
	}

	MEMORY_FREE(vd->platonicSolids);
	vd->platonicSolids = nullptr;
}

void RenderTF_PlatonicSolidsRender(RenderTF_VisualDebug *vd, Render_GraphicsEncoderHandle encoder) {
	RenderTF_PlatonicSolids *ps = vd->platonicSolids;

	Render_GraphicsEncoderSetScissor(encoder, Render_FrameBufferEntireScissor(vd->target));
	Render_GraphicsEncoderSetViewport(encoder, Render_FrameBufferEntireViewport(vd->target), { 0, 20});

	Render_GraphicsEncoderBindDescriptorSet(encoder, vd->descriptorSet, 0);
	Render_GraphicsEncoderBindPipeline(encoder, ps->pipeline);

	for(auto i=0;i < (int)SolidType::COUNT;++i) {
		Solid * solid = &ps->solids[i];
		uint32_t count = (uint32_t) CADT_VectorSize(solid->instanceData);
		if( count > solid->gpuCount) {
			if(Render_BufferHandleIsValid(solid->gpuInstanceData)) {
				Render_BufferDestroy(vd->renderer, solid->gpuInstanceData);
			}

			Render_BufferVertexDesc const ibDesc{
					count,
					sizeof(Instance),
					true
			};
			solid->gpuInstanceData = Render_BufferCreateVertex(vd->renderer, &ibDesc);
			solid->gpuCount = count;
		}
		if(count) {
			// upload the instance data
			Render_BufferUpdateDesc instanceUpdate = {
					CADT_VectorData(solid->instanceData),
					0,
					sizeof(Instance) * count
			};
			Render_BufferUpload(solid->gpuInstanceData, &instanceUpdate);

			Render_BufferHandle vertexBuffers[] = {ps->gpuVertexData, solid->gpuInstanceData};
			Render_GraphicsEncoderBindVertexBuffers(encoder, 2, vertexBuffers, nullptr);

			Render_GraphicsEncoderDrawInstanced(encoder,
																					solid->vertexCount, solid->startVertex,
																					count, 0);

			CADT_VectorResize(solid->instanceData, 0);
		}
	}
}


void RenderTF_PlatonicSolidsAddTetrahedron(RenderTF_VisualDebug* vd, Math_Mat4F transform) {
	RenderTF_PlatonicSolids *ps = vd->platonicSolids;

	Instance instance;
	memcpy(&instance, &transform, sizeof(float) * 12);
	CADT_VectorPushElement(ps->solids[(int)SolidType::Tetrahedron].instanceData, &instance);
}

void RenderTF_PlatonicSolidsAddCube(RenderTF_VisualDebug* vd, Math_Mat4F transform) {
	RenderTF_PlatonicSolids *ps = vd->platonicSolids;

	Instance instance;
	memcpy(&instance, &transform, sizeof(float) * 12);
	CADT_VectorPushElement(ps->solids[(int)SolidType::Cube].instanceData, &instance);
}

void RenderTF_PlatonicSolidsAddOctahedron(RenderTF_VisualDebug* vd, Math_Mat4F transform) {
	RenderTF_PlatonicSolids *ps = vd->platonicSolids;

	Instance instance;
	memcpy(&instance, &transform, sizeof(float) * 12);
	CADT_VectorPushElement(ps->solids[(int)SolidType::Octahedron].instanceData, &instance);
}

void RenderTF_PlatonicSolidsAddIcosahedron(RenderTF_VisualDebug* vd, Math_Mat4F transform) {
	RenderTF_PlatonicSolids *ps = vd->platonicSolids;

	Instance instance;
	memcpy(&instance, &transform, sizeof(float) * 12);
	CADT_VectorPushElement(ps->solids[(int)SolidType::Icosahedron].instanceData, &instance);
}

void RenderTF_PlatonicSolidsAddDodecahedron(RenderTF_VisualDebug* vd, Math_Mat4F transform) {
	RenderTF_PlatonicSolids *ps = vd->platonicSolids;

	Instance instance;
	memcpy(&instance, &transform, sizeof(float) * 12);
	CADT_VectorPushElement(ps->solids[(int)SolidType::Dodecahedron].instanceData, &instance);
}
