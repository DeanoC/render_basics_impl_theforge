#pragma once

/// if you use this always include BEFORE render_basics/ headers

#include "al2o3_platform/platform.h"
#include "gfx_theforge/theforge.h"
#include "gfx_shadercompiler/compiler.h"

#define Render_BlendState TheForge_BlendState
#define Render_CmdPool TheForge_CmdPool
#define Render_Cmd TheForge_Cmd
#define Render_ComputePipeline TheForge_Pipeline
#define Render_DepthState TheForge_DepthState
#define Render_DescriptorBinder TheForge_DescriptorBinder
#define Render_GraphicsPipeline TheForge_Pipeline
#define Render_Queue TheForge_Queue
#define Render_RasteriserState TheForge_RasterizerState
#define Render_RootSignature TheForge_RootSignature
#define Render_RenderTarget TheForge_RenderTarget
#define Render_Sampler TheForge_Sampler
#define Render_Shader TheForge_Shader
#define Render_Texture TheForge_Texture
#define Render_VertexLayout TheForge_VertexLayout const

#include "render_basics/api.h"
#include "render_basics/shader.h"
#include "render_basics/view.h"

typedef struct Render_BlitEncoder {
	Render_CmdPoolHandle cmdPool;

	Render_CmdHandle cmd;
} Render_BlitEncoder;

typedef struct Render_Buffer {
	TheForge_BufferHandle buffer;

	uint32_t maxFrames;
	uint32_t curFrame;
	uint64_t size; // size of a single frame, total size = maxFrame * size
} Render_Buffer;

typedef struct Render_ComputeEncoder {
	Render_CmdPoolHandle cmdPool;

	Render_CmdHandle cmd;
} Render_ComputeEncoder;

typedef struct Render_GraphicsEncoder {
	Render_CmdPoolHandle cmdPool;

	Render_CmdHandle cmd;
	Render_View view;
} Render_GraphicsEncoder;



typedef struct Render_Renderer {
	TheForge_RendererHandle renderer;

	InputBasic_ContextHandle input; ///< can be null

	TheForge_QueueHandle graphicsQueue;
	TheForge_CmdPoolHandle graphicsCmdPool;
	TheForge_QueueHandle computeQueue;
	TheForge_CmdPoolHandle computeCmdPool;
	TheForge_QueueHandle blitQueue;
	TheForge_CmdPoolHandle blitCmdPool;

	ShaderCompiler_ContextHandle shaderCompiler;

	TheForge_BlendStateHandle stockBlendState[Render_SBS_COUNT];
	TheForge_DepthStateHandle stockDepthState[Render_SDS_COUNT];
	TheForge_RasterizerStateHandle stockRasterState[Render_SRS_COUNT];
	TheForge_SamplerHandle stockSamplers[Render_SST_COUNT];
	TheForge_VertexLayout const *stockVertexLayouts[Render_SVL_COUNT];

} Render_Renderer;

typedef struct Render_FrameBuffer {
	Render_Renderer *renderer;
	TheForge_CmdPoolHandle commandPool;
	TheForge_QueueHandle presentQueue;
	uint32_t frameBufferCount;

	TheForge_SwapChainHandle swapChain;
	TheForge_RenderTargetHandle depthBuffer;
	TheForge_FenceHandle *renderCompleteFences;
	TheForge_SemaphoreHandle imageAcquiredSemaphore;
	TheForge_SemaphoreHandle *renderCompleteSemaphores;
	TheForge_CmdHandle *frameCmds;

	struct ImguiBindings_Context *imguiBindings;

	struct RenderTF_VisualDebug *visualDebug;

	// current (this frame) data
	uint32_t frameIndex;
	TheForge_RenderTargetHandle currentColourTarget;
	TheForge_CmdHandle currentCmd;
	Render_GraphicsEncoder graphicsEncoder;

} Render_FrameBuffer;

typedef struct Render_ShaderObject {
	TheForge_ShaderStage shaderType;
	ShaderCompiler_Output output;
	char name[64];
	char entryPoint[64];
} Render_ShaderObject;
