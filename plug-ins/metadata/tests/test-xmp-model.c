/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 2011 Róman Joost <romanofski@gimp.org>
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

#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "xmp-parse.h"
#include "xmp-encode.h"
#include "xmp-model.h"


#define ADD_TEST(function) \
  g_test_add ("/metadata-xmp-model/" #function, \
              GimpTestFixture, \
              NULL, \
              gimp_test_xmp_model_setup, \
              function, \
              gimp_test_xmp_model_teardown);


typedef struct
{
  XMPModel *xmp_model;
} GimpTestFixture;


static void gimp_test_xmp_model_setup       (GimpTestFixture *fixture,
                                             gconstpointer    data);
static void gimp_test_xmp_model_teardown    (GimpTestFixture *fixture,
                                             gconstpointer    data);


/**
 * gimp_test_xmp_model_setup:
 * @fixture: GimpTestFixture fixture
 * @data:
 *
 * Test fixture to setup an XMPModel.
 **/
static void
gimp_test_xmp_model_setup (GimpTestFixture *fixture,
                           gconstpointer    data)
{
  fixture->xmp_model = xmp_model_new ();
}


static void
gimp_test_xmp_model_teardown (GimpTestFixture *fixture,
                              gconstpointer    data)
{
  g_object_unref (fixture->xmp_model);
}


/**
 * test_xmp_model_is_empty:
 * @fixture:
 * @data:
 *
 * Test to assert that newly created models are empty
 **/
static void
test_xmp_model_is_empty (GimpTestFixture *fixture,
                         gconstpointer    data)
{
  XMPModel *xmp_model;

  xmp_model = xmp_model_new ();

  g_assert (xmp_model_is_empty (xmp_model));
}

/**
 * test_xmp_model_set_get_scalar_property:
 * @fixture:
 * @data:
 *
 * The test asserts the API to get/set scalar values. This should also
 * work for many XMPProperties as we operate on one column in the
 * XMPModel. The COL_XMP_VALUE column is the string representation of
 * the COL_XMP_VALUE_RAW column, which will be updated upon export.
 **/
static void
test_xmp_model_set_get_scalar_property (GimpTestFixture *fixture,
                                        gconstpointer    data)
{
  const gchar  *property_name = NULL;
  const gchar  *scalar_value;
  gboolean      result;

  /* Schema is nonsense, so nothing is set */
  result = xmp_model_set_scalar_property (fixture->xmp_model,
                                           "SCHEMA",
                                           "key",
                                           "value");
  g_assert (result == FALSE);
  g_assert (xmp_model_is_empty (fixture->xmp_model) == TRUE);

  /* Contributor is a scalar property. When set, we expect the XMPModel
   * not to be empty any more and that we can retrieve the same value.
   **/
  property_name = "me_text";
  result = xmp_model_set_scalar_property (fixture->xmp_model,
                                           "dc",
                                           "contributor",
                                           property_name);
  g_assert (result == TRUE);
  g_assert (xmp_model_is_empty (fixture->xmp_model) == FALSE);

  scalar_value = xmp_model_get_scalar_property (fixture->xmp_model,
                                                "dc",
                                                "contributor");
  g_assert_cmpstr (scalar_value, ==, property_name);

  /* Now we assure, that we can even set titles, which is of type
   * XMP_TYPE_LANG_ALT.
   * The scalar value returned is always the first value next to the
   * language specifier.
   **/
  property_name = "lang=\"x-default\" title_lang_alt";
  result = xmp_model_set_scalar_property (fixture->xmp_model,
                                          "dc",
                                          "title",
                                          property_name);
  g_assert (result == TRUE);

  scalar_value = xmp_model_get_scalar_property (fixture->xmp_model,
                                                "dc",
                                                "title");
  g_assert_cmpstr (scalar_value, ==, "title_lang_alt");

  /* Setting an XMP_TYPE_LANG_ALT value without a language identifier,
   * we'll assume it is 'x-default' and the accessor returns the same
   * value.
   */
  result = xmp_model_set_scalar_property (fixture->xmp_model,
                                           "dc",
                                           "title",
                                           "me too");
  g_assert (result == TRUE);

  scalar_value = xmp_model_get_scalar_property (fixture->xmp_model,
                                         "dc", "title");
  g_assert_cmpstr (scalar_value, ==, "me too");
}


/**
 * test_xmp_model_find_xmptype_by:
 * @fixture:
 * @data:
 *
 * Tests returning the correct property type by given schema and
 * property_name
 **/
static void
test_xmp_model_find_xmptype_by (GimpTestFixture *fixture,
                                gconstpointer    data)
{
  XMPType   type;

  type = xmp_model_find_xmptype_by (fixture->xmp_model, "non", "sense");
  g_assert (type == -1);

  type = xmp_model_find_xmptype_by (fixture->xmp_model, "dc", "title");
  g_assert (type == XMP_TYPE_LANG_ALT);

  type = xmp_model_find_xmptype_by (fixture->xmp_model, "dc", "contributor");
  g_assert (type == XMP_TYPE_TEXT);
}


/**
 * test_xmp_model_get_raw_property_value:
 * @fixture:
 * @data:
 *
 * Tests the xmp_model_get_raw_property_value, which returns raw values
 * from the XMPModel.
 **/
static void
test_xmp_model_get_raw_property_value (GimpTestFixture *fixture,
                                       gconstpointer    data)
{
  gchar *expected_alt[] = {"en_GB", "my title", NULL};
  gchar *expected_seq[] = {"Wilber", NULL};
  const gchar **result = NULL;

  // NULL is returned if no value is set by given schema and property
  // name
  g_assert (xmp_model_get_raw_property_value (fixture->xmp_model,
                                              "dc", "title") == NULL);

  // XMP_TYPE_LANG_ALT
  // Note: XMP_TYPE_TEXT is tested with wrapper function
  // xmp_model_set_scalar_property
  xmp_model_set_property (fixture->xmp_model,
                          XMP_TYPE_LANG_ALT,
                          "dc",
                          "title",
                          (const gchar **) g_strdupv (expected_alt));

  result = xmp_model_get_raw_property_value (fixture->xmp_model,
                                             "dc", "title");
  g_assert (result != NULL);
  g_assert_cmpstr (result[0], ==, expected_alt[0]);

  // XMP_TYPE_TEXT_SEQ
  xmp_model_set_property (fixture->xmp_model,
                          XMP_TYPE_TEXT_SEQ,
                          "dc",
                          "creator",
                          (const gchar**) g_strdupv (expected_seq));
  result = xmp_model_get_raw_property_value (fixture->xmp_model,
                                             "dc", "creator");
  g_assert (result != NULL);
  g_assert_cmpstr (result[0], ==, expected_seq[0]);
}

/**
 * test_xmp_model_parse_file:
 * @fixture:
 * @data:
 *
 * A very simple test parsing a file with XMP DC metadata.
 **/
static void
test_xmp_model_parse_file (GimpTestFixture *fixture,
                           gconstpointer    data)
{
  gchar   *uri = NULL;
  const gchar   *value = NULL;
  GError  *error = NULL;

  uri = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"),
                          "plug-ins/metadata/tests/files/test.xmp",
                          NULL);
  g_assert (uri != NULL);

  xmp_model_parse_file (fixture->xmp_model, uri, &error);
  g_assert (! xmp_model_is_empty (fixture->xmp_model));

  // title
  value = xmp_model_get_scalar_property (fixture->xmp_model, "dc", "title");
  g_assert_cmpstr (value, == , "image title");

  // creator
  value = xmp_model_get_scalar_property (fixture->xmp_model, "dc", "creator");
  g_assert_cmpstr (value, == , "roman");

  // description
  value = xmp_model_get_scalar_property (fixture->xmp_model, "dc", "description");
  g_assert_cmpstr (value, == , "bla");

  g_free (uri);
}

int main(int argc, char **argv)
{
  gint result = -1;

  g_type_init();
  g_test_init (&argc, &argv, NULL);

  ADD_TEST (test_xmp_model_is_empty);
  ADD_TEST (test_xmp_model_find_xmptype_by);
  ADD_TEST (test_xmp_model_set_get_scalar_property);
  ADD_TEST (test_xmp_model_get_raw_property_value);
  ADD_TEST (test_xmp_model_parse_file);

  result = g_test_run ();

  return result;
}