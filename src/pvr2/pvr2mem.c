/**
 * $Id$
 *
 * PVR2 (Video) VRAM handling routines (mainly for the 64-bit region)
 *
 * Copyright (c) 2005 Nathan Keynes.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "sh4/sh4core.h"
#include "pvr2.h"
#include "asic.h"
#include "dream.h"

unsigned char pvr2_main_ram[8 MB];

/************************* VRAM32 address space ***************************/

static int32_t FASTCALL pvr2_vram32_read_long( sh4addr_t addr )
{
    pvr2_render_buffer_invalidate(addr, FALSE);
    return *((int32_t *)(pvr2_main_ram+(addr&0x007FFFFF)));
}
static int32_t FASTCALL pvr2_vram32_read_word( sh4addr_t addr )
{
    pvr2_render_buffer_invalidate(addr, FALSE);
    return SIGNEXT16(*((int16_t *)(pvr2_main_ram+(addr&0x007FFFFF))));
}
static int32_t FASTCALL pvr2_vram32_read_byte( sh4addr_t addr )
{
    pvr2_render_buffer_invalidate(addr, FALSE);
    return SIGNEXT8(*((int8_t *)(pvr2_main_ram+(addr&0x007FFFFF))));
}
static void FASTCALL pvr2_vram32_write_long( sh4addr_t addr, uint32_t val )
{
    pvr2_render_buffer_invalidate(addr, TRUE);
    *(uint32_t *)(pvr2_main_ram + (addr&0x007FFFFF)) = val;
}
static void FASTCALL pvr2_vram32_write_word( sh4addr_t addr, uint32_t val )
{
    pvr2_render_buffer_invalidate(addr, TRUE);
    *(uint16_t *)(pvr2_main_ram + (addr&0x007FFFFF)) = (uint16_t)val;
}
static void FASTCALL pvr2_vram32_write_byte( sh4addr_t addr, uint32_t val )
{
    pvr2_render_buffer_invalidate(addr, TRUE);
    *(uint8_t *)(pvr2_main_ram + (addr&0x007FFFFF)) = (uint8_t)val;
}
static void FASTCALL pvr2_vram32_read_burst( unsigned char *dest, sh4addr_t addr )
{
    // Render buffers pretty much have to be (at least) 32-byte aligned
    pvr2_render_buffer_invalidate(addr, FALSE);
    memcpy( dest, (pvr2_main_ram + (addr&0x007FFFFF)), 32 );
}
static void FASTCALL pvr2_vram32_write_burst( sh4addr_t addr, unsigned char *src )
{
    // Render buffers pretty much have to be (at least) 32-byte aligned
    pvr2_render_buffer_invalidate(addr, TRUE);
    memcpy( (pvr2_main_ram + (addr&0x007FFFFF)), src, 32 );    
}

struct mem_region_fn mem_region_vram32 = { pvr2_vram32_read_long, pvr2_vram32_write_long, 
        pvr2_vram32_read_word, pvr2_vram32_write_word, 
        pvr2_vram32_read_byte, pvr2_vram32_write_byte, 
        pvr2_vram32_read_burst, pvr2_vram32_write_burst }; 

/************************* VRAM64 address space ***************************/

#define TRANSLATE_VIDEO_64BIT_ADDRESS(a)  ( (((a)&0x00FFFFF8)>>1)|(((a)&0x00000004)<<20)|((a)&0x03) )

static int32_t FASTCALL pvr2_vram64_read_long( sh4addr_t addr )
{
    addr = TRANSLATE_VIDEO_64BIT_ADDRESS(addr);
    pvr2_render_buffer_invalidate(addr, FALSE);
    return *((int32_t *)(pvr2_main_ram+(addr&0x007FFFFF)));
}
static int32_t FASTCALL pvr2_vram64_read_word( sh4addr_t addr )
{
    addr = TRANSLATE_VIDEO_64BIT_ADDRESS(addr);
    pvr2_render_buffer_invalidate(addr, FALSE);
    return SIGNEXT16(*((int16_t *)(pvr2_main_ram+(addr&0x007FFFFF))));
}
static int32_t FASTCALL pvr2_vram64_read_byte( sh4addr_t addr )
{
    addr = TRANSLATE_VIDEO_64BIT_ADDRESS(addr);
    pvr2_render_buffer_invalidate(addr, FALSE);
    return SIGNEXT8(*((int8_t *)(pvr2_main_ram+(addr&0x007FFFFF))));
}
static void FASTCALL pvr2_vram64_write_long( sh4addr_t addr, uint32_t val )
{
    texcache_invalidate_page(addr& 0x007FFFFF);
    addr = TRANSLATE_VIDEO_64BIT_ADDRESS(addr);
    pvr2_render_buffer_invalidate(addr, TRUE);
    *(uint32_t *)(pvr2_main_ram + (addr&0x007FFFFF)) = val;
}
static void FASTCALL pvr2_vram64_write_word( sh4addr_t addr, uint32_t val )
{
    texcache_invalidate_page(addr& 0x007FFFFF);
    addr = TRANSLATE_VIDEO_64BIT_ADDRESS(addr);
    pvr2_render_buffer_invalidate(addr, TRUE);
    *(uint16_t *)(pvr2_main_ram + (addr&0x007FFFFF)) = (uint16_t)val;
}
static void FASTCALL pvr2_vram64_write_byte( sh4addr_t addr, uint32_t val )
{
    texcache_invalidate_page(addr& 0x007FFFFF);
    addr = TRANSLATE_VIDEO_64BIT_ADDRESS(addr);
    pvr2_render_buffer_invalidate(addr, TRUE);
    *(uint8_t *)(pvr2_main_ram + (addr&0x007FFFFF)) = (uint8_t)val;
}
static void FASTCALL pvr2_vram64_read_burst( unsigned char *dest, sh4addr_t addr )
{
    pvr2_vram64_read( dest, addr, 32 );
}
static void FASTCALL pvr2_vram64_write_burst( sh4addr_t addr, unsigned char *src )
{
    pvr2_vram64_write( addr, src, 32 );
}

struct mem_region_fn mem_region_vram64 = { pvr2_vram64_read_long, pvr2_vram64_write_long, 
        pvr2_vram64_read_word, pvr2_vram64_write_word, 
        pvr2_vram64_read_byte, pvr2_vram64_write_byte, 
        pvr2_vram64_read_burst, pvr2_vram64_write_burst }; 

/******************************* Burst areas ******************************/

static void FASTCALL pvr2_vramdma1_write_burst( sh4addr_t destaddr, unsigned char *src )
{
    int region = MMIO_READ( ASIC, PVRDMARGN1 );
    if( region == 0 ) {
        pvr2_vram64_write( destaddr, src, 32 );
    } else {
        pvr2_vram32_write( destaddr, src, 32 );
    }   
}

static void FASTCALL pvr2_vramdma2_write_burst( sh4addr_t destaddr, unsigned char *src )
{
    int region = MMIO_READ( ASIC, PVRDMARGN2 );
    if( region == 0 ) {
        pvr2_vram64_write( destaddr, src, 32 );
    } else {
        pvr2_vram32_write( destaddr, src, 32 );
    }
}

static void FASTCALL pvr2_yuv_write_burst( sh4addr_t destaddr, unsigned char *src )
{
    pvr2_yuv_write( src, 32 );
}

struct mem_region_fn mem_region_pvr2ta = {
        unmapped_read_long, unmapped_write_long,
        unmapped_read_long, unmapped_write_long,
        unmapped_read_long, unmapped_write_long,
        unmapped_read_burst, pvr2_ta_write_burst };

struct mem_region_fn mem_region_pvr2yuv = {
        unmapped_read_long, unmapped_write_long,
        unmapped_read_long, unmapped_write_long,
        unmapped_read_long, unmapped_write_long,
        unmapped_read_burst, pvr2_yuv_write_burst };

struct mem_region_fn mem_region_pvr2vdma1 = {
        unmapped_read_long, unmapped_write_long,
        unmapped_read_long, unmapped_write_long,
        unmapped_read_long, unmapped_write_long,
        unmapped_read_burst, pvr2_vramdma1_write_burst };

struct mem_region_fn mem_region_pvr2vdma2 = {
        unmapped_read_long, unmapped_write_long,
        unmapped_read_long, unmapped_write_long,
        unmapped_read_long, unmapped_write_long,
        unmapped_read_burst, pvr2_vramdma2_write_burst };


void pvr2_dma_write( sh4addr_t destaddr, unsigned char *src, uint32_t count )
{
    int region;

    switch( destaddr & 0x13800000 ) {
    case 0x10000000:
    case 0x12000000:
        pvr2_ta_write( src, count );
        break;
    case 0x11000000:
    case 0x11800000:
        region = MMIO_READ( ASIC, PVRDMARGN1 );
        if( region == 0 ) {
            pvr2_vram64_write( destaddr, src, count );
        } else {
            pvr2_vram32_write( destaddr, src, count );
        }
        break;
    case 0x10800000:
    case 0x12800000:
        pvr2_yuv_write( src, count );
        break;
    case 0x13000000:
    case 0x13800000:
        region = MMIO_READ( ASIC, PVRDMARGN2 );
        if( region == 0 ) {
            pvr2_vram64_write( destaddr, src, count );
        } else {
            pvr2_vram32_write( destaddr, src, count );
        }
    }
}

void pvr2_vram32_write( sh4addr_t destaddr, unsigned char *src, uint32_t length )
{
    destaddr &= PVR2_RAM_MASK;
    pvr2_render_buffer_invalidate( PVR2_RAM_BASE + destaddr, TRUE );
    unsigned char *dest = pvr2_main_ram + destaddr;
    if( PVR2_RAM_SIZE - destaddr < length ) {
        length = PVR2_RAM_SIZE - destaddr;
    }
    memcpy( dest, src, length );
}

void pvr2_vram64_write( sh4addr_t destaddr, unsigned char *src, uint32_t length )
{
    int bank_flag = (destaddr & 0x04) >> 2;
    uint32_t *banks[2];
    uint32_t *dwsrc;
    int i;

    destaddr = destaddr & 0x7FFFFF;
    if( destaddr + length > 0x800000 ) {
        length = 0x800000 - destaddr;
    }

    for( i=destaddr & 0xFFFFF000; i < destaddr + length; i+= LXDREAM_PAGE_SIZE ) {
        texcache_invalidate_page( i );
    }

    banks[0] = ((uint32_t *)(pvr2_main_ram + ((destaddr & 0x007FFFF8) >>1)));
    banks[1] = banks[0] + 0x100000;
    if( bank_flag )
        banks[0]++;

    /* Handle non-aligned start of source */
    if( destaddr & 0x03 ) {
        unsigned char *dest = ((unsigned char *)banks[bank_flag]) + (destaddr & 0x03);
        for( i= destaddr & 0x03; i < 4 && length > 0; i++, length-- ) {
            *dest++ = *src++;
        }
        bank_flag = !bank_flag;
    }

    dwsrc = (uint32_t *)src;
    while( length >= 4 ) {
        *banks[bank_flag]++ = *dwsrc++;
        bank_flag = !bank_flag;
        length -= 4;
    }

    /* Handle non-aligned end of source */
    if( length ) {
        src = (unsigned char *)dwsrc;
        unsigned char *dest = (unsigned char *)banks[bank_flag];
        while( length-- > 0 ) {
            *dest++ = *src++;
        }
    }
}

/**
 * Write an image to 64-bit vram, with a line-stride different from the line-size.
 * The destaddr must be 64-bit aligned, and both line_bytes and line_stride_bytes
 * must be multiples of 8.
 */
void pvr2_vram64_write_stride( sh4addr_t destaddr, unsigned char *src, uint32_t line_bytes,
                               uint32_t line_stride_bytes, uint32_t line_count )
{
    int i,j;
    uint32_t *banks[2];
    uint32_t *dwsrc = (uint32_t *)src;
    uint32_t line_gap = (line_stride_bytes - line_bytes) >> 3;

    destaddr = destaddr & 0x7FFFF8;
    line_bytes >>= 3;

    for( i=destaddr; i < destaddr + line_stride_bytes*line_count; i+= LXDREAM_PAGE_SIZE ) {
        texcache_invalidate_page( i );
    }

    banks[0] = (uint32_t *)(pvr2_main_ram + (destaddr >>1));
    banks[1] = banks[0] + 0x100000;

    for( i=0; i<line_count; i++ ) {
        for( j=0; j<line_bytes; j++ ) {
            *banks[0]++ = *dwsrc++;
            *banks[1]++ = *dwsrc++;
        }
        banks[0] += line_gap;
        banks[1] += line_gap;
    }
}

/**
 * Read an image from 64-bit vram, with a destination line-stride different from the line-size.
 * The srcaddr must be 32-bit aligned, and both line_bytes and line_stride_bytes
 * must be multiples of 4. line_stride_bytes must be >= line_bytes.
 * This method is used to extract a "stride" texture from vram.
 */
void pvr2_vram64_read_stride( unsigned char *dest, uint32_t dest_line_bytes, sh4addr_t srcaddr,
                              uint32_t src_line_bytes, uint32_t line_count )
{
    int bank_flag = (srcaddr & 0x04) >> 2;
    uint32_t *banks[2];
    uint32_t *dwdest;
    uint32_t dest_line_gap = 0;
    uint32_t src_line_gap = 0;
    uint32_t line_bytes;
    int src_line_gap_flag;
    int i,j;

    srcaddr = srcaddr & 0x7FFFF8;
    if( src_line_bytes <= dest_line_bytes ) {
        dest_line_gap = (dest_line_bytes - src_line_bytes) >> 2;
        src_line_gap = 0;
        src_line_gap_flag = 0;
        line_bytes = src_line_bytes >> 2;
    } else {
        i = (src_line_bytes - dest_line_bytes);
        src_line_gap_flag = i & 0x04;
        src_line_gap = i >> 3;
        line_bytes = dest_line_bytes >> 2;
    }

    banks[0] = (uint32_t *)(pvr2_main_ram + (srcaddr>>1));
    banks[1] = banks[0] + 0x100000;
    if( bank_flag )
        banks[0]++;

    dwdest = (uint32_t *)dest;
    for( i=0; i<line_count; i++ ) {
        for( j=0; j<line_bytes; j++ ) {
            *dwdest++ = *banks[bank_flag]++;
            bank_flag = !bank_flag;
        }
        dwdest += dest_line_gap;
        banks[0] += src_line_gap;
        banks[1] += src_line_gap;
        if( src_line_gap_flag ) {
            banks[bank_flag]++;
            bank_flag = !bank_flag;
        }
    }
}


/**
 * @param dest Destination image buffer
 * @param banks Source data expressed as two bank pointers
 * @param offset Offset into banks[0] specifying where the next byte
 *  to read is (0..3)
 * @param x1,y1 Destination coordinates
 * @param width Width of current destination block
 * @param stride Total width of image (ie stride) in bytes
 */

static void pvr2_vram64_detwiddle_4( uint8_t *dest, uint8_t *banks[2], int offset,
                                     int x1, int y1, int width, int stride )
{
    if( width == 2 ) {
        x1 = x1 >> 1;
        uint8_t t1 = *banks[offset<4?0:1]++;
        uint8_t t2 = *banks[offset<3?0:1]++;
        dest[y1*stride + x1] = (t1 & 0x0F) | (t2<<4);
        dest[(y1+1)*stride + x1] = (t1>>4) | (t2&0xF0);
    } else if( width == 4 ) {
        pvr2_vram64_detwiddle_4( dest, banks, offset, x1, y1, 2, stride );
        pvr2_vram64_detwiddle_4( dest, banks, offset+2, x1, y1+2, 2, stride );
        pvr2_vram64_detwiddle_4( dest, banks, offset+4, x1+2, y1, 2, stride );
        pvr2_vram64_detwiddle_4( dest, banks, offset+6, x1+2, y1+2, 2, stride );

    } else {
        int subdivide = width >> 1;
        pvr2_vram64_detwiddle_4( dest, banks, offset, x1, y1, subdivide, stride );
        pvr2_vram64_detwiddle_4( dest, banks, offset, x1, y1+subdivide, subdivide, stride );
        pvr2_vram64_detwiddle_4( dest, banks, offset, x1+subdivide, y1, subdivide, stride );
        pvr2_vram64_detwiddle_4( dest, banks, offset, x1+subdivide, y1+subdivide, subdivide, stride );
    }
}

/**
 * @param dest Destination image buffer
 * @param banks Source data expressed as two bank pointers
 * @param offset Offset into banks[0] specifying where the next byte
 *  to read is (0..3)
 * @param x1,y1 Destination coordinates
 * @param width Width of current destination block
 * @param stride Total width of image (ie stride)
 */

static void pvr2_vram64_detwiddle_8( uint8_t *dest, uint8_t *banks[2], int offset,
                                     int x1, int y1, int width, int stride )
{
    if( width == 2 ) {
        dest[y1*stride + x1] = *banks[0]++;
        dest[(y1+1)*stride + x1] = *banks[offset<3?0:1]++;
        dest[y1*stride + x1 + 1] = *banks[offset<2?0:1]++;
        dest[(y1+1)*stride + x1 + 1] = *banks[offset==0?0:1]++;
        uint8_t *tmp = banks[0]; /* swap banks */
        banks[0] = banks[1];
        banks[1] = tmp;
    } else {
        int subdivide = width >> 1;
        pvr2_vram64_detwiddle_8( dest, banks, offset, x1, y1, subdivide, stride );
        pvr2_vram64_detwiddle_8( dest, banks, offset, x1, y1+subdivide, subdivide, stride );
        pvr2_vram64_detwiddle_8( dest, banks, offset, x1+subdivide, y1, subdivide, stride );
        pvr2_vram64_detwiddle_8( dest, banks, offset, x1+subdivide, y1+subdivide, subdivide, stride );
    }
}

/**
 * @param dest Destination image buffer
 * @param banks Source data expressed as two bank pointers
 * @param offset Offset into banks[0] specifying where the next word
 *  to read is (0 or 1)
 * @param x1,y1 Destination coordinates
 * @param width Width of current destination block
 * @param stride Total width of image (ie stride)
 */

static void pvr2_vram64_detwiddle_16( uint16_t *dest, uint16_t *banks[2], int offset,
                                      int x1, int y1, int width, int stride )
{
    if( width == 2 ) {
        dest[y1*stride + x1] = *banks[0]++;
        dest[(y1+1)*stride + x1] = *banks[offset]++;
        dest[y1*stride + x1 + 1] = *banks[1]++;
        dest[(y1+1)*stride + x1 + 1] = *banks[offset^1]++;
    } else {
        int subdivide = width >> 1;
        pvr2_vram64_detwiddle_16( dest, banks, offset, x1, y1, subdivide, stride );
        pvr2_vram64_detwiddle_16( dest, banks, offset, x1, y1+subdivide, subdivide, stride );
        pvr2_vram64_detwiddle_16( dest, banks, offset, x1+subdivide, y1, subdivide, stride );
        pvr2_vram64_detwiddle_16( dest, banks, offset, x1+subdivide, y1+subdivide, subdivide, stride );
    }
}

/**
 * Read an image from 64-bit vram stored as twiddled 4-bit pixels. The
 * image is written out to the destination in detwiddled form.
 * @param dest destination buffer, which must be at least width*height/2 in length
 * @param srcaddr source address in vram
 * @param width image width (must be a power of 2)
 * @param height image height (must be a power of 2)
 */
void pvr2_vram64_read_twiddled_4( unsigned char *dest, sh4addr_t srcaddr, uint32_t width, uint32_t height )
{
    int offset_flag = (srcaddr & 0x07);
    uint8_t *banks[2];
    uint8_t *wdest = (uint8_t*)dest;
    uint32_t stride = width >> 1;
    int i;

    srcaddr = srcaddr & 0x7FFFF8;

    banks[0] = (uint8_t *)(pvr2_main_ram + (srcaddr>>1));
    banks[1] = banks[0] + 0x400000;
    if( offset_flag & 0x04 ) { // If source is not 64-bit aligned, swap the banks
        uint8_t *tmp = banks[0];
        banks[0] = banks[1];
        banks[1] = tmp + 4;
        offset_flag &= 0x03;
    }
    banks[0] += offset_flag;

    if( width > height ) {
        for( i=0; i<width; i+=height ) {
            pvr2_vram64_detwiddle_4( wdest, banks, offset_flag, i, 0, height, stride );
        }
    } else if( height > width ) {
        for( i=0; i<height; i+=width ) {
            pvr2_vram64_detwiddle_4( wdest, banks, offset_flag, 0, i, width, stride );
        }
    } else if( width == 1 ) {
        *wdest = *banks[0];
    } else {
        pvr2_vram64_detwiddle_4( wdest, banks, offset_flag, 0, 0, width, stride );
    }
}

/**
 * Read an image from 64-bit vram stored as twiddled 8-bit pixels. The
 * image is written out to the destination in detwiddled form.
 * @param dest destination buffer, which must be at least width*height in length
 * @param srcaddr source address in vram
 * @param width image width (must be a power of 2)
 * @param height image height (must be a power of 2)
 */
void pvr2_vram64_read_twiddled_8( unsigned char *dest, sh4addr_t srcaddr, uint32_t width, uint32_t height )
{
    int offset_flag = (srcaddr & 0x07);
    uint8_t *banks[2];
    uint8_t *wdest = (uint8_t*)dest;
    int i;

    srcaddr = srcaddr & 0x7FFFF8;

    banks[0] = (uint8_t *)(pvr2_main_ram + (srcaddr>>1));
    banks[1] = banks[0] + 0x400000;
    if( offset_flag & 0x04 ) { // If source is not 64-bit aligned, swap the banks
        uint8_t *tmp = banks[0];
        banks[0] = banks[1];
        banks[1] = tmp + 4;
        offset_flag &= 0x03;
    }
    banks[0] += offset_flag;

    if( width > height ) {
        for( i=0; i<width; i+=height ) {
            pvr2_vram64_detwiddle_8( wdest, banks, offset_flag, i, 0, height, width );
        }
    } else if( height > width ) {
        for( i=0; i<height; i+=width ) {
            pvr2_vram64_detwiddle_8( wdest, banks, offset_flag, 0, i, width, width );
        }
    } else if( width == 1 ) {
        *wdest = *banks[0];
    } else {
        pvr2_vram64_detwiddle_8( wdest, banks, offset_flag, 0, 0, width, width );
    }
}

/**
 * Read an image from 64-bit vram stored as twiddled 16-bit pixels. The
 * image is written out to the destination in detwiddled form.
 * @param dest destination buffer, which must be at least width*height*2 in length
 * @param srcaddr source address in vram (must be 16-bit aligned)
 * @param width image width (must be a power of 2)
 * @param height image height (must be a power of 2)
 */
void pvr2_vram64_read_twiddled_16( unsigned char *dest, sh4addr_t srcaddr, uint32_t width, uint32_t height ) {
    int offset_flag = (srcaddr & 0x06) >> 1;
    uint16_t *banks[2];
    uint16_t *wdest = (uint16_t*)dest;
    int i;

    srcaddr = srcaddr & 0x7FFFF8;

    banks[0] = (uint16_t *)(pvr2_main_ram + (srcaddr>>1));
    banks[1] = banks[0] + 0x200000;
    if( offset_flag & 0x02 ) { // If source is not 64-bit aligned, swap the banks
        uint16_t *tmp = banks[0];
        banks[0] = banks[1];
        banks[1] = tmp + 2;
        offset_flag &= 0x01;
    }
    banks[0] += offset_flag;


    if( width > height ) {
        for( i=0; i<width; i+=height ) {
            pvr2_vram64_detwiddle_16( wdest, banks, offset_flag, i, 0, height, width );
        }
    } else if( height > width ) {
        for( i=0; i<height; i+=width ) {
            pvr2_vram64_detwiddle_16( wdest, banks, offset_flag, 0, i, width, width );
        }
    } else if( width == 1 ) {
        *wdest = *banks[0];
    } else {
        pvr2_vram64_detwiddle_16( wdest, banks, offset_flag, 0, 0, width, width );
    }
}

static void pvr2_vram_write_invert( sh4addr_t destaddr, unsigned char *src, uint32_t src_size, 
                             uint32_t line_size, uint32_t dest_stride,
                             uint32_t src_stride )
{
    unsigned char *dest = pvr2_main_ram + (destaddr & 0x007FFFFF);
    unsigned char *p = src + src_size - src_stride;
    while( p >= src ) {
        memcpy( dest, p, line_size );
        p -= src_stride;
        dest += dest_stride;
    }
}

static void pvr2_vram64_write_invert( sh4addr_t destaddr, unsigned char *src, 
                                      uint32_t src_size, uint32_t line_size, 
                                      uint32_t dest_stride, uint32_t src_stride )
{
    int i,j;
    uint32_t *banks[2];
    uint32_t *dwsrc = (uint32_t *)(src + src_size - src_stride);
    int32_t src_line_gap = ((int32_t)src_stride + line_size) >> 2; 
    int32_t dest_line_gap = ((int32_t)dest_stride - (int32_t)line_size) >> 3;

    destaddr = destaddr & 0x7FFFF8;

    for( i=destaddr; i < destaddr + dest_stride*(src_size/src_stride); i+= LXDREAM_PAGE_SIZE ) {
        texcache_invalidate_page( i );
    }

    banks[0] = (uint32_t *)(pvr2_main_ram + (destaddr >>1));
    banks[1] = banks[0] + 0x100000;

    while( dwsrc >= (uint32_t *)src ) { 
        for( j=0; j<line_size; j+=8 ) {
            *banks[0]++ = *dwsrc++;
            *banks[1]++ = *dwsrc++;
        }
        banks[0] += dest_line_gap;
        banks[1] += dest_line_gap;
        dwsrc -= src_line_gap;
    }    
}

/**
 * Copy a pixel buffer to vram, flipping and scaling at the same time. This
 * is not massively efficient, but it's used pretty rarely.
 */
static void pvr2_vram_write_invert_hscale( sh4addr_t destaddr, unsigned char *src, uint32_t src_size, 
                             uint32_t line_size, uint32_t dest_stride,
                             uint32_t src_stride, int bpp )
{
    unsigned char *dest = pvr2_main_ram + (destaddr & 0x007FFFFF);
    unsigned char *p = src + src_size - src_stride;
    while( p >= src ) {
        unsigned char *s = p, *d = dest;
        int i;
        while( s < p+line_size ) {
            for( i=0; i<bpp; i++ ) {
                *d++ = *s++;
            }
            s+= bpp;
        }
        p -= src_stride;
        dest += dest_stride;
    }
}

void pvr2_vram64_read( unsigned char *dest, sh4addr_t srcaddr, uint32_t length )
{
    int bank_flag = (srcaddr & 0x04) >> 2;
    uint32_t *banks[2];
    uint32_t *dwdest;
    int i;

    srcaddr = srcaddr & 0x7FFFFF;
    if( srcaddr + length > 0x800000 )
        length = 0x800000 - srcaddr;

    banks[0] = ((uint32_t *)(pvr2_main_ram + ((srcaddr&0x007FFFF8)>>1)));
    banks[1] = banks[0] + 0x100000;
    if( bank_flag )
        banks[0]++;

    /* Handle non-aligned start of source */
    if( srcaddr & 0x03 ) {
        char *src = ((char *)banks[bank_flag]) + (srcaddr & 0x03);
        for( i= srcaddr & 0x03; i < 4 && length > 0; i++, length-- ) {
            *dest++ = *src++;
        }
        bank_flag = !bank_flag;
    }

    dwdest = (uint32_t *)dest;
    while( length >= 4 ) {
        *dwdest++ = *banks[bank_flag]++;
        bank_flag = !bank_flag;
        length -= 4;
    }

    /* Handle non-aligned end of source */
    if( length ) {
        dest = (unsigned char *)dwdest;
        unsigned char *src = (unsigned char *)banks[bank_flag];
        while( length-- > 0 ) {
            *dest++ = *src++;
        }
    }
}

void pvr2_vram64_dump_file( sh4addr_t addr, uint32_t length, gchar *filename )
{
    uint32_t tmp[length>>2];
    FILE *f = fopen(filename, "wo");
    unsigned int i, j;

    if( f == NULL ) {
        ERROR( "Unable to write to dump file '%s' (%s)", filename, strerror(errno) );
        return;
    }
    pvr2_vram64_read( (unsigned char *)tmp, addr, length );
    fprintf( f, "%08X\n", addr );
    for( i =0; i<length>>2; i+=8 ) {
        for( j=i; j<i+8; j++ ) {
            if( j < length )
                fprintf( f, " %08X", tmp[j] );
            else
                fprintf( f, "         " );
        }
        fprintf( f, "\n" );
    }
    fclose(f);
}

void pvr2_vram64_dump( sh4addr_t addr, uint32_t length, FILE *f )
{
    unsigned char tmp[length];
    pvr2_vram64_read( tmp, addr, length );
    fwrite_dump( tmp, length, f );
}



/**
 * Flush the indicated render buffer back to PVR. Caller is responsible for
 * tracking whether there is actually anything in the buffer.
 *
 * FIXME: Handle horizontal scaler 
 *
 * @param buffer A render buffer indicating the address to store to, and the
 * format the data needs to be in.
 */
void pvr2_render_buffer_copy_to_sh4( render_buffer_t buffer )
{
    int line_size = buffer->width * colour_formats[buffer->colour_format].bpp;
    int src_stride = line_size;
    unsigned char target[buffer->size];

    display_driver->read_render_buffer( target, buffer, line_size, buffer->colour_format );

    if( (buffer->scale & 0xFFFF) == 0x0800 )
        src_stride <<= 1;

    if( (buffer->address & 0xFF000000) == 0x04000000 ) {
        pvr2_vram64_write_invert( buffer->address, target, buffer->size, line_size, 
                                  buffer->rowstride, src_stride );
    } else {
        /* Regular buffer */
        if( buffer->scale & SCALER_HSCALE ) {
            pvr2_vram_write_invert_hscale( buffer->address, target, buffer->size, line_size, buffer->rowstride,
                                           src_stride, colour_formats[buffer->colour_format].bpp );
        } else {
            pvr2_vram_write_invert( buffer->address, target, buffer->size, line_size, buffer->rowstride,
                                    src_stride );
        }
    }
    buffer->flushed = TRUE;
}

