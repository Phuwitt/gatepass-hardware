#pragma once
#include <stdint.h>
// Pure policy shared with host tests. A repeated request never produces a pulse.
inline bool permitPulse(bool allowed,bool serverEnabled,bool localEnabled,
                        bool fresh,bool repeated,uint32_t duration) {
 return allowed && serverEnabled && localEnabled && fresh && !repeated &&
        duration>=100 && duration<=5000;
}
inline bool elapsed(uint32_t now,uint32_t since,uint32_t interval) {
 return uint32_t(now-since)>=interval;
}
