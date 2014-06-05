/****************************************************************************
 *
 * Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
 * Copyright (C) 2003-2013 Sourcefire, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 ****************************************************************************/

#ifndef HI_MAIN_H
#define HI_MAIN_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "decode.h"
#include "stream/stream_api.h"
#include "hi_ui_config.h"
#include "util_utf.h"
#include "detection_util.h"
#include "search_engines/str_search.h"
#include "util_jsnorm.h"
#include "profiler.h"

#include <zlib.h>

extern int16_t hi_app_protocol_id;

extern THREAD_LOCAL DataBuffer HttpDecodeBuf;

#ifdef PERF_PROFILING
extern THREAD_LOCAL PreprocStats hiDetectPerfStats;
extern THREAD_LOCAL int hiDetectCalled;
#endif

#define MAX_METHOD_LEN  256

#define DEFAULT_HTTP_MEMCAP 150994944 /* 144 MB */
#define MIN_HTTP_MEMCAP     2304
#define MAX_HTTP_MEMCAP     603979776 /* 576 MB */
#define MAX_URI_EXTRACTED   2048
#define MAX_HOSTNAME        256

#define DEFAULT_MAX_GZIP_MEM 838860
#define GZIP_MEM_MIN    3276
#define MAX_GZIP_DEPTH    65535
#define DEFAULT_COMP_DEPTH 1460
#define DEFAULT_DECOMP_DEPTH 2920

#define DEFLATE_RAW_WBITS -15
#define DEFLATE_WBITS      15
#define GZIP_WBITS         31

typedef enum _HttpRespCompressType
{
    HTTP_RESP_COMPRESS_TYPE__GZIP     = 0x00000001,
    HTTP_RESP_COMPRESS_TYPE__DEFLATE  = 0x00000002

} _HttpRespCompressType;

typedef struct s_DECOMPRESS_STATE
{
    uint8_t inflate_init;
    int compr_bytes_read;
    int decompr_bytes_read;
    int compr_depth;
    int decompr_depth;
    uint16_t compress_fmt;
    uint8_t decompress_data;
    z_stream d_stream;
    bool deflate_initialized;

} DECOMPRESS_STATE;

typedef struct s_HTTP_RESP_STATE
{
    uint8_t inspect_body;
    uint8_t inspect_reassembled;
    uint8_t last_pkt_contlen;
    uint8_t last_pkt_chunked;
    uint32_t next_seq;
    uint32_t chunk_remainder;
    int data_extracted;
    uint32_t max_seq;
    bool flow_depth_excd;
}HTTP_RESP_STATE;

typedef struct s_HTTP_LOG_STATE
{
    uint32_t uri_bytes;
    uint32_t hostname_bytes;
    uint8_t uri_extracted[MAX_URI_EXTRACTED];
    uint8_t hostname_extracted[MAX_HOSTNAME];
}HTTP_LOG_STATE;

typedef struct _HttpsessionData
{
    uint32_t event_flags;
    HTTP_RESP_STATE resp_state;
    DECOMPRESS_STATE *decomp_state;
    HTTP_LOG_STATE *log_state;
    sfip_t *true_ip;
    decode_utf_state_t utf_state;
    uint8_t log_flags;
    uint8_t cli_small_chunk_count;
    uint8_t srv_small_chunk_count;
    MimeState *mime_ssn;
} HttpsessionData;

class HttpFlowData : public FlowData
{
public:
    HttpFlowData();
    ~HttpFlowData();

    static void init()
    { flow_id = FlowData::get_flow_id(); };

public:
    static unsigned flow_id;
    HttpsessionData session;
};

typedef struct _HISearch
{
    const char *name;
    int   name_len;

} HISearch;

typedef struct _HiSearchToken               
{   
    const char *name;
    int   name_len;
    int   search_id;
} HiSearchToken;

typedef struct _HISearchInfo
{
    int id;
    int index;
    int length;
} HISearchInfo;


#define COPY_URI 1
#define COPY_HOSTNAME 2

#define HTTP_LOG_URI        0x0001
#define HTTP_LOG_HOSTNAME   0x0002
#define HTTP_LOG_GZIP_DATA  0x0004
#define HTTP_LOG_JSNORM_DATA  0x0008

typedef enum _HiSearchIdEnum
{
    HI_JAVASCRIPT = 0,
    HI_LAST
} HiSearchId;

typedef enum _HtmlSearchIdEnum
{
    HTML_JS = 0,
    HTML_EMA,
    HTML_VB,
    HTML_LAST
} HtmlSearchId;

extern void *hi_javascript_search_mpse;
extern void *hi_htmltype_search_mpse;

extern THREAD_LOCAL HISearch hi_js_search[HI_LAST];
extern THREAD_LOCAL HISearch hi_html_search[HTML_LAST];
extern THREAD_LOCAL HISearch *hi_current_search;
extern THREAD_LOCAL HISearchInfo hi_search_info;

void ApplyFlowDepth(HTTPINSPECT_CONF *, Packet *, HttpsessionData *, int, int, uint32_t);
void HttpInspectRegisterXtraDataFuncs();

int HttpInspectMain(HTTPINSPECT_CONF *GlobalConf, Packet *p);
int ProcessGlobalConf(HTTPINSPECT_GLOBAL_CONF *, char *, int);
int PrintGlobalConf(HTTPINSPECT_GLOBAL_CONF *);
int PrintServerConf(HTTPINSPECT_CONF*);
int HttpInspectInitializeGlobalConfig(HTTPINSPECT_GLOBAL_CONF*);
HttpsessionData * SetNewHttpsessionData(Packet *, void *);
void FreeHttpsessionData(void *data);
int GetHttpTrueIP(Flow*, uint8_t **buf, uint32_t *len, uint32_t *type);
int GetHttpGzipData(Flow*, uint8_t **buf, uint32_t *len, uint32_t *type);
int GetHttpJSNormData(Flow*, uint8_t **buf, uint32_t *len, uint32_t *type);
int GetHttpUriData(Flow*, uint8_t **buf, uint32_t *len, uint32_t *type);
int GetHttpHostnameData(Flow*, uint8_t **buf, uint32_t *len, uint32_t *type);
void HI_SearchInit(void);
void HI_SearchFree(void);
int HI_SearchStrFound(void *, void *, int , void *, void *);

static inline void ResetGzipState(DECOMPRESS_STATE *ds)
{
    if (ds == NULL)
        return;

    inflateEnd(&(ds->d_stream));

    ds->inflate_init = 0;
    ds->compr_bytes_read = 0;
    ds->decompr_bytes_read = 0;
    ds->compress_fmt = 0;
    ds->decompress_data = 0;
}

static inline void ResetRespState(HTTP_RESP_STATE *ds)
{
    if (ds == NULL)
        return;
    ds->inspect_body = 0;
    ds->last_pkt_contlen = 0;
    ds->last_pkt_chunked = 0;
    ds->inspect_reassembled = 0;
    ds->next_seq = 0;
    ds->chunk_remainder = 0;
    ds->data_extracted = 0;
    ds->max_seq = 0;
}

static inline int SetLogBuffers(HttpsessionData *hsd)
{
    int iRet = 0;

    if (hsd->log_state == NULL)
    {
        hsd->log_state = (HTTP_LOG_STATE *)calloc(1, sizeof(HTTP_LOG_STATE));

        if ( !hsd->log_state )
            iRet = -1;
    }
    return iRet;
}

static inline void SetHttpDecode(uint16_t altLen)
{
    HttpDecodeBuf.len = altLen;
}


#endif
