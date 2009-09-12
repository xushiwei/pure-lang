/*
 * boot.c
 *
 */

#include <gnumeric-config.h>
#include <gnumeric.h>
#include <glib.h>
#include <goffice/app/error-info.h>
#include <gnm-plugin.h>

#include "pure-loader.h"

GNM_PLUGIN_MODULE_HEADER;

GType pure_get_loader_type(ErrorInfo **ret_error);

G_MODULE_EXPORT GType
pure_get_loader_type (ErrorInfo **ret_error)
{
  GO_INIT_RET_ERROR_INFO(ret_error);
  return gnm_pure_plugin_loader_get_type();
}

G_MODULE_EXPORT void
go_plugin_shutdown(GOPlugin *plugin, GOCmdContext *cc)
{
}

G_MODULE_EXPORT void
go_plugin_init(GOPlugin *plugin, GOCmdContext *cc)
{
  GTypeModule *module = go_plugin_get_type_module(plugin);
  gnm_pure_plugin_loader_register_type(module);
}

ModulePluginUIActions const pure_ui_actions[] = {
  { "PureMenu", NULL },
  { "PureReload", pure_reload },
  { "PureEdit", pure_edit },
  { NULL }
};
