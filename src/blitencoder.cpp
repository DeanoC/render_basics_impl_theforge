#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"


AL2O3_EXTERN_C Render_BlitEncoderHandle Render_BlitEncoderCreate(Render_RendererHandle renderer, Render_CmdPoolHandle cmdPoolHandle) {
	auto encoder = (Render_BlitEncoderHandle) MEMORY_CALLOC(1, sizeof(Render_BlitEncoder));
	if (!encoder) {
		return nullptr;
	}

	TheForge_AddCmd(cmdPoolHandle, false, &encoder->cmd);
	encoder->cmdPool = cmdPoolHandle;

	return encoder;
}

AL2O3_EXTERN_C void Render_BlitEncoderDestroy(Render_RendererHandle renderer, Render_BlitEncoderHandle encoder){
	if(!renderer || !encoder) return;

	TheForge_RemoveCmd(encoder->cmdPool, encoder->cmd);

	MEMORY_FREE(encoder);
}


