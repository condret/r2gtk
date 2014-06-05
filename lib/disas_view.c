#include "hypertext.h"
#include "disas_view.h"

void new_disas_view_buf(GtkTextBuffer *buffer, RGDisasList *disas_list){
    GtkTextIter iter;
    RGTextBufIter *bufiter = (RGTextBufIter *)g_new(RGTextBufIter,1);
    
    gtk_text_buffer_set_text(buffer, "", 0);
    gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
    bufiter->buffer = buffer;
    bufiter->iter = &iter;
    
    g_list_foreach(disas_list->entries_list, (GFunc)text_buffer_insert_offset, bufiter);
}

int rg_disas_view(RGDisasList *disas_list){
    GtkWidget *window, *scrolled_win, *textview, *preview_textview, *viewport, *vbox;
    GtkAdjustment *horizontal, *vertical;
    GtkTextBuffer *buffer, *preview_buffer;
    PangoFontDescription *font;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Disas View");
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_widget_set_size_request (window, 250, 150);

    g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

    font = pango_font_description_from_string ("Monospace 10");
    
    preview_textview = gtk_text_view_new ();
    gtk_widget_modify_font (preview_textview, font);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (preview_textview), FALSE);

    preview_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (preview_textview));
    new_disas_view_buf(preview_buffer, disas_list);
    
    textview = gtk_text_view_new ();
    gtk_widget_modify_font (textview, font);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (textview), FALSE);
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (textview), GTK_WRAP_NONE);
    g_signal_connect (textview, "key-press-event", 
	    G_CALLBACK (key_press_event), NULL);
    g_signal_connect (textview, "event-after", 
	    G_CALLBACK (event_after), NULL);
    g_signal_connect (textview, "motion-notify-event", 
	    G_CALLBACK (motion_notify_event), preview_textview);
    g_signal_connect (textview, "visibility-notify-event", 
	    G_CALLBACK (visibility_notify_event), NULL);

    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
    new_disas_view_buf(buffer, disas_list);
    
    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled_win), textview);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    //horizontal = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scrolled_win));
    //vertical = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win));
    /*
    viewport = gtk_viewport_new (0,0); //(horizontal, vertical);
    gtk_container_add (GTK_CONTAINER (viewport), preview_textview);
    gtk_container_set_border_width (GTK_CONTAINER (viewport), 5);
    */
    viewport = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (viewport), preview_textview);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (viewport),
                                  GTK_POLICY_NEVER, GTK_POLICY_NEVER);
    vbox = gtk_vbox_new (TRUE, 5);
    gtk_box_pack_start_defaults (GTK_BOX (vbox), viewport);
    gtk_box_pack_start_defaults (GTK_BOX (vbox), scrolled_win);

    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_widget_show_all (window);
    
    /*
    gtk_container_add (GTK_CONTAINER (window), scrolled_win);
    gtk_widget_show_all (window);
    */
    gtk_main();
    return 0;
}

void text_buffer_insert_link(GtkTextBuffer *buffer, 
	     GtkTextIter   *iter, 
	     gchar         *text, 
	     gint           address)
{
  GtkTextTag *tag;
  
  tag = gtk_text_buffer_create_tag (buffer, NULL, 
				    "foreground", "blue", 
				    "underline", PANGO_UNDERLINE_SINGLE, 
				    NULL);
  g_object_set_data (G_OBJECT (tag), "address", GINT_TO_POINTER (address));
  gtk_text_buffer_insert_with_tags (buffer, iter, text, -1, tag, NULL);
}

void text_buffer_insert_address(GtkTextBuffer *buffer, 
	     GtkTextIter   *iter, 
	     gint           address)
{
    char text[32];
    GtkTextTag *tag;
  
    tag = gtk_text_buffer_create_tag (buffer, NULL, 
				    "foreground", "green", 
				    NULL);
    g_object_set_data (G_OBJECT (tag), "offset", GINT_TO_POINTER (address));
    snprintf(text, sizeof(text), "0x%08llx", (unsigned long long) address);
    gtk_text_buffer_insert_with_tags (buffer, iter, text, -1, tag, NULL);
}

void text_buffer_insert_xref(gpointer item, gpointer bi){
    RGXrefEntry *xref = (RGXrefEntry *) item;
    RGTextBufIter *bufiter = (RGTextBufIter *) bi;
    
    char line[128];
    snprintf(line, sizeof(line), " %s XREF from 0x%08llx (%s)\n", 
                xref->type,
                xref->address,
                xref->funname);
    
    text_buffer_insert_link(bufiter->buffer, bufiter->iter, line, xref->address);
}

void text_buffer_insert_offset(gpointer item, gpointer bi){
    RGTextBufIter *bufiter = (RGTextBufIter *) bi;
    RGDisasEntry *entry = (RGDisasEntry *)item;
    
    g_list_foreach(entry->xrefs, (GFunc)text_buffer_insert_xref, bufiter);
    
    char text[128];
    snprintf(text, sizeof(text), "0x%08llx", entry->offset);
    GtkTextMark *mark;
    
    mark = gtk_text_buffer_create_mark (bufiter->buffer,
                         text,
                         bufiter->iter,
                         TRUE);
    
    gtk_text_mark_set_visible(mark, TRUE);

    char line[128];
    snprintf(line, sizeof(line), " %s %s\n", 
                entry->asmop.buf_hex,
                entry->asmop.buf_asm);
    text_buffer_insert_address(bufiter->buffer, bufiter->iter, entry->offset);
    gtk_text_buffer_insert(bufiter->buffer, bufiter->iter, line, -1);
}

void new_pd_list(RCore *r, RGDisasCursor *cursor){
	RGDisasList *disas_list = g_new(RGDisasList, 1);
    
    disas_list->entries_ht = g_hash_table_new(g_direct_hash, g_direct_equal);
    disas_list->entries_list = NULL;
    
	unsigned int len =  r->blocksize;
    ut64 current_offset = r->offset;
    unsigned char *block;
    block = malloc(R_MAX(len*10, len));
    memcpy (block, r->block, len);
    r_core_read_at (r, cursor->offset+len, block+len, (len*10)-len);
    r->num->value = rg_disas_buf(r, cursor->offset, block, disas_list, len);
    
    free(block);
    r->offset = current_offset;
    
    rg_disas_view(disas_list);
}
