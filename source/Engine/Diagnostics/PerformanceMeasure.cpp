#if INTERFACE
#include <Engine/Includes/Standard.h>
#include <Engine/Diagnostics/PerformanceTypes.h>

class PerformanceMeasure {
public:
    static bool            Initialized;
    static Perf_ViewRender PERF_ViewRender[8];
};
#endif

#include <Engine/Diagnostics/PerformanceMeasure.h>

bool            PerformanceMeasure::Initialized = false;
Perf_ViewRender PerformanceMeasure::PERF_ViewRender[8];

PUBLIC STATIC void PerformanceMeasure::Init() {
    if (PerformanceMeasure::Initialized)
        return;

    PerformanceMeasure::Initialized = true;
    memset(PerformanceMeasure::PERF_ViewRender, 0, sizeof(PerformanceMeasure::PERF_ViewRender));
}
