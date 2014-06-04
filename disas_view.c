#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <r_core.h>

typedef struct rg_disas_list{
    GHashTable *entries_ht;
    GList *entries_list;
} RGDisasList;

typedef struct rg_list_t{
    int *items;
    size_t used;
    size_t size;
} RGList;

typedef struct text_buf_iter{
    GtkTextIter *iter;
    GtkTextBuffer *buffer;
} RGTextBufIter;

void init_list(RGList *list, size_t list_size) {
    list->items = (int *)malloc(list_size * sizeof(int));
    list->used = 0;
    list->size = list_size;
}

void list_append(RGList *list, void *item) {
    if (list->used == list->size) {
        list->size *= 2;
        list->items = (int *)realloc(list->items, list->size * sizeof(int));
    }
    list->items[list->used++] = item;
}

void free_list(RGList *list) {
    free(list->items);
    list->items = NULL;
    list->used = list->size = 0;
}

typedef struct rg_xref_entry_t{
    const char *type;
    ut64 address;
    char *funname;
} RGXrefEntry;

typedef struct rg_disas_entry_t{
    ut64 offset;
    short oplen;
    char *type;
    long jump;
    long fail;
    RAsmOp asmop;
	RAnalOp analop;
    GList *xrefs;
    
} RGDisasEntry;

typedef struct rg_disas_cursor_t{
    ut64 offset;
} RGDisasCursor;

/** HYPERTEXT STUFF **/
/* Looks at all tags covering the position of iter in the text view, 
 * and if one of them is a link, follow it by showing the page identified
 * by the data attached to it.
 */
static void
follow_if_link (GtkWidget   *text_view, 
		GtkTextIter *iter)
{
    GSList *tags = NULL, *tagp = NULL;
    GtkTextMark *mark;
    
    tags = gtk_text_iter_get_tags (iter);
    for (tagp = tags;  tagp != NULL;  tagp = tagp->next)
    {
        GtkTextTag *tag = tagp->data;
        gint address = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (tag), "address"));
        
        char text[128];
        snprintf(text, sizeof(text), "0x%08x", address);
        mark = gtk_text_buffer_get_mark(gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view)), text);
        
        gtk_text_view_scroll_to_mark (text_view,
                              mark,
                              0.0,
                              TRUE,
                              0.0,
                              0.5);
        //gtk_text_view_scroll_mark_onscreen (text_view,
        //                            mark);
        if (address != 0){
            //show_page (gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view)), page);
            break;
        }
    }

    if (tags){
        g_slist_free (tags);
    }
}

/* Links can be activated by pressing Enter.
 */
static gboolean
key_press_event (GtkWidget *text_view,
		 GdkEventKey *event)
{
    GtkTextIter iter;
    GtkTextBuffer *buffer;

    switch (event->keyval)
    {
        case GDK_Return: 
        case GDK_KP_Enter:
            buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
            gtk_text_buffer_get_iter_at_mark (buffer, &iter, 
                    gtk_text_buffer_get_insert (buffer));
            follow_if_link (text_view, &iter);
        break;

        default:
        break;
    }

    return FALSE;
}

/* Links can also be activated by clicking.
 */
static gboolean
event_after (GtkWidget *text_view,
	     GdkEvent  *ev)
{
    GtkTextIter start, end, iter;
    GtkTextBuffer *buffer;
    GdkEventButton *event;
    gint x, y;

    if (ev->type != GDK_BUTTON_RELEASE){
        return FALSE;
    }
    event = (GdkEventButton *)ev;

    if (event->button != 1){
        return FALSE;
    }
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (text_view));

    /* we shouldn't follow a link if the user has selected something */
    gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
    if (gtk_text_iter_get_offset (&start) != gtk_text_iter_get_offset (&end)){
        return FALSE;
    }
    gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view), 
                                         GTK_TEXT_WINDOW_WIDGET,
                                         event->x, event->y, &x, &y);

    gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (text_view), &iter, x, y);

    follow_if_link (text_view, &iter);

    return FALSE;
}

gboolean hovering_over_link = FALSE;
GdkCursor *hand_cursor = NULL;
GdkCursor *regular_cursor = NULL;
/* Looks at all tags covering the position (x, y) in the text view, 
 * and if one of them is a link, change the cursor to the "hands" cursor
 * typically used by web browsers.
 */
static void
set_cursor_if_appropriate (GtkTextView    *text_view,
                           gint            x,
                           gint            y)
{
    GSList *tags = NULL, *tagp = NULL;
    GtkTextBuffer *buffer;
    GtkTextIter iter;
    gboolean hovering = FALSE;
    hand_cursor = gdk_cursor_new (GDK_HAND2);
    regular_cursor = gdk_cursor_new (GDK_XTERM);
    buffer = gtk_text_view_get_buffer (text_view);

    gtk_text_view_get_iter_at_location (text_view, &iter, x, y);

    tags = gtk_text_iter_get_tags (&iter);
    for (tagp = tags;  tagp != NULL;  tagp = tagp->next){
        GtkTextTag *tag = tagp->data;
        gint address = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tag), "address"));

        if (address != 0){
            hovering = TRUE;
            break;
        }
    }

    if (hovering != hovering_over_link){
        hovering_over_link = hovering;

        if (hovering_over_link){
            gdk_window_set_cursor (gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_TEXT), hand_cursor);
        }
        else{
            gdk_window_set_cursor (gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_TEXT), regular_cursor);
        }
    }

    if (tags){
        g_slist_free (tags);
    }
}

/* Update the cursor image if the pointer moved.  */
static gboolean
motion_notify_event (GtkWidget      *text_view,
		     GdkEventMotion *event)
{
    gint x, y;

    gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view), 
                                         GTK_TEXT_WINDOW_WIDGET,
                                         event->x, event->y, &x, &y);

    set_cursor_if_appropriate (GTK_TEXT_VIEW (text_view), x, y);

    gdk_window_get_pointer (text_view->window, NULL, NULL, NULL);
    return FALSE;
}

/* Also update the cursor image if the window becomes visible
 * (e.g. when a window covering it got iconified).
 */
static gboolean
visibility_notify_event (GtkWidget          *text_view,
			 GdkEventVisibility *event)
{
    gint wx, wy, bx, by;

    gdk_window_get_pointer (text_view->window, &wx, &wy, NULL);

    gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view), 
                                         GTK_TEXT_WINDOW_WIDGET,
                                         wx, wy, &bx, &by);

    set_cursor_if_appropriate (GTK_TEXT_VIEW (text_view), bx, by);

    return FALSE;
}

/** END HYPERTEXT STUFF **/
void print_xrefs(gpointer item){
    RGXrefEntry *xref = (RGXrefEntry *) item;
    
    printf("\nxref_type: %s", xref->type);
    printf(" funname: %s", xref->funname);
    printf(" address: %"PFMT64d, xref->address);
}

void print_list(gpointer item){
    RGDisasEntry *entry = (RGDisasEntry *)item;
    
    g_list_foreach(entry->xrefs, (GFunc)print_xrefs, NULL);
    printf("offset: %"PFMT64d, entry->offset);
    printf(" size: %d", entry->oplen);
    printf(" opcode: %s", entry->asmop.buf_asm);
    printf(" bytes: %s", entry->asmop.buf_hex);
    printf(" type: %s\n", r_anal_optype_to_string (entry->analop.type));
    
}

void rg_get_xrefs_at_offset(RCore *r, ut64 offset, RGDisasEntry *entry){
	RAnalFunction *f = r_anal_fcn_find(r->anal, offset, R_ANAL_FCN_TYPE_NULL);
	RList *xrefs;
	RAnalRef *refi;
	RListIter *iter;
    if(xrefs = r_anal_xref_get(r->anal, offset)){
        r_list_foreach(xrefs, iter, refi){
            if(refi->at == offset){
                RAnalFunction *fun = r_anal_fcn_find (
			                            r->anal, refi->addr,
			                            R_ANAL_FCN_TYPE_FCN |
			                            R_ANAL_FCN_TYPE_ROOT);
                
                const char *_xref_type = "UNKNOWN";
                switch(refi->type){
                    case R_ANAL_REF_TYPE_NULL:
                        _xref_type = "UNKNOWN";
                    break;
                    case R_ANAL_REF_TYPE_CODE:
                        _xref_type = "JMP";
                    break;
                    case R_ANAL_REF_TYPE_CALL:
                        _xref_type = "CALL";
                    break;
                    case R_ANAL_REF_TYPE_DATA:
                        _xref_type = "DATA";
                    break;
                    case R_ANAL_REF_TYPE_STRING:
                        _xref_type = "STRING";
                    break;
                }
                entry->xrefs = NULL;
                RGXrefEntry *xref_entry = g_new(RGXrefEntry, 1);
                xref_entry->type = _xref_type;
                xref_entry->address = refi->addr;
                xref_entry->funname = fun ? fun->name : "unk";
                
                entry->xrefs = g_list_prepend(entry->xrefs, xref_entry);
            }
        }
        entry->xrefs = g_list_reverse(entry->xrefs);
    }
}

int rg_disas_buf(RCore *r, ut64 addr, ut8 *buf, RGDisasList *disas_list, int len) {
	RAsmOp asmop;
	RAnalOp analop;
	int i, oplen, ret;
    
    
	if (r->anal && r->anal->cur && r->anal->cur->reset_counter	) {
		r->anal->cur->reset_counter (r->anal, addr);
	}
	for (i=0; i<len;) {
        RGDisasEntry *entry = g_new(RGDisasEntry, 1);
                
		ut64 offset = addr +i;

		r_asm_set_pc (r->assembler, offset);
		ret = r_asm_disassemble (r->assembler, &asmop, buf+i, len-i+5);
        
        entry->offset = offset;
        entry->asmop = asmop;
        
	    if(ret<1){
	        entry->type = "invalid";
	        entry->oplen = 1;
	        i++;
	        continue;
	    }
		r_anal_op (r->anal, &analop, offset, buf+i, len-i+5);

		oplen = r_asm_op_get_size (&asmop);
        
        entry->oplen = oplen;
        entry->analop = analop;
        entry->type = r_anal_optype_to_string (analop.type);
        
        if (analop.jump != UT64_MAX) {
			entry->jump = analop.jump;
			if (analop.fail != UT64_MAX)
				entry->fail = analop.fail;
		}
		
		entry->xrefs = NULL;
		rg_get_xrefs_at_offset(r, offset, entry);
		//g_list_foreach(entry->xrefs, (GFunc)print_xrefs, NULL);
		disas_list->entries_list = g_list_prepend(disas_list->entries_list, entry);
		g_hash_table_insert(disas_list->entries_ht, entry->offset, entry);
		i += oplen;
	}
	disas_list->entries_list = g_list_reverse(disas_list->entries_list);
	return R_TRUE;
}

void new_pd_list(RCore *r, RGDisasCursor *cursor){
	RGDisasList *disas_list = g_new(RGDisasList, 1);
    
    disas_list->entries_ht = g_hash_table_new(g_direct_hash, g_direct_equal);
    disas_list->entries_list = NULL;
    
	int len =  r->blocksize;
    ut64 current_offset = r->offset;
    const int bs = r->blocksize;
    char *block;
    block = malloc(R_MAX(len*10, len));
    memcpy (block, r->block, len);
    r_core_read_at (r, cursor->offset+len, block+len, (len*10)-len);
    r->num->value = rg_disas_buf(r, cursor->offset, block, disas_list, len);
    
    free(block);
    r->offset = current_offset;
    
    rg_disas_view(disas_list);
}

void new_pda_list(RCore *r, RGDisasCursor *cursor){}
void new_pdb_list(RCore *r){}
void new_pdr_list(RCore *r){}
void new_pdf_list(RCore *r){}
void new_pdi_list(RCore *r, ut64 *offset, ut64 len){}
void new_pds_list(RCore *r){}

static struct r_core_t r;

int main(int argc, char **argv, char **envp) {
    gtk_init (&argc, &argv);
    ut64 baddr = 0LL;
    RCoreFile *fh = NULL;
    char *pfile = NULL, *file = NULL, *filepath=NULL;
    int perms = R_IO_READ;
    ut64 mapaddr = 0LL;
    const char *asmarch = NULL;
	const char *asmos = NULL;
	const char *asmbits = NULL;
	short fullfile = 1, quiet=0, do_analysis=1;
	
	r_sys_set_environ (envp);
	
    r_core_init (&r);

	r_core_loadlibs (&r, R_CORE_LOADLIBS_ALL, NULL);
    r_config_set_i (r.config, "bin.baddr", baddr);
        
    pfile = argv[1];
	fh = r_core_file_open (&r, pfile, perms, mapaddr);
	if (perms & R_IO_WRITE) {
		if (!fh) {
			r_io_create (r.io, pfile, 0644, 0);
			fh = r_core_file_open (&r, pfile, perms, mapaddr);
		}
	}
    if (fh == NULL) {
		if (pfile && *pfile) {
			if (perms & R_IO_WRITE)
				eprintf ("Cannot open '%s' for writing.\n", pfile);
			else eprintf ("Cannot open '%s'\n", pfile);
		} else eprintf ("Missing file to open\n");
		return 1;
	}
	if (r.file == NULL){ // no given file
		return 1;
    }
    if (r.file && r.file->filename){
		filepath = r.file->filename;
    }
	if (!r_core_bin_load (&r, filepath, baddr)){
		r_config_set (r.config, "io.va", "false");
	}
    if (asmarch) r_config_set (r.config, "asm.arch", asmarch);
	if (asmbits) r_config_set (r.config, "asm.bits", asmbits);
	if (asmos) r_config_set (r.config, "asm.os", asmos);
	r_core_bin_update_arch_bits (&r);
	if (fullfile) r_core_block_size (&r, r.file->size);
	r_core_seek (&r, r.offset, 1);
	
	const char *global_rc = R2_PREFIX"/share/radare2/radare2rc";
	if (r_file_exists (global_rc)){
		(void)r_core_run_script (&r, global_rc);
	}
	
	char f[128];
	snprintf(f, sizeof(f), "%s.r2", pfile);
	if (r_file_exists (f)) {
		if (!quiet)
			eprintf ("NOTE: Loading '%s' script.\n", f);
		r_core_cmd_file (&r, f);
	}
	
	if (do_analysis) {
		r_core_cmd0 (&r, "aa");
		r_cons_flush ();
	}
	
	r.num->value = 0;
	
    RGDisasCursor *cursor=g_new(RGDisasCursor, 1);
    cursor->offset = r.offset;
    new_pd_list(&r, cursor);
    
    return 0;
}

static void 
text_buffer_insert_link(GtkTextBuffer *buffer, 
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

static void 
text_buffer_insert_address(GtkTextBuffer *buffer, 
	     GtkTextIter   *iter, 
	     gint           address)
{
  
  GtkTextTag *tag;
  
  tag = gtk_text_buffer_create_tag (buffer, NULL, 
				    "foreground", "green", 
				    NULL);
  g_object_set_data (G_OBJECT (tag), "offset", GINT_TO_POINTER (address));
  
  char text[32];
  snprintf(text, sizeof(text), "0x%08x", GINT_TO_POINTER(address));
  gtk_text_buffer_insert_with_tags (buffer, iter, text, -1, tag, NULL);
  
  
              
}


static void text_buffer_insert_xref(gpointer item, gpointer bi)
{
    RGXrefEntry *xref = (RGXrefEntry *) item;
    RGTextBufIter *bufiter = (RGTextBufIter *) bi;
    
    char line[128];
    snprintf(line, sizeof(line), " %s XREF from 0x%08x (%s)\n", 
                xref->type,
                xref->address,
                xref->funname);
    
    
    text_buffer_insert_link(bufiter->buffer, bufiter->iter, line, xref->address);
}
    
static void text_buffer_insert_offset(gpointer item, gpointer bi)
{
    RGTextBufIter *bufiter = (RGTextBufIter *) bi;
    RGDisasEntry *entry = (RGDisasEntry *)item;
    
    g_list_foreach(entry->xrefs, (GFunc)text_buffer_insert_xref, bufiter);
    
    char text[128];
    snprintf(text, sizeof(text), "0x%08x", entry->offset);
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


static void new_disas_view_buf(GtkTextBuffer *buffer, RGDisasList *disas_list){
    GtkTextIter iter;
    GList *item;
    RGTextBufIter *bufiter = (RGTextBufIter *)g_new(RGTextBufIter,1);
    
    gtk_text_buffer_set_text(buffer, "", 0);
    gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
    bufiter->buffer = buffer;
    bufiter->iter = &iter;
    
    g_list_foreach(disas_list->entries_list, (GFunc)text_buffer_insert_offset, bufiter);
}

int rg_disas_view(RGDisasList *disas_list){
  GtkWidget *window, *scrolled_win, *textview;
  GtkTextBuffer *buffer;
  PangoFontDescription *font;
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Disas View");
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  gtk_widget_set_size_request (window, 250, 150);
  
  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);
  
  font = pango_font_description_from_string ("Monospace 10");
  textview = gtk_text_view_new ();
  gtk_widget_modify_font (textview, font);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (textview), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (textview), GTK_WRAP_NONE);
  g_signal_connect (textview, "key-press-event", 
		G_CALLBACK (key_press_event), NULL);
  g_signal_connect (textview, "event-after", 
		G_CALLBACK (event_after), NULL);
  g_signal_connect (textview, "motion-notify-event", 
		G_CALLBACK (motion_notify_event), NULL);
  g_signal_connect (textview, "visibility-notify-event", 
		G_CALLBACK (visibility_notify_event), NULL);



  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
  new_disas_view_buf(buffer, disas_list);
  
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win), textview);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  
  gtk_container_add (GTK_CONTAINER (window), scrolled_win);
  gtk_widget_show_all (window);
  
  gtk_main();
  return 0;
}
