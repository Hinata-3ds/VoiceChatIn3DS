#pragma once

#include "types.h"
#include <3ds.h>
#include <CTRPluginFramework.hpp>


namespace CTRPluginFramework
{
    class KanjiConverter
    {
    public:

        /**
         * \return -1 if the conversion failed, 0 on success
        */
        static int ToKanji(std::string &out, const std::string utf8str);

        static int GetJSON(std::string &out, const std::string utf8str);
    private:
    };
}