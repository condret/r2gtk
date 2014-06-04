#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

void follow_if_link (GtkWidget   *text_view, GtkTextIter *iter);
gboolean key_press_event (GtkWidget *text_view, GdkEventKey *event);
gboolean event_after (GtkWidget *text_view, GdkEvent  *ev);
void set_cursor_if_appropriate (GtkTextView *text_view, gint x, gint y);
gboolean motion_notify_event (GtkWidget *text_view, GdkEventMotion *event);
gboolean visibility_notify_event (GtkWidget *text_view, GdkEventVisibility *event);
