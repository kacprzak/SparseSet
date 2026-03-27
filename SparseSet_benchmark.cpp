#include <benchmark/benchmark.h>

#include <SparseSet.hpp>
#include <cstdint>

static void BM_SparseSet_contains( benchmark::State& state )
{
	const auto& range = benchmark::CreateDenseRange( 0, state.range( 0 ), 16 );
	SparseSet< std::uint16_t > set{ range.begin(), range.end() };

	for( auto _ : state )
	{
		benchmark::DoNotOptimize( set.contains( state.range( 1 ) ) );
	}
}

BENCHMARK( BM_SparseSet_contains )->RangeMultiplier( 64 )->Ranges( { { 0, 8 << 9 }, { 8, 8 << 8 } } );
BENCHMARK_MAIN();
