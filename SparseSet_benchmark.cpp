#include <benchmark/benchmark.h>

#include <SparseSet.hpp>

#include <cstddef>
#include <cstdint>
#include <random>

static std::random_device rd;

static auto createSet( std::size_t size ) -> SparseSet< std::uint16_t >
{
	const auto& range = benchmark::CreateDenseRange( 0, size, 2 );

	return SparseSet< std::uint16_t >{ range.begin(), range.end() };
}

static void BM_SparseSet_contains( benchmark::State& state )
{
	std::mt19937 rng( rd() );
	std::uniform_int_distribution< std::uint16_t > dist( 0, state.range( 0 ) );

	auto set = createSet( state.range( 0 ) );

	for( auto _ : state )
	{
		benchmark::DoNotOptimize( set.contains( dist( rng ) ) );
	}
}
BENCHMARK( BM_SparseSet_contains )->RangeMultiplier( 32 )->Range( 0, 8 << 9 )->Complexity();

static void BM_SparseSet_insert( benchmark::State& state )
{
	std::mt19937 rng( rd() );
	std::uniform_int_distribution< std::uint16_t > dist( 0, state.range( 0 ) );

	SparseSet< std::uint16_t > set;

	for( auto _ : state )
	{
		benchmark::DoNotOptimize( set.insert( dist( rng ) ) );
	}
}
BENCHMARK( BM_SparseSet_insert )->RangeMultiplier( 32 )->Range( 0, 8 << 9 )->Complexity();

static void BM_SparseSet_erase( benchmark::State& state )
{
	std::mt19937 rng( rd() );
	std::uniform_int_distribution< std::uint16_t > dist( 0, state.range( 0 ) );

	auto set = createSet( state.range( 0 ) );

	for( auto _ : state )
	{
		benchmark::DoNotOptimize( set.erase( dist( rng ) ) );
	}
}
BENCHMARK( BM_SparseSet_erase )->RangeMultiplier( 32 )->Range( 0, 8 << 9 )->Complexity();

BENCHMARK_MAIN();
