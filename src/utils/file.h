#ifndef __FILE__H__
#define __FILE__H__

#include "storage/bufpage.h"
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

typedef enum
{
	/* message for compatibility check */
	FIO_AGENT_VERSION, /* never move this */
	FIO_OPEN,
	FIO_CLOSE,
	FIO_WRITE,
	FIO_SYNC,
	FIO_RENAME,
	FIO_SYMLINK,
	FIO_REMOVE,
	FIO_MKDIR,
	FIO_CHMOD,
	FIO_SEEK,
	FIO_TRUNCATE,
	FIO_PREAD,
	FIO_READ,
	FIO_LOAD,
	FIO_STAT,
	FIO_SEND,
	FIO_ACCESS,
	FIO_OPENDIR,
	FIO_READDIR,
	FIO_CLOSEDIR,
	FIO_PAGE,
	FIO_WRITE_COMPRESSED_ASYNC,
	FIO_GET_CRC32,
	/* used for incremental restore */
	FIO_GET_CHECKSUM_MAP,
	FIO_GET_LSN_MAP,
	/* used in fio_send_pages */
	FIO_SEND_PAGES,
	FIO_ERROR,
	FIO_SEND_FILE,
//	FIO_CHUNK,
	FIO_SEND_FILE_EOF,
	FIO_SEND_FILE_CORRUPTION,
	FIO_SEND_FILE_HEADERS,
	/* messages for closing connection */
	FIO_DISCONNECT,
	FIO_DISCONNECTED,
	FIO_LIST_DIR,
	FIO_CHECK_POSTMASTER,
	FIO_GET_ASYNC_ERROR,
	FIO_WRITE_ASYNC,
	FIO_READLINK,
	FIO_PAGE_ZERO
} fio_operations;

typedef enum
{
	FIO_LOCAL_HOST,  /* data is locate at local host */
	FIO_DB_HOST,     /* data is located at Postgres server host */
	FIO_BACKUP_HOST, /* data is located at backup host */
	FIO_REMOTE_HOST  /* date is located at remote host */
} fio_location;

#define FIO_FDMAX 64
#define FIO_PIPE_MARKER 0x40000000

#define SYS_CHECK(cmd) do if ((cmd) < 0) { fprintf(stderr, "%s:%d: (%s) %s\n", __FILE__, __LINE__, #cmd, strerror(errno)); exit(EXIT_FAILURE); } while (0)
#define IO_CHECK(cmd, size) do { int _rc = (cmd); if (_rc != (size)) fio_error(_rc, size, __FILE__, __LINE__); } while (0)

typedef struct
{
//	fio_operations cop;
//	16
	/* fio operation, see fio_operations enum */
	unsigned cop    : 32;
	/* */
	unsigned handle : 32;
	/* size of additional data sent after this header */
	unsigned size   : 32;
	/* additional small parameter for requests (varies between operations) or a result code for response */
	unsigned arg;
} fio_header;

extern fio_location MyLocation;

/* Check if FILE handle is local or remote (created by FIO) */
#define fio_is_remote_file(file) ((size_t)(file) <= FIO_FDMAX)

extern void    fio_redirect(int in, int out, int err);
extern void    fio_communicate(int in, int out);
extern void    fio_disconnect(void);

extern void    fio_get_agent_version(int* protocol, char* payload_buf, size_t payload_buf_size);
extern void    fio_error(int rc, int size, const char* file, int line);

/* FILE-style functions */
extern FILE*   fio_fopen(fio_location location, const char* name, const char* mode);
extern size_t  fio_fwrite(FILE* f, void const* buf, size_t size);
extern ssize_t fio_fwrite_async_compressed(FILE* f, void const* buf, size_t size, int compress_alg);
extern size_t  fio_fwrite_async(FILE* f, void const* buf, size_t size);
extern int     fio_check_error_file(FILE* f, char **errmsg);
extern ssize_t fio_fread(FILE* f, void* buf, size_t size);
extern int     fio_pread(FILE* f, void* buf, off_t offs);
extern int     fio_fprintf(FILE* f, const char* arg, ...) pg_attribute_printf(2, 3);
extern int     fio_fflush(FILE* f);
extern int     fio_fseek(FILE* f, off_t offs);
extern int     fio_ftruncate(FILE* f, off_t size);
extern int     fio_fclose(FILE* f);
extern int     fio_ffstat(FILE* f, struct stat* st);

extern FILE*   fio_open_stream(fio_location location, const char* name);
extern int     fio_close_stream(FILE* f);

/* fd-style functions */
extern int     fio_open(fio_location location, const char* name, int mode);
extern ssize_t fio_write(int fd, void const* buf, size_t size);
extern ssize_t fio_write_async(int fd, void const* buf, size_t size);
extern int     fio_check_error_fd(int fd, char **errmsg);
extern int     fio_check_error_fd_gz(gzFile f, char **errmsg);
extern ssize_t fio_read(int fd, void* buf, size_t size);
extern int     fio_flush(int fd);
extern int     fio_seek(int fd, off_t offs);
extern int     fio_fstat(int fd, struct stat* st);
extern int     fio_truncate(int fd, off_t size);
extern int     fio_close(int fd);

/* DIR-style functions */
extern DIR*    fio_opendir(fio_location location, const char* path);
extern struct dirent * fio_readdir(DIR *dirp);
extern int     fio_closedir(DIR *dirp);

/* pathname-style functions */
extern int     fio_sync(fio_location location, const char* path);
extern pg_crc32 fio_get_crc32(fio_location location, const char *file_path,
							  bool decompress, bool missing_ok);
extern pg_crc32 fio_get_crc32_truncated(fio_location location, const char *file_path,
										bool missing_ok);

extern int     fio_rename(fio_location location, const char* old_path, const char* new_path);
extern int     fio_symlink(fio_location location, const char* target, const char* link_path, bool overwrite);
extern int     fio_remove(fio_location location, const char* path, bool missing_ok);
extern int     fio_mkdir(fio_location location, const char* path, int mode);
extern int     fio_chmod(fio_location location, const char* path, int mode);
extern int     fio_access(fio_location location, const char* path, int mode);
extern int     fio_stat(fio_location location, const char* path, struct stat* st, bool follow_symlinks);
extern bool    fio_is_same_file(fio_location location, const char* filename1, const char* filename2, bool follow_symlink);
extern ssize_t fio_readlink(fio_location location, const char *path, char *value, size_t valsiz);
extern pid_t   fio_check_postmaster(fio_location location, const char *pgdata);

/* gzFile-style functions */
#ifdef HAVE_LIBZ
extern gzFile  fio_gzopen(fio_location location, const char* path, const char* mode, int level);
extern int     fio_gzclose(gzFile file);
extern int     fio_gzread(gzFile f, void *buf, unsigned size);
extern int     fio_gzwrite(gzFile f, void const* buf, unsigned size);
extern int     fio_gzeof(gzFile f);
extern z_off_t fio_gzseek(gzFile f, z_off_t offset, int whence);
extern const char* fio_gzerror(gzFile file, int *errnum);
#endif

#endif
