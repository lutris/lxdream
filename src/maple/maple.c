/**
 * $Id$
 *
 * Implements the core Maple bus, including DMA transfers to and from the bus.
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
#define MODULE maple_module

#include <assert.h>
#include <glib/gstrfuncs.h>
#include "eventq.h"
#include "dream.h"
#include "mem.h"
#include "asic.h"
#include "maple.h"

void maple_init( void );

struct dreamcast_module maple_module = { "Maple", maple_init, NULL, NULL, NULL,
        NULL, NULL, NULL };

struct maple_device_class *maple_device_classes[] = { 
        &controller_class, &keyboard_class, &lightgun_class, &mouse_class, &vmu_class, NULL };

/**
 * Fire interrupt to notify the completion of DMA transfer
 */
static void maple_event_handler( int eventid )
{
    MMIO_WRITE( ASIC, MAPLE_STATE, 0 );
    asic_event( EVENT_MAPLE_DMA );
}

void maple_init( void )
{
    register_event_callback( EVENT_MAPLE_DMA, maple_event_handler );
}

maple_device_t maple_new_device( const gchar *name )
{
    maple_device_class_t clz = maple_get_device_class(name);
    if( clz != NULL ) {
        return clz->new_device();
    } 
    return NULL;
}

maple_device_class_t maple_get_device_class( const gchar *name )
{
    int i;
    for( i=0; maple_device_classes[i] != NULL; i++ ) {
        if( g_strcasecmp(maple_device_classes[i]->name, name ) == 0 )
            return maple_device_classes[i];
    }
    return NULL;
}

const struct maple_device_class **maple_get_device_classes()
{
    return (const struct maple_device_class **)maple_device_classes;
}

lxdream_config_group_t maple_get_device_config( maple_device_t dev )
{
    if( dev->get_config == NULL )
        return NULL;
    return dev->get_config(dev);
}

/**
 * Input data looks like this:
 *    0: transfer control word
 *      0: length of data in words (not including 3 word header)
 *      1: low bit = lightgun mode
 *      2: low 2 bits = port # (0..3)
 *      3: 0x80 = last packet, 0x00 = normal packet
 *    4: output buffer address
 *    8: Command word
 *      8: command code
 *      9: destination address
 *     10: source address
 *     11: length of data in words (not including 3 word header)
 *   12: command-specific data
 */

/**
 * array is [port][subperipheral], so [0][0] is main peripheral on port A,
 * [1][2] is the second subperipheral on port B and so on.
 */
maple_device_t maple_devices[4][6];
int maple_periph_mask[4];
#define GETBYTE(n) ((uint32_t)(buf[n]))
#define GETWORD(n) (*((uint32_t *)(buf+(n))))
#define PUTBYTE(n,x) (buf[n] = (char)x)
#define PUTWORD(n,x) (*((uint32_t *)(return_buf+(n))) = (x))

maple_device_t maple_get_device( unsigned int port, unsigned int periph ) {
    if( port >= 4 )
        return NULL;
    if( periph >= 6 )
        return NULL;
    return maple_devices[port][periph];
}

void maple_handle_buffer( uint32_t address ) {
    unsigned char *buf = (unsigned char *)mem_get_region(address);
    if( buf == NULL ) {
        ERROR( "Invalid or unmapped buffer passed to maple (0x%08X)", address );
    } else {
        unsigned int last = 0;
        int i = 0, count;
        for( count=0; !last; count++ ) {
            unsigned int port, length, mode, periph, periph_id, out_length;
            unsigned int cmd, recv_addr, send_addr;
            uint32_t return_addr;
            unsigned char *return_buf;
            maple_device_t dev;

            last = GETBYTE(3) & 0x80; /* indicates last packet */
            port = GETBYTE(2) & 0x03;
            mode = GETBYTE(1) & 0x07;
            length = GETBYTE(0) & 0xFF;
            return_addr = GETWORD(4);

            switch( mode ) {
            case 2: /* lightgun */
                dev = maple_devices[port][0];
                if( dev != NULL && dev->start_gun != NULL ) {
                    dev->start_gun(dev);
                    return; // Pending
                } else {
                    /* FIXME: Determine how long maple IO really takes to process, 
                     * which is probably a function of the number of requests.
                     * For now, just use 0.2ms as a reasonable value.
                     */
                    event_schedule( EVENT_MAPLE_DMA, 200000 );
                    return;
                }
            case 7: /* skip */
                buf += 4;
                address +=4; 
                continue;
            }
            if( (return_addr & 0x1C000000) != 0x0C000000 ) {
                ERROR( "Bad return address in maple packet: %08X", return_addr );
                break;
            }
            return_buf = mem_get_region(return_addr);
            cmd = GETBYTE(8);
            recv_addr = GETBYTE(9);
            send_addr = GETBYTE(10);
            /* Sanity checks */
            if( GETBYTE(11) != length ||
                    /* send_addr >> 6 != port || */  /* TODO: check that this is ignored? */
                    recv_addr >> 6 != port ||
                    return_buf == NULL ) {
                ERROR( "Received bad packet: %02X %02X %02X %02X  %02X %02X %02X %02X  %02X %02X %02X %02X",
                        buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
                        buf[8], buf[9], buf[10], buf[11] );
                break;
            }
            periph = 0;
            periph_id = recv_addr & 0x3F;
            if( periph_id != 0x20 ) {
                for( i=0;i<5;i++ ) {
                    if( periph_id == (1<<i) ) {
                        periph = i+1;
                        break;
                    }
                }
                if( periph == 0 ) { /* Bad setting */
                    /* ERROR */
                }
            }

            dev = maple_devices[port][periph];
            if( dev == NULL ) {
                /* no device attached */
                *((uint32_t *)return_buf) = -1;
            } else {
                int status, func;
                unsigned int pt, phase, block, blkid;
                out_length = 0;
                switch( cmd ) {
                case MAPLE_CMD_INFO:
                    status = MAPLE_RESP_INFO;
                    memcpy( return_buf+4, dev->ident, 112 );
                    out_length = 0x1C;
                    if( periph == 0 ) {
                        /* Identify command on the primary device also sets the
                         * bits in the address in the response according to the
                         * sub-peripherals present.
                         */
                        recv_addr &= 0xE0;
                        for( i=0; i<5; i++ ) {
                            if( maple_devices[port][i+1] != NULL ) {
                                recv_addr |= (1<<i);
                            }
                        }
                    }
                    break;
                case MAPLE_CMD_EXT_INFO:
                    status = MAPLE_RESP_EXT_INFO;
                    memcpy( return_buf+4, dev->ident, 192 );
                    out_length = 0x30;
                    break;
                case MAPLE_CMD_RESET:
                    if( dev->reset == NULL )
                        status = MAPLE_RESP_ACK;
                    else status = dev->reset(dev);
                    break;
                case MAPLE_CMD_SHUTDOWN:
                    if( dev->shutdown == NULL )
                        status = MAPLE_RESP_ACK;
                    else status = dev->shutdown(dev);
                    break;
                case MAPLE_CMD_GET_COND:
                    func = GETWORD(12);
                    if( dev->get_condition == NULL )
                        status = MAPLE_ERR_CMD_UNKNOWN;
                    else status = dev->get_condition(dev, func,
                            return_buf+8,
                            &out_length );
                    if( status == 0 ) {
                        out_length++;
                        status = MAPLE_RESP_DATA;
                        PUTWORD(4,func);
                    }
                    break;
                case MAPLE_CMD_SET_COND:
                    func = GETWORD(12);
                    if( dev->set_condition == NULL )
                        status = MAPLE_ERR_CMD_UNKNOWN;
                    else status = dev->set_condition(dev, func,
                            buf+16,
                            length-1);
                    if( status == 0 )
                        status = MAPLE_RESP_ACK;
                    break;
                case MAPLE_CMD_MEM_INFO:
                    func = GETWORD(12);
                    pt = GETWORD(16);
                    if( dev->get_memory_info == NULL ) 
                        status = MAPLE_ERR_CMD_UNKNOWN;
                    else status = dev->get_memory_info(dev,func, pt, return_buf+8, &out_length);
                    if( status == 0 ) {
                        out_length++;
                        status = MAPLE_RESP_DATA;
                        PUTWORD(4,func);
                    }
                    break;
                case MAPLE_CMD_READ_BLOCK:
                    func = GETWORD(12);
                    pt = GETBYTE(16);
                    phase = GETBYTE(17);
                    block = (GETBYTE(18)<<8) | GETBYTE(19);
                    blkid = GETWORD(16);
                    if( dev->read_block == NULL )
                        status = MAPLE_ERR_CMD_UNKNOWN;
                    else status = dev->read_block(dev, func, pt, block, phase,
                            return_buf+12,
                            &out_length );
                    if( status == 0 ) {
                        status = MAPLE_RESP_DATA;
                        out_length += 2;
                        PUTWORD(4,func);
                        PUTWORD(8,blkid);
                    }
                    break;
                case MAPLE_CMD_WRITE_BLOCK:
                    func = GETWORD(12);
                    pt = GETBYTE(16);
                    phase = GETBYTE(17);
                    block = (GETBYTE(18)<<8) | GETBYTE(19);
                    if( dev->write_block == NULL )
                        status = MAPLE_ERR_CMD_UNKNOWN;
                    else {
                        status = dev->write_block(dev, func, pt, block, phase, 
                                buf+20, length-2);
                        if( status == 0 )
                            status = MAPLE_RESP_ACK;
                    }
                    break;
                case MAPLE_CMD_SYNC_BLOCK:
                    func = GETWORD(12);
                    pt = GETBYTE(16);
                    phase = GETBYTE(17);
                    block = (GETBYTE(18)<<8) | GETBYTE(19);
                    /* TODO: something? */
                    status = MAPLE_RESP_ACK;
                    break;
                default:
                    status = MAPLE_ERR_CMD_UNKNOWN;
                }
                return_buf[0] = status;
                return_buf[1] = send_addr;
                return_buf[2] = recv_addr;
                if( periph == 0 )
                    return_buf[2] |= maple_periph_mask[port];
                return_buf[3] = out_length;
            }
            buf += 12 + (length<<2);
            address += 12 + (length<<2);
        }
        event_schedule( EVENT_MAPLE_DMA, 200000 );
    }
}

void maple_attach_device( maple_device_t dev, unsigned int port,
                          unsigned int periph ) {
    assert( port < 4 );
    assert( periph < 6 );

    if( maple_devices[port][periph] != NULL ) {
        /* Detach existing peripheral first */
        maple_detach_device( port, periph );
    }

    maple_devices[port][periph] = dev;
    if( periph != 0 )
        maple_periph_mask[port] |= (1<<(periph-1));
    else maple_periph_mask[port] |= 0x20;
    if( dev->attach != NULL ) {
        dev->attach( dev );
    }
}

void maple_detach_device( unsigned int port, unsigned int periph ) {
    assert( port < 4 );
    assert( periph < 6 );

    maple_device_t dev = maple_devices[port][periph];
    if( dev == NULL ) /* already detached */
        return;
    maple_devices[port][periph] = NULL;
    if( dev->detach != NULL ) {
        dev->detach(dev);
    }
    if( dev->destroy != NULL ) {
        dev->destroy(dev);
    }
    if( periph == 0 ) {
        /* If we detach the main peripheral, we also have to detach all the
         * subperipherals, or the system could get quite confused
         */
        int i;
        maple_periph_mask[port] = 0;
        for( i=1; i<6; i++ ) {
            maple_detach_device(port,i);
        }
    } else {
        maple_periph_mask[port] &= (~(1<<(periph-1)));
    }

}

void maple_detach_all() {
    int i, j;
    for( i=0; i<4; i++ ) {
        for( j=0; j<6; j++ ) {
            if( maple_devices[i][j] != NULL ) {
                maple_device_t dev = maple_devices[i][j];
                if( dev->detach != NULL )
                    dev->detach(dev);
                if( dev->destroy != NULL )
                    dev->destroy(dev);
            }
        }
        maple_periph_mask[i] = 0;
    }
}

void maple_reattach_all() 
{
    int i, j;
    for( i=0; i<4; i++ ) {
        for( j=0; j<6; j++ ) {
            if( maple_devices[i][j] != NULL ) {
                maple_device_t dev = maple_devices[i][j];
                if( dev->detach != NULL ) 
                    dev->detach(dev);
                if( dev->attach != NULL )
                    dev->attach(dev);
            }
        }
    }
}

gboolean maple_should_grab()
{
    int mode = MAPLE_GRAB_DONTCARE;
    int i,j;
    for( i=0; i<4; i++ ) {
        for( j=0; j<6; j++ ) {
            if( maple_devices[i][j] != NULL ) {
                maple_device_t dev = maple_devices[i][j];
                if( (dev->device_class->flags&MAPLE_GRAB_MASK) > mode ) {
                    mode = dev->device_class->flags & MAPLE_GRAB_MASK;
                }
            }
        }
    }
    return mode == MAPLE_GRAB_YES;
}

void maple_default_destroy( maple_device_t mdev )
{
    free(mdev);
}
