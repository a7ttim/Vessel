// (c) 2024 Acid7Beast. Use with wisdom.
#include <gtest/gtest.h>
#include <format>
#include <ostream>
#include <numeric>

#include <Flow/Container.h>
#include <Flow/ProvideLimiter.h>
#include <Flow/ConsumeLimiter.h>

namespace Flow
{
	struct ContainerTestTag {};

	// Specialize for Test.
	template <>
	struct TagSelector<ContainerTestTag> {
		using Units = float;
	};
}

namespace {
	using Tag = ::Flow::ContainerTestTag;
	using Container = ::Flow::Container<Tag>;
	using ProvideLimiter = ::Flow::ProvideLimiter<Tag>;
	using ConsumeLimiter = ::Flow::ConsumeLimiter<Tag>;
	using Units = Container::Units;

	constexpr Units kEmptyAmountKg = 0.f;
	constexpr Units kCapacityAmountKg = 255.f;
	constexpr Units kHalfCapacityAmountKg = kCapacityAmountKg * 0.5f;

	class ContainerChecker
	{
		// Life circle.
	public:
		ContainerChecker(const Container& container)
			: _container{ container }
		{
		}

		// Public interface.
	public:
		// Check 100% fullness state.
		void CheckFullState() const
		{
			EXPECT_FLOAT_EQ(_container.GetRequestUnits(), kEmptyAmountKg);
			EXPECT_FLOAT_EQ(_container.GetAvailableUnits(), kCapacityAmountKg);
		}

		// Check 50% fullness state.
		void CheckHalfState() const
		{
			EXPECT_FLOAT_EQ(_container.GetRequestUnits(), kHalfCapacityAmountKg);
			EXPECT_FLOAT_EQ(_container.GetAvailableUnits(), kHalfCapacityAmountKg);
		}

		// Check 0% fullness state.
		void CheckEmptyState() const
		{
			EXPECT_FLOAT_EQ(_container.GetRequestUnits(), kCapacityAmountKg);
			EXPECT_FLOAT_EQ(_container.GetAvailableUnits(), kEmptyAmountKg);
		}

		// Private state.
	private:
		const Container& _container;
	};

	class ContainerFixture : public ::testing::Test
	{
		// Inheritable state.
	protected:
		Container::Properties properties{ kCapacityAmountKg };
		Container::State emptyState{ kEmptyAmountKg };
		Container::State halfState{ kHalfCapacityAmountKg };
		Container::State fullState{ kCapacityAmountKg };
		Container::State testState;
		Container provider{ properties };
		Container consumer{ properties };
		ContainerChecker providerChecker{ provider };
		ContainerChecker consumerChecker{ consumer };
	};

	TEST_F(ContainerFixture, ConstructorTest) {
		// Resources are full on creation.
		providerChecker.CheckFullState();
		consumerChecker.CheckFullState();
	}

	TEST_F(ContainerFixture, LoadStateTest) {
		// Make the consumer empty.
		consumer.LoadState(emptyState);
		consumerChecker.CheckEmptyState();

		// Make the consumer full.
		consumer.LoadState(fullState);
		consumerChecker.CheckFullState();

		// Make the consumer half.
		consumer.LoadState(halfState);
		consumerChecker.CheckHalfState();

		// Make the consumer empty again.
		consumer.LoadState(emptyState);
		consumerChecker.CheckEmptyState();
	}

	TEST_F(ContainerFixture, SaveStateTest) {
		// Check moving empty state to another container.
		consumer.LoadState(emptyState);
		consumer.SaveState(testState);
		provider.LoadState(testState);
		consumerChecker.CheckEmptyState();
		providerChecker.CheckEmptyState();

		// Check moving full state to another container.
		consumer.LoadState(fullState);
		consumer.SaveState(testState);
		provider.LoadState(testState);
		consumerChecker.CheckFullState();
		providerChecker.CheckFullState();

		// Check moving half state to another container.
		consumer.LoadState(halfState);
		consumer.SaveState(testState);
		provider.LoadState(testState);
		consumerChecker.CheckHalfState();
		providerChecker.CheckHalfState();

		// Check moving empty state to another container again.
		consumer.LoadState(emptyState);
		consumer.SaveState(testState);
		provider.LoadState(testState);
		consumerChecker.CheckEmptyState();
		providerChecker.CheckEmptyState();
	}

	TEST_F(ContainerFixture, ProvideOperatorTestIn) {
		// Check resource transit to consumer with `<<`.
		consumer.LoadState(emptyState);
		consumer << provider << provider;
		providerChecker.CheckEmptyState();
		consumerChecker.CheckFullState();
	}

	TEST_F(ContainerFixture, ProvideOperatorTestOut) {
		// Check resource transit to consumer with `>>`.
		consumer.LoadState(emptyState);
		provider >> consumer >> consumer;
		providerChecker.CheckEmptyState();
		consumerChecker.CheckFullState();
	}
	
	TEST_F(ContainerFixture, ProvideLimiterTestIn) {
		// Check resource transit through limiter with `<<`.
		consumer.LoadState(emptyState);
		consumer << ProvideLimiter(provider, kHalfCapacityAmountKg);
		providerChecker.CheckHalfState();
		consumerChecker.CheckHalfState();

		consumer << ProvideLimiter(provider, kHalfCapacityAmountKg);
		providerChecker.CheckEmptyState();
		consumerChecker.CheckFullState();
	}

	TEST_F(ContainerFixture, ProvideLimiterTestOut) {
		// Check resource transit through limiter with `>>`.
		consumer.LoadState(emptyState);
		ProvideLimiter(provider, kHalfCapacityAmountKg) >> consumer;
		providerChecker.CheckHalfState();
		consumerChecker.CheckHalfState();

		ProvideLimiter(provider, kHalfCapacityAmountKg) >> consumer;
		providerChecker.CheckEmptyState();
		consumerChecker.CheckFullState();
	}

	TEST_F(ContainerFixture, ConsumeLimiterTestIn) {
		// Check resource transit through limiter with `<<`.
		consumer.LoadState(emptyState);
		ConsumeLimiter(consumer, kHalfCapacityAmountKg) << provider;
		providerChecker.CheckHalfState();
		consumerChecker.CheckHalfState();

		ConsumeLimiter(consumer, kHalfCapacityAmountKg) << provider;
		providerChecker.CheckEmptyState();
		consumerChecker.CheckFullState();
	}

	TEST_F(ContainerFixture, ConsumeLimiterTestOut) {
		// Check resource transit through limiter with `>>`.
		consumer.LoadState(emptyState);
		provider >> ConsumeLimiter(consumer, kHalfCapacityAmountKg);
		providerChecker.CheckHalfState();
		consumerChecker.CheckHalfState();

		provider >> ConsumeLimiter(consumer, kHalfCapacityAmountKg);
		providerChecker.CheckEmptyState();
		consumerChecker.CheckFullState();
	}
} // namespace