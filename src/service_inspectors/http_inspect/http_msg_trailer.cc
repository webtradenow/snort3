//--------------------------------------------------------------------------
// Copyright (C) 2014-2016 Cisco and/or its affiliates. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------
// http_msg_trailer.cc author Tom Peters <thopeter@cisco.com>

#include <string.h>
#include <sys/types.h>
#include <stdio.h>

#include "detection/detection_util.h"

#include "http_enum.h"
#include "http_api.h"
#include "http_msg_trailer.h"

using namespace HttpEnums;

HttpMsgTrailer::HttpMsgTrailer(const uint8_t* buffer, const uint16_t buf_size,
    HttpFlowData* session_data_, SourceId source_id_, bool buf_owner, Flow* flow_,
    const HttpParaList* params_) :
    HttpMsgHeadShared(buffer, buf_size, session_data_, source_id_, buf_owner, flow_, params_)
{
    transaction->set_trailer(this, source_id);
}

void HttpMsgTrailer::update_flow()
{
    session_data->half_reset(source_id);
    session_data->section_type[source_id] = SEC__NOT_COMPUTE;
}

#ifdef REG_TEST
void HttpMsgTrailer::print_section(FILE* output)
{
    HttpMsgSection::print_section_title(output, "trailer");
    HttpMsgHeadShared::print_headers(output);
    get_classic_buffer(HTTP_BUFFER_TRAILER, 0, 0).print(output,
        HttpApi::classic_buffer_names[HTTP_BUFFER_TRAILER-1]);
    get_classic_buffer(HTTP_BUFFER_RAW_TRAILER, 0, 0).print(output,
        HttpApi::classic_buffer_names[HTTP_BUFFER_RAW_TRAILER-1]);
    HttpMsgSection::print_section_wrapup(output);
}
#endif

