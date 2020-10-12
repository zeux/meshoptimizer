#ifdef __wasi__
#include <stdlib.h>
#include <string.h>

#include <wasi/api.h>

extern "C" void __cxa_throw(void* ptr, void* type, void* destructor)
{
	abort();
}

extern "C" void* __cxa_allocate_exception(size_t thrown_size)
{
	abort();
}

extern "C" __wasi_errno_t __wasi_path_open32(__wasi_fd_t fd, __wasi_lookupflags_t dirflags, const char* path, size_t path_len, __wasi_oflags_t oflags, uint32_t fs_rights_base, uint32_t fs_rights_inherting, __wasi_fdflags_t fdflags, __wasi_fd_t* opened_fd)
    __attribute__((
        __import_module__("wasi_snapshot_preview1"),
        __import_name__("path_open32"),
        __warn_unused_result__));

__wasi_errno_t __wasi_path_open(__wasi_fd_t fd, __wasi_lookupflags_t dirflags, const char* path, size_t path_len, __wasi_oflags_t oflags, __wasi_rights_t fs_rights_base, __wasi_rights_t fs_rights_inherting, __wasi_fdflags_t fdflags, __wasi_fd_t* opened_fd)
{
	return __wasi_path_open32(fd, dirflags, path, path_len, oflags, fs_rights_base, fs_rights_inherting, fdflags, opened_fd);
}

extern "C" __wasi_errno_t __wasi_fd_seek32(__wasi_fd_t fd, int32_t offset, __wasi_whence_t whence, int32_t* newoffset)
    __attribute__((
        __import_module__("wasi_snapshot_preview1"),
        __import_name__("fd_seek32"),
        __warn_unused_result__));

__wasi_errno_t __wasi_fd_seek(__wasi_fd_t fd, __wasi_filedelta_t offset, __wasi_whence_t whence, __wasi_filesize_t* newoffset)
{
	int32_t newoffset32 = 0;
	__wasi_errno_t result = __wasi_fd_seek32(fd, int32_t(offset), whence, &newoffset32);
	*newoffset = newoffset32;
	return result;
}

extern "C" int system(const char* command)
{
	// WASI doesn't provide a system() equivalent; we highjack readlink here, the reasoning being that if we run against a real WASI implementation,
	// the effect is more likely to be benign.
	return __wasi_path_readlink(-1, command, strlen(command), 0, 0, 0);
}

#endif
