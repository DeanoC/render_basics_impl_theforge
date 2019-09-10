#pragma once

#include "al2o3_platform/platform.h"
#include "gfx_theforge/theforge.h"
#include "gfx_shadercompiler/compiler.h"

typedef struct Render_Renderer {
	TheForge_RendererHandle renderer;
	TheForge_CmdPoolHandle cmdPool;

	TheForge_QueueHandle graphicsQueue;

	ShaderCompiler_ContextHandle shaderCompiler;

} Render_Renderer;

