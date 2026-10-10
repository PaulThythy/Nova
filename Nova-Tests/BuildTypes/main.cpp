#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "Core/Log.h"
#include "Core/Platform.h"

namespace {

// Intentionally heavy CPU work (Mandelbrot escape-time) so Debug vs Release
// timings differ clearly. Result is consumed so the compiler cannot DCE it.
[[nodiscard]] double HeavyMandelbrot(int width, int height, int maxIterations)
{
    double checksum = 0.0;

    for (int y = 0; y < height; ++y) {
        const double ci = (static_cast<double>(y) / height) * 2.0 - 1.0;
        for (int x = 0; x < width; ++x) {
            const double cr = (static_cast<double>(x) / width) * 3.5 - 2.5;
            double zr = 0.0;
            double zi = 0.0;
            int iter = 0;

            while (zr * zr + zi * zi <= 4.0 && iter < maxIterations) {
                const double nextZr = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = nextZr;
                ++iter;
            }

            checksum += static_cast<double>(iter) * (zr + zi + 1.0);
        }
    }

    return checksum;
}

void LogBuildTypeBanner()
{
    NV_LOG_TRACE("TRACE visible: verbose diagnostics (Debug only)");
    NV_LOG_DEBUG("DEBUG visible: developer diagnostics (Debug only)");
    NV_LOG_INFO("INFO visible: general info (Debug + RelWithDebInfo)");
    NV_LOG_WARN("WARN visible: warnings (Debug + RelWithDebInfo + Release)");
    // ERROR always compiles in every configuration
    NV_LOG_ERROR("ERROR visible: always enabled (all build types)");

    // WARN so identity remains visible in Release; MinSizeRel uses ERROR below.
    const std::string identity = std::string("Build=") + NOVA_BUILD_TYPE_NAME + " Platform=" + NOVA_PLATFORM_NAME;
#if defined(NOVA_MINSIZEREL)
    NV_LOG_ERROR(identity);
#else
    NV_LOG_WARN(identity);
#endif
}

void LogPlatformSpecific()
{
#if defined(NOVA_WINDOWS)
    NV_LOG_INFO("Platform gate: this INFO log is compiled only on Windows (NOVA_WINDOWS)");
    NV_LOG_WARN("Platform gate: Windows-only WARN");
#elif defined(NOVA_LINUX)
    NV_LOG_INFO("Platform gate: this INFO log is compiled only on Linux (NOVA_LINUX)");
    NV_LOG_WARN("Platform gate: Linux-only WARN");
#elif defined(NOVA_MACOS)
    NV_LOG_INFO("Platform gate: this INFO log is compiled only on macOS (NOVA_MACOS)");
    NV_LOG_WARN("Platform gate: macOS-only WARN");
#else
    NV_LOG_WARN("Platform gate: unknown platform (NOVA_UNKNOWN_PLATFORM)");
#endif
}

} // namespace

int main(int /*argc*/, char** /*argv*/)
{
    LogBuildTypeBanner();
    LogPlatformSpecific();

    constexpr int kWidth = 1280;
    constexpr int kHeight = 720;
    constexpr int kMaxIterations = 1000;

    NV_LOG_DEBUG("Starting heavy Mandelbrot workload...");

    const auto start = std::chrono::steady_clock::now();
    const double checksum = HeavyMandelbrot(kWidth, kHeight, kMaxIterations);
    const auto end = std::chrono::steady_clock::now();

    const auto elapsedMs =
        std::chrono::duration<double, std::milli>(end - start).count();

    // WARN survives Release; use it so the timing result is always visible
    // except MinSizeRel (ERROR+ only). On MinSizeRel, fall back to ERROR.
    const std::string timing =
        "HeavyMandelbrot " + std::to_string(kWidth) + "x" + std::to_string(kHeight) +
        " @" + std::to_string(kMaxIterations) + " iters -> " +
        std::to_string(elapsedMs) + " ms (checksum=" + std::to_string(checksum) + ")";

#if defined(NOVA_MINSIZEREL)
    NV_LOG_ERROR(timing);
#else
    NV_LOG_WARN(timing);
#endif

    // Keep checksum alive for optimizers across toolchains.
    volatile double sink = checksum;
    (void)sink;

    return (std::isfinite(checksum) && elapsedMs >= 0.0) ? 0 : 1;
}