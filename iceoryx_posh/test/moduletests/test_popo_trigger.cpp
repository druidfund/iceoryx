// Copyright (c) 2020 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_waiter.hpp"
#include "iceoryx_posh/popo/trigger.hpp"

#include "test.hpp"
#include <thread>

using namespace iox;
using namespace iox::popo;
using namespace ::testing;

typedef TriggerState* CreateBaseTrigger();

namespace internalTesting
{
void* originValue;
uint64_t triggerIdValue = 0U;
TriggerState::Callback<void> callbackValue = nullptr;
cxx::ConstMethodCallback<bool> hasTriggerCallbackValue;
cxx::MethodCallback<void, const Trigger&> resetCallbackValue;
ConditionVariableData conditionVariableDataValue;

int triggerCallbackValueSetter = 0;

void triggerCallback(int* const origin)
{
    *origin = triggerCallbackValueSetter;
}

template <typename T>
TriggerState* createTriggerState()
{
    return new TriggerState(
        static_cast<T*>(originValue), triggerIdValue, reinterpret_cast<TriggerState::Callback<T>>(callbackValue));
}

template <typename T>
TriggerState* createTrigger()
{
    return new Trigger(static_cast<T*>(originValue),
                       &conditionVariableDataValue,
                       hasTriggerCallbackValue,
                       resetCallbackValue,
                       triggerIdValue,
                       reinterpret_cast<TriggerState::Callback<T>>(callbackValue));
}

class TriggerStateInheritance_test : public TestWithParam<CreateBaseTrigger*>
{
  public:
    TriggerStateInheritance_test()
    {
    }

    ~TriggerStateInheritance_test()
    {
        delete m_sut;
    }

    void createSut()
    {
        m_sut = (*GetParam())();
    }

    TriggerState* m_sut{nullptr};
};


TEST_P(TriggerStateInheritance_test, getTriggerIdReturnsValidTriggerId)
{
    triggerIdValue = 1234;
    createSut();

    EXPECT_EQ(m_sut->getTriggerId(), 1234);
}

TEST_P(TriggerStateInheritance_test, doesOriginateFromStatesOriginCorrectly)
{
    int bla;
    float fuu;
    originValue = &bla;
    createSut();

    EXPECT_EQ(m_sut->doesOriginateFrom(&bla), true);
    EXPECT_EQ(m_sut->doesOriginateFrom(&fuu), false);
}

TEST_P(TriggerStateInheritance_test, getOriginReturnsCorrectOriginWhenHavingCorrectType)
{
    int bla;
    originValue = &bla;
    createSut();

    EXPECT_EQ(m_sut->getOrigin<int>(), &bla);
    EXPECT_EQ(const_cast<const TriggerState*>(m_sut)->getOrigin<int>(), &bla);
}

TEST_P(TriggerStateInheritance_test, triggerCallbackReturnsTrueAndCallsCallbackWithSettedCallback)
{
    int bla;
    originValue = &bla;
    triggerCallbackValueSetter = 4242;
    callbackValue = reinterpret_cast<TriggerState::Callback<void>>(triggerCallback);
    createSut();

    EXPECT_TRUE((*m_sut)());
    EXPECT_EQ(bla, 4242);
}

TEST_P(TriggerStateInheritance_test, triggerCallbackReturnsFalseWithUnsetCallback)
{
    int bla;
    originValue = &bla;
    triggerCallbackValueSetter = 4242;
    callbackValue = TriggerState::Callback<void>(nullptr);
    createSut();

    EXPECT_FALSE((*m_sut)());
}

INSTANTIATE_TEST_CASE_P(TriggerStateAndChilds,
                        TriggerStateInheritance_test,
                        Values(&createTriggerState<int>, &createTrigger<int>));

class TriggerState_test : public Test
{
  public:
    virtual void SetUp()
    {
        internal::CaptureStderr();
    }
    virtual void TearDown()
    {
        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }
};

TEST_F(TriggerState_test, DefaultCTorConstructsEmptyTriggerState)
{
    int bla;
    TriggerState sut;

    EXPECT_EQ(sut.getTriggerId(), TriggerState::INVALID_TRIGGER_ID);
    EXPECT_EQ(sut.doesOriginateFrom(&bla), false);
    EXPECT_EQ(sut.getOrigin<void>(), nullptr);
    EXPECT_EQ(const_cast<const TriggerState&>(sut).getOrigin<void>(), nullptr);
    EXPECT_EQ(sut(), false);
}

class Trigger_test : public Test
{
  public:
    class TriggerClass
    {
      public:
        bool hasTriggered() const
        {
            return m_hasTriggered;
        }

        void resetCall(const Trigger& trigger)
        {
            m_resetCallTriggerArg = &trigger;
        }

        static void callback(TriggerClass* const ptr)
        {
            m_lastCallbackArgument = ptr;
        }

        bool m_hasTriggered = false;
        const Trigger* m_resetCallTriggerArg = nullptr;
        ConditionVariableData* m_condVar = nullptr;
        static TriggerClass* m_lastCallbackArgument;

        const Trigger* m_moveCallTriggerArg = nullptr;
        void* m_moveCallNewOriginArg = nullptr;
    };

    virtual void SetUp()
    {
        internal::CaptureStderr();
    }
    virtual void TearDown()
    {
        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }

    Trigger createValidTrigger(const uint64_t triggerId = 0)
    {
        return Trigger(&m_triggerClass,
                       &m_condVar,
                       {&m_triggerClass, &TriggerClass::hasTriggered},
                       {&m_triggerClass, &TriggerClass::resetCall},
                       triggerId,
                       TriggerClass::callback);
    }

    ConditionVariableData m_condVar;
    TriggerClass m_triggerClass;
};

Trigger_test::TriggerClass* Trigger_test::TriggerClass::m_lastCallbackArgument = nullptr;

TEST_F(Trigger_test, DefaultCTorConstructsEmptyTrigger)
{
    int bla;
    Trigger sut, sut2;

    EXPECT_EQ(sut.getTriggerId(), TriggerState::INVALID_TRIGGER_ID);
    EXPECT_EQ(sut.doesOriginateFrom(&bla), false);
    EXPECT_EQ(sut.getOrigin<void>(), nullptr);
    EXPECT_EQ(const_cast<const Trigger&>(sut).getOrigin<void>(), nullptr);
    EXPECT_EQ(sut(), false);
    EXPECT_EQ(static_cast<bool>(sut), false);
    EXPECT_EQ(sut.isValid(), false);
    EXPECT_EQ(sut.hasTriggered(), false);
    EXPECT_EQ(sut.getConditionVariableData(), nullptr);
}

TEST_F(Trigger_test, TriggerWithValidOriginIsValid)
{
    Trigger sut = createValidTrigger();

    EXPECT_TRUE(sut.isValid());
    EXPECT_TRUE(static_cast<bool>(sut));
}

TEST_F(Trigger_test, TriggerWithInvalidOriginIsValid)
{
    uint64_t triggerId = 0U;
    Trigger sut(static_cast<TriggerClass*>(nullptr),
                &m_condVar,
                {&m_triggerClass, &TriggerClass::hasTriggered},
                {&m_triggerClass, &TriggerClass::resetCall},
                triggerId,
                TriggerClass::callback);

    EXPECT_FALSE(sut.isValid());
    EXPECT_FALSE(static_cast<bool>(sut));
}

TEST_F(Trigger_test, TriggerWithInvalidHasTriggeredCallbackIsInvalid)
{
    uint64_t triggerId = 0U;
    Trigger sut(&m_triggerClass,
                &m_condVar,
                cxx::ConstMethodCallback<bool>(),
                {&m_triggerClass, &TriggerClass::resetCall},
                triggerId,
                TriggerClass::callback);

    EXPECT_FALSE(sut.isValid());
    EXPECT_FALSE(static_cast<bool>(sut));
}

TEST_F(Trigger_test, ResetInvalidatesTrigger)
{
    Trigger sut = createValidTrigger();
    sut.reset();

    EXPECT_FALSE(sut.isValid());
    EXPECT_FALSE(static_cast<bool>(sut));
}

TEST_F(Trigger_test, InvalidateInvalidatesTrigger)
{
    Trigger sut = createValidTrigger();
    sut.invalidate();

    EXPECT_FALSE(sut.isValid());
    EXPECT_FALSE(static_cast<bool>(sut));
}

TEST_F(Trigger_test, ResetCallsResetcallbackWithCorrectTriggerOrigin)
{
    Trigger sut = createValidTrigger();
    sut.reset();

    EXPECT_EQ(m_triggerClass.m_resetCallTriggerArg, &sut);
}

TEST_F(Trigger_test, TriggerWithEmptyResetCallIsValid)
{
    uint64_t triggerId = 0U;
    Trigger sut(&m_triggerClass,
                &m_condVar,
                {&m_triggerClass, &TriggerClass::hasTriggered},
                cxx::MethodCallback<void, const Trigger&>(),
                triggerId,
                TriggerClass::callback);

    EXPECT_TRUE(sut.isValid());
    EXPECT_TRUE(static_cast<bool>(sut));
}

TEST_F(Trigger_test, TriggerWithEmptyResetInvalidatesTriggerWhenBeingResetted)
{
    uint64_t triggerId = 0U;
    Trigger sut(&m_triggerClass,
                &m_condVar,
                {&m_triggerClass, &TriggerClass::hasTriggered},
                cxx::MethodCallback<void, const Trigger&>(),
                triggerId,
                TriggerClass::callback);

    sut.reset();

    EXPECT_FALSE(sut.isValid());
    EXPECT_FALSE(static_cast<bool>(sut));
}

TEST_F(Trigger_test, TriggerCallsHasTriggeredCallback)
{
    Trigger sut = createValidTrigger();

    m_triggerClass.m_hasTriggered = true;
    EXPECT_TRUE(sut.hasTriggered());
    m_triggerClass.m_hasTriggered = false;
    EXPECT_FALSE(sut.hasTriggered());
}

TEST_F(Trigger_test, HasTriggeredCallbackReturnsAlwaysFalseWhenInvalid)
{
    Trigger sut = createValidTrigger();
    m_triggerClass.m_hasTriggered = true;
    sut.reset();

    EXPECT_FALSE(sut.hasTriggered());
}

TEST_F(Trigger_test, ConditionVariableDataValidWhenTriggerIsValid)
{
    Trigger sut = createValidTrigger();
    EXPECT_EQ(sut.getConditionVariableData(), &m_condVar);
}

TEST_F(Trigger_test, ConditionVariableDataIsNullptrWhenTriggerIsInvalid)
{
    Trigger sut = createValidTrigger();
    sut.reset();
    EXPECT_EQ(sut.getConditionVariableData(), nullptr);
}

TEST_F(Trigger_test, TriggerTriggersConditionVariable)
{
    Trigger sut = createValidTrigger();
    std::thread t([&] { sut.trigger(); });

    EXPECT_TRUE(ConditionVariableWaiter(sut.getConditionVariableData())
                    .timedWait(units::Duration::seconds(static_cast<long double>(1.0))));
    t.join();
}

/// Two triggers are equal when they have the same:
///   - origin
///   - hasTriggeredCallback
///   - conditionVariableDataPtr
TEST_F(Trigger_test, TriggersWithDifferentOriginsAreNotEqual)
{
    Trigger sut = createValidTrigger();
    TriggerClass secondTriggerClass;

    Trigger sut2(&secondTriggerClass,
                 &m_condVar,
                 {&m_triggerClass, &TriggerClass::hasTriggered},
                 {&m_triggerClass, &TriggerClass::resetCall},
                 123,
                 TriggerClass::callback);

    EXPECT_FALSE(sut.isLogicalEqualTo(sut2));
}

TEST_F(Trigger_test, TriggersWithDifferentHasTriggeredCallsAreNotEqual)
{
    Trigger sut = createValidTrigger();
    TriggerClass secondTriggerClass;

    Trigger sut2(&m_triggerClass,
                 &m_condVar,
                 {&secondTriggerClass, &TriggerClass::hasTriggered},
                 {&m_triggerClass, &TriggerClass::resetCall},
                 123,
                 TriggerClass::callback);

    EXPECT_FALSE(sut.isLogicalEqualTo(sut2));
}

TEST_F(Trigger_test, TriggersWithDifferentConditionVariablesAreNotEqual)
{
    Trigger sut = createValidTrigger();
    ConditionVariableData condVar2;

    Trigger sut2(&m_triggerClass,
                 &condVar2,
                 {&m_triggerClass, &TriggerClass::hasTriggered},
                 {&m_triggerClass, &TriggerClass::resetCall},
                 123,
                 TriggerClass::callback);

    EXPECT_FALSE(sut.isLogicalEqualTo(sut2));
}

TEST_F(Trigger_test, TriggersAreEqualWhenEqualityRequirementsAreFulfilled)
{
    Trigger sut = createValidTrigger(891);

    Trigger sut2(&m_triggerClass,
                 &m_condVar,
                 {&m_triggerClass, &TriggerClass::hasTriggered},
                 {&m_triggerClass, &TriggerClass::resetCall},
                 891,
                 TriggerClass::callback);

    EXPECT_TRUE(sut.isLogicalEqualTo(sut2));
}

TEST_F(Trigger_test, UpdateOriginLeadsToDifferentOriginParameterInCallback)
{
    Trigger sut = createValidTrigger();
    TriggerClass secondTriggerClass;

    sut.updateOrigin(&secondTriggerClass);
    sut();

    EXPECT_EQ(&secondTriggerClass, TriggerClass::m_lastCallbackArgument);
}

TEST_F(Trigger_test, UpdateOriginLeadsToDifferentHasTriggeredCallback)
{
    Trigger sut = createValidTrigger();
    TriggerClass secondTriggerClass;

    sut.updateOrigin(&secondTriggerClass);

    secondTriggerClass.m_hasTriggered = false;
    EXPECT_FALSE(sut.hasTriggered());
    secondTriggerClass.m_hasTriggered = true;
    EXPECT_TRUE(sut.hasTriggered());
}

TEST_F(Trigger_test, UpdateOriginDoesNotUpdateHasTriggeredIfItsNotOriginatingFromOrigin)
{
    TriggerClass secondTriggerClass, thirdTriggerClass;
    Trigger sut(&m_triggerClass,
                &m_condVar,
                {&thirdTriggerClass, &TriggerClass::hasTriggered},
                {&m_triggerClass, &TriggerClass::resetCall},
                891,
                TriggerClass::callback);

    sut.updateOrigin(&secondTriggerClass);

    thirdTriggerClass.m_hasTriggered = false;
    EXPECT_FALSE(sut.hasTriggered());
    thirdTriggerClass.m_hasTriggered = true;
    EXPECT_TRUE(sut.hasTriggered());
}

TEST_F(Trigger_test, UpdateOriginLeadsToDifferentResetCallback)
{
    Trigger sut = createValidTrigger();
    TriggerClass secondTriggerClass;

    sut.updateOrigin(&secondTriggerClass);
    sut.reset();

    EXPECT_EQ(secondTriggerClass.m_resetCallTriggerArg, &sut);
}

TEST_F(Trigger_test, UpdateOriginDoesNotUpdateResetIfItsNotOriginatingFromOrigin)
{
    TriggerClass secondTriggerClass, thirdTriggerClass;
    Trigger sut(&m_triggerClass,
                &m_condVar,
                {&m_triggerClass, &TriggerClass::hasTriggered},
                {&thirdTriggerClass, &TriggerClass::resetCall},
                891,
                TriggerClass::callback);

    sut.updateOrigin(&secondTriggerClass);
    sut.reset();

    EXPECT_EQ(thirdTriggerClass.m_resetCallTriggerArg, &sut);
}

TEST_F(Trigger_test, TriggerIsLogicalEqualToItself)
{
    Trigger sut1(&m_triggerClass,
                 &m_condVar,
                 {&m_triggerClass, &TriggerClass::hasTriggered},
                 {&m_triggerClass, &TriggerClass::resetCall},
                 8911,
                 TriggerClass::callback);

    EXPECT_TRUE(sut1.isLogicalEqualTo(sut1));
}

TEST_F(Trigger_test, TwoTriggersAreLogicalEqualIfRequirementsAreFullfilled)
{
    Trigger sut1(&m_triggerClass,
                 &m_condVar,
                 {&m_triggerClass, &TriggerClass::hasTriggered},
                 {&m_triggerClass, &TriggerClass::resetCall},
                 891,
                 TriggerClass::callback);

    Trigger sut2(&m_triggerClass,
                 &m_condVar,
                 {&m_triggerClass, &TriggerClass::hasTriggered},
                 {&m_triggerClass, &TriggerClass::resetCall},
                 891,
                 TriggerClass::callback);


    EXPECT_TRUE(sut1.isLogicalEqualTo(sut2));
    EXPECT_TRUE(sut2.isLogicalEqualTo(sut1));
}

TEST_F(Trigger_test, TwoTriggersAreNotLogicalEqualIfTriggerIdDiffers)
{
    Trigger sut1(&m_triggerClass,
                 &m_condVar,
                 {&m_triggerClass, &TriggerClass::hasTriggered},
                 {&m_triggerClass, &TriggerClass::resetCall},
                 2891,
                 TriggerClass::callback);

    Trigger sut2(&m_triggerClass,
                 &m_condVar,
                 {&m_triggerClass, &TriggerClass::hasTriggered},
                 {&m_triggerClass, &TriggerClass::resetCall},
                 3891,
                 TriggerClass::callback);


    EXPECT_FALSE(sut1.isLogicalEqualTo(sut2));
    EXPECT_FALSE(sut2.isLogicalEqualTo(sut1));
}

TEST_F(Trigger_test, TwoTriggersAreNotLogicalEqualIfHasTriggeredCallbackDiffers)
{
    TriggerClass secondTriggerClass;
    Trigger sut1(&m_triggerClass,
                 &m_condVar,
                 {&m_triggerClass, &TriggerClass::hasTriggered},
                 {&m_triggerClass, &TriggerClass::resetCall},
                 4891,
                 TriggerClass::callback);

    Trigger sut2(&m_triggerClass,
                 &m_condVar,
                 {&secondTriggerClass, &TriggerClass::hasTriggered},
                 {&m_triggerClass, &TriggerClass::resetCall},
                 4891,
                 TriggerClass::callback);


    EXPECT_FALSE(sut1.isLogicalEqualTo(sut2));
    EXPECT_FALSE(sut2.isLogicalEqualTo(sut1));
}

TEST_F(Trigger_test, TwoTriggersAreNotLogicalEqualIfConditionVariableDiffers)
{
    ConditionVariableData condVar2;
    Trigger sut1(&m_triggerClass,
                 &m_condVar,
                 {&m_triggerClass, &TriggerClass::hasTriggered},
                 {&m_triggerClass, &TriggerClass::resetCall},
                 4891,
                 TriggerClass::callback);

    Trigger sut2(&m_triggerClass,
                 &condVar2,
                 {&m_triggerClass, &TriggerClass::hasTriggered},
                 {&m_triggerClass, &TriggerClass::resetCall},
                 4891,
                 TriggerClass::callback);


    EXPECT_FALSE(sut1.isLogicalEqualTo(sut2));
    EXPECT_FALSE(sut2.isLogicalEqualTo(sut1));
}

TEST_F(Trigger_test, TwoTriggersAreNotLogicalEqualIfOriginDiffers)
{
    TriggerClass secondTriggerClass;
    Trigger sut1(&m_triggerClass,
                 &m_condVar,
                 {&m_triggerClass, &TriggerClass::hasTriggered},
                 {&m_triggerClass, &TriggerClass::resetCall},
                 4891,
                 TriggerClass::callback);

    Trigger sut2(&secondTriggerClass,
                 &m_condVar,
                 {&m_triggerClass, &TriggerClass::hasTriggered},
                 {&m_triggerClass, &TriggerClass::resetCall},
                 4891,
                 TriggerClass::callback);


    EXPECT_FALSE(sut1.isLogicalEqualTo(sut2));
    EXPECT_FALSE(sut2.isLogicalEqualTo(sut1));
}


} // namespace internalTesting
