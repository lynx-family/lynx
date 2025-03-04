#ifndef ALLOCATE_H
#define ALLOCATE_H
#include <errno.h>
#ifndef _WIN32
#include <pthread.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if defined(ANDROID) || defined(__ANDROID__) || defined(OS_IOS)
#define THREAD_NUM 3
#define CREATE_THREAD_NUM 2
#else
#define THREAD_NUM 6
#define CREATE_THREAD_NUM 6
#endif

typedef uint64_t binmap_t;
typedef uint64_t bindex_t;
typedef uint64_t flag_t;
struct malloc_tree_chunk {
  size_t prev_foot;
  size_t head;
  struct malloc_tree_chunk* fd;
  struct malloc_tree_chunk* bk;

  struct malloc_tree_chunk* child[2];
  struct malloc_tree_chunk* parent;
  bindex_t index;
};
typedef struct malloc_tree_chunk* tbinptr;
struct malloc_segment {
  char* base;
  size_t size;
  struct malloc_segment* next;
  flag_t sflags;
};
typedef struct malloc_segment msegment;
typedef struct malloc_segment* msegmentptr;
struct malloc_chunk {
  size_t prev_foot;
  size_t head;
  struct malloc_chunk* fd;
  struct malloc_chunk* bk;
};

typedef struct malloc_chunk mchunk;
typedef struct malloc_chunk* mchunkptr;

#ifndef USE_LOCKS
#define USE_LOCKS 0
#endif
#define NSMALLBINS (32U)
#define NTREEBINS (32U)
struct malloc_state {
  binmap_t smallmap;
  binmap_t local_smallmap[THREAD_NUM];
  binmap_t treemap;
  binmap_t local_treemap[THREAD_NUM];
  size_t dvsize;
  size_t topsize;
  char* least_addr;
  mchunkptr dv;
  mchunkptr top;
  size_t trim_check;
  char* mmap_cache;
  size_t mmap_cache_size;
  size_t release_checks;
  size_t magic;
  mchunkptr* smallbins;
  mchunkptr* local_smallbins[THREAD_NUM];
  tbinptr* treebins;
  tbinptr* local_treebins[THREAD_NUM];
  size_t footprint;
  size_t max_footprint;
  size_t footprint_limit;
  flag_t mflags;
#if USE_LOCKS
  MLOCK_T mutex;
#endif
  msegment seg;
  size_t exts;
  void** mmap_array;
  uint32_t mmap_free_index;
  uint32_t mmap_size;
  uint32_t mmap_count;
  size_t cur_malloc_size;  // malloc size after gc
  size_t footprint_before_gc;
#ifdef ENABLE_TRACING_GC_LOG
  size_t malloc_size_before_gc;
  size_t malloc_size_after_gc;
  size_t finalizer_time;
  size_t free_time;
  size_t free_set_bit_time;
  size_t free_gene_freelist_time;
  size_t free_mmap_chunk_time;
  size_t release_time;
  size_t release_seg_num;
#endif
  void* runtime;
  void* pool;
  int gc_flag[THREAD_NUM];
  int local_idx_flag[THREAD_NUM];
#ifndef _WIN32
  pthread_mutex_t mtx;
#endif
  size_t seg_count;
  bool open_madvise;
#if defined(ANDROID) || defined(__ANDROID__) || defined(OS_IOS)
  char mem_name[30];
#endif
};
typedef struct malloc_state* mstate;
void* allocate(mstate m, size_t bytes);
void gcfree(mstate m, void* mem);
void* reallocate(mstate m, void* oldmem, size_t bytes);
size_t allocate_usable_size(void* mem);
size_t allocate_usable_size_debug(void* mem);
size_t allocate_usable_size_debug_protect(void* mem);
void destroy_allocate_instance(mstate m);

int atomic_acqurie_local_idx(mstate m);
void atomic_release_local_idx(mstate m, int local_idx);

void set_mark_multi(void* ptr);
bool is_marked_multi(void* ptr);
void set_alloc_tag(void* ptr, int alloc_tag);
int get_alloc_tag(void* ptr);
void set_hash_size(void* ptr, int hash_size);
int get_hash_size(void* ptr);
void set_heap_obj_len(void* ptr, int len);
int get_heap_obj_len(void* ptr);

#ifdef ENABLE_GC_DEBUG_TOOLS
#ifdef __cplusplus
extern "C" {
#endif
void delete_cur_mems(void* runtime, void* ptr);
void multi_delete_cur_mems(void* runtime, void* ptr, int local_idx);
void merge_mems(void* runtime);
void add_cur_mems(void* runtime, void* ptr);
#ifdef __cplusplus
}
#endif
#endif

int64_t get_daytime();
#ifdef ENABLE_TRACING_GC_LOG
size_t get_malloc_size(mstate m);
#endif

#define SIZE_T_SIZE (sizeof(size_t))
#define INT_SIZE (sizeof(int))
#define SIZE_T_BITSIZE (sizeof(size_t) << 3)

#define SIZE_T_ZERO ((size_t)0)
#define SIZE_T_ONE ((size_t)1)
#define SIZE_T_TWO ((size_t)2)
#define SIZE_T_FOUR ((size_t)4)
#define TWO_SIZE_T_SIZES (SIZE_T_SIZE << 1)
#define FOUR_SIZE_T_SIZES (SIZE_T_SIZE << 2)
#define SIX_SIZE_T_SIZES (FOUR_SIZE_T_SIZES + TWO_SIZE_T_SIZES)
#define HALF_MAX_SIZE_T (MAX_SIZE_T / 2U)

#define PINUSE_BIT (SIZE_T_ONE)
#define CINUSE_BIT (SIZE_T_TWO)
#define FLAG4_BIT (SIZE_T_FOUR)
#define INUSE_BITS (PINUSE_BIT | CINUSE_BIT)
#define FLAG_BITS (PINUSE_BIT | CINUSE_BIT | FLAG4_BIT)

#define FENCEPOST_HEAD (INUSE_BITS | SIZE_T_SIZE)

#define chunk_plus_offset(p, s) ((mchunkptr)(((char*)(p)) + (s)))
#define chunk_minus_offset(p, s) ((mchunkptr)(((char*)(p)) - (s)))

#define next_chunk(p) ((mchunkptr)(((char*)(p)) + ((p)->head & ~FLAG_BITS)))
#define prev_chunk(p) ((mchunkptr)(((char*)(p)) - ((p)->prev_foot)))

#define next_pinuse(p) ((next_chunk(p)->head) & PINUSE_BIT)

#define get_foot(p, s) (((mchunkptr)((char*)(p) + (s)))->prev_foot)
#define set_foot(p, s) (((mchunkptr)((char*)(p) + (s)))->prev_foot = (s))

#define set_size_and_pinuse_of_free_chunk(p, s) \
  ((p)->head = (s | PINUSE_BIT), set_foot(p, s))

#define set_free_with_pinuse(p, s, n) \
  (clear_pinuse(n), set_size_and_pinuse_of_free_chunk(p, s))

#define overhead_for(p) (is_mmapped(p) ? MMAP_CHUNK_OVERHEAD : CHUNK_OVERHEAD)

#if defined(DARWIN) || defined(_DARWIN)
#define HAVE_MMAP 1
#ifndef MALLOC_ALIGNMENT
#define MALLOC_ALIGNMENT ((size_t)16U)
#endif
#endif /* DARWIN */

#ifndef MALLOC_ALIGNMENT
#if defined(__x86_64__) || defined(__aarch64__)
#define MALLOC_ALIGNMENT ((size_t)(1 * sizeof(void*)))
#else
#define MALLOC_ALIGNMENT ((size_t)(2 * sizeof(void*)))
#endif
#endif /* MALLOC_ALIGNMENT */

#ifndef FOOTERS
#define FOOTERS 0
#endif /* FOOTERS */

#if FOOTERS
#define CHUNK_OVERHEAD (TWO_SIZE_T_SIZES + 2 * INT_SIZE)
#else /* FOOTERS */
#define CHUNK_OVERHEAD (SIZE_T_SIZE + 2 * INT_SIZE)
#endif /* FOOTERS */

#define CHUNK_ALIGN_MASK (MALLOC_ALIGNMENT - SIZE_T_ONE)

#define pad_request(req) \
  (((req) + CHUNK_OVERHEAD + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)

#define align_offset(A)                                           \
  ((((size_t)(A) & CHUNK_ALIGN_MASK) == 0)                        \
       ? 0                                                        \
       : ((MALLOC_ALIGNMENT - ((size_t)(A) & CHUNK_ALIGN_MASK)) & \
          CHUNK_ALIGN_MASK))
#define align_as_chunk(A) (mchunkptr)((A) + align_offset(chunk2mem(A)))

#define chunk2mem(p) ((void*)((char*)(p) + (TWO_SIZE_T_SIZES + 2 * INT_SIZE)))

#define segment_holds(S, A) \
  ((char*)(A) >= S->base && (char*)(A) < S->base + S->size)

#define cinuse(p) ((p)->head & CINUSE_BIT)
#define pinuse(p) ((p)->head & PINUSE_BIT)
#define flag4inuse(p) ((p)->head & FLAG4_BIT)
#define is_inuse(p) (((p)->head & INUSE_BITS) != PINUSE_BIT)
#define is_mmapped(p) (((p)->head & INUSE_BITS) == 0)

#define chunksize(p) ((p)->head & ~(FLAG_BITS))

#define clear_pinuse(p) ((p)->head &= ~PINUSE_BIT)
#define set_flag4(p) ((p)->head |= FLAG4_BIT)
#define clear_flag4(p) ((p)->head &= ~FLAG4_BIT)

#define mem2chunk(mem) \
  ((mchunkptr)((char*)(mem) - (TWO_SIZE_T_SIZES + 2 * INT_SIZE)))

#define IS_UNUSED_BIT (16U)
void init_bins(mstate m);
void local_gcfree(mstate fm, void* mem, int local_idx);
void* mmap_set_free(uint32_t v);

size_t release_unused_segments(mstate m);

bool mmap_is_free(const void* p);

bool is_marked(void* ptr);
void clear_mark(void* ptr);
int get_tag(void* ptr);
void local_insert_chunk(mstate m, mchunkptr mchunk, size_t size, int local_idx);

#endif  // ALLOCATE_H
