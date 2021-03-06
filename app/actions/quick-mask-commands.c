/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-quick-mask.h"

#include "widgets/gimphelp-ids.h"

#include "dialogs/dialogs.h"
#include "dialogs/channel-options-dialog.h"

#include "actions.h"
#include "quick-mask-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   quick_mask_configure_callback (GtkWidget     *dialog,
                                             GimpImage     *image,
                                             GimpChannel   *channel,
                                             GimpContext   *context,
                                             const gchar   *channel_name,
                                             const GimpRGB *channel_color,
                                             gboolean       save_selection,
                                             gpointer       user_data);


/*  public functionss */

void
quick_mask_toggle_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  GimpImage *image;
  gboolean   active;
  return_if_no_image (image, data);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (active != gimp_image_get_quick_mask_state (image))
    {
      gimp_image_set_quick_mask_state (image, active);
      gimp_image_flush (image);
    }
}

void
quick_mask_invert_cmd_callback (GtkAction *action,
                                GtkAction *current,
                                gpointer   data)
{
  GimpImage *image;
  gint       value;
  return_if_no_image (image, data);

  value = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (value != gimp_image_get_quick_mask_inverted (image))
    {
      gimp_image_quick_mask_invert (image);
      gimp_image_flush (image);
    }
}

void
quick_mask_configure_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

#define CONFIGURE_DIALOG_KEY "gimp-image-quick-mask-configure-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), CONFIGURE_DIALOG_KEY);

  if (! dialog)
    {
      GimpRGB color;

      gimp_image_get_quick_mask_color (image, &color);

      dialog = channel_options_dialog_new (image, NULL,
                                           action_data_get_context (data),
                                           widget,
                                           _("Quick Mask Attributes"),
                                           "gimp-quick-mask-edit",
                                           GIMP_STOCK_QUICK_MASK_ON,
                                           _("Edit Quick Mask Attributes"),
                                           GIMP_HELP_QUICK_MASK_EDIT,
                                           NULL,
                                           &color,
                                           _("Edit Quick Mask Color"),
                                           _("_Mask opacity"),
                                           FALSE,
                                           quick_mask_configure_callback,
                                           NULL);

      dialogs_attach_dialog (G_OBJECT (image), CONFIGURE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}


/*  private functions  */

static void
quick_mask_configure_callback (GtkWidget     *dialog,
                               GimpImage     *image,
                               GimpChannel   *channel,
                               GimpContext   *context,
                               const gchar   *channel_name,
                               const GimpRGB *channel_color,
                               gboolean       save_selection,
                               gpointer       user_data)
{
  GimpRGB old_color;

  gimp_image_get_quick_mask_color (image, &old_color);

  if (gimp_rgba_distance (&old_color, channel_color) > 0.0001)
    {
      gimp_image_set_quick_mask_color (image, channel_color);
      gimp_image_flush (image);
    }

  gtk_widget_destroy (dialog);
}
