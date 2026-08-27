// Copyright (c) 2026 Peter Martienssen
// SPDX-License-Identifier: MIT

#pragma once

struct ImageHeader
{
        unsigned short  width;
        unsigned short  height;
        unsigned short  bytesPerLine;
        unsigned int    imageSize;
        unsigned int    bytesUsed;
        unsigned int    pixelformat;
        unsigned int    sequence;
        unsigned long   timestamp;
        unsigned char   numPlanes;
        unsigned short  shift;
};

struct ControlHeader
{
        unsigned int    id;
        unsigned long   value;
};

struct QueryControlHeader
{
        unsigned int    id;
        unsigned int    type;
        char            name[32];
        int             minimum;
        int             maximum;
        int             step;
        int             default_value;
        int             value;
        unsigned int    flags;
};