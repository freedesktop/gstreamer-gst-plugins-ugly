/* GStreamer Mixer
 * Copyright (C) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * mixer.c: mixer design virtual class function wrappers
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mixer.h"
#include "mixer-marshal.h"

enum
{
  SIGNAL_MUTE_TOGGLED,
  SIGNAL_RECORD_TOGGLED,
  SIGNAL_VOLUME_CHANGED,
  SIGNAL_OPTION_CHANGED,
  LAST_SIGNAL
};

static void gst_mixer_class_init (GstMixerClass * klass);

static guint gst_mixer_signals[LAST_SIGNAL] = { 0 };

GType
gst_mixer_get_type (void)
{
  static GType gst_mixer_type = 0;

  if (!gst_mixer_type) {
    static const GTypeInfo gst_mixer_info = {
      sizeof (GstMixerClass),
      (GBaseInitFunc) gst_mixer_class_init,
      NULL,
      NULL,
      NULL,
      NULL,
      0,
      0,
      NULL,
    };

    gst_mixer_type = g_type_register_static (G_TYPE_INTERFACE,
        "GstMixer", &gst_mixer_info, 0);
    g_type_interface_add_prerequisite (gst_mixer_type,
        GST_TYPE_IMPLEMENTS_INTERFACE);
  }

  return gst_mixer_type;
}

static void
gst_mixer_class_init (GstMixerClass * klass)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    gst_mixer_signals[SIGNAL_RECORD_TOGGLED] =
        g_signal_new ("record-toggled",
        GST_TYPE_MIXER, G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GstMixerClass, record_toggled),
        NULL, NULL,
        gst_mixer_marshal_VOID__OBJECT_BOOLEAN, G_TYPE_NONE, 2,
        GST_TYPE_MIXER_TRACK, G_TYPE_BOOLEAN);
    gst_mixer_signals[SIGNAL_MUTE_TOGGLED] =
        g_signal_new ("mute-toggled",
        GST_TYPE_MIXER, G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GstMixerClass, mute_toggled),
        NULL, NULL,
        gst_mixer_marshal_VOID__OBJECT_BOOLEAN, G_TYPE_NONE, 2,
        GST_TYPE_MIXER_TRACK, G_TYPE_BOOLEAN);
    gst_mixer_signals[SIGNAL_VOLUME_CHANGED] =
        g_signal_new ("volume-changed",
        GST_TYPE_MIXER, G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GstMixerClass, volume_changed),
        NULL, NULL,
        gst_mixer_marshal_VOID__OBJECT_POINTER, G_TYPE_NONE, 2,
        GST_TYPE_MIXER_TRACK, G_TYPE_POINTER);
    gst_mixer_signals[SIGNAL_OPTION_CHANGED] =
        g_signal_new ("option-changed",
        GST_TYPE_MIXER, G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GstMixerClass, option_changed),
        NULL, NULL,
        gst_mixer_marshal_VOID__OBJECT_STRING, G_TYPE_NONE, 2,
        GST_TYPE_MIXER_OPTIONS, G_TYPE_STRING);

    initialized = TRUE;
  }

  klass->mixer_type = GST_MIXER_SOFTWARE;

  /* default virtual functions */
  klass->list_tracks = NULL;
  klass->set_volume = NULL;
  klass->get_volume = NULL;
  klass->set_mute = NULL;
  klass->set_record = NULL;
  klass->set_option = NULL;
  klass->get_option = NULL;
}

/**
 * gst_mixer_list_tracks:
 * @mixer: the #GstMixer (a #GstElement) to get the tracks from.
 *
 * Returns a list of available tracks for this mixer/element. Note
 * that it is allowed for sink (output) elements to only provide
 * the output tracks in this list. Likewise, for sources (inputs),
 * it is allowed to only provide input elements in this list.
 *
 * Returns: A #GList consisting of zero or more #GstMixerTracks.
 */

const GList *
gst_mixer_list_tracks (GstMixer * mixer)
{
  GstMixerClass *klass = GST_MIXER_GET_CLASS (mixer);

  if (klass->list_tracks) {
    return klass->list_tracks (mixer);
  }

  return NULL;
}

/**
 * gst_mixer_set_volume:
 * @mixer: The #GstMixer (a #GstElement) that owns the track.
 * @track: The #GstMixerTrack to set the volume on.
 * @volumes: an array of integers (of size track->num_channels)
 *           that gives the wanted volume for each channel in
 *           this track.
 *
 * Sets the volume on each channel in a track. Short note about
 * naming: a track is defined as one separate stream owned by
 * the mixer/element, such as 'Line-in' or 'Microphone'. A
 * channel is said to be a mono-stream inside this track. A
 * stereo track thus contains two channels.
 */

void
gst_mixer_set_volume (GstMixer * mixer, GstMixerTrack * track, gint * volumes)
{
  GstMixerClass *klass = GST_MIXER_GET_CLASS (mixer);

  if (klass->set_volume) {
    klass->set_volume (mixer, track, volumes);
  }
}

/*
 * gst_mixer_get_volume:
 * @mixer: the #GstMixer (a #GstElement) that owns the track
 * @track: the GstMixerTrack to get the volume from.
 * @volumes: a pre-allocated array of integers (of size
 *           track->num_channels) to store the current volume
 *           of each channel in the given track in.
 *
 * Get the current volume(s) on the given track.
 */

void
gst_mixer_get_volume (GstMixer * mixer, GstMixerTrack * track, gint * volumes)
{
  GstMixerClass *klass = GST_MIXER_GET_CLASS (mixer);

  if (klass->get_volume) {
    klass->get_volume (mixer, track, volumes);
  } else {
    gint i;

    for (i = 0; i < track->num_channels; i++) {
      volumes[i] = 0;
    }
  }
}

/**
 * gst_mixer_set_mute:
 * @mixer: the #GstMixer (a #GstElement) that owns the track.
 * @track: the #GstMixerTrack to operate on.
 * @mute: a boolean value indicating whether to turn on or off
 *        muting.
 *
 * Mutes or unmutes the given channel. To find out whether a
 * track is currently muted, use GST_MIXER_TRACK_HAS_FLAG ().
 */

void
gst_mixer_set_mute (GstMixer * mixer, GstMixerTrack * track, gboolean mute)
{
  GstMixerClass *klass = GST_MIXER_GET_CLASS (mixer);

  if (klass->set_mute) {
    klass->set_mute (mixer, track, mute);
  }
}

/**
 * gst_mixer_set_record:
 * @mixer: The #GstMixer (a #GstElement) that owns the track.
 * @track: the #GstMixerTrack to operate on.
 * @record: a boolean value that indicates whether to turn on
 *          or off recording.
 *
 * Enables or disables recording on the given track. Note that
 * this is only possible on input tracks, not on output tracks
 * (see GST_MIXER_TRACK_HAS_FLAG () and the GST_MIXER_TRACK_INPUT
 * flag).
 */

void
gst_mixer_set_record (GstMixer * mixer, GstMixerTrack * track, gboolean record)
{
  GstMixerClass *klass = GST_MIXER_GET_CLASS (mixer);

  if (klass->set_record) {
    klass->set_record (mixer, track, record);
  }
}

/**
 * gst_mixer_set_option:
 * @mixer: The #GstMixer (a #GstElement) that owns the optionlist.
 * @opts: The #GstMixerOptions that we operate on.
 * @value: The requested new option value.
 *
 * Sets a name/value option in the mixer to the requested value.
 */

void
gst_mixer_set_option (GstMixer * mixer, GstMixerOptions * opts, gchar * value)
{
  GstMixerClass *klass = GST_MIXER_GET_CLASS (mixer);

  if (klass->set_option) {
    klass->set_option (mixer, opts, value);
  }
}

/**
 * gst_mixer_get_option:
 * @mixer: The #GstMixer (a #GstElement) that owns the optionlist.
 * @opts: The #GstMixerOptions that we operate on.
 *
 * Get the current value of a name/value option in the mixer.
 *
 * Returns: current value of the name/value option.
 */

const gchar *
gst_mixer_get_option (GstMixer * mixer, GstMixerOptions * opts)
{
  GstMixerClass *klass = GST_MIXER_GET_CLASS (mixer);

  if (klass->get_option) {
    return klass->get_option (mixer, opts);
  }

  return NULL;
}

void
gst_mixer_mute_toggled (GstMixer * mixer, GstMixerTrack * track, gboolean mute)
{
  g_signal_emit (G_OBJECT (mixer),
      gst_mixer_signals[SIGNAL_MUTE_TOGGLED], 0, track, mute);

  g_signal_emit_by_name (G_OBJECT (track), "mute_toggled", mute);
}

void
gst_mixer_record_toggled (GstMixer * mixer,
    GstMixerTrack * track, gboolean record)
{
  g_signal_emit (G_OBJECT (mixer),
      gst_mixer_signals[SIGNAL_RECORD_TOGGLED], 0, track, record);

  g_signal_emit_by_name (G_OBJECT (track), "record_toggled", record);
}

void
gst_mixer_volume_changed (GstMixer * mixer,
    GstMixerTrack * track, gint * volumes)
{
  g_signal_emit (G_OBJECT (mixer),
      gst_mixer_signals[SIGNAL_VOLUME_CHANGED], 0, track, volumes);

  g_signal_emit_by_name (G_OBJECT (track), "volume_changed", volumes);
}

void
gst_mixer_option_changed (GstMixer * mixer,
    GstMixerOptions * opts, gchar * value)
{
  g_signal_emit (G_OBJECT (mixer),
      gst_mixer_signals[SIGNAL_OPTION_CHANGED], 0, opts, value);

  g_signal_emit_by_name (G_OBJECT (opts), "value_changed", value);
}