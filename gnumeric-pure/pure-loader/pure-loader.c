#include <pure/runtime.h>
#include "pure-gnumeric.h"

#include "pure-loader.h"
#include <gnumeric.h>

#include <application.h>
#include <workbook-view.h>
#include <workbook.h>
#include <sheet.h>
#include <value.h>
#include <expr.h>
#include <expr-impl.h>
#include <gnm-plugin.h>

#include <goffice/app/error-info.h>
#include <goffice/app/go-plugin.h>
#include <goffice/app/go-plugin-loader.h>
#include <goffice/app/go-plugin-service.h>
#include <goffice/app/io-context.h>
#include <goffice/app/module-plugin-defs.h>
#include <gsf/gsf-impl-utils.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#define TYPE_GNM_PURE_PLUGIN_LOADER	(gnm_pure_plugin_loader_get_type ())
#define GNM_PURE_PLUGIN_LOADER(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_GNM_PURE_PLUGIN_LOADER, GnmPurePluginLoader))
#define IS_GNM_PURE_PLUGIN_LOADER(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_GNM_PURE_PLUGIN_LOADER))

typedef struct {
  GObject base;
  gchar* module_name;
} GnmPurePluginLoader;
typedef GObjectClass GnmPurePluginLoaderClass;

static const char help_template_text[] =
  "This Pure function hasn't been documented.";

static GnmFuncHelp help_template[] = {
  { GNM_FUNC_HELP_NAME, NULL },
  { GNM_FUNC_HELP_DESCRIPTION, NULL },
  { GNM_FUNC_HELP_END }
};

static void pure_init_help_consts(void)
{
  pure_def(pure_sym("GNM_FUNC_HELP_NAME"), pure_int(GNM_FUNC_HELP_NAME));
  pure_def(pure_sym("GNM_FUNC_HELP_ARG"), pure_int(GNM_FUNC_HELP_ARG));
  pure_def(pure_sym("GNM_FUNC_HELP_DESCRIPTION"), pure_int(GNM_FUNC_HELP_DESCRIPTION));
  pure_def(pure_sym("GNM_FUNC_HELP_NOTE"), pure_int(GNM_FUNC_HELP_NOTE));
  pure_def(pure_sym("GNM_FUNC_HELP_EXAMPLES"), pure_int(GNM_FUNC_HELP_EXAMPLES));
  pure_def(pure_sym("GNM_FUNC_HELP_SEEALSO"), pure_int(GNM_FUNC_HELP_SEEALSO));
#ifndef OLD_API
  pure_def(pure_sym("GNM_FUNC_HELP_EXTREF"), pure_int(GNM_FUNC_HELP_EXTREF));
  pure_def(pure_sym("GNM_FUNC_HELP_EXCEL"), pure_int(GNM_FUNC_HELP_EXCEL));
  pure_def(pure_sym("GNM_FUNC_HELP_ODF"), pure_int(GNM_FUNC_HELP_ODF));
#endif
}

static GnmFuncHelp *pure_default_gnm_help(const char *name)
{
  GnmFuncHelp *help = g_new(GnmFuncHelp, 3);
  if (help) {
    int i;
    for (i = 0; i < 3; i++)
      help[i] = help_template[i];
    help[0].text = g_strdup_printf("%s:", name);
    help[1].text = g_strdup(help_template_text);
  }
  return help;
}

static inline bool is_pair(pure_expr *x, pure_expr **y, pure_expr **z)
{
  pure_expr *f;
  size_t n;
  if (pure_is_appv(x, &f, &n, NULL) && n==2 &&
      strcmp(pure_sym_pname(f->tag), "=>") == 0) {
    if (y) *y = x->data.x[0]->data.x[1];
    if (z) *z = x->data.x[1];
    return true;
  } else
    return false;
}

static GnmFuncHelp *pure_get_gnm_help(pure_expr *x)
{
  size_t i, j, n;
  pure_expr **xv = NULL, *a, *b;
  GnmFuncHelp *help = NULL;
  if (pure_is_listv(x, &n, &xv)) {
    if (n == 0) return NULL;
    help = g_new(GnmFuncHelp, n+1);
    help[n].type = GNM_FUNC_HELP_END;
    help[n].text = NULL;
    for (i = 0; i < n; i++) {
      int32_t iv;
      const char *s;
      if (!is_pair(xv[i], &a, &b) ||
	  !pure_is_int(a, &iv) || iv < GNM_FUNC_HELP_NAME ||
#ifdef OLD_API
	  iv > GNM_FUNC_HELP_SEEALSO ||
#else
	  iv > GNM_FUNC_HELP_ODF ||
#endif
	  // XXXFIXME: Do we have to convert to the system encoding here?
	  !pure_is_string(b, &s)) {
	for (j = 0; j < i; j++) g_free((char*)help[j].text);
	free(xv);
	g_free(help);
	return NULL;
      }
      help[i].type = iv;
      help[i].text = g_strdup(s);
    }
  }
  if (xv) free(xv);
  return help;
}

static pure_interp *interp;

static GnmValue*
call_pure_function_args(GnmFuncEvalInfo *ei, GnmValue const * const *args)
{
  return call_pure_function(ei, -1, args);
}

static GnmValue*
call_pure_function_nodes(GnmFuncEvalInfo *ei, int argc,
			 GnmExprConstPtr const *argv)
{
  GnmValue *v, **args = g_new(GnmValue*, argc);
  gint i;
  for (i = 0; i < argc; i++)
    args[i] = gnm_expr_eval(argv[i], ei->pos,
			    GNM_EXPR_EVAL_PERMIT_NON_SCALAR);
  v = call_pure_function(ei, argc, (GnmValue const * const *)args);
  g_free(args);
  return v;
}

static DependentFlags pure_async_func_link(GnmFuncEvalInfo *ei);
static void pure_async_func_unlink(GnmFuncEvalInfo *ei);

static gboolean
gplp_func_desc_load(GOPluginService *service,
		    char const *name,
		    GnmFuncDescriptor *res)
{
  pure_expr *descfun = pure_symbol(pure_sym("gnm_info"));
  // XXXFIXME: Do we have to convert from the system encoding here?
  pure_expr *desc = pure_app(descfun, pure_string_dup(name));
  gchar *arg_spec = NULL;
  GnmFuncHelp *help = NULL;

  if (desc) {
    size_t size;
    pure_expr **elems = NULL;
    const char *spec = NULL;
    // XXXFIXME: Do we have to convert to the system encoding here?
    if (pure_is_tuplev(desc, &size, &elems) && size>0 && size<=2 &&
	(size == 1)
	? (pure_is_string(elems[0], &spec) ||
	   (help = pure_get_gnm_help(elems[0])))
	: (pure_is_string(elems[0], &spec) &&
	   (help = pure_get_gnm_help(elems[1])))) {
      if (spec) arg_spec = g_strdup(spec);
    }
    if (elems) free(elems);
    pure_freenew(desc);
  }

  res->name = g_strdup(name);
  res->arg_spec = arg_spec;
  if (!help) help = pure_default_gnm_help(name);
  res->help = help;
  res->fn_args = NULL;
  res->fn_nodes = NULL;
  if (arg_spec)
    res->fn_args = &call_pure_function_args;
  else
    res->fn_nodes = &call_pure_function_nodes;
  res->linker = pure_async_func_link;
  res->unlinker = pure_async_func_unlink;
  res->impl_status = GNM_FUNC_IMPL_STATUS_UNIQUE_TO_GNUMERIC;
  res->test_status = GNM_FUNC_TEST_STATUS_UNKNOWN;

  return TRUE;
}

static void
gplp_set_attributes(GOPluginLoader *loader, GHashTable *attrs,
		    GOErrorInfo **ret_error)
{
  GnmPurePluginLoader *loader_pure = GNM_PURE_PLUGIN_LOADER(loader);
  gchar *module_name = NULL;
  GO_INIT_RET_ERROR_INFO(ret_error);
  module_name = g_hash_table_lookup(attrs, "module_name");
  if (module_name)
    loader_pure->module_name = g_strdup(module_name);
  else
    *ret_error = go_error_info_new_str(_("** Pure module name not given."));
}

static GList *modnames;

static void
gplp_load_base(GOPluginLoader *loader, GOErrorInfo **ret_error)
{
  GnmPurePluginLoader *loader_pure = GNM_PURE_PLUGIN_LOADER(loader);
  const char *modname = loader_pure->module_name;
  const char *dir = go_plugin_get_dir_name(go_plugin_loader_get_plugin(loader));
  gchar *script, *path;

  script = g_strconcat(modname, ".pure", NULL);
  path = g_build_filename(dir, script, NULL);
  if (g_file_test(path, G_FILE_TEST_EXISTS)) {
    if (!interp) {
      // First invocation, create an interpreter instance.
      interp = pure_create_interp(0, 0);
      if (interp) pure_init_help_consts();
    }
    if (!interp)
      *ret_error = go_error_info_new_printf(_("** Error creating Pure interpreter."));
    else {
      gchar *cmdbuf = g_strdup_printf("using \"%s\";\n", path);
      // FIXME: This only prints errors on stderr right now.
      pure_evalcmd(cmdbuf);
      g_free(cmdbuf);
      modnames = g_list_append(modnames, path);
    }
  } else {
    *ret_error = go_error_info_new_printf(_("** Couldn't find \"%s\"."), script);
  }
  g_free(script);
}

static void pure_reload_script(gpointer data, gpointer unused)
{
  const char *path = (const char*)data;
  gchar *cmdbuf = g_strdup_printf("using \"%s\";\n", path);
  pure_evalcmd(cmdbuf);
  g_free(cmdbuf);
}

static void datasource_reinit(void);

void pure_stop(GnmAction const *action, WorkbookControl *wbc)
{
  if (interp && g_list_first(modnames)) {
    datasource_reinit();
  }
}

void pure_reload(GnmAction const *action, WorkbookControl *wbc)
{
  if (interp && g_list_first(modnames)) {
    datasource_reinit();
    pure_delete_interp(interp);
    interp = pure_create_interp(0, 0);
    pure_switch_interp(interp);
    g_list_foreach(modnames, pure_reload_script, NULL);
  }
}

void pure_edit(GnmAction const *action, WorkbookControl *wbc)
{
  GList *current = g_list_last(modnames);
  if (current) {
    const char *path = (const char*)current->data;
    const char *editor = getenv("EDITOR");
    gchar *cmdbuf = g_strdup_printf("%s \"%s\" &", editor?editor:"emacs", path);
    if (system(cmdbuf) == -1) perror("system");
    g_free(cmdbuf);
  }
}

/* Support for asynchronous data sources (adapted from sample_datasource.c). */

char *pure_async_filename = NULL;
static int pure_async_fd = -1;
static FILE *pure_async_file = NULL;
static guint pure_async_source = 0;
static GHashTable *datasources = NULL;

typedef struct {
  GnmExprFunction const *node; /* Expression node that calls us. */
  GnmDependent *dep;           /* GnmDependent containing that node. */
  unsigned id;                 /* id of this datasource. */
} DSKey;

typedef struct {
  DSKey key;
  pure_expr *expr;  /* Pure funcall that initiated this datasource. */
  pure_expr *value; /* Current value of the datasource. */
  int pid; /* inferior process */
} DataSource;

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "gnumeric:pure"

static guint
dskey_hash(DSKey const *k)
{
  return
    (GPOINTER_TO_INT(k->node) << 16) + (GPOINTER_TO_INT(k->dep) << 8) + k->id;
}

static gint
dskey_equal(DSKey const *k1, DSKey const *k2)
{
  return k1->node == k2->node && k1->dep == k2->dep && k1->id == k2->id;
}

static void
update_value(DataSource *ds, pure_expr *x)
{
  Sheet *sheet = ds->key.dep->sheet;
  if (ds->value) pure_free(ds->value);
  if (x)
    ds->value = pure_new(x);
  else
    ds->value = NULL;
  dependent_queue_recalc(ds->key.dep);
  if (sheet && workbook_get_recalcmode(sheet->workbook))
    workbook_recalc(sheet->workbook);
#if 0
  {
    char *s = val?str(val):strdup("#N/A");
    fprintf(stderr, "%p-%p-%u <= %s\n", key.node, key.dep, key.id, s);
    free(s);
  }
#endif
}

static gboolean
cb_pure_async_input(GIOChannel *gioc, GIOCondition cond, gpointer ignored)
{
  DSKey key;
  pure_expr *val;
  while (pure_read_blob(pure_async_file, (keyval_t*)&key, &val)) {
    DataSource *ds = g_hash_table_lookup(datasources, &key);
    if (ds) update_value(ds, val);
  }
  return TRUE;
}

gboolean
pure_async_func_init(const GnmFuncEvalInfo *ei, pure_expr *ex,
		     unsigned id, pure_expr **x)
{
  DataSource *ds = NULL,
    new = { { ei->func_call, ei->pos->dep, id }, NULL, NULL, 0 };
  gboolean ret = FALSE;
  /* If caller wants to be notified of updates */
  if (new.key.node != NULL && new.key.dep != NULL) {
    ds = g_hash_table_lookup(datasources, &new.key);
    if (ds == NULL) {
      ds = g_new(DataSource, 1);
      new.value = NULL; if (ex) new.expr = pure_new(ex);
      *ds = new;
      g_hash_table_insert(datasources, &ds->key, ds);
      ret = TRUE;
    } else if ((ex&&ds->expr)?!same(ex, ds->expr):ex!=ds->expr) {
      /* The initiating expression has changed. Nuke any old process and make
	 sure that we start a new one. */
      if (ds->pid > 0) {
	int status;
	kill(ds->pid, SIGTERM);
	if (waitpid(ds->pid, &status, 0) < 0) perror("waitpid");
	ds->pid = 0;
      }
      if (ds->expr) pure_free(ds->expr);
      if (ex) ds->expr = pure_new(ex);
      if (ds->value) pure_free(ds->value);
      ds->value = NULL;
      ret = TRUE;
    }
#if 0
    if (ret) fprintf(stderr, "new datasource = %p-%p-%u\n",
		     ei->func_call, ei->pos->dep, id);
#endif
#if 0
    if (!ret) fprintf(stderr, "datasource = %p-%p-%u\n",
		      ei->func_call, ei->pos->dep, id);
#endif
  }
  if (ds->value)
    *x = ds->value;
  else
    *x = pure_app(pure_symbol(pure_sym("gnm_error")),
		  pure_string_dup("#N/A"));
  return ret;
}

void
pure_async_func_process(const GnmFuncEvalInfo *ei, unsigned id, int pid)
{
  DSKey key = { ei->func_call, ei->pos->dep, id };
  if (key.node != NULL && key.dep != NULL) {
    DataSource *ds = g_hash_table_lookup(datasources, &key);
    if (ds) ds->pid = pid;
  }
}

static DependentFlags
pure_async_func_link(GnmFuncEvalInfo *ei)
{
#if 0
  fprintf(stderr, "link func %p\n", ei);
#endif
  return DEPENDENT_ALWAYS_UNLINK;
}

static void
pure_async_func_unlink(GnmFuncEvalInfo *ei)
{
  DSKey key = { ei->func_call, ei->pos->dep, 0 };
  DataSource *ds;
#if 0
  fprintf(stderr, "unlink func %p\n", ei);
#endif
  while ((ds = g_hash_table_lookup(datasources, &key)) != NULL) {
#if 0
    fprintf(stderr, "delete datasource = %p-%p-%u [%d]\n",
	    ei->func_call, ei->pos->dep, ds->key.id, ds->pid);
#endif
    if (ds->pid > 0) {
      // get rid of the inferior process
      int status;
      kill(ds->pid, SIGTERM);
      if (waitpid(ds->pid, &status, 0) < 0) perror("waitpid");
    }
    if (ds->value) pure_free(ds->value);
    if (ds->expr) pure_free(ds->expr);
    g_free(ds);
    key.id++;
  }
}

static void
cb_datasource_fini(gpointer key, gpointer value, gpointer closure)
{
  DataSource *ds = value;
#if 0
  fprintf(stderr, "delete datasource = %p-%p-%u [%d]\n",
	  ds->key.node, ds->key.dep, ds->key.id, ds->pid);
#endif
  if (ds->pid > 0) {
    // get rid of the inferior process
    int status;
    kill(ds->pid, SIGTERM);
    if (waitpid(ds->pid, &status, 0) < 0) perror("waitpid");
  }
  if (ds->value) pure_free(ds->value);
  if (ds->expr) pure_free(ds->expr);
  g_free(ds);
}

static void
datasource_reinit(void)
{
  g_hash_table_foreach(datasources, cb_datasource_fini, NULL);
  g_hash_table_destroy(datasources);
  datasources = g_hash_table_new((GHashFunc)dskey_hash,
				 (GEqualFunc)dskey_equal);
}

static void
datasource_init(void)
{
  GIOChannel *channel = NULL;
  char *filename = NULL, nambuf[L_tmpnam];
  // Set up a pipe for asynchronous data processing.
#if 0
  fprintf(stderr, ">>>>>>>>>>>>>>>>>>>>>>>>>>>> LOAD PURE_ASYNC\n");
#endif
  g_return_if_fail(pure_async_fd < 0);
  if (tmpnam(nambuf) && (filename = g_strdup(nambuf)) &&
    unlink(filename) <= 0 &&
    mkfifo(filename, S_IRUSR | S_IWUSR) == 0) {
    pure_async_filename = filename;
    pure_async_fd = g_open(pure_async_filename, O_RDWR|O_NONBLOCK, 0);
  } else if (filename)
    g_free(filename);
  if (pure_async_fd >= 0) {
    pure_async_file = fdopen(pure_async_fd, "rb");
    channel = g_io_channel_unix_new(pure_async_fd);
    pure_async_source =
      g_io_add_watch(channel, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
		     cb_pure_async_input, NULL);
    g_io_channel_unref(channel);
  }
  datasources = g_hash_table_new((GHashFunc)dskey_hash,
				 (GEqualFunc)dskey_equal);
}

static void
datasource_fini(void)
{
#if 0
  fprintf(stderr, "UNLOAD PURE_ASYNC >>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
#endif
  if (pure_async_source) {
    g_source_remove(pure_async_source);
    pure_async_source = 0;
  }
  if (pure_async_filename) {
    g_unlink(pure_async_filename);
    g_free(pure_async_filename);
    pure_async_filename = NULL;
  }
  if (pure_async_fd >= 0) {
    close(pure_async_fd);
    pure_async_fd = -1;
  }
  if (pure_async_file != NULL) {
    fclose(pure_async_file);
    pure_async_file = NULL;
  }
  g_hash_table_destroy(datasources);
  datasources = NULL;
}

/****************************************************************************/

static void
gplp_load_service_function_group(GOPluginLoader *loader,
				 GOPluginService *service,
				 GOErrorInfo **ret_error)
{
  PluginServiceFunctionGroupCallbacks *cbs;
  g_return_if_fail (IS_GNM_PLUGIN_SERVICE_FUNCTION_GROUP (service));
  GO_INIT_RET_ERROR_INFO (ret_error);
  cbs = go_plugin_service_get_cbs(service);
  cbs->func_desc_load = &gplp_func_desc_load;
}

static gboolean
gplp_service_load(GOPluginLoader *l, GOPluginService *s, GOErrorInfo **err)
{
  if (IS_GNM_PLUGIN_SERVICE_FUNCTION_GROUP(s))
    gplp_load_service_function_group(l, s, err);
  else
    return FALSE;
  datasource_init();
  return TRUE;
}

static gboolean
gplp_service_unload(GOPluginLoader *l, GOPluginService *s, GOErrorInfo **err)
{
  if (IS_GNM_PLUGIN_SERVICE_FUNCTION_GROUP(s))
    ;
  else
    return FALSE;
  datasource_fini();
  return TRUE;
}

static void
gplp_finalize(GObject *obj)
{
  GnmPurePluginLoader *loader_pure = GNM_PURE_PLUGIN_LOADER(obj);
  g_free(loader_pure->module_name);
  loader_pure->module_name = NULL;
  G_OBJECT_CLASS(g_type_class_peek(G_TYPE_OBJECT))->finalize(obj);
}

static void
go_plugin_loader_init(GOPluginLoaderClass *iface)
{
  iface->set_attributes		= gplp_set_attributes;
  iface->load_base		= gplp_load_base;
  iface->service_load		= gplp_service_load;
  iface->service_unload		= gplp_service_unload;
}

static void
gplp_class_init (GObjectClass *gobject_class)
{
  gobject_class->finalize = gplp_finalize;
}

static void
gplp_init (GnmPurePluginLoader *loader_pure)
{
  g_return_if_fail (IS_GNM_PURE_PLUGIN_LOADER (loader_pure));
  loader_pure->module_name = NULL;
}

GSF_DYNAMIC_CLASS_FULL(GnmPurePluginLoader, gnm_pure_plugin_loader,
		       NULL, NULL, gplp_class_init, NULL,
		       gplp_init, G_TYPE_OBJECT, 0,
		       GSF_INTERFACE_FULL(gnm_pure_plugin_loader_type,
					  go_plugin_loader_init,
					  GO_TYPE_PLUGIN_LOADER))
