/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <mango/core/endian.hpp>
#include <mango/core/cpuinfo.hpp>
#include <mango/core/thread.hpp>
#include "jpeg.hpp"

namespace mango {
namespace jpeg {

    static const u8 g_zigzag_table [] =
    {
         0,  1,  8, 16,  9,  2,  3, 10,
        17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63,
    };

    // ----------------------------------------------------------------------------
    // markers
    // ----------------------------------------------------------------------------

    enum jpegMarker
    {
        // Start of Frame markers, non-differential, Huffman coding
        MARKER_SOF0     = 0xffc0,  // Baseline DCT
        MARKER_SOF1     = 0xffc1,  // Extended sequential DCT
        MARKER_SOF2     = 0xffc2,  // Progressive DCT
        MARKER_SOF3     = 0xffc3,  // Lossless (sequential)

        // Huffman table specification
        MARKER_DHT      = 0xffc4,  // Define Huffman table(s)

        // Start of Frame markers, differential, Huffman coding
        MARKER_SOF5     = 0xffc5,  // Differential sequential DCT
        MARKER_SOF6     = 0xffc6,  // Differential progressive DCT
        MARKER_SOF7     = 0xffc7,  // Differential lossless (sequential)

        // Start of Frame markers, non-differential, arithmetic coding
        MARKER_JPG      = 0xffc8,  // Reserved for JPEG extensions
        MARKER_SOF9     = 0xffc9,  // Extended sequential DCT
        MARKER_SOF10    = 0xffca,  // Progressive DCT
        MARKER_SOF11    = 0xffcb,  // Lossless (sequential)

        // Arithmetic coding conditioning specification
        MARKER_DAC      = 0xffcc,  // Define arithmetic coding conditioning(s)

        // Start of Frame markers, differential, arithmetic coding
        MARKER_SOF13    = 0xffcd,  // Differential sequential DCT
        MARKER_SOF14    = 0xffce,  // Differential progressive DCT
        MARKER_SOF15    = 0xffcf,  // Differential lossless (sequential)

        // Restart interval termination
        MARKER_RST0     = 0xffd0,  // Restart with modulo 8 count 0
        MARKER_RST1     = 0xffd1,  // Restart with modulo 8 count 1
        MARKER_RST2     = 0xffd2,  // Restart with modulo 8 count 2
        MARKER_RST3     = 0xffd3,  // Restart with modulo 8 count 3
        MARKER_RST4     = 0xffd4,  // Restart with modulo 8 count 4
        MARKER_RST5     = 0xffd5,  // Restart with modulo 8 count 5
        MARKER_RST6     = 0xffd6,  // Restart with modulo 8 count 6
        MARKER_RST7     = 0xffd7,  // Restart with modulo 8 count 7

        // Other markers
        MARKER_SOI      = 0xffd8,  // Start of image
        MARKER_EOI      = 0xffd9,  // End of image
        MARKER_SOS      = 0xffda,  // Start of scan
        MARKER_DQT      = 0xffdb,  // Define quantization table(s)
        MARKER_DNL      = 0xffdc,  // Define number of lines
        MARKER_DRI      = 0xffdd,  // Define restart interval
        MARKER_DHP      = 0xffde,  // Define hierarchical progression
        MARKER_EXP      = 0xffdf,  // Expand reference component(s)
        MARKER_APP0     = 0xffe0,  // Reserved for application segments
        MARKER_APP1     = 0xffe1,  // Reserved for application segments
        MARKER_APP2     = 0xffe2,  // Reserved for application segments
        MARKER_APP3     = 0xffe3,  // Reserved for application segments
        MARKER_APP4     = 0xffe4,  // Reserved for application segments
        MARKER_APP5     = 0xffe5,  // Reserved for application segments
        MARKER_APP6     = 0xffe6,  // Reserved for application segments
        MARKER_APP7     = 0xffe7,  // Reserved for application segments
        MARKER_APP8     = 0xffe8,  // Reserved for application segments
        MARKER_APP9     = 0xffe9,  // Reserved for application segments
        MARKER_APP10    = 0xffea,  // Reserved for application segments
        MARKER_APP11    = 0xffeb,  // Reserved for application segments
        MARKER_APP12    = 0xffec,  // Reserved for application segments
        MARKER_APP13    = 0xffed,  // Reserved for application segments
        MARKER_APP14    = 0xffee,  // Reserved for application segments
        MARKER_APP15    = 0xffef,  // Reserved for application segments
        MARKER_JPG0     = 0xfff0,  // Reserved for JPEG extensions
        MARKER_JPG1     = 0xfff1,  // Reserved for JPEG extensions
        MARKER_JPG2     = 0xfff2,  // Reserved for JPEG extensions
        MARKER_JPG3     = 0xfff3,  // Reserved for JPEG extensions
        MARKER_JPG4     = 0xfff4,  // Reserved for JPEG extensions
        MARKER_JPG5     = 0xfff5,  // Reserved for JPEG extensions
        MARKER_JPG6     = 0xfff6,  // Reserved for JPEG extensions
        MARKER_JPG7     = 0xfff7,  // Reserved for JPEG extensions
        MARKER_JPG8     = 0xfff8,  // Reserved for JPEG extensions
        MARKER_JPG9     = 0xfff9,  // Reserved for JPEG extensions
        MARKER_JPG10    = 0xfffa,  // Reserved for JPEG extensions
        MARKER_JPG11    = 0xfffb,  // Reserved for JPEG extensions
        MARKER_JPG12    = 0xfffc,  // Reserved for JPEG extensions
        MARKER_JPG13    = 0xfffd,  // Reserved for JPEG extensions
        MARKER_COM      = 0xfffe,  // Comment

        // Reserved markers
        MARKER_TEM      = 0xff01,  // For temporary private use in arithmetic coding
        MARKER_RES      = 0xff02   // Reserved (0x02 .. 0xbf)
    };

    // ----------------------------------------------------------------------------
    // jpeg_memchr()
    // ----------------------------------------------------------------------------

#if defined(JPEG_ENABLE_SSE2)

    static inline const u8* jpeg_memchr(const u8* p, u8 value, size_t count)
    {
        __m128i ref = _mm_set1_epi8(value);
        while (count >= 16)
        {
            __m128i v = _mm_loadu_si128(reinterpret_cast<__m128i const *>(p));
            u32 mask = _mm_movemask_epi8(_mm_cmpeq_epi8(v, ref));
            if (mask)
            {
                int index = u32_tzcnt(mask);
                for (int i = index; i < 16; ++i)
                {
                    if (p[i] == value)
                        return p + i;
                }
            }
            count -= 16;
            p += 16;
        }
        
        for (size_t i = 0; i < count; ++i)
        {
            if (p[i] == value)
                return p + i;
        }
        
        return p;
    }

#else

    static inline const u8* jpeg_memchr(const u8* p, u8 value, size_t count)
    {
        p = reinterpret_cast<const u8 *>(std::memchr(p, value, count));
        return p ? p : p + count;
    }

#endif

    // ----------------------------------------------------------------------------
    // jpegBuffer
    // ----------------------------------------------------------------------------

    void jpegBuffer::restart()
    {
        data = 0;
        remain = 0;
        nextFF = reinterpret_cast<const u8*>(std::memchr(ptr, 0xff, end - ptr));
    }

    DataType jpegBuffer::bytes(int count)
    {
        DataType temp = 0;

        for (int i = 0; i < count; ++i)
        {
            int a = ptr < end ? *ptr++ : 0;
            if (a == 0xff)
            {
                int b = ptr < end ? *ptr++ : 0;
                if (b)
                {
                    // Found a marker; keep returning zero until it has been processed
                    ptr -= 2;
                    a = 0;
                }
            }
            temp = (temp << 8) | a;
        }

        // When nextFF is NULL that means no 0xff bytes left in the stream which means corruted jpeg
        // because the EOI marker should always be present. NULL would prohibit fast path from running
        // and the slow path would re-scan for nextFF for ALL remaining bytes in the jpeg. This check
        // simply puts the decoder into slow path permanently. Fast path would be nicer but since we already
        // do know the stream is corrupted we want to guard every read against EOF condition.
        if (nextFF)
        {
            nextFF = jpeg_memchr(ptr, 0xff, end - ptr);
        }

        return temp;
    }

    // ----------------------------------------------------------------------------
    // Parser
    // ----------------------------------------------------------------------------

    Parser::Parser(ConstMemory memory)
        : quantTableVector(64 * JPEG_MAX_COMPS_IN_SCAN)
        , blockVector(nullptr)
    {
        restartInterval = 0;
        restartCounter = 0;

        for (int i = 0; i < JPEG_MAX_COMPS_IN_SCAN; ++i)
        {
            quantTable[i].table = quantTableVector.data() + i * 64;
        }

        m_surface = nullptr;

        cpu_flags = getCPUFlags();

        // configure default implementation
        processState.idct = idct8;
        processState.colorspace = ColorSpace::CMYK;

#if defined(JPEG_ENABLE_NEON)
        processState.idct = idct_neon;
        m_idct_name = "NEON iDCT";
#endif

#if defined(JPEG_ENABLE_SSE2)
        if (cpu_flags & CPU_SSE2)
        {
            processState.idct = idct_sse2;
        m_idct_name = "SSE2 iDCT";
        }
#endif

        if (isJPEG(memory))
        {
            parse(memory, false);
        }

        // precision is not known until parse() above is complete
        if (precision == 12)
        {
            // Force 12 bit idct
            // This will round down to 8 bit precision until we have a 12 bit capable color conversion
            processState.idct = idct12;
            m_idct_name = "12 bit iDCT";
        }
    }

    Parser::~Parser()
    {
    }

    bool Parser::isJPEG(ConstMemory memory) const
    {
        if (!memory.address || memory.size < 4)
            return false;

        if (uload16be(memory.address) != MARKER_SOI)
            return false;

#if 0
        // Scan for EOI marker
        const u8* p = memory.address + memory.size - 2;
        for (int i = 0; i < 32; ++i, --p)
        {
            u16 marker = uload16be(p);
            if (marker == MARKER_EOI)
                return true;
        }

        return false;
#else
        // Let's not be so picky.. EOI marker is optional, right?
        // (A lot of JPEG writers think so and we just have to deal with it :)
        return true;
#endif
    }

    const u8* Parser::stepMarker(const u8* p) const
    {
        u16 size = uload16be(p);
        p += size;
        p += (p[1] == 0xff); // HACK: some really ancient jpeg encoders encode markers sometimes as
                             // (0xff, 0xff, ID) ; this will skip to the "correct" 0xff (the second one)
        return p;
    }

    const u8* Parser::seekMarker(const u8* start, const u8* end) const
    {
        const u8* p = start;
        --end; // marker is two bytes: don't look at last byte

        while (p < end)
        {
            p = jpeg_memchr(p, 0xff, end - p);
            if (p[1])
            {
                return p; // found a marker
            }
            p += 2;
        }

        //if (*p != 0xff)
        ++p; // skip last byte (warning! if it is 0xff a marker can be potentially missed)
        debugPrint("  Seek: %d bytes\n", int(p - start));

        return p;
    }

    void Parser::processSOI()
    {
        debugPrint("[ SOI ]\n");
        restartInterval = 0;
    }

    void Parser::processEOI()
    {
        debugPrint("[ EOI ]\n");
    }

    void Parser::processCOM(const u8* p)
    {
        debugPrint("[ COM ]\n");
        MANGO_UNREFERENCED(p);
    }

    void Parser::processTEM(const u8* p)
    {
        debugPrint("[ TEM ]\n");
        MANGO_UNREFERENCED(p);
    }

    void Parser::processRES(const u8* p)
    {
        debugPrint("[ RES ]\n");

        // Reserved for jpeg extensions
        MANGO_UNREFERENCED(p);
    }

    void Parser::processJPG(const u8* p)
    {
        debugPrint("[ JPG ]\n");

        // Reserved for jpeg extensions
        MANGO_UNREFERENCED(p);
    }

    void Parser::processJPG(const u8* p, u16 marker)
    {
        debugPrint("[ JPG%d ]\n", int(marker - MARKER_JPG0));

        // Reserved for jpeg extensions
        MANGO_UNREFERENCED(p);
        MANGO_UNREFERENCED(marker);
    }

    void Parser::processAPP(const u8* p, u16 marker)
    {
        debugPrint("[ APP%d ]\n", int(marker - MARKER_APP0));

        int size = uload16be(p) - 2;
        p += 2;

        switch (marker)
        {
            case MARKER_APP0:
            {
                const u8 magicJFIF[] = { 0x4a, 0x46, 0x49, 0x46, 0 }; // 'JFIF', 0
                const u8 magicJFXX[] = { 0x4a, 0x46, 0x58, 0x58, 0 }; // 'JFXX', 0

                if (!std::memcmp(p, magicJFIF, 5) || !std::memcmp(p, magicJFXX, 5))
                {
                    p += 5;
                    size -= 5;

                    debugPrint("  JFIF: %i bytes\n", size);

                    int version = p[0] * 100 + p[1];
                    int units = p[2]; // 0 - no units, 1 - dots per inch, 2 - dots per cm
                    int Xdensity = (p[3] << 16) | p[4];
                    int Ydensity = (p[5] << 16) | p[6];
                    int Xthumbnail = p[7];
                    int Ythumbnail = p[8];

                    const char* unit_str = "";

                    switch (units)
                    {
                        case 1: unit_str = "dpi"; break;
                        case 2: unit_str = "cpi"; break;
                    }

                    debugPrint("    version: %i\n", version);
                    debugPrint("    density: %i x %i %s\n", Xdensity, Ydensity, unit_str);
                    debugPrint("    thumbnail: %i x %i\n", Xthumbnail, Ythumbnail);

                    // TODO: process thumbnail / store JFIF block
                    MANGO_UNREFERENCED(version);
                    MANGO_UNREFERENCED(Xdensity);
                    MANGO_UNREFERENCED(Ydensity);
                    MANGO_UNREFERENCED(Xthumbnail);
                    MANGO_UNREFERENCED(Ythumbnail);
                    MANGO_UNREFERENCED(unit_str);
                }

                break;
            }

            case MARKER_APP1:
            {
                const u8 magicExif0[] = { 0x45, 0x78, 0x69, 0x66, 0, 0 }; // 'Exif', 0, 0
                const u8 magicExif255[] = { 0x45, 0x78, 0x69, 0x66, 0, 0xff }; // 'Exif', 0, 0xff

                if (!std::memcmp(p, magicExif0, 6) || !std::memcmp(p, magicExif255, 6))
                {
                    p += 6;
                    size -= 6;
                    exif_memory = ConstMemory(p, size);
                    debugPrint("  EXIF: %d bytes\n", size);
                }

                // TODO: detect and support XMP

                break;
            }

            case MARKER_APP2:
            {
                const u8 magicICC[] = { 0x49, 0x43, 0x43, 0x5f, 0x50, 0x52, 0x4f, 0x46, 0x49, 0x4c, 0x45, 0 }; // 'ICC_PROFILE', 0

                if (!std::memcmp(p, magicICC, 12))
                {
                    // skip magic
                    p += 12;
                    size -= 12;

                    // read sequence information
                    u8 sequence_number = p[0];
                    u8 sequence_total = p[1];
                    p += 2;
                    size -= 2;

                    debugPrint("  ICC: %d / %d (%d bytes)\n", sequence_number, sequence_total, size);
                    MANGO_UNREFERENCED(sequence_number);
                    MANGO_UNREFERENCED(sequence_total);

                    // append ICC segment (JPEG markers have a maximum size and are split)
                    icc_buffer.append(p, size);
                }

                break;
            }

            case MARKER_APP3:
            {
                const u8 magicMETA[] = { 0x4d, 0x45, 0x54, 0x41, 0, 0 }; // 'META', 0, 0
                const u8 magicMeta[] = { 0x4d, 0x65, 0x74, 0x61, 0, 0 }; // 'Meta', 0, 0

                if (!std::memcmp(p, magicMETA, 6) || !std::memcmp(p, magicMeta, 6))
                {
                    p += 6;
                    size -= 6;
                    exif_memory = ConstMemory(p, size);
                    debugPrint("  EXIF: %d bytes\n", size);
                }

                break;
            }

            case MARKER_APP14:
            {
                const u8 magicAdobe[] = { 0x41, 0x64, 0x6f, 0x62, 0x65 }; // 'Adobe'
                if (size == 12 && !std::memcmp(p, magicAdobe, 5))
                {
                    u16 version = uload16be(p + 5);
                    //u16 flags0 = uload16be(p + 7);
                    //u16 flags1 = uload16be(p + 9);
                    u8 color_transform = p[11]; // 0 - CMYK, 1 - YCbCr, 2 - YCCK
                    if (color_transform <= 2)
                    {
                        processState.colorspace = ColorSpace(color_transform);
                    }
                    debugPrint("  Version: %d\n", version);
                    debugPrint("  ColorTransform: %d\n", color_transform);
                    MANGO_UNREFERENCED(version);
                    MANGO_UNREFERENCED(color_transform);
                }
                break;
            }
        }
    }

    void Parser::processSOF(const u8* p, u16 marker)
    {
        debugPrint("[ SOF%d ]\n", int(marker - MARKER_SOF0));

        is_progressive = false;
        is_multiscan = false;
        is_arithmetic = false;
        is_lossless = false;
        is_differential = false;

        u16 length = uload16be(p + 0);
        precision = p[2];
        ysize = uload16be(p + 3);
        xsize = uload16be(p + 5);
        components = p[7];
        p += 8;

        debugPrint("  Image: %d x %d x %d\n", xsize, ysize, precision);

        if (xsize <= 0 || ysize <= 0)
        {
            header.setError("Incorrect dimensions (%d x %d)", xsize, ysize);
            return;
        }

        if (components < 1 || components > 4)
        {
            header.setError("Incorrect number of components (%d)", components);
            return;
        }

        is_arithmetic = marker > MARKER_SOF7;
        m_compression = is_arithmetic ? "Arithmetic" : "Huffman";

        const char* encoding = "";
        switch (marker)
        {
            // Huffman
            case MARKER_SOF0:  encoding = "Baseline DCT"; break;
            case MARKER_SOF1:  encoding = "Extended sequential DCT"; break;
            case MARKER_SOF2:  encoding = "Progressive DCT"; break;
            case MARKER_SOF3:  encoding = "Lossless"; break;
            case MARKER_SOF5:  encoding = "Differential sequential DCT"; break;
            case MARKER_SOF6:  encoding = "Differential progressive DCT"; break;
            case MARKER_SOF7:  encoding = "Differential lossless"; break;
            // Arithmetic
            case MARKER_SOF9:  encoding = "Extended sequential DCT"; break;
            case MARKER_SOF10: encoding = "Progressive DCT"; break;
            case MARKER_SOF11: encoding = "Lossless"; break;
            case MARKER_SOF13: encoding = "Differential sequential DCT"; break;
            case MARKER_SOF14: encoding = "Differential progressive DCT"; break;
            case MARKER_SOF15: encoding = "Differential lossless"; break;
        }

        m_encoding = encoding;

        debugPrint("  Encoding: %s\n", m_encoding.c_str());
        debugPrint("  Compression: %s\n", m_compression.c_str());

        switch (marker)
        {
            case MARKER_SOF2:
            case MARKER_SOF6:
            case MARKER_SOF10:
            case MARKER_SOF14:
                is_progressive = true;
                break;
        }

        switch (marker)
        {
            case MARKER_SOF7:
            case MARKER_SOF15:
                is_differential = true;
                // fall-through
            case MARKER_SOF3:
            case MARKER_SOF11:
                is_lossless = true;
                break;
        }

        Hmax = 0;
        Vmax = 0;
        blocks_in_mcu = 0;
        int offset = 0;

        processState.frames = components;

        for (int i = 0; i < components; ++i)
        {
            Frame& frame = processState.frame[i];

            frame.compid = p[0];
            u8 x = p[1];
            frame.Hsf = (x >> 4) & 0xf;
            frame.Vsf = (x >> 0) & 0xf;
            frame.Tq = p[2];
            frame.offset = offset;
            p += 3;

            if (components == 1)
            {
                // Optimization: force block size to 8x8 with grayscale images
                frame.Hsf = 1;
                frame.Vsf = 1;
            }

            Hmax = std::max(Hmax, frame.Hsf);
            Vmax = std::max(Vmax, frame.Vsf);
            blocks_in_mcu += frame.Hsf * frame.Vsf;

            for (int y = 0; y < frame.Vsf; ++y)
            {
                for (int x = 0; x < frame.Hsf; ++x)
                {
                    processState.block[offset].qt = quantTable[frame.Tq].table;
                    ++offset;
                }
            }

            std::string compid_name;
            if (frame.compid >= 32 && frame.compid < 128)
            {
                compid_name = makeString(" (%c)", char(frame.compid));
            }

            debugPrint("  Frame: %d, compid: %d%s, Hsf: %d, Vsf: %d, Tq: %d, offset: %d\n",
                i, frame.compid, compid_name.c_str(), frame.Hsf, frame.Vsf, frame.Tq, frame.offset);

            if (frame.Hsf > 8 || frame.Vsf > 8)
            {
                header.setError("Incorrect frame sampling rate (%d x %d)", frame.Hsf, frame.Vsf);
                return;
            }

            frames.push_back(frame);
        }

        processState.blocks = offset;

        // Compute frame sampling factors against maximum sampling factor,
        // then convert them into power-of-two presentation.
        for (int i = 0; i < components; ++i)
        {
            Frame& frame = processState.frame[i];
            if (!frame.Hsf || !frame.Vsf)
            {
                header.setError("Incorrect sampling factors (%d x %d)", frame.Hsf, frame.Vsf);
                return;
            }
            frame.Hsf = u32_log2(Hmax / frame.Hsf);
            frame.Vsf = u32_log2(Vmax / frame.Vsf);
        }

        xblock = 8 * Hmax;
        yblock = 8 * Vmax;

        if (!xblock || !yblock)
        {
            header.setError("Incorrect dimensions (%d x %d)", xblock, yblock);
            return;
        }

        debugPrint("  Blocks per MCU: %d\n", blocks_in_mcu);
        debugPrint("  MCU size: %d x %d\n", xblock, yblock);

        // Align to next MCU boundary
        int xmask = xblock - 1;
        int ymask = yblock - 1;
        width  = (xsize + xmask) & ~xmask;
        height = (ysize + ymask) & ~ymask;

        // MCU resolution
        xmcu = width  / xblock;
        ymcu = height / yblock;
        mcus = xmcu * ymcu;

        // clipping
        xclip = xsize % xblock;
        yclip = ysize % yblock;

        debugPrint("  %d MCUs (%d x %d) -> (%d x %d)\n", mcus, xmcu, ymcu, xmcu*xblock, ymcu*yblock);
        debugPrint("  Image: %d x %d\n", xsize, ysize);
        debugPrint("  Clip: %d x %d\n", xclip, yclip);

        // configure header
        header.width = xsize;
        header.height = ysize;
        header.format = components > 1 ? Format(FORMAT_B8G8R8A8) : Format(FORMAT_L8);

        MANGO_UNREFERENCED(length);
    }

    const u8* Parser::processSOS(const u8* p, const u8* end)
    {
        debugPrint("[ SOS ]\n");

        u16 length = uload16be(p);
        decodeState.comps_in_scan = p[2]; // Ns
        p += 3;

        if (decodeState.comps_in_scan != processState.frames && !is_progressive)
        {
            is_multiscan = true;
        }

        debugPrint("  components: %i%s\n", 
            decodeState.comps_in_scan, is_multiscan ? " (MultiScan)" : "");
        MANGO_UNREFERENCED(length);

        decodeState.blocks = 0;

        for (int i = 0; i < decodeState.comps_in_scan; ++i)
        {
            u8 cs = p[0]; // Scan component selector
            u8 x = p[1];
            p += 2;
            int dc = (x >> 4) & 0xf; // DC entropy coding table destination selector
            int ac = (x >> 0) & 0xf; // AC entropy coding table destination selector

            int compid = cs; // ...

            // find frame
            Frame* frame = nullptr;
            int frameIndex = 0;

            for (int j = 0; j < int(frames.size()); ++j)
            {
                if (frames[j].compid == compid)
                {
                    frameIndex = j;
                    frame = &frames[j];
                }
            }

            scanFrame = frame;

            const int size = frame->Hsf * frame->Vsf;
            int offset = frame->offset;

            std::string cs_name;
            if (cs >= 32 && cs < 128)
            {
                cs_name = makeString(" (%c)", char(cs));
            }

            debugPrint("  Component: %i%s, DC: %i, AC: %i, offset: %d, size: %d\n",
                cs, cs_name.c_str(), dc, ac, frame->offset, size);

            for (int j = 0; j < size; ++j)
            {
                DecodeBlock& block = decodeState.block[decodeState.blocks];

                if (is_arithmetic)
                {
                    block.offset = offset * 64;
                    block.pred = frameIndex;
                    block.index.dc = dc;
                    block.index.ac = ac;
                }
                else
                {
                    block.offset = offset * 64;
                    block.pred = frameIndex;
                    block.table.dc = &huffTable[0][dc];
                    block.table.ac = &huffTable[1][ac];
                }

                debugPrint("      - offset: %d, pred: %d,\n", offset * 64, frameIndex);
                ++offset;
                ++decodeState.blocks;
            }
        }

        int Ss = p[0];
        int Se = p[1];

        u8 x = p[2];
        int Al = (x >> 0) & 0xf;
        int Ah = (x >> 4) & 0xf;
        p += 3;

        decodeState.spectralStart = Ss;
        decodeState.spectralEnd = Se;
        decodeState.successiveLow = Al;
        decodeState.successiveHigh = Ah;

        debugPrint("  Spectral range: (%d, %d)\n", Ss, Se);

        bool dc_scan = (decodeState.spectralStart == 0);
        bool refine_scan = (decodeState.successiveHigh != 0);

        decodeState.zigzagTable = g_zigzag_table;

        restartCounter = restartInterval;

        if (is_arithmetic)
        {
#ifdef MANGO_ENABLE_LICENSE_BSD
            Arithmetic& arithmetic = decodeState.arithmetic;

            decodeState.buffer.ptr = p;
            decodeState.buffer.end = end;

            // restart
            decodeState.buffer.restart();

            arithmetic.restart(decodeState.buffer);

            if (is_lossless)
            {
                decodeLossless();
            }
            else if (is_multiscan)
            {
                decodeState.decode = arith_decode_mcu;
                decodeMultiScan();
            }
            else if (is_progressive)
            {
                if (dc_scan)
                {
                    if (!refine_scan)
                    {
                        debugPrint("  * decode_dc_first()\n");
                        decodeState.decode = arith_decode_dc_first;
                    }
                    else
                    {
                        debugPrint("  * decode_dc_refine()\n");
                        decodeState.decode = arith_decode_dc_refine;
                    }
                }
                else
                {
                    if (!refine_scan)
                    {
                        debugPrint("  * decode_ac_first()\n");
                        decodeState.decode = arith_decode_ac_first;
                    }
                    else
                    {
                        debugPrint("  * decode_ac_refine()\n");
                        decodeState.decode = arith_decode_ac_refine;
                    }
                }
                decodeProgressive();
            }
            else
            {
                decodeState.decode = arith_decode_mcu;
                decodeSequential();
            }
#endif // MANGO_ENABLE_LICENSE_BSD
        }
        else
        {
            Huffman& huffman = decodeState.huffman;

            decodeState.buffer.ptr = p;
            decodeState.buffer.end = end;

            // restart
            decodeState.buffer.restart();

            huffman.restart();

            if (is_lossless)
            {
                decodeLossless();
            }
            else if (is_multiscan)
            {
                decodeState.decode = huff_decode_mcu;
                decodeMultiScan();
            }
            else if (is_progressive)
            {
                if (dc_scan)
                {
                    if (!refine_scan)
                    {
                        debugPrint("  * decode_dc_first()\n");
                        decodeState.decode = huff_decode_dc_first;
                    }
                    else
                    {
                        debugPrint("  * decode_dc_refine()\n");
                        decodeState.decode = huff_decode_dc_refine;
                    }
                }
                else
                {
                    if (!refine_scan)
                    {
                        debugPrint("  * decode_ac_first()\n");
                        decodeState.decode = huff_decode_ac_first;
                    }
                    else
                    {
                        debugPrint("  * decode_ac_refine()\n");
                        decodeState.decode = huff_decode_ac_refine;
                    }
                }
                decodeProgressive();
            }
            else
            {
                decodeState.decode = huff_decode_mcu;
                decodeSequential();
            }
        }

        // TODO: we should sync here since the decoder has prefetched more bytes that it could consume
        p = decodeState.buffer.ptr;
        p -= 8; // hack

        return p;
    }

    void Parser::processDQT(const u8* p)
    {
        debugPrint("[ DQT ]\n");

        u16 Lq = uload16be(p); // Quantization table definition length
        p += 2;
        Lq -= 2;

        for (; Lq > 0;)
        {
            u8 x = *p++;
            u8 Pq = (x >> 4) & 0xf; // Quantization table element precision (0 = 8 bits, 1 = 16 bits)
            u8 Tq = (x >> 0) & 0xf; // Quantization table destination identifier (0..3)

            const int bits = (Pq + 1) * 8;
            debugPrint("  Quantization table #%i element precision: %i bits\n", Tq, bits);

            if (Tq >= JPEG_MAX_COMPS_IN_SCAN)
            {
                header.setError("Incorrect quantization table (Tq: %d >= %d)", Tq, JPEG_MAX_COMPS_IN_SCAN);
                return;
            }

            QuantTable& table = quantTable[Tq];
            table.bits = bits;

            switch (Pq)
            {
                case 0:
                    for (int i = 0; i < 64; ++i)
                    {
                        table.table[g_zigzag_table[i]] = *p++;
                    }
                    break;

                case 1:
                    for (int i = 0; i < 64; ++i)
                    {
                        table.table[g_zigzag_table[i]] = uload16be(p);
                        p += 2;
                    }
                    break;

                default:
                    header.setError("Incorrect quantization table element precision (%d)", Pq);
                    return;
            }

            Lq -= (1 + (Pq + 1) * 64);
        }
    }

    void Parser::processDHT(const u8* p)
    {
        debugPrint("[ DHT ]\n");

        int Lh = uload16be(p); // Huffman table definition length
        p += 2;
        Lh -= 2;

        for (; Lh > 0;)
        {
            u8 x = *p++;
            u8 Tc = (x >> 4) & 0xf; // Table class - 0 = DC table or lossless table, 1 = AC table.
            u8 Th = (x >> 0) & 0xf; // Huffman table identifier

            if (Tc >= 2)
            {
                header.setError("Incorrect huffman table class (%d)", Tc);
                return;
            }

            if (Th >= JPEG_MAX_COMPS_IN_SCAN)
            {
                header.setError("Incorrect huffman table identifier (%d)", Th);
                return;
            }

            HuffTable& table = huffTable[Tc][Th];

            debugPrint("  Huffman table #%d table class: %d\n", Th, Tc);
            debugPrint("    codes: ");

            int count = 0;

            for (int i = 1; i <= 16; ++i)
            {
                u8 L = *p++; // Number of Huffman codes of length i bits
                table.size[i] = L;
                count += L;
                debugPrint("%i ", L);
            }

            Lh -= 17;
            debugPrint("\n");

            for (int i = 0; i < count; ++i)
            {
                table.value[i] = *p++;
            }

            Lh -= count;
            table.configure();
        }
    }

    void Parser::processDAC(const u8* p)
    {
        debugPrint("[ DAC ]\n");

        u16 La = uload16be(p); // Arithmetic coding conditioning definition length
        p += 2;

        int n = (La - 2) / 2;

        debugPrint("  n: %i\n", n);

        for (int i = 0; i < n; ++i)
        {
            u8 x = p[0];
            u8 Tc = (x >> 4) & 0xf; // Table class - 0 = DC table or lossless table, 1 = AC table
            u8 Tb = (x >> 0) & 0xf; // Arithmetic coding conditioning table destination identifier
            u8 Cs = p[1]; // Conditioning table value
            p += 2;

            switch (Tc)
            {
                case 0:
                    // DC table
                    decodeState.arithmetic.dc_L[Tb] = (Cs & 0xf);
                    decodeState.arithmetic.dc_U[Tb] = (Cs >> 4);
                    break;

                case 1:
                    // AC table
                    decodeState.arithmetic.ac_K[Tb] = Cs;
                    break;
            }

            debugPrint("  Tc: %i, Tb: %i, Cs: %i\n", Tc, Tb, Cs);
        }
    }

    void Parser::processDNL(const u8* p)
    {
        debugPrint("[ DNL ]\n");

        u16 Ld = uload16be(p + 0); // Define number of lines segment length
        u16 NL = uload16be(p + 2); // Number of lines
        MANGO_UNREFERENCED(NL); // TODO: ysize = NL
        MANGO_UNREFERENCED(Ld);
    }

    void Parser::processDRI(const u8* p)
    {
        debugPrint("[ DRI ]\n");

        int Lh = uload16be(p + 0); // length
        if (Lh != 4)
        {
            // signal error
        }

        restartInterval = uload16be(p + 2); // number of MCU in restart interval
        debugPrint("  Restart interval: %i\n", restartInterval);
    }

    void Parser::processDHP(const u8* p)
    {
        debugPrint("[ DHP ]\n");

        // TODO: "Define Hierarchical Progression" marker
        MANGO_UNREFERENCED(p);
    }

    void Parser::processEXP(const u8* p)
    {
        debugPrint("[ EXP ]\n");

        u16 Le = uload16be(p); // Expand reference components segment length
        u8 x = p[2];
        u8 Eh = (x >> 4) & 0xf; // Expand horizontally
        u8 Ev = (x >> 0) & 0xf; // Expand vertically

        // Unsupported marker
        MANGO_UNREFERENCED(Le);
        MANGO_UNREFERENCED(Eh);
        MANGO_UNREFERENCED(Ev);
    }

    void Parser::parse(ConstMemory memory, bool decode)
    {
        const u8* end = memory.address + memory.size;
        const u8* p = memory.address;

        for ( ; p < end; )
        {
            if (!header.success)
            {
                // we are in error state -> abort parsing
                break;
            }

            u16 marker = uload16be(p);
            p += 2;

            u64 time0 = Time::us();

            switch (marker)
            {
                case MARKER_SOI:
                    processSOI();
                    break;

                case MARKER_EOI:
                    processEOI();
                    p = end; // terminate parsing
                    break;

                case MARKER_DHT:
                    processDHT(p);
                    p = stepMarker(p);
                    break;

                case MARKER_DAC:
                    processDAC(p);
                    p = stepMarker(p);
                    break;

                case MARKER_DQT:
                    processDQT(p);
                    p = stepMarker(p);
                    break;

                case MARKER_DNL:
                    processDNL(p);
                    p = stepMarker(p);
                    break;

                case MARKER_DRI:
                    processDRI(p);
                    p = stepMarker(p);
                    break;

                case MARKER_DHP:
                    processDHP(p);
                    p = stepMarker(p);
                    break;

                case MARKER_EXP:
                    processEXP(p);
                    p = stepMarker(p);
                    break;

                case MARKER_COM:
                    processCOM(p);
                    p = stepMarker(p);
                    break;

                case MARKER_TEM:
                    processTEM(p);
                    p = stepMarker(p);
                    break;

                case MARKER_RES:
                    processRES(p);
                    p = stepMarker(p);
                    break;

                case MARKER_APP0:
                case MARKER_APP1:
                case MARKER_APP2:
                case MARKER_APP3:
                case MARKER_APP4:
                case MARKER_APP5:
                case MARKER_APP6:
                case MARKER_APP7:
                case MARKER_APP8:
                case MARKER_APP9:
                case MARKER_APP10:
                case MARKER_APP11:
                case MARKER_APP12:
                case MARKER_APP13:
                case MARKER_APP14:
                case MARKER_APP15:
                    processAPP(p, marker);
                    p = stepMarker(p);
                    break;

                case MARKER_JPG:
                    processJPG(p);
                    p = stepMarker(p);
                    break;

                case MARKER_JPG0:
                case MARKER_JPG1:
                case MARKER_JPG2:
                case MARKER_JPG3:
                case MARKER_JPG4:
                case MARKER_JPG5:
                case MARKER_JPG6:
                case MARKER_JPG7:
                case MARKER_JPG8:
                case MARKER_JPG9:
                case MARKER_JPG10:
                case MARKER_JPG11:
                case MARKER_JPG12:
                case MARKER_JPG13:
                    processJPG(p, marker);
                    p = stepMarker(p);
                    break;

                case MARKER_SOF0:
                case MARKER_SOF1:
                case MARKER_SOF2:
                case MARKER_SOF3:
                case MARKER_SOF5:
                case MARKER_SOF6:
                case MARKER_SOF7:
                case MARKER_SOF9:
                case MARKER_SOF10:
                case MARKER_SOF11:
                case MARKER_SOF13:
                case MARKER_SOF14:
                case MARKER_SOF15:
                    processSOF(p, marker);
                    p = stepMarker(p);
                    if (!decode)
                    {
                        // parse header mode (no decoding)
                        scan_memory = ConstMemory(p, end - p);
                        p = end; // terminate parsing
                    }
                    break;

                case MARKER_RST0:
                case MARKER_RST1:
                case MARKER_RST2:
                case MARKER_RST3:
                case MARKER_RST4:
                case MARKER_RST5:
                case MARKER_RST6:
                case MARKER_RST7:
                    break;

                case MARKER_SOS:
                    if (decode)
                    {
                        p = processSOS(p, end);
                    }
                    break;

                default:
                    debugPrint("[ 0x%x ]\n", marker);
                    p = seekMarker(p, end);
                    break;
            }

            u64 time1 = Time::us();
            debugPrint("  Time: %d us\n\n", int(time1 - time0));

            MANGO_UNREFERENCED(time0);
            MANGO_UNREFERENCED(time1);
        }
    }

    inline bool isRestartMarker(const u8* p)
    {
        // TODO: clean up this hack
        bool is = false;
        if (p[0] == 0xff)
        {
            int index = p[1] - 0xd0;
            is = index >= 0 && index <= 7;
        }
        return is;
    }

    void Parser::restart()
    {
        if (is_arithmetic)
        {
            decodeState.arithmetic.restart(decodeState.buffer);
        }
        else
        {
            decodeState.huffman.restart();
        }

        // restart
        decodeState.buffer.restart();
    }

    bool Parser::handleRestart()
    {
        if (restartInterval > 0 && !--restartCounter)
        {
            restartCounter = restartInterval;

            if (isRestartMarker(decodeState.buffer.ptr))
            {
                restart();
                decodeState.buffer.ptr += 2;
                return true;
            }
        }

        return false;
    }

    void Parser::configureCPU(Sample sample)
    {
        const char* simd = "";

        // configure default implementation
        switch (sample)
        {
            case JPEG_U8_Y:
                processState.process_y           = process_y_8bit;
                processState.process_ycbcr       = process_ycbcr_8bit;
                processState.process_ycbcr_8x8   = nullptr;
                processState.process_ycbcr_8x16  = nullptr;
                processState.process_ycbcr_16x8  = nullptr;
                processState.process_ycbcr_16x16 = nullptr;
                break;
            case JPEG_U8_BGR:
                processState.process_y           = process_y_24bit;
                processState.process_ycbcr       = process_ycbcr_bgr;
                processState.process_ycbcr_8x8   = process_ycbcr_bgr_8x8;
                processState.process_ycbcr_8x16  = process_ycbcr_bgr_8x16;
                processState.process_ycbcr_16x8  = process_ycbcr_bgr_16x8;
                processState.process_ycbcr_16x16 = process_ycbcr_bgr_16x16;
                break;
            case JPEG_U8_RGB:
                processState.process_y           = process_y_24bit;
                processState.process_ycbcr       = process_ycbcr_rgb;
                processState.process_ycbcr_8x8   = process_ycbcr_rgb_8x8;
                processState.process_ycbcr_8x16  = process_ycbcr_rgb_8x16;
                processState.process_ycbcr_16x8  = process_ycbcr_rgb_16x8;
                processState.process_ycbcr_16x16 = process_ycbcr_rgb_16x16;
                break;
            case JPEG_U8_BGRA:
                processState.process_y           = process_y_32bit;
                processState.process_ycbcr       = process_ycbcr_bgra;
                processState.process_ycbcr_8x8   = process_ycbcr_bgra_8x8;
                processState.process_ycbcr_8x16  = process_ycbcr_bgra_8x16;
                processState.process_ycbcr_16x8  = process_ycbcr_bgra_16x8;
                processState.process_ycbcr_16x16 = process_ycbcr_bgra_16x16;
                break;
            case JPEG_U8_RGBA:
                processState.process_y           = process_y_32bit;
                processState.process_ycbcr       = process_ycbcr_rgba;
                processState.process_ycbcr_8x8   = process_ycbcr_rgba_8x8;
                processState.process_ycbcr_8x16  = process_ycbcr_rgba_8x16;
                processState.process_ycbcr_16x8  = process_ycbcr_rgba_16x8;
                processState.process_ycbcr_16x16 = process_ycbcr_rgba_16x16;
                break;
        }

        // CMYK / YCCK
        processState.process_cmyk = process_cmyk_bgra;

#if defined(JPEG_ENABLE_NEON)
        // NEON is built-in; no runtime check in this version
        switch (sample)
        {
            case JPEG_U8_Y:
                break;
            case JPEG_U8_BGR:
                processState.process_ycbcr_8x8   = process_ycbcr_bgr_8x8_neon;
                processState.process_ycbcr_8x16  = process_ycbcr_bgr_8x16_neon;
                processState.process_ycbcr_16x8  = process_ycbcr_bgr_16x8_neon;
                processState.process_ycbcr_16x16 = process_ycbcr_bgr_16x16_neon;
                simd = "NEON";
                break;
            case JPEG_U8_RGB:
                processState.process_ycbcr_8x8   = process_ycbcr_rgb_8x8_neon;
                processState.process_ycbcr_8x16  = process_ycbcr_rgb_8x16_neon;
                processState.process_ycbcr_16x8  = process_ycbcr_rgb_16x8_neon;
                processState.process_ycbcr_16x16 = process_ycbcr_rgb_16x16_neon;
                simd = "NEON";
                break;
            case JPEG_U8_BGRA:
                processState.process_ycbcr_8x8   = process_ycbcr_bgra_8x8_neon;
                processState.process_ycbcr_8x16  = process_ycbcr_bgra_8x16_neon;
                processState.process_ycbcr_16x8  = process_ycbcr_bgra_16x8_neon;
                processState.process_ycbcr_16x16 = process_ycbcr_bgra_16x16_neon;
                simd = "NEON";
                break;
            case JPEG_U8_RGBA:
                processState.process_ycbcr_8x8   = process_ycbcr_rgba_8x8_neon;
                processState.process_ycbcr_8x16  = process_ycbcr_rgba_8x16_neon;
                processState.process_ycbcr_16x8  = process_ycbcr_rgba_16x8_neon;
                processState.process_ycbcr_16x16 = process_ycbcr_rgba_16x16_neon;
                simd = "NEON";
                break;
        }
#endif

#if defined(JPEG_ENABLE_SSE2)

        if (cpu_flags & CPU_SSE2)
        {
            switch (sample)
            {
                case JPEG_U8_Y:
                    break;
                case JPEG_U8_BGR:
                    break;
                case JPEG_U8_RGB:
                    break;
                case JPEG_U8_BGRA:
                    processState.process_ycbcr_8x8   = process_ycbcr_bgra_8x8_sse2;
                    processState.process_ycbcr_8x16  = process_ycbcr_bgra_8x16_sse2;
                    processState.process_ycbcr_16x8  = process_ycbcr_bgra_16x8_sse2;
                    processState.process_ycbcr_16x16 = process_ycbcr_bgra_16x16_sse2;
                    simd = "SSE2";
                    break;
                case JPEG_U8_RGBA:
                    processState.process_ycbcr_8x8   = process_ycbcr_rgba_8x8_sse2;
                    processState.process_ycbcr_8x16  = process_ycbcr_rgba_8x16_sse2;
                    processState.process_ycbcr_16x8  = process_ycbcr_rgba_16x8_sse2;
                    processState.process_ycbcr_16x16 = process_ycbcr_rgba_16x16_sse2;
                    simd = "SSE2";
                    break;
            }
        }

#endif // JPEG_ENABLE_SSE2

#if defined(JPEG_ENABLE_SSE4)

        if (cpu_flags & CPU_SSSE3)
        {
            switch (sample)
            {
                case JPEG_U8_Y:
                    break;
                case JPEG_U8_BGR:
                    processState.process_ycbcr_8x8   = process_ycbcr_bgr_8x8_ssse3;
                    processState.process_ycbcr_8x16  = process_ycbcr_bgr_8x16_ssse3;
                    processState.process_ycbcr_16x8  = process_ycbcr_bgr_16x8_ssse3;
                    processState.process_ycbcr_16x16 = process_ycbcr_bgr_16x16_ssse3;
                    simd = "SSSE3";
                    break;
                case JPEG_U8_RGB:
                    processState.process_ycbcr_8x8   = process_ycbcr_rgb_8x8_ssse3;
                    processState.process_ycbcr_8x16  = process_ycbcr_rgb_8x16_ssse3;
                    processState.process_ycbcr_16x8  = process_ycbcr_rgb_16x8_ssse3;
                    processState.process_ycbcr_16x16 = process_ycbcr_rgb_16x16_ssse3;
                    simd = "SSSE3";
                    break;
                case JPEG_U8_BGRA:
                    break;
                case JPEG_U8_RGBA:
                    break;
            }
        }

#endif // JPEG_ENABLE_SSE4

        std::string id;

        // determine jpeg type -> select innerloops
        switch (components)
        {
            case 1:
                processState.process = processState.process_y;
                processState.clipped = processState.process_y;
                id = "Y";
                break;

            case 3:
                processState.process = processState.process_ycbcr;
                processState.clipped = processState.process_ycbcr;
                id = "YCbCr";

                // detect optimized cases
                if (blocks_in_mcu <= 6)
                {
                    if (xblock == 8 && yblock == 8)
                    {
                        if (processState.process_ycbcr_8x8)
                        {
                            processState.process = processState.process_ycbcr_8x8;
                            id = makeString("%s YCbCr 8x8", simd);
                        }
                    }

                    if (xblock == 8 && yblock == 16)
                    {
                        if (processState.process_ycbcr_8x16)
                        {
                            processState.process = processState.process_ycbcr_8x16;
                            id = makeString("%s YCbCr 8x16", simd);
                        }
                    }

                    if (xblock == 16 && yblock == 8)
                    {
                        if (processState.process_ycbcr_16x8)
                        {
                            processState.process = processState.process_ycbcr_16x8;
                            id = makeString("%s YCbCr 16x8", simd);
                        }
                    }

                    if (xblock == 16 && yblock == 16)
                    {
                        if (processState.process_ycbcr_16x16)
                        {
                            processState.process = processState.process_ycbcr_16x16;
                            id = makeString("%s YCbCr 16x16", simd);
                        }
                    }
                }
                break;

            case 4:
                processState.process = processState.process_cmyk;
                processState.clipped = processState.process_cmyk;
                id = "CMYK";
                break;
        }

        m_ycbcr_name = id;
        debugPrint("  Decoder: %s\n", id.c_str());
    }

    ImageDecodeStatus Parser::decode(Surface& target)
    {
        ImageDecodeStatus status;

        status.success = true;
        status.direct = true;

        if (!scan_memory.address)
        {
            status.success = false;
            return status;
        }

        // allocate blocks
        AlignedPointer<s16> tempBlockVector(mcus * blocks_in_mcu * 64);
        blockVector = tempBlockVector.data();

        // find best matching format
        SampleFormat sf = getSampleFormat(target.format);

        // configure innerloops based on CPU caps
        configureCPU(sf.sample);

        if (is_lossless)
        {
            // lossless only supports L8 and BGRA
            if (components == 1)
            {
                sf.sample = JPEG_U8_Y;
                sf.format = FORMAT_L8;
            }
            else
            {
                sf.sample = JPEG_U8_BGRA;
                sf.format = FORMAT_B8G8R8A8;
            }
        }
        else if (components == 4)
        {
            // CMYK / YCCK is in the slow-path anyway so force BGRA
            sf.sample = JPEG_U8_BGRA;
            sf.format = FORMAT_B8G8R8A8;
        }

        // target surface size has to match (clipping isn't yet supported)
        if (target.width != xsize || target.height != ysize)
        {
            status.direct = false;
        }

        if (target.format != sf.format)
        {
            status.direct = false;
        }

        if (status.direct)
        {
            m_surface = &target;

            parse(scan_memory, true);

            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            if (is_progressive || is_multiscan)
			{
	            finishProgressive();
			}
        }
        else
        {
            Bitmap temp(width, height, sf.format);
            m_surface = &temp;

            parse(scan_memory, true);

            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            if (is_progressive || is_multiscan)
			{
	            finishProgressive();
			}

            target.blit(0, 0, temp);
        }

        blockVector = nullptr;
        status.info = getInfo();

        return status;
    }

    std::string Parser::getInfo() const
    {
        std::string info = m_encoding;

        info += ", ";
        info += m_compression;

        if (!m_idct_name.empty())
        {
            info += ", ";
            info += m_idct_name;
        }

        if (!m_ycbcr_name.empty())
        {
            info += ", ";
            info += m_ycbcr_name;
        }

        return info;
    }

    void Parser::decodeLossless()
    {
        int predictor = decodeState.spectralStart;
        int pointTransform = decodeState.successiveLow;

        auto decodeFunction = huff_decode_mcu_lossless;
        int* previousDC = decodeState.huffman.last_dc_value;

        if (is_arithmetic)
        {
#ifdef MANGO_ENABLE_LICENSE_BSD
            decodeFunction = arith_decode_mcu_lossless;
            previousDC = decodeState.arithmetic.last_dc_value;
#else
            // We don't have a license for arithmetic decoder
            return;
#endif
        }

        const int width = m_surface->width;
        const int height = m_surface->height;
        const int xlast = width - 1;
        const int components = decodeState.comps_in_scan;

        int initPredictor = 1 << (precision - pointTransform - 1);

        std::vector<int> scanLineCache[JPEG_MAX_BLOCKS_IN_MCU];

        for (int i = 0; i < components; ++i)
        {
            scanLineCache[i] = std::vector<int>(width + 1, 0);
        }

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                s16 data[JPEG_MAX_BLOCKS_IN_MCU];

                decodeFunction(data, &decodeState);
                bool restarted = handleRestart();

                for (int currentComponent = 0; currentComponent < components; ++currentComponent)
                {
                    // Predictors
                    int* cache = scanLineCache[currentComponent].data();
                    int Ra = data[currentComponent];
                    int Rb = cache[x + 1];
                    int Rc = cache[x + 0];

                    int Px;

                    // NOTE: need more test files to make this more conformant
                    // NOTE: color sub-sampling is not supported (need test files)

                    if ((x == 0 && y == 0) || restarted)
                        Px = initPredictor;
                    else if (predictor == 0) 
                        Px = 0;
                    else if (x == xlast)
                        Px = cache[0];
                    else if (predictor == 1 || y == 0 || restarted)
                        Px = Ra;
                    else if (predictor == 2)
                        Px = Rb;
                    else if (predictor == 3)
                        Px = Rc;
                    else if (predictor == 4)
                        Px = Ra + Rb - Rc;
                    else if (predictor == 5)
                        Px = Ra + ((Rb - Rc) >> 1);
                    else if (predictor == 6)
                        Px = Rb + ((Ra - Rc) >> 1);
                    else if (predictor == 7)
                        Px = (Ra + Rb) >> 1;
                    else
                        Px = 0;

                    previousDC[currentComponent] = Px;

                    cache[x] = data[currentComponent];
                    data[currentComponent] = data[currentComponent] >> (precision - 8);
                }

                // TODO: optimize
                if (components == 1)
                {
                    int s = byteclamp(data[0] + 128);
                    u8* image = m_surface->address<u8>(x, y);
                    image[0] = s;
                }
                else
                {
                    int r = byteclamp(data[0] + 128);
                    int g = byteclamp(data[1] + 128);
                    int b = byteclamp(data[2] + 128);
                    u32* image = m_surface->address<u32>(x, y);
                    image[0] = makeBGRA(r, g, b, 255);
                }
            }
        }
    }

    void Parser::decodeSequential()
    {
#ifdef JPEG_ENABLE_THREAD
        const int count = ThreadPool::getInstanceSize();
#else
        const int count = 1;
#endif
        if (count > 1)
        {
            decodeSequentialMT();
        }
        else
        {
            decodeSequentialST();
        }
    }

    void Parser::decodeSequentialST()
    {
        const int stride = m_surface->stride;
        const int xstride = m_surface->format.bytes() * xblock;
        const int ystride = stride * yblock;
        u8* image = m_surface->address<u8>(0, 0);

        s16 data[640];

        for (int y = 0; y < ymcu; ++y)
        {
            u8* dest = image;

            ProcessFunc process = processState.process;
            int width = xblock;
            int height = yblock;

            if (yclip && y == ymcu - 1)
            {
                process = processState.clipped;
                height = yclip;
            }

            for (int x = 0; x < xmcu; ++x)
            {
                decodeState.decode(data, &decodeState);
                handleRestart();

                if (xclip && x == xmcu - 1)
                {
                    process = processState.clipped;
                    width = xclip;
                }

                process(dest, stride, data, &processState, width, height);
                dest += xstride;
            }

            image += ystride;
        }
    }

    void Parser::decodeSequentialMT()
    {
        const int stride = m_surface->stride;
        const int xstride = m_surface->format.bytes() * xblock;
        const int ystride = stride * yblock;
        u8* image = m_surface->address<u8>(0, 0);

        ConcurrentQueue queue("jpeg.sequential", Priority::HIGH);

        if (!restartInterval)
        {
            s16* data = blockVector;
            const int mcu_data_size = blocks_in_mcu * 64;

            const int pool_size = ThreadPool::getInstanceSize();

            const int S = pool_size > 1 ? 4 * pool_size : 1;
            const int N = std::max(ymcu / S, pool_size);

            // use threadpool to process blocks
            for (int y = 0; y < ymcu; y += N)
            {
                const int y0 = y;
                const int y1 = std::min(y + N, ymcu);
                const int count = (y1 - y0) * xmcu;
                debugPrint("  Process: [%d, %d] --> ThreadPool.\n", y0, y1 - 1);

                s16* idata = data + y * (xmcu * mcu_data_size);

                for (int i = 0; i < count; ++i)
                {
                    decodeState.decode(idata + i * mcu_data_size, &decodeState);
                    handleRestart();
                }

                // enqueue task
                queue.enqueue([=]
                {
                    for (int y = y0; y < y1; ++y)
                    {
                        u8* dest = image + y * ystride;
                        s16* source = data + y * xmcu * mcu_data_size;

                        ProcessFunc process = processState.process;
                        int width = xblock;
                        int height = yblock;

                        if (yclip && y == ymcu - 1)
                        {
                            process = processState.clipped;
                            height = yclip;
                        }

                        for (int x = 0; x < xmcu; ++x)
                        {
                            if (xclip && x == xmcu - 1)
                            {
                                process = processState.clipped;
                                width = xclip;
                            }

                            process(dest, stride, source, &processState, width, height);
                            source += mcu_data_size;
                            dest += xstride;
                        }
                    }
                });
            }
        }
        else
        {
            const u8* p = decodeState.buffer.ptr;

            for (int i = 0; i < mcus; i += restartInterval)
            {
                // enqueue task
                queue.enqueue([=]
                {
                    AlignedPointer<s16> data(640);

                    DecodeState state = decodeState;
                    state.buffer.ptr = p;

                    const int left = std::min(restartInterval, mcus - i);

                    for (int j = 0; j < left; ++j)
                    {
                        int n = i + j;

                        state.decode(data, &state);

                        int x = n % xmcu;
                        int y = n / xmcu;

                        u8* dest = image + y * ystride + x * xstride;

                        ProcessFunc process = processState.process;
                        int width = xblock;
                        int height = yblock;

                        if (xclip && x == xmcu - 1)
                        {
                            process = processState.clipped;
                            width = xclip;
                        }

                        if (yclip && y == ymcu - 1)
                        {
                            process = processState.clipped;
                            height = yclip;
                        }

                        process(dest, stride, data, &processState, width, height);
                    }
                });

                // seek next restart marker
                p = seekMarker(p, decodeState.buffer.end);
                p += 2;
            }

            decodeState.buffer.ptr = p;
        }

        // synchronize
        queue.wait();
    }

    void Parser::decodeMultiScan()
    {
        s16* data = blockVector;
        data += decodeState.block[0].offset;

        for (int i = 0; i < mcus; ++i)
        {
            decodeState.decode(data, &decodeState);
            handleRestart();
            data += blocks_in_mcu * 64;
        }
    }

    void Parser::decodeProgressive()
    {
        const bool dc_scan = (decodeState.spectralStart == 0);
        s16* data = blockVector;

        if (dc_scan)
        {
            if (decodeState.comps_in_scan == 1 && decodeState.blocks > 1)
            {
                // HACK: process 8x8 blocks in more expensive ac scanner code below
                // NOTE: we propably should do this only when 8x8 block count is not multiple of MCU block size
                decodeState.block[0].offset = 0;
                decodeState.blocks = 1;
            }
            else
            {
                for (int i = 0; i < mcus; ++i)
                {
                    decodeState.decode(data, &decodeState);
                    handleRestart();
                    data += blocks_in_mcu * 64;
                }

                return;
            }
        }

        const int hsf = u32_log2(scanFrame->Hsf);
        const int vsf = u32_log2(scanFrame->Vsf);
        const int hsize = (Hmax >> hsf) * 8;
        const int vsize = (Vmax >> vsf) * 8;

        debugPrint("    hf: %i x %i, log2: %i x %i\n", 1 << hsf, 1 << vsf, hsf, vsf);
        debugPrint("    bs: %i x %i  scanSize: %d\n", hsize, vsize, decodeState.blocks);

        const int scan_offset = scanFrame->offset;

        const int xs = ((xsize + hsize - 1) / hsize);
        const int ys = ((ysize + vsize - 1) / vsize);

        debugPrint("    blocks: %d x %d (%d x %d)\n", xs, ys, xs * hsize, ys * vsize);

        const int HMask = (1 << hsf) - 1;
        const int VMask = (1 << vsf) - 1;

        for (int y = 0; y < ys; ++y)
        {
            int mcu_yoffset = (y >> vsf) * xmcu;
            int block_yoffset = ((y & VMask) << hsf) + scan_offset;

            for (int x = 0; x < xs; ++x)
            {
                int mcu_offset = (mcu_yoffset + (x >> hsf)) * blocks_in_mcu;
                int block_offset = (x & HMask) + block_yoffset;
                s16* mcudata = data + (block_offset + mcu_offset) * 64;

                // decode
                decodeState.decode(mcudata, &decodeState);
                handleRestart();
            }
        }
    }

    void Parser::finishProgressive()
    {
#ifdef JPEG_ENABLE_THREAD
        const int count = ThreadPool::getInstanceSize();
#else
        const int count = 1;
#endif
        if (count > 1)
        {
            finishProgressiveMT();
        }
        else
        {
            finishProgressiveST();
        }
    }

    void Parser::finishProgressiveST()
    {
        const int stride = m_surface->stride;
        const int xstride = m_surface->format.bytes() * xblock;
        const int ystride = stride * yblock;
        u8* image = m_surface->address<u8>(0, 0);

        const int mcu_data_size = blocks_in_mcu * 64;
        s16* data = blockVector;

        for (int y = 0; y < ymcu; ++y)
        {
            u8* dest = image + y * ystride;

            ProcessFunc process = processState.process;
            int width = xblock;
            int height = yblock;

            if (yclip && y == ymcu - 1)
            {
                process = processState.clipped;
                height = yclip;
            }

            for (int x = 0; x < xmcu; ++x)
            {
                if (xclip && x == xmcu - 1)
                {
                    process = processState.clipped;
                    width = xclip;
                }

                process(dest, stride, data, &processState, width, height);
                data += mcu_data_size;
                dest += xstride;
            }
        }
    }

    void Parser::finishProgressiveMT()
    {
        const int stride = m_surface->stride;
        const int xstride = m_surface->format.bytes() * xblock;
        const int ystride = stride * yblock;
        u8* image = m_surface->address<u8>(0, 0);

        const int mcu_data_size = blocks_in_mcu * 64;
        s16* data = blockVector;

        ConcurrentQueue queue("jpeg.progressive", Priority::HIGH);
        const int pool_size = ThreadPool::getInstanceSize();

        const int S = pool_size > 1 ? 4 * pool_size : 1;
        const int N = std::max(ymcu / S, pool_size);

        // use threadpool to process blocks
        for (int y = 0; y < ymcu; y += N)
        {
            const int y0 = y;
            const int y1 = std::min(y + N, ymcu);
            debugPrint("  Process: [%d, %d] --> ThreadPool.\n", y0, y1 - 1);

            // enqueue task
            queue.enqueue([=]
            {
                for (int y = y0; y < y1; ++y)
                {
                    u8* dest = image + y * ystride;
                    s16* source = data + y * xmcu * mcu_data_size;

                    ProcessFunc process = processState.process;
                    int width = xblock;
                    int height = yblock;

                    if (yclip && y == ymcu - 1)
                    {
                        process = processState.clipped;
                        height = yclip;
                    }

                    for (int x = 0; x < xmcu; ++x)
                    {
                        if (xclip && x == xmcu - 1)
                        {
                            process = processState.clipped;
                            width = xclip;
                        }

                        process(dest, stride, source, &processState, width, height);
                        source += mcu_data_size;
                        dest += xstride;
                    }
                }
            });
        }

        // synchronize
        queue.wait();
    }

} // namespace jpeg
} // namespace mango
