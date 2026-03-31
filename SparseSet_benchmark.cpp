#include <benchmark/benchmark.h>

#include <SparseSet.hpp>

#include <cstddef>
#include <cstdint>

namespace
{

auto createRandomSet( std::size_t size ) -> SparseSet< std::uint16_t >
{
	SparseSet< std::uint16_t > set;
	set.reserve( size );

	for( auto i = 0u; i < size; ++i )
		set.insert( std::rand() % size );

	return set;
}

class BM_SparseSet : public ::benchmark::Fixture
{
public:
	void SetUp( const ::benchmark::State& st ) override { set = createRandomSet( st.range( 0 ) ); }
	void TearDown( const ::benchmark::State& ) override { set.clear(); }

	SparseSet< std::uint16_t > set;
};

} // namespace

BENCHMARK_DEFINE_F( BM_SparseSet, contains )( benchmark::State& state )
{
	const auto size = state.range( 0 );

	for( auto _ : state )
	{
		for( auto i = 0u; i < size; ++i )
		{
			bool contains = set.contains( std::rand() % size );
			benchmark::DoNotOptimize( contains );
		}
	}

	state.SetItemsProcessed( state.iterations() * size );
	state.SetComplexityN( size );
}
BENCHMARK_REGISTER_F( BM_SparseSet, contains )->Range( 8, 8 << 10 )->Complexity( benchmark::o1 );

BENCHMARK_DEFINE_F( BM_SparseSet, insert )( benchmark::State& state )
{
	const auto size = state.range( 0 );

	for( auto _ : state )
	{
		for( auto i = 0u; i < size; ++i )
		{
			bool success = set.insert( std::rand() % size );
			benchmark::DoNotOptimize( success );
		}
	}

	state.SetItemsProcessed( state.iterations() * size );
	state.SetComplexityN( size );
}
BENCHMARK_REGISTER_F( BM_SparseSet, insert )->Range( 8, 8 << 10 )->Complexity( benchmark::o1 );

BENCHMARK_DEFINE_F( BM_SparseSet, erase )( benchmark::State& state )
{
	const auto size = state.range( 0 );

	for( auto _ : state )
	{
		for( auto i = 0u; i < size; ++i )
		{
			bool success = set.erase( std::rand() % size );
			benchmark::DoNotOptimize( success );
		}
	}

	state.SetItemsProcessed( state.iterations() * size );
	state.SetComplexityN( size );
}
BENCHMARK_REGISTER_F( BM_SparseSet, erase )->Range( 8, 8 << 10 )->Complexity( benchmark::o1 );

BENCHMARK_DEFINE_F( BM_SparseSet, sort )( benchmark::State& state )
{
	const auto size = state.range( 0 );

	for( auto _ : state )
	{
		set.sort();
	}

	state.SetComplexityN( size );
}
BENCHMARK_REGISTER_F( BM_SparseSet, sort )->Range( 8, 8 << 10 )->Complexity( benchmark::oNLogN );

BENCHMARK_MAIN();
