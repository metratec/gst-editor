#include <gtk/gtk.h>

void
gst_editor_popup_warning (const gchar * message)
{
  GtkWidget *dialog;

  /* throw up a dialog box; FIXME: we don't have a parent */
  /* FIXME: maybe even fallback the GError and do error
   * handling higher up ? */

  dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_WARNING,
      GTK_BUTTONS_CLOSE, message);
  gtk_widget_show (dialog);
  g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
      G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (dialog));
}
