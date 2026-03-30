#include <benchmark/benchmark.h>

#include <SparseSet.hpp>

#include <cstddef>
#include <cstdint>
#include <random>

static std::random_device rd;

static auto createRandomSet( std::size_t size ) -> SparseSet< std::uint16_t >
{
	SparseSet< std::uint16_t > set;
	set.reserve( size );

	for( auto i = 0u; i < size; ++i )
		set.insert( std::rand() % size );

	return set;
}

static void BM_SparseSet_contains( benchmark::State& state )
{
	const auto size = state.range( 0 );
	SparseSet< std::uint16_t > set;

	for( auto _ : state )
	{
		state.PauseTiming();
		set = createRandomSet( size );
		state.ResumeTiming();

		for( auto i = 0u; i < size; ++i )
		{
			bool contains = set.contains( std::rand() % size );
			benchmark::DoNotOptimize( contains );
		}
	}

	state.SetItemsProcessed( state.iterations() * size );
	state.SetComplexityN( size );
}
BENCHMARK( BM_SparseSet_contains )->Range( 8, 8 << 10 )->Complexity( benchmark::o1 );

static void BM_SparseSet_insert( benchmark::State& state )
{
	const auto size = state.range( 0 );
	SparseSet< std::uint16_t > set;

	for( auto _ : state )
	{
		state.PauseTiming();
		set = createRandomSet( size );
		state.ResumeTiming();

		for( auto i = 0u; i < size; ++i )
		{
			bool success = set.insert( std::rand() % size );
			benchmark::DoNotOptimize( success );
		}
	}

	state.SetItemsProcessed( state.iterations() * size );
	state.SetComplexityN( size );
}
BENCHMARK( BM_SparseSet_insert )->Range( 8, 8 << 10 )->Complexity( benchmark::o1 );

static void BM_SparseSet_erase( benchmark::State& state )
{
	const auto size = state.range( 0 );
	SparseSet< std::uint16_t > set;

	for( auto _ : state )
	{
		state.PauseTiming();
		set = createRandomSet( size );
		state.ResumeTiming();

		for( auto i = 0u; i < size; ++i )
		{
			bool success = set.erase( std::rand() % size );
			benchmark::DoNotOptimize( success );
		}
	}

	state.SetItemsProcessed( state.iterations() * size );
	state.SetComplexityN( size );
}
BENCHMARK( BM_SparseSet_erase )->Range( 8, 8 << 10 )->Complexity( benchmark::o1 );

BENCHMARK_MAIN();
