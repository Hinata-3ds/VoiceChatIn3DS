#include "KanjiConverter.hpp"

#include "jsmn.h"

namespace CTRPluginFramework
{
    int KanjiConverter::ToKanji(std::string &out, const std::string utf8str)
    {
        int ret = 0;
        std::string js;
        ret = GetJSON(js, "http://www.google.com/transliterate?langpair=ja-Hira|ja&text=" + utf8str);

        if (ret != 0)
            return ret;
        
        jsmn_parser p;
        jsmntok_t t[128];

        jsmn_init(&p);
        int r = jsmn_parse(&p, js.c_str(), js.length(), t, sizeof(t) / sizeof(t[0]));

        for (int i = 0; i < t[0].size; i++)
        {
            std::string retstr(js.c_str(), t[4 + 8*i].start, t[4 + 8*i].end - t[4 + 8*i].start);
            out += retstr;
        }

        return 0;
    }

    int KanjiConverter::GetJSON(std::string &out, const std::string utf8str)
    {
        httpcInit(0);

        int ret = 0;
        httpcContext context;

        u32 statuscode = 0;

        u32 readsize = 0, size = 0;
        u8 *buf, *lastbuf;

        httpcOpenContext(&context, HTTPC_METHOD_GET, utf8str.c_str(), 1);
        httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
        httpcAddRequestHeaderField(&context, "User-Agent", "3gx");
        httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");

        ret = httpcBeginRequest(&context);

        if (ret != 0)
        {
            httpcCloseContext(&context);
            httpcExit();
            return ret;
        }

        ret = httpcGetResponseStatusCode(&context, &statuscode);

        if (ret != 0)
        {
            httpcCloseContext(&context);
            httpcExit();
            return ret;
        }

        buf = (u8 *)malloc(0x1000);
        if (buf == NULL)
        {
            httpcCloseContext(&context);
            httpcExit();
            return -1;
        }

        do
        {
            ret = httpcDownloadData(&context, buf + size, 0x1000, &readsize);
            size += readsize;
            if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING)
            {
                lastbuf = buf;
                buf = (u8 *)realloc(buf, size + 0x1000);
                if (buf == NULL)
                {
                    httpcCloseContext(&context);
                    free(lastbuf);
                    httpcExit();
                    return -1;
                }
            }
        } while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

        if (ret != 0)
        {
            httpcCloseContext(&context);
            free(buf);
            httpcExit();
            return -1;
        }

        lastbuf = buf;
        buf = (u8 *)realloc(buf, size);
        if (buf == NULL)
        {
            httpcCloseContext(&context);
            free(lastbuf);
            httpcExit();
            return -1;
        }

        httpcCloseContext(&context);
        httpcExit();

        Process::ReadString((u32)buf, out, size, StringFormat::Utf8);

        free(buf);

        return 0;
    }
}