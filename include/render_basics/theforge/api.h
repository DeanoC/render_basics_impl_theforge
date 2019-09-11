#pragma once

#include "al2o3_platform/platform.h"
#include "gfx_theforge/theforge.h"
#include "gfx_shadercompiler/compiler.h"

typedef struct Render_Renderer {
	TheForge_RendererHandle renderer;

	TheForge_QueueHandle graphicsQueue;
	TheForge_CmdPoolHandle graphicsCmdPool;
	TheForge_QueueHandle computeQueue;
	TheForge_CmdPoolHandle computeCmdPool;
	TheForge_QueueHandle blitQueue;
	TheForge_CmdPoolHandle blitCmdPool;

	ShaderCompiler_ContextHandle shaderCompiler;

} Render_Renderer;

