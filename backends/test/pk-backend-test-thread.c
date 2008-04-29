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

#include <gmodule.h>
#include <glib.h>
#include <string.h>
#include <pk-backend.h>

static gboolean is_cancelled = FALSE;

/**
 * backend_initialize:
 * This should only be run once per backend load, i.e. not every transaction
 */
static void
backend_initialize (PkBackend *backend)
{
	pk_debug ("FILTER: initialize");
}

/**
 * backend_destroy:
 * This should only be run once per backend load, i.e. not every transaction
 */
static void
backend_destroy (PkBackend *backend)
{
	pk_debug ("FILTER: destroy");
}

/**
 * backend_search_group_thread:
 */
static gboolean
backend_search_group_thread (PkBackend *backend)
{
	pk_backend_set_status (backend, PK_STATUS_ENUM_QUERY);

	/* emit */
	pk_backend_package (backend, PK_INFO_ENUM_INSTALLED,
			    "glib2;2.14.0;i386;fedora", "The GLib library");
	pk_backend_package (backend, PK_INFO_ENUM_INSTALLED,
			    "gtk2;gtk2-2.11.6-6.fc8;i386;fedora", "GTK+ Libraries for GIMP");
	pk_backend_finished (backend);
	return TRUE;
}

/**
 * backend_search_group:
 */
static void
backend_search_group (PkBackend *backend, PkFilterEnum filters, const gchar *search)
{
	pk_backend_thread_create (backend, backend_search_group_thread);
}

/**
 * backend_search_name_thread:
 */
static gboolean
backend_search_name_thread (PkBackend *backend)
{
	GTimer *timer;
	guint percentage;
	PkFilterEnum filters;
	gchar *filters_text;
	const gchar *search;

	filters = pk_backend_get_uint (backend, "filters");
	search = pk_backend_get_string (backend, "search");

	filters_text = pk_filter_enums_to_text (filters);
	pk_debug ("started task (%p) search=%s filters=%s", backend, search, filters_text);
	g_free (filters_text);
	pk_backend_set_status (backend, PK_STATUS_ENUM_QUERY);
	timer = g_timer_new ();
	percentage = 0;
	do {
		/* now is a good time to see if we should cancel the thread */
		if (is_cancelled) {
			pk_backend_error_code (backend, PK_ERROR_ENUM_TRANSACTION_CANCELLED,
					       "The thread was stopped successfully");
			pk_backend_finished (backend);
			return TRUE;
		}
		pk_backend_set_percentage (backend, percentage);
		percentage += 10;
		g_usleep (1000*100);
	} while (percentage < 100);
	g_timer_destroy (timer);
	pk_backend_set_percentage (backend, 100);
	pk_debug ("exited task (%p)", backend);

	pk_backend_package (backend, PK_INFO_ENUM_INSTALLED,
			    "glib2;2.14.0;i386;fedora", "The GLib library");
	pk_backend_package (backend, PK_INFO_ENUM_INSTALLED,
			    "gtk2;gtk2-2.11.6-6.fc8;i386;fedora", "GTK+ Libraries for GIMP");
	pk_backend_finished (backend);
	return TRUE;
}

/**
 * backend_search_name:
 */
static void
backend_search_name (PkBackend *backend, PkFilterEnum filters, const gchar *search)
{
	pk_backend_thread_create (backend, backend_search_name_thread);
}

/**
 * backend_cancel:
 */
static void
backend_cancel (PkBackend *backend)
{
	pk_debug ("cancelling %p", backend);
	is_cancelled = TRUE;
}

PK_BACKEND_OPTIONS (
	"Test Thread",				/* description */
	"Richard Hughes <richard@hughsie.com>",	/* author */
	backend_initialize,			/* initalize */
	backend_destroy,			/* destroy */
	NULL,					/* get_groups */
	NULL,					/* get_filters */
	backend_cancel,				/* cancel */
	NULL,					/* get_depends */
	NULL,					/* get_description */
	NULL,					/* get_files */
	NULL,					/* get_packages */
	NULL,					/* get_repo_list */
	NULL,					/* get_requires */
	NULL,					/* get_update_detail */
	NULL,					/* get_updates */
	NULL,					/* install_file */
	NULL,					/* install_package */
	NULL,					/* install_signature */
	NULL,					/* refresh_cache */
	NULL,					/* remove_package */
	NULL,					/* repo_enable */
	NULL,					/* repo_set_data */
	NULL,					/* resolve */
	NULL,					/* rollback */
	NULL,					/* search_details */
	NULL,					/* search_file */
	backend_search_group,			/* search_group */
	backend_search_name,			/* search_name */
	NULL,					/* service_pack */
	NULL,					/* update_package */
	NULL,					/* update_system */
	NULL					/* what_provides */
);

