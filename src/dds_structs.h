#pragma once

#include "defines.h"

typedef struct DDSPixelFormat {
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t RGB_bit_count;
    uint32_t R_bit_mask;
    uint32_t G_bit_mask;
    uint32_t B_bit_mask;
    uint32_t A_bit_mask;
} DDSPixelFormat;

typedef struct DDSHeader {
    uint32_t size;
    uint32_t flags;
    uint32_t width;
    uint32_t height;
    uint32_t pitch_or_linear_size;
    uint32_t depth;
    uint32_t mipmap_count;
    uint32_t reserved1[11];
    DDSPixelFormat dds_pf;
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    uint32_t caps4;
    uint32_t reserved2;
} DDSHeader;

typedef struct DDSFile {
    char magic[4];
    DDSHeader header;
    char data_begin;
} DDSFile;