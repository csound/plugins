#include <gtest/gtest.h>
#include "STKInstrumentAdapter.hpp"

// Observe the rate the instrument sees, without depending on any sound model.
struct SampleRateProbe {
    double constructedAt = stk::Stk::sampleRate();
    double noteStartedAt = 0;

    SampleRateProbe() = default;
    explicit SampleRateProbe(stk::StkFloat) {}

    void noteOn(stk::StkFloat, stk::StkFloat) {
        noteStartedAt = stk::Stk::sampleRate();
    }
};

template <typename Adapter>
class StkAdapterTest : public ::testing::Test {
protected:
    INSDS instrument = {};
    cs_float frequency = 440;
    cs_float gain = 0.5;
    Adapter adapter;

    void SetUp() override {
        stk::Stk::setSampleRate(44100);
        adapter.opds.insdshead = &instrument;
        adapter.ifrequency = &frequency;
        adapter.igain = &gain;
    }

    void TearDown() override {
        adapter.deinit(nullptr);
        stk::Stk::setSampleRate(44100);
    }
};

// Default construction (e.g. BeeThree) and a lowest-frequency argument (Saxofony).
using Adapters = ::testing::Types<
    csound::STKInstrumentAdapter<SampleRateProbe>,
    csound::STKInstrumentAdapter1<SampleRateProbe>>;
TYPED_TEST_SUITE(StkAdapterTest, Adapters);

TYPED_TEST(StkAdapterTest, SetsSampleRateBeforeConstructingInstrument) {
    this->instrument.esr = 48000;
    ASSERT_EQ(this->adapter.init(nullptr), OK);

    EXPECT_DOUBLE_EQ(this->adapter.instrument->constructedAt, 48000);
    EXPECT_DOUBLE_EQ(this->adapter.instrument->noteStartedAt, 48000);
}

TYPED_TEST(StkAdapterTest, NewInstrumentsFollowSampleRateChanges) {
    for (double rate : {48000, 96000, 44100}) {
        SCOPED_TRACE(rate);
        this->instrument.esr = rate;
        ASSERT_EQ(this->adapter.init(nullptr), OK);

        EXPECT_DOUBLE_EQ(this->adapter.instrument->constructedAt, rate);
        EXPECT_DOUBLE_EQ(this->adapter.instrument->noteStartedAt, rate);
        this->adapter.deinit(nullptr);
    }
}

TYPED_TEST(StkAdapterTest, ReinitializationSetsSampleRateBeforeNoteOn) {
    this->instrument.esr = 48000;
    ASSERT_EQ(this->adapter.init(nullptr), OK);
    auto *existing = this->adapter.instrument;

    // Another initialization may have changed STK's shared sample rate.
    stk::Stk::setSampleRate(44100);
    ASSERT_EQ(this->adapter.init(nullptr), OK);

    EXPECT_EQ(this->adapter.instrument, existing);
    EXPECT_DOUBLE_EQ(this->adapter.instrument->noteStartedAt, 48000);
}
