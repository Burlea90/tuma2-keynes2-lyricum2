#ifndef __GAME_SRC_STDAFX_H__
#define __GAME_SRC_STDAFX_H__

#include "../../config/CommonDefines.h"

#include "../CommonDefines.h"

// Basic features
// Enable or disable memory pooling for specific object types
//#define M2_USE_POOL
// Enable or disable heap allocation debugging
//#define DEBUG_ALLOC

#include "debug_allocator.h"

#include "../libthecore/stdafx.h"

#include "../common/singleton.h"
#include "../common/service.h"
#include "../common/CommonDefines.h"

#include <m2_services.hpp>
#include <m2_utils.hpp>
#include <m2_length.hpp>
#include <m2_itemlength.hpp>
#include <m2_constants.hpp>
#include <m2_tables.hpp>
#include <m2_vnumhelper.hpp>

#include <algorithm>
#include <math.h>
#include <list>
#include <map>
#include <set>
#include <queue>
#include <string>
#include <vector>
#include <cfloat>

#include <unordered_map>
#include <unordered_set>

#if defined(_WIN32)
#define isdigit iswdigit
#define isspace iswspace
#endif

#include "typedef.h"
#include "locale.hpp"
#include "event.h"

#define PASSES_PER_SEC(sec) ((sec) * passes_per_sec)

#ifndef M_PI
#define M_PI    3.14159265358979323846 /* pi */
#endif
#ifndef M_PI_2
#define M_PI_2  1.57079632679489661923 /* pi/2 */
#endif

#define IN
#define OUT

#endif

