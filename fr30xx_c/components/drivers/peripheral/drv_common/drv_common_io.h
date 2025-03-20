/* --------------------------------------------------------------------
** 
** Synopsys DesignWare AMBA Software Driver Kit and
** documentation (hereinafter, "Software") is an Unsupported
** proprietary work of Synopsys, Inc. unless otherwise expressly
** agreed to in writing between Synopsys and you.
** 
** The Software IS NOT an item of Licensed Software or Licensed
** Product under any End User Software License Agreement or Agreement
** for Licensed Product with Synopsys or any supplement thereto. You
** are permitted to use and redistribute this Software in source and
** binary forms, with or without modification, provided that
** redistributions of source code must retain this notice. You may not
** view, use, disclose, copy or distribute this file or any information
** contained herein except pursuant to this license grant from Synopsys.
** If you do not agree with this notice, including the disclaimer
** below, then you are not authorized to use the Software.
** 
** THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS"
** BASIS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
** FOR A PARTICULAR PURPOSE ARE HEREBY DISCLAIMED. IN NO EVENT SHALL
** SYNOPSYS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
** OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
** USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
** 
** --------------------------------------------------------------------
*/

#ifndef FR_COMMON_IO_H
#define FR_COMMON_IO_H

#ifdef __cplusplus
extern "C" {    // allow C++ to use these headers
#endif

// The following macros all perform 32-bit reads/writes for varying data
// bus widths.  For example, FR_OUT32_8 writes a 32-bit word in 8-bit
// chunks.
#define FR_OUT32_32P(v,p)   FR_OUT32_32(v, &(p))
#define FR_OUT32_16P(v,p)   FR_OUT32_16(v, &(p))
#define FR_OUT32_8P(v,p)    FR_OUT32_8(v, &(p))

#define FR_IN32_32P(p)      FR_IN32_32(&(p))
#define FR_IN32_16P(p)      FR_IN32_16(&(p))
#define FR_IN32_8P(p)       FR_IN32_8(&(p))

// The following macros perform 8-, 16- and 32-bit read/writes for
// various data bus widths.  These macros rely on a 'dev' structure of
// type dw_device, which has a 'data_width' member to determine the data
// bus width.
#define FR_OUT32P(v,p)      FR_OUT32(v, &(p))
#define FR_OUT16P(v,p)      FR_OUT16(v, &(p))
#define FR_OUT8P(v,p)       FR_OUT8(v, &(p))

#define FR_IN32P(p)         FR_IN32(&(p))
#define FR_IN16P(p)         FR_IN16(&(p))
#define FR_IN8P(p)          FR_IN8(&(p))

// The following macros perform 32-bit read/writes.  These macros rely
// on a 'dev' structure of type dw_device, which has a 'data_width'
// member to determine the data bus width.  They always read/write a
// 32-bit word, taking the data bus width into account.
#define FR_OUTP(v,p)        FR_OUT(v, &(##p))
#define FR_INP(p)           FR_IN(&(p))

// The default behaviour is to check individual devices' bus data
// widths.  This assumes the presence of a 'dev' structure of type
// dw_device which has a 'data_width' member.  If this member contains
// an illegal value, reads/writes default to 8-bit accesses.
//
// If all device data bus widths are the same (i.e. a single APB bus),
// and APB_DATA_WIDTH is defined, we can define all the IN/OUT macros
// for the correct data width.
    

// 32-bit data bus.  All macros can be defined to use their respective
// data read/write widths.
#define FR_OUT(v,p)         FR_OUT32_32(v,p)
#define FR_IN(p)            FR_IN32_32(p)
#define FR_OUT32(v,p)       FR_OUT32_32(v,p)
#define FR_OUT16(v,p)       FR_OUT16_16(v,p)
#define FR_OUT8(v,p)        FR_OUT8_8(v,p)
#define FR_IN32(p)          FR_IN32_32(p)
#define FR_IN16(p)          FR_IN16_16(p)
#define FR_IN8(p)           FR_IN8_8(p)
    

// The default endianness is little endian.
// the following macro performs a 32-bit write
#define FR_OUT32_32(v,p)                        \
do {                                            \
    *((volatile uint32_t *) (p)) = (v);         \
} while(0)

// the following macro performs two 16-bit writes
#define FR_OUT32_16(v,p)                        \
do {                                            \
    volatile uint16_t *ptr16;                   \
    uint16_t v16;                               \
    ptr16 = (uint16_t *) (p);                   \
    v16 = (uint16_t) ((v) & 0x0000ffff);        \
    *ptr16 = v16;                               \
    v16 = (uint16_t) (((v) & 0xffff0000) >> 16);\
    *(ptr16 + 1) = v16;                         \
} while (0)

// the following macro performs two 8-bit writes
#define FR_OUT32_8(v,p)                         \
do {                                            \
    volatile uint8_t *ptr8;                     \
    uint8_t v8;                                 \
    ptr8 = (uint8_t *) (p);                     \
    v8 = (uint8_t) ((v) & 0x000000ff);          \
    *ptr8 = v8;                                 \
    v8 = (uint8_t) (((v) & 0x0000ff00) >> 8);   \
    *(ptr8 + 1) = v8;                           \
    v8 = (uint8_t) (((v) & 0x00ff0000) >> 16);  \
    *(ptr8 + 2) = v8;                           \
    v8 = (uint8_t) (((v) & 0xff000000) >> 24);  \
    *(ptr8 + 3) = v8;                           \
} while(0)

// the following macro performs 16-bit writes
#define FR_OUT16_16(v,p)                            \
do {                                                \
    *((volatile uint16_t *) (p)) = (uint16_t) (v);  \
} while(0)

// the following macro performs two 8-bit writes
#define FR_OUT16_8(v,p)                             \
do {                                                \
    volatile uint8_t *ptr8;                         \
    uint16_t v8;                                    \
    ptr8 = (uint8_t *) (p);                         \
    v8 = (uint8_t) ((v) & 0x00ff);                  \
    *ptr8 = v8;                                     \
    v8 = (uint8_t) (((v) & 0xff00) >> 8);           \
    *(ptr8 + 1) = v8;                               \
} while(0)

// the following macro performs an 8-bit write
#define FR_OUT8_8(v,p)                              \
do {                                                \
    *((volatile uint8_t *) (p)) = (uint8_t) (v);    \
} while(0)

// the following macro performs a 32-bit reads
#define FR_IN32_32(p)   *((uint32_t *) (p))

// the following macro performs two 16-bit reads
#define FR_IN32_16(p)   ((uint32_t) *((uint16_t *) (p))     \
        | (*((uint16_t *) (p) + 1) << 16))

// the following macro performs 8-bit reads
#define FR_IN32_8(p)    ((uint32_t) (*((uint8_t *) (p)))    \
        | (*((uint8_t *) (p) + 1) << 8)                     \
        | (*((uint8_t *) (p) + 2) << 16)                    \
        | (*((uint8_t *) (p) + 3) << 24))

// the following macro performs a 16-bit read
#define FR_IN16_16(p)   ((uint16_t) *((volatile uint16_t *) (p)))

// the following macro performs two 8-bit reads
#define FR_IN16_8(p)    ((uint16_t) (*((volatile uint8_t *) (p))    \
    | (*((volatile uint8_t *) (p) + 1) << 8)))

// the following macro performs an 8-bit read
#define FR_IN8_8(p)     ((uint8_t) *((volatile uint8_t *) (p)))



#ifdef __cplusplus
}
#endif

#endif  // FR_COMMON_IO_H

