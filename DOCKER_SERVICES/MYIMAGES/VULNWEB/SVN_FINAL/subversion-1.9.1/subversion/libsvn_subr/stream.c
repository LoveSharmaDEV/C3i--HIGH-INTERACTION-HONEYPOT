/*
 * stream.c:   svn_stream operations
 *
 * ====================================================================
 *    Licensed to the Apache Software Foundation (ASF) under one
 *    or more contributor license agreements.  See the NOTICE file
 *    distributed with this work for additional information
 *    regarding copyright ownership.  The ASF licenses this file
 *    to you under the Apache License, Version 2.0 (the
 *    "License"); you may not use this file except in compliance
 *    with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing,
 *    software distributed under the License is distributed on an
 *    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *    KIND, either express or implied.  See the License for the
 *    specific language governing permissions and limitations
 *    under the License.
 * ====================================================================
 */

#include <assert.h>
#include <stdio.h>

#include <apr.h>
#include <apr_pools.h>
#include <apr_strings.h>
#include <apr_file_io.h>
#include <apr_errno.h>
#include <apr_poll.h>
#include <apr_portable.h>

#include <zlib.h>

#include "svn_pools.h"
#include "svn_io.h"
#include "svn_error.h"
#include "svn_string.h"
#include "svn_utf.h"
#include "svn_checksum.h"
#include "svn_path.h"
#include "svn_private_config.h"
#include "private/svn_atomic.h"
#include "private/svn_error_private.h"
#include "private/svn_eol_private.h"
#include "private/svn_io_private.h"
#include "private/svn_subr_private.h"
#include "private/svn_utf_private.h"


struct svn_stream_t {
  void *baton;
  svn_read_fn_t read_fn;
  svn_read_fn_t read_full_fn;
  svn_stream_skip_fn_t skip_fn;
  svn_write_fn_t write_fn;
  svn_close_fn_t close_fn;
  svn_stream_mark_fn_t mark_fn;
  svn_stream_seek_fn_t seek_fn;
  svn_stream_data_available_fn_t data_available_fn;
  svn_stream__is_buffered_fn_t is_buffered_fn;
  apr_file_t *file; /* Maybe NULL */
};


/*** Forward declarations. ***/

static svn_error_t *
skip_default_handler(void *baton, apr_size_t len, svn_read_fn_t read_full_fn);


/*** Generic streams. ***/

svn_stream_t *
svn_stream_create(void *baton, apr_pool_t *pool)
{
  svn_stream_t *stream;

  stream = apr_pcalloc(pool, sizeof(*stream));
  stream->baton = baton;
  return stream;
}


void
svn_stream_set_baton(svn_stream_t *stream, void *baton)
{
  stream->baton = baton;
}


void
svn_stream_set_read2(svn_stream_t *stream,
                     svn_read_fn_t read_fn,
                     svn_read_fn_t read_full_fn)
{
  stream->read_fn = read_fn;
  stream->read_full_fn = read_full_fn;
}

void
svn_stream_set_skip(svn_stream_t *stream, svn_stream_skip_fn_t skip_fn)
{
  stream->skip_fn = skip_fn;
}

void
svn_stream_set_write(svn_stream_t *stream, svn_write_fn_t write_fn)
{
  stream->write_fn = write_fn;
}

void
svn_stream_set_close(svn_stream_t *stream, svn_close_fn_t close_fn)
{
  stream->close_fn = close_fn;
}

void
svn_stream_set_mark(svn_stream_t *stream, svn_stream_mark_fn_t mark_fn)
{
  stream->mark_fn = mark_fn;
}

void
svn_stream_set_seek(svn_stream_t *stream, svn_stream_seek_fn_t seek_fn)
{
  stream->seek_fn = seek_fn;
}

void
svn_stream_set_data_available(svn_stream_t *stream,
                              svn_stream_data_available_fn_t data_available_fn)
{
  stream->data_available_fn = data_available_fn;
}

void
svn_stream__set_is_buffered(svn_stream_t *stream,
                            svn_stream__is_buffered_fn_t is_buffered_fn)
{
  stream->is_buffered_fn = is_buffered_fn;
}

/* Standard implementation for svn_stream_read_full() based on
   multiple svn_stream_read2() calls (in separate function to make
   it more likely for svn_stream_read_full to be inlined) */
static svn_error_t *
full_read_fallback(svn_stream_t *stream, char *buffer, apr_size_t *len)
{
  apr_size_t remaining = *len;
  while (remaining > 0)
    {
      apr_size_t length = remaining;
      SVN_ERR(svn_stream_read2(stream, buffer, &length));

      if (length == 0)
        {
          *len -= remaining;
          return SVN_NO_ERROR;
        }

      remaining -= length;
      buffer += length;
    }

  return SVN_NO_ERROR;
}

svn_boolean_t
svn_stream_supports_partial_read(svn_stream_t *stream)
{
  return stream->read_fn != NULL;
}

svn_error_t *
svn_stream_read2(svn_stream_t *stream, char *buffer, apr_size_t *len)
{
  if (stream->read_fn == NULL)
    return svn_error_create(SVN_ERR_STREAM_NOT_SUPPORTED, NULL, NULL);

  return svn_error_trace(stream->read_fn(stream->baton, buffer, len));
}

svn_error_t *
svn_stream_read_full(svn_stream_t *stream, char *buffer, apr_size_t *len)
{
  if (stream->read_full_fn == NULL)
    return svn_error_trace(full_read_fallback(stream, buffer, len));

  return svn_error_trace(stream->read_full_fn(stream->baton, buffer, len));
}

svn_error_t *
svn_stream_skip(svn_stream_t *stream, apr_size_t len)
{
  if (stream->skip_fn == NULL)
    return svn_error_trace(
            skip_default_handler(stream->baton, len, stream->read_full_fn));

  return svn_error_trace(stream->skip_fn(stream->baton, len));
}


svn_error_t *
svn_stream_write(svn_stream_t *stream, const char *data, apr_size_t *len)
{
  if (stream->write_fn == NULL)
    return svn_error_create(SVN_ERR_STREAM_NOT_SUPPORTED, NULL, NULL);

  return svn_error_trace(stream->write_fn(stream->baton, data, len));
}


svn_error_t *
svn_stream_reset(svn_stream_t *stream)
{
  return svn_error_trace(
            svn_stream_seek(stream, NULL));
}

svn_boolean_t
svn_stream_supports_mark(svn_stream_t *stream)
{
  return stream->mark_fn != NULL;
}

svn_error_t *
svn_stream_mark(svn_stream_t *stream, svn_stream_mark_t **mark,
                apr_pool_t *pool)
{
  if (stream->mark_fn == NULL)
    return svn_error_create(SVN_ERR_STREAM_SEEK_NOT_SUPPORTED, NULL, NULL);

  return svn_error_trace(stream->mark_fn(stream->baton, mark, pool));
}

svn_error_t *
svn_stream_seek(svn_stream_t *stream, const svn_stream_mark_t *mark)
{
  if (stream->seek_fn == NULL)
    return svn_error_create(SVN_ERR_STREAM_SEEK_NOT_SUPPORTED, NULL, NULL);

  return svn_error_trace(stream->seek_fn(stream->baton, mark));
}

svn_error_t *
svn_stream_data_available(svn_stream_t *stream,
                          svn_boolean_t *data_available)
{
  if (stream->data_available_fn == NULL)
    return svn_error_create(SVN_ERR_STREAM_NOT_SUPPORTED, NULL, NULL);

  return svn_error_trace(stream->data_available_fn(stream->baton,
                                                   data_available));
}

svn_boolean_t
svn_stream__is_buffered(svn_stream_t *stream)
{
  if (stream->is_buffered_fn == NULL)
    return FALSE;

  return stream->is_buffered_fn(stream->baton);
}

svn_error_t *
svn_stream_close(svn_stream_t *stream)
{
  if (stream->close_fn == NULL)
    return SVN_NO_ERROR;
  return svn_error_trace(stream->close_fn(stream->baton));
}

svn_error_t *
svn_stream_puts(svn_stream_t *stream,
                const char *str)
{
  apr_size_t len;
  len = strlen(str);
  return svn_error_trace(svn_stream_write(stream, str, &len));
}

svn_error_t *
svn_stream_printf(svn_stream_t *stream,
                  apr_pool_t *pool,
                  const char *fmt,
                  ...)
{
  const char *message;
  va_list ap;

  va_start(ap, fmt);
  message = apr_pvsprintf(pool, fmt, ap);
  va_end(ap);

  return svn_error_trace(svn_stream_puts(stream, message));
}


svn_error_t *
svn_stream_printf_from_utf8(svn_stream_t *stream,
                            const char *encoding,
                            apr_pool_t *pool,
                            const char *fmt,
                            ...)
{
  const char *message, *translated;
  va_list ap;

  va_start(ap, fmt);
  message = apr_pvsprintf(pool, fmt, ap);
  va_end(ap);

  SVN_ERR(svn_utf_cstring_from_utf8_ex2(&translated, message, encoding,
                                        pool));

  return svn_error_trace(svn_stream_puts(stream, translated));
}

/* Guts of svn_stream_readline().
 * Returns the line read from STREAM in *STRINGBUF, and indicates
 * end-of-file in *EOF.  If DETECT_EOL is TRUE, the end-of-line indicator
 * is detected automatically and returned in *EOL.
 * If DETECT_EOL is FALSE, *EOL must point to the desired end-of-line
 * indicator.  STRINGBUF is allocated in POOL. */
static svn_error_t *
stream_readline_bytewise(svn_stringbuf_t **stringbuf,
                         svn_boolean_t *eof,
                         const char *eol,
                         svn_stream_t *stream,
                         apr_pool_t *pool)
{
  svn_stringbuf_t *str;
  apr_size_t numbytes;
  const char *match;
  char c;

  /* Since we're reading one character at a time, let's at least
     optimize for the 90% case.  90% of the time, we can avoid the
     stringbuf ever having to realloc() itself if we start it out at
     80 chars.  */
  str = svn_stringbuf_create_ensure(SVN__LINE_CHUNK_SIZE, pool);

  /* Read into STR up to and including the next EOL sequence. */
  match = eol;
  while (*match)
    {
      numbytes = 1;
      SVN_ERR(svn_stream_read_full(stream, &c, &numbytes));
      if (numbytes != 1)
        {
          /* a 'short' read means the stream has run out. */
          *eof = TRUE;
          *stringbuf = str;
          return SVN_NO_ERROR;
        }

      if (c == *match)
        match++;
      else
        match = eol;

      svn_stringbuf_appendbyte(str, c);
    }

  *eof = FALSE;
  svn_stringbuf_chop(str, match - eol);
  *stringbuf = str;

  return SVN_NO_ERROR;
}

static svn_error_t *
stream_readline_chunky(svn_stringbuf_t **stringbuf,
                       svn_boolean_t *eof,
                       const char *eol,
                       svn_stream_t *stream,
                       apr_pool_t *pool)
{
  /* Read larger chunks of data at once into this buffer and scan
   * that for EOL. A good chunk size should be about 80 chars since
   * most text lines will be shorter. However, don't use a much
   * larger value because filling the buffer from the stream takes
   * time as well.
   */
  char buffer[SVN__LINE_CHUNK_SIZE+1];

  /* variables */
  svn_stream_mark_t *mark;
  apr_size_t numbytes;
  const char *eol_pos;
  apr_size_t total_parsed = 0;

  /* invariant for this call */
  const size_t eol_len = strlen(eol);

  /* Remember the line start so this plus the line length will be
   * the position to move to at the end of this function.
   */
  SVN_ERR(svn_stream_mark(stream, &mark, pool));

  /* Read the first chunk. */
  numbytes = SVN__LINE_CHUNK_SIZE;
  SVN_ERR(svn_stream_read_full(stream, buffer, &numbytes));
  buffer[numbytes] = '\0';

  /* Look for the EOL in this first chunk. If we find it, we are done here.
   */
  eol_pos = strstr(buffer, eol);
  if (eol_pos != NULL)
    {
      *stringbuf = svn_stringbuf_ncreate(buffer, eol_pos - buffer, pool);
      total_parsed = eol_pos - buffer + eol_len;
    }
  else if (numbytes < SVN__LINE_CHUNK_SIZE)
    {
      /* We hit EOF but not EOL.
       */
      *stringbuf = svn_stringbuf_ncreate(buffer, numbytes, pool);
      *eof = TRUE;
      return SVN_NO_ERROR;
     }
  else
    {
      /* A larger buffer for the string is needed. */
      svn_stringbuf_t *str;
      str = svn_stringbuf_create_ensure(2*SVN__LINE_CHUNK_SIZE, pool);
      svn_stringbuf_appendbytes(str, buffer, numbytes);
      *stringbuf = str;

      /* Loop reading chunks until an EOL was found. If we hit EOF, fall
       * back to the standard implementation. */
      do
      {
        /* Append the next chunk to the string read so far.
         */
        svn_stringbuf_ensure(str, str->len + SVN__LINE_CHUNK_SIZE);
        numbytes = SVN__LINE_CHUNK_SIZE;
        SVN_ERR(svn_stream_read_full(stream, str->data + str->len, &numbytes));
        str->len += numbytes;
        str->data[str->len] = '\0';

        /* Look for the EOL in the new data plus the last part of the
         * previous chunk because the EOL may span over the boundary
         * between both chunks.
         */
        eol_pos = strstr(str->data + str->len - numbytes - (eol_len-1), eol);

        if ((numbytes < SVN__LINE_CHUNK_SIZE) && (eol_pos == NULL))
        {
          /* We hit EOF instead of EOL. */
          *eof = TRUE;
          return SVN_NO_ERROR;
        }
      }
      while (eol_pos == NULL);

      /* Number of bytes we actually consumed (i.e. line + EOF).
       * We need to "return" the rest to the stream by moving its
       * read pointer.
       */
      total_parsed = eol_pos - str->data + eol_len;

      /* Terminate the string at the EOL postion and return it. */
      str->len = eol_pos - str->data;
      str->data[str->len] = 0;
    }

  /* Move the stream read pointer to the first position behind the EOL.
   */
  SVN_ERR(svn_stream_seek(stream, mark));
  return svn_error_trace(svn_stream_skip(stream, total_parsed));
}

/* Guts of svn_stream_readline().
 * Returns the line read from STREAM in *STRINGBUF, and indicates
 * end-of-file in *EOF.  EOL must point to the desired end-of-line
 * indicator.  STRINGBUF is allocated in POOL. */
static svn_error_t *
stream_readline(svn_stringbuf_t **stringbuf,
                svn_boolean_t *eof,
                const char *eol,
                svn_stream_t *stream,
                apr_pool_t *pool)
{
  *eof = FALSE;

  /* Often, we operate on APR file or string-based streams and know what
   * EOL we are looking for. Optimize that common case.
   */
  if (svn_stream_supports_mark(stream) &&
      svn_stream__is_buffered(stream))
    {
      /* We can efficiently read chunks speculatively and reposition the
       * stream pointer to the end of the line once we found that.
       */
      SVN_ERR(stream_readline_chunky(stringbuf,
                                     eof,
                                     eol,
                                     stream,
                                     pool));
    }
  else
    {
      /* Use the standard byte-byte implementation.
       */
      SVN_ERR(stream_readline_bytewise(stringbuf,
                                       eof,
                                       eol,
                                       stream,
                                       pool));
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_stream_readline(svn_stream_t *stream,
                    svn_stringbuf_t **stringbuf,
                    const char *eol,
                    svn_boolean_t *eof,
                    apr_pool_t *pool)
{
  return svn_error_trace(stream_readline(stringbuf, eof, eol, stream,
                                         pool));
}

svn_error_t *svn_stream_copy3(svn_stream_t *from, svn_stream_t *to,
                              svn_cancel_func_t cancel_func,
                              void *cancel_baton,
                              apr_pool_t *scratch_pool)
{
  char *buf = apr_palloc(scratch_pool, SVN__STREAM_CHUNK_SIZE);
  svn_error_t *err;
  svn_error_t *err2;

  /* Read and write chunks until we get a short read, indicating the
     end of the stream.  (We can't get a short write without an
     associated error.) */
  while (1)
    {
      apr_size_t len = SVN__STREAM_CHUNK_SIZE;

      if (cancel_func)
        {
          err = cancel_func(cancel_baton);
          if (err)
             break;
        }

      err = svn_stream_read_full(from, buf, &len);
      if (err)
         break;

      if (len > 0)
        err = svn_stream_write(to, buf, &len);

      if (err || (len != SVN__STREAM_CHUNK_SIZE))
          break;
    }

  err2 = svn_error_compose_create(svn_stream_close(from),
                                  svn_stream_close(to));

  return svn_error_compose_create(err, err2);
}

svn_error_t *
svn_stream_contents_same2(svn_boolean_t *same,
                          svn_stream_t *stream1,
                          svn_stream_t *stream2,
                          apr_pool_t *pool)
{
  char *buf1 = apr_palloc(pool, SVN__STREAM_CHUNK_SIZE);
  char *buf2 = apr_palloc(pool, SVN__STREAM_CHUNK_SIZE);
  apr_size_t bytes_read1 = SVN__STREAM_CHUNK_SIZE;
  apr_size_t bytes_read2 = SVN__STREAM_CHUNK_SIZE;
  svn_error_t *err = NULL;

  *same = TRUE;  /* assume TRUE, until disproved below */
  while (bytes_read1 == SVN__STREAM_CHUNK_SIZE
         && bytes_read2 == SVN__STREAM_CHUNK_SIZE)
    {
      err = svn_stream_read_full(stream1, buf1, &bytes_read1);
      if (err)
        break;
      err = svn_stream_read_full(stream2, buf2, &bytes_read2);
      if (err)
        break;

      if ((bytes_read1 != bytes_read2)
          || (memcmp(buf1, buf2, bytes_read1)))
        {
          *same = FALSE;
          break;
        }
    }

  return svn_error_compose_create(err,
                                  svn_error_compose_create(
                                    svn_stream_close(stream1),
                                    svn_stream_close(stream2)));
}


/*** Stream implementation utilities ***/

/* Skip data from any stream by reading and simply discarding it. */
static svn_error_t *
skip_default_handler(void *baton, apr_size_t len, svn_read_fn_t read_full_fn)
{
  apr_size_t bytes_read = 1;
  char buffer[4096];
  apr_size_t to_read = len;

  while ((to_read > 0) && (bytes_read > 0))
    {
      bytes_read = sizeof(buffer) < to_read ? sizeof(buffer) : to_read;
      SVN_ERR(read_full_fn(baton, buffer, &bytes_read));
      to_read -= bytes_read;
    }

  return SVN_NO_ERROR;
}



/*** Generic readable empty stream ***/

static svn_error_t *
read_handler_empty(void *baton, char *buffer, apr_size_t *len)
{
  *len = 0;
  return SVN_NO_ERROR;
}

static svn_error_t *
write_handler_empty(void *baton, const char *data, apr_size_t *len)
{
  return SVN_NO_ERROR;
}

static svn_error_t *
mark_handler_empty(void *baton, svn_stream_mark_t **mark, apr_pool_t *pool)
{
  *mark = NULL; /* Seek to start of stream marker */
  return SVN_NO_ERROR;
}

static svn_error_t *
seek_handler_empty(void *baton, const svn_stream_mark_t *mark)
{
  return SVN_NO_ERROR;
}

static svn_boolean_t
is_buffered_handler_empty(void *baton)
{
  return FALSE;
}


svn_stream_t *
svn_stream_empty(apr_pool_t *pool)
{
  svn_stream_t *stream;

  stream = svn_stream_create(NULL, pool);
  svn_stream_set_read2(stream, read_handler_empty, read_handler_empty);
  svn_stream_set_write(stream, write_handler_empty);
  svn_stream_set_mark(stream, mark_handler_empty);
  svn_stream_set_seek(stream, seek_handler_empty);
  svn_stream__set_is_buffered(stream, is_buffered_handler_empty);
  return stream;
}



/*** Stream duplication support ***/
struct baton_tee {
  svn_stream_t *out1;
  svn_stream_t *out2;
};


static svn_error_t *
write_handler_tee(void *baton, const char *data, apr_size_t *len)
{
  struct baton_tee *bt = baton;

  SVN_ERR(svn_stream_write(bt->out1, data, len));
  SVN_ERR(svn_stream_write(bt->out2, data, len));

  return SVN_NO_ERROR;
}


static svn_error_t *
close_handler_tee(void *baton)
{
  struct baton_tee *bt = baton;

  SVN_ERR(svn_stream_close(bt->out1));
  SVN_ERR(svn_stream_close(bt->out2));

  return SVN_NO_ERROR;
}


svn_stream_t *
svn_stream_tee(svn_stream_t *out1,
               svn_stream_t *out2,
               apr_pool_t *pool)
{
  struct baton_tee *baton;
  svn_stream_t *stream;

  if (out1 == NULL)
    return out2;

  if (out2 == NULL)
    return out1;

  baton = apr_palloc(pool, sizeof(*baton));
  baton->out1 = out1;
  baton->out2 = out2;
  stream = svn_stream_create(baton, pool);
  svn_stream_set_write(stream, write_handler_tee);
  svn_stream_set_close(stream, close_handler_tee);

  return stream;
}



/*** Ownership detaching stream ***/

static svn_error_t *
read_handler_disown(void *baton, char *buffer, apr_size_t *len)
{
  return svn_error_trace(svn_stream_read2(baton, buffer, len));
}

static svn_error_t *
read_full_handler_disown(void *baton, char *buffer, apr_size_t *len)
{
  return svn_error_trace(svn_stream_read_full(baton, buffer, len));
}

static svn_error_t *
skip_handler_disown(void *baton, apr_size_t len)
{
  return svn_error_trace(svn_stream_skip(baton, len));
}

static svn_error_t *
write_handler_disown(void *baton, const char *buffer, apr_size_t *len)
{
  return svn_error_trace(svn_stream_write(baton, buffer, len));
}

static svn_error_t *
mark_handler_disown(void *baton, svn_stream_mark_t **mark, apr_pool_t *pool)
{
  return svn_error_trace(svn_stream_mark(baton, mark, pool));
}

static svn_error_t *
seek_handler_disown(void *baton, const svn_stream_mark_t *mark)
{
  return svn_error_trace(svn_stream_seek(baton, mark));
}

static svn_error_t *
data_available_disown(void *baton, svn_boolean_t *data_available)
{
  return svn_error_trace(svn_stream_data_available(baton, data_available));
}

static svn_boolean_t
is_buffered_handler_disown(void *baton)
{
  return svn_stream__is_buffered(baton);
}

svn_stream_t *
svn_stream_disown(svn_stream_t *stream, apr_pool_t *pool)
{
  svn_stream_t *s = svn_stream_create(stream, pool);

  svn_stream_set_read2(s, read_handler_disown, read_full_handler_disown);
  svn_stream_set_skip(s, skip_handler_disown);
  svn_stream_set_write(s, write_handler_disown);
  svn_stream_set_mark(s, mark_handler_disown);
  svn_stream_set_seek(s, seek_handler_disown);
  svn_stream_set_data_available(s, data_available_disown);
  svn_stream__set_is_buffered(s, is_buffered_handler_disown);

  return s;
}



/*** Generic stream for APR files ***/
struct baton_apr {
  apr_file_t *file;
  apr_pool_t *pool;
};

/* svn_stream_mark_t for streams backed by APR files. */
struct mark_apr {
  apr_off_t off;
};

static svn_error_t *
read_handler_apr(void *baton, char *buffer, apr_size_t *len)
{
  struct baton_apr *btn = baton;
  svn_error_t *err;

  if (*len == 1)
    {
      err = svn_io_file_getc(buffer, btn->file, btn->pool);
      if (err)
        {
          *len = 0;
          if (APR_STATUS_IS_EOF(err->apr_err))
            {
              svn_error_clear(err);
              err = SVN_NO_ERROR;
            }
        }
    }
  else
    {
      err = svn_io_file_read(btn->file, buffer, len, btn->pool);
      if (err && APR_STATUS_IS_EOF(err->apr_err))
        {
          svn_error_clear(err);
          err = NULL;
        }
    }

  return svn_error_trace(err);
}

static svn_error_t *
read_full_handler_apr(void *baton, char *buffer, apr_size_t *len)
{
  struct baton_apr *btn = baton;
  svn_error_t *err;
  svn_boolean_t eof;

  if (*len == 1)
    {
      err = svn_io_file_getc(buffer, btn->file, btn->pool);
      if (err)
        {
          *len = 0;
          if (APR_STATUS_IS_EOF(err->apr_err))
            {
              svn_error_clear(err);
              err = SVN_NO_ERROR;
            }
        }
    }
  else
    err = svn_io_file_read_full2(btn->file, buffer, *len, len,
                                 &eof, btn->pool);

  return svn_error_trace(err);
}

static svn_error_t *
skip_handler_apr(void *baton, apr_size_t len)
{
  struct baton_apr *btn = baton;
  apr_off_t offset = len;

  return svn_error_trace(
            svn_io_file_seek(btn->file, APR_CUR, &offset, btn->pool));
}

static svn_error_t *
write_handler_apr(void *baton, const char *data, apr_size_t *len)
{
  struct baton_apr *btn = baton;
  svn_error_t *err;

  if (*len == 1)
    {
      err = svn_io_file_putc(*data, btn->file, btn->pool);
      if (err)
        *len = 0;
    }
  else
    err = svn_io_file_write_full(btn->file, data, *len, len, btn->pool);

  return svn_error_trace(err);
}

static svn_error_t *
close_handler_apr(void *baton)
{
  struct baton_apr *btn = baton;

  return svn_error_trace(svn_io_file_close(btn->file, btn->pool));
}

static svn_error_t *
mark_handler_apr(void *baton, svn_stream_mark_t **mark, apr_pool_t *pool)
{
  struct baton_apr *btn = baton;
  struct mark_apr *mark_apr;

  mark_apr = apr_palloc(pool, sizeof(*mark_apr));
  mark_apr->off = 0;
  SVN_ERR(svn_io_file_seek(btn->file, APR_CUR, &mark_apr->off, btn->pool));
  *mark = (svn_stream_mark_t *)mark_apr;
  return SVN_NO_ERROR;
}

static svn_error_t *
seek_handler_apr(void *baton, const svn_stream_mark_t *mark)
{
  struct baton_apr *btn = baton;
  apr_off_t offset = (mark != NULL) ? ((const struct mark_apr *)mark)->off : 0;

  SVN_ERR(svn_io_file_seek(btn->file, APR_SET, &offset, btn->pool));

  return SVN_NO_ERROR;
}

static svn_error_t *
data_available_handler_apr(void *baton, svn_boolean_t *data_available)
{
  struct baton_apr *btn = baton;
  apr_status_t status;
#if !defined(WIN32) || APR_FILES_AS_SOCKETS
  apr_pollfd_t pfd;
  int n;

  pfd.desc_type = APR_POLL_FILE;
  pfd.desc.f = btn->file;
  pfd.p = btn->pool; /* If we had a scratch pool... Luckily apr doesn't
                        store anything in this pool at this time */
  pfd.reqevents = APR_POLLIN;

  status = apr_poll(&pfd, 1, &n, 0);

  if (status == APR_SUCCESS)
    {
      *data_available = (n > 0);
      return SVN_NO_ERROR;
    }
  else if (APR_STATUS_IS_EOF(status) || APR_STATUS_IS_TIMEUP(status))
    {
      *data_available = FALSE;
      return SVN_NO_ERROR;
    }
  else
    {
      return svn_error_create(SVN_ERR_STREAM_NOT_SUPPORTED,
                              svn_error_wrap_apr(
                                  status,
                                  _("Polling for available data on filestream "
                                    "failed")),
                              NULL);
    }
#else
  HANDLE h;
  DWORD dwAvail;
  status = apr_os_file_get(&h, btn->file);

  if (status)
    return svn_error_wrap_apr(status, NULL);

  if (PeekNamedPipe(h, NULL, 0, NULL, &dwAvail, NULL))
    {
      *data_available = (dwAvail > 0);
      return SVN_NO_ERROR;
    }

  return svn_error_create(SVN_ERR_STREAM_NOT_SUPPORTED,
                          svn_error_wrap_apr(apr_get_os_error(), NULL),
                          _("Windows doesn't support polling on files"));
#endif
}

static svn_boolean_t
is_buffered_handler_apr(void *baton)
{
  struct baton_apr *btn = baton;
  return (apr_file_flags_get(btn->file) & APR_BUFFERED) != 0;
}

svn_error_t *
svn_stream_open_readonly(svn_stream_t **stream,
                         const char *path,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool)
{
  apr_file_t *file;

  SVN_ERR(svn_io_file_open(&file, path, APR_READ | APR_BUFFERED,
                           APR_OS_DEFAULT, result_pool));
  *stream = svn_stream_from_aprfile2(file, FALSE, result_pool);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_stream_open_writable(svn_stream_t **stream,
                         const char *path,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool)
{
  apr_file_t *file;

  SVN_ERR(svn_io_file_open(&file, path,
                           APR_WRITE
                             | APR_BUFFERED
                             | APR_CREATE
                             | APR_EXCL,
                           APR_OS_DEFAULT, result_pool));
  *stream = svn_stream_from_aprfile2(file, FALSE, result_pool);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_stream_open_unique(svn_stream_t **stream,
                       const char **temp_path,
                       const char *dirpath,
                       svn_io_file_del_t delete_when,
                       apr_pool_t *result_pool,
                       apr_pool_t *scratch_pool)
{
  apr_file_t *file;

  SVN_ERR(svn_io_open_unique_file3(&file, temp_path, dirpath,
                                   delete_when, result_pool, scratch_pool));
  *stream = svn_stream_from_aprfile2(file, FALSE, result_pool);

  return SVN_NO_ERROR;
}


svn_stream_t *
svn_stream_from_aprfile2(apr_file_t *file,
                         svn_boolean_t disown,
                         apr_pool_t *pool)
{
  struct baton_apr *baton;
  svn_stream_t *stream;

  if (file == NULL)
    return svn_stream_empty(pool);

  baton = apr_palloc(pool, sizeof(*baton));
  baton->file = file;
  baton->pool = pool;
  stream = svn_stream_create(baton, pool);
  svn_stream_set_read2(stream, read_handler_apr, read_full_handler_apr);
  svn_stream_set_write(stream, write_handler_apr);
  svn_stream_set_skip(stream, skip_handler_apr);
  svn_stream_set_mark(stream, mark_handler_apr);
  svn_stream_set_seek(stream, seek_handler_apr);
  svn_stream_set_data_available(stream, data_available_handler_apr);
  svn_stream__set_is_buffered(stream, is_buffered_handler_apr);
  stream->file = file;

  if (! disown)
    svn_stream_set_close(stream, close_handler_apr);

  return stream;
}

apr_file_t *
svn_stream__aprfile(svn_stream_t *stream)
{
  return stream->file;
}


/* Compressed stream support */

#define ZBUFFER_SIZE 4096       /* The size of the buffer the
                                   compressed stream uses to read from
                                   the substream. Basically an
                                   arbitrary value, picked to be about
                                   page-sized. */

struct zbaton {
  z_stream *in;                 /* compressed stream for reading */
  z_stream *out;                /* compressed stream for writing */
  void *substream;              /* The substream */
  void *read_buffer;            /* buffer   used   for  reading   from
                                   substream */
  int read_flush;               /* what flush mode to use while
                                   reading */
  apr_pool_t *pool;             /* The pool this baton is allocated
                                   on */
};

/* zlib alloc function. opaque is the pool we need. */
static voidpf
zalloc(voidpf opaque, uInt items, uInt size)
{
  apr_pool_t *pool = opaque;

  return apr_palloc(pool, items * size);
}

/* zlib free function */
static void
zfree(voidpf opaque, voidpf address)
{
  /* Empty, since we allocate on the pool */
}

/* Helper function to figure out the sync mode */
static svn_error_t *
read_helper_gz(svn_stream_t *substream,
               char *buffer,
               uInt *len, int *zflush)
{
  uInt orig_len = *len;

  /* There's no reason this value should grow bigger than the range of
     uInt, but Subversion's API requires apr_size_t. */
  apr_size_t apr_len = (apr_size_t) *len;

  SVN_ERR(svn_stream_read_full(substream, buffer, &apr_len));

  /* Type cast back to uInt type that zlib uses.  On LP64 platforms
     apr_size_t will be bigger than uInt. */
  *len = (uInt) apr_len;

  /* I wanted to use Z_FINISH here, but we need to know our buffer is
     big enough */
  *zflush = (*len) < orig_len ? Z_SYNC_FLUSH : Z_SYNC_FLUSH;

  return SVN_NO_ERROR;
}

/* Handle reading from a compressed stream */
static svn_error_t *
read_handler_gz(void *baton, char *buffer, apr_size_t *len)
{
  struct zbaton *btn = baton;
  int zerr;

  if (btn->in == NULL)
    {
      btn->in = apr_palloc(btn->pool, sizeof(z_stream));
      btn->in->zalloc = zalloc;
      btn->in->zfree = zfree;
      btn->in->opaque = btn->pool;
      btn->read_buffer = apr_palloc(btn->pool, ZBUFFER_SIZE);
      btn->in->next_in = btn->read_buffer;
      btn->in->avail_in = ZBUFFER_SIZE;

      SVN_ERR(read_helper_gz(btn->substream, btn->read_buffer,
                             &btn->in->avail_in, &btn->read_flush));

      zerr = inflateInit(btn->in);
      SVN_ERR(svn_error__wrap_zlib(zerr, "inflateInit", btn->in->msg));
    }

  btn->in->next_out = (Bytef *) buffer;
  btn->in->avail_out = (uInt) *len;

  while (btn->in->avail_out > 0)
    {
      if (btn->in->avail_in <= 0)
        {
          btn->in->avail_in = ZBUFFER_SIZE;
          btn->in->next_in = btn->read_buffer;
          SVN_ERR(read_helper_gz(btn->substream, btn->read_buffer,
                                 &btn->in->avail_in, &btn->read_flush));
        }

      /* Short read means underlying stream has run out. */
      if (btn->in->avail_in == 0)
        {
          *len = 0;
          return SVN_NO_ERROR;
        }

      zerr = inflate(btn->in, btn->read_flush);
      if (zerr == Z_STREAM_END)
        break;
      else if (zerr != Z_OK)
        return svn_error_trace(svn_error__wrap_zlib(zerr, "inflate",
                                                    btn->in->msg));
    }

  *len -= btn->in->avail_out;
  return SVN_NO_ERROR;
}

/* Compress data and write it to the substream */
static svn_error_t *
write_handler_gz(void *baton, const char *buffer, apr_size_t *len)
{
  struct zbaton *btn = baton;
  apr_pool_t *subpool;
  void *write_buf;
  apr_size_t buf_size, write_len;
  int zerr;

  if (btn->out == NULL)
    {
      btn->out = apr_palloc(btn->pool, sizeof(z_stream));
      btn->out->zalloc = zalloc;
      btn->out->zfree = zfree;
      btn->out->opaque =  btn->pool;

      zerr = deflateInit(btn->out, Z_DEFAULT_COMPRESSION);
      SVN_ERR(svn_error__wrap_zlib(zerr, "deflateInit", btn->out->msg));
    }

  /* The largest buffer we should need is 0.1% larger than the
     compressed data, + 12 bytes. This info comes from zlib.h.  */
  buf_size = *len + (*len / 1000) + 13;
  subpool = svn_pool_create(btn->pool);
  write_buf = apr_palloc(subpool, buf_size);

  btn->out->next_in = (Bytef *) buffer;  /* Casting away const! */
  btn->out->avail_in = (uInt) *len;

  while (btn->out->avail_in > 0)
    {
      btn->out->next_out = write_buf;
      btn->out->avail_out = (uInt) buf_size;

      zerr = deflate(btn->out, Z_NO_FLUSH);
      SVN_ERR(svn_error__wrap_zlib(zerr, "deflate", btn->out->msg));
      write_len = buf_size - btn->out->avail_out;
      if (write_len > 0)
        SVN_ERR(svn_stream_write(btn->substream, write_buf, &write_len));
    }

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}

/* Handle flushing and closing the stream */
static svn_error_t *
close_handler_gz(void *baton)
{
  struct zbaton *btn = baton;
  int zerr;

  if (btn->in != NULL)
    {
      zerr = inflateEnd(btn->in);
      SVN_ERR(svn_error__wrap_zlib(zerr, "inflateEnd", btn->in->msg));
    }

  if (btn->out != NULL)
    {
      void *buf;
      apr_size_t write_len;

      buf = apr_palloc(btn->pool, ZBUFFER_SIZE);

      while (TRUE)
        {
          btn->out->next_out = buf;
          btn->out->avail_out = ZBUFFER_SIZE;

          zerr = deflate(btn->out, Z_FINISH);
          if (zerr != Z_STREAM_END && zerr != Z_OK)
            return svn_error_trace(svn_error__wrap_zlib(zerr, "deflate",
                                                        btn->out->msg));
          write_len = ZBUFFER_SIZE - btn->out->avail_out;
          if (write_len > 0)
            SVN_ERR(svn_stream_write(btn->substream, buf, &write_len));
          if (zerr == Z_STREAM_END)
            break;
        }

      zerr = deflateEnd(btn->out);
      SVN_ERR(svn_error__wrap_zlib(zerr, "deflateEnd", btn->out->msg));
    }

  return svn_error_trace(svn_stream_close(btn->substream));
}


svn_stream_t *
svn_stream_compressed(svn_stream_t *stream, apr_pool_t *pool)
{
  struct svn_stream_t *zstream;
  struct zbaton *baton;

  assert(stream != NULL);

  baton = apr_palloc(pool, sizeof(*baton));
  baton->in = baton->out = NULL;
  baton->substream = stream;
  baton->pool = pool;
  baton->read_buffer = NULL;
  baton->read_flush = Z_SYNC_FLUSH;

  zstream = svn_stream_create(baton, pool);
  svn_stream_set_read2(zstream, NULL /* only full read support */,
                       read_handler_gz);
  svn_stream_set_write(zstream, write_handler_gz);
  svn_stream_set_close(zstream, close_handler_gz);

  return zstream;
}


/* Checksummed stream support */

struct checksum_stream_baton
{
  svn_checksum_ctx_t *read_ctx, *write_ctx;
  svn_checksum_t **read_checksum;  /* Output value. */
  svn_checksum_t **write_checksum;  /* Output value. */
  svn_stream_t *proxy;

  /* True if more data should be read when closing the stream. */
  svn_boolean_t read_more;

  /* Pool to allocate read buffer and output values from. */
  apr_pool_t *pool;
};

static svn_error_t *
read_handler_checksum(void *baton, char *buffer, apr_size_t *len)
{
  struct checksum_stream_baton *btn = baton;

  SVN_ERR(svn_stream_read2(btn->proxy, buffer, len));

  if (btn->read_checksum)
    SVN_ERR(svn_checksum_update(btn->read_ctx, buffer, *len));

  return SVN_NO_ERROR;
}

static svn_error_t *
read_full_handler_checksum(void *baton, char *buffer, apr_size_t *len)
{
  struct checksum_stream_baton *btn = baton;
  apr_size_t saved_len = *len;

  SVN_ERR(svn_stream_read_full(btn->proxy, buffer, len));

  if (btn->read_checksum)
    SVN_ERR(svn_checksum_update(btn->read_ctx, buffer, *len));

  if (saved_len != *len)
    btn->read_more = FALSE;

  return SVN_NO_ERROR;
}


static svn_error_t *
write_handler_checksum(void *baton, const char *buffer, apr_size_t *len)
{
  struct checksum_stream_baton *btn = baton;

  if (btn->write_checksum && *len > 0)
    SVN_ERR(svn_checksum_update(btn->write_ctx, buffer, *len));

  return svn_error_trace(svn_stream_write(btn->proxy, buffer, len));
}

static svn_error_t *
data_available_handler_checksum(void *baton, svn_boolean_t *data_available)
{
  struct checksum_stream_baton *btn = baton;

  return svn_error_trace(svn_stream_data_available(btn->proxy,
                                                   data_available));
}

static svn_error_t *
close_handler_checksum(void *baton)
{
  struct checksum_stream_baton *btn = baton;

  /* If we're supposed to drain the stream, do so before finalizing the
     checksum. */
  if (btn->read_more)
    {
      char *buf = apr_palloc(btn->pool, SVN__STREAM_CHUNK_SIZE);
      apr_size_t len = SVN__STREAM_CHUNK_SIZE;

      do
        {
          SVN_ERR(read_full_handler_checksum(baton, buf, &len));
        }
      while (btn->read_more);
    }

  if (btn->read_ctx)
    SVN_ERR(svn_checksum_final(btn->read_checksum, btn->read_ctx, btn->pool));

  if (btn->write_ctx)
    SVN_ERR(svn_checksum_final(btn->write_checksum, btn->write_ctx, btn->pool));

  return svn_error_trace(svn_stream_close(btn->proxy));
}


svn_stream_t *
svn_stream_checksummed2(svn_stream_t *stream,
                        svn_checksum_t **read_checksum,
                        svn_checksum_t **write_checksum,
                        svn_checksum_kind_t checksum_kind,
                        svn_boolean_t read_all,
                        apr_pool_t *pool)
{
  svn_stream_t *s;
  struct checksum_stream_baton *baton;

  if (read_checksum == NULL && write_checksum == NULL)
    return stream;

  baton = apr_palloc(pool, sizeof(*baton));
  if (read_checksum)
    baton->read_ctx = svn_checksum_ctx_create(checksum_kind, pool);
  else
    baton->read_ctx = NULL;

  if (write_checksum)
    baton->write_ctx = svn_checksum_ctx_create(checksum_kind, pool);
  else
    baton->write_ctx = NULL;

  baton->read_checksum = read_checksum;
  baton->write_checksum = write_checksum;
  baton->proxy = stream;
  baton->read_more = read_all;
  baton->pool = pool;

  s = svn_stream_create(baton, pool);
  svn_stream_set_read2(s, read_handler_checksum, read_full_handler_checksum);
  svn_stream_set_write(s, write_handler_checksum);
  svn_stream_set_data_available(s, data_available_handler_checksum);
  svn_stream_set_close(s, close_handler_checksum);
  return s;
}

/* Miscellaneous stream functions. */

svn_error_t *
svn_stringbuf_from_stream(svn_stringbuf_t **str,
                          svn_stream_t *stream,
                          apr_size_t len_hint,
                          apr_pool_t *result_pool)
{
#define MIN_READ_SIZE 64

  apr_size_t to_read = 0;
  svn_stringbuf_t *text
    = svn_stringbuf_create_ensure(len_hint ? len_hint : MIN_READ_SIZE,
                                  result_pool);

  do
    {
      to_read = text->blocksize - 1 - text->len;
      SVN_ERR(svn_stream_read_full(stream, text->data + text->len, &to_read));
      text->len += to_read;

      if (to_read && text->blocksize < text->len + MIN_READ_SIZE)
        svn_stringbuf_ensure(text, text->blocksize * 2);
    }
  while (to_read);

  text->data[text->len] = '\0';
  *str = text;

  return SVN_NO_ERROR;
}

struct stringbuf_stream_baton
{
  svn_stringbuf_t *str;
  apr_size_t amt_read;
};

/* svn_stream_mark_t for streams backed by stringbufs. */
struct stringbuf_stream_mark {
    apr_size_t pos;
};

static svn_error_t *
read_handler_stringbuf(void *baton, char *buffer, apr_size_t *len)
{
  struct stringbuf_stream_baton *btn = baton;
  apr_size_t left_to_read = btn->str->len - btn->amt_read;

  *len = (*len > left_to_read) ? left_to_read : *len;
  memcpy(buffer, btn->str->data + btn->amt_read, *len);
  btn->amt_read += *len;
  return SVN_NO_ERROR;
}

static svn_error_t *
skip_handler_stringbuf(void *baton, apr_size_t len)
{
  struct stringbuf_stream_baton *btn = baton;
  apr_size_t left_to_read = btn->str->len - btn->amt_read;

  len = (len > left_to_read) ? left_to_read : len;
  btn->amt_read += len;
  return SVN_NO_ERROR;
}

static svn_error_t *
write_handler_stringbuf(void *baton, const char *data, apr_size_t *len)
{
  struct stringbuf_stream_baton *btn = baton;

  svn_stringbuf_appendbytes(btn->str, data, *len);
  return SVN_NO_ERROR;
}

static svn_error_t *
mark_handler_stringbuf(void *baton, svn_stream_mark_t **mark, apr_pool_t *pool)
{
  struct stringbuf_stream_baton *btn;
  struct stringbuf_stream_mark *stringbuf_stream_mark;

  btn = baton;

  stringbuf_stream_mark = apr_palloc(pool, sizeof(*stringbuf_stream_mark));
  stringbuf_stream_mark->pos = btn->amt_read;
  *mark = (svn_stream_mark_t *)stringbuf_stream_mark;
  return SVN_NO_ERROR;
}

static svn_error_t *
seek_handler_stringbuf(void *baton, const svn_stream_mark_t *mark)
{
  struct stringbuf_stream_baton *btn = baton;

  if (mark != NULL)
    {
      const struct stringbuf_stream_mark *stringbuf_stream_mark;

      stringbuf_stream_mark = (const struct stringbuf_stream_mark *)mark;
      btn->amt_read = stringbuf_stream_mark->pos;
    }
  else
    btn->amt_read = 0;

  return SVN_NO_ERROR;
}

static svn_error_t *
data_available_handler_stringbuf(void *baton, svn_boolean_t *data_available)
{
  struct stringbuf_stream_baton *btn = baton;

  *data_available = ((btn->str->len - btn->amt_read) > 0);
  return SVN_NO_ERROR;
}

static svn_boolean_t
is_buffered_handler_stringbuf(void *baton)
{
  return TRUE;
}

svn_stream_t *
svn_stream_from_stringbuf(svn_stringbuf_t *str,
                          apr_pool_t *pool)
{
  svn_stream_t *stream;
  struct stringbuf_stream_baton *baton;

  if (! str)
    return svn_stream_empty(pool);

  baton = apr_palloc(pool, sizeof(*baton));
  baton->str = str;
  baton->amt_read = 0;
  stream = svn_stream_create(baton, pool);
  svn_stream_set_read2(stream, read_handler_stringbuf, read_handler_stringbuf);
  svn_stream_set_skip(stream, skip_handler_stringbuf);
  svn_stream_set_write(stream, write_handler_stringbuf);
  svn_stream_set_mark(stream, mark_handler_stringbuf);
  svn_stream_set_seek(stream, seek_handler_stringbuf);
  svn_stream_set_data_available(stream, data_available_handler_stringbuf);
  svn_stream__set_is_buffered(stream, is_buffered_handler_stringbuf);
  return stream;
}

struct string_stream_baton
{
  const svn_string_t *str;
  apr_size_t amt_read;
};

/* svn_stream_mark_t for streams backed by stringbufs. */
struct string_stream_mark {
    apr_size_t pos;
};

static svn_error_t *
read_handler_string(void *baton, char *buffer, apr_size_t *len)
{
  struct string_stream_baton *btn = baton;
  apr_size_t left_to_read = btn->str->len - btn->amt_read;

  *len = (*len > left_to_read) ? left_to_read : *len;
  memcpy(buffer, btn->str->data + btn->amt_read, *len);
  btn->amt_read += *len;
  return SVN_NO_ERROR;
}

static svn_error_t *
mark_handler_string(void *baton, svn_stream_mark_t **mark, apr_pool_t *pool)
{
  struct string_stream_baton *btn;
  struct string_stream_mark *marker;

  btn = baton;

  marker = apr_palloc(pool, sizeof(*marker));
  marker->pos = btn->amt_read;
  *mark = (svn_stream_mark_t *)marker;
  return SVN_NO_ERROR;
}

static svn_error_t *
seek_handler_string(void *baton, const svn_stream_mark_t *mark)
{
  struct string_stream_baton *btn = baton;

  if (mark != NULL)
    {
      const struct string_stream_mark *marker;

      marker = (const struct string_stream_mark *)mark;
      btn->amt_read = marker->pos;
    }
  else
    btn->amt_read = 0;

  return SVN_NO_ERROR;
}

static svn_error_t *
skip_handler_string(void *baton, apr_size_t len)
{
  struct string_stream_baton *btn = baton;
  apr_size_t left_to_read = btn->str->len - btn->amt_read;

  len = (len > left_to_read) ? left_to_read : len;
  btn->amt_read += len;
  return SVN_NO_ERROR;
}

static svn_error_t *
data_available_handler_string(void *baton, svn_boolean_t *data_available)
{
  struct string_stream_baton *btn = baton;

  *data_available = ((btn->str->len - btn->amt_read) > 0);
  return SVN_NO_ERROR;
}

static svn_boolean_t
is_buffered_handler_string(void *baton)
{
  return TRUE;
}

svn_stream_t *
svn_stream_from_string(const svn_string_t *str,
                       apr_pool_t *pool)
{
  svn_stream_t *stream;
  struct string_stream_baton *baton;

  if (! str)
    return svn_stream_empty(pool);

  baton = apr_palloc(pool, sizeof(*baton));
  baton->str = str;
  baton->amt_read = 0;
  stream = svn_stream_create(baton, pool);
  svn_stream_set_read2(stream, read_handler_string, read_handler_string);
  svn_stream_set_mark(stream, mark_handler_string);
  svn_stream_set_seek(stream, seek_handler_string);
  svn_stream_set_skip(stream, skip_handler_string);
  svn_stream_set_data_available(stream, data_available_handler_string);
  svn_stream__set_is_buffered(stream, is_buffered_handler_string);
  return stream;
}


svn_error_t *
svn_stream_for_stdin(svn_stream_t **in, apr_pool_t *pool)
{
  apr_file_t *stdin_file;
  apr_status_t apr_err;

  apr_err = apr_file_open_stdin(&stdin_file, pool);
  if (apr_err)
    return svn_error_wrap_apr(apr_err, "Can't open stdin");

  *in = svn_stream_from_aprfile2(stdin_file, TRUE, pool);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_stream_for_stdout(svn_stream_t **out, apr_pool_t *pool)
{
  apr_file_t *stdout_file;
  apr_status_t apr_err;

  apr_err = apr_file_open_stdout(&stdout_file, pool);
  if (apr_err)
    return svn_error_wrap_apr(apr_err, "Can't open stdout");

  *out = svn_stream_from_aprfile2(stdout_file, TRUE, pool);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_stream_for_stderr(svn_stream_t **err, apr_pool_t *pool)
{
  apr_file_t *stderr_file;
  apr_status_t apr_err;

  apr_err = apr_file_open_stderr(&stderr_file, pool);
  if (apr_err)
    return svn_error_wrap_apr(apr_err, "Can't open stderr");

  *err = svn_stream_from_aprfile2(stderr_file, TRUE, pool);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_string_from_stream(svn_string_t **result,
                       svn_stream_t *stream,
                       apr_pool_t *result_pool,
                       apr_pool_t *scratch_pool)
{
  svn_stringbuf_t *work = svn_stringbuf_create_ensure(SVN__STREAM_CHUNK_SIZE,
                                                      result_pool);
  char *buffer = apr_palloc(scratch_pool, SVN__STREAM_CHUNK_SIZE);

  while (1)
    {
      apr_size_t len = SVN__STREAM_CHUNK_SIZE;

      SVN_ERR(svn_stream_read_full(stream, buffer, &len));
      svn_stringbuf_appendbytes(work, buffer, len);

      if (len < SVN__STREAM_CHUNK_SIZE)
        break;
    }

  SVN_ERR(svn_stream_close(stream));

  *result = apr_palloc(result_pool, sizeof(**result));
  (*result)->data = work->data;
  (*result)->len = work->len;

  return SVN_NO_ERROR;
}


/* These are somewhat arbitrary, if we ever get good empirical data as to
   actually valid values, feel free to update them. */
#define BUFFER_BLOCK_SIZE 1024
#define BUFFER_MAX_SIZE 100000

svn_stream_t *
svn_stream_buffered(apr_pool_t *result_pool)
{
  return svn_stream__from_spillbuf(svn_spillbuf__create(BUFFER_BLOCK_SIZE,
                                                        BUFFER_MAX_SIZE,
                                                        result_pool),
                                   result_pool);
}



/*** Lazyopen Streams ***/

/* Custom baton for lazyopen-style wrapper streams. */
typedef struct lazyopen_baton_t {

  /* Callback function and baton for opening the wrapped stream. */
  svn_stream_lazyopen_func_t open_func;
  void *open_baton;

  /* The wrapped stream, or NULL if the stream hasn't yet been
     opened. */
  svn_stream_t *real_stream;
  apr_pool_t *pool;

  /* Whether to open the wrapped stream on a close call. */
  svn_boolean_t open_on_close;

} lazyopen_baton_t;


/* Use B->open_func/baton to create and set B->real_stream iff it
   isn't already set. */
static svn_error_t *
lazyopen_if_unopened(lazyopen_baton_t *b)
{
  if (b->real_stream == NULL)
    {
      svn_stream_t *stream;
      apr_pool_t *scratch_pool = svn_pool_create(b->pool);

      SVN_ERR(b->open_func(&stream, b->open_baton,
                           b->pool, scratch_pool));

      svn_pool_destroy(scratch_pool);

      b->real_stream = stream;
    }

  return SVN_NO_ERROR;
}

/* Implements svn_read_fn_t */
static svn_error_t *
read_handler_lazyopen(void *baton,
                      char *buffer,
                      apr_size_t *len)
{
  lazyopen_baton_t *b = baton;

  SVN_ERR(lazyopen_if_unopened(b));
  SVN_ERR(svn_stream_read2(b->real_stream, buffer, len));

  return SVN_NO_ERROR;
}

/* Implements svn_read_fn_t */
static svn_error_t *
read_full_handler_lazyopen(void *baton,
                      char *buffer,
                      apr_size_t *len)
{
  lazyopen_baton_t *b = baton;

  SVN_ERR(lazyopen_if_unopened(b));
  SVN_ERR(svn_stream_read_full(b->real_stream, buffer, len));

  return SVN_NO_ERROR;
}

/* Implements svn_stream_skip_fn_t */
static svn_error_t *
skip_handler_lazyopen(void *baton,
                      apr_size_t len)
{
  lazyopen_baton_t *b = baton;

  SVN_ERR(lazyopen_if_unopened(b));
  SVN_ERR(svn_stream_skip(b->real_stream, len));

  return SVN_NO_ERROR;
}

/* Implements svn_write_fn_t */
static svn_error_t *
write_handler_lazyopen(void *baton,
                       const char *data,
                       apr_size_t *len)
{
  lazyopen_baton_t *b = baton;

  SVN_ERR(lazyopen_if_unopened(b));
  SVN_ERR(svn_stream_write(b->real_stream, data, len));

  return SVN_NO_ERROR;
}

/* Implements svn_close_fn_t */
static svn_error_t *
close_handler_lazyopen(void *baton)
{
  lazyopen_baton_t *b = baton;

  if (b->open_on_close)
    SVN_ERR(lazyopen_if_unopened(b));
  if (b->real_stream)
    SVN_ERR(svn_stream_close(b->real_stream));

  return SVN_NO_ERROR;
}

/* Implements svn_stream_mark_fn_t */
static svn_error_t *
mark_handler_lazyopen(void *baton,
                      svn_stream_mark_t **mark,
                      apr_pool_t *pool)
{
  lazyopen_baton_t *b = baton;

  SVN_ERR(lazyopen_if_unopened(b));
  SVN_ERR(svn_stream_mark(b->real_stream, mark, pool));

  return SVN_NO_ERROR;
}

/* Implements svn_stream_seek_fn_t */
static svn_error_t *
seek_handler_lazyopen(void *baton,
                      const svn_stream_mark_t *mark)
{
  lazyopen_baton_t *b = baton;

  SVN_ERR(lazyopen_if_unopened(b));
  SVN_ERR(svn_stream_seek(b->real_stream, mark));

  return SVN_NO_ERROR;
}

static svn_error_t *
data_available_handler_lazyopen(void *baton,
                                svn_boolean_t *data_available)
{
  lazyopen_baton_t *b = baton;

  SVN_ERR(lazyopen_if_unopened(b));
  return svn_error_trace(svn_stream_data_available(b->real_stream,
                                                   data_available));
}

/* Implements svn_stream__is_buffered_fn_t */
static svn_boolean_t
is_buffered_lazyopen(void *baton)
{
  lazyopen_baton_t *b = baton;

  /* No lazy open as we cannot handle an open error. */
  if (!b->real_stream)
    return FALSE;

  return svn_stream__is_buffered(b->real_stream);
}

svn_stream_t *
svn_stream_lazyopen_create(svn_stream_lazyopen_func_t open_func,
                           void *open_baton,
                           svn_boolean_t open_on_close,
                           apr_pool_t *result_pool)
{
  lazyopen_baton_t *lob = apr_pcalloc(result_pool, sizeof(*lob));
  svn_stream_t *stream;

  lob->open_func = open_func;
  lob->open_baton = open_baton;
  lob->real_stream = NULL;
  lob->pool = result_pool;
  lob->open_on_close = open_on_close;

  stream = svn_stream_create(lob, result_pool);
  svn_stream_set_read2(stream, read_handler_lazyopen,
                       read_full_handler_lazyopen);
  svn_stream_set_skip(stream, skip_handler_lazyopen);
  svn_stream_set_write(stream, write_handler_lazyopen);
  svn_stream_set_close(stream, close_handler_lazyopen);
  svn_stream_set_mark(stream, mark_handler_lazyopen);
  svn_stream_set_seek(stream, seek_handler_lazyopen);
  svn_stream_set_data_available(stream, data_available_handler_lazyopen);
  svn_stream__set_is_buffered(stream, is_buffered_lazyopen);

  return stream;
}

/* Baton for install streams */
struct install_baton_t
{
  struct baton_apr baton_apr;
  const char *tmp_path;
};

#ifdef WIN32

#if _WIN32_WINNT < 0x600 /* Does the SDK assume Windows Vista+? */
typedef struct _FILE_RENAME_INFO {
  BOOL   ReplaceIfExists;
  HANDLE RootDirectory;
  DWORD  FileNameLength;
  WCHAR  FileName[1];
} FILE_RENAME_INFO, *PFILE_RENAME_INFO;

typedef struct _FILE_DISPOSITION_INFO {
  BOOL DeleteFile;
} FILE_DISPOSITION_INFO, *PFILE_DISPOSITION_INFO;

#define FileRenameInfo 3
#define FileDispositionInfo 4

typedef BOOL (WINAPI *SetFileInformationByHandle_t)(HANDLE hFile,
                                                    int FileInformationClass,
                                                    LPVOID lpFileInformation,
                                                    DWORD dwBufferSize);

static volatile SetFileInformationByHandle_t SetFileInformationByHandle_p = 0;
#define SetFileInformationByHandle (*SetFileInformationByHandle_p)

static volatile svn_atomic_t SetFileInformationByHandle_a = 0;


static svn_error_t *
find_SetFileInformationByHandle(void *baton, apr_pool_t *scratch_pool)
{
  HMODULE kernel32 = GetModuleHandle("Kernel32.dll");

  if (kernel32)
    {
      SetFileInformationByHandle_p =
                    (SetFileInformationByHandle_t)
                      GetProcAddress(kernel32, "SetFileInformationByHandle");
    }

  return SVN_NO_ERROR;
}
#endif /* WIN32 < Vista */

/* Create and open a tempfile in DIRECTORY. Return its handle and path */
static svn_error_t *
create_tempfile(HANDLE *hFile,
                const char **file_path,
                const char *directory,
                apr_pool_t *result_pool,
                apr_pool_t *scratch_pool)
{
  const char *unique_name;
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);
  static svn_atomic_t tempname_counter;
  int baseNr = (GetTickCount() << 11) + 13 * svn_atomic_inc(&tempname_counter)
               + GetCurrentProcessId();
  int i = 0;
  HANDLE h;

  /* Shares common idea with io.c's temp_file_create */

  do
    {
      apr_uint32_t unique_nr;
      WCHAR *w_name;

      /* Generate a number that should be unique for this application and
         usually for the entire computer to reduce the number of cycles
         through this loop. (A bit of calculation is much cheaper than
         disk io) */
      unique_nr = baseNr + 7 * i++;


      svn_pool_clear(iterpool);
      unique_name = svn_dirent_join(directory,
                                    apr_psprintf(iterpool, "svn-%X",
                                                 unique_nr),
                                    iterpool);

      SVN_ERR(svn_io__utf8_to_unicode_longpath(&w_name, unique_name,
                                               iterpool));

      /* Create a completely not-sharable file to avoid indexers, and other
         filesystem watchers locking the file while we are still writing.

         We need DELETE privileges to move the file. */
      h = CreateFileW(w_name, GENERIC_WRITE | DELETE, 0 /* share */,
                      NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

      if (h == INVALID_HANDLE_VALUE)
        {
          apr_status_t status = apr_get_os_error();
          if (i > 1000)
            return svn_error_createf(SVN_ERR_IO_UNIQUE_NAMES_EXHAUSTED,
                           svn_error_wrap_apr(status, NULL),
                           _("Unable to make name in '%s'"),
                           svn_dirent_local_style(directory, scratch_pool));

          if (!APR_STATUS_IS_EEXIST(status) && !APR_STATUS_IS_EACCES(status))
            return svn_error_wrap_apr(status, NULL);
        }
    }
  while (h == INVALID_HANDLE_VALUE);

  *hFile = h;
  *file_path = apr_pstrdup(result_pool, unique_name);
  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}

/* Implements svn_close_fn_t */
static svn_error_t *
install_close(void *baton)
{
  struct install_baton_t *ib = baton;

  /* Flush the data cached in APR, but don't close the file yet */
  SVN_ERR(svn_io_file_flush(ib->baton_apr.file, ib->baton_apr.pool));

  return SVN_NO_ERROR;
}

#endif /* WIN32 */

svn_error_t *
svn_stream__create_for_install(svn_stream_t **install_stream,
                               const char *tmp_abspath,
                               apr_pool_t *result_pool,
                               apr_pool_t *scratch_pool)
{
  apr_file_t *file;
  struct install_baton_t *ib;
  const char *tmp_path;

#ifdef WIN32
  HANDLE hInstall;
  apr_status_t status;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(tmp_abspath));

  SVN_ERR(create_tempfile(&hInstall, &tmp_path, tmp_abspath,
                          scratch_pool, scratch_pool));

  /* Wrap as a standard APR file to allow sharing implementation.

     But do note that some file functions (such as retrieving the name)
     don't work on this wrapper. */
  /* ### Buffered, or not? */
  status = apr_os_file_put(&file, &hInstall,
                           APR_WRITE | APR_BINARY | APR_BUFFERED,
                           result_pool);

  if (status)
    {
      CloseHandle(hInstall);
      return svn_error_wrap_apr(status, NULL);
    }

  tmp_path = svn_dirent_internal_style(tmp_path, result_pool);
#else

  SVN_ERR_ASSERT(svn_dirent_is_absolute(tmp_abspath));

  SVN_ERR(svn_io_open_unique_file3(&file, &tmp_path, tmp_abspath,
                                   svn_io_file_del_none,
                                   result_pool, scratch_pool));
#endif
  *install_stream = svn_stream_from_aprfile2(file, FALSE, result_pool);

  ib = apr_pcalloc(result_pool, sizeof(*ib));
  ib->baton_apr = *(struct baton_apr*)(*install_stream)->baton;

  assert((void*)&ib->baton_apr == (void*)ib); /* baton pointer is the same */

  (*install_stream)->baton = ib;

  ib->tmp_path = tmp_path;

#ifdef WIN32
  /* Don't close the file on stream close; flush instead */
  svn_stream_set_close(*install_stream, install_close);
#else
  /* ### Install pool cleanup handler for tempfile? */
#endif

  return SVN_NO_ERROR;
}

svn_error_t *
svn_stream__install_stream(svn_stream_t *install_stream,
                           const char *final_abspath,
                           svn_boolean_t make_parents,
                           apr_pool_t *scratch_pool)
{
  struct install_baton_t *ib = install_stream->baton;
  svn_error_t *err;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(final_abspath));
#ifdef WIN32

#if _WIN32_WINNT < 0x600
  SVN_ERR(svn_atomic__init_once(&SetFileInformationByHandle_a,
                                find_SetFileInformationByHandle,
                                NULL, scratch_pool));

  if (!SetFileInformationByHandle_p)
    SVN_ERR(svn_io_file_close(ib->baton_apr.file, scratch_pool));
  else
#endif /* WIN32 < Windows Vista */
    {
      WCHAR *w_final_abspath;
      size_t path_len;
      size_t rename_size;
      FILE_RENAME_INFO *rename_info;
      HANDLE hFile;

      apr_os_file_get(&hFile, ib->baton_apr.file);

      SVN_ERR(svn_io__utf8_to_unicode_longpath(&w_final_abspath,
                                               svn_dirent_local_style(
                                                          final_abspath,
                                                          scratch_pool),
                                               scratch_pool));
      path_len = wcslen(w_final_abspath);
      rename_size = sizeof(*rename_info) + sizeof(WCHAR) * path_len;

      /* The rename info struct doesn't need hacks for long paths,
         so no ugly escaping calls here */
      rename_info = apr_pcalloc(scratch_pool, rename_size);
      rename_info->ReplaceIfExists = TRUE;
      rename_info->FileNameLength = path_len;
      memcpy(rename_info->FileName, w_final_abspath, path_len * sizeof(WCHAR));

      if (!SetFileInformationByHandle(hFile, FileRenameInfo, rename_info,
                                      rename_size))
        {
          svn_boolean_t retry = FALSE;
          err = svn_error_wrap_apr(apr_get_os_error(), NULL);

          /* ### rhuijben: I wouldn't be surprised if we later find out that we
                           have to fall back to close+rename on some specific
                           error values here, to support some non standard NAS
                           and filesystem scenarios. */

          if (make_parents && err && APR_STATUS_IS_ENOENT(err->apr_err))
            {
              svn_error_t *err2;

              err2 = svn_io_make_dir_recursively(svn_dirent_dirname(final_abspath,
                                                            scratch_pool),
                                                 scratch_pool);

              if (err2)
                return svn_error_trace(svn_error_compose_create(err, err2));
              else
                svn_error_clear(err);

              retry = TRUE;
              err = NULL;
            }
          else if (err && (APR_STATUS_IS_EACCES(err->apr_err)
                           || APR_STATUS_IS_EEXIST(err->apr_err)))
            {
              svn_error_clear(err);
              retry = TRUE;
              err = NULL;

              /* Set the destination file writable because Windows will not allow
                 us to rename when final_abspath is read-only. */
              SVN_ERR(svn_io_set_file_read_write(final_abspath, TRUE,
                                                 scratch_pool));
            }

          if (retry)
            {
              if (!SetFileInformationByHandle(hFile, FileRenameInfo,
                                              rename_info, rename_size))
                {
                  err = svn_error_wrap_apr(
                                apr_get_os_error(),
                                _("Can't move '%s' to '%s'"),
                                svn_dirent_local_style(ib->tmp_path,
                                                       scratch_pool),
                                svn_dirent_local_style(final_abspath,
                                                       scratch_pool));
                }
            }
        }
      else
        err = NULL;

      return svn_error_compose_create(err,
                                      svn_io_file_close(ib->baton_apr.file,
                                                        scratch_pool));
    }
#endif

  err = svn_io_file_rename(ib->tmp_path, final_abspath, scratch_pool);

  /* A missing directory is too common to not cover here. */
  if (make_parents && err && APR_STATUS_IS_ENOENT(err->apr_err))
    {
      svn_error_t *err2;

      err2 = svn_io_make_dir_recursively(svn_dirent_dirname(final_abspath,
                                                            scratch_pool),
                                         scratch_pool);

      if (err2)
        /* Creating directory didn't work: Return all errors */
        return svn_error_trace(svn_error_compose_create(err, err2));
      else
        /* We could create a directory: retry install */
        svn_error_clear(err);

      SVN_ERR(svn_io_file_rename(ib->tmp_path, final_abspath, scratch_pool));
    }
  else
    SVN_ERR(err);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_stream__install_get_info(apr_finfo_t *finfo,
                             svn_stream_t *install_stream,
                             apr_int32_t wanted,
                             apr_pool_t *scratch_pool)
{
  struct install_baton_t *ib = install_stream->baton;

#ifdef WIN32
  /* On WIN32 the file is still open, so we can obtain the information
     from the handle without race conditions */
  apr_status_t status;

  status = apr_file_info_get(finfo, wanted, ib->baton_apr.file);

  if (status)
    return svn_error_wrap_apr(status, NULL);
#else
  SVN_ERR(svn_io_stat(finfo, ib->tmp_path, wanted, scratch_pool));
#endif

  return SVN_NO_ERROR;
}

svn_error_t *
svn_stream__install_delete(svn_stream_t *install_stream,
                           apr_pool_t *scratch_pool)
{
  struct install_baton_t *ib = install_stream->baton;

#ifdef WIN32
  BOOL done;

#if _WIN32_WINNT < 0x600

  SVN_ERR(svn_atomic__init_once(&SetFileInformationByHandle_a,
                                find_SetFileInformationByHandle,
                                NULL, scratch_pool));

  if (!SetFileInformationByHandle_p)
    done = FALSE;
  else
#endif /* WIN32 < Windows Vista */
    {
      FILE_DISPOSITION_INFO disposition_info;
      HANDLE hFile;

      apr_os_file_get(&hFile, ib->baton_apr.file);

      disposition_info.DeleteFile = TRUE;

      /* Mark the file as delete on close to avoid having to reopen
         the file as part of the delete handling. */
      done = SetFileInformationByHandle(hFile, FileDispositionInfo,
                                        &disposition_info,
                                        sizeof(disposition_info));
    }

   SVN_ERR(svn_io_file_close(ib->baton_apr.file, scratch_pool));

   if (done)
     return SVN_NO_ERROR; /* File is already gone */
#endif

  return svn_error_trace(svn_io_remove_file2(ib->tmp_path, FALSE,
                                             scratch_pool));
}
