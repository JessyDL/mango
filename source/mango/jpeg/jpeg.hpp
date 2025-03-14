/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

//#define MANGO_ENABLE_DEBUG_PRINT

#define JPEG_ENABLE_THREAD
#define JPEG_ENABLE_SIMD
#define JPEG_ENABLE_MODERN_HUFFMAN

#include <vector>
#include <string>
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include <mango/math/math.hpp>

#define JPEG_MAX_BLOCKS_IN_MCU   10  // Maximum # of blocks per MCU in the JPEG specification
#define JPEG_MAX_COMPS_IN_SCAN   4   // JPEG limit on # of components in one scan
#define JPEG_NUM_ARITH_TBLS      16  // Arith-coding tables are numbered 0..15
#define JPEG_DC_STAT_BINS        64  // ...
#define JPEG_AC_STAT_BINS        256 // ...
#define JPEG_HUFF_LOOKUP_BITS    8   // Huffman look-ahead table log2 size
#define JPEG_HUFF_LOOKUP_SIZE    (1 << JPEG_HUFF_LOOKUP_BITS)

#ifdef JPEG_ENABLE_SIMD

    #if defined(MANGO_ENABLE_SSE2)
        #define JPEG_ENABLE_SSE2
    #endif

    #if defined(MANGO_ENABLE_SSE4_1)
        #define JPEG_ENABLE_SSE4
    #endif

    #if defined(MANGO_ENABLE_AVX2)
        #define JPEG_ENABLE_AVX2
    #endif

    #if defined(MANGO_ENABLE_NEON)
        #define JPEG_ENABLE_NEON
    #endif

#endif

namespace mango {
namespace jpeg {

    // ----------------------------------------------------------------------------
    // typedefs
    // ----------------------------------------------------------------------------

    using mango::u8;
    using mango::u16;
    using mango::u32;
    using mango::u64;
    using mango::s16;
    using mango::Memory;
    using mango::Memory;
    using mango::Format;
    using mango::Surface;
	using mango::Stream;
    using mango::ThreadPool;

#ifdef MANGO_CPU_64BIT

    using DataType = u64;
    #define JPEG_REGISTER_BITS 64

#else

    using DataType = u32;
    #define JPEG_REGISTER_BITS 32

#endif

    // supported external data formats (encode from, decode to)
    enum Sample
    {
        JPEG_U8_Y,
        JPEG_U8_BGR,
        JPEG_U8_RGB,
        JPEG_U8_BGRA,
        JPEG_U8_RGBA,
    };

    enum class ColorSpace
    {
        CMYK = 0,
        YCBCR = 1,
        YCCK = 2
    };

    struct SampleFormat
    {
        Sample sample;
        Format format;
    };

    struct QuantTable
    {
        s16* table;  // Quantization table
        int  bits;   // Quantization table precision (8 or 16 bits)
    };

#ifndef JPEG_ENABLE_MODERN_HUFFMAN

    struct HuffTable
    {
        u8  size[17];
        u8  value[256];

        int maxcode[18];
        int valoffset[18+1];
        int lookup[JPEG_HUFF_LOOKUP_SIZE];

        void configure();
    };

#else

    struct HuffTable
    {
        u8 size[17];
        u8 value[256];

        // acceleration tables
        DataType maxcode[18];
        u8* valueAddress[19];
        u8 lookupSize[JPEG_HUFF_LOOKUP_SIZE];
        u8 lookupValue[JPEG_HUFF_LOOKUP_SIZE];

        void configure();
    };

#endif

    struct jpegBuffer
    {
        const u8* ptr;
        const u8* end;
        const u8* nextFF;

        DataType data;
        int remain;

        void restart();
        DataType bytes(int count);

#ifdef MANGO_CPU_64BIT

        // 64 bit register
        void ensure16()
        {
            if (remain < 16)
            {
                remain += 48;
                DataType temp;

                if (ptr + 8 < nextFF)
                {
                    temp = mango::uload64be(ptr) >> 16;
                    ptr += 6;
                }
                else
                {
                    temp = bytes(6);
                }

                data = (data << 48) | temp;
            }
        }

#else

        // 32 bit register
        void ensure16()
        {
            if (remain < 16)
            {
                remain += 16;
                DataType temp;

                if (ptr + 2 < nextFF)
                {
                    temp = mango::uload16be(ptr);
                    ptr += 2;
                }
                else
                {
                    temp = bytes(2);
                }

                data = (data << 16) | temp;
            }
        }

#endif
    };

    struct Huffman
    {
        int last_dc_value[JPEG_MAX_COMPS_IN_SCAN];
        int eob_run;

        void restart();
    };

    struct Arithmetic
    {
        u32 c;
        u32 a;
        int ct;

        int last_dc_value[JPEG_MAX_COMPS_IN_SCAN]; // Last DC coef for each component
        int dc_context[JPEG_MAX_COMPS_IN_SCAN]; // Context index for DC conditioning

        u8 dc_L[JPEG_NUM_ARITH_TBLS]; // L values for DC arith-coding tables
        u8 dc_U[JPEG_NUM_ARITH_TBLS]; // U values for DC arith-coding tables
        u8 ac_K[JPEG_NUM_ARITH_TBLS]; // K values for AC arith-coding tables

        u8 dc_stats[JPEG_NUM_ARITH_TBLS][JPEG_DC_STAT_BINS];
        u8 ac_stats[JPEG_NUM_ARITH_TBLS][JPEG_AC_STAT_BINS];
        u8 fixed_bin[4]; // Statistics bin for coding with fixed probability 0.5

        Arithmetic();
        ~Arithmetic();

        void restart(jpegBuffer& buffer);
    };

    struct Frame
    {
        int compid; // Component identifier
        int Hsf;    // Horizontal sampling factor
        int Vsf;    // Vertical sampling factor
        int Tq;     // Quantization table destination selector
        int offset;
    };

    struct DecodeBlock
    {
        int offset;
        int pred;
        union
        {
            struct
            {
                int dc;
                int ac;
            } index;
            struct
            {
                HuffTable* dc;
                HuffTable* ac;
            } table;
        };
    };

    struct DecodeState
    {
        jpegBuffer buffer;
        Huffman huffman;
        Arithmetic arithmetic;

        DecodeBlock block[JPEG_MAX_BLOCKS_IN_MCU];
        int blocks;
        int comps_in_scan;

        const u8* zigzagTable;

        int spectralStart;
        int spectralEnd;
        int successiveHigh;
        int successiveLow;

        void (*decode)(s16* output, DecodeState* state);
    };

    struct Block
    {
        s16* qt;
    };

    struct ProcessState
    {
        // NOTE: this is just quantization tables
        Block block[JPEG_MAX_BLOCKS_IN_MCU];
        int blocks;

        Frame frame[JPEG_MAX_COMPS_IN_SCAN];
        int frames;
        ColorSpace colorspace;

	    void (*idct) (u8* dest, const s16* data, const s16* qt);

        void (*process            ) (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
        void (*clipped            ) (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
        void (*process_y          ) (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
        void (*process_cmyk       ) (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
        void (*process_ycbcr      ) (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
        void (*process_ycbcr_8x8  ) (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
        void (*process_ycbcr_8x16 ) (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
        void (*process_ycbcr_16x8 ) (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
        void (*process_ycbcr_16x16) (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    };

    // ----------------------------------------------------------------------------
    // Parser
    // ----------------------------------------------------------------------------

    class Parser
    {
    protected:
        QuantTable quantTable[JPEG_MAX_COMPS_IN_SCAN];
        HuffTable huffTable[2][JPEG_MAX_COMPS_IN_SCAN];

        AlignedPointer<s16> quantTableVector;
        s16* blockVector;

        std::vector<Frame> frames;
        Frame* scanFrame; // current Progressive AC scan frame

        DecodeState decodeState;
        ProcessState processState;

        int restartInterval;
        int restartCounter;

        std::string m_encoding;
        std::string m_compression;
        std::string m_idct_name;
        std::string m_ycbcr_name;

        Surface* m_surface;
        u64 cpu_flags;

        int width;  // Image width, does include alignment
        int height; // Image height, does include alignment
        int xsize;  // Image width, does not include alignment
        int ysize;  // Image height, does not include alignment
        int xclip;
        int yclip;
        int precision; // 8 or 12 bits
        int components; // 1..4
        bool is_progressive;
        bool is_multiscan;
        bool is_arithmetic;
        bool is_lossless;
        bool is_differential;
        int Hmax;
        int Vmax;
        int blocks_in_mcu;
        int xblock;
        int yblock;
        int xmcu;
        int ymcu;
        int mcus;

        bool isJPEG(ConstMemory memory) const;

        const u8* stepMarker(const u8* p) const;
        const u8* seekMarker(const u8* p, const u8* end) const;

        void processSOI();
        void processEOI();
        void processCOM(const u8* p);
        void processTEM(const u8* p);
        void processRES(const u8* p);
        void processJPG(const u8* p);
        void processJPG(const u8* p, u16 marker);
        void processAPP(const u8* p, u16 marker);
        void processSOF(const u8* p, u16 marker);
        const u8* processSOS(const u8* p, const u8* end);
        void processDQT(const u8* p);
        void processDNL(const u8* p);
        void processDRI(const u8* p);
        void processDHT(const u8* p);
        void processDAC(const u8* p);
        void processDHP(const u8* p);
        void processEXP(const u8* p);

        void parse(ConstMemory memory, bool decode);

        void restart();
        bool handleRestart();

        void decodeLossless();
        void decodeSequential();
        void decodeSequentialST();
        void decodeSequentialMT();
        void decodeMultiScan();
        void decodeProgressive();
        void finishProgressive();
        void finishProgressiveST();
        void finishProgressiveMT();

        void configureCPU(Sample sample);
        std::string getInfo() const;

    public:
        ImageHeader header;
        ConstMemory exif_memory; // Exif block, if one is present
        ConstMemory scan_memory; // Scan block
        Buffer icc_buffer; // ICC color profile block, if one is present

        Parser(ConstMemory memory);
        ~Parser();

        ImageDecodeStatus decode(Surface& target);
    };

    // ----------------------------------------------------------------------------
    // functions
    // ----------------------------------------------------------------------------

    typedef void (*ProcessFunc)(u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);

    void huff_decode_mcu_lossless   (s16* output, DecodeState* state);
    void huff_decode_mcu            (s16* output, DecodeState* state);
    void huff_decode_dc_first       (s16* output, DecodeState* state);
    void huff_decode_dc_refine      (s16* output, DecodeState* state);
    void huff_decode_ac_first       (s16* output, DecodeState* state);
    void huff_decode_ac_refine      (s16* output, DecodeState* state);

#ifdef MANGO_ENABLE_LICENSE_BSD

    void arith_decode_mcu_lossless  (s16* output, DecodeState* state);
    void arith_decode_mcu           (s16* output, DecodeState* state);
    void arith_decode_dc_first      (s16* output, DecodeState* state);
    void arith_decode_dc_refine     (s16* output, DecodeState* state);
    void arith_decode_ac_first      (s16* output, DecodeState* state);
    void arith_decode_ac_refine     (s16* output, DecodeState* state);

#endif

    void idct8                          (u8* dest, const s16* data, const s16* qt);
    void idct12                         (u8* dest, const s16* data, const s16* qt);

    void process_y_8bit                 (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_y_24bit                (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_y_32bit                (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_cmyk_bgra              (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_8bit             (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_bgr              (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_8x8          (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_8x16         (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_16x8         (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_16x16        (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_rgb              (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_8x8          (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_8x16         (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_16x8         (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_16x16        (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_bgra             (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_8x8         (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_8x16        (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_16x8        (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_16x16       (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_rgba             (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_8x8         (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_8x16        (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_16x8        (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_16x16       (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);

#if defined(JPEG_ENABLE_NEON)

    void idct_neon                      (u8* dest, const s16* data, const s16* qt);

    void process_ycbcr_bgra_8x8_neon    (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_8x16_neon   (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_16x8_neon   (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_16x16_neon  (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_rgba_8x8_neon    (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_8x16_neon   (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_16x8_neon   (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_16x16_neon  (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_bgr_8x8_neon     (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_8x16_neon    (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_16x8_neon    (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_16x16_neon   (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_rgb_8x8_neon     (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_8x16_neon    (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_16x8_neon    (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_16x16_neon   (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);

#endif

#if defined(JPEG_ENABLE_SSE2)

    void idct_sse2                      (u8* dest, const s16* data, const s16* qt);

    void process_ycbcr_bgra_8x8_sse2    (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_8x16_sse2   (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_16x8_sse2   (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgra_16x16_sse2  (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_rgba_8x8_sse2    (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_8x16_sse2   (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_16x8_sse2   (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgba_16x16_sse2  (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);

#endif // JPEG_ENABLE_SSE2

#if defined(JPEG_ENABLE_SSE4)

    void process_ycbcr_bgr_8x8_ssse3    (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_8x16_ssse3   (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_16x8_ssse3   (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_bgr_16x16_ssse3  (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);

    void process_ycbcr_rgb_8x8_ssse3    (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_8x16_ssse3   (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_16x8_ssse3   (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);
    void process_ycbcr_rgb_16x16_ssse3  (u8* dest, int stride, const s16* data, ProcessState* state, int width, int height);

#endif // JPEG_ENABLE_SSE4

    SampleFormat getSampleFormat(const Format& format);
	ImageEncodeStatus encodeImage(Stream& stream, const Surface& surface, float quality);

} // namespace jpeg
} // namespace mango
