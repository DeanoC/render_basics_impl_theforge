#include "al2o3_platform/platform.h"
#include "al2o3_cadt/vector.h"
#include "render_basics/buffer.h"
#include "render_basics/pipeline.h"
#include "render_basics/framebuffer.h"
#include "render_basics/graphicsencoder.h"
#include "render_basics/descriptorset.h"
#include "visdebug.hpp"

struct RenderTF_PlatonicSolids {
	Render_BufferHandle gpuVertexData;
	uint32_t tetrahedronStartVertex;
	uint32_t cubeStartVertex;

	Render_ShaderHandle shader;
	Render_GraphicsPipelineHandle pipeline;

	CADT_VectorHandle tetrahedronInstanceData;
	uint32_t gpuTetrahedronCount;
	Render_BufferHandle gpuTetrahedronInstanceData;

	CADT_VectorHandle cubeInstanceData;
	uint32_t gpuCubeCount;
	Render_BufferHandle gpuCubeInstanceData;

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

	VFile_Handle vfile = VFile_FromMemory(VertexShader, strlen(VertexShader) + 1, false);
	if (!vfile) {
		return nullptr;
	}
	VFile_Handle ffile = VFile_FromMemory(FragmentShader, strlen(FragmentShader) + 1, false);
	if (!ffile) {
		VFile_Close(vfile);
		return nullptr;
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
			 0,  1,  0,
			 1, -1,  1,
			-1, -1,  1,
			 0, -1, -1,
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
} // end anon namespac
bool RenderTF_PlatonicSolidsCreate(RenderTF_VisualDebug *vd) {

	vd->platonicSolids = (RenderTF_PlatonicSolids *) MEMORY_CALLOC(1, sizeof(RenderTF_PlatonicSolids));
	if (!vd->platonicSolids) {
		return false;
	}
	RenderTF_PlatonicSolids *ps = vd->platonicSolids;

	ps->shader = CreateShaders(vd);
	if(!ps->shader) {
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
					{TheForge_SS_TEXCOORD2, 12, "INSTANCEROW", TinyImageFormat_R32G32B32A32_SFLOAT, 1, 2, sizeof(float)*4, TheForge_VAR_INSTANCE }
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
	platonicPipeDesc.depthStencilFormat = Render_FrameBufferDepthFormat(vd->target);
	platonicPipeDesc.sampleCount = 1;
	platonicPipeDesc.sampleQuality = 0;
	platonicPipeDesc.primitiveTopo = Render_PT_TRI_LIST;
	ps->pipeline = Render_GraphicsPipelineCreate(vd->renderer, &platonicPipeDesc);
	if (!ps->pipeline) {
		RenderTF_PlatonicSolidsDestroy(vd);
		return false;
	}

	// todo use temp allocator
	CADT_VectorHandle vertices = CADT_VectorCreate(sizeof(Vertex));

	uint32_t curIndex = 0;
	ps->tetrahedronStartVertex = curIndex;
	uint32_t const numTetraVertices = CreateTetrahedon(vertices);
	curIndex += numTetraVertices;
	ps->cubeStartVertex = curIndex;
	uint32_t const numCubeVertices = CreateCube(vertices);
	curIndex += numCubeVertices;

	Render_BufferVertexDesc vbDesc{
			(uint32_t) CADT_VectorSize(vertices),
			sizeof(Vertex),
			false
	};

	ps->gpuVertexData = Render_BufferCreateVertex(vd->renderer, &vbDesc);
	if (!ps->gpuVertexData) {
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

	ps->tetrahedronInstanceData = CADT_VectorCreate(sizeof(Instance));
	ps->cubeInstanceData = CADT_VectorCreate(sizeof(Instance));

	return true;
}

void RenderTF_PlatonicSolidsDestroy(RenderTF_VisualDebug *vd) {
	RenderTF_PlatonicSolids *ps = vd->platonicSolids;

	if(ps->gpuCubeInstanceData) {
		Render_BufferDestroy(vd->renderer, ps->gpuCubeInstanceData);
	}

	if(ps->gpuTetrahedronInstanceData) {
		Render_BufferDestroy(vd->renderer, ps->gpuTetrahedronInstanceData);
	}

	if(ps->cubeInstanceData) {
		CADT_VectorDestroy(ps->cubeInstanceData);
	}

	if(ps->tetrahedronInstanceData) {
		CADT_VectorDestroy(ps->tetrahedronInstanceData);
	}

	if (ps->shader) {
		Render_ShaderDestroy(vd->renderer, ps->shader);
	}

	if (ps->pipeline) {
		Render_GraphicsPipelineDestroy(vd->renderer, ps->pipeline);
	}

	if (ps->gpuVertexData) {
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

	uint32_t const tetrahedronCount = (uint32_t)CADT_VectorSize(ps->tetrahedronInstanceData);
	if(tetrahedronCount > ps->gpuTetrahedronCount) {
		Render_BufferVertexDesc const ibDesc{
				tetrahedronCount,
				sizeof(Instance),
				true
		};
		ps->gpuTetrahedronInstanceData = Render_BufferCreateVertex(vd->renderer, &ibDesc);
		ps->gpuTetrahedronCount = tetrahedronCount;
	}
	uint32_t const cubeCount = (uint32_t)CADT_VectorSize(ps->cubeInstanceData);
	if(cubeCount > ps->gpuCubeCount) {
		Render_BufferVertexDesc const ibDesc{
				cubeCount,
				sizeof(Instance),
				true
		};
		ps->gpuCubeInstanceData = Render_BufferCreateVertex(vd->renderer, &ibDesc);
		ps->gpuCubeCount = cubeCount;
	}


	if(tetrahedronCount) {
		// upload the instance data
		Render_BufferUpdateDesc uniformUpdate = {
				CADT_VectorData(ps->tetrahedronInstanceData),
				0,
				sizeof(Instance) * tetrahedronCount
		};
		Render_BufferUpload(ps->gpuTetrahedronInstanceData, &uniformUpdate);

		Render_BufferHandle vertexBuffers[] = { ps->gpuVertexData, ps->gpuTetrahedronInstanceData};
		Render_GraphicsEncoderBindVertexBuffers(encoder, 2, vertexBuffers, nullptr);

		Render_GraphicsEncoderDrawInstanced(encoder,
				4 * 3, ps->tetrahedronStartVertex,
				tetrahedronCount,0);

		CADT_VectorResize(ps->tetrahedronInstanceData, 0);
	}

	if(cubeCount) {
		// upload the instance data
		Render_BufferUpdateDesc uniformUpdate = {
				CADT_VectorData(ps->cubeInstanceData),
				0,
				sizeof(Instance) * cubeCount
		};
		Render_BufferUpload(ps->gpuCubeInstanceData, &uniformUpdate);

		Render_BufferHandle vertexBuffers[] = { ps->gpuVertexData, ps->gpuCubeInstanceData};
		Render_GraphicsEncoderBindVertexBuffers(encoder, 2, vertexBuffers, nullptr);

		Render_GraphicsEncoderDrawInstanced(encoder,
																				6 * 6, ps->cubeStartVertex,
																				cubeCount,0);

		CADT_VectorResize(ps->cubeInstanceData, 0);
	}

}

/*
auto PlatonicSolids::CreateOctahedron() -> std::unique_ptr<MeshMod::Mesh>
{
	using namespace MeshMod;
	using namespace Math;
	auto mesh = std::make_unique<Mesh>("Octahedron");

	static const vec3 pos[] = {
			{-1, 0,  0},
			{1,  0,  0},
			{0,  -1, 0},
			{0,  1,  0},
			{0,  0,  -1},
			{0,  0,  1},
	};
	using vi = VertexIndex;
	static const VertexIndexContainer faces[] = {
			{ vi(0), vi(3), vi(5) },
			{ vi(0), vi(5), vi(2) },
			{ vi(4), vi(3), vi(0) },
			{ vi(4), vi(0), vi(2) },
			{ vi(5), vi(3), vi(1) },
			{ vi(5), vi(1), vi(2) },
			{ vi(4), vi(1), vi(3) },
			{ vi(4), vi(2), vi(1) },
	};
	for(auto p : pos)
	{
		mesh->getVertices().add(p.x, p.y, p.z);
	}
	for(auto f : faces)
	{
		mesh->getPolygons().addPolygon(f);
	}
	mesh->updateEditState(MeshMod::Mesh::TopologyEdits);
	mesh->updateFromEdits();

	assert(mesh->getVertices().getCount() == 6);
	assert(mesh->getPolygons().getCount() == 8);
	assert(mesh->getHalfEdges().getCount() == 24);

	return std::move(mesh);
}

auto PlatonicSolids::CreateIcosahedron() -> std::unique_ptr<MeshMod::Mesh>
{
	using namespace MeshMod;
	using namespace Math;

	using namespace MeshMod;
	using namespace Math;
	auto mesh = std::make_unique<Mesh>("Icosahedron");

	// Phi - the square root of 5 plus 1 divided by 2
	double phi = (1.0 + sqrt(5.0)) * 0.5;

	float a = 0.5f;
	float b = (float)(1.0 / (2.0f * phi));

	// scale a bit TODO do this properly
	a = a * 2;
	b = b * 2;

	static const vec3 pos[] = {
			{0,  b,  -a},
			{b,  a,  0},
			{-b, a,  0},
			{0,  b,  a},
			{0,  -b, a},
			{-a, 0,  b},
			{a,  0,  b},
			{0,  -b, -a},
			{a,  0,  -b},
			{-a, 0,  -b},
			{b,  -a, 0},
			{-b, -a, 0}
	};

	using vi = VertexIndex;
	static const VertexIndexContainer faces[] = {
			{ vi(0), vi(1), vi(2) },
			{ vi(0), vi(2), vi(9) },
			{ vi(0), vi(7), vi(8) },
			{ vi(0), vi(8), vi(1) },
			{ vi(0), vi(9), vi(7) },
			{ vi(1), vi(6), vi(3) },
			{ vi(1), vi(8), vi(6) },
			{ vi(1), vi(3), vi(2) },
			{ vi(2), vi(3), vi(5) },
			{ vi(2), vi(5), vi(9) },
			{ vi(3), vi(4), vi(5) },
			{ vi(3), vi(6), vi(4) },
			{ vi(4), vi(6), vi(10) },
			{ vi(4), vi(10),vi(11) },
			{ vi(5), vi(4), vi(11) },
			{ vi(5), vi(11),vi(9) },
			{ vi(6), vi(8), vi(10) },
			{ vi(7), vi(9), vi(11) },
			{ vi(7), vi(10),vi(8) },
			{ vi(7), vi(11),vi(10) },
	};
	for(auto p : pos)
	{
		mesh->getVertices().add(p.x, p.y, p.z);
	}
	for(auto f : faces)
	{
		mesh->getPolygons().addPolygon(f);
	}
	mesh->updateEditState(MeshMod::Mesh::TopologyEdits);
	mesh->updateFromEdits();

	assert(mesh->getVertices().getCount() == 12);
	assert(mesh->getPolygons().getCount() == 20);
	assert(mesh->getHalfEdges().getCount() == 60);

	return mesh;
}

*/

void RenderTF_PlatonicSolidsAddTetrahedron(RenderTF_VisualDebug* vd, Math_Mat4F transform) {
	RenderTF_PlatonicSolids *ps = vd->platonicSolids;

	Instance instance;
	memcpy(&instance, &transform, sizeof(float) * 12);
	CADT_VectorPushElement(ps->tetrahedronInstanceData, &instance);
}

void RenderTF_PlatonicSolidsAddCube(RenderTF_VisualDebug* vd, Math_Mat4F transform) {
	RenderTF_PlatonicSolids *ps = vd->platonicSolids;

	Instance instance;
	memcpy(&instance, &transform, sizeof(float) * 12);
	CADT_VectorPushElement(ps->cubeInstanceData, &instance);
}

