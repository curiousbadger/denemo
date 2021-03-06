/*
 * importmusicxml.c
 *
 * Functions for importing a MusicXML file
 *
 * for Denemo, a gtk+ frontend to GNU Lilypond
 * (C)  2010 Richard Shann
 *
 * License: this file may be used under the FSF GPL version 3 or later
 */
#include <string.h>
#include <denemo/denemo.h>
#include "core/prefops.h"            //for get_user_data_dir()
#include "export/file.h"
#include "core/utils.h"
#include "core/view.h"

/* libxml includes: for libxml2 this should be <libxml.h> */
#include <libxml/parser.h>
#include <libxml/tree.h>

//A bad array access accessing before the array start is causing a crash sporadically (perhaps a bad xml file?) this pads the memory allocated as work-around
//#define g_malloc0(a) (g_malloc0(2*(a)) + (a))

gint InitialVoiceNum = 0;

GString *Warnings;

/* Defines for making traversing XML trees easier */

#define FOREACH_CHILD_ELEM(childElem, parentElem) \
for ((childElem) = (parentElem)->xmlChildrenNode; \
     (childElem) != NULL; \
     (childElem) = (childElem)->next)

#define ELEM_NAME_EQ(childElem, childElemName) \
(strcmp ((gchar *)(childElem)->name, (childElemName)) == 0)

#define ILLEGAL_ELEM(parentElemName, childElem) \
do \
  { \
    g_warning ("Illegal element inside <%s>: <%s>", parentElemName, \
               (childElem)->name); \
  } while (0)

#define RETURN_IF_ELEM_NOT_FOUND(parentElemName, childElem, childElemName) \
do \
  { \
    if (childElem == NULL) \
      { \
        g_warning ("Element <%s> not found inside <%s>", childElemName, \
                   parentElemName); \
        return -1; \
      } \
  } while (0)

/**
 * Get the text from the child node list of elem, convert it to an integer,
 * and return it.  If unsuccessful, return G_MAXINT.
 */
static gint
getXMLIntChild (xmlNodePtr elem)
{
  gchar *text = (gchar *) xmlNodeListGetString (elem->doc, elem->xmlChildrenNode, 1);
  gint num = G_MAXINT;
  if (text == NULL)
    {
      g_warning ("No child text found %s", elem->name);
    }
  else
    {
      if (sscanf (text, " %d", &num) != 1)
        {
          g_warning ("Could not convert child text \"%s\" of <%s> to number", text, elem->name);
          num = G_MAXINT;
        }
      g_free (text);
    }
  return num;
}





#define INSERT_REST(num, den, rest) \
if(duration >= (num*divisions)/den)\
  {\
    g_string_append (script, "(d-InsertRest" rest ")(d-SetNonprinting)");\
    return insert_invisible_rest (script, duration - (num*divisions)/den, divisions);\
  } else

static gint
insert_invisible_rest (GString * script, gint duration, gint divisions)
{
  //g_assert (divisions);
  if (duration == 0)
    return TRUE;
  //g_debug("invis rest  %d, %d\n",  duration, divisions);
  INSERT_REST (4, 1, "0") INSERT_REST (2, 1, "1") INSERT_REST (1, 1, "2") INSERT_REST (1, 2, "3") INSERT_REST (1, 4, "4") INSERT_REST (1, 8, "5") INSERT_REST (1, 16, "6") INSERT_REST (1, 32, "7") INSERT_REST (1, 64, "8") g_warning ("Cannot cope with rest of %d/%d quarter notes", duration, divisions);
  return FALSE;
}

#undef INSERT_REST



static void
parse_time (GString ** scripts, gint numvoices, gint measurenum, xmlNodePtr rootElem)
{
  xmlNodePtr childElem;
  gint numerator = 0, denominator = 0;
  gint i;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {

    if (ELEM_NAME_EQ (childElem, "beats"))
      numerator = getXMLIntChild (childElem);
    if (ELEM_NAME_EQ (childElem, "beat-type"))
      denominator = getXMLIntChild (childElem);
  }
  if (numerator && denominator)
    for (i = 0; i < numvoices; i++)
      if (measurenum == 1)
        g_string_append_printf (scripts[i + 1], "(d-InitialTimeSig \"%d/%d\")", numerator, denominator);
      else
        g_string_append_printf (scripts[i + 1], "(d-InsertTimeSig \"%d/%d\")(if (not (Appending?))(d-MoveCursorRight))", numerator, denominator);
}

const gchar *
get_clef (gint line, gchar * sign)
{
  switch (line)
    {
    case 1:
      if (*sign == 'G')
        return "French";
    case 2:
      if (*sign == 'G')
        return "Treble";
    case 3:
      if (*sign == 'C')
        return "Alto";
    case 4:
      if (*sign == 'F')
        return "Bass";
      if (*sign == 'C')
        return "Tenor";
    default:
      return "Treble";
    }

}

static void
parse_key (GString ** scripts, gint numvoices, gint measurenum, xmlNodePtr rootElem)
{
  xmlNodePtr childElem;
  gint fifths = 0;
  gchar *mode = NULL;
  gint i;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {

    if (ELEM_NAME_EQ (childElem, "fifths"))
      fifths = getXMLIntChild (childElem);
    if (ELEM_NAME_EQ (childElem, "mode"))
      mode = xmlNodeListGetString (childElem->doc, childElem->xmlChildrenNode, 1);
  }
  if (mode)
    for (i = 0; i < numvoices; i++)
      if (measurenum == 1)
        g_string_append_printf (scripts[i + 1], "(d-InitialKey \"C major\")(d-IncrementKeysig %d)", fifths);
      else
        g_string_append_printf (scripts[i + 1], "(d-InsertKey \"C major\")(d-IncrementKeysig %d)", fifths);
 g_free (mode);
}

static void
parse_clef (GString ** scripts, gint division, gint * voice_timings, gint voicenum, gint numvoices, gint * staff_for_voice, gint divisions, gint measurenum, xmlNodePtr rootElem)
{
  xmlNodePtr childElem;
  gint line = 0;
  gchar *sign = NULL;
  gchar *number = xmlGetProp (rootElem, (xmlChar *) "number");
  gint staffnum = 0;
  if (number)
    staffnum = atoi (number);
  if (staffnum == 0)
    staffnum = 1;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {                             //g_debug("clef change %s \n", childElem->name);
    if (ELEM_NAME_EQ (childElem, "line"))
      line = getXMLIntChild (childElem);
    if (ELEM_NAME_EQ (childElem, "sign"))
      sign = xmlNodeListGetString (childElem->doc, childElem->xmlChildrenNode, 1);
  }                             //g_assert(voicenum>0);
  if (division > voice_timings[voicenum - 1])
    {
      insert_invisible_rest (scripts[voicenum], division - voice_timings[voicenum - 1], divisions);
      voice_timings[voicenum - 1] = division;
    }
  if (sign)
    {
      gint i;
      const gchar *clef = get_clef (line, sign);
      for (i = 0; i < numvoices; i++)
        {
          if (staff_for_voice[i] == staffnum)
            if (measurenum == 1)
              g_string_append_printf (scripts[i + 1], "(d-InitialClef \"%s\")", clef);
            else
              g_string_append_printf (scripts[i + 1], "(d-InsertClef \"%s\")", clef);
        }
    }
    g_free (sign);
}

static const gchar *
alteration (gint alter)
{
  switch (alter)
    {
    case -2:
      return "eses";
    case -1:
      return "es";
    case 1:
      return "is";
    case 2:
      return "isis";
    default:
      return "";
    }
}

static const gchar *
octave_string (gint octave)
{

  gchar *octavation;
  switch (octave)
    {
    case 0:
      octavation = ",,,";
      break;
    case 1:
      octavation = ",,";
      break;
    case 2:
      octavation = ",";
      break;
    case 3:
      octavation = "";
      break;
    case 4:
      octavation = "'";
      break;
    case 5:
      octavation = "''";
      break;

    case 6:
      octavation = "'''";
      break;

    case 7:
      octavation = "''''";
      break;

    case 8:
      octavation = "'''''";
      break;

    case 9:
      octavation = "''''''";
      break;

    default:
      octavation = "%{duration not implemented%}";
      break;
    }
  return octavation;
}

static gchar *
insert_note (gchar * type, gint octave, gchar * step, gint alter)
{
  if (step == NULL)
    {
      g_warning ("Note without step");
      return g_strdup ("");
    }
  gchar *duration_text = "";
  if (!strcmp (type, "whole"))
    duration_text = "\n(d-Set0)";
  else if (!strcmp (type, "half"))
    duration_text = "\n(d-Set1)";
  else if (!strcmp (type, "quarter"))
    duration_text = "\n(d-Set2)";
  else if (!strcmp (type, "eighth"))
    duration_text = "\n(d-Set3)";
  else if (!strcmp (type, "16th"))
    duration_text = "\n(d-Set4)";
  else if (!strcmp (type, "32nd"))
    duration_text = "\n(d-Set5)";
  else if (!strcmp (type, "64th"))
    duration_text = "\n(d-Set6)";
  else if (!strcmp (type, "128th"))
    duration_text = "\n(d-Set7)";
  else if (!strcmp (type, "256th"))
    duration_text = "\n(d-Set8)";
  else if (!strcmp (type, "breve"))
    duration_text = "\n(d-SetBreve)";
  else if (!strcmp (type, "longa"))
    duration_text = "\n(d-SetLonga)";
  else
    g_warning ("Note duration %s not implemented", type);
  const gchar *octavation = octave_string (octave);
  gchar *put_text = g_strdup_printf ("(d-InsertC)(d-PutNoteName \"%c%s%s\")", g_ascii_tolower (*step), alteration (alter), octavation);
  GString *ret = g_string_new (duration_text);
  g_string_append (ret, put_text);

  return g_string_free (ret, FALSE);
}

static gchar *
add_note (gint octave, gchar * step, gint alter)
{                               // d-InsertNoteInChord lily (d-ShiftCursor is relative (d-MoveTo ????
  const gchar *octavation = octave_string (octave);
  gchar *text = g_strdup_printf ("(d-InsertNoteInChord \"%c%s%s\")", g_ascii_tolower (*step), alteration (alter), octavation);
  GString *ret = g_string_new (text);
  g_free (text);
  return g_string_free (ret, FALSE);
}

static void
get_numstaffs_from_note (xmlNodePtr rootElem, gint * maxstaffs, gint * maxvoices)
{
  xmlNodePtr childElem;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    if (ELEM_NAME_EQ (childElem, "staff"))
      {
        gint staffnum = getXMLIntChild (childElem);
        if (staffnum > *maxstaffs)
          *maxstaffs = staffnum;        //g_debug("staff num %d ...", staffnum);
      }
    if (ELEM_NAME_EQ (childElem, "voice"))
      {
        gint voicenum = getXMLIntChild (childElem);
        if (voicenum > *maxvoices)
          *maxvoices = voicenum;
      }
  }
  //g_debug("So far %d %d\t", *maxstaffs, *maxvoices);
}

static void
get_numstaffs_in_measure (xmlNodePtr rootElem, gint * maxstaffs, gint * maxvoices)
{
  xmlNodePtr childElem;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    if (ELEM_NAME_EQ (childElem, "note"))
      {
        get_numstaffs_from_note (childElem, maxstaffs, maxvoices);
      }
    if (ELEM_NAME_EQ (childElem, "backup"))
      {                         //should someone do backup without specifying a voice for it.
        if (*maxvoices == 1)
          *maxvoices = (*maxvoices) + 1;
      }

  }

}


static gint
parseDuration (gint * current_voice, xmlNodePtr rootElem)
{
  xmlNodePtr childElem;
  gint duration = 0;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    if (ELEM_NAME_EQ (childElem, "duration"))
      duration = getXMLIntChild (childElem);

    if (ELEM_NAME_EQ (childElem, "voice"))
      *current_voice = getXMLIntChild (childElem);
  }
  return duration;
}

static void
get_rest_for_duration (GString * ret, gint duration, gint divisions)
{
  //g_debug("Rest duration %d, divisions %d\n", duration, divisions);
  if (duration >= 4 * divisions)
    {
      g_string_append (ret, "(d-InsertRest0)");
      duration -= 4 * divisions;
      return get_rest_for_duration (ret, duration, divisions);
    }
  else if (duration >= 2 * divisions)
    {
      g_string_append (ret, "(d-InsertRest1)");
      duration -= 2 * divisions;
      return get_rest_for_duration (ret, duration, divisions);
    }
  else if (duration >= 1 * divisions)
    {
      g_string_append (ret, "(d-InsertRest2)");
      duration -= 1 * divisions;
      return get_rest_for_duration (ret, duration, divisions);
    }
  else if (2 * duration >= divisions && (divisions / 2))
    {
      g_string_append (ret, "(d-InsertRest3)");
      duration -= divisions / 2;
      return get_rest_for_duration (ret, duration, divisions);
    }
  else if (4 * duration >= divisions && (divisions / 4))
    {
      g_string_append (ret, "(d-InsertRest4)");
      duration -= divisions / 4;
      return get_rest_for_duration (ret, duration, divisions);
    }
  else if (8 * duration >= divisions && (divisions / 8))
    {
      g_string_append (ret, "(d-InsertRest5)");
      duration -= divisions / 8;
      return get_rest_for_duration (ret, duration, divisions);
    }
  else if (16 * duration >= divisions && (divisions / 16))
    {
      g_string_append (ret, "(d-InsertRest6)");
      duration -= divisions / 16;
      return get_rest_for_duration (ret, duration, divisions);
    }
  else if (32 * duration >= divisions && (divisions / 32))
    {
      g_string_append (ret, "(d-InsertRest7)");
      duration -= divisions / 32;
      return get_rest_for_duration (ret, duration, divisions);
    }
  else if (duration == 0)
    return;

  g_string_append (ret, "\n;Duration of rest not recognized\n");
}


static gchar *
add_rest (gchar * type, gint duration, gint divisions)
{
  gchar *duration_text = "";
  if (!strcmp (type, "whole"))
    {
      if (4 * divisions == duration)
        duration_text = "(d-InsertRest0)";
      else
        {
          GString *ret = g_string_new ("");
          get_rest_for_duration (ret, duration, divisions);
          return g_string_free (ret, FALSE);
        }
    }
  else if (!strcmp (type, "half"))
    duration_text = "(d-InsertRest1)";
  else if (!strcmp (type, "quarter"))
    duration_text = "(d-InsertRest2)";
  else if (!strcmp (type, "eighth"))
    duration_text = "(d-InsertRest3)";
  else if (!strcmp (type, "16th"))
    duration_text = "(d-InsertRest4)";
  else if (!strcmp (type, "32nd"))
    duration_text = "(d-InsertRest5)";
  else if (!strcmp (type, "64th"))
    duration_text = "(d-InsertRest6)";
  else if (!strcmp (type, "128th"))
    duration_text = "(d-InsertRest7)";
  else if (!strcmp (type, "256th"))
    duration_text = "(d-InsertRest8)";
  else if (!strcmp (type, "breve"))
    duration_text = "(d-InsertBreveRest)";
  else if (!strcmp (type, "longa"))
    duration_text = "(d-InsertLongaRest)";
  else
    g_warning ("Restduration %s not implemented", type);
  return g_strdup (duration_text);
}

static void
modify_time (xmlNodePtr rootElem, gint * actual_notes, gint * normal_notes)
{
  xmlNodePtr childElem;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    if (ELEM_NAME_EQ (childElem, "actual-notes"))
      *actual_notes = getXMLIntChild (childElem);
    if (ELEM_NAME_EQ (childElem, "normal-notes"))
      *normal_notes = getXMLIntChild (childElem);
  }
}




static void
parse_ornaments (GString * notations, xmlNodePtr rootElem)
{
  xmlNodePtr childElem;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    if (ELEM_NAME_EQ (childElem, "trill-mark"))
      {
        g_string_append (notations, "(d-ToggleTrill)");
      }
    if (ELEM_NAME_EQ (childElem, "turn"))
      {
        g_string_append (notations, "(d-ToggleTurn)");
      }
  }
}

static void
parse_articulations (GString * notations, xmlNodePtr rootElem)
{
  xmlNodePtr childElem;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    if (ELEM_NAME_EQ (childElem, "staccato"))
      g_string_append (notations, "(d-ToggleStaccato)");
    if (ELEM_NAME_EQ (childElem, "staccatissimo"))
      g_string_append (notations, "(d-ToggleStaccatissimo)");
  }
}

static void
parse_notations (GString * notations, xmlNodePtr rootElem)
{
  xmlNodePtr childElem;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    if (ELEM_NAME_EQ (childElem, "articulations"))
      {
        parse_articulations (notations, childElem);
      }
    if (ELEM_NAME_EQ (childElem, "slur"))
      {
        gchar *type = xmlGetProp (childElem, (xmlChar *) "type");

        if (type && (!strcmp (type, "start")))
          g_string_append (notations, "(d-ToggleBeginSlur)");
        if (type && (!strcmp (type, "stop")))
          g_string_append (notations, "(d-ToggleEndSlur)");
      }

    if (ELEM_NAME_EQ (childElem, "fermata"))
      {
        g_string_append (notations, "(d-ToggleFermata)");
      }
    //  I think we need functions to apply that aren't toggles.
//note tuplets will be ignored, we will depend on the timing changes (as at present), since tuplet start/end is a separate object which once inserted prevents us seeing the note/chord

    if (ELEM_NAME_EQ (childElem, "ornaments"))
      parse_ornaments (notations, childElem);
  }
}


// *division is the current position of the tick counter from the start of the measure
static gchar *
parse_note (xmlNodePtr rootElem, GString ** scripts, gint * staff_for_voice, gint * division, gint divisions, gint * voice_timings, gint * current_voice, gint * actual_notes, gint * normal_notes, gboolean is_nonprinting)
{
  GString *ret = g_string_new ("");
  xmlNodePtr childElem;
  gint octave, alter = 0;
  gchar *step = NULL;
  gchar *type = NULL;
  gboolean in_chord = FALSE, is_dotted = FALSE, is_double_dotted = FALSE, is_rest = FALSE, is_whole_measure_rest = FALSE, is_grace = FALSE, is_tied = FALSE;
  GString *notations = g_string_new ("");
  gint voicenum = 1, staffnum = 1;
  gint duration = 0;
  gint initial_actual_notes = *actual_notes;
  gint initial_normal_notes = *normal_notes;
  gboolean timing_set = FALSE;  //for case where one voice ends during a tuplet and the next one starts during a tuplet
  GString *text = g_string_new ("");
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    if (ELEM_NAME_EQ (childElem, "pitch"))
      {
        xmlNodePtr grandchildElem;
        FOREACH_CHILD_ELEM (grandchildElem, childElem)
        {
          if (ELEM_NAME_EQ (grandchildElem, "step"))
            step = xmlNodeListGetString (grandchildElem->doc, grandchildElem->xmlChildrenNode, 1);
          if (ELEM_NAME_EQ (grandchildElem, "octave"))
            octave = getXMLIntChild (grandchildElem);
          if (ELEM_NAME_EQ (grandchildElem, "alter"))
            alter = getXMLIntChild (grandchildElem);
        }
      }
    if (ELEM_NAME_EQ (childElem, "chord"))
      {
        in_chord = TRUE;
      }
    if (ELEM_NAME_EQ (childElem, "grace"))
      {
        is_grace = TRUE;
      }
    if (ELEM_NAME_EQ (childElem, "rest"))
      {
        is_rest = TRUE;
        gchar *whole  = xmlGetProp (childElem, (xmlChar *) "measure");
        is_whole_measure_rest = !g_strcmp0 (whole , "yes");
      }
    if (ELEM_NAME_EQ (childElem, "dot"))
      {
        if (is_dotted)
          is_double_dotted = TRUE;
        is_dotted = TRUE;
      }

    if (ELEM_NAME_EQ (childElem, "tie"))
      {
        gchar *start = xmlGetProp (childElem, (xmlChar *) "type");
        if (start && !strcmp ("start", start))
          is_tied = TRUE;
      }

    if (ELEM_NAME_EQ (childElem, "type"))
      type = xmlNodeListGetString (childElem->doc, childElem->xmlChildrenNode, 1);
    if (ELEM_NAME_EQ (childElem, "duration"))
      {
        duration = getXMLIntChild (childElem);

      }
    if (ELEM_NAME_EQ (childElem, "voice"))
      voicenum = getXMLIntChild (childElem);
    if (ELEM_NAME_EQ (childElem, "staff"))
      staffnum = getXMLIntChild (childElem);
/*
               <notations>
                <tuplet number="1" type="stop"/>
                </notations>
*/
    if (ELEM_NAME_EQ (childElem, "notations"))
      {
        parse_notations (notations, childElem);
      }

    if (ELEM_NAME_EQ (childElem, "time-modification"))
      {
        timing_set = TRUE;
        modify_time (childElem, actual_notes, normal_notes);
      }
  }
  if (voicenum < 1)
    {
      g_warning ("Bad MusicXML file voice 0 encountered");
      voicenum = 1;
    }
  if (staffnum < 1)
    {
      g_warning ("Bad MusicXML file staff 0 encountered");
      staffnum = 1;
    }
  if (staff_for_voice[voicenum - 1] == 0)
    staff_for_voice[voicenum - 1] = staffnum;

#ifdef FIXED_PROBLEM_WITH_VOICE_CHANGE_IN_CHORDS
// PROBLEM - this inserts an object which will come before inserting notes in a chord...
// Do we need a command to add a note to a chord which works in this position???? otherwise it's like tuplets.
  if (!in_chord && (staff_for_voice[voicenum - 1] != staffnum))
    {
      g_string_append_printf (scripts[voicenum], "(d-ChangeStaff \"voice %d\")(d-MoveCursorRight)", staffnum + InitialVoiceNum);        //always at end of bar !!!!!!!!!!! voice 1 staff 1 in debmand example.
      staff_for_voice[voicenum - 1] = staffnum;
      // g_warning("Voice %d in staff %d + %d need a staff change directive", voicenum, staffnum, InitialVoiceNum);
    }
#else
  if (!in_chord && (staff_for_voice[voicenum - 1] != staffnum))
    {
      g_string_append (ret, "Change Staff Omitted ");
    }
#endif


  if (*division > voice_timings[voicenum - 1])
    {
      insert_invisible_rest (scripts[voicenum], *division - voice_timings[voicenum - 1], divisions);
      voice_timings[voicenum - 1] = *division;
    }

  if (type)
    {

            g_string_append (text, in_chord ? add_note (octave, step, alter) : (is_rest ? add_rest (type, duration, divisions) : insert_note (type, octave, step, alter)));

      if (is_nonprinting)
        g_string_append (text, "(d-SetNonprinting)");
      if (!(in_chord || is_grace))
        voice_timings[voicenum - 1] += duration;
      if (is_tied && !in_chord)
        g_string_append (text, "(d-ToggleTie)");
      if (is_grace)
        g_string_append (text, "(d-ToggleGrace)");
      if ((!in_chord) && is_dotted)
        g_string_append (text, "(d-AddDot)");
      if (is_double_dotted)
        g_string_append (text, "(d-AddDot)");


    }
  else if (is_rest)
    {                           //for the case where a rest is given without a type, just a duration.
     if(is_whole_measure_rest)
            g_string_append (text, "(d-InsertWholeMeasureRest)(d-MoveCursorLeft)");
     else
            get_rest_for_duration (text, duration, divisions);

    voice_timings[voicenum - 1] += duration;

    }


  if (!timing_set)
    {
      *actual_notes = 1;
      *normal_notes = 1;
    }

  if (((*current_voice != voicenum) && !(((initial_actual_notes) == 1) && (initial_normal_notes == 1))))
    {                           /* an unterminated tuplet in the last voice *///g_assert(*current_voice>0);
      g_string_append (scripts[*current_voice], "\n;Voice terminated during a tuplet\n(d-EndTuplet)");

      initial_actual_notes = 1;
      initial_normal_notes = 1;
    }

  if (((initial_actual_notes != *actual_notes) || (initial_normal_notes != *normal_notes)))
    {
      gchar *str;
      if ((initial_actual_notes) == 1 && (initial_normal_notes == 1))
        str = g_strdup_printf ("(d-StartTuplet \"%d/%d\")", *normal_notes, *actual_notes);
      //str = g_strdup_printf("\n;not end tuplet and entered with normal timings %d  \n(d-StartTuplet \"%d/%d\")", in_chord, *normal_notes, *actual_notes);
      else
        {
        if (*normal_notes==1 && *actual_notes==1)
            str = g_strdup_printf ("\n;Leaving tuplet timing\n(d-EndTuplet)");
        else
            str = g_strdup_printf ("\n;Changed timings\n(d-EndTuplet)(d-StartTuplet \"%d/%d\")", *normal_notes, *actual_notes);
        }
      g_string_append (scripts[voicenum], str);
      g_free (str);
    }



  g_string_append (scripts[voicenum], text->str);
  g_string_append (scripts[voicenum], notations->str);
  g_string_free (notations, TRUE);
  g_string_free (text, TRUE);

  if (!(in_chord || is_grace))
    *division = *division + duration;
  *current_voice = voicenum;
  g_free (step);
  g_free (type);
  return g_string_free (ret, FALSE);
}

static void
get_staff_for_voice_note (xmlNodePtr rootElem, gint * staff_for_voice)
{
  xmlNodePtr childElem;
  gint voicenum = 1, staffnum = 1;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    if (ELEM_NAME_EQ (childElem, "voice"))
      voicenum = getXMLIntChild (childElem);
    if (ELEM_NAME_EQ (childElem, "staff"))
      staffnum = getXMLIntChild (childElem);
  }
  if (voicenum < 1)
    {
      g_warning ("Bad MusicXML file voice 0 encountered");
      voicenum = 1;
    }
  if (staffnum < 1)
    {
      g_warning ("Bad MusicXML file staff 0 encountered");
      staffnum = 1;
    }
  if (staff_for_voice[voicenum - 1] == 0)
    staff_for_voice[voicenum - 1] = staffnum;
}

static void
parse_attributes (xmlNodePtr rootElem, GString ** scripts, gint numvoices, gint * staff_for_voice, gint division, gint * voice_timings, gint * divisions, gint * current_voice, gint measurenum)
{
  xmlNodePtr childElem;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {                             //g_debug("attribute %s at division %d\n", childElem->name, division);
    if (ELEM_NAME_EQ (childElem, "time"))
      parse_time (scripts, numvoices, measurenum, childElem);
    if (ELEM_NAME_EQ (childElem, "key"))
      parse_key (scripts, numvoices, measurenum, childElem);
    if (ELEM_NAME_EQ (childElem, "clef"))
      parse_clef (scripts, division, voice_timings, *current_voice, numvoices, staff_for_voice, *divisions, measurenum, childElem);
    if (ELEM_NAME_EQ (childElem, "divisions"))
      *divisions = getXMLIntChild (childElem);
  }

}



static void
parse_barline (xmlNodePtr rootElem, GString ** scripts, gint numvoices)
{
  xmlNodePtr childElem;
  gchar *text = NULL;
  gchar *style = NULL, *repeat = NULL;
  gint i;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {                             //g_debug("attribute %s at division %d\n", childElem->name, division);
    if (ELEM_NAME_EQ (childElem, "bar-style"))
      style = xmlNodeListGetString (childElem->doc, childElem->xmlChildrenNode, 1);
    if (ELEM_NAME_EQ (childElem, "repeat"))
      repeat = xmlGetProp (childElem, "direction");
  }
  if (repeat)
    {
      if ((!strcmp (repeat, "backward")))
        text = "(d-RepeatEnd)";
      else if ((!strcmp (repeat, "forward")))
        text = "(d-RepeatStart)";
      else if ((!strcmp (repeat, "forward-backward")))
        text = "(d-RepeatEndStart)";
    }
  else if (style)
    {
      if ((!strcmp (style, "light-light")))
        text = "(d-DoubleBarline)";
      else if ((!strcmp (style, "light-heavy")))
        text = "(d-ClosingBarline)";
    }
  if (text)
    for (i = 0; i < numvoices; i++)
      g_string_append (scripts[i + 1], text);
 g_free (style);
}



 /*          <direction placement="above">
    <direction-type>
    <wedge default-y="34" spread="0" type="crescendo"/>
    </direction-type>
    </direction>
  */
static gchar *
parse_direction_type (xmlNodePtr rootElem, GString * script, gchar *placement)
{
  xmlNodePtr childElem;
  gchar *pending = NULL;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    if (ELEM_NAME_EQ (childElem, "wedge"))
      {
        gchar *type = xmlGetProp (childElem, (xmlChar *) "type");
        gchar *spread = xmlGetProp (childElem, (xmlChar *) "spread");
        if (type && spread)
          {
            if (!strcmp (type, "crescendo"))
              g_string_append (script, "(if (Appending?)(d-MoveCursorLeft))(d-ToggleStartCrescendo)(GoToMeasureEnd)");
            if (!strcmp (type, "diminuendo"))
              g_string_append (script, "(if (Appending?)(d-MoveCursorLeft))(d-ToggleStartDiminuendo)(GoToMeasureEnd)");

            if (!strcmp (type, "stop"))
              {
                if (!strcmp (spread, "0"))
                  g_string_append (script, "(if (Appending?)(d-MoveCursorLeft))(d-ToggleEndDiminuendo)(GoToMeasureEnd)");
                else
                  g_string_append (script, "(if (Appending?)(d-MoveCursorLeft))(d-ToggleEndCrescendo)(GoToMeasureEnd)");
              }
          }
      }
     if (ELEM_NAME_EQ (childElem, "words"))
      {
          //FIXME get italic etc here xmlGetProp
          gchar *words = xmlNodeListGetString (childElem->doc, childElem->xmlChildrenNode, 1);
          //pending = g_strdup_printf ("(d-TextAnnotation \"%s\")(GoToMeasureEnd)", g_strescape(words, NULL));
          gchar *font_style = xmlGetProp (childElem, "font-style");
          if(font_style)
            {
                if(!strcmp(font_style, "italic"))
                    font_style = "\\\\italic";
                else
                    font_style = "";
            }
          else
            font_style = "";
          if(placement)
            {
            if(!strcmp(placement, "above"))
                placement = "^";
            else if(!strcmp(placement, "below"))
                placement = "_";
            else
                placement = "-";
            }
          else
            placement = "-";
        //if(pending==NULL)
         //   pending = "";
         gchar *thewords = escape_scheme (words);
        pending = g_strdup_printf ("(StandaloneText \"TextAnnotation\" \"%s\" \"%s\" \"%s\")(GoToMeasureEnd)", /*words g_strescape(words, NULL)*/ escape_scheme (words), placement, font_style);
        g_free (thewords);
        g_free (words);

      }


      /*
  <direction placement="below">
        <direction-type>
          <dynamics default-x="-21" default-y="-67" halign="center">
            <f/>
          </dynamics>
        </direction-type>
        <offset sound="yes">4</offset>
        <staff>2</staff>
        <sound dynamics="98"/>
      </direction>
   */
    if (ELEM_NAME_EQ (childElem, "dynamics"))
      {
       // if(pending==NULL)
       //     pending = "";
       // pending = g_strdup_printf ("%s(d-DynamicText \"%s\")(GoToMeasureEnd)", pending, childElem->xmlChildrenNode->name);

       g_string_append_printf (script, "(if (Appending?)(d-MoveCursorLeft))(d-DynamicText \"%s\")(GoToMeasureEnd)", childElem->xmlChildrenNode->name);

      }
  }
  return pending;
}

static gchar *
parse_direction (xmlNodePtr rootElem, GString * script, gchar *placement)
{
  xmlNodePtr childElem;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    if (ELEM_NAME_EQ (childElem, "direction-type"))
      return parse_direction_type (childElem, script, placement);



  }
  return NULL;
}

static void
get_staff_for_voice_measure (xmlNodePtr rootElem, gint * staff_for_voice)
{
  xmlNodePtr childElem;
  gint division = 0;
  gint current_voice = 1;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    if (ELEM_NAME_EQ (childElem, "note"))
      {
        get_staff_for_voice_note (childElem, staff_for_voice);
      }
  }
}

static gchar *
parse_measure (xmlNodePtr rootElem, GString ** scripts, gint * staff_for_voice, gint * divisions, gint * voice_timings, gint numvoices, gint measurenum)
{
  GString *ret = g_string_new ("");
  gint note_count = 0;
  xmlNodePtr childElem;
  gint division = 0;
  gint current_voice = 1;
  gint actual_notes = 1, normal_notes = 1;      /* for tuplets */
  gint last_voice_with_notes = 1;       /* in case a voice with not "note" elements moves the current voice on while unfinished stuff in last voice */
  GString *pendings = g_string_new("");
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    //g_debug("name %s at voicenumber %d at division %d\n", childElem->name, current_voice, division);
    if (ELEM_NAME_EQ (childElem, "attributes"))
      parse_attributes (childElem, scripts, numvoices, staff_for_voice, division, voice_timings, divisions, &current_voice, measurenum);

    if (ELEM_NAME_EQ (childElem, "backup"))
      {
        division -= parseDuration (&current_voice, childElem);  //g_debug("backward arrives at %d\n", division);
      }
    if (ELEM_NAME_EQ (childElem, "forward"))
      {
        division += parseDuration (&current_voice, childElem);  //g_debug("forward arrives at %d\n", division);
      }
    if (ELEM_NAME_EQ (childElem, "note"))
      {
        gchar *printing = xmlGetProp (childElem, "print-object");
        gboolean is_nonprinting = FALSE;
        if (printing && !strcmp (printing, "no"))
          is_nonprinting = TRUE;

        gchar *warning = parse_note (childElem, scripts, staff_for_voice, &division, *divisions, voice_timings, &current_voice, &actual_notes, &normal_notes, is_nonprinting);
        if(pendings->len)
            {
                g_string_prepend (pendings, "(d-MoveCursorLeft)");
                g_string_append (scripts[current_voice], pendings->str);
                g_string_assign(pendings, "");
            }
        note_count++;
        if (*warning)
          g_string_append_printf (ret, "%s at note number %d, ", warning, note_count);
        last_voice_with_notes = current_voice;
      }


    if (ELEM_NAME_EQ (childElem, "direction"))
      {                         //g_assert(current_voice>0);
        gchar *placement = xmlGetProp (childElem, "placement");
        gchar *text = parse_direction (childElem, scripts[current_voice], placement);
        if(text)
            g_string_append(pendings, text);
      }
    if (ELEM_NAME_EQ (childElem, "barline"))
      {
        parse_barline (childElem, scripts, numvoices);
      }
  }
  //g_assert(last_voice_with_notes>0);
  if ((actual_notes != 1) || (normal_notes != 1))
    g_string_append_printf (scripts[last_voice_with_notes], "\n;measure end with tuplet still active in voice %d\n(d-EndTuplet)", current_voice);
  g_string_free(pendings, TRUE);
  return g_string_free (ret, FALSE);
}

static gchar *
parse_part (xmlNodePtr rootElem)
{
  GString *warnings = g_string_new ("");
  gint i, j;
  xmlNodePtr childElem;
  gint numstaffs = 1, numvoices = 1;
  gint divisions = 384;         //will be overriden anyway.

  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    gint maxstaffs = 1, maxvoices = 1;
    if (ELEM_NAME_EQ (childElem, "measure"))
      {
        get_numstaffs_in_measure (childElem, &maxstaffs, &maxvoices);
        if (maxstaffs > numstaffs)
          numstaffs = maxstaffs;
        if (maxvoices > numvoices)
          numvoices = maxvoices;
      }
  }
  g_info ("Number of staffs %d, voices %d\n", numstaffs, numvoices);
  gint *staff_for_voice = (gint *) g_malloc0 (numvoices * sizeof (gint));

  GString **scripts = (GString **) g_malloc0 ((1 + numvoices) * sizeof (GString *));
  for (i = 0; i <= numvoices; i++)
    scripts[i] = g_string_new ("\n");
  gint *voice_timings = (gint *) g_malloc0 (numvoices * sizeof (gint));


  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    if (ELEM_NAME_EQ (childElem, "measure"))
      {
        get_staff_for_voice_measure (childElem, staff_for_voice);
      }
  }
  for (i = 0; i < numvoices; i++)
    if (staff_for_voice[i] == 0)
      {
        g_info ("Voicenum %d was not actually used", i + 1);
        staff_for_voice[i] = 1; //if a voice was not actually used, assign it to the first staff
      }

  gint *numvoices_for_staff = (gint *) g_malloc0 (numstaffs * sizeof (gint));

  for (i = 0; i < numvoices; i++)
    {                           //g_assert(staff_for_voice[i]>0);
      numvoices_for_staff[staff_for_voice[i] - 1]++;
    }

/* create script to make enough staffs and voices, we are already in staff 1 voice 1 */
  g_string_append (scripts[0], "(d-PushPosition)");
  for (i = 0; i < numstaffs; i++)
    {
      //g_debug("Staff %d with %d voices\n", i, numvoices_for_staff[i]);
      if (i > 0)                /*already have first staff */
        g_string_append (scripts[0], "(d-AddAfter)");
      for (j = 1 /*already have first voice */ ; j < numvoices_for_staff[i]; j++)
        {
          //g_debug("Voice %d on Staff %d\n", j, i);
          g_string_append (scripts[0], "(d-AddAfter)(d-SetCurrentStaffAsVoice)");
        }
    }
  g_string_append (scripts[0], "(d-PopPosition)");

  g_string_append (scripts[0], "(d-PushPosition)");
  gint measure_count = 1;
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {

    if (ELEM_NAME_EQ (childElem, "measure"))
      {
        gint maxduration = 0;
        memset (voice_timings, 0, numvoices * sizeof (gint));
        gchar *warning = parse_measure (childElem, scripts, staff_for_voice, &divisions, voice_timings, numvoices, measure_count);
        if (*warning)
          g_string_append_printf (warnings, "%s in bar %d.\n", warning, measure_count);
        for (i = 0; i < numvoices; i++)
          if (maxduration < voice_timings[i])
            maxduration = voice_timings[i];
        for (i = 0; i < numvoices; i++)
          {
            if (voice_timings[i] < maxduration)
              {
                insert_invisible_rest (scripts[i + 1], maxduration - voice_timings[i], divisions);
              }
            g_string_append (scripts[0], scripts[i + 1]->str);
            g_string_assign (scripts[i + 1], "");
            g_string_append_printf (scripts[0], "%s\n;;;;;;;finished voice %d\n", "(d-MoveToStaffDown)", i + 1);
            voice_timings[i] = 0;
          }
        g_string_append_printf (scripts[0], "(d-PopPosition)(if (not (d-MoveToMeasureRight))(d-AddMeasure))(d-PushPosition)\n;;;;;;;;;End of measure %d\n ", measure_count);
        measure_count++;
      }
  }
  g_string_append (scripts[0], "(d-PopPosition)");

  for (i = 0; i < numvoices; i++)
    {

      g_string_append (scripts[0], "(d-MoveToBeginning)(d-MoveToStaffDown)");
    }
  g_string_append (scripts[0], "(d-AddAfter)(d-InitialKey \"C major\")");
  if (warnings->len)
    g_warning ("Parsing MusicXML gave these warnings:\n%s", warnings->str);
  g_string_free (warnings, TRUE);
  InitialVoiceNum += numvoices;

  g_free (numvoices_for_staff);
  g_free (staff_for_voice);
  g_free (voice_timings);
  return g_string_free (scripts[0], FALSE);
}

static void
parse_identification (xmlNodePtr rootElem, GString *script)
{
  gchar *title = NULL;
  xmlNodePtr childElem;
   FOREACH_CHILD_ELEM (childElem, rootElem)
  {
    if (ELEM_NAME_EQ (childElem, "creator"))
        {
            title = xmlNodeListGetString (childElem->doc, childElem->xmlChildrenNode, 1);
            g_string_append_printf (script, "(d-BookComposer \"%s\")", title);
        }
    if (ELEM_NAME_EQ (childElem, "rights"))
        {
            title = xmlNodeListGetString (childElem->doc, childElem->xmlChildrenNode, 1);
            g_string_append_printf (script, "(d-BookCopyright \"%s\")", title);
        }
  }
  g_free (title);
}

gint
mxmlinput (gchar * filename)
{
  GError *err = NULL;
  gint ret = 0;
  xmlDocPtr doc = NULL;
  xmlNsPtr ns;
  xmlNodePtr rootElem;
  gboolean spillover = Denemo.prefs.spillover;
  Denemo.prefs.spillover = FALSE;

  /* ignore blanks between nodes that appear as "text" */
  xmlKeepBlanksDefault (0);
  /* Try to parse the file. */

  doc = xmlParseFile (filename);
  if (doc == NULL)
    {
      g_warning ("Could not read MusicXML file %s", filename);
      Denemo.prefs.spillover = spillover;
      return -1;
    }

  rootElem = xmlDocGetRootElement (doc);
  xmlNodePtr childElem;
  GString *script = g_string_new (";Score\n\n(d-MasterVolume 0) (d-IncreaseGuard) (d-StaffProperties \"denemo_name=voice 1\")");
  gint part_count = 1;
  InitialVoiceNum = 0;
  if (Warnings == NULL)
    Warnings = g_string_new ("");
  else
    g_string_assign (Warnings, "");
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
       if (ELEM_NAME_EQ (childElem, "movement-title"))
        {
            gchar *title = xmlNodeListGetString (childElem->doc, childElem->xmlChildrenNode, 1);
            if(title)
                g_string_append_printf (script, "(d-BookTitle \"%s\")", escape_scheme(title));
            g_free (title);
        }
     if (ELEM_NAME_EQ (childElem, "identification"))   {
            parse_identification (childElem, script);
    }
    if (ELEM_NAME_EQ (childElem, "part"))
      {
        g_string_append_printf (script, "\n;;-------------------------Part (ie Instrument) %d ----------------------\n", part_count++);
        g_string_append (script, parse_part (childElem));
      }
  }
  g_string_append (script, "(d-DeleteStaff)(d-MoveToEnd)(if (None?) (d-DeleteMeasureAllStaffs))(d-MasterVolume 1)(d-MoveToBeginning)(if (and (not (None?))(UnderfullMeasure?))(d-Upbeat))   (d-DecreaseGuard)  ");
#ifdef DEVELOPER
  {
    FILE *fp = fopen ("/home/rshann/junk.scm", "w");
    if (fp)
      {
        fprintf (fp, ";Parser not yet finished upbeat bug present:\n %s", script->str);
        fclose (fp);
      }
  }
#endif
  call_out_to_guile (script->str);
  g_string_free (script, TRUE);
  Denemo.prefs.spillover = spillover;
  return ret;
}

/* this code is a utility for generating a scheme structure for inclusion in the MusicGlyph script */
#ifdef DEVELOPER
static void parseFont (xmlNodePtr rootElem, GString *script) {
    xmlNodePtr childElem;g_print("Parse font\n");
     gchar buffer1[5000],buffer2[5000], current[5000];
     gboolean new_class = FALSE;
     current[0] = 0;
     FOREACH_CHILD_ELEM (childElem, rootElem)
  {
     if (ELEM_NAME_EQ (childElem, "glyph"))
        {
            gchar *title = xmlGetProp (childElem, (xmlChar *) "glyph-name");
            if(title) {

                gint i, j;
                gchar *c;

                for(i=0, j=0, c = title; *c; c++) {
                    if(*c=='.')
                        {
                            for(c++; *c;c++)
                                buffer2[j++] = *c;
                            c--;
                        }
                    else
                        buffer1[i++] = *c;
                }
                buffer1[i]=buffer2[j] = 0;

                if(*buffer2 == 0)
                    {
                        gchar * misc = "...";
                        new_class = strcmp(current, misc);
                        strcpy(current, misc);
                        strcpy(buffer2, buffer1);
                     }
                else
                    if((new_class = strcmp(current, buffer1)))
                        strcpy (current, buffer1);
                if(new_class)
                            g_string_append_printf(script, "))\n(cons (_ \"%s\") (list \n", current);
                g_string_append_printf (script, "\t(cons (_ \"%s\") \"%s\")\n", buffer2, title);

            }
        }
    }
}


static void parseDefs (xmlNodePtr rootElem, GString *script) {
    xmlNodePtr childElem;g_print("Parse defs\n");
     FOREACH_CHILD_ELEM (childElem, rootElem)
  {
     if (ELEM_NAME_EQ (childElem, "font"))
        {
           parseFont(childElem, script);
        }
    }
}

gint
fontinput (gchar * filename)
{
  GError *err = NULL;
  gint ret = 0;
  xmlDocPtr doc = NULL;
  xmlNsPtr ns;
  xmlNodePtr rootElem;
  /* ignore blanks between nodes that appear as "text" */
  xmlKeepBlanksDefault (0);
  /* Try to parse the file. */

  doc = xmlParseFile (filename);
  if (doc == NULL)
    {
      g_warning ("Could not read font file %s", filename);
      return -1;
    }

  rootElem = xmlDocGetRootElement (doc);
  xmlNodePtr childElem;
  GString *script = g_string_new ("");
  FOREACH_CHILD_ELEM (childElem, rootElem)
  {
      if (ELEM_NAME_EQ (childElem, "defs"))
        {
            parseDefs(childElem, script);
        }

  }
  {
    FILE *fp = fopen ("/home/rshann/junk.scm", "w");
    if (fp)
      {
        fprintf (fp, ";font glyphs:\n;;%s))\n", script->str);
        fclose (fp);
      }
  }
  return ret;
}
#endif
