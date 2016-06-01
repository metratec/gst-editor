#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/editor/editor.h>
#include <gst/element-browser/browser.h>
#include <gst/debug-ui/debug-ui.h>

int
main (int argc, char *argv[])
{
  GstElementFactory *chosen = NULL;

  //const struct poptOption *gst_table;
  GOptionContext *ctx;

  if (!g_thread_supported ())
    g_thread_init (NULL);
  g_type_init ();
  
//  glade_init ();
      
      //gst_table = gst_init_get_popt_table ();
      ctx = g_option_context_new (PACKAGE);
  //g_option_context_add_main_entries (ctx, options, GETTEXT_PACKAGE);
  g_option_context_add_group (ctx, gst_init_get_option_group ());

  gst_init (&argc, &argv);
  gtk_init (&argc, &argv);
  
#if 0
      if (!gnome_program_init ("GStreamer Plugin Inspector", VERSION,
          LIBGNOMEUI_MODULE, argc, argv, GNOME_PARAM_GOPTION_CONTEXT, ctx,
          NULL))
    g_error ("cannot gnome_progam_init(), aborting...");
  
#endif
      gste_init ();

  chosen = (GstElementFactory *) gst_element_browser_pick_modal ();
  if (chosen)
    g_print ("selected '%s'\n", GST_OBJECT_NAME (chosen));
  else
    g_print ("didn't choose any\n");
  return 0;
}
