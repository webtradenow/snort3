/*
** Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// cd_erspan3_module.cc author Josh Rosenbaum <jrosenba@cisco.com>

#include "codecs/link/cd_erspan3_module.h"


static const Parameter erspan3_params[] =
{
    { nullptr, Parameter::PT_MAX, nullptr, nullptr, nullptr }
};


static const RuleMap erspan3_rules[] =
{
    { DECODE_ERSPAN3_DGRAM_LT_HDR, "(" CD_ERSPAN3_NAME ") captured < ERSpan Type3 Header Length" },
    { 0, nullptr }
};

//-------------------------------------------------------------------------
// rpc module
//-------------------------------------------------------------------------

Erspan3Module::Erspan3Module() : DecodeModule(CD_ERSPAN3_NAME, erspan3_params, erspan3_rules)
{ }

bool Erspan3Module::set(const char*, Value&, SnortConfig*)
{
    return true;
}
