/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Console.hpp"

#include "../platform/platform.h"

#include <cstdio>
#include <string>

#ifdef __SWITCH__
extern FILE* switch_log;
extern FILE* switch_err;
#endif

namespace Console
{
    void Write(char c)
    {
#ifdef __SWITCH__
        if (!switch_log)
            switch_log = fopen("/switch/openrct2/log.txt","w");
        if (switch_log)
            fputc(c, switch_log);
#endif
        fputc(c, stdout);
    }

    void Write(const utf8* str)
    {
#ifdef __SWITCH__
        if (!switch_log)
            switch_log = fopen("/switch/openrct2/log.txt","w");
        if (switch_log)
            fputs(str, switch_log);
#endif
        fputs(str, stdout);
    }

    void WriteSpace(size_t count)
    {
        std::string sz(count, ' ');
        Write(sz.c_str());
    }

    void WriteFormat(const utf8* format, ...)
    {
        va_list args;

        va_start(args, format);
#ifdef __SWITCH__
        if (!switch_log)
            switch_log = fopen("/switch/openrct2/log.txt","w");
        if (switch_log)
            vfprintf(switch_log, format, args);
#endif
        vfprintf(stdout, format, args);
        va_end(args);
    }

    void WriteLine()
    {
        puts("");
    }

    void WriteLine(const utf8* format, ...)
    {
        va_list args;

        va_start(args, format);
        auto formatLn = std::string(format) + "\n";
#ifdef __SWITCH__
        if (!switch_log)
            switch_log = fopen("/switch/openrct2/log.txt","w");
        if (switch_log)
            vfprintf(switch_log, formatLn.c_str(), args);
#endif
        vfprintf(stdout, formatLn.c_str(), args);
        va_end(args);
    }

    namespace Error
    {
        void Write(char c)
        {
#ifdef __SWITCH__
            if (!switch_err)
                switch_err = fopen("/switch/openrct2/err.txt","w");
            if (switch_err)
                fputc(c, switch_err);
#endif
            fputc(c, stderr);
        }

        void Write(const utf8* str)
        {
#ifdef __SWITCH__
            if (!switch_err)
                switch_err = fopen("/switch/openrct2/err.txt","w");
            if (switch_err)
                fputs(str, switch_err);
#endif
            fputs(str, stderr);
        }

        void WriteFormat(const utf8* format, ...)
        {
            va_list args;

            va_start(args, format);
#ifdef __SWITCH__
            if (!switch_err)
                switch_err = fopen("/switch/openrct2/err.txt","w");
            if (switch_err)
                vfprintf(switch_err, format, args);
#endif
            vfprintf(stderr, format, args);
            va_end(args);
        }

        void WriteLine()
        {
#ifdef __SWITCH__
            if (!switch_err)
                switch_err = fopen("/switch/openrct2/err.txt","w");
            if (switch_err)
                fputs(PLATFORM_NEWLINE, switch_err);
#endif
            fputs(PLATFORM_NEWLINE, stderr);
        }

        void WriteLine(const utf8* format, ...)
        {
            va_list args;
            va_start(args, format);
            WriteLine_VA(format, args);
            va_end(args);
        }

        void WriteLine_VA(const utf8* format, va_list args)
        {
            auto formatLn = std::string(format) + "\n";
            vfprintf(stdout, formatLn.c_str(), args);
        }
    } // namespace Error
} // namespace Console
