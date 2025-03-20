/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>             // standard definition
#include <stdbool.h>            // standard boolean
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "fr30xx.h"
#include "heap.h"

/*
 * MACRO DEFINITIONS
 ****************************************************************************************
 */
/// Pattern used to check if memory block is not corrupted
#define HEAP_LIST_PATTERN           (0xA55AA55A)
#define HEAP_ALLOCATED_PATTERN      (0x83388338)
#define HEAP_FREE_PATTERN           (0xF00FF00F)

#define CO_ALIGN4_HI(val)           (((val)+3)&~3)

#define HEAP_MEM_DEBUG              1

#if HEAP_MEM_DEBUG
#if defined (__ARMCC_VERSION) || defined(__ICCARM__)
/// Assertions showing a critical error that could require a full system reset
#define ASSERT_ERR(cond)                              \
    do {                                              \
        if (!(cond)) {                                \
            assert_err(#cond, __FILE__, __LINE__);  \
        }                                             \
    } while(0)

/// Assertions showing a critical error that could require a full system reset
#define ASSERT_INFO(cond, param0, param1)             \
    do {                                              \
        if (!(cond)) {                                \
            assert_param((int)param0, (int)param1, __FILE__, __LINE__);  \
        }                                             \
    } while(0)

/// Assertions showing a non-critical problem that has to be fixed by the SW
#define ASSERT_WARN(cond, param0, param1)             \
    do {                                              \
        if (!(cond)) {                                \
            assert_warn((int)param0, (int)param1, __FILE__, __LINE__); \
        }                                             \
    } while(0)
#elif defined(__CC_ARM) || defined(__GNUC__)
/// Assertions showing a critical error that could require a full system reset
#define ASSERT_ERR(cond)                              \
    do {                                              \
        if (!(cond)) {                                \
            assert_err(#cond, __MODULE__, __LINE__);  \
        }                                             \
    } while(0)

/// Assertions showing a critical error that could require a full system reset
#define ASSERT_INFO(cond, param0, param1)             \
    do {                                              \
        if (!(cond)) {                                \
            assert_param((int)param0, (int)param1, __MODULE__, __LINE__);  \
        }                                             \
    } while(0)

/// Assertions showing a non-critical problem that has to be fixed by the SW
#define ASSERT_WARN(cond, param0, param1)             \
    do {                                              \
        if (!(cond)) {                                \
            assert_warn((int)param0, (int)param1, __MODULE__, __LINE__); \
        }                                             \
    } while(0)
#else
#error "Unsupported platform"
#endif
#else
/// Assertions showing a critical error that could require a full system reset
#define ASSERT_ERR(cond)

/// Assertions showing a critical error that could require a full system reset
#define ASSERT_INFO(cond, param0, param1)

/// Assertions showing a non-critical problem that has to be fixed by the SW
#define ASSERT_WARN(cond, param0, param1)

/// DUMP data array present in the SW.
#define DUMP_DATA(data, length)
#endif //PLF_DEBUG

/*
 * LOCAL TYPE DEFINITIONS
 ****************************************************************************************
 */
/// Free memory block delimiter structure (size must be word multiple)
/// Heap can be represented as a doubled linked list.
struct mblock_free
{
    /// Used to check if memory block has been corrupted or not
    uint32_t corrupt_check;
    /// Size of the current free block (including delimiter)
    uint32_t free_size;

#if HEAP_MEM_DEBUG
    uint32_t return_addr;
#endif

    /// Next free block pointer
    struct mblock_free* next;
    /// Previous free block pointer
    struct mblock_free* previous;
};

/// Used memory block delimiter structure (size must be word multiple)
struct mblock_used
{
    /// Used to check if memory block has been corrupted or not
    uint32_t corrupt_check;
    /// Size of the current used block (including delimiter)
    uint32_t size;

#if HEAP_MEM_DEBUG
    uint32_t return_addr;
#endif
};

/// heap environment definition
struct heap_env_tag
{
    /// Root pointer = pointer to first element of heap linked lists
    struct mblock_free * heap[HEAP_TYPE_BLOCKS];
    /// Size of heaps
    uint32_t heap_size[HEAP_TYPE_BLOCKS];

#if (HEAP_MEM_DEBUG)
    /// Size of heap used
    uint32_t heap_used[HEAP_TYPE_BLOCKS];
    /// Maximum heap memory used
    uint32_t max_heap_used_single[HEAP_TYPE_BLOCKS];
    /// Maximum heap memory used
    uint32_t max_heap_used;
#endif //HEAP_MEM_DEBUG
};

/// Heap environment
struct heap_env_tag heap_env_app;

#undef configTOTAL_HEAP_SIZE
#define configTOTAL_HEAP_SIZE       0x19000

#undef configTOTAL_DRAM_SIZE
#define configTOTAL_DRAM_SIZE       0x5000

static uint32_t ucHeap[ (configTOTAL_HEAP_SIZE >> 2)];
static __attribute__((section("dram_section"))) uint32_t ucHeap_dram[ (configTOTAL_DRAM_SIZE >> 2)];

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static void assert_err(const char *condition, const char * file, int line)
{
//    printf("%s[%d]: %s\r\n", file, line, condition);
    while(1);
}

static void assert_param(int param0, int param1, const char * file, int line)
{
//    printf("%s[%d]: 0x%08x, 0x%08x\r\n", file, line, param0, param1);
    while(1);
}

static void assert_warn(int param0, int param1, const char * file, int line)
{
//    printf("%s[%d]: 0x%08x, 0x%08x\r\n", file, line, param0, param1);
    while(1);
}

/**
 * Check if memory pointer is within heap address range
 *
 * @param[in] type Memory type.
 * @param[in] mem_ptr Memory pointer
 * @return True if it's in memory heap, False else.
 */
static bool mem_is_in_heap(uint8_t type, void* mem_ptr)
{
    bool ret = false;
    uint8_t* block = (uint8_t*)heap_env_app.heap[type];
    uint32_t size = heap_env_app.heap_size[type];

    if((((uint32_t)mem_ptr) >= ((uint32_t)block))
            && (((uint32_t)mem_ptr) <= (((uint32_t)block) + size)))
    {
        ret = true;
    }

    return ret;
}

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void heap_mem_init(uint8_t type, uint8_t* heap, uint32_t heap_size)
{
    // align first free descriptor to word boundary
    heap_env_app.heap[type] =  (struct mblock_free*)CO_ALIGN4_HI((uint32_t)heap);

    GLOBAL_INT_DISABLE();

    // initialize the first block
    //  + compute the size from the last aligned word before heap_end
    heap_env_app.heap[type]->free_size = ((uint32_t)&heap[heap_size] & (~3)) - (uint32_t)(heap_env_app.heap[type]);
    heap_env_app.heap[type]->corrupt_check = HEAP_LIST_PATTERN;
    heap_env_app.heap[type]->next = NULL;
    heap_env_app.heap[type]->previous = NULL;

    heap_env_app.heap_size[type] = heap_size;

    GLOBAL_INT_RESTORE();
}

__RAM_CODE __attribute__((noinline)) void *heap_mem_alloc(uint8_t type, uint32_t size)
{
    struct mblock_free *node = NULL,*found = NULL;
    uint8_t cursor = 0;
    struct mblock_used *alloc = NULL;
    uint32_t totalsize;

#if HEAP_MEM_DEBUG
    volatile uint32_t address;
    __asm("MOV %[result], LR":[result] "=r" (address));
#endif

    // compute overall block size (including requested size PLUS descriptor size)
    totalsize = CO_ALIGN4_HI(size) + sizeof(struct mblock_used);
    if(totalsize < sizeof(struct mblock_free))
    {
        totalsize = sizeof(struct mblock_free);
    }

    // sanity check: the totalsize should be large enough to hold free block descriptor
    ASSERT_ERR(totalsize >= sizeof(struct mblock_free));

    // protect accesses to descriptors
    GLOBAL_INT_DISABLE();

#if HEAP_MEM_DEBUG
    uint32_t totalfreesize = 0;
#endif //HEAP_MEM_DEBUG

    // Select Heap to use, first try to use current heap.
    node = heap_env_app.heap[type];
    ASSERT_ERR(node != NULL);

    // go through free memory blocks list
    while (node != NULL)
    {
        ASSERT_ERR(node->corrupt_check == HEAP_LIST_PATTERN);
#if HEAP_MEM_DEBUG
        totalfreesize += node->free_size;
#endif //HEAP_MEM_DEBUG

        // check if there is enough room in this free block
        if (node->free_size >= (totalsize))
        {
            if ((node->free_size >= (totalsize + sizeof(struct mblock_free)))
                    || (node->previous != NULL))
            {
                // if a match was already found, check if this one is smaller
                if ((found == NULL) || (found->free_size > node->free_size))
                {
                    found = node;
                }
            }
        }

        // move to next block
        node = node->next;
    }

    // Update size to use complete list if possible.
    if(found != NULL)
    {
        if (found->free_size < (totalsize + sizeof(struct mblock_free)))
        {
            totalsize = found->free_size;
        }
    }

#if HEAP_MEM_DEBUG
    heap_env_app.heap_used[type] = heap_env_app.heap_size[type] - totalfreesize;
    if(found != NULL)
    {
        heap_env_app.heap_used[type] += totalsize;
        if (heap_env_app.max_heap_used_single[type] < heap_env_app.heap_used[type])
        {
            heap_env_app.max_heap_used_single[type] = heap_env_app.heap_used[type];
        }
    }

    {
        // calculate max used size
        uint32_t totalusedsize = 0;
        for(cursor = 0 ; cursor < HEAP_TYPE_BLOCKS ; cursor ++)
        {
            totalusedsize +=  heap_env_app.heap_used[cursor];
        }

        if(heap_env_app.max_heap_used < totalusedsize)
        {
            heap_env_app.max_heap_used = totalusedsize;
        }
    }
#endif //HEAP_MEM_DEBUG

    // Re-boot platform if no more empty space
    if(found == NULL)
    {
        heap_dump_used_mem(HEAP_TYPE_SRAM_BLOCK);
        heap_dump_used_mem(HEAP_TYPE_DRAM_BLOCK);
        heap_dump_used_mem(HEAP_TYPE_BTDM_BLOCK);
        while(1);
    }
    else
    {
        // sublist completely reused
        if (found->free_size == totalsize)
        {
            ASSERT_ERR(found->previous != NULL);

            // update double linked list
            found->previous->next = found->next;
            if(found->next != NULL)
            {
                found->next->previous = found->previous;
            }

            // compute the pointer to the beginning of the free space
            alloc = (struct mblock_used*) ((uint32_t)found);
        }
        else
        {
            // found a free block that matches, subtract the allocation size from the
            // free block size. If equal, the free block will be kept with 0 size... but
            // moving it out of the linked list is too much work.
            found->free_size -= totalsize;

            // compute the pointer to the beginning of the free space
            alloc = (struct mblock_used*) ((uint32_t)found + found->free_size);
        }

        // save the size of the allocated block
        alloc->size = totalsize;
        alloc->corrupt_check = HEAP_ALLOCATED_PATTERN;

#if HEAP_MEM_DEBUG
        alloc->return_addr = address;
#endif

        // move to the user memory space
        alloc++;
    }

    // end of protection (as early as possible)
    GLOBAL_INT_RESTORE();
    ASSERT_ERR(node == NULL);

    return (void*)alloc;
}

__RAM_CODE __attribute__((noinline)) void heap_mem_free(void* mem_ptr)
{
    struct mblock_free *freed;
    struct mblock_used *bfreed;
    struct mblock_free *node, *next_node, *prev_node;
    uint32_t size;
    uint8_t cursor = 0;

#if HEAP_MEM_DEBUG
    volatile uint32_t address;
    __asm("MOV %[result], LR":[result] "=r" (address));
#endif

    // sanity checks
    ASSERT_INFO(mem_ptr != NULL, mem_ptr, 0);

    // point to the block descriptor (before user memory so decrement)
    bfreed = ((struct mblock_used *)mem_ptr) - 1;

    // check if memory block has been corrupted or not
    ASSERT_INFO(bfreed->corrupt_check == HEAP_ALLOCATED_PATTERN, bfreed->corrupt_check, mem_ptr);
    // change corruption token in order to know if buffer has been already freed.
    bfreed->corrupt_check = HEAP_FREE_PATTERN;

    // point to the first node of the free elements linked list
    size = bfreed->size;
    node = NULL;

    freed = ((struct mblock_free *)bfreed);
#if HEAP_MEM_DEBUG
    freed->return_addr = address;
#endif

    // protect accesses to descriptors
    GLOBAL_INT_DISABLE();

    // Retrieve where memory block comes from
    while(((cursor < HEAP_TYPE_BLOCKS)) && (node == NULL))
    {
        if(mem_is_in_heap(cursor, mem_ptr))
        {
            // Select Heap to use, first try to use current heap.
            node = heap_env_app.heap[cursor];
        }
        else
        {
            cursor ++;
        }
    }

    // sanity checks
    ASSERT_ERR(node != NULL);
    ASSERT_ERR(((uint32_t)mem_ptr > (uint32_t)node));

    prev_node = NULL;

    while(node != NULL)
    {
        ASSERT_ERR(node->corrupt_check == HEAP_LIST_PATTERN);
        // check if the freed block is right after the current block
        if ((uint32_t)freed == ((uint32_t)node + node->free_size))
        {
            // append the freed block to the current one
            node->free_size += size;

            // check if this merge made the link between free blocks
            if (((uint32_t) node->next) == (((uint32_t)node) + node->free_size))
            {
                next_node = node->next;
                // add the size of the next node to the current node
                node->free_size += next_node->free_size;
                // update the next of the current node
                ASSERT_ERR(next_node != NULL);
                node->next = next_node->next;
                // update linked list.
                if(next_node->next != NULL)
                {
                    next_node->next->previous = node;
                }
            }
            goto free_end;
        }
        else if ((uint32_t)freed < (uint32_t)node)
        {
            // sanity check: can not happen before first node
            ASSERT_ERR(prev_node != NULL);

            // update the next pointer of the previous node
            prev_node->next = freed;
            freed->previous = prev_node;

            freed->corrupt_check = HEAP_LIST_PATTERN;

            // check if the released node is right before the free block
            if (((uint32_t)freed + size) == (uint32_t)node)
            {
                // merge the two nodes
                freed->next = node->next;
                if(node->next != NULL)
                {
                    node->next->previous = freed;
                }
                freed->free_size = node->free_size + size;
            }
            else
            {
                // insert the new node
                freed->next = node;
                node->previous = freed;
                freed->free_size = size;
            }
            goto free_end;
        }

        // move to the next free block node
        prev_node = node;
        node = node->next;

    }

    freed->corrupt_check = HEAP_LIST_PATTERN;

    // if reached here, freed block is after last free block and not contiguous
    prev_node->next = (struct mblock_free*)freed;
    freed->next = NULL;
    freed->previous = prev_node;
    freed->free_size = size;
    freed->corrupt_check = HEAP_LIST_PATTERN;

free_end:

#if HEAP_MEM_DEBUG
    heap_env_app.heap_used[cursor] -= size;
#endif //HEAP_MEM_DEBUG

    // end of protection
    GLOBAL_INT_RESTORE();
}

#if (HEAP_MEM_DEBUG)
uint32_t heap_get_mem_usage(uint8_t type)
{
    ASSERT_ERR(type < HEAP_TYPE_BLOCKS);

    return heap_env_app.heap_used[type];
}

uint32_t heap_get_mem_available(uint8_t type)
{
    ASSERT_ERR(type < HEAP_TYPE_BLOCKS);
    
    return heap_env_app.heap_size[type] - heap_env_app.heap_used[type];
}

uint32_t heap_get_max_mem_usage_single(uint8_t type)
{
    ASSERT_ERR(type < HEAP_TYPE_BLOCKS);

    return heap_env_app.max_heap_used_single[type];
}

uint32_t heap_get_max_mem_usage(void)
{
    uint32_t ret = heap_env_app.max_heap_used;

    return ret;
}

void heap_dump_used_mem(uint8_t type)
{
    struct mblock_free *node;
    uint32_t heap_size;

#define HEAP_RSV_TAIL_SIZE      0x10
    
    if (type >= HEAP_TYPE_BLOCKS) {
        return;
    }
    
    node = heap_env_app.heap[type];
    heap_size = heap_env_app.heap_size[type];
    GLOBAL_INT_DISABLE();
    while (heap_size > HEAP_RSV_TAIL_SIZE) {
        if (node->corrupt_check == HEAP_ALLOCATED_PATTERN) {
            struct mblock_used *used_node = (void *)node;
            printf("LR: 0x%08x, SIZE: 0x%08x, ADDR: 0x%08x.\r\n", used_node->return_addr, used_node->size, (uint32_t)used_node);
            heap_size -= used_node->size;
            node = (void *)((uint32_t)node + used_node->size);
        }
        else {
            heap_size -= node->free_size;
            node = (void *)((uint32_t)node + node->free_size);
        }
    }
    GLOBAL_INT_RESTORE();
}
#endif // (HEAP_MEM_DEBUG)

