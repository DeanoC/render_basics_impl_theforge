#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"

#include "render_basics/theforge/api.h"
#include "render_basics/theforge/handlemanager.h"
#include "render_basics/api.h"


AL2O3_EXTERN_C Render_ComputeEncoderHandle Render_ComputeEncoderCreate(Render_RendererHandle renderer) {

	Render_ComputeEncoderHandle handle = Render_ComputeEncoderHandleAlloc();
	Render_ComputeEncoder* encoder = Render_ComputeEncoderHandleToPtr(handle);
	encoder->cmdPool = renderer->blitCmdPool;
	TheForge_AddCmd(encoder->cmdPool, false, &encoder->cmd);

	return handle;
}

AL2O3_EXTERN_C void Render_ComputeEncoderDestroy(Render_RendererHandle renderer, Render_ComputeEncoderHandle handle){

	Render_ComputeEncoder* encoder = Render_ComputeEncoderHandleToPtr(handle);
	TheForge_RemoveCmd(encoder->cmdPool, encoder->cmd);
	Render_ComputeEncoderHandleRelease(handle);

}
