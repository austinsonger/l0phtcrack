#include"stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>

#include"ntreg.h"

#define LOG(...)
#define DLOG(...)
#define LOAD_WD_LOGONLY(...)
#define strncaseeq(a,b,c) (strnicmp(a,b,c))
#define strcaseeq(a,b) (stricmp(a,b))
#define streq(a,b) (strcmp(a,b))
#define strneq(a,b,c) (strncmp(a,b,c))

#if ENABLE_DEBUG_PRINTF
#undef LOG
#define LOG(...) printf(__VA_ARGS__)
#undef DLOG
#define DLOG(...) printf(__VA_ARGS__)
#endif

/* Optimized non-terminated string copy
*
* Inline code that accepts a non-terminated string and a length and
* copies the string, null-terminating it. Attempts to use word-size
* operations when possible.
*/
static inline void str_memcpy(char *dest, const char *src, int len)
{
	unsigned long *ldest = (unsigned long *)dest;
	const unsigned long *lsrc = (unsigned long *)src;
	int remain = len;

	/* Alignment check - don't use word-sized copy for unaligned objects */
	if ((uintptr_t)dest & (sizeof(unsigned long) - 1)) goto bytewise_copy;
	if ((uintptr_t)src & (sizeof(unsigned long) - 1)) goto bytewise_copy;

	/* Word copy */
	while (remain > (int)sizeof(unsigned long)) {
		*ldest = *lsrc;
		ldest++; lsrc++;
		remain -= sizeof(unsigned long);
	}

bytewise_copy:
	dest = (char *)ldest;
	src = (char *)lsrc;
	while (remain > 0) {
		*dest = *src;
		dest++; src++;
		remain--;
	}
	*dest = '\0';
}

const char *val_types[REG_MAX + 1] = {
	"REG_NONE", "REG_SZ", "REG_EXPAND_SZ", "REG_BINARY", "REG_DWORD",       /* 0 - 4 */
	"REG_DWORD_BIG_ENDIAN", "REG_LINK",                                     /* 5 - 6 */
	"REG_MULTI_SZ", "REG_RESOUCE_LIST", "REG_FULL_RES_DESC", "REG_RES_REQ", /* 7 - 10 */
	"REG_QWORD",                                                            /* 11     */
};

static char *string_prog2regw(void *string, int len, int* out_len);

/* Utility routines */

/* toupper() table for registry hashing functions, so we don't have to
* dependent upon external locale lib files
*/

static const unsigned char reg_touppertable[] = {

	/* ISO 8859-1 is probably not the one.. */

	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, /* 0x00-0x07 */
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, /* 0x08-0x0f */
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, /* 0x10-0x17 */
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, /* 0x18-0x1f */
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, /* 0x20-0x27 */
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, /* 0x28-0x2f */
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, /* 0x30-0x37 */
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, /* 0x38-0x3f */
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, /* 0x40-0x47 */
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, /* 0x48-0x4f */
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, /* 0x50-0x57 */
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, /* 0x58-0x5f */
	0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, /* 0x60-0x67 */
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, /* 0x68-0x6f */
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, /* 0x70-0x77 */
	0x58, 0x59, 0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, /* 0x78-0x7f */

	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, /* 0x80-0x87 */
	0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, /* 0x88-0x8f */
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, /* 0x90-0x97 */
	0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, /* 0x98-0x9f */
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, /* 0xa0-0xa7 */
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, /* 0xa8-0xaf */
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0x00, 0xb6, 0xb7, /* 0xb0-0xb7 */
	0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, /* 0xb8-0xbf */
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, /* 0xc0-0xc7 */
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, /* 0xc8-0xcf */
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, /* 0xd0-0xd7 */
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, /* 0xd8-0xdf */
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, /* 0xe0-0xe7 */
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, /* 0xe8-0xef */
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xf7, /* 0xf0-0xf7 */
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0x00, /* 0xf8-0xff */

};


/* Use table above in strcasecmp else add_key may put names in wrong order
and windows actually verifies that on hive load!!
or at least it finds out in some cases..
*/


static int strn_casecmp(const char *s1, const char *s2, size_t n)
{
	char r;

	while (*s1 && *s2 && n) {
		r = (unsigned char)reg_touppertable[(unsigned char)*s1] - (unsigned char)reg_touppertable[(unsigned char)*s2];
		if (r) return r;
		n--;
		s1++;
		s2++;
	}
	if ((!*s1 && !*s2) || !n) return 0;
	if (!*s1) return -1;
	return 1;
}


static inline char *str_dup(const char * const  str)
{
	char *str_new;
	int len;

	if (!str) return 0;

	len = strlen(str) + 1;
	str_new = (char *)malloc(len);
	strncpy(str_new, str, len);
	return str_new;
}


/* Get INTEGER from memory. This is probably low-endian specific? */
/* This is totally unnecessary on i386/x86_64, so macro it out! */
#define get_int32(a) *(int32_t *)(a)
#if 0
static inline int get_int32(char *array)
{
	return ((array[0] & 0xff) + ((array[1] << 8) & 0xff00) +
		((array[2] << 16) & 0xff0000) +
		((array[3] << 24) & 0xff000000));
}
#endif


/* Flush pages marked as dirty to the hive file */
int flush_dirty_pages(struct hive *hdesc)
{
	int i = 0;
	int offset;
	int ret;

	LOAD_WD_LOGONLY();

	while (i < hdesc->dirty_entries) {
		offset = hdesc->dirty[i] << 12;
		fseek(hdesc->fp, offset, SEEK_SET);
		ret = fwrite(hdesc->buffer + offset, 0x1000, 1, hdesc->fp);
		if (!ret) goto write_error;
		i++;
	}

	hdesc->dirty_entries = 0;
	return 0;

write_error:
	LOG("error: flush_dirty_pages: write failed @ offset 0x%x\n", offset);
	return -1;
}


/* Mark pages as dirty for writing */
int mark_pages_dirty(struct hive *hdesc, int start, int end)
{
	int startpage;
	int endpage;
	int i;

	LOAD_WD_LOGONLY();

	startpage = (start >> 12);
	endpage = (end >> 12);

	if ((startpage < 0) || (startpage > hdesc->pages)) goto bad_start;
	if ((endpage < 0) || (endpage > hdesc->pages)) goto bad_end;

	/* Swap start and end if they are backwards */
	if (startpage > endpage) {
		i = endpage;
		endpage = startpage;
		startpage = i;
	}

	/* Add each page to the dirty list */
	while (startpage <= endpage) {
		i = 0;
		if (hdesc->dirty_entries >= MAX_DIRTY) {
			i = flush_dirty_pages(hdesc);
			if (i) return i;
		}
		while (i < hdesc->dirty_entries) {
			/* Don't add pages already present */
			if (hdesc->dirty[i] == startpage) {
				i = -1; /* needed for 'continue' later */
				startpage++;
				break;
			}
			i++;
		}

		/* Skip pages already present */
		if (i < 0) continue;

		/* Store this page number in the dirty list */
		hdesc->dirty_entries++;
		hdesc->dirty[i] = startpage;
		startpage++;
	}

	return 0;
bad_start:
	LOG("error: mark_pages_dirty: bad start page %d (0x%x)\n", startpage, start);
	return -1;
bad_end:
	LOG("error: mark_pages_dirty: bad end page %d (0x%x)\n", endpage, end);
	return -1;
}


/* Parse the datablock
* vofs = offset into struct (after size linkage)
*/

inline int parse_block(struct hive *hdesc, int vofs)
{
	/*  unsigned short id; */
	int seglen;

	LOAD_WD_LOGONLY();

	seglen = get_int32(hdesc->buffer + vofs);

	if (seglen == 0) {
		LOG("parse_block: Zero data block size (not registry or corrupt file?)\n");
		return 0;
	}

	if (seglen < 0) {
		seglen = -seglen;
		hdesc->usetot += seglen;
		hdesc->useblk++;
	}
	else {
		hdesc->unusetot += seglen;
		hdesc->unuseblk++;
	}
	return seglen;
}

/* ================================================================ */
/* Scan and allocation routines */

/* Find start of page given a current pointer into the buffer
* hdesc = hive
* vofs = offset pointer into buffer
* returns: offset to start of page (and page header)
*/

int find_page_start(struct hive * const hdesc, const int vofs)
{
	int r, prev;
	struct hbin_page *h;

	LOAD_WD_LOGONLY();

	/* Again, assume start at 0x1000 */

	r = 0x1000;
	while (r < hdesc->size) {
		prev = r;
		h = (struct hbin_page *)(hdesc->buffer + r);
		if (h->id != 0x6E696268) return 0;
		if (h->ofs_next == 0) {
			LOG("find_page_start: zero len or ofs_next found in page at 0x%x\n", r);
			return 0;
		}
		r += h->ofs_next;
		if (r > vofs) return prev;
	}
	return 0;
}

/* Find free space in page
* size = requested size in bytes
* pofs = offset to start of actual page header
* returns: offset to free block, or 0 for error
*/

int find_free_blk(struct hive * const hdesc, const int pofs, const int size)
{
	int vofs = pofs + 0x20;
	int seglen;
	struct hbin_page *p;

	LOAD_WD_LOGONLY();

	p = (struct hbin_page *)(hdesc->buffer + pofs);
	while ((vofs - pofs) < (p->ofs_next - HBIN_ENDFILL)) {
		seglen = get_int32(hdesc->buffer + vofs);
		if (seglen == 0) {
			LOG("find_free_blk: Zero data block size; block at offset %0x\n", vofs);
			if ((vofs - pofs) == (p->ofs_next - 4)) {
				LOG("find_free_blk: at exact end of hbin, do not care.\n");
				return 0;
			}
			return 0;
		}

		if (seglen < 0) {
			seglen = -seglen;
		}
		else {
			if (seglen >= size) {
				return vofs;
			}
		}
		vofs += seglen;
	}
	return 0;
}

/* Search pages from start to find free block
* hdesc - hive
* size - space requested, in bytes
* returns: offset to free block, 0 if not found or error
*/

int find_free(struct hive * const hdesc, int size)
{
	int r, blk;
	struct hbin_page *h;

	LOAD_WD_LOGONLY();

	/* Align to 8 byte boundary */
	if (size & 7) size += (8 - (size & 7));

	/* Again, assume start at 0x1000 */
	r = 0x1000;
	while (r < hdesc->endofs) {
		h = (struct hbin_page *)(hdesc->buffer + r);
		if (h->id != 0x6E696268) {
			LOG("find_free: Hive block not of type HBIN at %p", (hdesc->buffer + r));
			return 0;
		}
		if (h->ofs_next == 0) {
			LOG("find_free: zero len or ofs_next found in page at 0x%x\n", r);
			return 0;
		}
		blk = find_free_blk(hdesc, r, size);
		if (blk) return blk;
		r += h->ofs_next;
	}
	LOG("find_free: No free block of size %d found\n", size);
	return 0;
}

/* Add new hbin to end of file. If file contains data at end
* that is not in a hbin, include that too
* hdesc - hive as usual
* size - minimum size (will be rounded up to next 0x1000 alignment)
* returns offset to first block in new hbin
*/

int add_bin(struct hive * const hdesc, int size)
{
	int r, newsize, newbinofs;
	struct hbin_page *newbin;
	struct regf_header *hdr;

	LOAD_WD_LOGONLY();

	if (hdesc->state & HMODE_NOEXPAND) goto error_noexpand;

	/* Add header and link, round up to page boundary, usually 0x1000 */
	r = ((size + 0x20 + 4) & ~0xfff) + HBIN_PAGESIZE;

	newbinofs = hdesc->endofs;

	/* We must allocate more buffer */
	if ((newbinofs + r) >= hdesc->size) {
		/* File normally multiple of 0x40000 bytes */
		newsize = ((newbinofs + r) & ~(REGF_FILEDIVISOR - 1)) + REGF_FILEDIVISOR;

		hdesc->buffer = (char *)realloc(hdesc->buffer, newsize);
		if (!hdesc->buffer) goto error_realloc;
		/* New allocated space needs to be written to disk */
		hdesc->pages = newsize >> 12;
		if (mark_pages_dirty(hdesc, hdesc->size, newsize)) goto error_dirty;
		hdesc->size = newsize;
	}

	/* At this point, we have large enough space at end of file */
	newbin = (struct hbin_page *)(hdesc->buffer + newbinofs);

	memset((void *)newbin, 0, r); /* zero out new hbin, easier to debug too */

	newbin->id = 0x6E696268; /* 'hbin' */
	newbin->ofs_self = newbinofs - 0x1000;     /* Point to ourselves minus regf. Seem to be that.. */
	newbin->ofs_next = r;                  /* size of this new bin */

	/* Wonder if anything else in the hbin header matters? */

	/* Set whole hbin to be one contious unused block */
	newbin->firstlink = (r - 0x20 - 0);  /* Positive linkage = unused */

	/* Update REGF header */
	hdr = (struct regf_header *) hdesc->buffer;
	hdr->filesize = newbinofs + r - 0x1000;               /* Point header to new end of data */

	/* Update state */
	if (mark_pages_dirty(hdesc, 0, 0)) goto error_dirty;
	if (mark_pages_dirty(hdesc, newbinofs, newbinofs)) goto error_dirty;
	hdesc->state |= HMODE_DIDEXPAND;
	hdesc->lastbin = newbinofs;  /* Last bin */
	hdesc->endofs = newbinofs + r;   /* New data end */

	return (newbinofs + 0x20);

error_noexpand:
	LOG("add_bin: %s cannot be expanded as required (NOEXPAND is set)\n", hdesc->filename);
	return 0;
error_realloc:
	LOG("add_bin: realloc to size %d failed\n", newsize);
	return 0;
error_dirty:
	LOG("add_bin: mark_pages_dirty returned an error\n");
	return -1;
}


/* Allocate a block of requested size if possible
* hdesc - hive
* pofs - If >0 will try this page first (ptr may be inside page)
* size - number of bytes to allocate
* returns: 0 - failed, else pointer to allocated block.
* WARNING: Will realloc() buffer if it has to be expanded!
* ALL POINTERS TO BUFFER IS INVALID AFTER THAT. (offsets are still correct)
* Guess I'd better switch to mmap() one day..
* This function WILL CHANGE THE HIVE (change block linkage) if it
* succeeds.
*/

int alloc_block(struct hive *hdesc, int ofs, int size)
{
	int pofs = 0;
	int blk = 0;
	int newbin;
	int trail, trailsize, oldsz;

	LOAD_WD_LOGONLY();

	if (hdesc->state & HMODE_NOALLOC) {
		LOG("\nalloc_block: Hive <%s> is in no allocation safe mode,"
			"new space not allocated. Operation will fail!\n", hdesc->filename);
		return 0;
	}

	size += 4;  /* Add linkage */
	if (size & 7) size += (8 - (size & 7));

	/* Check current page first */
	if (ofs) {
		pofs = find_page_start(hdesc, ofs);
		blk = find_free_blk(hdesc, pofs, size);
	}

	/* Then check whole hive */
	if (!blk) {
		blk = find_free(hdesc, size);
	}

	if (blk) {  /* Got the space */
		oldsz = get_int32(hdesc->buffer + blk);
		trailsize = oldsz - size;

		if (trailsize == 4) {
			trailsize = 0;
			size += 4;
		}
		if (trailsize & 7) { /* Trail must be 8 aligned */
			trailsize -= (8 - (trailsize & 7));
			size += (8 - (trailsize & 7));
		}
		if (trailsize == 4) {
			trailsize = 0;
			size += 4;
		}

		/* Now change pointers on this to reflect new size */
		*(int *)((hdesc->buffer) + blk) = -(size);
		if (mark_pages_dirty(hdesc, blk, blk + size)) return -1;
		/* If the fit was exact (unused block was same size as wee need)
		* there is no need for more, else make free block after end
		* of newly allocated one */

		hdesc->useblk++;
		hdesc->unuseblk--;
		hdesc->usetot += size;
		hdesc->unusetot -= size;

		if (trailsize) {
			trail = blk + size;

			*(int *)((hdesc->buffer) + trail) = (int)trailsize;
			if (mark_pages_dirty(hdesc, trail, trail)) return -1;

			hdesc->useblk++;    /* This will keep blockcount */
			hdesc->unuseblk--;
			hdesc->usetot += 4; /* But account for more linkage bytes */
			hdesc->unusetot -= 4;

		}

		return blk;
	}
	else {
		LOG("alloc_block: failed to alloc %d bytes, trying to expand hive.\n", size);

		newbin = add_bin(hdesc, size);
		if (newbin) return alloc_block(hdesc, newbin, size); /* Nasty... recall ourselves. */
		/* Fallthrough to fail if add_bin fails */
	}
	LOG("alloc_block: failed to expand hive");
	return 0;
}

/* Free a block in registry
* hdesc - hive
* blk   - offset of block, MUST POINT TO THE LINKAGE!
* returns bytes freed (incl linkage bytes) or 0 if fail
* Will CHANGE HIVE IF SUCCESSFUL (changes linkage)
*/

int free_block(struct hive * const hdesc, int blk)
{
	int pofs, vofs, seglen, next, nextsz, prevsz, size;
	int prev = 0;
	struct hbin_page *p;

	LOAD_WD_LOGONLY();

	if (hdesc->state & HMODE_NOALLOC) {
		LOG("free_block: %s cannot free blocks as needed (NOALLOC set)\n", hdesc->filename);
		return 0;
	}

	size = get_int32(hdesc->buffer + blk);
	if (size >= 0) {
		LOG("free_block: trying to free already free block: %x\n", blk);
		return 0;
	}
	size = -size;

	/* So, we must find start of the block BEFORE us */
	pofs = find_page_start(hdesc, blk);
	if (!pofs) {
		LOG("free_block: find_page_start failed: offset %d\n", blk);
		return 0;
	}

	p = (struct hbin_page *)(hdesc->buffer + pofs);
	vofs = pofs + 0x20;
	prevsz = -32;

	if (vofs != blk) {  /* Block is not at start of page? */
		while ((vofs - pofs) < (p->ofs_next - HBIN_ENDFILL)) {

			seglen = get_int32(hdesc->buffer + vofs);
			if (seglen == 0) {
				LOG("free_block: Zero data block size (not registry or corrupt file?)\n");
				return 0;
			}
			if (seglen < 0) {
				seglen = -seglen;
			}
			prev = vofs;
			vofs += seglen;
			if (vofs == blk) break;
		}

		if (vofs != blk) {
			LOG("free_block: Ran off end of page. Error in chains? vofs %x, pofs %x, blk %x\n", vofs, pofs, blk);
			return 0;
		}
		prevsz = get_int32(hdesc->buffer + prev);
	}

	/* We also need details on next block (unless at end of page) */
	next = blk + size;

	nextsz = 0;
	if ((next - pofs) < (p->ofs_next - HBIN_ENDFILL)) nextsz = get_int32(hdesc->buffer + next);

	/* Now check if next block is free, if so merge it with the one to be freed */
	if (nextsz > 0) {
		size += nextsz;   /* Swallow it in current block */
		hdesc->useblk--;
		hdesc->usetot -= 4;
		hdesc->unusetot -= 4;   /* FIXME !??!?? */
	}

	/* Now free the block (possibly with ajusted size as above) */
	*(int *)((hdesc->buffer) + blk) = (int)size;
	hdesc->usetot -= size;
	hdesc->unusetot -= size;  /* FIXME !?!? */
	hdesc->unuseblk--;

	if (mark_pages_dirty(hdesc, blk, blk)) return -1;

	/* Check if previous block is also free, if so, merge.. */
	if (prevsz > 0) {
		hdesc->usetot -= prevsz;
		hdesc->unusetot += prevsz;
		prevsz += size;
		/* And swallow current.. */
		*(int *)((hdesc->buffer) + prev) = (int)prevsz;
		if (mark_pages_dirty(hdesc, prev, prev)) return -1;
		hdesc->useblk--;
		return prevsz;
	}
	return size;
}


/* ================================================================ */

/* ** Registry manipulation routines ** */

/* "directory scan", return next name/pointer of a subkey on each call
* nkofs = offset to directory to scan
* lfofs = pointer to int to hold the current scan position,
*         set position to 0 to start.
* sptr  = pointer to struct to hold a single result
* returns: -1 = error. 0 = end of key. 1 = more subkeys to scan
* NOTE: caller must free the name-buffer (struct ex_data *name)
*/
int ex_next_n(const struct hive * const hdesc, int nkofs,
	int * const  count,
	int * const  countri,
struct ex_data * const  sptr)
{
	struct nk_key *key, *newnkkey;
	int newnkofs;
	struct lf_key *lfkey;
	struct li_key *likey;
	struct ri_key *rikey;

	LOAD_WD_LOGONLY();

	if (!nkofs) return -1;
	if (!sptr) return -1;
	key = (struct nk_key *)(hdesc->buffer + nkofs);
	if (key->id != 0x6b6e) goto error_not_nk;

	lfkey = (struct lf_key *)(hdesc->buffer + key->ofs_lf + 0x1004);
	rikey = (struct ri_key *)(hdesc->buffer + key->ofs_lf + 0x1004);
	if (hdesc->size < ((intptr_t)lfkey - (intptr_t)hdesc->buffer)) goto error_lf_eof;
	if (hdesc->size < ((intptr_t)rikey - (intptr_t)hdesc->buffer)) goto error_ri_eof;

	if (rikey->id == 0x6972) {   /* Is it extended 'ri'-block? */
		if (*countri < 0 || *countri >= rikey->no_lis) { /* End of ri's? */
			return 0;
		}
		/* Get the li of lf-struct that's current based on countri */
		likey = (struct li_key *)(hdesc->buffer + rikey->hash[*countri].ofs_li + 0x1004);
		if (hdesc->size < ((intptr_t)likey - (intptr_t)hdesc->buffer)) goto error_li_eof;
		if (likey->id == 0x696c) {
			newnkofs = likey->hash[*count].ofs_nk + 0x1000;
		}
		else {
			lfkey = (struct lf_key *)(hdesc->buffer + rikey->hash[*countri].ofs_li + 0x1004);
			newnkofs = lfkey->h.hash[*count].ofs_nk + 0x1000;
			if (hdesc->size < newnkofs) goto error_nk_eof;
		}

		/* Check if current li/lf is exhausted */
		if (*count >= likey->no_keys - 1) { /* Last legal entry in li list? */
			(*countri)++;  /* Bump up ri count so we take next ri entry next time */
			(*count) = -1;  /* Reset li traverse counter for next round, not used later here */
		}
	}
	else { /* Plain handler */
		if (key->no_subkeys <= 0 || *count >= key->no_subkeys) return 0;
		if (lfkey->id == 0x696c) {   /* Is it 3.x 'li' instead? */
			likey = (struct li_key *)(hdesc->buffer + key->ofs_lf + 0x1004);
			newnkofs = likey->hash[*count].ofs_nk + 0x1000;
		}
		else {
			newnkofs = lfkey->h.hash[*count].ofs_nk + 0x1000;
		}
	}

	if (hdesc->size < newnkofs) goto error_nk_eof;
	sptr->nkoffs = newnkofs;
	newnkkey = (struct nk_key *)(hdesc->buffer + newnkofs + 4);
	sptr->nk = newnkkey;

	if (newnkkey->id != 0x6b6e) goto error_not_nk2;
	else {
		if (newnkkey->len_name <= 0) {
			LOG("ex_next_n: nk at 0x%0x has no name\n", newnkofs);
		}
		else if (newnkkey->type & 0x20) {
			str_memcpy(sptr->name, newnkkey->keyname, newnkkey->len_name);
		}
		else {
			string_regw2prog(sptr->name, newnkkey->keyname, newnkkey->len_name);
		}
	}

	(*count)++;
	return 1;
	/*  return( *count <= key->no_subkeys); */

error_not_nk:
	LOG("ex_next_n: error: Not a 'nk' node at 0x%0x\n", nkofs);
	return -1;
error_not_nk2:
	LOG("ex_next_n: error: Not a 'nk' node at 0x%0x\n", newnkofs);
	return -1;
error_lf_eof:
	LOG("ex_next_n: error: 'lf' key pointer beyond end of file");
	return -1;
error_ri_eof:
	LOG("ex_next_n: error: 'ri' key pointer beyond end of file");
	return -1;
error_li_eof:
	LOG("ex_next_n: error: 'li' key pointer beyond end of file");
	return -1;
error_nk_eof:
	LOG("ex_next_n: error: new 'nk' pointer beyond end of file\n");
	return -1;
}

/* "directory scan" for VALUES, return next name/pointer of a value on each call
* nkofs = offset to directory to scan
* lfofs = pointer to int to hold the current scan position,
*         set position to 0 to start.
* sptr  = pointer to struct to hold a single result
* returns: -1 = error. 0 = end of key. 1 = more values to scan
* NOTE: caller must free the name-buffer (struct vex_data *name)
*/
int ex_next_v(struct hive * const hdesc, int nkofs,
	int * const  count,
struct vex_data * const  sptr)
{
	struct nk_key *key;
	int32_t vkofs, vlistofs;
	int32_t *vlistkey;
	struct vk_key *vkkey;

	LOAD_WD_LOGONLY();

	if (!nkofs) goto error_nkofs_null;
	if (!sptr) goto error_sptr_null;
	if ((int)(nkofs + sizeof(struct nk_key)) > hdesc->size) goto error_nk_eof;

	key = (struct nk_key *)(hdesc->buffer + nkofs);

	if (key->id != 0x6b6e) goto error_not_nk;

	if (key->no_values <= 0 || *count >= key->no_values) return 0;

	vlistofs = key->ofs_vallist + 0x1004;
	vlistkey = (int32_t *)(hdesc->buffer + vlistofs);
	if (hdesc->size < vlistofs) goto error_vlistofs_size;

	vkofs = vlistkey[*count] + 0x1004;
	if ((int)(vkofs + sizeof(struct vk_key)) > hdesc->size) goto error_key_offset_size;
	vkkey = (struct vk_key *)(hdesc->buffer + vkofs);
	if (vkkey->id != 0x6b76) goto error_not_vk;

	sptr->vk = vkkey;
	sptr->vkoffs = vkofs;
	sptr->name[0] = '\0';
	sptr->size = (vkkey->len_data & 0x7fffffff);

	if (vkkey->len_name >0) {
		if (vkkey->flag & 1) {
			str_memcpy(sptr->name, vkkey->keyname, vkkey->len_name);
		}
		else {
			string_regw2prog(sptr->name, vkkey->keyname, vkkey->len_name);
		}
	}
	else {
		strncpy(sptr->name, "", 2);
	}

	sptr->type = vkkey->val_type;

	if (sptr->size) {
		if (vkkey->val_type == REG_DWORD) {
			if (vkkey->len_data & 0x80000000) {
				sptr->val = (int)(vkkey->ofs_data);
			}
		}
	}

	(*count)++;
	return (*count <= key->no_values);

error_nkofs_null:
	LOG("ex_next_v: nkofs is NULL\n");
	return -1;
error_sptr_null:
	LOG("ex_next_v: sptr is NULL\n");
	return -1;
error_nk_eof:
	LOG("ex_next_v: 'nk' offset beyond end of file\n");
	return -1;
error_not_nk:
	LOG("ex_next_v: not a 'nk' node at 0x%0x\n", nkofs);
	return -1;
error_vlistofs_size:
	LOG("ex_next_v: value list offset too large\n");
	return -1;
error_key_offset_size:
	LOG("ex_next_v: value key offset too large\n");
	return -1;
error_not_vk:
	LOG("ex_next_v: hit non-VK node during scan at offset 0x%0x\n", vkofs);
	return -1;
}

/* traceback - trace nk's back to root,
* building path string as we go.
* nkofs  = offset to nk-node
* path   = pointer to pathstring-buffer
* maxlen = max length of path-buffer
* return: length of path string
*/

int get_abs_path(struct hive *hdesc, int nkofs, char *path, int maxlen)
{
	/* int newnkofs; */
	struct nk_key *key;
	char tmp[ABSPATHLEN + 1];
	char keyname[ABSPATHLEN];
	int len_name;

	LOAD_WD_LOGONLY();

	maxlen = (maxlen < ABSPATHLEN ? maxlen : ABSPATHLEN);
	key = (struct nk_key *)(hdesc->buffer + nkofs);

	if (key->id != 0x6b6e) {
		LOG("get_abs_path: Not a 'nk' node\n");
		return 0;
	}

	if (key->type == KEY_ROOT) {   /* We're at the root */
		return strlen(path);
	}

	strncpy(tmp, path, ABSPATHLEN - 1);

	if (key->type & 0x20) {
		str_memcpy(keyname, key->keyname, key->len_name);
	}
	else {
		string_regw2prog(keyname, key->keyname, key->len_name);
	}
	len_name = strlen(keyname);
	if ((int)(strlen(path) + len_name) >= maxlen - 6) {
		_snprintf(path, maxlen, "(...)%s", tmp);
		return strlen(path);   /* Stop trace when string exhausted */
	}
	*path = '\\';
	memcpy(path + 1, keyname, len_name);
	strncpy(path + len_name + 1, tmp, maxlen - 6 - len_name);
	return get_abs_path(hdesc, key->ofs_parent + 0x1004, path, maxlen); /* go back one more */
}


/* Value index table lookup
* hdesc - hive as usual
* vlistofs - offset of table
* name - value name to look for
* returns index into table or -1 if err
*/

static int vlist_find(const struct hive * const  hdesc,
	int vlistofs, int numval,
	const char * const  name, int type)
{
	struct vk_key *vkkey;
	int i, vkofs, len;
	int32_t *vlistkey;
	int approx = -1;

	len = strlen(name);
	vlistkey = (int32_t *)(hdesc->buffer + vlistofs);

	for (i = 0; i < numval; i++) {
		vkofs = vlistkey[i] + 0x1004;
		vkkey = (struct vk_key *)(hdesc->buffer + vkofs);
		if (vkkey->len_name == 0 && *name == '@' && len == 1) { /* @ is alias for nameless value */
			return i;
		}

		if ((type & TPF_EXACT) && vkkey->len_name != len) continue;  /* Skip if exact match and not exact size */

		if (vkkey->len_name >= len) {                  /* Only check for names that are longer or equal than we seek */
			if (!strncaseeq(name, vkkey->keyname, len)) {    /* Name match */  /* XXX: winregfs */
				if (vkkey->len_name == len) return i;        /* Exact match always best, returns */
				if (approx == -1) approx = i;                 /* Else remember first partial match */
			}
		}
	}
	return approx;
}

#if 0
/* Recursevely follow 'nk'-nodes based on a path-string,
* returning offset of last 'nk' or 'vk'
* vofs - offset to start node
* path - null-terminated pathname (relative to vofs, \ is separator)
* type - type to return TPF_??, see ntreg.h
* return: offset to nk or vk (or NULL if not found)
*/

int trav_path(struct hive * const hdesc, int vofs,
	const char *  path, int type)
{
	const struct nk_key *key, *newnkkey;
	const struct lf_key *lfkey;
	const struct li_key *likey;
	const struct ri_key *rikey;

	const int32_t *vlistkey;
	int newnkofs, i, lfofs, vlistofs, r, ricnt, subs;
	int plen = 0;
	int adjust = 0;
	char *buf;
	char part[ABSPATHLEN + 1];
	char *partptr = NULL;

	char *partw = NULL;
	int partw_len, part_len;
	int cmp;

	LOAD_WD_LOGONLY();

	if (!hdesc) {
		LOG("trav_path: hive pointer is NULL\n");
		return 0;
	}
	buf = hdesc->buffer;

	if (!vofs) vofs = hdesc->rootofs + 4;     /* No current key given, so start at root */

	if (!(type & TPF_ABS) && *path == '\\' && *(path + 1) != '\\') {      /* Start from root if path starts with \ */
		path++;
		vofs = hdesc->rootofs + 4;
	}

	key = (struct nk_key *)(buf + vofs);
	if (key->id != 0x6b6e) {
		LOG("trav_path: Not a 'nk' node\n");
		return 0;
	}

	if (!(type & TPF_ABS)) {  /* Only traverse path if not absolute literal value name passed */

		/* TODO: Need to rethink this.. */

		/* Find \ delimiter or end of string, copying to name part buffer as we go,
		rewriting double \\s */
		partptr = part;
		for (plen = 0; path[plen] && (path[plen] != '\\' || path[plen + 1] == '\\'); plen++) {
			if (path[plen] == '\\' && path[plen + 1] == '\\') plen++; /* Skip one if double */
			*partptr++ = path[plen];
		}
		*partptr = '\0';
		if (!plen) return (vofs - 4);     /* Path has no length - we're there! */

		adjust = (path[plen] == '\\') ? 1 : 0;

		if ((plen == 1) && (*(path + 1) && *path == '.') && !(type & TPF_EXACT)) {     /* Handle '.' current dir */
			return trav_path(hdesc, vofs, path + plen + adjust, type);
		}
		if (!(type & TPF_EXACT) && (plen == 2) && !strcaseeq("..", path)) { /* Get parent key */
			newnkofs = key->ofs_parent + 0x1004;
			/* Return parent (or only root if at the root) */
			return trav_path(hdesc, (key->type == KEY_ROOT ? vofs : newnkofs), path + plen + adjust, type);
		}
	}

	/* at last name of path, and we want vk, and the nk has values */
	if ((type & TPF_VK_ABS) || (!path[plen] && (type & TPF_VK) && key->no_values)) {
		if (type & TPF_ABS) {
			strcpy(part, path);
			plen = de_escape(part, 0);
			partptr = part + plen;
		}

		vlistofs = key->ofs_vallist + 0x1004;
		vlistkey = (int32_t *)(buf + vlistofs);
		i = vlist_find(hdesc, vlistofs, key->no_values, part, type);
		if (i != -1) {
			return (vlistkey[i] + 0x1000);
		}
	}

	if (key->no_subkeys > 0) {    /* If it has subkeys, loop through the hash */
		lfofs = key->ofs_lf + 0x1004;    /* lf (hash) record */
		lfkey = (struct lf_key *)(buf + lfofs);

		if (lfkey->id == 0x6972) { /* ri struct need special parsing */
			/* Prime loop state */

			rikey = (struct ri_key *)lfkey;
			ricnt = rikey->no_lis;
			r = 0;
			likey = (struct li_key *)(hdesc->buffer + rikey->hash[r].ofs_li + 0x1004);
			subs = likey->no_keys;
			if (likey->id != 0x696c) {  /* Bwah, not li anyway, XP uses lh usually which is actually smarter */
				lfkey = (struct lf_key *)(hdesc->buffer + rikey->hash[r].ofs_li + 0x1004);
				likey = NULL;
			}
		}
		else {
			if (lfkey->id == 0x696c) { /* li? */
				likey = (struct li_key *)(buf + lfofs);
			}
			else {
				likey = NULL;
			}
			rikey = NULL;
			ricnt = 0; r = 0; subs = key->no_subkeys;
		}

		partw = string_prog2regw(part, partptr - part, &partw_len);
		part_len = strlen(part);
		do {
			for (i = 0; i < subs; i++) {
				if (likey) newnkofs = likey->hash[i].ofs_nk + 0x1004;
				else newnkofs = lfkey->h.hash[i].ofs_nk + 0x1004;
				newnkkey = (struct nk_key *)(buf + newnkofs);
				if (newnkkey->id != 0x6b6e) {
					LOG("trav_path: error: not a 'nk' node\n");
				}
				else {
					if (newnkkey->len_name <= 0) {
						LOG("trav_path: warning: no name\n");
					}
					else if (
						((part_len <= newnkkey->len_name) && !(type & TPF_EXACT)) ||
						((part_len == newnkkey->len_name) && (type & TPF_EXACT))) {
						/* Can't match if name is shorter than we look for */
						if (newnkkey->type & 0x20)
							cmp = strncaseeq(part, newnkkey->keyname, part_len);
						else
							cmp = memcmp(partw, newnkkey->keyname, partw_len);
						if (!cmp) {
							free(partw);
							return trav_path(hdesc, newnkofs, path + plen + adjust, type);
						}
					}
				} /* if id OK */
			} /* hash loop */
			r++;
			if (ricnt && r < ricnt) {
				newnkofs = rikey->hash[r].ofs_li;
				likey = (struct li_key *)(hdesc->buffer + newnkofs + 0x1004);
				subs = likey->no_keys;
				if (likey->id != 0x696c) {  /* Bwah, not li anyway, XP uses lh usually which is actually smarter */
					lfkey = (struct lf_key *)(hdesc->buffer + rikey->hash[r].ofs_li + 0x1004);
					likey = NULL;
				}
			}
		} while (r < ricnt && ricnt);
		free(partw);

	} /* if subkeys */

	LOG("trav_path: not found: %s\n", path);
	return 0;
}

#endif


/* Recursevely follow 'nk'-nodes based on a path-string,
* returning offset of last 'nk' or 'vk'
* vofs - offset to start node
* path - null-terminated pathname (relative to vofs, \ is separator)
* type - type to return TPF_??, see ntreg.h
* return: offset to nk or vk (or NULL if not found)
*/

int trav_path(struct hive *hdesc, int vofs, const char *path, int type)
{
	struct nk_key *key, *newnkkey;
	struct lf_key *lfkey;
	struct li_key *likey;
	struct ri_key *rikey;

	int32_t *vlistkey;
	int newnkofs, plen, i, lfofs, vlistofs, adjust, r, ricnt, subs;
	char *buf;
	char part[ABSPATHLEN + 1];
	char *partptr;

	if (!hdesc) return(0);
	buf = hdesc->buffer;

	//  PRINTF("trav_path: called with vofs = %x, path = <%s>, type = %x\n",vofs, path, type);


	if (!vofs) vofs = hdesc->rootofs + 4;     /* No current key given , so start at root */

	if (!(type & TPF_ABS) && *path == '\\' && *(path + 1) != '\\') {      /* Start from root if path starts with \ */
		path++;
		vofs = hdesc->rootofs + 4;
	}

	key = (struct nk_key *)(buf + vofs);
	//  PRINTF("check of nk at offset: 0x%0x\n",vofs); 

	if (key->id != 0x6b6e) {
//		PRINTF("trav_path: Error: Not a 'nk' node!\n");
		return(0);
	}

	if (!(type & TPF_ABS)) {  /* Only traverse path if not absolute literal value name passed */

		/* TODO: Need to rethink this.. */

		/* Find \ delimiter or end of string, copying to name part buffer as we go,
		rewriting double \\s */
		partptr = part;
		for (plen = 0; path[plen] && (path[plen] != '\\' || path[plen + 1] == '\\'); plen++) {
			if (path[plen] == '\\' && path[plen + 1] == '\\') plen++; /* Skip one if double */
			*partptr++ = path[plen];
		}
		*partptr = '\0';

#if 0    
		PRINTF("Name part: <%s>\n", part);
		PRINTF("Name path: <%s>\n", path);
#endif

		adjust = (path[plen] == '\\') ? 1 : 0;
		// PRINTF("Checking for <%s> with len %d\n",path,plen); 

		if (!plen) return(vofs - 4);     /* Path has no lenght - we're there! */

		if ((plen == 1) && (*(path + 1) && *path == '.') && !(type & TPF_EXACT)) {     /* Handle '.' current dir */
			//  PRINTF("** handle current\n");
			return(trav_path(hdesc, vofs, path + plen + adjust, type));
		}
		if (!(type & TPF_EXACT) && (plen == 2) && !strncmp("..", path, 2)) { /* Get parent key */
			newnkofs = key->ofs_parent + 0x1004;
			/* Return parent (or only root if at the root) */
			return(trav_path(hdesc, (key->type == KEY_ROOT ? vofs : newnkofs), path + plen + adjust, type));
		}

	}

	/* at last name of path, and we want vk, and the nk has values */
	if ((type & TPF_VK_ABS) || (!path[plen] && (type & TPF_VK) && key->no_values)) {
		//  if ( (!path[plen] && (type & TPF_VK) && key->no_values) ) {   
		if (type & TPF_ABS) {
			strcpy(part, path);
			plen = de_escape(part, 0);
			partptr = part + plen;
		}

		//  PRINTF("VK namematch for <%s>, type = %d\n",part,type);
		vlistofs = key->ofs_vallist + 0x1004;
		vlistkey = (int32_t *)(buf + vlistofs);
		i = vlist_find(hdesc, vlistofs, key->no_values, part, type);
		if (i != -1) {
			return(vlistkey[i] + 0x1000);
		}
	}

	if (key->no_subkeys > 0) {    /* If it has subkeys, loop through the hash */
		char *partw = NULL;
		int partw_len, part_len;

		// PRINTF("trav_path: subkey loop: path = %s, part = %s\n",path,part);

		lfofs = key->ofs_lf + 0x1004;    /* lf (hash) record */
		lfkey = (struct lf_key *)(buf + lfofs);

		if (lfkey->id == 0x6972) { /* ri struct need special parsing */
			/* Prime loop state */

			rikey = (struct ri_key *)lfkey;
			ricnt = rikey->no_lis;
			r = 0;
			likey = (struct li_key *)(hdesc->buffer + rikey->hash[r].ofs_li + 0x1004);
			subs = likey->no_keys;
			if (likey->id != 0x696c) {  /* Bwah, not li anyway, XP uses lh usually which is actually smarter */
				lfkey = (struct lf_key *)(hdesc->buffer + rikey->hash[r].ofs_li + 0x1004);
				likey = NULL;
			}
		}
		else {
			if (lfkey->id == 0x696c) { /* li? */
				likey = (struct li_key *)(buf + lfofs);
			}
			else {
				likey = NULL;
			}
			rikey = NULL;
			ricnt = 0; r = 0; subs = key->no_subkeys;
		}

		partw = string_prog2regw(part, partptr - part, &partw_len);
		//    string_prog2rega(part, partptr-part);
		part_len = strlen(part);
		do {
			for (i = 0; i < subs; i++) {
				if (likey) newnkofs = likey->hash[i].ofs_nk + 0x1004;
				else newnkofs = lfkey->h.hash[i].ofs_nk + 0x1004;
				newnkkey = (struct nk_key *)(buf + newnkofs);
				if (newnkkey->id != 0x6b6e) {
//					PRINTF("ERROR: not 'nk' node! (strange?)\n");
				}
				else {
					if (newnkkey->len_name <= 0) {
//						PRINTF("[No name]\n");
					}
					else if (
						((part_len <= newnkkey->len_name) && !(type & TPF_EXACT)) ||
						((part_len == newnkkey->len_name) && (type & TPF_EXACT))
						) {
						/* Can't match if name is shorter than we look for */
						int cmp;
						//	    PRINTF("trav_path: part = <%s>, part_len = %d\n",part,part_len);
						if (newnkkey->type & 0x20)
							cmp = strncmp(part, newnkkey->keyname, part_len);
						else
							cmp = memcmp(partw, newnkkey->keyname, partw_len);
						if (!cmp) {
							//  PRINTF("Key at 0x%0x matches! recursing! new path = %s\n",newnkofs,path+plen+adjust); 
							free(partw);
							return(trav_path(hdesc, newnkofs, path + plen + adjust, type));
						}
					}
				} /* if id OK */
			} /* hash loop */
			r++;
			if (ricnt && r < ricnt) {
				newnkofs = rikey->hash[r].ofs_li;
				likey = (struct li_key *)(hdesc->buffer + newnkofs + 0x1004);
				subs = likey->no_keys;
				if (likey->id != 0x696c) {  /* Bwah, not li anyway, XP uses lh usually which is actually smarter */
					lfkey = (struct lf_key *)(hdesc->buffer + rikey->hash[r].ofs_li + 0x1004);
					likey = NULL;
				}
			}
		} while (r < ricnt && ricnt);
		free(partw);

	} /* if subkeys */

	/* Not found */
	return(0);
}



/* This function only remains as an example for reading a key. */
/*
void nk_ls(struct hive *hdesc, const char *path, int vofs, int type)
{
struct nk_key *key;
int nkofs;
struct ex_data ex;
struct vex_data vex;
int count = 0, countri = 0;

nkofs = trav_path(hdesc, vofs, path, 0);

if(!nkofs) {
printf("nk_ls: Key <%s> not found\n",path);
return;
}
nkofs += 4;

key = (struct nk_key *)(hdesc->buffer + nkofs);

if (key->id != 0x6b6e) printf("Error: Not a 'nk' node at offset %x!\n",nkofs);

printf("Node has %d subkeys and %d values",key->no_subkeys,key->no_values);
if (key->len_classnam) printf(", and class-data of %d bytes",key->len_classnam);
printf("\n");

if (key->no_subkeys) {
printf("  key name\n");
while ((ex_next_n(hdesc, nkofs, &count, &countri, &ex) > 0)) {
printf("[%6x] %c <%s>\n", ex.nkoffs, (ex.nk->len_classnam)?'*':' ',ex.name);
free(ex.name);
}
}
count = 0;
if (key->no_values) {
printf("  size     type              value name             [value if type DWORD]\n");
while ((ex_next_v(hdesc, nkofs, &count, &vex) > 0)) {
printf("%6d  %x %-16s   <%s>", vex.size, vex.type,
(vex.type < REG_MAX ? val_types[vex.type] : "(unknown)"), vex.name);

if (vex.type == REG_DWORD) printf(" %*d [0x%x]",25-(int)strlen(vex.name),vex.val , vex.val);
printf("\n");
free(vex.name);
}
}
}
*/

/* Get the type of a value */
int get_val_type(struct hive *hdesc, int vofs, const char *path, int exact)
{
	struct vk_key *vkkey;
	int vkofs;

	LOAD_WD_LOGONLY();

	vkofs = trav_path(hdesc, vofs, path, exact | TPF_VK_EXACT);
	if (!vkofs) {
		LOG("get_val_type: trav_path failed: %s offset %d\n", path, vofs);
		return -1;
	}
	vkofs += 4;
	vkkey = (struct vk_key *)(hdesc->buffer + vkofs);
	return vkkey->val_type;
}

/* Set/change the type of a value. Strangely we need this in some situation in SAM
* and delete + add value is a bit overkill */

int set_val_type(struct hive *hdesc, int vofs, const char *path, int exact, int type)
{
	struct vk_key *vkkey;
	int vkofs;

	LOAD_WD_LOGONLY();

	vkofs = trav_path(hdesc, vofs, path, exact | TPF_VK);
	if (!vkofs) {
		LOG("set_val_type: trav_path failed: %s offset %d\n", path, vofs);
		return -1;
	}
	vkofs += 4;
	vkkey = (struct vk_key *)(hdesc->buffer + vkofs);
	vkkey->val_type = type;
	return vkkey->val_type;
}

/* Get len of a value, given current key + path */
int get_val_len(struct hive *hdesc, int vofs, const char *path, int exact)
{
	struct vk_key *vkkey;
	int vkofs;
	int len;

	LOAD_WD_LOGONLY();

	vkofs = trav_path(hdesc, vofs, path, exact | TPF_VK);
	if (!vkofs) {
		LOG("get_val_len: trav_path failed: %s offset %d\n", path, vofs);
		return -1;
	}

	vkofs += 4;
	if (vkofs > hdesc->size) {
		LOG("get_val_len: error: 'vk' offset too large: %x\n", vkofs);
		return -1;
	}

	vkkey = (struct vk_key *)(hdesc->buffer + vkofs);
	if (vkkey->id != 0x6b76) {
		LOG("get_val_len: Cell ID is not 'vk': offset 0x%x, subpath \\%s\n", vkofs, path);
		return -1;
	}


	len = vkkey->len_data & 0x7fffffff;

	if (vkkey->len_data == 0x80000000 && (exact & TPF_VK_SHORT)) {  /* Special inline case, return size of 4 (dword) */
		len = 4;
	}

	return len;
}

/* Get void-pointer to value-data, also if inline.
* If val_type != 0 a check for correct value type is done
* Caller must keep track of value's length (call function above to get it)
*/
void *get_val_data(struct hive *hdesc, int vofs, const char *path, int val_type, int exact)
{
	struct vk_key *vkkey;
	int vkofs;

	LOAD_WD_LOGONLY();

	vkofs = trav_path(hdesc, vofs, path, exact | TPF_VK);
	if (!vkofs) {
		LOG("get_val_data: not found: %s\n", path);
		return NULL;
	}
	vkofs += 4;
	vkkey = (struct vk_key *)(hdesc->buffer + vkofs);

	if (vkkey->id != 0x6b76) {
		LOG("get_val_data: Cell ID is not 'vk': offset 0x%x, subpath \\%s\n", vkofs, path);
		return NULL;
	}

	if (vkkey->len_data == 0) {
		LOG("get_val_data: zero-length value data: %s\n", path);
		return NULL;
	}

	if (vkkey->len_data == 0x80000000 && (exact & TPF_VK_SHORT)) {  /* Special inline case (len = 0x80000000) */
		return &vkkey->val_type; /* Data (4 bytes?) in type field */
	}

	if (val_type && vkkey->val_type && (vkkey->val_type) != val_type) {
		LOG("get_val_data: not of correct type: %s\n", path);
		return NULL;
	}

	/* Negative len is inline, return ptr to offset-field which in
	* this case contains the data itself
	*/
	if (vkkey->len_data & 0x80000000) return &vkkey->ofs_data;
	/* Normal return, return data pointer */
	return (hdesc->buffer + vkkey->ofs_data + 0x1004);
}


/* Get and copy key data (if any) to buffer
* if kv==NULL will allocate needed return struct & buffer
* else will use buffer allocated for it (if it fits)
* return len+data or NULL if not found (or other error)
* NOTE: caller must deallocate buffer! a simple free(keyval) will suffice.
*/
struct keyval *get_val2buf(struct hive *hdesc, struct keyval *kv,
	int vofs, const char *path, int type, int exact)
{
	int l, i, parts, list, blockofs, blocksize, point, copylen, restlen;
	struct keyval *kr;
	void *keydataptr;
	struct db_key *db;
	void *addr;

	LOAD_WD_LOGONLY();

	l = get_val_len(hdesc, vofs, path, exact);
	DLOG("get_val2buf: value length for 'vk' offset %x subpath '\\%s' = %x\n", vofs, path, l);
	if (l == -1) {
		LOG("get_val2buf: get_val_len error: %s offset %d\n", path, vofs);
		return NULL;
	}
	if (kv && (kv->len < l)) {
		LOG("get_val2buf: Buffer overflow\n");
		return NULL;
	}

	keydataptr = get_val_data(hdesc, vofs, path, type, exact);
	if (!keydataptr) {
		LOG("get_val2buf: error reading value data\n");
		return NULL;
	}
	/* Allocate space for data + header, or use supplied buffer */
	if (kv) {
		kr = kv;
	}
	else {
		ALLOC(kr, 1, 2 * sizeof(struct keyval));
		ALLOC(kr->data, 1, l * sizeof(int32_t));
	}

	kr->len = l;

	if (l > VAL_DIRECT_LIMIT) {       /* Where do the db indirects start? seems to be around 16k */
		DLOG("get_val2buf: notice: kr->len > VAL_DIRECT_LIMIT: %x\n", l);
		db = (struct db_key *)keydataptr;
		if (db->id != 0x6264) {
			LOG("get_val2buf: data block expected but not found: 0x%" PRIxPTR "\n", (intptr_t)db - (intptr_t)hdesc->buffer);
			if (!kv) { free(kr->data); free(kr); }
			return NULL;
		}
		parts = db->no_part;
		list = db->ofs_data + 0x1004;
		LOG("get_val2buf: Long value: parts %d, list %x\n", parts, list);

		point = 0;
		restlen = l;
		for (i = 0; i < parts; i++) {
			blockofs = get_int32(hdesc->buffer + list + (i << 2)) + 0x1000;
			blocksize = -get_int32(hdesc->buffer + blockofs) - 8;

			/* Copy this part, up to size of block or rest lenght in last block */
			copylen = (blocksize > restlen) ? restlen : blocksize;

			DLOG("get_val2buf: Datablock %d offset %x, size %x (%d)\n", i, blockofs, blocksize, blocksize);
			DLOG("             : Point = %x, restlen = %x, copylen = %x\n", point, restlen, copylen);

			addr = (void *)((int *)kr->data + point);
			memcpy(addr, hdesc->buffer + blockofs + 4, copylen);

			point += copylen;
			restlen -= copylen;
		}
	}
	else {
		if (l && kr && keydataptr) memcpy(kr->data, keydataptr, l);
	}
	return kr;
}

/* Sanity checker when transferring data into a block
* ofs = offset to data block, point to start of actual datablock linkage
* data = data to copy
* size = size of data to copy
*/

int fill_block(struct hive *hdesc, int ofs, void *data, int size)
{
	int32_t blksize;

	LOAD_WD_LOGONLY();

	blksize = get_int32(hdesc->buffer + ofs);
	blksize = -blksize;

	/*  if (blksize < size || ( (ofs & 0xfffff000) != ((ofs+size) & 0xfffff000) )) { */
	if (blksize < (unsigned int)size) goto error_too_small;

	memcpy(hdesc->buffer + ofs + 4, data, size);
	if (mark_pages_dirty(hdesc, ofs + 4, ofs + size + 4)) goto error_dirty;
	return 0;

error_too_small:
	LOG("fill_block: too small for data: ofs %x, size %x, blksize %x\n", ofs, size, blksize);
	return -1;
error_dirty:
	LOG("fill_block: mark_pages_dirty returned an error\n");
	return -1;
}


/* Free actual data of a value, and update value descriptor
* hdesc - hive
* vofs  - current value offset
*/

int free_val_data(struct hive *hdesc, int vkofs)
{
	struct vk_key *vkkey;
	struct db_key *db;
	int len, i, blockofs, parts, list;
#if LOG_IS_USED
	int blocksize;
#endif

	LOAD_WD_LOGONLY();

	vkkey = (struct vk_key *)(hdesc->buffer + vkofs);
	len = vkkey->len_data;

	if (!(len & 0x80000000)) {  /* Check for inline, if so, skip it, nothing to do */

		if (len > VAL_DIRECT_LIMIT) {       /* Where do the db indirects start? seems to be around 16k */

			db = (struct db_key *)(hdesc->buffer + vkkey->ofs_data + 0x1004);
			if (db->id != 0x6264) {
				LOG("free_val_data: db id incorrect\n");
				return -1;
			}
			parts = db->no_part;
			list = db->ofs_data + 0x1004;

			DLOG("free_val_data: Long value: parts %d, list %x\n", parts, list);

			for (i = 0; i < parts; i++) {
				blockofs = get_int32(hdesc->buffer + list + (i << 2)) + 0x1000;
#if LOG_IS_USED
				blocksize = -get_int32(hdesc->buffer + blockofs);
				LOG("free_val_data: Freeing long block %d: offset %x, size %x (%d)\n", i, blockofs, blocksize, blocksize);
#endif
				free_block(hdesc, blockofs);
			}

			DLOG("free_val_data: Freeing indirect list at %x\n", list - 4);
			free_block(hdesc, list - 4);
			DLOG("free_val_data: Freeing db structure at %x\n", vkkey->ofs_data + 0x1000);
		} /* Fall through to regular which deallocs data or db block ofs_data point to */

		if (len) free_block(hdesc, vkkey->ofs_data + 0x1000);

	} /* inline check */
	vkkey->len_data = 0;
	vkkey->ofs_data = 0;
	mark_pages_dirty(hdesc, vkofs, vkofs);

	return vkofs;
}


/* Allocate data for value. Frees old data (if any) which will be destroyed
* hdesc - hive
* vofs  - current key
* path  - path to value
* size  - size of data
* Returns: 0 - error, >0 pointer to actual dataspace
*/

int alloc_val_data(struct hive *hdesc, int vofs, const char *path, int size, int exact)
{
	struct vk_key *vkkey;
	struct db_key *db;
	int vkofs, dbofs, listofs, blockofs, blocksize, parts;
	int datablk, i, j;
	int *ptr;

	LOAD_WD_LOGONLY();

	vkofs = trav_path(hdesc, vofs, path, exact);
	if (!vkofs) goto error_trav_path;

	vkofs += 4;
	vkkey = (struct vk_key *)(hdesc->buffer + vkofs);

	/* Get rid of old data if any */
	if (free_val_data(hdesc, vkofs) < 0) goto error_free;

	/* Allocate space for new data */
	if (size > 4) {
		if (size > VAL_DIRECT_LIMIT) {  /* We must allocate indirect stuff *sigh* */
			parts = size / VAL_DIRECT_LIMIT + 1;
			DLOG("alloc_val_data: large key: size %x (%d), parts %d\n", size, size, parts);

			dbofs = alloc_block(hdesc, vkofs, sizeof(struct db_key));    /* Alloc db structure */
			db = (struct db_key *)(hdesc->buffer + dbofs + 4);
			db->id = 0x6264;
			db->no_part = parts;
			listofs = alloc_block(hdesc, vkofs, 4 * parts);  /* block offset list */
			db = (struct db_key *)(hdesc->buffer + dbofs + 4);
			db->ofs_data = listofs - 0x1000;
			if (mark_pages_dirty(hdesc, dbofs, dbofs + 4)) goto error_dirty;
			LOG("alloc_val_data: dbofs %x, listofs %x\n", dbofs, listofs);

			for (i = 0; i < parts; i++) {
				blocksize = VAL_DIRECT_LIMIT;      /* Windows seem to alway allocate the whole block */
				blockofs = alloc_block(hdesc, vkofs, blocksize);
				LOG("alloc_val_data: block %d, blockofs %x\n", i, blockofs);
				j = listofs + 4 + (i << 2);
				ptr = (int *)(hdesc->buffer + j);
				*ptr = blockofs - 0x1000;
				if (mark_pages_dirty(hdesc, j, j)) goto error_dirty;
			}
			datablk = dbofs;

		}
		else { /* Regular size < 16 k direct alloc */
			datablk = alloc_block(hdesc, vkofs, size);
		}

	}
	else { /* 4 bytes or less are inlined */
		/*    datablk = vkofs + (int32_t)&(vkkey->ofs_data) - (int32_t)vkkey; */
		datablk = vkofs + (int)(&(vkkey->ofs_data) - (int *)vkkey);
		size |= 0x80000000;
	}

	if (!datablk) goto error_db_ptr;

	vkkey = (struct vk_key *)(hdesc->buffer + vkofs); /* alloc_block may move pointer, realloc() buf */

	/* Link in new datablock */
	if (!(size & 0x80000000)) vkkey->ofs_data = datablk - 0x1000;
	vkkey->len_data = size;
	if (mark_pages_dirty(hdesc, vkofs, vkofs)) goto error_dirty;

	return (datablk + 4);

error_trav_path:
	LOG("alloc_val_data: trav_path failed: %s offset %d\n", path, vofs);
	return 0;
error_free:
	LOG("alloc_val_data: error freeing old data\n");
	return 0;
error_db_ptr:
	LOG("alloc_val_data: data block pointer is NULL\n");
	return 0;
error_dirty:
	LOG("alloc_val_data: mark_pages_dirty returned an error\n");
	return 0;
}


/* Add a value to a key.
* Just add the metadata (empty value), to put data into it, use
* put_buf2val afterwards
* hdesc - hive
* nkofs - current key
* name  - name of value
* type  - type of value
* returns: 0 err, >0 offset to value metadata
*/

struct vk_key *add_value(struct hive *hdesc, int nkofs, const char *name, int type)
{
	struct nk_key *nk;
	int oldvlist = 0, newvlist, newvkofs, i;
	struct vk_key *newvkkey;
	char *blank = "";

	LOAD_WD_LOGONLY();

	if (!name || !*name) goto error_null_name;
	nk = (struct nk_key *)(hdesc->buffer + nkofs);
	if (nk->id != 0x6b6e) goto error_not_nk;

	if (vlist_find(hdesc, nk->ofs_vallist + 0x1004, nk->no_values, name, TPF_EXACT) != -1) {
		if (!del_value(hdesc, nkofs, name)) goto error_del_value;
	}

	if (!streq(name, "@")) name = blank;

	if (nk->no_values) oldvlist = nk->ofs_vallist;

	newvlist = alloc_block(hdesc, nkofs, nk->no_values * 4 + 4);
	if (!newvlist) goto error_vlist_alloc;

	nk = (struct nk_key *)(hdesc->buffer + nkofs); /* In case buffer was moved.. */

	if (oldvlist) {   /* Copy old data if any */
		memcpy(hdesc->buffer + newvlist + 4, hdesc->buffer + oldvlist + 0x1004, nk->no_values * 4 + 4);
		if (mark_pages_dirty(hdesc, newvlist + 4, newvlist + 8 + nk->no_values * 4)) goto error_dirty;
	}

	/* Allocate value descriptor including its name */
	newvkofs = alloc_block(hdesc, newvlist, sizeof(struct vk_key) + strlen(name));
	if (!newvkofs) goto error_vkofs_alloc;
	nk = (struct nk_key *)(hdesc->buffer + nkofs); /* In case buffer was moved.. */

	/* Success, now fill in the metadata */

	newvkkey = (struct vk_key *)(hdesc->buffer + newvkofs + 4);

	/* Add pointer in value list */
	i = newvlist + 4 + (nk->no_values * 4);
	*(int *)(hdesc->buffer + i) = newvkofs - 0x1000;
	if (mark_pages_dirty(hdesc, i, i)) goto error_dirty;

	/* Fill in vk struct */
	newvkkey->id = 0x6b76;
	newvkkey->len_name = strlen(name);
	if (type == REG_DWORD || type == REG_DWORD_BIG_ENDIAN) {
		newvkkey->len_data = 0x80000004;  /* Prime the DWORD inline stuff */
	}
	else {
		newvkkey->len_data = 0x80000000;  /* Default inline zero size */
	}
	newvkkey->ofs_data = 0;
	newvkkey->val_type = type;
	newvkkey->flag = newvkkey->len_name ? 1 : 0;  /* Seems to be 1, but 0 for no name default value */
	newvkkey->dummy1 = 0;
	memcpy((char *)&newvkkey->keyname, name, newvkkey->len_name);  /* And copy name */

	/* Finally update the key and free the old valuelist */
	nk->no_values++;
	nk->ofs_vallist = newvlist - 0x1000;
	if (oldvlist) free_block(hdesc, oldvlist + 0x1000);
	if (mark_pages_dirty(hdesc, newvkofs + 4, newvkofs + 4)) goto error_dirty;

	return newvkkey;

error_null_name:
	LOG("add_value: Null or empty value name\n");
	return NULL;
error_not_nk:
	LOG("add_value: Key pointer not to 'nk' node\n");
	return NULL;
error_del_value:
	LOG("add_value: could not remove old value: %s\n", name);
	return NULL;
error_dirty:
	LOG("add_block: mark_pages_dirty returned an error\n");
	return NULL;
error_vkofs_alloc:
	free_block(hdesc, newvlist);
	LOG("add_block: alloc_block failed to allocate value descriptor\n");
	return NULL;
error_vlist_alloc:
	LOG("add_value: failed to allocate new value list\n");
	return NULL;
}

/* Remove a vk-struct incl dataspace if any
* Mostly for use by higher level stuff
* hdesc - hive
* vkofs - offset to vk
*/

int del_vk(struct hive *hdesc, int vkofs)
{
	struct vk_key *vk;

	LOAD_WD_LOGONLY();

	vk = (struct vk_key *)(hdesc->buffer + vkofs);
	if (vk->id != 0x6b76) {
		LOG("del_vk: Key pointer not to 'vk' node\n");
		return 1;
	}

	if (!(vk->len_data & 0x80000000) && vk->ofs_data) {
		if (free_val_data(hdesc, vkofs) < 0) {
			LOG("del_vk: error freeing value data\n");
			return -1;
		}
	}
	free_block(hdesc, vkofs - 4);
	return 0;
}


/* Delete single value from key
* hdesc - yer usual hive
* nkofs - current keyoffset
* name  - name of value to delete
* returns: 0 - ok, 1 - failed
*/

int del_value(struct hive *hdesc, int nkofs, const char *name)
{
	int vlistofs, slot, o, n, vkofs, newlistofs;
	int32_t *vlistkey, *tmplist, *newlistkey;
	struct nk_key *nk;
	char *blank = "";

	LOAD_WD_LOGONLY();

	if (!name || !*name) {
		LOG("del_value: Null or empty name given\n");
		return 1;
	}
	if (!streq(name, "@")) name = blank;
	nk = (struct nk_key *)(hdesc->buffer + nkofs);
	if (nk->id != 0x6b6e) {
		LOG("del_value: Key pointer not to 'nk' node: %s\n", name);
		return 1;
	}

	if (!nk->no_values) {
		LOG("del_value: Key has no values: %s\n", name);
		return 1;
	}

	vlistofs = nk->ofs_vallist + 0x1004;
	vlistkey = (int32_t *)(hdesc->buffer + vlistofs);

	slot = vlist_find(hdesc, vlistofs, nk->no_values, name, TPF_VK_EXACT);

	if (slot == -1) {
		LOG("del_value: not found: %s\n", name);
		return 1;
	}

	/* Delete vk and data */
	vkofs = vlistkey[slot] + 0x1004;
	del_vk(hdesc, vkofs);

	/* Copy out old index list */
	CREATE(tmplist, int32_t, nk->no_values);
	memcpy(tmplist, vlistkey, nk->no_values * sizeof(int32_t));

	free_block(hdesc, vlistofs - 4);  /* Get rid of old list */

	nk->no_values--;

	if (nk->no_values) {
		newlistofs = alloc_block(hdesc, vlistofs, nk->no_values * sizeof(int32_t));
		if (!newlistofs) {
			LOG("del_value: Was not able to alloc new index list\n");
			return 1;
		}
		nk = (struct nk_key *)(hdesc->buffer + nkofs); /* In case buffer was moved */

		/* Now copy over, omitting deleted entry */
		newlistkey = (int32_t *)(hdesc->buffer + newlistofs + 4);
		for (n = 0, o = 0; n < nk->no_values; o++, n++) {
			if (o == slot) o++;
			newlistkey[n] = tmplist[o];
		}
		nk->ofs_vallist = newlistofs - 0x1000;
	}
	else {
		nk->ofs_vallist = -1;
	}
	return 0;
}


/* Add a subkey to a key
* hdesc - usual..
* nkofs - offset of current nk
* name  - name of key to add
* return: ptr to new keystruct, or NULL
*/

struct nk_key *add_key(struct hive *hdesc, int nkofs, const char *name)
{
	int slot, newlfofs = 0, oldlfofs = 0, newliofs = 0;
	int oldliofs = 0;
	int o, n, i, onkofs, newnkofs, cmp;
	int rimax, rislot, riofs, namlen;
	struct ri_key *ri = NULL;
	struct lf_key *newlf = NULL, *oldlf;
	struct li_key *newli = NULL, *oldli;
	struct nk_key *key, *newnk, *onk;
	int32_t hash;

	LOAD_WD_LOGONLY();

	key = (struct nk_key *)(hdesc->buffer + nkofs);

	if (key->id != 0x6b6e) goto error_not_nk;

	namlen = strlen(name);

	slot = -1;
	if (key->no_subkeys) {   /* It already has subkeys */

		oldlfofs = key->ofs_lf;
		oldliofs = key->ofs_lf;

		oldlf = (struct lf_key *)(hdesc->buffer + oldlfofs + 0x1004);
		if (oldlf->id != 0x666c && oldlf->id != 0x686c && oldlf->id != 0x696c && oldlf->id != 0x697)
			goto error_bad_index_type;

		rimax = 0; ri = NULL; riofs = 0; rislot = -1;
		if (oldlf->id == 0x6972) {  /* Indirect index 'ri', init loop */
			riofs = key->ofs_lf;
			ri = (struct ri_key *)(hdesc->buffer + riofs + 0x1004);
			rimax = ri->no_lis - 1;

			oldliofs = ri->hash[rislot + 1].ofs_li;
			oldlfofs = ri->hash[rislot + 1].ofs_li;

		}

		do {   /* 'ri' loop, at least run once if no 'ri' deep index */
			if (ri) { /* Do next 'ri' slot */
				rislot++;
				oldliofs = ri->hash[rislot].ofs_li;
				oldlfofs = ri->hash[rislot].ofs_li;
				oldli = (struct li_key *)(hdesc->buffer + oldliofs + 0x1004);
				oldlf = (struct lf_key *)(hdesc->buffer + oldlfofs + 0x1004);
			}

			oldli = (struct li_key *)(hdesc->buffer + oldliofs + 0x1004);
			oldlf = (struct lf_key *)(hdesc->buffer + oldlfofs + 0x1004);

			slot = -1;

			if (oldli->id == 0x696c) {  /* li */

				free(newli);
				ALLOC(newli, 8 + 4 * oldli->no_keys + 4, 1);
				newli->no_keys = oldli->no_keys;
				newli->id = oldli->id;

				/* Now copy old, checking where to insert (alphabetically) */
				for (o = 0, n = 0; o < oldli->no_keys; o++, n++) {
					onkofs = oldli->hash[o].ofs_nk;
					onk = (struct nk_key *)(onkofs + hdesc->buffer + 0x1004);
					if (slot == -1) {
						cmp = strn_casecmp(name, onk->keyname, (namlen > onk->len_name) ? namlen : onk->len_name);
						if (!cmp) goto error_key_exists;
						if (cmp < 0) {
							slot = o;
							rimax = rislot; /* Cause end of 'ri' search, too */
							n++;
						}
					}
					newli->hash[n].ofs_nk = oldli->hash[o].ofs_nk;
				}
				if (slot == -1) slot = oldli->no_keys;

			}
			else { /* lf or lh */

				oldlf = (struct lf_key *)(hdesc->buffer + oldlfofs + 0x1004);

				free(newlf);
				ALLOC(newlf, 8 + 8 * oldlf->no_keys + 8, 1);
				newlf->no_keys = oldlf->no_keys;
				newlf->id = oldlf->id;

				/* Now copy old, checking where to insert (alphabetically) */
				for (o = 0, n = 0; o < oldlf->no_keys; o++, n++) {
					onkofs = oldlf->h.hash[o].ofs_nk;
					onk = (struct nk_key *)(onkofs + hdesc->buffer + 0x1004);
					if (slot == -1) {

						cmp = strn_casecmp(name, onk->keyname, (namlen > onk->len_name) ? namlen : onk->len_name);
						if (!cmp) goto error_key_exists;
						if (cmp < 0) {
							slot = o;
							rimax = rislot;  /* Cause end of 'ri' search, too */
							n++;
						}
					}
					newlf->h.hash[n].ofs_nk = oldlf->h.hash[o].ofs_nk;
					newlf->h.hash[n].name[0] = oldlf->h.hash[o].name[0];
					newlf->h.hash[n].name[1] = oldlf->h.hash[o].name[1];
					newlf->h.hash[n].name[2] = oldlf->h.hash[o].name[2];
					newlf->h.hash[n].name[3] = oldlf->h.hash[o].name[3];
				}
				if (slot == -1) slot = oldlf->no_keys;
			} /* li else check */
		} while ((rislot < rimax) && (rimax > 0));  /* 'ri' wrapper loop */

	}
	else { /* Parent was empty, make new index block */
		ALLOC(newlf, 8 + 8, 1);
		newlf->no_keys = 0;    /* Will increment to 1 when filling in the offset later */
		/* Use ID (lf, lh or li) we fetched from root node, so we use same as rest of hive */
		newlf->id = hdesc->nkindextype;
		slot = 0;
	} /* if has keys before */

	/* Make and fill in new nk */
	newnkofs = alloc_block(hdesc, nkofs, sizeof(struct nk_key) + strlen(name));
	if (!newnkofs) goto error_newnkofs;
	key = (struct nk_key *)(hdesc->buffer + nkofs);  /* In case buffer moved */
	newnk = (struct nk_key *)(hdesc->buffer + newnkofs + 4);

	newnk->id = 0x6b6e;
	newnk->type = KEY_NORMAL;    /* Some versions use 0x1020 a lot.. */
	newnk->ofs_parent = nkofs - 0x1004;
	newnk->no_subkeys = 0;
	newnk->ofs_lf = -1;
	newnk->no_values = 0;
	newnk->ofs_vallist = -1;
	newnk->ofs_sk = key->ofs_sk; /* Get parents for now. 0 or -1 here crashes XP */
	newnk->ofs_classnam = -1;
	newnk->len_name = strlen(name);
	newnk->len_classnam = 0;
	memcpy(newnk->keyname, name, newnk->len_name);

	if (newli) {  /* Handle li */

		/* And put its offset into parents index list */
		newli->hash[slot].ofs_nk = newnkofs - 0x1000;
		newli->no_keys++;

		/* Allocate space for our new li list and copy it into reg */
		newliofs = alloc_block(hdesc, nkofs, 8 + 4 * newli->no_keys);
		if (!newliofs) goto error_newliofs;
		key = (struct nk_key *)(hdesc->buffer + nkofs);  /* In case buffer moved */
		newnk = (struct nk_key *)(hdesc->buffer + newnkofs + 4);

		/*    memcpy(hdesc->buffer + newliofs + 4, newli, 8 + 4*newli->no_keys); */
		if (fill_block(hdesc, newliofs, newli, 8 + 4 * newli->no_keys)) goto error_fill_block;
	}
	else {  /* lh or lf */
		/* And put its offset into parents index list */
		newlf->h.hash[slot].ofs_nk = newnkofs - 0x1000;
		newlf->no_keys++;
		if (newlf->id == 0x666c) {        /* lf hash */
			newlf->h.hash[slot].name[0] = 0;
			newlf->h.hash[slot].name[1] = 0;
			newlf->h.hash[slot].name[2] = 0;
			newlf->h.hash[slot].name[3] = 0;
			strncpy(newlf->h.hash[slot].name, name, 4);
		}
		else if (newlf->id == 0x686c) {  /* lh. XP uses this. hashes whole name */
			for (i = 0, hash = 0; i < (int)strlen(name); i++) {
				hash *= 37;
				hash += reg_touppertable[(unsigned char)name[i]];
			}
			newlf->h.lh_hash[slot].hash = hash;
		}

		/* Allocate space for our new lf list and copy it into reg */
		newlfofs = alloc_block(hdesc, nkofs, 8 + 8 * newlf->no_keys);
		if (!newlfofs) goto error_alloc_index;
		key = (struct nk_key *)(hdesc->buffer + nkofs);  /* In case buffer moved */
		newnk = (struct nk_key *)(hdesc->buffer + newnkofs + 4);

		/*    memcpy(hdesc->buffer + newlfofs + 4, newlf, 8 + 8*newlf->no_keys); */
		if (fill_block(hdesc, newlfofs, newlf, 8 + 8 * newlf->no_keys)) goto error_fill_block;
	} /* li else */
	/* Update parent, and free old lf list */
	key->no_subkeys++;
	if (ri) {  /* ri index */
		ri->hash[rislot].ofs_li = (newlf ? newlfofs : newliofs) - 0x1000;
	}
	else { /* Parent key */
		key->ofs_lf = (newlf ? newlfofs : newliofs) - 0x1000;
	}

	if (newlf && oldlfofs) free_block(hdesc, oldlfofs + 0x1000);
	if (newli && oldliofs) free_block(hdesc, oldliofs + 0x1000);

	free(newlf);
	free(newli);
	return newnk;

error_not_nk:
	LOG("add_key: current pointer not 'nk'\n");
	return NULL;
error_bad_index_type:
	LOG("add_key: index type not supported: 0x%04x\n", oldlf->id);
	return NULL;
error_key_exists:
	free(newlf);
	LOG("add_key: key %s already exists!\n", name);
	return NULL;
error_newnkofs:
	free(newlf);
	free(newli);
	LOG("add_key: can't alloc space for key descriptor for %s\n", name);
	return NULL;
error_newliofs:
	free(newli);
	free_block(hdesc, newnkofs);
	LOG("add_key: can't alloc space for index table for %s\n", name);
	return NULL;
error_alloc_index:
	free(newlf);
	free_block(hdesc, newnkofs);
	LOG("add_key: can't alloc space for index table for %s\n", name);
	return NULL;
error_fill_block:
	LOG("add_key: fill_block failed\n");
	return NULL;
}

/* Delete a subkey from a key
* hdesc - usual..
* nkofs - offset of current nk
* name  - name of key to delete (must match exactly, also case)
* return: 1 - err, 0 - ok
*/

int del_key(struct hive *hdesc, int nkofs, const char *name)
{

	int slot = 0, newlfofs = 0, oldlfofs = 0, o, n, onkofs;
	int delnkofs = 0;
	int oldliofs = 0, no_keys = 0, newriofs = 0;
	int namlen;
	int rimax, riofs, rislot;
	struct ri_key *ri, *newri = NULL;
	struct lf_key *newlf = NULL, *oldlf = NULL;
	struct li_key *newli = NULL, *oldli = NULL;
	struct nk_key *key, *onk, *delnk = NULL;
	char fullpath[501];

	LOAD_WD_LOGONLY();

	key = (struct nk_key *)(hdesc->buffer + nkofs);

	namlen = strlen(name);

	if (key->id != 0x6b6e) goto error_not_nk;

	slot = -1;
	if (!key->no_subkeys) goto error_no_subkeys;

	oldlfofs = key->ofs_lf;
	oldliofs = key->ofs_lf;

	oldlf = (struct lf_key *)(hdesc->buffer + oldlfofs + 0x1004);
	if (oldlf->id != 0x666c && oldlf->id != 0x686c && oldlf->id != 0x696c && oldlf->id != 0x6972)
		goto error_index;

	rimax = 0; ri = NULL; riofs = 0;
	rislot = 0;

	if (oldlf->id == 0x6972) {  /* Indirect index 'ri', init loop */
		riofs = key->ofs_lf;
		ri = (struct ri_key *)(hdesc->buffer + riofs + 0x1004);
		rimax = ri->no_lis - 1;

		rislot = -1; /* Starts at slot 0 below */
	}

	do {   /* 'ri' loop, at least run once if no 'ri' deep index */

		if (ri) { /* Do next 'ri' slot */
			rislot++;
			oldliofs = ri->hash[rislot].ofs_li;
			oldlfofs = ri->hash[rislot].ofs_li;
		}

		oldli = (struct li_key *)(hdesc->buffer + oldliofs + 0x1004);
		oldlf = (struct lf_key *)(hdesc->buffer + oldlfofs + 0x1004);

		slot = -1;

		if (oldlf->id == 0x696c) {   /* 'li' handler */
			free(newli);
			ALLOC(newli, 8 + 4 * oldli->no_keys - 4, 1);
			newli->no_keys = oldli->no_keys - 1; no_keys = newli->no_keys;
			newli->id = oldli->id;

			/* Now copy old, checking where to delete */
			for (o = 0, n = 0; o < oldli->no_keys; o++, n++) {
				onkofs = oldli->hash[o].ofs_nk;
				onk = (struct nk_key *)(onkofs + hdesc->buffer + 0x1004);
				if (slot == -1 && onk->len_name == namlen && !strneq(name, onk->keyname, (onk->len_name > namlen) ? onk->len_name : namlen)) {
					slot = o;
					delnkofs = onkofs; delnk = onk;
					rimax = rislot;
					o++;
				}
				newli->hash[n].ofs_nk = oldli->hash[o].ofs_nk;
			}
			if (mark_pages_dirty(hdesc, 4 + 4 * oldli->no_keys, 4 + 4 * oldli->no_keys)) goto error_dirty;

		}
		else { /* 'lf' or 'lh' are similar */

			free(newlf);
			ALLOC(newlf, 8 + 8 * oldlf->no_keys - 8, 1);
			newlf->no_keys = oldlf->no_keys - 1; no_keys = newlf->no_keys;
			newlf->id = oldlf->id;

			/* Now copy old, checking where to delete */
			for (o = 0, n = 0; o < oldlf->no_keys; o++, n++) {

				onkofs = oldlf->h.hash[o].ofs_nk;
				onk = (struct nk_key *)(onkofs + hdesc->buffer + 0x1004);

				if (slot == -1 && (onk->len_name == namlen) && !strneq(name, onk->keyname, onk->len_name)) {
					slot = o;
					delnkofs = onkofs; delnk = onk;
					rimax = rislot;
					o++;
				}

				if (n < newlf->no_keys) { /* Only store if not last index in old */
					newlf->h.hash[n].ofs_nk = oldlf->h.hash[o].ofs_nk;
					newlf->h.hash[n].name[0] = oldlf->h.hash[o].name[0];
					newlf->h.hash[n].name[1] = oldlf->h.hash[o].name[1];
					newlf->h.hash[n].name[2] = oldlf->h.hash[o].name[2];
					newlf->h.hash[n].name[3] = oldlf->h.hash[o].name[3];
				}

			}
			if (mark_pages_dirty(hdesc, 8 * oldlf->no_keys, 8 * oldlf->no_keys)) goto error_dirty;

		} /* else lh or lf */

	} while (rislot < rimax);  /* ri traverse loop */

	if (slot == -1) goto error_subkey_not_found;

	if (delnk->no_values || delnk->no_subkeys) goto error_not_empty;

	/* Allocate space for our new lf list and copy it into reg */
	if (no_keys && (newlf || newli)) {
		newlfofs = alloc_block(hdesc, nkofs, 8 + (newlf ? 8 : 4) * no_keys);

		/* alloc_block may invalidate pointers if hive expanded. Recalculate this one.
		* Thanks to Jacky To for reporting it here, and suggesting a fix
		* (better would of course be for me to redesign stuff :)
		*/
		if (delnkofs) delnk = (struct nk_key *)(delnkofs + hdesc->buffer + 0x1004);

		if (!newlfofs) goto error_alloc_key;
		key = (struct nk_key *)(hdesc->buffer + nkofs);
		oldli = (struct li_key *)(hdesc->buffer + oldliofs + 0x1004);
		oldlf = (struct lf_key *)(hdesc->buffer + oldlfofs + 0x1004);

		/*    memcpy(hdesc->buffer + newlfofs + 4,
		((void *)newlf ? (void *)newlf : (void *)newli), 8 + (newlf ? 8 : 4) * no_keys);
		*/
		if (fill_block(hdesc, newlfofs,
			((void *)newlf ? (void *)newlf : (void *)newli), 8 + (newlf ? 8 : 4) * no_keys))
			goto error_fill_block;
	}
	else {  /* Last deleted, will throw away index */
		newlfofs = 0xfff;  /* We subtract 0x1000 later */
	}

	if (newlfofs < 0xfff) goto error_newlfofs;

	/* Check for CLASS data, if so, deallocate it too */
	if (delnk != NULL && delnk->len_classnam) {
		free_block(hdesc, delnk->ofs_classnam + 0x1000);
	}
	/* Now it's safe to zap the nk */
	free_block(hdesc, delnkofs + 0x1000);
	/* And the old index list */
	free_block(hdesc, (oldlfofs ? oldlfofs : oldliofs) + 0x1000);

	/* Update parent */
	key->no_subkeys--;

	key = (struct nk_key *)(hdesc->buffer + nkofs);
	oldli = (struct li_key *)(hdesc->buffer + oldliofs + 0x1004);
	oldlf = (struct lf_key *)(hdesc->buffer + oldlfofs + 0x1004);

	if (ri) {
		ri = (struct ri_key *)(hdesc->buffer + riofs + 0x1004); /* In case realloc */

		if (newlfofs == 0xfff) {

			*fullpath = 0;
			get_abs_path(hdesc, nkofs, fullpath, 480);

			if (ri->no_lis > 1) {  /* We have subindiceblocks left? */
				/* Delete from array */
				ALLOC(newri, 8 + 4 * ri->no_lis - 4, 1);
				newri->no_lis = ri->no_lis - 1;
				newri->id = ri->id;
				for (o = 0, n = 0; o < ri->no_lis; o++, n++) {
					if (n == rislot) o++;
					newri->hash[n].ofs_li = ri->hash[o].ofs_li;
				}
				newriofs = alloc_block(hdesc, nkofs, 8 + newri->no_lis * 4);
				if (!newriofs) goto error_alloc_ri_index;
				key = (struct nk_key *)(hdesc->buffer + nkofs);
				oldli = (struct li_key *)(hdesc->buffer + oldliofs + 0x1004);
				oldlf = (struct lf_key *)(hdesc->buffer + oldlfofs + 0x1004);

				if (fill_block(hdesc, newriofs, newri, 8 + newri->no_lis * 4)) goto error_fill_block;
				free_block(hdesc, riofs + 0x1000);
				key->ofs_lf = newriofs - 0x1000;
				free(newri);
			}
			else { /* Last entry in ri was deleted, get rid of it, key is empty */
				free_block(hdesc, riofs + 0x1000);
				key->ofs_lf = -1;
			}
		}
		else {
			ri->hash[rislot].ofs_li = newlfofs - 0x1000;
		}
	}
	else {
		key->ofs_lf = newlfofs - 0x1000;
	}

	free(newlf);
	return 0;

error_not_nk:
	LOG("del_key: current pointer not 'nk'\n");
	return 1;
error_no_subkeys:
	LOG("del_key: key has no subkeys\n");
	return 1;
error_index:
	LOG("del_key: index other than 'lf', 'li' or 'lh' not supported: 0x%04x\n", oldlf->id);
	return 1;
error_subkey_not_found:
	LOG("del_key: subkey not found: %s\n", name);
	free(newlf);
	free(newli);
	return 1;
error_not_empty:
	LOG("del_key: subkey %s has subkeys or values. Not deleted.\n", name);
	free(newlf);
	free(newli);
	return 1;
error_alloc_key:
	free(newlf);
	LOG("del_key: not deleted: can't alloc space for new key descriptor for %s\n", name);
	return 1;
error_newlfofs:
	LOG("del_key: error: newlfofs = %x\n", newlfofs);
	return 1;
error_alloc_ri_index:
	free(newlf);
	free(newri);
	LOG("del_key: not deleted: unable to allocate space for ri-index for %s\n", name);
	return 1;
error_dirty:
	LOG("del_key: mark_pages_dirty returned an error\n");
	return 1;
error_fill_block:
	LOG("del_key: fill_block failed\n");
	return 1;
}


/* Write to registry value.
* If same size as existing, copy back in place to avoid changing too much
* otherwise allocate new dataspace, then free the old
* Thus enough space to hold both new and old data is needed
* Pass in buffer with data len as first DWORD (as routines above)
* returns: 0 - error, len - OK (len of data)
*/

int put_buf2val(struct hive *hdesc, struct keyval *kv,
	int vofs, const char *path, int type, int exact)
{
	int l;
	void *keydataptr, *addr;
	struct db_key *db;
	int32_t copylen, blockofs, blocksize, restlen, point, list, parts;
	int i;

	LOAD_WD_LOGONLY();

	if (!kv) goto error_null_kv;
	l = get_val_len(hdesc, vofs, path, exact);
	if (l == -1) goto error_get_val_len;

	if (kv->len != l) {  /* Realloc data block if not same size as existing */
		if (!alloc_val_data(hdesc, vofs, path, kv->len, exact)) {
			LOG("put_buf2val: alloc_val_data failed: %s\n", path);
			return 0;
		}
	}

	keydataptr = get_val_data(hdesc, vofs, path, type, exact);
	if (!keydataptr) {
		LOG("put_buf2val: get_val_data failed: %s\n", path);
		return 0;
	}

	if (kv->len > VAL_DIRECT_LIMIT) {       /* Where do the db indirects start? seems to be around 16k */
		db = (struct db_key *)keydataptr;
		if (db->id != 0x6264) goto error_db_not_found;
		parts = db->no_part;
		list = db->ofs_data + 0x1004;
		LOG("put_buf2val: long value: parts %d, list %x\n", parts, list);

		point = 0;
		restlen = kv->len;
		for (i = 0; i < parts; i++) {
			blockofs = get_int32(hdesc->buffer + list + (i << 2)) + 0x1000;
			blocksize = -get_int32(hdesc->buffer + blockofs) - 8;

			/* Copy this part, up to size of block or rest length in last block */
			copylen = (blocksize > restlen) ? restlen : blocksize;
			addr = (char *)(kv->data) + point;
			if (fill_block(hdesc, blockofs, addr, copylen)) goto error_fill_block;

			point += copylen;
			restlen -= copylen;
		}
	}
	else {
		memcpy(keydataptr, kv->data, kv->len);
		i = (uintptr_t)keydataptr - (uintptr_t)(hdesc->buffer);
		if (mark_pages_dirty(hdesc, i, i + kv->len)) return -1;
	}

	if (flush_dirty_pages(hdesc)) return 0;

	return (kv->len);

error_fill_block:
	LOG("put_buf2val: fill_block failed\n");
	return 0;
error_db_not_found:
	LOG("put_buf2val: data block expected but not found: 0x%" PRIxPTR "\n",
		(intptr_t)db - (intptr_t)hdesc->buffer);
	return 0;
error_get_val_len:
	LOG("put_buf2val: get_val_len failed for %s offset %d\n", path, vofs);
	return 0;
error_null_kv:
	LOG("put_buf2val: NULL key value pointer given\n");
	return 0;
}

/* And, yer basic DWORD write */

/*int put_dword(struct hive *hdesc, int vofs, const char *path, int exact, int dword)
{
struct keyval *kr;
int r;

ALLOC(kr,1,16);

kr->len = 4;
*(kr->data) = dword;

r = put_buf2val(hdesc, kr, vofs, path, REG_DWORD, exact);
free(kr);
return r;
}*/

/* ================================================================ */

/* Code to export registry entries to .reg file initiated by
* Leo von Klenze
* Then expanded a bit to handle more types etc.
*/

/*
* converts a value string from an registry entry into a c string. It does not
* use any encoding functions.
* It works very primitive by just taking every second char.
* The caller must free the resulting string, that was allocated with malloc.
*
* cstring: the destination string
* string:  string where every second char is \0
* len:     length of the string
* return:  the converted string as char*
*/
void string_regw2prog(char * const  cstring, const void * const  string, int len)
{
	int i, k;
	int out_len = 0;

	for (i = 0; i < len; i += 2)
	{
		unsigned v = ((unsigned char *)string)[i] + ((unsigned char *)string)[i + 1] * 256u;
		if (v < 128)
			out_len += 1;
		else if (v < 0x800)
			out_len += 2;
		else
			out_len += 3;
	}

	for (i = 0, k = 0; i < len; i += 2)
	{
		unsigned v = ((unsigned char *)string)[i] + ((unsigned char *)string)[i + 1] * 256u;
		if (v < 128)
			cstring[k++] = v;
		else if (v < 0x800) {
			cstring[k++] = 0xc0 | (v >> 6);
			cstring[k++] = 0x80 | (v & 0x3f);
		}
		else {
			cstring[k++] = 0xe0 | (v >> 12);
			cstring[k++] = 0x80 | ((v >> 6) & 0x3f);
			cstring[k++] = 0x80 | (v & 0x3f);
		}
	}
	cstring[out_len] = '\0';
	return;
}


/* Utility functions used elsewhere in the library */

static char *string_prog2regw(void *string, int len, int *out_len)
{
	unsigned char *regw = (unsigned char*)malloc(len * 2 + 2);
	unsigned char *out = regw;
	unsigned char *in = (unsigned char*)string;

	for (; len>0; ++in, --len) {
		if (!(in[0] & 0x80)) {
			*out++ = *in;
			*out++ = 0;
		}
		else if ((in[0] & 0xe0) == 0xc0 && len >= 2) {
			*out++ = (in[0] & 0x1f) << 6 | (in[1] & 0x3f);
			*out++ = (in[0] & 0x1f) >> 2;
			++in, --len;
		}
		else if (len >= 3) {
			/* assume 3 byte*/
			*out++ = (in[1] & 0xf) << 6 | (in[2] & 0x3f);
			*out++ = (in[0] & 0xf) << 4 | ((in[1] & 0x3f) >> 2);
			in += 2;
			len -= 2;
		}
	}
	*out_len = out - regw;
	out[0] = out[1] = 0;
	return (char *)regw;
}


int de_escape(char *s, int wide)
{
	int src = 0;
	int dst = 0;

	if (wide) {
		while (*(s + src) || *(s + src + 1)) {
			if (*(s + src) == '\\' && *(s + src + 1) == 0) src += 2; /* Skip over backslash */
			*(s + dst) = *(s + src);
			*(s + dst + 1) = *(s + src + 1);
			dst += 2;
			src += 2;
		}
		*(s + dst) = 0;
		*(s + dst + 1) = 0;
		dst += 2;
	}
	else {
		while (*(s + src)) {
			if (*(s + src) == '\\') src++;
			*(s + dst) = *(s + src);
			dst++;
			src++;
		}
		*(s + dst) = 0;
		dst++;
	}

	return dst;
}


/* ================================================================ */
/* Hive control (load/save/close) etc */

void close_hive(struct hive *hdesc)
{
	if (hdesc->state & HMODE_OPEN) {
		fclose(hdesc->fp);
	}
	free(hdesc->filename);
	free(hdesc->buffer);
	free(hdesc);
}


/* Compute checksum of REGF header page
* hdesc = hive
* returns checksum value, 32 bit int
*/

static inline int32_t calc_regfsum(struct hive * const hdesc)
{
	int32_t checksum = 0;
	struct regf_header *  hdr;
	int i;

	hdr = (struct regf_header *) hdesc->buffer;

	for (i = 0; i < 0x1fc / 4; ++i)
		checksum ^= ((int32_t *)hdr)[i];

	return checksum;
}


/* Write the hive back to disk (only if dirty & not readonly) */
int write_hive(struct hive *hdesc)
{
	struct regf_header *hdr;

	LOAD_WD_LOGONLY();

	if (hdesc->state & HMODE_RO) goto error_ro;
	if (!(hdesc->state & HMODE_OPEN)) goto error_closed;
	/* Seek to beginning of file */
	fseek(hdesc->fp, 0L, SEEK_SET);

	/* compute new checksum */

	hdr = (struct regf_header *) hdesc->buffer;
	hdr->checksum = calc_regfsum(hdesc);
	if (mark_pages_dirty(hdesc, 0, 0)) goto error_dirty;
	if (flush_dirty_pages(hdesc)) goto error_flush;
	return 0;

error_ro:
	LOG("write_hive: Hive is read-only, cannot write\n");
	return 0;
error_closed:
	LOG("write_hive: cannot write to closed hive: %s\n", hdesc->filename);
	return 1;
error_dirty:
	LOG("write_hive: mark_pages_dirty returned an error\n");
	return -1;
error_flush:
	LOG("write_hive: flush_dirty_pages returned an error\n");
	return -1;
}

struct hive *open_hive(const char *filename, int mode)
{
	struct hive *hdesc;
	int r, vofs;
	const char *fmode = "r+b";
	struct stat sbuf;
	uint32_t pofs;
	int32_t checksum;
	struct hbin_page *p = NULL;
	struct regf_header *hdr;
	struct nk_key *nk;
	struct ri_key *rikey;

	LOAD_WD_LOGONLY();

	if (!filename || !*filename) {
		LOG("open_hive: Null or empty filename given\n");
		return NULL;
	}

	CREATE(hdesc, struct hive, 1);

	hdesc->filename = str_dup(filename);
	hdesc->state = 0;
	hdesc->size = 0;
	hdesc->buffer = NULL;
	hdesc->dirty_entries = 0;

	/* Disable writing if read-only mode requested */
	if (mode & HMODE_RO)
		fmode = "rb";

	if (stat(hdesc->filename, &sbuf)) {
		LOG("open_hive: cannot stat hive %s\n", hdesc->filename);
		return NULL;
	}

	hdesc->fp = fopen(hdesc->filename, fmode);
	if (hdesc->fp == NULL && ((mode & HMODE_RO)==0)) {
		LOG("open_hive: failed: %s: %s; trying read-only\n", strerror(errno), hdesc->filename);
		fmode = "rb";
		mode |= HMODE_RO;
		hdesc->fp = fopen(hdesc->filename, fmode);
		if (hdesc->fp == NULL) {
			LOG("open_hive: read-only failed: %s: %s\n", strerror(errno), hdesc->filename);
			close_hive(hdesc);
			return NULL;
		}
	}

	hdesc->size = sbuf.st_size;
	hdesc->state = mode | HMODE_OPEN;

	/* Read the whole file */

	ALLOC(hdesc->buffer, 1, hdesc->size);

	errno = 0;
	r = fread(hdesc->buffer, hdesc->size, 1, hdesc->fp);
	if (!r) {
		LOG("open_hive: read failed: %s: %s\n", strerror(errno), hdesc->filename);
		close_hive(hdesc);
		return NULL;
	}

	/* Now run through file, tallying all pages */
	/* NOTE/KLUDGE: Assume first page starts at offset 0x1000 */

	pofs = 0x1000;
	hdesc->pages = hdesc->size >> 12;

	hdr = (struct regf_header *)hdesc->buffer;
	if (hdr->id != 0x66676572) {
		LOG("open_hive: not a registry file: %s\n", filename);
		return hdesc;
	}

	checksum = calc_regfsum(hdesc);
	if (checksum != hdr->checksum) {
		LOG("open_hive: header checksum mismatch: calc %08x, hdr %08x\n",
			checksum, hdr->checksum);
	}

	hdesc->rootofs = hdr->ofs_rootkey + 0x1000;

	/* Cache the roots subkey index type (li,lf,lh) so we can use the correct
	* one when creating the first subkey in a key */

	nk = (struct nk_key *)(hdesc->buffer + hdesc->rootofs + 4);
	if (nk->id == 0x6b6e) {
		rikey = (struct ri_key *)(hdesc->buffer + nk->ofs_lf + 0x1004);
		hdesc->nkindextype = rikey->id;
		if (hdesc->nkindextype == 0x6972) {  /* Gee, big root, must check indirectly */
			rikey = (struct ri_key *)(hdesc->buffer + rikey->hash[0].ofs_li + 0x1004);
			hdesc->nkindextype = rikey->id;
		}
		if (hdesc->nkindextype != 0x666c &&
			hdesc->nkindextype != 0x686c &&
			hdesc->nkindextype != 0x696c) {
			hdesc->nkindextype = 0x666c;
		}

	}
	else {
		LOG("open_hive: Root key is not a key (not of type 'nk')\n");
	}

	while (pofs < (unsigned int)(hdr->filesize + 0x1000)) {   /* Loop through hbins until end according to regf header */
		p = (struct hbin_page *)(hdesc->buffer + pofs);
		if (p->id != 0x6E696268) {
			break;
		}
		if (p->ofs_next == 0) {
			LOG("open_hive: Corrupt file: zero-size page at %x\n", pofs);
			return hdesc;
		}

		vofs = pofs + 0x20; /* Skip page header, and run through blocks in hbin */

		while ((int)(vofs - pofs) < p->ofs_next && vofs < hdesc->size) {
			vofs += parse_block(hdesc, vofs);
		}
		pofs += p->ofs_next;
	} /* hbin loop */

	hdesc->endofs = hdr->filesize + 0x1000;
	hdesc->lastbin = pofs - p->ofs_next;  /* Compensate for loop that added at end above */

	return hdesc;
}

/* 
* Get and copy keys CLASS-data (if any) to buffer
* Returns a buffer with the data (first int32_t is size). see ntreg.h
* NOTE: caller must deallocate buffer! a simple free(keyval) will suffice.
*/
struct keyval *get_class(struct hive *hdesc, int curnk, const char *path)
{
	int clen = 0, dofs = 0, nkofs;
	struct nk_key *key;
	struct keyval *data;
	void *classdata;

	if (!path && !curnk) return(NULL);

	nkofs = trav_path(hdesc, curnk, path, 0);

	if (!nkofs) {
		return(NULL);
	}
	nkofs += 4;
	key = (struct nk_key *)(hdesc->buffer + nkofs);

	clen = key->len_classnam;
	if (!clen) {
		return(NULL);
	}

	dofs = key->ofs_classnam;
	classdata = (void *)(hdesc->buffer + dofs + 0x1004);

	ALLOC(data, sizeof(struct keyval) + clen, 1);
	data->len = clen;
	memcpy(&data->data, classdata, clen);
	return(data);
}


