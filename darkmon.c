#include <gio/gio.h>
#include <stdio.h>

void update(GSettings *settings, gchar *key, gpointer data) {
	gchar *colorscheme = g_settings_get_string(settings, "color-scheme");
	gint dark = strcmp(colorscheme, "prefer-dark") == 0;
	g_free(colorscheme);
	printf("%s\n", dark ? "dark" : "light");
	fflush(stdout);
}

int main(int argc, char *argv[]) {
	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	GSettings *settings = g_settings_new("org.gnome.desktop.interface");
	g_signal_connect(settings, "changed::color-scheme",
			G_CALLBACK(update), NULL);
	update(settings, "color-scheme", NULL);
	g_main_loop_run(loop);
	return 0;
}
