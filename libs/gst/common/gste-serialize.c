/* gst-editor
 * Copyright (C) <2016> Robin Haberkorn <haberkorn@metratec.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Serialization of Gstreamer elements and pipelines into a format
 * compatible with GstParse.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <gst/gst.h>

#include "gste-serialize.h"

typedef struct _GsteSerializeCallbacks {
  GsteSerializeAppendCallback append;
  GsteSerializeObjectSavedCallback object_saved;
  gpointer user_data;

  GsteSerializeFlags flags;
  guint recursion_depth;
} GsteSerializeCallbacks;

static guint gst_element_count_pads (GstElement * element, GstPadDirection direction);

static void gst_object_save_thyself (GstObject * object,
    GstElement * last_element, GstElement * next_element, GsteSerializeCallbacks * cb);

static inline void
append_space (GsteSerializeCallbacks * cb)
{
  /*
   * The first space is omitted.
   */
  if (cb->flags & GSTE_SERIALIZE_NEED_SPACE)
    cb->append (" ", cb->user_data);
  cb->flags |= GSTE_SERIALIZE_NEED_SPACE;
}

/**
 * Quote a string so that GstParse will interpret the quoted
 * string to mean unquoted_string.
 *
 * This follows the lexical rules of GstParse's parse.l.
 * That's why neither g_shell_quote(), nor g_strescape()
 * can be used.
 *
 * Quotes are avoided if possible.
 */
static gchar *
gste_serialize_quote (const gchar *unquoted_string)
{
  gsize quoted_len = 1 + 1 + 1;
  gchar *quoted, *p;

  if (!strchr (unquoted_string, ' '))
    return g_strdup (unquoted_string);

  for (const gchar *p = unquoted_string; *p; p++)
    quoted_len += *p == '"' ? 2 : 1;
  p = quoted = g_malloc (quoted_len);

  *p++ = '"';
  while (*unquoted_string) {
    if (*unquoted_string == '"')
      *p++ = '\\';
    *p++ = *unquoted_string++;
  }
  *p++ = '"';
  *p = '\0';

  return quoted;
}

static inline gboolean
gst_is_capsfilter (GstObject * object, GsteSerializeCallbacks * cb)
{
  /*
   * GstCapsFilter has no public header, so we identify it using
   * its class name.
   */
  return !(cb->flags & GSTE_SERIALIZE_CAPSFILTER_AS_ELEMENT) &&
      !g_strcmp0 (G_OBJECT_TYPE_NAME (object), "GstCapsFilter");
}

static GstPad *
gst_pad_get_peer_with_caps (GstPad * pad, GList ** caps_list, GsteSerializeCallbacks * cb)
{
  GstPad *peer = gst_pad_get_peer (pad);

  while (peer && GST_OBJECT_PARENT (peer)) {
    GstObject *peer_parent = GST_OBJECT_PARENT (peer);

    /*
     * Proxy/Ghost pads are only skipped for real GstBins.
     * Bin derivations are assumed to manage their source/sink pads,
     * so we can refer to their name (we have to unless GSTE_SERIALIZE_ALL_BINS
     * is set as otherwise we may not serialize any GstBin children).
     */
    gboolean skip_proxy_pads = G_OBJECT_TYPE (peer_parent) == GST_TYPE_BIN;

    if (skip_proxy_pads && G_OBJECT_TYPE (peer) == GST_TYPE_PROXY_PAD) {
      /*
       * Skip links to proxy pads, so we end up with a "normal" peer
       * in the bin it is linked to.
       * While it is possible to serialize links between bins
       * with the syntax (...bin...) ! (...bin...), this will fail
       * if the bins have multiple ghost pads as we will have to qualify
       * the pad name but have no way to define which elements in the
       * bin they are connected to.
       */
      GstPad *ghost = GST_PAD (peer_parent);
      gst_object_unref (peer);
      peer = gst_pad_get_peer (ghost);
    } else if (skip_proxy_pads && GST_IS_GHOST_PAD (peer)) {
      /*
       * Skip links to ghost pads. See above.
       */
      GstGhostPad *ghost = GST_GHOST_PAD_CAST (peer);
      peer = gst_ghost_pad_get_target (ghost);
      gst_object_unref (ghost);
    } else if (gst_is_capsfilter (peer_parent, cb)) {
      /*
       * If the special GstCapsFilter syntax is allowed,
       * we return all capabilities of capsfilters between
       * `pad` and the last capsfilter's peer.
       */
      GstPad *caps_srcpad;

      /*
       * We assume that capsfilters always have the "caps" property.
       * The alternative would be to use g_object_get_property() and
       * gst_value_serialize().
       * It may theoretically not contain any capabilities though.
       * We simply ignore those capsfilters.
       */
      if (caps_list) {
        GstCaps *caps;

        g_object_get (GST_OBJECT_PARENT (peer), "caps", &caps, NULL);
        if (caps) {
          /*
           * Empty and ANY caps cannot be serialized in the special
           * filtered link syntax and they do not contribute anything
           * meaningful anyway.
           */
          if (!gst_caps_is_empty (caps) && !gst_caps_is_any (caps))
            /* transfer ownership of caps to caps_list */
            *caps_list = g_list_append (*caps_list, caps);
          else
            gst_caps_unref (caps);
        }
      }

      /*
       * We can assert that all GstCapsFilter elements have a
       * src pad.
       */
      caps_srcpad = gst_element_get_static_pad (GST_ELEMENT (GST_OBJECT_PARENT (peer)),
          "src");
      g_assert (caps_srcpad != NULL);
      gst_object_unref (peer);
      peer = gst_pad_get_peer (caps_srcpad);
      gst_object_unref (caps_srcpad);
    } else {
      break;
    }
  }

  return peer;
}

static void
gst_pad_save_name (GstPad * pad, gboolean have_dot, GsteSerializeCallbacks * cb)
{
  GstPadDirection direction = gst_pad_get_direction (pad);
  GstObject *parent = GST_OBJECT_PARENT (pad);
  GstPadTemplate *pad_template = gst_pad_get_pad_template (pad);

  if (pad_template &&
      GST_PAD_TEMPLATE_PRESENCE (pad_template) != GST_PAD_ALWAYS) {
    /*
     * We must serialize the link using its pad template name.
     * FIXME: If the element has no normal pads and only one
     * pad template, we could omit the pad template name.
     */
    if (!have_dot) {
      append_space (cb);
      cb->append (".", cb->user_data);
    }
    cb->append (GST_PAD_TEMPLATE_NAME_TEMPLATE (pad_template), cb->user_data);
  } else if (cb->flags & GSTE_SERIALIZE_VERBOSE ||
      !parent || gst_element_count_pads (GST_ELEMENT (parent), direction) > 1) {
    /*
     * The pad name can be omitted if there is only one
     * pad of the same type in the object.
     */
    if (!have_dot) {
      append_space (cb);
      cb->append (".", cb->user_data);
    }
    cb->append (GST_OBJECT_NAME (pad), cb->user_data);
  }

  if (pad_template)
    gst_object_unref (pad_template);
}

static void
gst_pad_save_thyself (GstPad * pad, GstElement * last_element, GstElement * next_element,
    GsteSerializeCallbacks * cb)
{
  GstObject *parent = GST_OBJECT_PARENT (pad);
  GstPad *peer;
  GstObject *peer_parent;

  /* list of GstCaps that we own */
  GList *caps_list = NULL;

  peer = gst_pad_get_peer_with_caps (pad, &caps_list, cb);
  if (!peer) {
    /* no need to serialize an unlinked pad */
    g_list_free_full (caps_list, (GDestroyNotify)gst_caps_unref);
    return;
  }

  peer_parent = GST_OBJECT_PARENT (peer);
  if (!peer_parent) {
    /* no need to serialize a pad not linked to an element */
    g_list_free_full (caps_list, (GDestroyNotify)gst_caps_unref);
    gst_object_unref (peer);
    return;
  }

  /*
   * For pads serialized as part of an element, the link's source
   * element can be omitted.
   */
  if (parent &&
      (cb->flags & GSTE_SERIALIZE_VERBOSE || parent != GST_OBJECT_CAST (last_element))) {
    append_space (cb);
    /*
     * This assumes that the peer's parent already has an unique name.
     * FIXME: Guarantee that.
     */
    cb->append (GST_OBJECT_NAME (parent), cb->user_data);
    cb->append (".", cb->user_data);
    gst_pad_save_name (pad, TRUE, cb);
  } else {
    gst_pad_save_name (pad, FALSE, cb);
  }

  /*
   * Serialize all capability filters.
   * This list is empty if GSTE_SERIALIZE_CAPSFILTER_AS_ELEMENT is given.
   *
   * NOTE: We could iterate the capsfilters here, leaving peer/peer_parent
   * at the last non-GstCapsFilter element, but this way we can completely
   * ignore unlinked capsfilters (which may not be correct...)
   */
  if (caps_list != NULL) {
    /*
     * We do not have to qualify the link to the capsfilter
     * element since it is guaranteed to be serialized immediately
     * following the link (and we cannot specify its "name" anyway).
     * GstCapsFilter elements only have one static "sink" and "src" pad.
     */
    append_space (cb);
    cb->append ("!", cb->user_data);

    append_space (cb);
    /*
     * NOTE: Why the gst-launch manpage claims that capsfilters can
     * be escaped; this actually only refers to the POSIX shell escaping.
     * When parsing with gst_parse_launch() is used for parsing, we do not
     * actually need the quotes and they will result in parsing errors.
     */
    //cb->append ("'", cb->user_data);

    for (GList *cur = caps_list; cur != NULL; cur = g_list_next (cur)) {
      gchar *contents = gst_caps_to_string (GST_CAPS_CAST (cur->data));
      cb->append (contents, cb->user_data);
      g_free (contents);

      if (g_list_next (cur))
        cb->append (":", cb->user_data);
    }

    //cb->append ("'", cb->user_data);
  }

  g_list_free_full (caps_list, (GDestroyNotify)gst_caps_unref);

  append_space (cb);
  cb->append ("!", cb->user_data);

  /*
   * For pads serialized as part of a bin, the link's sink
   * element can be omitted since often it will refer to the
   * next element in the bin.
   */
  if (cb->flags & GSTE_SERIALIZE_VERBOSE ||
      peer_parent != GST_OBJECT_CAST (next_element)) {
    append_space (cb);
    /*
     * This assumes that the peer's parent already has a unique name.
     * FIXME: Guarantee that.
     */
    cb->append (GST_OBJECT_NAME (peer_parent), cb->user_data);
    cb->append (".", cb->user_data);
    gst_pad_save_name (peer, TRUE, cb);
  } else {
    gst_pad_save_name (peer, FALSE, cb);
  }

  gst_object_unref (peer);
}

static void
gst_object_save_properties (GstObject * object, GsteSerializeCallbacks * cb)
{
  GObjectClass *klass = G_OBJECT_GET_CLASS (object);
  GParamSpec **specs;
  guint nspecs;

  /*
   * Iterate all properties of the object's class.
   */
  specs = g_object_class_list_properties (klass, &nspecs);
  for (guint i = 0; i < nspecs; i++) {
    GParamSpec *spec = specs[i];
    GValue value = G_VALUE_INIT;

    if (!(spec->flags & G_PARAM_READABLE))
      continue;

    /*
     * It makes no sense to serialize properties that GstParse cannot
     * deserialize anyway.
     * This is not even done in VERBOSE mode, as the resulting pipeline
     * may produce parsing errors.
     */
    if (!(spec->flags & (G_PARAM_WRITABLE | G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY)))
      continue;

    /*
     * Omit the "parent" property: It is implicit by the hierarachy
     * of elements.
     * NOTE: If the first element passed into the save call (recursion = 1)
     * has a "parent" property, it may be a good idea to output the "parent"
     * property. On the other hand, when deserializing a stand-alone element
     * again, the "parent" may not refer to an existing element.
     */
    if (!strcmp (spec->name, "parent"))
      continue;

    g_value_init (&value, spec->value_type);
    g_object_get_property (G_OBJECT (object), spec->name, &value);

    if (!(cb->flags & GSTE_SERIALIZE_VERBOSE) && g_param_value_defaults (spec, &value)) {
      /*
       * To make output more readable, properties with their default values can
       * be skipped since GstParse will initialize them with the same value
       * anyway.
       * The default value is not logged since we cannot easily
       * serialize it without risking critical messages.
       */
      g_debug ("Skipping property \"%s\" in class %s with default value",
               spec->name, G_OBJECT_CLASS_NAME (klass));
    } else if (GST_VALUE_HOLDS_STRUCTURE (&value) && !gst_value_get_structure (&value)) {
      /*
       * NOTE: Apparently at least for some versions of GStreamer, we need
       * a workaround for NULL GstStructure properties since otherwise,
       * gst_value_serialize() will log a critical message.
       */
      append_space (cb);
      cb->append (spec->name, cb->user_data);
      cb->append ("=\"\"", cb->user_data);
    } else {
      /*
       * The GstParse deserialization (see gst_parse_element_set() in grammar.y)
       * uses gst_value_deserialize(), so this should produce the desired output.
       * There is also g_strdup_value_contents() but it should only be used
       * for debug output.
       * The element name is a property of GstObject and should thus be
       * serialized automatically.
       */
      gchar *contents = gst_value_serialize (&value);

      if (contents) {
        /*
         * The quoting is necessary because some properties like GstCaps and
         * GEnums may be serialized with spaces which must be escaped or quoted
         * in GstParse strings (see parser.l).
         * All other syntactic components that we generate are identifiers
         * which cannot be quoted anyway.
         */
        gchar *quoted = gste_serialize_quote (contents);

        append_space (cb);
        cb->append (spec->name, cb->user_data);
        cb->append ("=", cb->user_data);
        cb->append (quoted, cb->user_data);

        g_free (quoted);
      } else {
        /*
         * This is more or less expected for certain properties with opaque
         * element-specific types (e.g. an opaque C pointer).
         * But there may also be less exotic properties that must be handled
         * separately.
         * Since GstParse can only deserialize with gst_value_deserialize(),
         * this should not be worth warning, though.
         */
        g_debug ("Cannot serialize property \"%s\" of type %s in class %s",
            spec->name, g_type_name (spec->value_type), G_OBJECT_CLASS_NAME (klass));
      }

      g_free (contents);
    }

    g_value_unset (&value);
  }
  g_free (specs);
}

static guint
gst_element_count_pads (GstElement * element, GstPadDirection direction)
{
  guint ret = 0;

  for (GList *pads = g_list_last (GST_ELEMENT_PADS (element));
       pads; pads = g_list_previous (pads)) {
    GstPad *pad = GST_PAD_CAST (pads->data);

    if (gst_pad_get_direction (pad) == direction)
      ret++;
  }

  return ret;
}

static void
gst_element_save_pads (GstElement * element, GstElement * next_element,
    GsteSerializeCallbacks * cb)
{
  GList *pads_filtered = NULL;

  /*
   * When serializing a single element from a larger pipeline,
   * it does not make sense to output its pads/links since the elements
   * might not be available when they are deserialized.
   */
  if (cb->recursion_depth == 0)
    return;

  /*
   * Create a filtered pad list containing only the pads
   * that will be serialized in the end.
   * This is important for passing the previous/next element
   * pointers to the pad serialization.
   */
  for (GList *pads = GST_ELEMENT_PADS (element);
       pads != NULL; pads = g_list_next (pads)) {
    GstPad *pad = GST_PAD_CAST (pads->data);
    GstPad *peer;
    GstObject *peer_parent;

    /*
     * Only serialize direct pads and source pads.
     * If we are serializing as part of an element, there is no
     * need to serialize both source and sink pads since each
     * link only needs to appear once in the pipeline and in general
     * we want to serialize from source to sink pads (as the number of
     * fully-qualified peer names can be reduced).
     */
    if (GST_ELEMENT_CAST (GST_OBJECT_PARENT (pad)) != element ||
        gst_pad_get_direction (pad) == GST_PAD_SINK)
      continue;

    /*
     * Unlinked pads cannot get serialized.
     * This check is redundantly in gst_pad_save_thyself() since
     * it matters for stand-alone pad serialization as well.
     */
    peer = gst_pad_get_peer_with_caps (pad, NULL, cb);
    if (!peer)
      continue;
    peer_parent = GST_OBJECT_PARENT (peer);
    gst_object_unref (peer);

    /*
     * Pads not linked to any element cannot get serialized.
     * This check is redundantly in gst_pad_save_thyself() since
     * it matters for stand-alone pad serialization as well.
     */
    if (!peer_parent)
      continue;

    pads_filtered = g_list_append (pads_filtered, pad);
  }

  for (GList *cur = pads_filtered; cur != NULL; cur = g_list_next (cur)) {
    /*
     * This uses the generic gst_object_save_thyself(), so
     * the object_saved callback gets called.
     *
     * The last_element gets only set for the first serialized pad
     * and the next_element for the last since we can only omit immediately
     * preceding and following element names.
     */
    gst_object_save_thyself (GST_OBJECT (cur->data),
        cur == pads_filtered ? element : NULL,
        g_slist_next (cur) ? NULL : next_element, cb);
  }

  g_list_free (pads_filtered);
}

static void
gst_element_save_thyself (GstElement * element, GstElement * next_element,
    GsteSerializeCallbacks * cb)
{
  GstElementFactory *factory = gst_element_get_factory (element);

  /*
   * The GstParse syntax expects the name of the element's
   * factory instead of its GType name.
   */
  append_space (cb);
  cb->append (GST_OBJECT_NAME (factory), cb->user_data);

  /*
   * Serialize all properties of the element.
   * A space will be output after every property.
   */
  gst_object_save_properties (GST_OBJECT (element), cb);

  /*
   * This outputs links following the element serialization.
   */
  gst_element_save_pads (element, next_element, cb);
}

static void
gst_bin_save_children (GstBin * bin, GsteSerializeCallbacks * cb)
{
  GList *children = g_list_last (GST_BIN_CHILDREN (bin));

  cb->recursion_depth++;

  /*
   * All elements of the bin are serialized in reverse order
   * (ie. the order thay have been added to the bin).
   * NOTE: By using the reverse of gst_bin_iterate_sorted()
   * the number of named links may be minimized.
   * Both iteration orders have their advantages:
   *  - The natural order might reflect what was in the pipeline
   *    read via GstParse. Using this order ensures output,
   *    similar to the GstParse input.
   *  - The sorted order might be better for pipelines created
   *    with GstEditor.
   * Thus, this should probably be a setting of GsteSerialize.
   */
  while (children) {
    GstElement *child = GST_ELEMENT_CAST (children->data);
    GstElement *next_element = NULL;

    children = g_list_previous (children);
    if (children)
      next_element = GST_ELEMENT_CAST (children->data);

    /*
     * Usually, we can ignore GstCapsFilter elements since
     * they serialized as part of the pad links.
     */
    if (!gst_is_capsfilter (GST_OBJECT (child), cb))
      /*
       * The child might need special handling and we want
       * the object_saved callback to be invoked.
       */
      gst_object_save_thyself (GST_OBJECT (child), NULL, next_element, cb);
  }

  cb->recursion_depth--;
}

static void
gst_bin_save_thyself (GstBin * bin, GstElement * next_element,
    GsteSerializeCallbacks * cb)
{
  append_space (cb);

  /*
   * For GstBin derivations, we should specify the GObject type
   * of the bin -- GstParse will use that for constructing the
   * instance.
   * This is especially useful for GstPipelines serialized
   * as bins.
   */
  if (cb->flags & GSTE_SERIALIZE_VERBOSE ||
      G_OBJECT_TYPE (bin) != GST_TYPE_BIN) {
    GstElementFactory *factory = gst_element_get_factory (GST_ELEMENT (bin));

    cb->append (GST_OBJECT_NAME (factory), cb->user_data);
    cb->append (".", cb->user_data);
  }

  cb->append ("(", cb->user_data);

  /*
   * Serialize all properties and children of the bin.
   * A space will be output after every property.
   * NOTE: We could very well supress the next space for the
   * sake of beauty. But to work around a GstParse bug, we must
   * output a space before the closing bracket, so we keep the
   * space after the opening brace for symmetry.
   */
  //cb->flags &= ~GSTE_SERIALIZE_NEED_SPACE;
  gst_object_save_properties (GST_OBJECT (bin), cb);
  gst_bin_save_children (bin, cb);

  /*
   * NOTE: The space in front of the bracket is necessary because
   * GstParse will otherwise fail to parse immediately preceding quotes.
   * This is apparently a bug in GstParse.
   */
  append_space (cb);
  cb->append (")", cb->user_data);

  /*
   * Even though GstBins have pads as well we do not serialize
   * them (here) since they are all ghost pads.
   * If the bin has more than one ghost pad, it would have to
   * be fully-qualified by pad name, but there is no way to
   * explicitly construct ghost pads in the gst-launch syntax.
   * Instead cross-bin links are serialized as links after elements.
   */
  //gst_element_save_pads (GST_ELEMENT (bin), next_element, cb);
}

/*
 * This is used for serializing pipelines by default.
 * Pipeline properties are ignored.
 * Since they are sometimes important, it is possibly to force the
 * the GstBin syntax for GstPipelines using the
 * GSTE_SERIALIZE_PIPELINES_AS_BINS flag: pipeline.( ... )
 */
static void
gst_pipeline_save_thyself (GstPipeline * pipeline, GsteSerializeCallbacks * cb)
{
  gst_bin_save_children (GST_BIN (pipeline), cb);
}

static void
gst_object_save_thyself (GstObject * object,
    GstElement * last_element, GstElement * next_element, GsteSerializeCallbacks * cb)
{
  GType type = G_OBJECT_TYPE (object);

  /*
   * The optional object_saved callback serves a similar
   * purpose than the GStreamer 0.10 "object-saved" signal
   * and allows applications to add meta-data to a container
   * format.
   * Since the only way to associate meta-data with the object
   * as represented in the serialization is via its name and
   * GstObjects may be nameless, the callback is invoked before
   * the serialization takes place.
   * Users may then call gst_object_set_name (obj, NULL) to
   * assign an unique name in the callback if necessary.
   * NOTE: It would also be possible to add one callback per
   * GstObject type.
   */
  if (cb->object_saved)
    cb->object_saved (object, cb->user_data);

  /*
   * Recursively handle the different object types.
   * NOTE: Normally GstBin derivations are not serialized with the
   * brace syntax since they are usually factory-constructable elements.
   * This can be disabled with GSTE_SERIALIZE_ALL_BINS.
   * GstCapsFilters are also serialized like ordinary elements,
   * but are ignored in GstBins by default, resulting in the special
   * filtered-link syntax to be serialized
   */
  if (GST_IS_PIPELINE (object)) {
    /*
     * Top-level pipelines are normally not serialized to produce
     * a familiar gst-launch-like output.
     * Pipelines embedded in pipelines must be serialized like GstBins
     * though since they would otherwise be lost.
     */
    if (cb->recursion_depth > 0 || cb->flags & GSTE_SERIALIZE_PIPELINES_AS_BINS)
      gst_bin_save_thyself (GST_BIN (object), next_element, cb);
    else
      gst_pipeline_save_thyself (GST_PIPELINE (object), cb);
  } else if (type == GST_TYPE_BIN ||
      ((cb->flags & GSTE_SERIALIZE_ALL_BINS) && GST_IS_BIN (object))) {
    gst_bin_save_thyself (GST_BIN (object), next_element, cb);
  } else if (GST_IS_ELEMENT (object)) {
    gst_element_save_thyself (GST_ELEMENT (object), next_element, cb);
  } else if (GST_IS_PAD (object)) {
    gst_pad_save_thyself (GST_PAD (object), last_element, next_element, cb);
  } else {
    g_warning ("Cannot serialize GstObject with type %s",
        g_type_name (type));
  }
}

void
gste_serialize_save (GstObject * object, GsteSerializeFlags flags,
    GsteSerializeAppendCallback append,
    GsteSerializeObjectSavedCallback object_saved,
    gpointer user_data)
{
  GsteSerializeCallbacks cb = {
      .append = append,
      .object_saved = object_saved,
      .user_data = user_data,
      .flags = flags,
      .recursion_depth = 0
  };

  gst_object_save_thyself (object, NULL, NULL, &cb);
}

/*
 * Test program. Compile with:
 * gcc -g -O0 -Wall -std=c99 -o gste-serialize gste-serialize.c -DGSTE_SERIALIZE_TEST \
 *     `pkg-config --cflags --libs gstreamer-1.0`
 */
#ifdef GSTE_SERIALIZE_TEST

static void
append_stdio_cb (const gchar * str, gpointer user_data)
{
  FILE *stream = user_data;
  fputs(str, stream);
}

int
main (int argc, char **argv)
{
  GError *error = NULL;
  GOptionContext *optctx;
  GsteSerializeFlags flags;
  const gchar *element_name;
  GstElement *pipeline, *element;

  g_log_set_always_fatal (G_LOG_LEVEL_ERROR |
      G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);
  g_log_set_handler (NULL,
      G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
      g_log_default_handler, NULL);

  optctx = g_option_context_new ("<launch line>");
  /* NOTE: gst_init() not required */
  g_option_context_add_group (optctx, gst_init_get_option_group ());

  if (!g_option_context_parse (optctx, &argc, &argv, &error))
    g_error ("Option parsing failed: %s", error->message);

  g_option_context_free (optctx);

  flags = atoi (g_getenv ("GSTE_SERIALIZE_FLAGS") ? : "0");
  element_name = g_getenv ("GSTE_SERIALIZE_ELEMENT");

  element = pipeline = gst_parse_launchv ((const gchar **)argv+1, &error);
  if (error)
    g_error ("Pipeline parsing error: %s", error->message);
  if (element_name)
    element = gst_bin_get_by_name (GST_BIN (pipeline), element_name);
  if (!element)
    g_error ("Element not found!");

  g_printf ("Serialized pipeline:\n\"");
  gste_serialize_save (GST_OBJECT (element), flags,
      append_stdio_cb, NULL, stdout);
  g_printf ("\"\n");

  return 0;
}

#endif
