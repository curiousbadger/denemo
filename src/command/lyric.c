/* lyric.c
 *
 * Functions for the manipulations of lyrics
 *
 * for Denemo, a gtk+ frontend for GNU Lilypond
 * (c)2009 Richard Shann
   */
#include <string.h>             /* for strcmp() */
#include "command/lyric.h"
#include "command/objops.h"
#include "command/staffops.h"
#include "core/utils.h"

static GtkWidget *DummyVerse;   /* a non-existent verse */

GtkTextView*
verse_get_current_view(DenemoStaff* staff){
  if(!staff)
    return NULL;
  if(!staff->current_verse_view)
    return NULL;
  return staff->current_verse_view->data;
}

void verse_set_current_view(DenemoStaff* staff, guint id){
  if(staff)
    staff->current_verse_view = g_list_nth (staff->verse_views, id);
  else
    g_debug("Trying to set a verse on an invalid staff");
}

guint
verse_get_current_pos(DenemoStaff* staff){
  return g_list_position(staff->verse_views, staff->current_verse_view);
}

gboolean
lyric_changed_cb (GtkTextBuffer * buffer)
{
  score_status (Denemo.project, TRUE);
  draw_score_area();
  return FALSE;
}

static GtkWidget *
new_lyric_editor (void)
{
  GtkWidget *view = gtk_text_view_new ();
  GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  if (Denemo.prefs.newbie)
    gtk_widget_set_tooltip_text (sw, _("The text of a verse can be typed or pasted here. Press Esc to return to editing notes.\nSeparate syllables with space double hyphen space, -- , if they should have their own note(s).\nNew lines and extra spaces have no special significance. Slurs on notes make them take only one syllable. Use the underscore _ for blank syllables."));
  gtk_container_add (GTK_CONTAINER (sw), view);

  return view;
}

DenemoObject *
newlyric (gint baseduration, gint numdots, gchar * lys)
{
  g_warning ("Not implemented");
  return NULL;
}

static void
switch_page (GtkNotebook * notebook, gpointer dummy, guint pagenum, DenemoStaff * staff)
{
  draw_score_area();
  verse_set_current_view (staff, pagenum);
}

//scans *next for a syllable putting the syllable into gs and moving *next to the address beyond the syllable
//return TRUE if a syllable was found
static gboolean
scan_syllable (gchar ** next, GString * gs)
{
  gboolean result;
  gchar *initial = *next;
  result = pango_scan_string ((const char **) next, gs);
  if (result && (*gs->str == '\\') && (*(gs->str + 1) != '\\') && (*(gs->str + 1) != '\"'))
  {
    while (**next && **next != '\n')
      (*next)++;              //skip to end of line
    return scan_syllable (next, gs);
  }
  if (result && ((!strcmp (gs->str, "--") || (!strcmp (gs->str, "__")))))
    return scan_syllable (next, gs);
  return result;
}


//get the count of the syllable at the cursor.
static gint get_syllable_count (GtkTextBuffer *buffer)
{
  GString *gs = g_string_new("");
  GtkTextIter cursor, startiter;
  gtk_text_buffer_get_iter_at_mark (buffer, &cursor, gtk_text_buffer_get_insert (buffer));   
  gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (buffer), &startiter);
  gchar *text = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (buffer), &startiter, &cursor, FALSE);
  gchar *next = text;
  gint count = 0;
  while (scan_syllable(&next, gs))
    {
            count++;
        
    }
  g_string_free(gs, TRUE);
  g_free(text);
  return count?count:1;
}
/* fills the measure and object fields of the passed in position with values for num'th syllable */
static void
get_pos_at_syllable_count (DenemoStaff * staff, gint num, DenemoPosition *pos)
{
  gint count = 0;
  gint measurenum, objnum;
  GList *curmeasure = staff->measures;
  gboolean in_slur = FALSE;
  for (measurenum = 0; curmeasure && (count < num); measurenum++, curmeasure = curmeasure->next)
    {
      objnode *curobj;
      for (objnum=0, curobj = curmeasure->data; curobj && (count < num); objnum++, curobj = curobj->next)
        {
          DenemoObject *obj = curobj->data;

          if (obj->type == CHORD)
            {
              chord *thechord = ((chord *) obj->object);
              if (thechord->notes && !in_slur)
                count++;
              if (thechord->slur_begin_p)
                in_slur = TRUE;
              if (thechord->slur_end_p)
                in_slur = FALSE;
              if (thechord->is_tied)
                count--;
            }
        }                       //for objs
    }                           //for measures

  pos->measure = measurenum;
  pos->object = objnum;
}
/* count the number of syllables up to Denemo cursor position */
 gint
syllable_count (void)
{
  DenemoStaff *thestaff = Denemo.project->movement->currentstaff->data;
  gint count = 0;
  gint i;
  GList *curmeasure;
  gboolean in_slur = FALSE;
  objnode *curobj = Denemo.project->movement->currentobject;
  for (curmeasure = thestaff->measures; curmeasure; curmeasure = curmeasure->next)
    {
      objnode *thisobj;
      for (thisobj = curmeasure->data; thisobj && (thisobj != curobj); thisobj = thisobj->next)
        {
          DenemoObject *obj = thisobj->data;
          if (obj->type == CHORD)
            {
              chord *thechord = ((chord *) obj->object);
              if (thechord->notes && !in_slur)
                count++;
              if (thechord->slur_begin_p)
                in_slur = TRUE;
              if (thechord->slur_end_p)
                in_slur = FALSE;
              if (thechord->is_tied)
                count--;
            }
        }                       //for objs
        
       if(thisobj==curobj) break;   
    }                           //for measures

  return count;
}

static gint get_character_count_at_syllable (gchar *text, gint count)
{
    gint chars = 0;
    GString *gs = g_string_new ("");
    for(;count;count--) {
        gchar *next = text+chars;
        gint this;
        if(!scan_syllable (&next, gs))
            break;
        chars = next - text;
    }
  g_string_free(gs, TRUE);
  return chars;
}
gboolean synchronize_lyric_cursor(void)
{
  DenemoStaff *thestaff = Denemo.project->movement->currentstaff->data; 
  gint count = syllable_count() + 1;
  GtkTextView* verse_view = verse_get_current_view(thestaff);
  if (verse_view)
    {
        gchar *text = get_text_from_view (verse_view);
        gint character_count = get_character_count_at_syllable (text, count);
        GtkTextBuffer *textbuffer = gtk_text_view_get_buffer (verse_view); 
        GtkTextIter where; 
        gtk_text_buffer_get_iter_at_offset (textbuffer, &where, character_count);
        gtk_text_buffer_place_cursor (textbuffer, &where);
        gtk_widget_grab_focus (verse_view);
        gtk_text_view_scroll_mark_onscreen (verse_view, gtk_text_buffer_get_insert(textbuffer));
        return TRUE;
    }   
    return FALSE;
}

static void 
synchronize_cursor(GtkWidget *textview)
{
    DenemoStaff *thestaff = Denemo.project->movement->currentstaff->data;
    gint count;
    DenemoPosition pos;
    count = get_syllable_count (gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview)));
    get_pos_at_syllable_count (thestaff, count, &pos);
    goto_movement_staff_obj (NULL, 0, Denemo.project->movement->currentstaffnum, pos.measure, pos.object);
}

static gboolean 
text_inserted_cb (GtkWidget *textview, GdkEventKey *event )
{
    static gboolean seen_space;
    gchar *str = event->string;
    guint keyval = event->keyval;
    if ((keyval==0x20) || (keyval==0xFF0D)|| (keyval==0xFF09)|| (keyval==0xFF8D)) //space return tab Enter
        {
         seen_space = TRUE;
        } 
    else if ((keyval==0xFF51) || (keyval==0xFF52) ||(keyval==0xFF53) ||(keyval==0xFF54) || seen_space)//arrows
        {
            seen_space = FALSE;
            synchronize_cursor(textview);
        }
//Note a Control-l is received when used as a shortcut to switch to the lyrics pane, so we use Esc or Tab to switch back
   if(  (keyval == 0xFF09) //TAB
        || (keyval == 0xFF1B)) //ESC
    {
      switch_back_to_main_window ();
      return TRUE;
    }
    return FALSE;
}

static gboolean 
button_released_cb (GtkWidget *textview)
{
    synchronize_cursor(textview);
    return FALSE;
}


GtkWidget *
add_verse_to_staff (DenemoMovement * movement, DenemoStaff * staff)
{
  if(Denemo.non_interactive)
    return NULL;
  GtkWidget *notebook, *textview;
  if (staff->verse_views == NULL)
  {
    notebook = gtk_notebook_new ();
    gtk_widget_show (notebook);

    g_signal_connect (G_OBJECT (notebook), "switch_page", G_CALLBACK (switch_page), staff);
    if (movement->lyricsbox == NULL)
      //FIXME we need a proper way of getting to the top vbox, that will not break when scorearea is moved in the widget hierarchy.
      install_lyrics_preview (movement, gtk_widget_get_parent (gtk_widget_get_parent (Denemo.scorearea)));
    gtk_box_pack_start (GTK_BOX (movement->lyricsbox), notebook, TRUE, TRUE, 0);
    if (movement->measurewidth == DENEMO_INITIAL_MEASURE_WIDTH)
      movement->measurewidth = DENEMO_INITIAL_MEASURE_WIDTH * 3;
  }
  else
  {
    GtkWidget *w = staff->verse_views->data;
    notebook = gtk_widget_get_parent (gtk_widget_get_parent (w));
  }
  textview = new_lyric_editor ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (textview), GTK_WRAP_WORD_CHAR);
  gtk_widget_show_all (gtk_widget_get_parent (textview));
  staff->verse_views = g_list_append (staff->verse_views, textview);
  verse_set_current_view (staff, g_list_position (staff->verse_views, g_list_last (staff->verse_views)));
  gint pagenum = gtk_notebook_append_page (GTK_NOTEBOOK (notebook), gtk_widget_get_parent (textview), NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), pagenum);
  gchar *tablabel = g_strdup_printf (_("Verse %d"), pagenum + 1);
  gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (notebook), gtk_widget_get_parent (textview), tablabel);
  g_free (tablabel);
  if (pagenum)
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), TRUE);
  GtkTextView* verse_view = verse_get_current_view (staff);
  g_signal_connect (G_OBJECT (gtk_text_view_get_buffer (verse_view)), "changed", G_CALLBACK (lyric_changed_cb), NULL);
  g_signal_connect (G_OBJECT(verse_view), "key-release-event",  G_CALLBACK (text_inserted_cb), NULL);
  g_signal_connect (G_OBJECT(verse_view), "button-release-event",  G_CALLBACK (button_released_cb), NULL);
  GdkRGBA grayed = {0.8, 0.8, 0.8, 1.0};
  GdkRGBA white = {1, 1, 1, 1.0};
  gtk_widget_override_background_color (verse_view, GTK_STATE_FLAG_FOCUSED, &white);
  gtk_widget_override_background_color (verse_view, GTK_STATE_FLAG_NORMAL, &grayed);
  return textview;
}

void
add_verse (GtkAction * action, DenemoScriptParam * param)
{
  DenemoProject *project = Denemo.project;
  DenemoMovement *movement = project->movement;
  if (project->movement->currentstaff)
  {
    DenemoStaff *staff = movement->currentstaff->data;
    add_verse_to_staff (movement, staff);
    signal_structural_change (project);
    GtkTextView* verse_view = verse_get_current_view (staff);
    gtk_widget_show (verse_view);
  }
}

void
delete_verse (GtkAction * action, DenemoScriptParam * param)
{
  DenemoProject *gui = Denemo.project;
  DenemoMovement *si = gui->movement;
  if (si->currentstaff)
  {
    DenemoStaff *staff = si->currentstaff->data;
    GtkTextView* verse_view = verse_get_current_view (staff);
    if (verse_view)
    {
      staff->verse_views = g_list_remove_link (staff->verse_views, verse_view);
      gtk_widget_destroy (gtk_widget_get_parent (verse_view));
      verse_set_current_view (staff, 0);
      signal_structural_change (gui);
      score_status (gui, TRUE);
      draw_score_area();
    }
  }

}


gchar *
get_text_from_view (GtkWidget * textview)
{
  GtkTextIter startiter, enditer;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
  gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (buffer), &startiter);
  gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (buffer), &enditer);
  return gtk_text_buffer_get_text (GTK_TEXT_BUFFER (buffer), &startiter, &enditer, FALSE);
}


// For the first call a textview is passed and the count'th syllable in that textview is set to be the next syllable returned. 
// Subsequent calls with NULL for textview return the next syllable of the textview that was set up by the above
static gchar *
lyric_iterator (GtkWidget * textview, gint count)
{
  static const gchar *next;
  static gchar *lyrics;
  static GString *gs;
  if (gs == NULL)
    gs = g_string_new ("");
  if (textview == NULL)
  {
    gboolean result = scan_syllable ((gchar **) & next, gs);
    if (result && gs->len)
      return gs->str;
    else
      return NULL;
  }
  if (textview != DummyVerse)
  {
    if (lyrics)
      g_free (lyrics);
    lyrics = get_text_from_view (textview);
    next = lyrics;
    while (count--)
      scan_syllable ((gchar **) & next, gs);
  }
  return NULL;
}

//for every chord while drawing next_syllable is called.
gchar *
next_syllable (void)
{
  return lyric_iterator (NULL, 0);
}


/* rename reset_lyrics */
void
reset_lyrics (DenemoStaff * staff, gint count)
{
  if(Denemo.non_interactive)
    return;

  if (DummyVerse == NULL)
    DummyVerse = gtk_text_view_new ();
  GtkTextView* verse_view = verse_get_current_view (staff);
  if (staff && verse_view)
    lyric_iterator (verse_view, count);
  else
    lyric_iterator (DummyVerse, count);
}



void
install_lyrics_preview (DenemoMovement * si, GtkWidget * top_vbox)
{
  if(Denemo.non_interactive)
    return;
  GtkWidget *parent = gtk_widget_get_parent(top_vbox);
      
  if (si->lyricsbox == NULL)
    si->lyricsbox = gtk_vbox_new (FALSE, 1);    //box to hold notebook of textview widgets
  if(parent)
    gtk_paned_add2 (GTK_PANED (parent), si->lyricsbox);
  if (Denemo.prefs.lyrics_pane)
    gtk_widget_show (si->lyricsbox);
}

/* hide the notebook of verses for the current staff */
void
hide_lyrics (void)
{
  DenemoProject *gui = Denemo.project;
  if (gui->movement->currentstaff && ((DenemoStaff *) gui->movement->currentstaff->data)->verse_views)
    gtk_widget_hide (gtk_widget_get_parent (gtk_widget_get_parent (((DenemoStaff *) gui->movement->currentstaff->data)->verse_views->data)));      //hide the notebook
}

/* show the notebook of verses for the current staff hide all others*/
void
show_lyrics (void)
{
  DenemoProject *gui = Denemo.project;
  DenemoMovement *si = gui->movement;
  if (si->currentstaff && ((DenemoStaff *) si->currentstaff->data)->verse_views)
    gtk_widget_show (gtk_widget_get_parent (gtk_widget_get_parent (((DenemoStaff *) si->currentstaff->data)->verse_views->data)));   //show the notebook
  select_lyrics ();
}

/* hide the notebooks of verses for the non-current staffs */
void
select_lyrics (void)
{
  DenemoProject *gui = Denemo.project;
  DenemoMovement *si = gui->movement;
  GList *current = si->thescore;
  for (; current; current = current->next)
  {
    if (current != si->currentstaff && ((DenemoStaff *) current->data)->verse_views)
      gtk_widget_hide (gtk_widget_get_parent (gtk_widget_get_parent (((DenemoStaff *) current->data)->verse_views->data)));        //hide the notebook
  }
}

gchar *
get_lyrics_for_current_verse (DenemoStaff * thestaff)
{
  GtkTextView* verse_view = verse_get_current_view (thestaff);
  if (verse_view)
    return get_text_from_view (verse_view);
  else
    return NULL;
}

gboolean
append_lyrics_for_current_verse (DenemoStaff * thestaff, gchar * text)
{
  GtkTextView* verse_view = verse_get_current_view (thestaff);
  if (verse_view)
  {
    GtkTextIter iter;
    GtkTextBuffer *textbuffer = gtk_text_view_get_buffer (verse_view);
    gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (textbuffer), &iter);
    gtk_text_buffer_insert (textbuffer, &iter, text, -1);
    return TRUE;
  }
  else
    return FALSE;
}

gboolean
put_lyrics_for_current_verse (DenemoStaff * thestaff, gchar * text)
{
  GtkTextView* verse_view = verse_get_current_view (thestaff);
  if (verse_view)
  {
    GtkTextBuffer *textbuffer = gtk_text_view_get_buffer (verse_view);
    GtkTextIter startiter, enditer;
    gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (textbuffer), &startiter);
    gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (textbuffer), &enditer);
    gtk_text_buffer_delete (textbuffer, &startiter, &enditer);
    gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (textbuffer), &enditer);
    gtk_text_buffer_insert (textbuffer, &enditer, text, -1);
    return TRUE;
  }
  else
    return FALSE;
}

gchar * get_lyrics_for_verse_num (gint number)
{
  DenemoProject * gui = Denemo.project; if (gui->movement->currentstaff)
  {
    DenemoStaff * thestaff = ((DenemoStaff *) gui->movement->currentstaff->data); if (thestaff->verse_views)
    {
      GList * verse = g_list_nth (thestaff->verse_views, number - 1); if (verse) return get_text_from_view (verse->data);
    }
  }
  return NULL;
}
