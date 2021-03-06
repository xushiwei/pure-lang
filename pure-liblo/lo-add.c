
#include <pure/runtime.h>
#include <lo/lo.h>
#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#ifdef _WIN32
/* For some reason mingw fails to link in the lo_server_thread stuff.
   Force it to. */
static void *dummy = &lo_server_thread_new;
#endif

/* XXXFIXME: This needs to be crosschecked against the liblo source. We need
   these to hack some fields which aren't provided anywhere in the liblo API. */

typedef struct _lo_address {
	char            *host;
	int              socket;
	char            *port;
	int              protocol;
	struct addrinfo *ai;
	int              errnum;
	const char      *errstr;
	int              ttl;
} *mylo_address;

typedef struct _lo_message {
	char      *types;
	size_t     typelen;
	size_t     typesize;
	void      *data;
	size_t     datalen;
	size_t     datasize;
	lo_address source;
        lo_arg   **argv;
#if HAVE_TIMETAG
        /* timestamp from bundle (LO_TT_IMMEDIATE for unbundled messages) */
        lo_timetag ts;
#endif
} *mylo_message;

// #define DEBUG

/* GSL-compatible int matrix. */
typedef struct _gsl_matrix_int
{
  size_t size1;
  size_t size2;
  size_t tda;
  int *data;
  void *block;
  int owner;
} gsl_matrix_int;

int Pure_lo_message_add(lo_message msg, const char *types, pure_expr *x)
{
  int count = 0;
  void *p;
  size_t sz;
  pure_expr **xs;
  int i;
  int64_t i64;
  mpz_t z;
  float f;
  char *s;
  lo_blob b;
  uint8_t *m;
  lo_timetag tt;
  double d;
  int ret = 0;

  if (!pure_is_listv(x, &sz, &xs) && !pure_is_tuplev(x, &sz, &xs)) return -2;

  while (types && *types) {
    pure_expr *x = NULL;

    switch (*types++) {

    case LO_INT32:
      if (count < sz && (x = xs[count++]) && pure_is_int(x, &i))
	lo_message_add_int32(msg, i);
      else
	goto err;
      break;

    case LO_FLOAT:
      if (count < sz && (x = xs[count++]) && pure_is_double(x, &d))
	lo_message_add_float(msg, (float)d);
      else
	goto err;
      break;

    case LO_STRING:
      if (count < sz && (x = xs[count++]) && pure_is_cstring_dup(x, &s)) {
	lo_message_add_string(msg, s);
	free(s);
      } else
	goto err;
      break;

    case LO_BLOB:
      if (count < sz && (x = xs[count++]) && pure_is_pointer(x, &p)) {
	b = (lo_blob)p;
	lo_message_add_blob(msg, b);
      } else
	goto err;
      break;

    case LO_INT64:
      if (count < sz && (x = xs[count++]) && pure_is_mpz(x, &z)) {
	lo_message_add_int64(msg, pure_get_int64(x));
	mpz_clear(z);
      } else
	goto err;
      break;

    case LO_TIMETAG:
      if (count < sz && (x = xs[count++])) {
	if (pure_is_pointer(x, &p)) {
	  tt = *(lo_timetag*)p;
	  lo_message_add_timetag(msg, tt);
	} else if (pure_is_int_matrix(x, &p)) {
	  /* We also allow a 1x2 int vector here. */
	  gsl_matrix_int *mat = (gsl_matrix_int*)p;
	  int *data = mat->data;
	  size_t nrows = mat->size1, ncols = mat->size2;
	  if (nrows == 1 && ncols == 2) {
	    tt = *(lo_timetag*)data;
	    lo_message_add_timetag(msg, tt);
	  } else
	    goto err;
	} else
	  goto err;
      } else
	goto err;
      break;

    case LO_DOUBLE:
      if (count < sz && (x = xs[count++]) && pure_is_double(x, &d))
	lo_message_add_double(msg, d);
      else
	goto err;
      break;

    case LO_SYMBOL:
      if (count < sz && (x = xs[count++]) && pure_is_cstring_dup(x, &s)) {
	lo_message_add_symbol(msg, s);
	free(s);
      } else
	goto err;
      break;

    case LO_CHAR:
      if (count < sz && (x = xs[count++]) && pure_is_int(x, &i))
	lo_message_add_char(msg, i);
      else
	goto err;
      break;

    case LO_MIDI:
      if (count < sz && (x = xs[count++])) {
	if (pure_is_pointer(x, &p)) {
	  m = (uint8_t*)p;
	  lo_message_add_midi(msg, m);
	} else if (pure_is_int_matrix(x, &p)) {
	  /* We also allow a 1x4 int vector here. */
	  gsl_matrix_int *mat = (gsl_matrix_int*)p;
	  int *data = mat->data;
	  size_t nrows = mat->size1, ncols = mat->size2;
	  if (nrows == 1 && ncols == 4) {
	    m = (uint8_t*)matrix_to_byte_array(NULL, x);
	    if (m) {
	      lo_message_add_midi(msg, m);
	      free(m);
	    }
	  } else
	    goto err;
	} else
	  goto err;
      } else
	goto err;
      break;

    case LO_TRUE:
      lo_message_add_true(msg);
      break;

    case LO_FALSE:
      lo_message_add_false(msg);
      break;

    case LO_NIL:
      lo_message_add_nil(msg);
      break;

    case LO_INFINITUM:
      lo_message_add_infinitum(msg);
      break;

    default:
      ret = -1; // unknown type
#ifdef DEBUG
      fprintf(stderr, "liblo warning: unknown type '%c'\n", *(types-1));
#endif
      break;
    }
  }
  if (count < sz) {
    ret = -2; // bad format/args
#ifdef DEBUG
    fprintf(stderr, "liblo error: lo_send or lo_message_add called with "
	    "mismatching types and data, exiting.\n");
#endif
  }
  if (xs) free(xs);
  return ret;
 err:
#ifdef DEBUG
  fprintf(stderr, "liblo error: lo_send or lo_message_add called with "
	  "invalid arg %d, probably arg mismatch, exiting.\n", count);
#endif
  if (xs) free(xs);
  return -2;
}

int Pure_lo_send(mylo_address t, const char *path, const char *types,
		 pure_expr *x)
{
  int ret;
  lo_message msg = lo_message_new();

  t->errnum = 0;
  t->errstr = NULL;

  ret = Pure_lo_message_add(msg, types, x);

  if (ret) {
    lo_message_free(msg);
    t->errnum = ret;
    if (ret == -1) t->errstr = "unknown type";
    else t->errstr = "bad format/args";
    return ret;
  }

  ret = lo_send_message(t, path, msg);
  lo_message_free(msg);

  return ret;
}

int Pure_lo_send_timestamped(mylo_address t, lo_timetag *ts,
			     const char *path, const char *types, pure_expr *x)
{
  int ret;
  lo_message msg = lo_message_new();
  lo_bundle b = lo_bundle_new(*ts);

  t->errnum = 0;
  t->errstr = NULL;

  ret = Pure_lo_message_add(msg, types, x);

  if (t->errnum) {
    lo_message_free(msg);
    return t->errnum;
  }

  lo_bundle_add_message(b, path, msg);
  ret = lo_send_bundle(t, b);
  lo_message_free(msg);
  lo_bundle_free(b);

  return ret;
}

int Pure_lo_send_from(mylo_address to, lo_server from, lo_timetag *ts,
		      const char *path, const char *types, pure_expr *x)
{
  lo_bundle b = NULL;
  int ret;
  lo_message msg = lo_message_new();
  if (ts->sec!=LO_TT_IMMEDIATE.sec || ts->frac!=LO_TT_IMMEDIATE.frac)
    b = lo_bundle_new(*ts);

  // Clear any previous errors
  to->errnum = 0;
  to->errstr = NULL;

  ret = Pure_lo_message_add(msg, types, x);

  if (to->errnum) {
    if (b) lo_bundle_free(b);
    lo_message_free(msg);
    return to->errnum;
  }

  if (b) {
    lo_bundle_add_message(b, path, msg);
    ret = lo_send_bundle_from(to, from, b);
  } else {
    ret = lo_send_message_from(to, from, path, msg);
  }

  // Free-up memory
  lo_message_free(msg);
  if (b) lo_bundle_free(b);

  return ret;
}

/***************************************************************************/

/* The ringbuffer implementation is from Jack 0.116.2. We include this here to
   avoid depending on Jack and ease porting to non-Linux platforms. The
   original sources (licensed under the LGPL v2.1 or later) can be found at
   http://jackaudio.org/. -ag */

/*
  Copyright (C) 2000 Paul Davis
  Copyright (C) 2003 Rohan Drape
    
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.
    
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.
    
  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, write to the Free Software 
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
    
  ISO/POSIX C version of Paul Davis's lock free ringbuffer C++ code.
  This is safe for the case of one read thread and one write thread.
*/

#include <sys/types.h>
#ifdef USE_MLOCK
#include <sys/mman.h>
#endif /* USE_MLOCK */

typedef struct  
{
  char  *buf;
  size_t len;
} 
jack_ringbuffer_data_t ;

typedef struct
{
  char		 *buf;
  volatile size_t write_ptr;
  volatile size_t read_ptr;
  size_t	  size;
  size_t	  size_mask;
  int		  mlocked;
} 
jack_ringbuffer_t ;

/* Create a new ringbuffer to hold at least `sz' bytes of data. The
   actual buffer size is rounded up to the next power of two.  */

static jack_ringbuffer_t *
jack_ringbuffer_create (size_t sz)
{
  int power_of_two;
  jack_ringbuffer_t *rb;

  rb = malloc (sizeof (jack_ringbuffer_t));

  for (power_of_two = 1; 1 << power_of_two < sz; power_of_two++);

  rb->size = 1 << power_of_two;
  rb->size_mask = rb->size;
  rb->size_mask -= 1;
  rb->write_ptr = 0;
  rb->read_ptr = 0;
  rb->buf = malloc (rb->size);
  rb->mlocked = 0;

  return rb;
}

/* Free all data associated with the ringbuffer `rb'. */

static void
jack_ringbuffer_free (jack_ringbuffer_t * rb)
{
#ifdef USE_MLOCK
  if (rb->mlocked) {
    munlock (rb->buf, rb->size);
  }
#endif /* USE_MLOCK */
  free (rb->buf);
  free (rb);
}

/* Lock the data block of `rb' using the system call 'mlock'.  */

static int
jack_ringbuffer_mlock (jack_ringbuffer_t * rb)
{
#ifdef USE_MLOCK
  if (mlock (rb->buf, rb->size)) {
    return -1;
  }
#endif /* USE_MLOCK */
  rb->mlocked = 1;
  return 0;
}

/* Reset the read and write pointers to zero. This is not thread
   safe. */

static void
jack_ringbuffer_reset (jack_ringbuffer_t * rb)
{
  rb->read_ptr = 0;
  rb->write_ptr = 0;
}

/* Return the number of bytes available for reading.  This is the
   number of bytes in front of the read pointer and behind the write
   pointer.  */

static size_t
jack_ringbuffer_read_space (const jack_ringbuffer_t * rb)
{
  size_t w, r;

  w = rb->write_ptr;
  r = rb->read_ptr;

  if (w > r) {
    return w - r;
  } else {
    return (w - r + rb->size) & rb->size_mask;
  }
}

/* Return the number of bytes available for writing.  This is the
   number of bytes in front of the write pointer and behind the read
   pointer.  */

static size_t
jack_ringbuffer_write_space (const jack_ringbuffer_t * rb)
{
  size_t w, r;

  w = rb->write_ptr;
  r = rb->read_ptr;

  if (w > r) {
    return ((r - w + rb->size) & rb->size_mask) - 1;
  } else if (w < r) {
    return (r - w) - 1;
  } else {
    return rb->size - 1;
  }
}

/* The copying data reader.  Copy at most `cnt' bytes from `rb' to
   `dest'.  Returns the actual number of bytes copied. */

static size_t
jack_ringbuffer_read (jack_ringbuffer_t * rb, char *dest, size_t cnt)
{
  size_t free_cnt;
  size_t cnt2;
  size_t to_read;
  size_t n1, n2;

  if ((free_cnt = jack_ringbuffer_read_space (rb)) == 0) {
    return 0;
  }

  to_read = cnt > free_cnt ? free_cnt : cnt;

  cnt2 = rb->read_ptr + to_read;

  if (cnt2 > rb->size) {
    n1 = rb->size - rb->read_ptr;
    n2 = cnt2 & rb->size_mask;
  } else {
    n1 = to_read;
    n2 = 0;
  }

  memcpy (dest, &(rb->buf[rb->read_ptr]), n1);
  rb->read_ptr = (rb->read_ptr + n1) & rb->size_mask;

  if (n2) {
    memcpy (dest + n1, &(rb->buf[rb->read_ptr]), n2);
    rb->read_ptr = (rb->read_ptr + n2) & rb->size_mask;
  }

  return to_read;
}

/* The copying data reader w/o read pointer advance.  Copy at most 
   `cnt' bytes from `rb' to `dest'.  Returns the actual number of bytes 
copied. */

static size_t
jack_ringbuffer_peek (jack_ringbuffer_t * rb, char *dest, size_t cnt)
{
  size_t free_cnt;
  size_t cnt2;
  size_t to_read;
  size_t n1, n2;
  size_t tmp_read_ptr;

  tmp_read_ptr = rb->read_ptr;

  if ((free_cnt = jack_ringbuffer_read_space (rb)) == 0) {
    return 0;
  }

  to_read = cnt > free_cnt ? free_cnt : cnt;

  cnt2 = tmp_read_ptr + to_read;

  if (cnt2 > rb->size) {
    n1 = rb->size - tmp_read_ptr;
    n2 = cnt2 & rb->size_mask;
  } else {
    n1 = to_read;
    n2 = 0;
  }

  memcpy (dest, &(rb->buf[tmp_read_ptr]), n1);
  tmp_read_ptr = (tmp_read_ptr + n1) & rb->size_mask;

  if (n2) {
    memcpy (dest + n1, &(rb->buf[tmp_read_ptr]), n2);
  }

  return to_read;
}


/* The copying data writer.  Copy at most `cnt' bytes to `rb' from
   `src'.  Returns the actual number of bytes copied. */

static size_t
jack_ringbuffer_write (jack_ringbuffer_t * rb, const char *src, size_t cnt)
{
  size_t free_cnt;
  size_t cnt2;
  size_t to_write;
  size_t n1, n2;

  if ((free_cnt = jack_ringbuffer_write_space (rb)) == 0) {
    return 0;
  }

  to_write = cnt > free_cnt ? free_cnt : cnt;

  cnt2 = rb->write_ptr + to_write;

  if (cnt2 > rb->size) {
    n1 = rb->size - rb->write_ptr;
    n2 = cnt2 & rb->size_mask;
  } else {
    n1 = to_write;
    n2 = 0;
  }

  memcpy (&(rb->buf[rb->write_ptr]), src, n1);
  rb->write_ptr = (rb->write_ptr + n1) & rb->size_mask;

  if (n2) {
    memcpy (&(rb->buf[rb->write_ptr]), src + n1, n2);
    rb->write_ptr = (rb->write_ptr + n2) & rb->size_mask;
  }

  return to_write;
}

/* Advance the read pointer `cnt' places. */

static void
jack_ringbuffer_read_advance (jack_ringbuffer_t * rb, size_t cnt)
{
  size_t tmp = (rb->read_ptr + cnt) & rb->size_mask;
  rb->read_ptr = tmp;
}

/* Advance the write pointer `cnt' places. */

static void
jack_ringbuffer_write_advance (jack_ringbuffer_t * rb, size_t cnt)
{
  size_t tmp = (rb->write_ptr + cnt) & rb->size_mask;
  rb->write_ptr = tmp;
}

/* The non-copying data reader.  `vec' is an array of two places.  Set
   the values at `vec' to hold the current readable data at `rb'.  If
   the readable data is in one segment the second segment has zero
   length.  */

static void
jack_ringbuffer_get_read_vector (const jack_ringbuffer_t * rb,
				 jack_ringbuffer_data_t * vec)
{
  size_t free_cnt;
  size_t cnt2;
  size_t w, r;

  w = rb->write_ptr;
  r = rb->read_ptr;

  if (w > r) {
    free_cnt = w - r;
  } else {
    free_cnt = (w - r + rb->size) & rb->size_mask;
  }

  cnt2 = r + free_cnt;

  if (cnt2 > rb->size) {

    /* Two part vector: the rest of the buffer after the current write
       ptr, plus some from the start of the buffer. */

    vec[0].buf = &(rb->buf[r]);
    vec[0].len = rb->size - r;
    vec[1].buf = rb->buf;
    vec[1].len = cnt2 & rb->size_mask;

  } else {

    /* Single part vector: just the rest of the buffer */

    vec[0].buf = &(rb->buf[r]);
    vec[0].len = free_cnt;
    vec[1].len = 0;
  }
}

/* The non-copying data writer.  `vec' is an array of two places.  Set
   the values at `vec' to hold the current writeable data at `rb'.  If
   the writeable data is in one segment the second segment has zero
   length.  */

static void
jack_ringbuffer_get_write_vector (const jack_ringbuffer_t * rb,
				  jack_ringbuffer_data_t * vec)
{
  size_t free_cnt;
  size_t cnt2;
  size_t w, r;

  w = rb->write_ptr;
  r = rb->read_ptr;

  if (w > r) {
    free_cnt = ((r - w + rb->size) & rb->size_mask) - 1;
  } else if (w < r) {
    free_cnt = (r - w) - 1;
  } else {
    free_cnt = rb->size - 1;
  }

  cnt2 = w + free_cnt;

  if (cnt2 > rb->size) {

    /* Two part vector: the rest of the buffer after the current write
       ptr, plus some from the start of the buffer. */

    vec[0].buf = &(rb->buf[w]);
    vec[0].len = rb->size - w;
    vec[1].buf = rb->buf;
    vec[1].len = cnt2 & rb->size_mask;
  } else {
    vec[0].buf = &(rb->buf[w]);
    vec[0].len = free_cnt;
    vec[1].len = 0;
  }
}

/***************************************************************************/

/* Generic server callback function which stores messages in a ringbuffer. */

static bool init = false;
static jack_ringbuffer_t *rb = NULL;

static size_t packet_size(const char *host, const char *port, int proto,
			 lo_timetag *ts,
			 const char *path, const char *types,
			 lo_arg **argv, int argc)
{
  size_t sz = 0, i = 0, n = strlen(host), m = strlen(port);
  sz += sizeof(size_t);
  sz += n;
  sz += sizeof(size_t);
  sz += m;
  sz += sizeof(int);
  n = strlen(path); m = strlen(types);
  sz += sizeof(size_t);
  sz += n;
  sz += sizeof(size_t);
  sz += m;
  sz += sizeof(lo_timetag);
  while (types && *types) {
    switch (*types++) {
    case LO_CHAR:
      sz += sizeof(char);
      break;
    case LO_INT32:
      sz += sizeof(int);
      break;
    case LO_INT64:
      sz += sizeof(int64_t);
      break;
    case LO_FLOAT:
      sz += sizeof(float);
      break;
    case LO_DOUBLE:
      sz += sizeof(double);
      break;
    case LO_STRING:
    case LO_SYMBOL:
      m = strlen((char*)argv[i]);
      sz += sizeof(size_t);
      sz += m;
      break;
    case LO_BLOB:
      m = lo_blob_datasize(argv[i]);
      sz += sizeof(size_t);
      sz += m;
      break;
    case LO_TIMETAG:
      sz += sizeof(lo_timetag);
      break;
    case LO_MIDI:
      sz += 4*sizeof(uint8_t);
      break;
    default:
      /* no data */
      break;
    }
  }
  return sz;
}

/* NOTE: These are obviously not atomic, so we assume that there's only one
   writer and one reader here. */

static bool write_packet(const char *host, const char *port, int proto,
			 lo_timetag *ts,
			 const char *path, const char *types,
			 lo_arg **argv, int argc)
{
  size_t i = 0, n = strlen(host), m = strlen(port);
  if (packet_size(host, port, proto, ts, path, types, argv, argc) >
      jack_ringbuffer_write_space(rb))
    return false;
  jack_ringbuffer_write(rb, (char*)&n, sizeof(size_t));
  jack_ringbuffer_write(rb, host, n);
  jack_ringbuffer_write(rb, (char*)&m, sizeof(size_t));
  jack_ringbuffer_write(rb, port, m);
  jack_ringbuffer_write(rb, (char*)&proto, sizeof(int));
  jack_ringbuffer_write(rb, (char*)ts, sizeof(lo_timetag));
  n = strlen(path); m = strlen(types);
  jack_ringbuffer_write(rb, (char*)&n, sizeof(size_t));
  jack_ringbuffer_write(rb, path, n);
  jack_ringbuffer_write(rb, (char*)&m, sizeof(size_t));
  jack_ringbuffer_write(rb, types, m);
  while (types && *types) {
    switch (*types++) {
    case LO_CHAR:
      jack_ringbuffer_write(rb, (char*)argv[i++], sizeof(char));
      break;
    case LO_INT32:
      jack_ringbuffer_write(rb, (char*)argv[i++], sizeof(int));
      break;
    case LO_INT64:
      jack_ringbuffer_write(rb, (char*)argv[i++], sizeof(int64_t));
      break;
    case LO_FLOAT:
      jack_ringbuffer_write(rb, (char*)argv[i++], sizeof(float));
      break;
    case LO_DOUBLE:
      jack_ringbuffer_write(rb, (char*)argv[i++], sizeof(double));
      break;
    case LO_STRING:
    case LO_SYMBOL:
      m = strlen((char*)argv[i]);
      jack_ringbuffer_write(rb, (char*)&m, sizeof(size_t));
      jack_ringbuffer_write(rb, (char*)argv[i++], m);
      break;
    case LO_BLOB:
      m = lo_blob_datasize(argv[i]);
      jack_ringbuffer_write(rb, (char*)&m, sizeof(size_t));
      jack_ringbuffer_write(rb, (char*)lo_blob_dataptr(argv[i++]), m);
      break;
    case LO_TIMETAG:
      jack_ringbuffer_write(rb, (char*)argv[i++], sizeof(lo_timetag));
      break;
    case LO_MIDI:
      jack_ringbuffer_write(rb, (char*)argv[i++], 4*sizeof(uint8_t));
      break;
    default:
      /* no data */
      break;
    }
  }
  return true;
}

static void read_packet(char **host, char **port, int *proto,
			lo_timetag *ts,
			char **path, char **_types,
			lo_arg ***argv, int *argc)
{
  size_t i = 0, n, m;
  char *buf, *types;
  jack_ringbuffer_read(rb, (char*)&n, sizeof(size_t));
  *host = calloc(n+1, 1); assert(*host);
  jack_ringbuffer_read(rb, *host, n);
  jack_ringbuffer_read(rb, (char*)&m, sizeof(size_t));
  *port = calloc(m+1, 1); assert(*port);
  jack_ringbuffer_read(rb, *port, m);
  jack_ringbuffer_read(rb, (char*)proto, sizeof(int));
  jack_ringbuffer_read(rb, (char*)ts, sizeof(lo_timetag));
  jack_ringbuffer_read(rb, (char*)&n, sizeof(size_t));
  *path = calloc(n+1, 1); assert(*path);
  jack_ringbuffer_read(rb, *path, n);
  jack_ringbuffer_read(rb, (char*)&m, sizeof(size_t));
  *_types = calloc(m+1, 1); assert(*_types);
  jack_ringbuffer_read(rb, *_types, m);
  types = *_types;
  *argc = m; /* upper bound */
  *argv = calloc(m, sizeof(lo_arg*)); assert(*argv);
  while (types && *types) {
    switch (*types++) {
    case LO_CHAR:
      buf = malloc(sizeof(char)); assert(buf);
      (*argv)[i++] = (lo_arg*)buf;
      jack_ringbuffer_read(rb, buf, sizeof(char));
      break;
    case LO_INT32:
      buf = malloc(sizeof(int)); assert(buf);
      (*argv)[i++] = (lo_arg*)buf;
      jack_ringbuffer_read(rb, buf, sizeof(int));
      break;
    case LO_INT64:
      buf = malloc(sizeof(int64_t)); assert(buf);
      (*argv)[i++] = (lo_arg*)buf;
      jack_ringbuffer_read(rb, buf, sizeof(int64_t));
      break;
    case LO_FLOAT:
      buf = malloc(sizeof(float)); assert(buf);
      (*argv)[i++] = (lo_arg*)buf;
      jack_ringbuffer_read(rb, buf, sizeof(float));
      break;
    case LO_DOUBLE:
      buf = malloc(sizeof(double)); assert(buf);
      (*argv)[i++] = (lo_arg*)buf;
      jack_ringbuffer_read(rb, buf, sizeof(double));
      break;
    case LO_STRING:
    case LO_SYMBOL:
      jack_ringbuffer_read(rb, (char*)&m, sizeof(size_t));
      buf = calloc(m+1, 1); assert(buf); (*argv)[i++] = (lo_arg*)buf;
      jack_ringbuffer_read(rb, buf, m);
      break;
    case LO_BLOB:
      jack_ringbuffer_read(rb, (char*)&m, sizeof(size_t));
      buf = calloc(m+1, 1); assert(buf);
      jack_ringbuffer_read(rb, buf, m);
      (*argv)[i++] = lo_blob_new(m, buf); free(buf);
      break;
    case LO_TIMETAG:
      buf = malloc(sizeof(lo_timetag)); assert(buf);
      (*argv)[i++] = (lo_arg*)buf;
      jack_ringbuffer_read(rb, buf, sizeof(lo_timetag));
      break;
    case LO_MIDI:
      buf = malloc(4*sizeof(uint8_t)); assert(buf);
      (*argv)[i++] = (lo_arg*)buf;
      jack_ringbuffer_read(rb, buf, 4*sizeof(uint8_t));
      break;
    default:
      /* no data */
      break;
    }
  }
  *argc = i;
}
			 
int Pure_osc_handler(const char *path, const char *types, lo_arg **argv,
		     int argc, void *msg, void *user_data)
{
  lo_address src = lo_message_get_source(msg);
  const char *host = lo_address_get_hostname(src);
  const char *port = lo_address_get_port(src);
  int proto = lo_address_get_protocol(src);
#if HAVE_TIMETAG
  lo_timetag ts = lo_message_get_timestamp(msg);
#else
  lo_timetag ts = LO_TT_IMMEDIATE;
#endif

  if (!init) {
    rb = jack_ringbuffer_create(0x100000);
    init = true;
    if (!rb)
      fprintf(stderr, "generic_handler: error allocating ringbuffer\n");
  }
  if (rb)
    write_packet(host, port, proto, &ts, path, types, argv, argc);
  return 0;
}

pure_expr *Pure_osc_recv_noblock(void)
{
  pure_expr *res;
  mylo_address src;
  mylo_message msg;
  char *host, *port, *path, *types, *_types;
  int proto;
  lo_timetag ts;
  lo_arg **argv;
  int argc, i = 0;
  if (!rb || jack_ringbuffer_read_space(rb) == 0) return NULL;
  read_packet(&host, &port, &proto, &ts, &path, &types, &argv, &argc);
  _types = types;
  /* Reconstruct the message. */
  src = lo_address_new(host, port);
  src->protocol = proto;
  msg = lo_message_new();
  assert(src && msg);
  /* We have to to go a long way here. There must be a better way to do this. */
  while (types && *types) {
    switch (*types++) {
    case LO_CHAR:
      lo_message_add_char(msg, argv[i++]->c);
      break;
    case LO_INT32:
      lo_message_add_int32(msg, argv[i++]->i);
      break;
    case LO_INT64:
      lo_message_add_int64(msg, argv[i++]->h);
      break;
    case LO_FLOAT:
      lo_message_add_float(msg, argv[i++]->f);
      break;
    case LO_DOUBLE:
      lo_message_add_double(msg, argv[i++]->d);
      break;
    case LO_STRING:
      lo_message_add_string(msg, (char*)argv[i++]);
      break;
    case LO_SYMBOL:
      lo_message_add_symbol(msg, (char*)argv[i++]);
      break;
    case LO_BLOB:
      lo_message_add_blob(msg, (lo_blob)argv[i++]);
      break;
    case LO_TIMETAG:
      lo_message_add_timetag(msg, argv[i++]->t);
      break;
    case LO_MIDI:
      lo_message_add_midi(msg, argv[i++]->m);
      break;
    case LO_TRUE:
      lo_message_add_true(msg);
      break;
    case LO_FALSE:
      lo_message_add_false(msg);
      break;
    case LO_NIL:
      lo_message_add_nil(msg);
      break;
    case LO_INFINITUM:
      lo_message_add_infinitum(msg);
      break;
    default:
      /* ?? */
      break;
    }
  }
  argc = i;
  msg->source = src;
#if HAVE_TIMETAG
  msg->ts = ts;
#endif
  res = pure_tuplel(2, pure_cstring_dup(path), pure_pointer(msg));
  free(host); free(port); free(path); free(_types);
  for (i = 0; i < argc; i++)
    free(argv[i]);
  free(argv);
  return res;
}

pure_expr *Pure_osc_recv(void)
{
  pure_expr *res;
  while ((res = Pure_osc_recv_noblock()) == NULL)
    usleep(10000);
  return res;
}

void Pure_osc_flush(void)
{
  pure_expr *res;
  while ((res = Pure_osc_recv_noblock())) pure_freenew(res);
}

/* Generic error callback. */

void Pure_osc_error(int num, const char *msg, const char *path)
{
  if (path)
    fprintf(stderr, "liblo server error %d: %s (%s)\n", num, msg, path);
  else
    fprintf(stderr, "liblo server error %d: %s\n", num, msg);
}
