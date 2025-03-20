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

#ifndef FR_COMMON_BITOPS_H
#define FR_COMMON_BITOPS_H

#ifdef __cplusplus
extern "C" {    // allow C++ to use these headers
#endif

/****h* include.bitops
 * NAME
 *  FR_common_bitops.h -- bit manipulation macros
 * DESCRIPTION
 *  This set of macros accesses data using a width/shift parameter.
 *  This assumes that constants bfoXXX and bfwXXX are defined, 
 *  where XXX is the value specified in the parameter.
 *  + bfoXXX is the offset of the lowest bit.
 *  + bfwXXX is the number of bits to be accessed.
 ***/

/****d* include.bitops/FR_BITS
 * DESCRIPTION
 *  Constant definitions for various bits of a 32-bit word.
 * SOURCE
 */
#define FR_BIT0     0x00000001
#define FR_BIT1     0x00000002
#define FR_BIT2     0x00000004
#define FR_BIT3     0x00000008
#define FR_BIT4     0x00000010
#define FR_BIT5     0x00000020
#define FR_BIT6     0x00000040
#define FR_BIT7     0x00000080
#define FR_BIT8     0x00000100
#define FR_BIT9     0x00000200
#define FR_BIT10    0x00000400
#define FR_BIT11    0x00000800
#define FR_BIT12    0x00001000
#define FR_BIT13    0x00002000
#define FR_BIT14    0x00004000
#define FR_BIT15    0x00008000
#define FR_BIT16    0x00010000
#define FR_BIT17    0x00020000
#define FR_BIT18    0x00040000
#define FR_BIT19    0x00080000
#define FR_BIT20    0x00100000
#define FR_BIT21    0x00200000
#define FR_BIT22    0x00400000
#define FR_BIT23    0x00800000
#define FR_BIT24    0x01000000
#define FR_BIT25    0x02000000
#define FR_BIT26    0x04000000
#define FR_BIT27    0x08000000
#define FR_BIT28    0x10000000
#define FR_BIT29    0x20000000
#define FR_BIT30    0x40000000
#define FR_BIT31    0x80000000
#define FR_BITS_ALL 0xFFFFFFFF
/*****/

/****d* include.bitops/FR_BIT_WIDTH
 * DESCRIPTION
 *  Returns the width of the specified bit-field.
 * ARGUMENTS
 *  __bfws      a width/shift pair       
 * SOURCE
 */
#define FR_BIT_WIDTH(__bfws)    ((uint32_t) (bfw ## __bfws))
/*****/

/****d* include.bitops/FR_BIT_OFFSET
 * DESCRIPTION
 *  Returns the offset of the specified bit-field.
 * ARGUMENTS
 *  __bfws      a width/shift pair       
 * SOURCE
 */
#define FR_BIT_OFFSET(__bfws)   ((uint32_t) (bfo ## __bfws))
/*****/

/****d* include.bitops/FR_BIT_MASK
 * DESCRIPTION
 *  Returns a mask with the bits to be addressed set and all others
 *  cleared.
 * ARGUMENTS
 *  __bfws      a width/shift pair       
 * SOURCE
 */
#define FR_BIT_MASK(__bfws) ((uint32_t) (((bfw ## __bfws) == 32) ?  \
        0xFFFFFFFF : ((1U << (bfw ## __bfws)) - 1)) << (bfo ## __bfws))
/*****/

/****d* include.bitops/FR_BIT_CLEAR
 * DESCRIPTION
 *  Clear the specified bits.
 * ARGUMENTS
 *  __datum     the word of data to be modified
 *  __bfws      a width/shift pair       
 * SOURCE
 */
#define FR_BIT_CLEAR(__datum, __bfws)                               \
    ((__datum) = ((uint32_t) (__datum) & ~FR_BIT_MASK(__bfws)))
/*****/

/****d* include.bitops/FR_BIT_GET_UNSHIFTED
 * DESCRIPTION
 *  Returns the relevant bits masked from the data word, still at their
 *  original offset.
 * ARGUMENTS
 *  __datum     the word of data to be accessed
 *  __bfws      a width/shift pair       
 * SOURCE
 */
#define FR_BIT_GET_UNSHIFTED(__datum, __bfws)                       \
    ((uint32_t) ((__datum) & FR_BIT_MASK(__bfws)))
/*****/

/****d* include.bitops/FR_BIT_GET
 * DESCRIPTION
 *  Returns the relevant bits masked from the data word shifted to bit
 *  zero (i.e. access the specifed bits from a word of data as an
 *  integer value).
 * ARGUMENTS
 *  __datum     the word of data to be accessed
 *  __bfws      a width/shift pair       
 * SOURCE
 */
#define FR_BIT_GET(__datum, __bfws)                                 \
    ((uint32_t) (((__datum) & FR_BIT_MASK(__bfws)) >>               \
                 (bfo ## __bfws)))
/*****/

/****d* include.bitops/FR_BIT_SET
 * DESCRIPTION
 *  Place the specified value into the specified bits of a word of data
 *  (first the data is read, and the non-specified bits are re-written).
 * ARGUMENTS
 *  __datum     the word of data to be accessed
 *  __bfws      a width/shift pair       
 *  __val       the data value to be shifted into the specified bits
 * SOURCE
 */
#define FR_BIT_SET(__datum, __bfws, __val)                          \
    ((__datum) = ((uint32_t) (__datum) & ~FR_BIT_MASK(__bfws)) |    \
            ((__val << (bfo ## __bfws)) & FR_BIT_MASK(__bfws)))
/*****/

/****d* include.bitops/FR_BIT_SET_NOREAD
 * DESCRIPTION
 *  Place the specified value into the specified bits of a word of data
 *  without reading first - for sensitive interrupt type registers
 * ARGUMENTS
 *  __datum     the word of data to be accessed
 *  __bfws      a width/shift pair       
 *  __val       the data value to be shifted into the specified bits
 * SOURCE
 */
#define FR_BIT_SET_NOREAD(__datum, __bfws, __val)                   \
    ((uint32_t) ((__datum) = (((__val) << (bfo ## __bfws)) &        \
                              FR_BIT_MASK(__bfws))))
/*****/

/****d* include.bitops/FR_BIT_BUILD
 * DESCRIPTION
 *  Shift the specified value into the desired bits.
 * ARGUMENTS
 *  __bfws      a width/shift pair       
 *  __val       the data value to be shifted into the specified bits
 * SOURCE
 */
#define FR_BIT_BUILD(__bfws, __val)                                 \
    ((uint32_t) (((__val) << (bfo ## __bfws)) & FR_BIT_MASK(__bfws)))
/*****/

#ifdef __cplusplus
}
#endif

#endif      // FR_COMMON_BITOPS_H

