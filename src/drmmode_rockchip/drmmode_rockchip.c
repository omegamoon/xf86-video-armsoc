/*
 * Copyright © 2017 Omegamoon
 * Based on the rockchip libdrm implementation, which in turn seems to 
 * be pretty much a copy of the Exynos implementation
 */

#include "../drmmode_driver.h"
#include <stddef.h>
#include <xf86drmMode.h>
#include <xf86drm.h>
#include <sys/ioctl.h>

#define ROCKCHIP_BO_CONTIG 0
#define ROCKCHIP_BO_NONCONTIG 1

/**
 * User-desired buffer creation information structure.
 *
 * @size: user-desired memory allocation size.
 *	- this size value would be page-aligned internally.
 * @flags: user request for setting memory type or cache attributes.
 * @handle: returned a handle to created gem object.
 *	- this handle will be set by gem module of kernel side.
 */
struct drm_rockchip_gem_create {
	uint64_t size;
	uint32_t flags;
	uint32_t handle;
};

#define DRM_ROCKCHIP_GEM_CREATE	0x00
#define DRM_IOCTL_ROCKCHIP_GEM_CREATE	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_ROCKCHIP_GEM_CREATE, struct drm_rockchip_gem_create)

/* Cursor dimensions
 * Technically we probably don't have any size limit.. since we
 * are just using an overlay... but xserver will always create
 * cursor images in the max size, so don't use width/height values
 * that are too big
 */
#define CURSORW  (64)
#define CURSORH  (64)

/*
 * Padding added down each side of cursor image. This is a workaround for a bug
 * causing corruption when the cursor reaches the screen edges.
 */
#define CURSORPAD (16)

#define ALIGN(val, align)	(((val) + (align) - 1) & ~((align) - 1))

/* Optional function */
static int init_plane_for_cursor(int drm_fd, uint32_t plane_id) {
	return 0;
}

static int create_custom_gem(int fd, struct armsoc_create_gem *create_gem)
{
	struct drm_rockchip_gem_create req;
	int ret;
	unsigned int pitch;

	/* make pitch a multiple of 64 bytes for best performance */
	pitch = ALIGN(create_gem->width * ((create_gem->bpp + 7) / 8), 64);
	memset(&req, 0, sizeof(req));
	req.size = create_gem->height * pitch;

	assert((create_gem->buf_type == ARMSOC_BO_SCANOUT) ||
			(create_gem->buf_type == ARMSOC_BO_NON_SCANOUT));

	/* Contiguous allocations are not supported in some rockchip drm versions.
	 * When they are supported all allocations are effectively contiguous
	 * anyway, so for simplicity we always request non contiguous buffers.
	 */
	req.flags = ROCKCHIP_BO_NONCONTIG;

	ret = drmIoctl(fd, DRM_IOCTL_ROCKCHIP_GEM_CREATE, &req);
	if (ret)
		return ret;

	/* Convert custom req to generic create_gem */
	create_gem->handle = req.handle;
	create_gem->pitch = pitch;
	create_gem->size = req.size;

	return 0;
}

struct drmmode_interface rockchip_interface = {
	"rockchip"            /* name of drm driver */,
	1                     /* use_page_flip_events */,
	1                     /* use_early_display */,
	CURSORW               /* cursor width */,
	CURSORH               /* cursor_height */,
	CURSORPAD             /* cursor padding */,
	HWCURSOR_API_PLANE    /* cursor_api */,
	init_plane_for_cursor /* init_plane_for_cursor */,
	0                     /* vblank_query_supported */,
	create_custom_gem     /* create_custom_gem */,
};
