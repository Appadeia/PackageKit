/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007-2008 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:pk-package-list
 * @short_description: A list of Package data needed for an offline cache
 *
 * These provide a way to query and store a list of packages.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <glib/gi18n.h>

#include "pk-debug.h"
#include "pk-common.h"
#include "pk-package-id.h"
#include "pk-package-item.h"
#include "pk-package-list.h"

static void     pk_package_list_class_init	(PkPackageListClass *klass);
static void     pk_package_list_init		(PkPackageList      *plist);
static void     pk_package_list_finalize	(GObject            *object);

#define PK_PACKAGE_LIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PK_TYPE_PACKAGE_LIST, PkPackageListPrivate))

/**
 * PkPackageListPrivate:
 *
 * Private #PkPackageList data
 **/
struct _PkPackageListPrivate
{
	GPtrArray	*array;
};

G_DEFINE_TYPE (PkPackageList, pk_package_list, G_TYPE_OBJECT)

/**
 * pk_package_list_add:
 **/
gboolean
pk_package_list_add (PkPackageList *plist, PkInfoEnum info, const gchar *package_id, const gchar *summary)
{
	PkPackageItem *item;

	g_return_val_if_fail (PK_IS_PACKAGE_LIST (plist), FALSE);
	g_return_val_if_fail (package_id != NULL, FALSE);

	item = pk_package_item_new (info, package_id, summary);
	g_ptr_array_add (plist->priv->array, item);

	return TRUE;
}

/**
 * pk_package_list_add_item:
 *
 * Makes a deep copy, and adds to the array
 **/
gboolean
pk_package_list_add_item (PkPackageList *plist, PkPackageItem *item)
{
	gboolean ret;
	PkPackageItem *item_new;

	g_return_val_if_fail (PK_IS_PACKAGE_LIST (plist), FALSE);
	g_return_val_if_fail (item != NULL, FALSE);

	ret = pk_package_list_contains_item (plist, item);
	if (ret) {
		pk_debug ("already added item");
		return FALSE;
	}

	item_new = pk_package_item_copy (item);
	g_ptr_array_add (plist->priv->array, item_new);

	return TRUE;
}

/**
 * pk_package_list_get_string:
 **/
gchar *
pk_package_list_get_string (PkPackageList *plist)
{
	PkPackageItem *item;
	guint i;
	guint length;
	const gchar *info_text;
	GString *package_cache;

	g_return_val_if_fail (PK_IS_PACKAGE_LIST (plist), NULL);

	package_cache = g_string_new ("");
	length = plist->priv->array->len;
	for (i=0; i<length; i++) {
		item = g_ptr_array_index (plist->priv->array, i);
		info_text = pk_info_enum_to_text (item->info);
		g_string_append_printf (package_cache, "%s\t%s\t%s\n", info_text, item->package_id, item->summary);
	}

	/* remove trailing newline */
	if (package_cache->len != 0) {
		g_string_set_size (package_cache, package_cache->len-1);
	}

	return g_string_free (package_cache, FALSE);
}

/**
 * pk_package_list_get_size:
 **/
guint
pk_package_list_get_size (PkPackageList *plist)
{
	g_return_val_if_fail (PK_IS_PACKAGE_LIST (plist), 0);
	return plist->priv->array->len;
}

/**
 * pk_package_list_get_item:
 **/
PkPackageItem *
pk_package_list_get_item (PkPackageList *plist, guint item)
{
	g_return_val_if_fail (PK_IS_PACKAGE_LIST (plist), NULL);
	if (item >= plist->priv->array->len) {
		pk_debug ("item too large!");
		return NULL;
	}
	return g_ptr_array_index (plist->priv->array, item);
}

/**
 * pk_package_list_clear:
 **/
gboolean
pk_package_list_clear (PkPackageList *plist)
{
	PkPackageItem *item;

	g_return_val_if_fail (PK_IS_PACKAGE_LIST (plist), FALSE);

	while (plist->priv->array->len > 0) {
		item = g_ptr_array_index (plist->priv->array, 0);
		pk_package_item_free (item);
		g_ptr_array_remove_index_fast (plist->priv->array, 0);
	}
	return TRUE;
}

/**
 * pk_package_list_contains:
 **/
gboolean
pk_package_list_contains (PkPackageList *plist, const gchar *package_id)
{
	PkPackageItem *item;
	guint i;
	guint length;
	gboolean ret = FALSE;

	g_return_val_if_fail (PK_IS_PACKAGE_LIST (plist), FALSE);
	g_return_val_if_fail (package_id != NULL, FALSE);

	length = plist->priv->array->len;
	for (i=0; i<length; i++) {
		item = g_ptr_array_index (plist->priv->array, i);
		ret = pk_package_id_equal (item->package_id, package_id);
		if (ret) {
			break;
		}
	}
	return ret;
}

/**
 * pk_package_list_contains_item:
 **/
gboolean
pk_package_list_contains_item (PkPackageList *plist, PkPackageItem *item)
{
	PkPackageItem *item_temp;
	guint i;
	guint length;
	gboolean ret = FALSE;

	g_return_val_if_fail (PK_IS_PACKAGE_LIST (plist), FALSE);
	g_return_val_if_fail (item != NULL, FALSE);

	length = plist->priv->array->len;
	for (i=0; i<length; i++) {
		item_temp = g_ptr_array_index (plist->priv->array, i);
		ret = pk_package_item_equal (item_temp, item);
		if (ret) {
			break;
		}
	}
	return ret;
}

/**
 * pk_package_list_class_init:
 * @klass: The PkPackageListClass
 **/
static void
pk_package_list_class_init (PkPackageListClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_package_list_finalize;
	g_type_class_add_private (klass, sizeof (PkPackageListPrivate));
}

/**
 * pk_package_list_init:
 **/
static void
pk_package_list_init (PkPackageList *plist)
{
	g_return_if_fail (plist != NULL);
	g_return_if_fail (PK_IS_PACKAGE_LIST (plist));

	plist->priv = PK_PACKAGE_LIST_GET_PRIVATE (plist);
	plist->priv->array = g_ptr_array_new ();
}

/**
 * pk_package_list_finalize:
 * @object: The object to finalize
 **/
static void
pk_package_list_finalize (GObject *object)
{
	PkPackageList *plist;
	g_return_if_fail (object != NULL);
	g_return_if_fail (PK_IS_PACKAGE_LIST (object));
	plist = PK_PACKAGE_LIST (object);
	g_return_if_fail (plist->priv != NULL);

	/* removed any cached packages */
	pk_package_list_clear (plist);
	g_ptr_array_free (plist->priv->array, TRUE);

	G_OBJECT_CLASS (pk_package_list_parent_class)->finalize (object);
}

/**
 * pk_package_list_new:
 *
 * Return value: a new PkPackageList object.
 **/
PkPackageList *
pk_package_list_new (void)
{
	PkPackageList *plist;
	plist = g_object_new (PK_TYPE_PACKAGE_LIST, NULL);
	return PK_PACKAGE_LIST (plist);
}

/***************************************************************************
 ***                          MAKE CHECK TESTS                           ***
 ***************************************************************************/
#ifdef PK_BUILD_TESTS
#include <libselftest.h>

void
libst_package_list (LibSelfTest *test)
{
	PkPackageList *plist;
	gchar *text;
	gboolean ret;

	if (libst_start (test, "PkPackageList", CLASS_AUTO) == FALSE) {
		return;
	}

	/************************************************************/
	libst_title (test, "create");
	plist = pk_package_list_new ();
	if (plist != NULL) {
		libst_success (test, NULL);
	} else {
		libst_failed (test, NULL);
	}

	/************************************************************/
	libst_title (test, "add entry");
	ret = pk_package_list_add (plist, PK_INFO_ENUM_INSTALLED, "gnome;1.23;i386;data", "GNOME!");
	if (ret) {
		libst_success (test, NULL);
	} else {
		libst_failed (test, NULL);
	}

	/************************************************************/
	libst_title (test, "check not exists");
	ret = pk_package_list_contains (plist, "liferea;1.23;i386;data");
	if (ret == FALSE) {
		libst_success (test, NULL);
	} else {
		libst_failed (test, NULL);
	}

	/************************************************************/
	libst_title (test, "check exists");
	ret = pk_package_list_contains (plist, "gnome;1.23;i386;data");
	if (ret) {
		libst_success (test, NULL);
	} else {
		libst_failed (test, NULL);
	}

	/************************************************************/
	libst_title (test, "check exists different data");
	ret = pk_package_list_contains (plist, "gnome;1.23;i386;fedora");
	if (ret) {
		libst_success (test, NULL);
	} else {
		libst_failed (test, NULL);
	}

	/************************************************************/
	libst_title (test, "add entry");
	text = pk_package_list_get_string (plist);
	if (pk_strequal (text, "installed\tgnome;1.23;i386;data\tGNOME!")) {
		libst_success (test, NULL);
	} else {
		libst_failed (test, "get string incorrect '%s'", text);
	}
	g_free (text);

	/************************************************************/
	libst_title (test, "add entry with NULL summary");
	ret = pk_package_list_add (plist, PK_INFO_ENUM_INSTALLED, "nosummary;1.23;i386;data", NULL);
	if (ret) {
		libst_success (test, NULL);
	} else {
		libst_failed (test, "could not add NULL summary");
	}

	libst_end (test);
}
#endif

