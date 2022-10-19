#include "file/file.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <str/str.h>
#include <string.h>
#include <sum/sum.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

typedef struct {
#ifdef _WIN32
    HANDLE handle;
#else
    int fd;
#endif
    bool valid;
} PlatformFile;

static PlatformFile platform_fopen(str path) {
#ifdef _WIN32
    HANDLE handle = CreateFileA(
        path.ptr,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    return (PlatformFile){
        .handle = handle,
        .valid = handle != INVALID_HANDLE_VALUE,
    };
#else
    int fd = open(path.ptr, O_RDONLY);
    return (PlatformFile){.fd = fd, .valid = fd != -1};
#endif
}

static void platform_fclose(PlatformFile file) {
    if (file.valid) {
#ifdef _WIN32
        CloseHandle(file.handle);
#else
        close(file.fd);
#endif
    }
}

static size_t platform_filelen(PlatformFile f) {
#ifdef _WIN32
    LARGE_INTEGER size;
    GetFileSizeEx(f.handle, &size);
    return (size_t)size.QuadPart;
#else
    struct stat st;
    fstat(f.fd, &st);
    return (size_t)st.st_size;
#endif
}

static size_t platform_fread(PlatformFile f, char* buf, size_t len) {
#ifdef _WIN32
    DWORD read;
    ReadFile(f.handle, buf, (DWORD)len, &read, NULL);
    return (size_t)read;
#else
    return (size_t)read(f.fd, buf, len);
#endif
}

SlurpFileResult slurp_file(str filename) {
    PlatformFile f = platform_fopen(filename);
    if (!f.valid) {
        str msg = str_printf("failed to open '" str_fmt "' for reading", str_arg(filename));
        return (SlurpFileResult)SUM_ERR(msg);
    }

    size_t len = platform_filelen(f);

    char* buf = malloc(len + 1);
    if (buf == NULL) {
        platform_fclose(f);
        str msg =
            str_printf("failed to allocate memory for '" str_fmt "' contents", str_arg(filename));
        return (SlurpFileResult)SUM_ERR(msg);
    }

    size_t read = platform_fread(f, buf, len);
    if (read != len) {
        platform_fclose(f);
        free(buf);
        str msg = str_printf("failed to read '" str_fmt "' contents", str_arg(filename));
        return (SlurpFileResult)SUM_ERR(msg);
    }

    buf[len] = '\0';

    platform_fclose(f);

    return (SlurpFileResult)SUM_OK(str_acquire_chars(buf, len));
}

typedef struct PlatformStatResult {
    bool valid;
#ifdef _WIN32
    DWORD attributes;
#else
    struct stat st;
#endif
} PlatformStatResult;

static PlatformStatResult platform_stat(str path) {
#ifdef _WIN32
    str path_zero = str_null;
    str_cpy(&path_zero, path);
    DWORD attributes = GetFileAttributesA(path_zero.ptr);
    str_free(path_zero);
    return (PlatformStatResult){
        .valid = attributes != INVALID_FILE_ATTRIBUTES,
        .attributes = attributes,
    };
#else
    struct stat st;
    str path_zero = str_null;
    str_cpy(&path_zero, path);
    int result = stat(path_zero.ptr, &st);
    str_free(path_zero);
    return (PlatformStatResult){
        .valid = result == 0,
        .st = st,
    };
#endif
}

bool file_exists(str filename) {
    return platform_stat(filename).valid;
}

bool file_isdir(str filename) {
    PlatformStatResult stat_result = platform_stat(filename);
    if (!stat_result.valid) {
        return false;
    }

#ifdef _WIN32
    return (stat_result.attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    return S_ISDIR(stat_result.st.st_mode);
#endif
}

typedef SUM_MAYBE_TYPE(str) PlatformMkdirResult;

static PlatformMkdirResult platform_mkdir(str path) {
#ifdef _WIN32
    BOOL result = CreateDirectoryA(path.ptr, NULL);
    if (result) {
        return (PlatformMkdirResult)SUM_NOTHING;
    }

    DWORD error = GetLastError();
    if (error == ERROR_ALREADY_EXISTS) {
        return (PlatformMkdirResult)SUM_NOTHING;
    }

    str msg = str_lit("intermediate directory does not exist");
    return (PlatformMkdirResult)SUM_JUST(msg);
#else
    int result = mkdir(path.ptr, 0777);
    if (result == 0) {
        return (PlatformMkdirResult)SUM_NOTHING;
    }

    if (errno == EEXIST) {
        return (PlatformMkdirResult)SUM_NOTHING;
    }

    if (errno == ENOENT) {
        str msg = str_lit("intermediate directory does not exist");
        return (PlatformMkdirResult)SUM_JUST(msg);
    }

    str msg = str_printf("%s", strerror(errno));
    return (PlatformMkdirResult)SUM_JUST(msg);
#endif
}

static str path_normalize(str path) {
    // works on both windows and unix-based OSes, because both can use '/' as
    // path separator
    char* result = malloc(path.len + 1);
    assert(result != NULL);
    for (size_t i = 0; i < path.len; i++) {
        if (path.ptr[i] == '\\') {
            result[i] = '/';
        } else {
            result[i] = path.ptr[i];
        }
    }
    result[path.len] = '\0';
    return str_acquire_chars(result, path.len);
}

FileMkdirRecError file_mkdir_rec(str path) {
    path = path_normalize(path);

    if (file_exists(path)) {
        if (file_isdir(path)) {
            str_free(path);
            return (FileMkdirRecError)SUM_NOTHING;
        }
        str msg = str_printf("'" str_fmt "' exists and is not a directory", str_arg(path));
        str_free(path);
        return (FileMkdirRecError)SUM_JUST(msg);
    }

    str_find_result last_slash = str_find_last(path, '/');
    if (!last_slash.found) {
        str_free(path);
        return (FileMkdirRecError)SUM_NOTHING;
    }

    str parent = str_upto(path, last_slash.pos);
    FileMkdirRecError parent_err = file_mkdir_rec(parent);
    if (parent_err.present) {
        str_free(path);
        return parent_err;
    }

    PlatformMkdirResult result = platform_mkdir(path);
    if (result.present) {
        str_free(path);
        return (FileMkdirRecError)SUM_JUST(result.value);
    }

    str_free(path);
    return (FileMkdirRecError)SUM_NOTHING;
}
