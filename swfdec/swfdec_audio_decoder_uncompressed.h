/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_AUDIO_DECODER_UNCOMPRESSED_H_
#define _SWFDEC_AUDIO_DECODER_UNCOMPRESSED_H_

#include <swfdec/swfdec_audio_decoder.h>

G_BEGIN_DECLS


typedef struct _SwfdecAudioDecoderUncompressed SwfdecAudioDecoderUncompressed;
typedef struct _SwfdecAudioDecoderUncompressedClass SwfdecAudioDecoderUncompressedClass;

#define SWFDEC_TYPE_AUDIO_DECODER_UNCOMPRESSED                    (swfdec_audio_decoder_uncompressed_get_type())
#define SWFDEC_IS_AUDIO_DECODER_UNCOMPRESSED(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AUDIO_DECODER_UNCOMPRESSED))
#define SWFDEC_IS_AUDIO_DECODER_UNCOMPRESSED_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AUDIO_DECODER_UNCOMPRESSED))
#define SWFDEC_AUDIO_DECODER_UNCOMPRESSED(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AUDIO_DECODER_UNCOMPRESSED, SwfdecAudioDecoderUncompressed))
#define SWFDEC_AUDIO_DECODER_UNCOMPRESSED_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AUDIO_DECODER_UNCOMPRESSED, SwfdecAudioDecoderUncompressedClass))
#define SWFDEC_AUDIO_DECODER_UNCOMPRESSED_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AUDIO_DECODER_UNCOMPRESSED, SwfdecAudioDecoderUncompressedClass))

struct _SwfdecAudioDecoderUncompressed
{
  SwfdecAudioDecoder		decoder;

  SwfdecBufferQueue *		queue;		/* queue collecting output buffers */
};

struct _SwfdecAudioDecoderUncompressedClass
{
  SwfdecAudioDecoderClass	decoder_class;
};

GType			swfdec_audio_decoder_uncompressed_get_type	(void);


G_END_DECLS
#endif
