/* drawkey.cpp
 *
 * Function for drawing the key signature
 *
 * for Denemo, a gtk+ frontend to GNU Lilypond
 * (c) 1999-2005  Matthew Hiller, Adam Tee
 */

#include "accwidths.h"
#include "drawingprims.h"
#include "utils.h"

#define SPACE_BETWEEN_ACCS 8
#define m * HALF_LINE_SPACE

/**
 * This function draws the key, if desired, and returns the width required
 * to draw it number describes the number of the new key, prevnumber the number of
 * the preceding key 
 */

gint
draw_key (cairo_t *cr, gint xx, gint y,
	  gint number, gint prevnumber, gint dclef, gboolean wetrun, keysig *keysig)
{
  /* These are just hard-coded offsets in pixels from the top of the staff.
   * mid_c_offset arrays. There's probably
   * a better way to do this, but I haven't thought of it */
  static gint treble_flat_ys[7] = { 4 m, 1 m, 5 m, 2 m, 6 m, 3 m, 7 m };
  static gint treble_sharp_ys[7] = { 0, 3 m, -1 m, 2 m, 5 m, 1 m, 4 m };
  static gint bass_flat_ys[7] = { 6 m, 3 m, 7 m, 4 m, 8 m, 5 m, 9 m };
  static gint bass_sharp_ys[7] = { 2 m, 5 m, 1 m, 4 m, 7 m, 3 m, 6 m };
  static gint alto_flat_ys[7] = { 5 m, 2 m, 6 m, 3 m, 7 m, 4 m, 8 m };
  static gint alto_sharp_ys[7] = { 1 m, 4 m, 0, 3 m, 6 m, 2 m, 5 m };
  static gint tenor_flat_ys[7] = { 2 m, 0, 4 m, 1 m, 5 m, 2 m, 6 m };
  static gint tenor_sharp_ys[7] = { -1 m, 2 m, 5 m, 1 m, 4 m, 0 m, 3 m };
  static gint soprano_flat_ys[7] = { 2 m, 6 m, 3 m, 7 m, 4 m, 8 m, 5 m };
  static gint soprano_sharp_ys[7] = { 5 m, 8 m, 4 m, 7 m, 3 m, 6 m, 2 m };

  gint *theys = 0;
  gint *theprevys = 0;
  gint i;
  gint startindex, endindex;
  gint origx = xx;
  
  gint override = 0;
  if(wetrun && keysig->directives) {
    gint count=0;
    GList *g=keysig->directives;
    for(;g;g=g->next, count++) {
      DenemoDirective* directive = g->data;
      override = override | directive->override;
      if(directive->display) { 
        drawnormaltext_cr( cr, directive->display->str, xx + directive->tx, y+count*10 );
      }
      if(directive->graphic) {
        drawbitmapinverse_cr (cr, directive->graphic,
			   xx+directive->gx+count,  y+directive->gy, FALSE);
      }
    }
  }
 if(!(DENEMO_OVERRIDE_GRAPHIC & override)) { 
  /* first, set the arrays we're using to something useful */
  if (wetrun)
    {
      switch (dclef)
	{
	case DENEMO_TREBLE_CLEF:
	case DENEMO_G_8_CLEF:
	  theprevys = (prevnumber < 0) ? treble_flat_ys : treble_sharp_ys;
	  theys = (number < 0) ? treble_flat_ys : treble_sharp_ys;
	  break;
	case DENEMO_BASS_CLEF:
	case DENEMO_F_8_CLEF:
	case DENEMO_FRENCH_CLEF:
	  theprevys = (prevnumber < 0) ? bass_flat_ys : bass_sharp_ys;
	  theys = (number < 0) ? bass_flat_ys : bass_sharp_ys;
	  break;
	case DENEMO_ALTO_CLEF:
	  theprevys = (prevnumber < 0) ? alto_flat_ys : alto_sharp_ys;
	  theys = (number < 0) ? alto_flat_ys : alto_sharp_ys;
	  break;

	case DENEMO_TENOR_CLEF:
	  theprevys = (prevnumber < 0) ? tenor_flat_ys : tenor_sharp_ys;
	  theys = (number < 0) ? tenor_flat_ys : tenor_sharp_ys;
	  break;
	case DENEMO_SOPRANO_CLEF:
	  theprevys = (prevnumber < 0) ? soprano_flat_ys : soprano_sharp_ys;
	  theys = (number < 0) ? soprano_flat_ys : soprano_sharp_ys;
	  break;
	default:
	  /* Silently default to the treble stuff. Fix me. */
	  theprevys = (prevnumber < 0) ? treble_flat_ys : treble_sharp_ys;
	  theys = (number < 0) ? treble_flat_ys : treble_sharp_ys;
	  break;
	}
    }

  /* First, check to see if we ought to draw naturals. */
  if (prevnumber < 0)
    {
      /* Draw as many accidentals as we need. */
      if (number < 0)
	startindex = -number;
      else
	startindex = 0;
      endindex = -prevnumber;
      /* Note that the loop will immediately exit if number <= prevnumber */
      for (i = startindex; i < endindex; i++, xx += NATURAL_WIDTH + 2)
	{
	  if (wetrun)
	    draw_accidental (cr, xx, y + theprevys[i], 0);
	}
    }
  else if (prevnumber > 0)
    {
      /* Analogous to above */
      if (number > 0)
	startindex = number;
      else
	startindex = 0;
      for (i = startindex; i < prevnumber; i++, xx += NATURAL_WIDTH + 2)
	{
	  if (wetrun)
	    draw_accidental (cr, xx, y + theprevys[i], 0);
	}
    }

  /* Now draw the new indicators themselves */
  if (number < 0)
    {
      number = -number;
      for (i = 0; i < number; i++, xx += FLAT_WIDTH + 2)
	if (wetrun)
	  draw_accidental (cr, xx, y + theys[i], -1);
    }
  else
    for (i = 0; i < number; i++, xx += SHARP_WIDTH + 2)
      if (wetrun)
	draw_accidental (cr, xx, y + theys[i], 1);
  }
  return xx - origx;
}
