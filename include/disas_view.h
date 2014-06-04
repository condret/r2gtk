#include <gtk/gtk.h>
#include "disas.h"
#include "hypertext.h"

typedef struct text_buf_iter{
    GtkTextIter *iter;
    GtkTextBuffer *buffer;
} RGTextBufIter;

void text_buffer_insert_link(GtkTextBuffer *buffer, GtkTextIter   *iter, 
                        gchar *text, gint address);

void text_buffer_insert_address(GtkTextBuffer *buffer, GtkTextIter *iter, 
                                        gint address);


void text_buffer_insert_xref(gpointer item, gpointer bi);

void text_buffer_insert_offset(gpointer item, gpointer bi);

void new_disas_view_buf(GtkTextBuffer *buffer, RGDisasList *disas_list);
int rg_disas_view(RGDisasList *disas_list);
void new_pd_list(RCore *r, RGDisasCursor *cursor);
