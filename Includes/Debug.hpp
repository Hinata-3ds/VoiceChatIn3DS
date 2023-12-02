#pragma once

#define DEBUG

#ifdef DEBUG
    #define DEBUG_NOTIFY(msg) CTRPluginFramework::OSD::Notify((msg))
    #define DEBUG_NOTIFYC(msg, color) CTRPluginFramework::OSD::Notify((msg), (color))
#else
    #define DEBUG_NOTIFY(msg)
    #define DEBUG_NOTIFYC(msg, color)
#endif