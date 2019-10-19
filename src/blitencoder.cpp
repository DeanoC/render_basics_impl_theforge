#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"

#include "render_basics/theforge/api.h"
#include "render_basics/theforge/handlemanager.h"
#include "render_basics/api.h"


AL2O3_EXTERN_C Render_BlitEncoderHandle Render_BlitEncoderCreate(Render_RendererHandle renderer) {

	Render_BlitEncoderHandle handle = Render_BlitEncoderHandleAlloc();
	Render_BlitEncoder* encoder = Render_BlitEncoderHandleToPtr(handle);
	encoder->cmdPool = renderer->blitCmdPool;
	TheForge_AddCmd(encoder->cmdPool, false, &encoder->cmd);

	return handle;
}

AL2O3_EXTERN_C void Render_BlitEncoderDestroy(Render_RendererHandle renderer, Render_BlitEncoderHandle handle) {

	Render_BlitEncoder* encoder = Render_BlitEncoderHandleToPtr(handle);
	TheForge_RemoveCmd(encoder->cmdPool, encoder->cmd);
	Render_BlitEncoderHandleRelease(handle);

}


