/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GRALLOC_PRIV_H_
#define GRALLOC_PRIV_H_

#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <unistd.h>

#include <hardware/gralloc.h>
#include <cutils/native_handle.h>
#include <alloc_device.h>
#include <utils/Log.h>
#define GRALLOC_ARM_UMP_MODULE 1
#define GRALLOC_ARM_DMA_BUF_MODULE 0

#if GRALLOC_ARM_UMP_MODULE
#include <ump/ump.h>
#endif

struct private_handle_t;

struct private_module_t
{
	gralloc_module_t base;

	private_handle_t* framebuffer;
	uint32_t fbFormat;
	uint32_t flags;
	uint32_t numBuffers;
	uint32_t bufferMask;
	pthread_mutex_t lock;
	buffer_handle_t currentBuffer;
	int ion_client;
	int mIonFd;
	int mIonBufNum;
	
	struct fb_var_screeninfo info;
	struct fb_fix_screeninfo finfo;
	float xdpi;
	float ydpi;
	float fps;

	enum
	{
		// flag to indicate we'll post this buffer
		PRIV_USAGE_LOCKED_FOR_POST = 0x80000000
	};

	/* default constructor */
	private_module_t();
};

#ifdef __cplusplus
struct private_handle_t : public native_handle
{
#else
struct private_handle_t
{
	struct native_handle nativeHandle;
#endif

	enum
	{
		PRIV_FLAGS_FRAMEBUFFER = 0x00000001,
		PRIV_FLAGS_USES_UMP    = 0x00000002,
		PRIV_FLAGS_USES_ION    = 0x00000004,
		PRIV_FLAGS_USES_PHY	 = 0x00000008,
		PRIV_FLAGS_NOT_OVERLAY	 = 0x00000010,
	};

	enum
	{
		LOCK_STATE_WRITE     =   1<<31,
		LOCK_STATE_MAPPED    =   1<<30,
		LOCK_STATE_READ_MASK =   0x3FFFFFFF
	};
	//fds
	int     fd;

	// ints
#if GRALLOC_ARM_DMA_BUF_MODULE
	/*shared file descriptor for dma_buf sharing*/
	int     share_fd;
#endif
	int     magic;
	int     flags;
	int     size;
	int     base;
	int     lockState;
	int     writeOwner;
	int     pid;

	// Following members are for UMP memory only
#if GRALLOC_ARM_UMP_MODULE
	int     ump_id;
	int     ump_mem_handle;
#define GRALLOC_ARM_UMP_NUM_INTS 2
#else
#define GRALLOC_ARM_UMP_NUM_INTS 0
#endif

	// Following members is for framebuffer only
	int     offset;
	//int     fd;//move to "fds"

	int     phyaddr;
	int     format;
	int     width;
	int     height;
	int     resv0;
	int     resv1;   

#if GRALLOC_ARM_DMA_BUF_MODULE
	int     ion_client;
	struct ion_handle *ion_hnd;
#define GRALLOC_ARM_DMA_BUF_NUM_INTS 3 
#else
#define GRALLOC_ARM_DMA_BUF_NUM_INTS 0
#endif

#if GRALLOC_ARM_DMA_BUF_MODULE
#define GRALLOC_ARM_NUM_FDS 1	
#else
#define GRALLOC_ARM_NUM_FDS 0	
#endif

#ifdef __cplusplus
	static const int sNumInts = 9  -1 + 6 + GRALLOC_ARM_UMP_NUM_INTS + GRALLOC_ARM_DMA_BUF_NUM_INTS;
	static const int sNumFds = 1;
	static const int sMagic = 0x3141592;

#if GRALLOC_ARM_UMP_MODULE
	private_handle_t(int flags, int size, int base, int lock_state, ump_secure_id secure_id, ump_handle handle, int offset = 0, int fd = 0):
		magic(sMagic),
		flags(flags),
		size(size),
		base(base),
		lockState(lock_state),
		writeOwner(0),
		pid(getpid()),
		ump_id((int)secure_id),
		ump_mem_handle((int)handle),
		fd(fd),
		offset(offset)
#if GRALLOC_ARM_DMA_BUF_MODULE
		,ion_client(-1),
		ion_hnd(NULL)
#endif

	{
		version = sizeof(native_handle);
		numFds = sNumFds;
		numInts = sNumInts;
	}
#endif

#if GRALLOC_ARM_DMA_BUF_MODULE
	private_handle_t(int flags, int size, int base, int lock_state):
		magic(sMagic),
		flags(flags),
		size(size),
		base(base),
		lockState(lock_state),
		writeOwner(0),
		pid(getpid()),
#if GRALLOC_ARM_UMP_MODULE
		ump_id((int)UMP_INVALID_SECURE_ID),
		ump_mem_handle((int)UMP_INVALID_MEMORY_HANDLE),
#endif
		fd(0),
		offset(0),
		ion_client(-1),
		ion_hnd(NULL)

	{
		version = sizeof(native_handle);
		numFds = sNumFds;
		numInts = sNumInts;
	}

#endif

	private_handle_t(int flags, int size, int base, int lock_state, int fb_file, int fb_offset):
		magic(sMagic),
		flags(flags),
		size(size),
		base(base),
		lockState(lock_state),
		writeOwner(0),
		pid(getpid()),
#if GRALLOC_ARM_UMP_MODULE
		ump_id((int)UMP_INVALID_SECURE_ID),
		ump_mem_handle((int)UMP_INVALID_MEMORY_HANDLE),
#endif
		fd(fb_file),
		offset(fb_offset)
#if GRALLOC_ARM_DMA_BUF_MODULE
		,ion_client(-1),
		ion_hnd(NULL)
#endif

	{
		version = sizeof(native_handle);
		numFds = sNumFds;
		numInts = sNumInts;
	}

	~private_handle_t()
	{
		magic = 0;
	}

	bool usesPhysicallyContiguousMemory()
	{
		return (flags & (PRIV_FLAGS_FRAMEBUFFER|PRIV_FLAGS_USES_PHY)) ? true : false;
	}

	static int validate(const native_handle* h)
	{
		const private_handle_t* hnd = (const private_handle_t*)h;
		if (!h || h->version != sizeof(native_handle) || h->numInts != sNumInts || h->numFds != sNumFds || hnd->magic != sMagic)
		{
			return -EINVAL;
		}
		return 0;
	}

	static private_handle_t* dynamicCast(const native_handle* in)
	{
		if (validate(in) == 0)
		{
			return (private_handle_t*) in;
		}
		return NULL;
	}
#endif
};
enum {
    /* FIXME: this only exists to work-around some issues with
     * the video and camera frameworks. don't implement unless
     * you know what you're doing.
     */
    GRALLOC_MODULE_PERFORM_CREATE_HANDLE_FROM_BUFFER = 0x080000001,
    GRALLOC_MODULE_PERFORM_FREE_HANDLE,
    GRALLOC_MODULE_PERFORM_GET_MALI_DATA,
    GRALLOC_MODULE_GET_MALI_INTERNAL_BUF_OFF
};
#endif /* GRALLOC_PRIV_H_ */
