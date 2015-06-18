#include <chrono>
#include <random>
#include <stdexcept>

#include "errorgen.hpp"

//RESILIENCE BASED ON MEMOIZATION: Error generator
unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
std::default_random_engine errorGenerator = std::default_random_engine( seed );
std::poisson_distribution<int> errorDistribution( 8 );

void gen_fail() {
    int random = errorDistribution( errorGenerator );
    if( random < 5 )
        throw std::runtime_error( "Random error." );
}

// Introduce errors only in created tasks, not in implicit tasks.
//if( wd.getParent() != NULL ) {
//    int random = _errorDistribution( _errorGenerator );
//    if( random < 5 )
//        throw std::runtime_error( "Random error." );
//}
