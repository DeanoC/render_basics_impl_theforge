#pragma once

/// if you use this always include BEFORE render_basics/ headers

#include "al2o3_platform/platform.h"
#include "gfx_theforge/theforge.h"
#include "gfx_shadercompiler/compiler.h"

#define Render_BlendState TheForge_BlendState
#define Render_CmdPool TheForge_CmdPool
#define Render_Cmd TheForge_Cmd
#define Render_DepthState TheForge_DepthState
#define Render_Queue TheForge_Queue
#define Render_RasteriserState TheForge_RasterizerState
#define Render_RenderTarget TheForge_RenderTarget
#define Render_Sampler TheForge_Sampler
#define Render_VertexLayout TheForge_VertexLayout const

#include "render_basics/api.h"

typedef struct Render_Renderer {
	TheForge_RendererHandle renderer;

	TheForge_QueueHandle graphicsQueue;
	TheForge_CmdPoolHandle graphicsCmdPool;
	TheForge_QueueHandle computeQueue;
	TheForge_CmdPoolHandle computeCmdPool;
	TheForge_QueueHandle blitQueue;
	TheForge_CmdPoolHandle blitCmdPool;

	ShaderCompiler_ContextHandle shaderCompiler;

	TheForge_BlendStateHandle stockBlendState[Render_SSB_COUNT];
	TheForge_DepthStateHandle stockDepthState[Render_SDS_COUNT];
	TheForge_RasterizerStateHandle stockRasterState[Render_SRS_COUNT];
	TheForge_SamplerHandle stockSamplers[Render_SST_COUNT];
	TheForge_VertexLayout const *stockVertexLayouts[Render_SVL_COUNT];

} Render_Renderer;

