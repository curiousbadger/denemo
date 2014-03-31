/* timesigdialog.cpp
 * a callback that creates a dialog boxes prompting the
 * user for information on changing the time signature
 
 * for Denemo, a gtk+ frontend to GNU Lilypond
 * (c) 2000-2005 Matthew Hiller */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "display/calculatepositions.h"
#include "command/commandfuncs.h"
#include "command/contexts.h"
#include <denemo/denemo.h>
#include "ui/dialogs.h"
#include "display/draw.h"
#include "command/measureops.h"
#include "command/objops.h"
#include "command/staffops.h"
#include "core/utils.h"
#include "command/selectops.h"

/**
 * Is the integer a power of 2, or the value 1
 *
 */
static gint
ispow2 (gint x)
{
  if (x < 1)
    return 0;
  for (; !(x & 1); x >>= 1)
    ;                           /* Go through all the low order bits that are equal to 0 */
  return (x == 1);
}


/**
 * Set the timesig values for the given staff
 * 
 */
static void
settimesig (DenemoStaff * curstaffstruct, gint time1, gint time2)
{
  curstaffstruct->timesig.time1 = time1;
  curstaffstruct->timesig.time2 = time2;
  beamsandstemdirswholestaff (curstaffstruct);
}

/**
 * Set the initial timesig on current staff or across entire score
 * @param si pointer to the DenemoMovement structure.
 * @param curstaffstruct the staff to set the timesig for.
 * @param time1 the time signature nominator
 * @param time2 the time signature denominator
 * @param all_staves apply the new time signature to all staves
 *
 * @return None
 */
void
dnm_setinitialtimesig (DenemoMovement * si, DenemoStaff * curstaffstruct, gint time1, gint time2, gboolean all_staves)
{
  staffnode *curstaff;
  take_snapshot ();
  if (time1 && time2 && ispow2 (time2))
    {
      if (all_staves)
        {
          for (curstaff = si->thescore; curstaff; curstaff = curstaff->next)
            {
              curstaffstruct = (DenemoStaff *) curstaff->data;
              settimesig (curstaffstruct, time1, time2);
            }
          find_leftmost_allcontexts (si);
        }
      else
        {
          settimesig (curstaffstruct, time1, time2);
          find_leftmost_staffcontext (curstaffstruct, si);
        }

    }
  displayhelper (Denemo.project);
  score_status(Denemo.project, TRUE);
}


/**
 * Insert time sig change on either a single staff or across the 
 * entire score.
 */
static void
insert_timesig (DenemoMovement * si, DenemoStaff * curstaffstruct, gint time1, gint time2)
{
  staffnode *curstaff = NULL;
  measurenode *curmeasure = NULL;
  objnode *firstobj = NULL;
  DenemoObject *firstmudobj = NULL;
  gboolean replacing = FALSE;   /* if we don't use this trick, anomalous
                                 * stuff can happen when replacing a time
                                 * signature */

  for (curstaff = si->thescore; curstaff; curstaff = curstaff->next)
    {
      if (((DenemoStaff *) curstaff->data)->is_parasite)
        continue;

      curmeasure = g_list_nth (firstmeasurenode (curstaff), si->currentmeasurenum - 1);
      /* First, look to see if there already is a time signature change at
         the beginning of this measure. If so, delete it first. */
      if (!curmeasure)
        continue;
      firstobj = firstobjnode (curmeasure);
      if (firstobj)
        firstmudobj = (DenemoObject *) firstobj->data;
      else
        firstmudobj = NULL;
      if (firstmudobj && firstmudobj->type == TIMESIG)
        {
          replacing = TRUE;
          curmeasure->data = g_list_remove_link ((objnode *) curmeasure->data, firstobj);
          freeobject (firstmudobj);
          g_list_free_1 (firstobj);
        }
      curmeasure->data = g_list_prepend ((objnode *) curmeasure->data, dnm_newtimesigobj (time1, time2));
      if (curmeasure == si->currentmeasure)
        {
          if (!replacing)
            si->cursor_x++;
          if (si->cursor_appending)
            si->currentobject = g_list_last ((objnode *) curmeasure->data);
          else
            si->currentobject = g_list_nth ((objnode *) curmeasure->data, si->cursor_x);
        }
      beamsandstemdirswholestaff ((DenemoStaff *) curstaff->data);
    }

}

/**
 * Callback to insert a time sig change
 * Calls timesig_change with the INSERT argument
 */
void
timesig_change_insert (GtkAction * action, DenemoScriptParam * param)
{
  GET_1PARAM (action, param, timesigname);
  DenemoProject *gui = Denemo.project;
  if (query)
    {
      gchar *curtimesig = g_strdup_printf ("%d/%d", gui->movement->cursortime1, gui->movement->cursortime2);
      g_string_assign (param->string, curtimesig);
      g_free (curtimesig);
      param->status = TRUE;
      return;
    }
  if (timesigname == NULL)
    timesig_change (gui, INSERT);
  else
    {
      DenemoStaff *curstaffstruct = (DenemoStaff *) gui->movement->currentstaff->data;
      gint time1, time2;
      sscanf (timesigname, "%d/%d", &time1, &time2);
      if (time1 && time2)
        {
          insert_timesig (gui->movement, curstaffstruct, time1, time2);
          param->status = TRUE;
          displayhelper (gui);
        }
    }
}

/**
 * Callback to change the initial  time sig change
 * Calls timesig_change with the CHANGEINITIAL argument
 */
void
timesig_change_initial (GtkAction * action, DenemoScriptParam * param)
{
  GET_1PARAM (action, param, timesigname);
  DenemoProject *gui = Denemo.project;
  if (query)
    {
      GList *curstaff = gui->movement->thescore;
      DenemoStaff *curstaffstruct = (DenemoStaff *) curstaff->data;
      gchar *curtimesig = g_strdup_printf ("%d/%d", curstaffstruct->timesig.time1, curstaffstruct->timesig.time2);
      g_string_assign (param->string, curtimesig);
      g_free (curtimesig);
      param->status = TRUE;
      return;
    }

  if (timesigname == NULL)
    timesig_change (gui, CHANGEINITIAL);
  else
    {
      DenemoStaff *curstaffstruct = (DenemoStaff *) gui->movement->currentstaff->data;
      gint time1, time2;
      sscanf (timesigname, "%d/%d", &time1, &time2);
      if (time1 && time2)
        {
          dnm_setinitialtimesig (gui->movement, curstaffstruct, time1, time2, TRUE);
          param->status = TRUE;
          displayhelper (gui);
        }
    }
}

/**
 * Time sig change dialog.  allows the user to set
 * the time signature to insert or change
 */
void
timesig_change (DenemoProject * gui, actiontype action)
{
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *textentry1;
  GtkWidget *textentry2;
  GtkWidget *checkbutton;

  DenemoStaff *curstaffstruct = (DenemoStaff *) gui->movement->currentstaff->data;

  if (gui->movement->lily_file && action == CHANGEINITIAL)
    return;                     /* no code for this yet - just edit textually */

  dialog = gtk_dialog_new_with_buttons (((action == CHANGEINITIAL) ? _("Change initial time signature") : _("Insert time signature change")), NULL,     /* parent window */
                                        (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_STOCK_CANCEL, NULL);

  GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  gtk_container_set_border_width (GTK_CONTAINER (content_area), 12);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (content_area), vbox);
  label = gtk_label_new (_("Enter desired time signature:"));
  gtk_container_add (GTK_CONTAINER (vbox), label);

  textentry1 = gtk_spin_button_new_with_range (1, 128, 1.0);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (textentry1), 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (textentry1), (gdouble) curstaffstruct->timesig.time1);
  gtk_container_add (GTK_CONTAINER (vbox), textentry1);

  gtk_entry_set_activates_default (GTK_ENTRY (textentry1), TRUE);

  textentry2 = gtk_spin_button_new_with_range (1, 16, 1.0);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (textentry2), 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (textentry2), (gdouble) curstaffstruct->timesig.time2);

  gtk_container_add (GTK_CONTAINER (vbox), textentry2);

  gtk_entry_set_activates_default (GTK_ENTRY (textentry2), TRUE);

  checkbutton = gtk_check_button_new_with_label (_("Current Staff Only"));
  if (action == CHANGEINITIAL)  // there is no code for changing keysig just in one staff, or at least none called here.
    gtk_container_add (GTK_CONTAINER (vbox), checkbutton);

  // gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton), TRUE);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
  gtk_widget_show_all (dialog);
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      gint time1 = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (textentry1));
      gint time2 = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (textentry2));
      gboolean all_staves = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton));
      if (action == CHANGEINITIAL)
        {
          dnm_setinitialtimesig (gui->movement, curstaffstruct, time1, time2, all_staves);
        }
      else
        {
          if (gui->movement->currentobject && ((DenemoObject *) gui->movement->currentobject->data)->type == TIMESIG)
            deleteobject (NULL, NULL);
          insert_timesig (gui->movement, curstaffstruct, time1, time2);
        }
      displayhelper (gui);
    }
  score_status (gui, TRUE);
  gtk_widget_destroy (dialog);
}