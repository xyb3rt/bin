#include <fcntl.h>
#include <gtk/gtk.h>
#include <vte/vte.h>

gint acme;
gint dark;
const char *font = "monospace 10";
const char *colors[16] = {
	"#111111", "#cc0000", "#4e9a06", "#c4a000", "#3465a4", "#75507b",
	"#06989a", "#cccccc", "#555555", "#ef2929", "#8ae234", "#fce94f",
	"#729fcf", "#ad7fa8", "#34e2e2", "#eeeeee",
};
GdkRGBA palette[G_N_ELEMENTS(colors)];
static const double zooms[] = {
	1.0/1.728, 1.0/1.44, 1.0/1.2, 1.0, 1.0*1.2, 1.0*1.44, 1.0*1.728
};

static void new_window(GApplication *app, GApplicationCommandLine *cmdline);

static void done(VteTerminal *term, GApplicationCommandLine *cmdline,
		int status) {
	GtkWindow *win = GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(term)));
	if (cmdline) {
		g_application_command_line_set_exit_status(cmdline, status);
		g_object_unref(cmdline);
	}
	if (!win) {
		return;
	}
	GtkApplication *app = GTK_APPLICATION(gtk_window_get_application(win));
	if (g_list_length(gtk_application_get_windows(app)) == 1) {
		exit(status);
	}
	gtk_window_close(win);
}

static void set_colors(VteTerminal *term) {
	GdkRGBA bg, fg;
	gdk_rgba_parse(&bg, dark ? "#111111" : "#ffffdd");
	gdk_rgba_parse(&fg, dark ? "#eeeeee" : "#111111");
	vte_terminal_set_colors(term, &fg, &bg, palette, 16);
}

static void set_win_colors(gpointer win, gpointer data) {
	set_colors(VTE_TERMINAL(gtk_window_get_child((GtkWindow *)win)));
}

static void set_dark(GSettings *settings, gchar *key, gpointer data) {
	GtkApplication *app = (GtkApplication *)data;
	gchar *colorscheme = g_settings_get_string(settings, "color-scheme");
	dark = strcmp(colorscheme, "prefer-dark") == 0;
	g_free(colorscheme);
	g_object_set(gtk_settings_get_default(),
			"gtk-application-prefer-dark-theme", dark, NULL);
	g_list_foreach(gtk_application_get_windows(app), set_win_colors, NULL);
}

static gint on_cmdline(GApplication *app, GApplicationCommandLine *cmdline) {
	new_window(app, cmdline);
	return 0;
}

static gboolean on_key(GtkEventController *controller, guint key, guint code,
		GdkModifierType mod, gpointer data) {
	GtkWindow *win = (GtkWindow *)data;
	GApplication *app = G_APPLICATION(gtk_window_get_application(win));
	VteTerminal *term = VTE_TERMINAL(gtk_window_get_child(win));
	if (!(mod & GDK_CONTROL_MASK)) {
		return FALSE;
	} else if (key == GDK_KEY_N && (mod & GDK_SHIFT_MASK)) {
		new_window(app, NULL);
	} else if (key == GDK_KEY_C && (mod & GDK_SHIFT_MASK)) {
		vte_terminal_copy_clipboard_format(term, VTE_FORMAT_TEXT);
	} else if (key == GDK_KEY_V && (mod & GDK_SHIFT_MASK)) {
		vte_terminal_paste_clipboard(term);
	} else if (key == GDK_KEY_0) {
		gtk_window_set_default_size(win, -1, -1);
		vte_terminal_set_font_scale(term, 1.0);
	} else if (key == GDK_KEY_minus) {
		gdouble scale = vte_terminal_get_font_scale(term);
		for (size_t i = G_N_ELEMENTS(zooms); i > 0; i--) {
			if (scale - zooms[i-1] > 1e-6) {
				gtk_window_set_default_size(win, -1, -1);
				vte_terminal_set_font_scale(term, zooms[i-1]);
				break;
			}
		}
	} else if (key == GDK_KEY_plus) {
		gdouble scale = vte_terminal_get_font_scale(term);
		for (size_t i = 0; i < G_N_ELEMENTS(zooms); i++) {
			if (zooms[i] - scale > 1e-6) {
				gtk_window_set_default_size(win, -1, -1);
				vte_terminal_set_font_scale(term, zooms[i]);
				break;
			}
		}
	} else {
		return FALSE;
	}
	return TRUE;
}

static void on_title(VteTerminal *term, gpointer data) {
	GtkWindow *win = (GtkWindow *)data;
	char *title;
	g_object_get(term, "window-title", &title, NULL);
	gtk_window_set_title(win, title && title[0] != '\0' ? title :
			g_get_application_name());
	g_free(title);
}

static void on_exited(VteTerminal *term, int status, gpointer data) {
	GApplicationCommandLine *cmdline = (GApplicationCommandLine *)data;
	done(term, cmdline, WIFEXITED(status) ? WEXITSTATUS(status) : 1);
}

static void on_spawned(VteTerminal *term, GPid pid, GError *error,
		gpointer data) {
	GApplicationCommandLine *cmdline = (GApplicationCommandLine *)data;
	if (pid == -1) {
		if (cmdline) {
			g_application_command_line_printerr(cmdline, "%s\n",
					error->message);
		} else {
			fprintf(stderr, "%s\n", error->message);
		}
		done(term, cmdline, 1);
	} else {
		GtkWidget *widget = GTK_WIDGET(term);
		gtk_widget_grab_focus(widget);
		gtk_window_present(GTK_WINDOW(gtk_widget_get_parent(widget)));
	}
}

static void new_window(GApplication *app, GApplicationCommandLine *cmdline) {
	GtkWidget *widget;
	widget = gtk_application_window_new(GTK_APPLICATION(app));
	GtkWindow *win = GTK_WINDOW(widget);
	gtk_window_set_title(win, g_get_application_name());
	widget = vte_terminal_new();
	gtk_window_set_child(win, widget);
	VteTerminal *term = VTE_TERMINAL(widget);
	vte_terminal_set_audible_bell(term, FALSE);
	vte_terminal_set_bold_is_bright(term, FALSE);
	vte_terminal_set_cursor_blink_mode(term, VTE_CURSOR_BLINK_OFF);
	vte_terminal_set_mouse_autohide(term, TRUE);
	if (acme) {
		vte_terminal_set_size(term, 100, 60);
		vte_terminal_set_scrollback_lines(term, 0);
	}
	set_colors(term);
	PangoFontDescription *fd = pango_font_description_from_string(font);
	vte_terminal_set_font(term, fd);
	pango_font_description_free(fd);
	g_signal_connect(term, "window-title-changed", G_CALLBACK(on_title),
			win);
	g_signal_connect(term, "child-exited", G_CALLBACK(on_exited), cmdline);
	GtkEventController *controller = gtk_event_controller_key_new();
	g_signal_connect(controller, "key-pressed", G_CALLBACK(on_key), win);
	gtk_widget_add_controller(widget, controller);
	char **argv = NULL;
	const char *cwd = NULL;
	GStrvBuilder *cmdb = g_strv_builder_new();
	if (acme) {
		g_strv_builder_add(cmdb, "vim");
	}
	if (cmdline) {
		argv = g_application_command_line_get_arguments(cmdline, NULL);
		cwd = g_application_command_line_get_cwd(cmdline);
		g_object_ref(cmdline);
	}
	if (argv && argv[0] && argv[1]) {
		g_strv_builder_addv(cmdb, (const char **)&argv[1]);
	} else if (acme) {
		int fd = cwd ? open(cwd, O_RDONLY | O_DIRECTORY) : AT_FDCWD;
		if (fd != -1) {
			if (faccessat(fd, "guide", F_OK, 0) == 0) {
				g_strv_builder_add(cmdb, "guide");
			}
			if (fd != AT_FDCWD) {
				close(fd);
			}
		}
	} else {
		char *sh = getenv("SHELL");
		g_strv_builder_add(cmdb, sh && sh[0] != '\0' ? sh : "sh");
	}
	char **cmdv = g_strv_builder_unref_to_strv(cmdb);
	vte_terminal_spawn_async(term, VTE_PTY_DEFAULT, cwd, cmdv, NULL,
			G_SPAWN_DEFAULT, NULL, NULL, NULL, -1, NULL,
			on_spawned, cmdline);
	g_strfreev(cmdv);
	g_strfreev(argv);
}

int main(int argc, char *argv[]) {
	const char *s = argc > 0 ? strrchr(argv[0], '/') : NULL;
	acme = strcmp(s ? &s[1] : argv[0], "acme") == 0;
	gtk_init();
	for (size_t i = 0; i < G_N_ELEMENTS(colors); i++) {
		gdk_rgba_parse(&palette[i], colors[i]);
	}
	g_set_application_name(acme ? "Acme" : "XyTerm");
	GtkApplication *app = gtk_application_new(acme ? "xyb3rt.acme" :
			"xyb3rt.xyterm", G_APPLICATION_HANDLES_COMMAND_LINE);
	g_signal_connect(app, "command-line", G_CALLBACK(on_cmdline), NULL);
	GSettings *settings = g_settings_new("org.gnome.desktop.interface");
	g_signal_connect(settings, "changed::color-scheme",
			G_CALLBACK(set_dark), app);
	set_dark(settings, "color-scheme", app);
	return g_application_run(G_APPLICATION(app), argc, argv);
}
