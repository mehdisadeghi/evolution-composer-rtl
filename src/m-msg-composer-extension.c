/*
 * Copyright (C) 2016 Red Hat, Inc. (www.redhat.com)
 *
 * This library is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <composer/e-msg-composer.h>
#include "m-msg-composer-extension.h"

struct _MMsgComposerExtensionPrivate {
	gint dummy;
};

G_DEFINE_DYNAMIC_TYPE (MMsgComposerExtension, m_msg_composer_extension, E_TYPE_EXTENSION)

static guint64
current_page_id (EWebKitEditor *wk_editor)
{
	return webkit_web_view_get_page_id (WEBKIT_WEB_VIEW (wk_editor));
}

static void
webkit_editor_set_element_attribute (EWebKitEditor *wk_editor,
                                     const gchar *selector,
                                     const gchar *attribute,
                                     const gchar *value)
{
	GDBusProxy *web_extension;
	g_object_get (wk_editor, "web-extension", &web_extension, NULL);

	if (!web_extension) {
		g_warning ("EHTMLEditorWebExtension not ready at %s!", G_STRFUNC);
		return;
	}

	e_util_invoke_g_dbus_proxy_call_with_error_check (
		web_extension,
		"ElementSetAttributeBySelector",
		g_variant_new (
			"(tsss)", current_page_id (wk_editor), selector, attribute, value),
		NULL);
}

static void
action_msg_composer_set_text_direction (GtkAction *action,
							            MMsgComposerExtension *msg_composer_ext,
								    	const gchar *dir)
{
	EMsgComposer *composer;
	EHTMLEditor *html_editor;
	EContentEditor *content_editor;

	g_return_if_fail (M_IS_MSG_COMPOSER_EXTENSION (msg_composer_ext));

	composer = E_MSG_COMPOSER (e_extension_get_extensible (E_EXTENSION (msg_composer_ext)));
	html_editor = e_msg_composer_get_editor (composer);
	content_editor = e_html_editor_get_content_editor (html_editor);

	if (g_strcmp0 (G_OBJECT_TYPE_NAME (content_editor), "EWebKitEditor") != 0) {
		g_warning ("EWebKitEditor not found. RTL module only works with EWebKitEditor.");
		return;
	}

	webkit_editor_set_element_attribute((EWebKitEditor*)content_editor, "html", "dir", dir);
	char style[33];
	sprintf(style, "direction:%s;unicode-bidi:embed;", dir);

	webkit_editor_set_element_attribute((EWebKitEditor*)content_editor,
									    "body",
									    "style",
									    style);	
}


static void
action_msg_composer_ltr (GtkAction *action,
						 MMsgComposerExtension *msg_composer_ext)
{
	action_msg_composer_set_text_direction(action,
										   msg_composer_ext,
										   "ltr");
}

static void
action_msg_composer_rtl (GtkAction *action,
						 MMsgComposerExtension *msg_composer_ext)
{
	action_msg_composer_set_text_direction(action,
										   msg_composer_ext,
										   "rtl");
}

static GtkActionEntry msg_composer_entries[] = {
	{ "ltr-composer-action",
	  "format-text-direction-ltr",
	  N_("Left_to_right"),
	  NULL,
	  N_("Left to right"),
	  G_CALLBACK (action_msg_composer_ltr) },
	{ "rtl-composer-action",
	  "format-text-direction-rtl",
	  N_("Right_to_left"),
	  NULL,
	  N_("Right to left"),
	  G_CALLBACK (action_msg_composer_rtl) }

};

static void
m_msg_composer_extension_add_ui (MMsgComposerExtension *msg_composer_ext,
				 EMsgComposer *composer)
{
	const gchar *ui_def =
		"<toolbar name='html-toolbar'>\n"
		"  <toolitem action='ltr-composer-action'/>\n"
		"  <toolitem action='rtl-composer-action'/>\n"
		"</toolbar>\n";

	EHTMLEditor *html_editor;
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
	GError *error = NULL;

	g_return_if_fail (M_IS_MSG_COMPOSER_EXTENSION (msg_composer_ext));
	g_return_if_fail (E_IS_MSG_COMPOSER (composer));

	html_editor = e_msg_composer_get_editor (composer);
	ui_manager = e_html_editor_get_ui_manager (html_editor);
	action_group = e_html_editor_get_action_group (html_editor, "html");

	/* Add actions to the action group. */
	e_action_group_add_actions_localized (
		action_group, GETTEXT_PACKAGE,
		msg_composer_entries, G_N_ELEMENTS (msg_composer_entries), msg_composer_ext);

	gtk_ui_manager_add_ui_from_string (ui_manager, ui_def, -1, &error);

	if (error) {
		g_warning ("%s: Failed to add ui definition: %s", G_STRFUNC, error->message);
		g_error_free (error);
	}

	gtk_ui_manager_ensure_update (ui_manager);
}

static void
m_msg_composer_extension_constructed (GObject *object)
{
	EExtension *extension;
	EExtensible *extensible;

	extension = E_EXTENSION (object);
	extensible = e_extension_get_extensible (extension);

	/* Chain up to parent's constructed() method. */
	G_OBJECT_CLASS (m_msg_composer_extension_parent_class)->constructed (object);

	//g_print ("Hello world from %s!\n", G_OBJECT_TYPE_NAME (extensible));

	m_msg_composer_extension_add_ui (M_MSG_COMPOSER_EXTENSION (object), E_MSG_COMPOSER (extensible));
}

static void
m_msg_composer_extension_class_init (MMsgComposerExtensionClass *class)
{
	GObjectClass *object_class;
	EExtensionClass *extension_class;

	g_type_class_add_private (class, sizeof (MMsgComposerExtensionPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->constructed = m_msg_composer_extension_constructed;

	/* Set the type to extend, it's supposed to implement the EExtensible interface */
	extension_class = E_EXTENSION_CLASS (class);
	extension_class->extensible_type = E_TYPE_MSG_COMPOSER;
}

static void
m_msg_composer_extension_class_finalize (MMsgComposerExtensionClass *class)
{
}

static void
m_msg_composer_extension_init (MMsgComposerExtension *msg_composer_ext)
{
	msg_composer_ext->priv = G_TYPE_INSTANCE_GET_PRIVATE (msg_composer_ext, M_TYPE_MSG_COMPOSER_EXTENSION, MMsgComposerExtensionPrivate);
}

void
m_msg_composer_extension_type_register (GTypeModule *type_module)
{
	m_msg_composer_extension_register_type (type_module);
}