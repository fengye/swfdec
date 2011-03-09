/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2008 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include "swfdec_sound.h"
#include "swfdec_audio_decoder.h"
#include "swfdec_audio_event.h"
#include "swfdec_audio_internal.h"
#include "swfdec_bits.h"
#include "swfdec_buffer.h"
#include "swfdec_button.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"
#include "swfdec_sound_provider.h"
#include "swfdec_sprite.h"
#include "swfdec_swf_decoder.h"

/*** SWFDEC_SOUND_PROVIDER ***/

static void
swfdec_sound_sound_provider_start (SwfdecSoundProvider *provider,
    SwfdecActor *actor, gsize samples_offset, guint loops)
{
  SwfdecSound *sound = SWFDEC_SOUND (provider);
  SwfdecAudio *audio;

  audio = swfdec_audio_event_new (SWFDEC_PLAYER (swfdec_gc_object_get_context (actor)),
      sound, samples_offset, loops);
  swfdec_audio_set_actor (audio, actor);
  g_object_unref (audio);
}

typedef struct {
  SwfdecActor *	actor;
  SwfdecSound *	sound;
} RemoveData;

static gboolean
swfdec_sound_object_should_stop (SwfdecAudio *audio, gpointer datap)
{
  RemoveData *data = datap;
  SwfdecAudioEvent *event;

  if (!SWFDEC_IS_AUDIO_EVENT (audio))
    return FALSE;
  event = SWFDEC_AUDIO_EVENT (audio);
  if (event->sound != data->sound)
    return FALSE;
  return audio->actor == data->actor;
}

static void
swfdec_sound_sound_provider_stop (SwfdecSoundProvider *provider, SwfdecActor *actor)
{
  RemoveData data = { actor, SWFDEC_SOUND (provider) };

  swfdec_player_stop_sounds (SWFDEC_PLAYER (swfdec_gc_object_get_context (actor)), 
      swfdec_sound_object_should_stop, &data);
}

static SwfdecSoundMatrix *
swfdec_sound_sound_provider_get_matrix (SwfdecSoundProvider *provider)
{
  return NULL;
}

static void
swfdec_sound_sound_provider_init (SwfdecSoundProviderInterface *iface)
{
  iface->start = swfdec_sound_sound_provider_start;
  iface->stop = swfdec_sound_sound_provider_stop;
  iface->get_matrix = swfdec_sound_sound_provider_get_matrix;
}

/*** SWFDEC_SOUND ***/

G_DEFINE_TYPE_WITH_CODE (SwfdecSound, swfdec_sound, SWFDEC_TYPE_CHARACTER,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_SOUND_PROVIDER, swfdec_sound_sound_provider_init))

static void
swfdec_sound_dispose (GObject *object)
{
  SwfdecSound * sound = SWFDEC_SOUND (object);

  if (sound->decoded)
    swfdec_buffer_unref (sound->decoded);
  if (sound->encoded)
    swfdec_buffer_unref (sound->encoded);

  G_OBJECT_CLASS (swfdec_sound_parent_class)->dispose (object);
}

static void
swfdec_sound_class_init (SwfdecSoundClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_sound_dispose;
}

static void
swfdec_sound_init (SwfdecSound * sound)
{

}

int
tag_func_define_sound (SwfdecSwfDecoder * s, guint tag)
{
  SwfdecBits *b = &s->b;
  int id;
  int n_samples;
  SwfdecSound *sound;

  id = swfdec_bits_get_u16 (b);
  sound = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_SOUND);
  if (!sound)
    return SWFDEC_STATUS_OK;

  sound->codec = swfdec_bits_getbits (b, 4);
  SWFDEC_LOG ("  codec: %u", sound->codec);
  sound->format = swfdec_audio_format_parse (b);
  SWFDEC_LOG ("  format: %s", swfdec_audio_format_to_string (sound->format));
  n_samples = swfdec_bits_get_u32 (b);
  sound->n_samples = n_samples;

  switch (sound->codec) {
    case 0:
      if (swfdec_audio_format_is_16bit (sound->format))
	SWFDEC_WARNING ("undefined endianness for s16 sound");
      /* just assume LE and hope it works (FIXME: want a switch for this?) */
      sound->codec = SWFDEC_AUDIO_CODEC_UNCOMPRESSED;
      /* fall through */
    case 3:
      sound->encoded = swfdec_bits_get_buffer (&s->b, -1);
      break;
    case 2:
      sound->skip = swfdec_bits_get_u16 (b);
      sound->encoded = swfdec_bits_get_buffer (&s->b, -1);
      break;
    case 1:
    case 5:
    case 6:
      sound->encoded = swfdec_bits_get_buffer (&s->b, -1);
      break;
    default:
      SWFDEC_WARNING ("unknown codec %d", sound->codec);
  }
  sound->n_samples *= swfdec_audio_format_get_granularity (sound->format);

  swfdec_decoder_use_audio_codec (SWFDEC_DECODER (s), sound->codec, sound->format);

  return SWFDEC_STATUS_OK;
}

SwfdecBuffer *
swfdec_sound_get_decoded (SwfdecSound *sound)
{
  gpointer decoder;
  SwfdecBuffer *tmp;
  SwfdecBufferQueue *queue;
  guint sample_bytes;
  guint n_samples;
  guint depth;

  g_return_val_if_fail (SWFDEC_IS_SOUND (sound), NULL);

  if (sound->decoded) {
    return sound->decoded;
  }
  if (sound->encoded == NULL)
    return NULL;

  decoder = swfdec_audio_decoder_new (sound->codec, sound->format);
  if (decoder == NULL)
    return NULL;

  swfdec_audio_decoder_push (decoder, sound->encoded);
  swfdec_audio_decoder_push (decoder, NULL);
  queue = swfdec_buffer_queue_new ();
  while ((tmp = swfdec_audio_decoder_pull (decoder))) {
    swfdec_buffer_queue_push (queue, tmp);
  }
  g_object_unref (decoder);
  depth = swfdec_buffer_queue_get_depth (queue);
  if (depth == 0) {
    SWFDEC_ERROR ("decoding didn't produce any data, bailing");
    return NULL;
  }
  tmp = swfdec_buffer_queue_pull (queue, depth);
  swfdec_buffer_queue_unref (queue);

  sample_bytes = 4;
  n_samples = sound->n_samples;

  SWFDEC_LOG ("after decoding, got %"G_GSIZE_FORMAT" samples, should get %u and skip %u", 
      tmp->length / sample_bytes, n_samples, sound->skip);
  if (sound->skip) {
    SwfdecBuffer *tmp2 = swfdec_buffer_new_subbuffer (tmp, sound->skip * sample_bytes, 
	tmp->length - sound->skip * sample_bytes);
    swfdec_buffer_unref (tmp);
    tmp = tmp2;
  }
  if (tmp->length > n_samples * sample_bytes) {
    SwfdecBuffer *tmp2 = swfdec_buffer_new_subbuffer (tmp, 0, n_samples * sample_bytes);
    SWFDEC_DEBUG ("%u samples in %u bytes should be available, but %"G_GSIZE_FORMAT" bytes are, cutting them off",
	n_samples, n_samples * sample_bytes, tmp->length);
    swfdec_buffer_unref (tmp);
    tmp = tmp2;
  } else if (tmp->length < n_samples * sample_bytes) {
    /* we handle this case in swfdec_sound_render */
    /* FIXME: this message is important when writing new codecs, so I made it a warning.
     * It's probably not worth more than INFO for the usual case though */
    SWFDEC_WARNING ("%u samples in %u bytes should be available, but only %"G_GSIZE_FORMAT" bytes are",
	n_samples, n_samples * sample_bytes, tmp->length);
  }
  /* only assign here, the decoding code checks this variable */
  sound->decoded = tmp;

  return sound->decoded;
}

void
swfdec_sound_chunk_free (SwfdecSoundChunk *chunk)
{
  g_return_if_fail (chunk != NULL);

  g_free (chunk->envelope);
  g_free (chunk);
}

SwfdecSoundChunk *
swfdec_sound_parse_chunk (SwfdecSwfDecoder *s, SwfdecBits *b, int id)
{
  int has_envelope;
  int has_loops;
  int has_out_point;
  int has_in_point;
  guint i, j;
  SwfdecSound *sound;
  SwfdecSoundChunk *chunk;

  sound = swfdec_swf_decoder_get_character (s, id);
  if (!SWFDEC_IS_SOUND (sound)) {
    SWFDEC_ERROR ("given id %d does not reference a sound object", id);
    return NULL;
  }

  chunk = g_new0 (SwfdecSoundChunk, 1);
  chunk->sound = sound;
  SWFDEC_DEBUG ("parsing sound chunk for sound %d", SWFDEC_CHARACTER (sound)->id);

  swfdec_bits_getbits (b, 2);
  chunk->stop = swfdec_bits_getbits (b, 1);
  chunk->no_restart = swfdec_bits_getbits (b, 1);
  has_envelope = swfdec_bits_getbits (b, 1);
  has_loops = swfdec_bits_getbits (b, 1);
  has_out_point = swfdec_bits_getbits (b, 1);
  has_in_point = swfdec_bits_getbits (b, 1);
  if (has_in_point) {
    chunk->start_sample = swfdec_bits_get_u32 (b);
    SWFDEC_LOG ("  start_sample = %u", chunk->start_sample);
  } else {
    chunk->start_sample = 0;
  }
  if (has_out_point) {
    chunk->stop_sample = swfdec_bits_get_u32 (b);
    if (chunk->stop_sample == 0) {
      SWFDEC_FIXME ("stop sample == 0???");
    }
    SWFDEC_LOG ("  stop_sample = %u", chunk->stop_sample);
    if (chunk->stop_sample <= chunk->start_sample) {
      SWFDEC_ERROR ("stopping before starting? (start sample %u, stop sample %u)",
	  chunk->start_sample, chunk->stop_sample);
      chunk->stop_sample = 0;
    }
  } else {
    chunk->stop_sample = 0;
  }
  if (has_loops) {
    chunk->loop_count = swfdec_bits_get_u16 (b);
    if (chunk->loop_count == 0) {
      SWFDEC_ERROR ("loop_count 0 not allowed, setting to 1");
      chunk->loop_count = 1;
    }
    SWFDEC_LOG ("  loop_count = %u", chunk->loop_count);
  } else {
    chunk->loop_count = 1;
  }
  if (has_envelope) {
    chunk->n_envelopes = swfdec_bits_get_u8 (b);
    chunk->envelope = g_new0 (SwfdecSoundEnvelope, chunk->n_envelopes);
    SWFDEC_LOG ("  n_envelopes = %u", chunk->n_envelopes);
  }

  for (i = 0; i < chunk->n_envelopes && swfdec_bits_left (b); i++) {
    chunk->envelope[i].offset = swfdec_bits_get_u32 (b);
    if (i > 0 && chunk->envelope[i-1].offset > chunk->envelope[i].offset) {
      SWFDEC_ERROR ("unordered sound envelopes");
      chunk->envelope[i].offset = chunk->envelope[i-1].offset;
    }

    for (j = 0; j < 2; j++) {
      chunk->envelope[i].volume[j] = swfdec_bits_get_u16 (b);
      if (chunk->envelope[i].volume[j] > 32768) {
	SWFDEC_FIXME ("too big envelope volumes (%u > 32768) not handled correctly",
	    chunk->envelope[i].volume[j]);
	chunk->envelope[i].volume[j] = 32768;
      }
    }

    SWFDEC_LOG ("    envelope = %u { %u, %u }", chunk->envelope[i].offset,
	(guint) chunk->envelope[i].volume[0], (guint) chunk->envelope[i].volume[1]);
  }

  if (i < chunk->n_envelopes)
    SWFDEC_ERROR ("out of bits when reading sound envelopes");

  return chunk;
}

int
tag_func_define_button_sound (SwfdecSwfDecoder * s, guint tag)
{
  guint i;
  guint id;
  SwfdecButton *button;

  id = swfdec_bits_get_u16 (&s->b);
  button = (SwfdecButton *) swfdec_swf_decoder_get_character (s, id);
  if (!SWFDEC_IS_BUTTON (button)) {
    SWFDEC_ERROR ("id %u is not a button", id);
    return SWFDEC_STATUS_OK;
  }
  SWFDEC_LOG ("loading sound events for button %u", id);
  for (i = 0; i < 4; i++) {
    id = swfdec_bits_get_u16 (&s->b);
    if (id) {
      SWFDEC_LOG ("loading sound %u for button event %u", id, i);
      if (button->sounds[i]) {
	SWFDEC_ERROR ("need to delete previous sound for button %u's event %u", 
	    SWFDEC_CHARACTER (button)->id, i);
	swfdec_sound_chunk_free (button->sounds[i]);
      }
      button->sounds[i] = swfdec_sound_parse_chunk (s, &s->b, id);
    }
  }

  return SWFDEC_STATUS_OK;
}

/**
 * swfdec_sound_buffer_get_n_samples:
 * @buffer: data to examine
 * @format: format the data in @buffer is in
 *
 * Determines the number of samples inside @buffer that would be available if
 * it were to be rendered using the default Flash format, 44100Hz.
 *
 * Returns: Number of samples contained in @buffer when rendered
 **/
guint
swfdec_sound_buffer_get_n_samples (const SwfdecBuffer *buffer, SwfdecAudioFormat format)
{
  g_return_val_if_fail (buffer != NULL, 0);
  g_return_val_if_fail (buffer->length % (2 * swfdec_audio_format_get_channels (format)) == 0, 0);

  return buffer->length / (2 * swfdec_audio_format_get_channels (format)) *
    swfdec_audio_format_get_granularity (format);
}

/**
 * swfdec_sound_render_buffer:
 * @dest: target buffer to render to
 * @source: source data to render
 * @offset: offset in 44100Hz samples into @source
 * @n_samples: number of samples to render into @dest. If more data would be
 *	       rendered than is available in @source, 0 samples are used instead.
 *
 * Adds data from @source into @dest
 **/
void
swfdec_sound_buffer_render (gint16 *dest, const SwfdecBuffer *source, 
    guint offset, guint n_samples)
{
  g_return_if_fail (dest != NULL);
  g_return_if_fail (source != NULL);
  g_return_if_fail ((offset + n_samples) * 4 <= source->length);

  memcpy (dest, source->data + 4 * offset, 4 * n_samples);
}

