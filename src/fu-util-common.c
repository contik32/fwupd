/*
 * Copyright (C) 2017-2018 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include <config.h>

#include <appstream-glib.h>
#include <stdio.h>
#include <glib/gi18n.h>
#include <gusb.h>

#include "fu-util-common.h"
#include "fu-device.h"

void
fu_util_print_data (const gchar *title, const gchar *msg)
{
	gsize title_len;
	g_auto(GStrv) lines = NULL;

	if (msg == NULL)
		return;
	g_print ("%s:", title);

	/* pad */
	title_len = strlen (title) + 1;
	lines = g_strsplit (msg, "\n", -1);
	for (guint j = 0; lines[j] != NULL; j++) {
		for (gsize i = title_len; i < 25; i++)
			g_print (" ");
		g_print ("%s\n", lines[j]);
		title_len = 0;
	}
}

guint
fu_util_prompt_for_number (guint maxnum)
{
	gint retval;
	guint answer = 0;

	do {
		char buffer[64];

		/* swallow the \n at end of line too */
		if (!fgets (buffer, sizeof (buffer), stdin))
			break;
		if (strlen (buffer) == sizeof (buffer) - 1)
			continue;

		/* get a number */
		retval = sscanf (buffer, "%u", &answer);

		/* positive */
		if (retval == 1 && answer <= maxnum)
			break;

		/* TRANSLATORS: the user isn't reading the question */
		g_print (_("Please enter a number from 0 to %u: "), maxnum);
	} while (TRUE);
	return answer;
}

gboolean
fu_util_prompt_for_boolean (gboolean def)
{
	do {
		char buffer[4];
		if (!fgets (buffer, sizeof (buffer), stdin))
			continue;
		if (strlen (buffer) == sizeof (buffer) - 1)
			continue;
		if (g_strcmp0 (buffer, "\n") == 0)
			return def;
		buffer[0] = g_ascii_toupper (buffer[0]);
		if (g_strcmp0 (buffer, "Y\n") == 0)
			return TRUE;
		if (g_strcmp0 (buffer, "N\n") == 0)
			return FALSE;
	} while (TRUE);
	return FALSE;
}

gboolean
fu_util_print_device_tree (GNode *n, gpointer data)
{
	FwupdDevice *dev = FWUPD_DEVICE (n->data);
	const gchar *name;
	g_autoptr(GString) str = g_string_new (NULL);

	/* root node */
	if (dev == NULL) {
		g_print ("○\n");
		return FALSE;
	}

	/* add previous branches */
	for (GNode *c = n->parent; c->parent != NULL; c = c->parent) {
		if (g_node_next_sibling (c) == NULL)
			g_string_prepend (str, "  ");
		else
			g_string_prepend (str, "│ ");
	}

	/* add this branch */
	if (g_node_last_sibling (n) == n)
		g_string_append (str, "└─ ");
	else
		g_string_append (str, "├─ ");

	/* dump to the console */
	name = fwupd_device_get_name (dev);
	if (name == NULL)
		name = "Unknown device";
	g_string_append (str, name);
	for (guint i = strlen (name) + 2 * g_node_depth (n); i < 45; i++)
		g_string_append_c (str, ' ');
	g_print ("%s %s\n", str->str, fu_device_get_id (dev));
	return FALSE;
}

gboolean
fu_util_is_interesting_device (FwupdDevice *dev)
{
	if (fwupd_device_has_flag (dev, FWUPD_DEVICE_FLAG_UPDATABLE))
		return TRUE;
	if (fwupd_device_get_update_error (dev) != NULL)
		return TRUE;
	return FALSE;
}

gchar *
fu_util_get_user_cache_path (const gchar *fn)
{
	g_autofree gchar *basename = g_path_get_basename (fn);
	g_autofree gchar *cachedir_legacy = NULL;

	/* return the legacy path if it exists rather than renaming it to
	 * prevent problems when using old and new versions of fwupd */
	cachedir_legacy = g_build_filename (g_get_user_cache_dir (), "fwupdmgr", NULL);
	if (g_file_test (cachedir_legacy, G_FILE_TEST_IS_DIR))
		return g_build_filename (cachedir_legacy, basename, NULL);

	return g_build_filename (g_get_user_cache_dir (), "fwupd", basename, NULL);
}

gchar *
fu_util_get_versions (void)
{
	GString *string = g_string_new ("");

	g_string_append_printf (string,
				"client version:\t%i.%i.%i\n",
				FWUPD_MAJOR_VERSION,
				FWUPD_MINOR_VERSION,
				FWUPD_MICRO_VERSION);
#ifdef FWUPD_GIT_DESCRIBE
	g_string_append_printf (string,
				"checkout info:\t%s\n", FWUPD_GIT_DESCRIBE);
#endif
	g_string_append_printf (string,
				"compile-time dependency versions\n");
	g_string_append_printf (string,
				"\tappstream-glib:\t%d.%d.%d\n",
				AS_MAJOR_VERSION,
				AS_MINOR_VERSION,
				AS_MICRO_VERSION);
	g_string_append_printf (string,
				"\tgusb:\t%d.%d.%d\n",
				G_USB_MAJOR_VERSION,
				G_USB_MINOR_VERSION,
				G_USB_MICRO_VERSION);
#ifdef EFIVAR_LIBRARY_VERSION
	g_string_append_printf (string,
				"\tefivar:\t%s",
				EFIVAR_LIBRARY_VERSION);
#endif
	return g_string_free (string, FALSE);
}
