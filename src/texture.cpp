#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "gfx_theforge/theforge.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/texture.h"
#include "render_basics/theforge/handlemanager.h"

TheForge_DescriptorType Render_TextureUsageFlagsToDescriptorType(Render_TextureUsageFlags tuf) {
	uint32_t dt = 0;
	if (tuf & Render_TUF_SHADER_READ) {
		dt |= TheForge_DESCRIPTOR_TYPE_TEXTURE;
	}
	if (tuf & Render_TUF_SHADER_WRITE) {
		dt |= TheForge_DESCRIPTOR_TYPE_RW_TEXTURE;
	}

	return (TheForge_DescriptorType) dt;
}

AL2O3_EXTERN_C Render_TextureHandle Render_TextureSyncCreate(Render_RendererHandle renderer,
																														 Render_TextureCreateDesc const *desc) {

	Render_TextureHandle handle = Render_TextureHandleAlloc();
	if (!handle.handle) {
		return {Handle_InvalidDynamicHandle32};
	}
	Render_Texture* texture = Render_TextureHandleToPtr(handle);

	// the forge has seperate textures and render targets, whereas we just define it via ROP_READ/WRITE
	// split creation if ROP_WRITE is defined
	if(desc->usageflags & Render_TUF_ROP_WRITE) {
		// TODO check render target format can be written to and if require blended with

		TheForge_RenderTargetDesc rtDesc {
			TheForge_TCF_OWN_MEMORY_BIT,
			desc->width,
			desc->height,
			desc->depth,
			desc->slices,
			desc->mipLevels,
			(TheForge_SampleCount) (desc->sampleCount ? desc->sampleCount : 1),
			desc->format,
			{{
					 desc->renderTargetClearValue.r,
					 desc->renderTargetClearValue.g,
					 desc->renderTargetClearValue.b,
					 desc->renderTargetClearValue.a}},
			TheForge_RS_UNDEFINED,
			Render_TextureUsageFlagsToDescriptorType(desc->usageflags),
			desc->debugName,
		};

		TheForge_AddRenderTarget(renderer->renderer, &rtDesc, &texture->renderTarget);
		if(!texture->renderTarget) {
			Render_TextureHandleRelease(handle);
			return {Handle_InvalidDynamicHandle32};
		}
		texture->texture = TheForge_RenderTargetGetTexture(texture->renderTarget);

	} else {
		// plain texture
		ASSERT((desc->usageflags & Render_TUF_ROP_READ) == 0);

		TheForge_TextureDesc const texDesc{
				TheForge_TCF_NONE,
				desc->width,
				desc->height,
				desc->depth,
				desc->slices,
				desc->mipLevels,
				(TheForge_SampleCount) (desc->sampleCount ? desc->sampleCount : 1),
				desc->sampleQuality,
				desc->format,
				{{}},
				TheForge_RS_UNDEFINED,
				Render_TextureUsageFlagsToDescriptorType(desc->usageflags),
				nullptr,
				desc->debugName,
				nullptr,
				0,
				0
		};

		TheForge_AddTexture(renderer->renderer, &texDesc, &texture->texture);
		if (!texture->texture) {
			Render_TextureHandleRelease(handle);
			return {Handle_InvalidDynamicHandle32};
		}
	}

	if (desc->initialData != nullptr) {
		Render_TextureUpdateDesc update{
				desc->format,
				desc->width,
				desc->height,
				desc->depth,
				desc->slices,
				desc->mipLevels,
				desc->initialData,
		};

		Render_TextureSyncUpdate(handle, &update);
	}

	return handle;
}

AL2O3_EXTERN_C void Render_TextureDestroy(Render_RendererHandle renderer, Render_TextureHandle handle) {
	if (!renderer || !handle.handle) {
		return;
	}
	Render_Texture* texture = Render_TextureHandleToPtr(handle);
	if(texture->renderTarget) {
		TheForge_RemoveRenderTarget(renderer->renderer, texture->renderTarget);
	} else {
		TheForge_RemoveTexture(renderer->renderer, texture->texture);
	}

	Render_TextureHandleRelease(handle);
}

AL2O3_EXTERN_C void Render_TextureSyncUpdate(Render_TextureHandle handle, Render_TextureUpdateDesc const *desc) {
	TheForge_RawImageData rid{
			(unsigned char const *) desc->data,
			desc->format,
			desc->width,
			desc->height,
			desc->depth,
			desc->slices,
			desc->mipLevels,
			true
	};

	Render_Texture* texture = Render_TextureHandleToPtr(handle);

	TheForge_TextureUpdateDesc updateDesc{
			texture->texture,
			&rid
	};

	TheForge_UpdateTexture(&updateDesc, false);
}


AL2O3_EXTERN_C uint32_t Render_TextureGetWidth(Render_TextureHandle handle) {
	return TheForge_TextureGetWidth(Render_TextureHandleToPtr(handle)->texture);
}
AL2O3_EXTERN_C uint32_t Render_TextureGetHeight(Render_TextureHandle handle) {
	return TheForge_TextureGetHeight(Render_TextureHandleToPtr(handle)->texture);
}
AL2O3_EXTERN_C uint32_t Render_TextureGetDepth(Render_TextureHandle handle) {
	return TheForge_TextureGetDepth(Render_TextureHandleToPtr(handle)->texture);
}
AL2O3_EXTERN_C uint32_t Render_TextureGetSliceCount(Render_TextureHandle handle) {
	return TheForge_TextureGetArraySize(Render_TextureHandleToPtr(handle)->texture);
}
AL2O3_EXTERN_C uint32_t Render_TextureGetMipLevelCount(Render_TextureHandle handle) {
	return TheForge_TextureGetMipLevels(Render_TextureHandleToPtr(handle)->texture);
}
AL2O3_EXTERN_C TinyImageFormat Render_TextureGetFormat(Render_TextureHandle handle) {
	return TheForge_TextureGetFormat(Render_TextureHandleToPtr(handle)->texture);
}
