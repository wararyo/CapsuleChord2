#ifndef TEST_UTILITIES_H
#define TEST_UTILITIES_H

#include <unity.h>
#include <vector>
#include <cstdint>
#include <string>

namespace TestUtil {
    // ベクトル比較のヘルパー関数
    inline void assertVectorEquals(
        const std::vector<uint8_t>& expected,
        const std::vector<uint8_t>& actual,
        const char* msg = nullptr)
    {
        if (expected.size() != actual.size()) {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s: Size mismatch (expected: %zu, actual: %zu)",
                    msg ? msg : "Vector comparison failed",
                    expected.size(),
                    actual.size());
            TEST_FAIL_MESSAGE(buf);
            return;
        }

        for (size_t i = 0; i < expected.size(); i++) {
            if (expected[i] != actual[i]) {
                char buf[256];
                snprintf(buf, sizeof(buf), "%s: Element[%zu] mismatch (expected: %u, actual: %u)",
                        msg ? msg : "Vector element mismatch",
                        i,
                        expected[i],
                        actual[i]);
                TEST_FAIL_MESSAGE(buf);
                return;
            }
        }
    }

    // 浮動小数点の比較（許容誤差付き）
    inline void assertFloatNear(float expected, float actual, float tolerance = 0.0001f, const char* msg = nullptr) {
        if (std::abs(expected - actual) > tolerance) {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s: Float mismatch (expected: %f, actual: %f, tolerance: %f)",
                    msg ? msg : "Float comparison failed",
                    expected,
                    actual,
                    tolerance);
            TEST_FAIL_MESSAGE(buf);
        }
    }

    // 文字列比較（std::string版）
    inline void assertStringEquals(const std::string& expected, const std::string& actual, const char* msg = nullptr) {
        if (expected != actual) {
            char buf[512];
            snprintf(buf, sizeof(buf), "%s: String mismatch (expected: '%s', actual: '%s')",
                    msg ? msg : "String comparison failed",
                    expected.c_str(),
                    actual.c_str());
            TEST_FAIL_MESSAGE(buf);
        }
    }
}

#endif // TEST_UTILITIES_H
