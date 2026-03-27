#include <benchmark/benchmark.h>

#include <SparseSet.hpp>
#include <cstdint>

static void BM_SparseSet_contains( benchmark::State& state )
{
	const auto& range = benchmark::CreateDenseRange( 0, 8 << 10, 16 );
	SparseSet< std::uint16_t > set{ range.begin(), range.end() };

	for( auto _ : state )
	{
		benchmark::DoNotOptimize( set.contains( state.range( 0 ) ) );
	}
}

BENCHMARK( BM_SparseSet_contains )->Range( 8, 4096 );
BENCHMARK_MAIN();
