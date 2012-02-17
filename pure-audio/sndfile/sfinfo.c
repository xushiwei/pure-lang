
#include "sfinfo.h"
#include <stdlib.h>

#define SIZEOF_SF_INFO sizeof(SF_INFO)
#define SIZEOF_SF_FORMAT_INFO sizeof(SF_FORMAT_INFO)

#define define(sym) pure_def(pure_sym(#sym), pure_int(sym))

void __sndfile_defs(void)
{
  define(SIZEOF_SF_INFO);
  define(SIZEOF_SF_FORMAT_INFO);
}

/* NOTE: This function isn't really needed as the same information is also
   available with the sf_version_string() API function. However, we supply
   this function for consistency with libsamplerate (which provides a
   src_get_version() function). In addition, this forces libsndfile to be
   linked into the module which may not be the case otherwise on some systems
   which use the --as-needed linker option by default, such as Arch Linux. */
const char *sf_get_version(void)
{
  static char buffer[128] ;
  sf_command(NULL, SFC_GET_LIB_VERSION, buffer, sizeof(buffer));
  return buffer;
}

SF_INFO *sf_make_info(int samplerate, int channels, int format)
{
  SF_INFO *info = malloc(SIZEOF_SF_INFO),
    _info = {0, samplerate, channels, format, 0, 0};
  if (info) *info = _info;
  return info;
}

SF_INFO *sf_new_info(void)
{
  SF_INFO *info = malloc(SIZEOF_SF_INFO),
    _info = {0, 0, 0, 0, 0, 0};
  if (info) *info = _info;
  return info;
}

pure_expr *sf_get_info(SF_INFO *info)
{
  if (info)
    return pure_tuplel(6, pure_int64(info->frames),
		       pure_int(info->samplerate),
		       pure_int(info->channels),
		       pure_int(info->format),
		       pure_int(info->sections),
		       pure_int(info->seekable));
  else
    return 0;
}

SF_FORMAT_INFO *sf_make_format_info(int format)
{
  SF_FORMAT_INFO *info = malloc(SIZEOF_SF_FORMAT_INFO),
    _info = {format, 0, 0};
  if (info) *info = _info;
  return info;
}

pure_expr *sf_get_format_info(SF_FORMAT_INFO *info)
{
  if (info) {
    pure_expr *x = info->name ? pure_cstring_dup(info->name) : pure_tuplel(0);
    pure_expr *y = info->extension ? pure_cstring_dup(info->extension) :
      pure_tuplel(0);
    return pure_tuplel(3, pure_int(info->format), x, y);
  } else
    return 0;
}
