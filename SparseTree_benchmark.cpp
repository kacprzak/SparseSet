#include <benchmark/benchmark.h>

#include <SparseTree.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace
{

auto createRandomSet( std::size_t size )
{
	sparse::SparseTree< std::uint16_t, float > tree;
	tree.reserve( size );

	tree.insert( 0u, {} );

	for( auto i = 1u; i < size; ++i )
		tree.insert( i, i, std::rand() % tree.size() );

	return tree;
}

class BM_SparseTree : public ::benchmark::Fixture
{
public:
	void SetUp( const ::benchmark::State& st ) override { tree = createRandomSet( st.range( 0 ) ); }
	void TearDown( const ::benchmark::State& ) override { tree.clear(); }

	sparse::SparseTree< std::uint16_t, float > tree;
};

} // namespace

BENCHMARK_DEFINE_F( BM_SparseTree, erase )( benchmark::State& state )
{
	const auto size = state.range( 0 );

	for( auto _ : state )
	{
		for( auto i = 0u; i < size; ++i )
		{
			bool success = tree.erase( std::rand() % size );
			benchmark::DoNotOptimize( success );
		}
	}

	state.SetItemsProcessed( state.iterations() * size );
	state.SetComplexityN( size );
}
BENCHMARK_REGISTER_F( BM_SparseTree, erase )->Range( 8, 8 << 10 )->Complexity();

BENCHMARK_DEFINE_F( BM_SparseTree, for_each_bfs )( benchmark::State& state )
{
	const auto size = state.range( 0 );

	for( auto _ : state )
	{
		float sum = 0.f;
		tree.for_each_bfs( [ &sum ]( const auto& kv ) { sum += kv.second; } );
		benchmark::DoNotOptimize( sum );
	}

	state.SetItemsProcessed( state.iterations() * size );
	state.SetComplexityN( size );
}
BENCHMARK_REGISTER_F( BM_SparseTree, for_each_bfs )->Range( 8, 8 << 10 )->Complexity();
