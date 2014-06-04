#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
/* Looks at all tags covering the position of iter in the text view, 
 * and if one of them is a link, follow it by showing the page identified
 * by the data attached to it.
 */
void follow_if_link(GtkWidget   *text_view, GtkTextIter *iter){
    GSList *tags = NULL, *tagp = NULL;
    GtkTextMark *mark;
    char text[128];
    tags = gtk_text_iter_get_tags (iter);
    for (tagp = tags;  tagp != NULL;  tagp = tagp->next){
        GtkTextTag *tag = tagp->data;
        gint address = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (tag), "address"));
        snprintf(text, sizeof(text), "0x%08x", address);
        mark = gtk_text_buffer_get_mark(
                    gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view)), 
                    text
               );
        gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (text_view), mark, 0.0, TRUE, 0.0, 0.5);
        if (address != 0){
            break;
        }
    }
    if (tags){
        g_slist_free (tags);
    }
}

/* Links can be activated by pressing Enter.
 */
gboolean key_press_event(GtkWidget *text_view, GdkEventKey *event){
    GtkTextIter iter;
    GtkTextBuffer *buffer;
    switch (event->keyval){
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
gboolean event_after(GtkWidget *text_view, GdkEvent  *ev){
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
void set_cursor_if_appropriate(GtkTextView *text_view, gint x, gint y){
    GSList *tags = NULL, *tagp = NULL;
    GtkTextIter iter;
    gboolean hovering = FALSE;
    
    hand_cursor = gdk_cursor_new (GDK_HAND2);
    regular_cursor = gdk_cursor_new (GDK_XTERM);
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
gboolean motion_notify_event(GtkWidget *text_view, GdkEventMotion *event){
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
gboolean visibility_notify_event(GtkWidget *text_view, GdkEventVisibility *event){
    gint wx, wy, bx, by;

    gdk_window_get_pointer (text_view->window, &wx, &wy, NULL);
    gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view), 
                                         GTK_TEXT_WINDOW_WIDGET,
                                         wx, wy, &bx, &by);
    set_cursor_if_appropriate (GTK_TEXT_VIEW (text_view), bx, by);
    return FALSE;
}

