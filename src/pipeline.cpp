#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/pipeline.h"
#include "render_basics/theforge/handlemanager.h"
#include "render_basics/theforge/api.h"
AL2O3_EXTERN_C Render_PipelineHandle Render_GraphicsPipelineCreate(Render_RendererHandle renderer,
																																					 Render_GraphicsPipelineDesc const *desc) {
	TheForge_PipelineDesc pipelineDesc{};
	pipelineDesc.type = TheForge_PT_GRAPHICS;
	TheForge_GraphicsPipelineDesc &gfxPipeDesc = pipelineDesc.graphicsDesc;

	gfxPipeDesc.shaderProgram = Render_ShaderHandleToPtr(desc->shader)->shader;
	gfxPipeDesc.rootSignature = Render_RootSignatureHandleToPtr(desc->rootSignature)->signature;

	gfxPipeDesc.rasterizerState = Render_RasteriserStateHandleToPtr(desc->rasteriserState)->state;
	gfxPipeDesc.blendState = Render_BlendStateHandleToPtr(desc->blendState)->state;
	gfxPipeDesc.depthState = Render_DepthStateHandleToPtr(desc->depthState)->state;
	gfxPipeDesc.depthStencilFormat = desc->depthStencilFormat;

	gfxPipeDesc.renderTargetCount = desc->colourRenderTargetCount;
	gfxPipeDesc.sampleCount = (TheForge_SampleCount) desc->sampleCount;
	gfxPipeDesc.sampleQuality = desc->sampleQuality;
	gfxPipeDesc.pColorFormats = desc->colourFormats;

	gfxPipeDesc.pVertexLayout = desc->vertexLayout;
	gfxPipeDesc.primitiveTopo = (TheForge_PrimitiveTopology) desc->primitiveTopo;

	Render_PipelineHandle handle = Render_PipelineHandleAlloc();
	Render_Pipeline* pipeline = Render_PipelineHandleToPtr(handle);
	TheForge_AddPipeline(renderer->renderer, &pipelineDesc, &pipeline->pipeline);
	if(!pipeline->pipeline) {
		Render_PipelineHandleRelease(handle);
		return { 0 };
	}
	return handle;
}

AL2O3_EXTERN_C Render_PipelineHandle Render_ComputePipelineCreate(Render_RendererHandle renderer,
																																				 Render_ComputePipelineDesc const *desc) {
	TheForge_PipelineDesc pipelineDesc{};
	pipelineDesc.type = TheForge_PT_COMPUTE;
	TheForge_GraphicsPipelineDesc &gfxPipeDesc = pipelineDesc.graphicsDesc;

	gfxPipeDesc.shaderProgram = Render_ShaderHandleToPtr(desc->shader)->shader;
	gfxPipeDesc.rootSignature = Render_RootSignatureHandleToPtr(desc->rootSignature)->signature;

	Render_PipelineHandle handle = Render_PipelineHandleAlloc();
	Render_Pipeline* pipeline = Render_PipelineHandleToPtr(handle);
	TheForge_AddPipeline(renderer->renderer, &pipelineDesc, &pipeline->pipeline);
	if(!pipeline->pipeline) {
		Render_PipelineHandleRelease(handle);
		return { 0 };
	}

	return handle;
}

AL2O3_EXTERN_C void Render_PipelineDestroy(Render_RendererHandle renderer,
																									 Render_PipelineHandle handle) {
	if (!renderer || !Render_PipelineHandleIsValid(handle)) {
		return;
	}
	Render_Pipeline* pipeline = Render_PipelineHandleToPtr(handle);

	TheForge_RemovePipeline(renderer->renderer, pipeline->pipeline);
	Render_PipelineHandleRelease(handle);
}
