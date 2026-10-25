#include "csound.h"
#include "gtest/gtest.h"
#include <string>

class PluginLoadTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;

    void SetUp() override {
        // Load the selected library, without scanning installed plugins.
        csound = csoundCreate(nullptr, "");
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
    }

    void TearDown() override {
        csoundDestroy(csound);
    }

    std::string messages() {
        std::string result;
        while (csoundGetMessageCnt(csound)) {
            result += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        return result;
    }

    void compile(const char *plugin, const char *orchestra) {
        std::string option = std::string("--opcode-lib=") + plugin;
        ASSERT_EQ(csoundSetOption(csound, option.c_str()), 0);
        EXPECT_EQ(csoundCompileOrc(csound, orchestra, 0), 0) << messages();
    }
};

#ifdef CSOUND_TEST_CHUA
TEST_F(PluginLoadTests, ChuaRegistersItsOpcode) {
    compile(CSOUND_TEST_CHUA, R"(
        instr 1
        a1, a2, a3 chuap 0.00945, 7.5, 2e-7, 0.00105, 0, -0.00121, 1.76e-5, 1.5e-8, 0, -0.1, 0.1, 5e-6
        endin
    )");
}
#endif

#ifdef CSOUND_TEST_LINEAR_ALGEBRA
TEST_F(PluginLoadTests, LinearAlgebraRegistersItsOpcodes) {
    compile(CSOUND_TEST_LINEAR_ALGEBRA, R"(
        instr 1
        ivector la_i_vr_create 3
        endin
    )");
}
#endif

#ifdef CSOUND_TEST_FLUID
TEST_F(PluginLoadTests, FluidRegistersItsOpcodes) {
    compile(CSOUND_TEST_FLUID, R"(
        instr 1
        iengine fluidEngine
        aleft, aright fluidOut iengine
        endin
    )");
}
#endif
