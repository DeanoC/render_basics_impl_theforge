#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/pipeline.h"

AL2O3_EXTERN_C Render_GraphicsPipelineHandle Render_GraphicsPipelineCreate(Render_RendererHandle renderer,
																																					 Render_GraphicsPipelineDesc const *desc) {
	TheForge_PipelineDesc pipelineDesc{};
	pipelineDesc.type = TheForge_PT_GRAPHICS;
	TheForge_GraphicsPipelineDesc &gfxPipeDesc = pipelineDesc.graphicsDesc;

	gfxPipeDesc.shaderProgram = desc->shader;
	gfxPipeDesc.rootSignature = desc->rootSignature;

	gfxPipeDesc.rasterizerState = desc->rasteriserState;
	gfxPipeDesc.blendState = desc->blendState;
	gfxPipeDesc.depthState = desc->depthState;
	gfxPipeDesc.depthStencilFormat = desc->depthStencilFormat;

	gfxPipeDesc.renderTargetCount = desc->colourRenderTargetCount;
	gfxPipeDesc.sampleCount = (TheForge_SampleCount) desc->sampleCount;
	gfxPipeDesc.sampleQuality = desc->sampleQuality;
	gfxPipeDesc.pColorFormats = desc->colourFormats;

	gfxPipeDesc.pVertexLayout = desc->vertexLayout;
	gfxPipeDesc.primitiveTopo = (TheForge_PrimitiveTopology) desc->primitiveTopo;

	Render_GraphicsPipelineHandle pipeline = nullptr;
	TheForge_AddPipeline(renderer->renderer, &pipelineDesc, &pipeline);
	return pipeline;
}

AL2O3_EXTERN_C Render_ComputePipelineHandle Render_ComputePipelineCreate(Render_RendererHandle renderer,
																																				 Render_ComputePipelineDesc const *desc) {
	TheForge_PipelineDesc pipelineDesc{};
	pipelineDesc.type = TheForge_PT_COMPUTE;
	TheForge_GraphicsPipelineDesc &gfxPipeDesc = pipelineDesc.graphicsDesc;

	gfxPipeDesc.shaderProgram = desc->shader;
	gfxPipeDesc.rootSignature = desc->rootSignature;

	Render_ComputePipelineHandle pipeline = nullptr;
	TheForge_AddPipeline(renderer->renderer, &pipelineDesc, &pipeline);
	return pipeline;
}

AL2O3_EXTERN_C void Render_GraphicsPipelineDestroy(Render_RendererHandle renderer,
																									 Render_GraphicsPipelineHandle pipeline) {
	if (!renderer || !pipeline) {
		return;
	}

	TheForge_RemovePipeline(renderer->renderer, pipeline);
}
AL2O3_EXTERN_C void Render_CpmputePipelineDestroy(Render_RendererHandle renderer,
																									Render_ComputePipelineHandle pipeline) {
	if (!renderer || !pipeline) {
		return;
	}

	TheForge_RemovePipeline(renderer->renderer, pipeline);
}
