/**
 * $Id: mmio_win.c,v 1.8 2007-10-21 05:21:35 nkeynes Exp $
 *
 * Implements the MMIO register viewing window
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

#include <stdint.h>
#include <gnome.h>
#include "debugif.h"
#include "debugcb.h"
#include "gui/gtkui.h"
#include "mem.h"
#include "mmio.h"

GtkWidget *mmr_win;
GtkNotebook *mmr_book;

static void printbits( char *out, int nbits, uint32_t value )
{
    if( nbits < 32 ) {
        int i;
        for( i=32; i>nbits; i-- ) {
            if( !(i % 8) ) *out++ = ' ';
            *out++ = ' ';
        }
    }
    while( nbits > 0 ) {
        *out++ = (value&(1<<--nbits) ? '1' : '0');
        if( !(nbits % 8) ) *out++ = ' ';
    }
    *out = '\0';
}

static void printhex( char *out, int nbits, uint32_t value )
{
    char tmp[10], *p = tmp;
    int i;

    sprintf( tmp, "%08X", value );
    for( i=32; i>0; i-=4, p++ ) {
        if( i <= nbits ) *out++ = *p;
        else *out++ = ' ';
    }
    *out = '\0';
}
    
        
static GtkCList *create_mmr_page( char *name, struct mmio_region *io_rgn )
{
    GtkCList *list;
    GtkWidget *scroll;
    GtkWidget *tab;
    GtkCheckButton *trace_button;
    GtkVBox *vbox;

    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show( scroll );
    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(scroll),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS );
    list = GTK_CLIST(gtk_clist_new(5));
    gtk_clist_set_column_width(list, 0, 70);
    gtk_clist_set_column_width(list, 1, 75);
    gtk_clist_set_column_width(list, 2, 70);
    gtk_clist_set_column_width(list, 3, 280);
    gtk_clist_set_column_width(list, 4, 160);
    gtk_clist_set_column_justification(list, 0, GTK_JUSTIFY_CENTER );
    gtk_clist_set_column_justification(list, 2, GTK_JUSTIFY_CENTER );
    gtk_clist_set_column_justification(list, 3, GTK_JUSTIFY_CENTER );
    gtk_clist_set_column_title(list, 0, "Address");
    gtk_clist_set_column_title(list, 1, "Register");
    gtk_clist_set_column_title(list, 2, "Value");
    gtk_clist_set_column_title(list, 3, "Bit Pattern");
    gtk_clist_set_column_title(list, 4, "Description");
    gtk_clist_column_titles_show(list);
    gtk_widget_modify_font( GTK_WIDGET(list), gui_fixed_font );
    gtk_widget_show( GTK_WIDGET(list) );
    tab = gtk_label_new(_(name));
    gtk_widget_show( tab );
    gtk_container_add( GTK_CONTAINER(scroll), GTK_WIDGET(list) );
    
    vbox = GTK_VBOX(gtk_vbox_new( FALSE, 0 ));
    gtk_widget_show( GTK_WIDGET(vbox) );
    gtk_container_add( GTK_CONTAINER(vbox), GTK_WIDGET(scroll) );

    trace_button = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Trace access"));
    gtk_widget_show( GTK_WIDGET(trace_button) );
    gtk_container_add( GTK_CONTAINER(vbox), GTK_WIDGET(trace_button) );
    gtk_box_set_child_packing( GTK_BOX(vbox), GTK_WIDGET(trace_button), 
			       FALSE, FALSE, 0, GTK_PACK_START );
    gtk_notebook_append_page( mmr_book, GTK_WIDGET(vbox), tab );
    gtk_object_set_data( GTK_OBJECT(mmr_win), name, list );
    g_signal_connect ((gpointer) trace_button, "toggled",
                    G_CALLBACK (on_trace_button_toggled),
                    io_rgn);
    return list;
}

void mmio_win_update( mmio_window_t win )
{
    int i,j, count = 0;
    GtkCList *page, *all_page;
    char data[10], bits[40];

    all_page = GTK_CLIST(gtk_object_get_data( GTK_OBJECT(mmr_win), "All" ));
    
    for( i=0; i < num_io_rgns; i++ ) {
        page = GTK_CLIST(gtk_object_get_data( GTK_OBJECT(mmr_win),
                                              io_rgn[i]->id ));
        for( j=0; io_rgn[i]->ports[j].id != NULL; j++ ) {
            if( *io_rgn[i]->ports[j].val !=
                *(uint32_t *)(io_rgn[i]->save_mem+io_rgn[i]->ports[j].offset)){
                int sz = io_rgn[i]->ports[j].width;
                /* Changed */
                printhex( data, sz, *io_rgn[i]->ports[j].val );
                printbits( bits, sz, *io_rgn[i]->ports[j].val );
                
                gtk_clist_set_text( page, j, 2, data );
                gtk_clist_set_text( page, j, 3, bits );
                gtk_clist_set_foreground( page, j, &gui_colour_changed );
                
                gtk_clist_set_text( all_page, count, 2, data );
                gtk_clist_set_text( all_page, count, 3, bits );
                gtk_clist_set_foreground( all_page, count, &gui_colour_changed );
                
            } else {
                gtk_clist_set_foreground( page, j, &gui_colour_normal );
                gtk_clist_set_foreground( all_page, count, &gui_colour_normal );
            }
            count++;
        }
        memcpy( io_rgn[i]->save_mem, io_rgn[i]->mem, PAGE_SIZE );
    }
}

void init_mmr_win( void )
{
    int i, j;
    GtkCList *all_list;
    mmr_win = create_mmr_win();
    mmr_book = GTK_NOTEBOOK( gtk_object_get_data( GTK_OBJECT(mmr_win), "mmr_notebook" ));
    
    /* kill the dummy page glade insists on adding */
    gtk_notebook_remove_page( mmr_book, 0 );
    
    all_list = create_mmr_page( "All", NULL );
    for( i=0; i < num_io_rgns; i++ ) {
        GtkCList *list = create_mmr_page( io_rgn[i]->id, io_rgn[i] );
        
        for( j=0; io_rgn[i]->ports[j].id != NULL; j++ ) {
            int sz = io_rgn[i]->ports[j].width;
            char addr[10], data[10], bits[40];
            char *arr[] = { addr, io_rgn[i]->ports[j].id, data, bits,
                            io_rgn[i]->ports[j].desc };
            sprintf( addr, "%08X",
                     io_rgn[i]->base + io_rgn[i]->ports[j].offset );
            printhex( data, sz, *io_rgn[i]->ports[j].val );
            printbits( bits, io_rgn[i]->ports[j].width,
                       *io_rgn[i]->ports[j].val );
            gtk_clist_append( list, arr );
            gtk_clist_append( all_list, arr );
        }
    }
}

mmio_window_t mmio_window_new( const gchar *title )
{
    init_mmr_win();
    gtk_widget_show( mmr_win );
    return NULL;
}

void mmio_window_show( mmio_window_t mmio, gboolean show )
{
    gtk_widget_show( mmr_win );
}

void mmr_close_win( void )
{
    gtk_widget_hide( mmr_win );
}
