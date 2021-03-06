/*----------------------------------------------------------------------------
 *
 *   Copyright (C) 2016 - 2020 Antonio Augusto Alves Junior
 *
 *   This file is part of Hydra Data Analysis Framework.
 *
 *   Hydra is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Hydra is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Hydra.  If not, see <http://www.gnu.org/licenses/>.
 *
 *---------------------------------------------------------------------------*/

/*
 * Decays.inl
 *
 *  Created on: 05/08/2017
 *      Author: Antonio Augusto Alves Junior
 */

#ifndef DECAYS_INL_
#define DECAYS_INL_

/*
 template<size_t N, detail::Backend BACKEND>
 Decays<N, detail::BackendPolicy<BACKEND> >::

 */

#include<hydra/Sobol.h>
#include<hydra/Random.h>

namespace hydra {

namespace detail {

template<size_t N, typename Functor, typename ArgType>
struct EvalOnDaugthers: public hydra_thrust::unary_function<
		ArgType, GReal_t> {
	EvalOnDaugthers(Functor const& functor) :
			fFunctor(functor) {	}

	__hydra_host__  __hydra_device__
	EvalOnDaugthers(EvalOnDaugthers<N, Functor, ArgType> const&other) :
			fFunctor(other.fFunctor) {
	}

	//template<typename T>
	__hydra_host__  __hydra_device__
	GReal_t operator()(ArgType& value) {

		typename detail::tuple_type<N,Vector4R>::type particles= detail::dropFirst(value);
		return hydra_thrust::get<0>(value)
				* (fFunctor( particles));

	}

	Functor fFunctor;
};

struct FlagDaugthers1
{

	FlagDaugthers1(GReal_t max) :
		 fMax(max),
		 fSeed(0xd5a61266f0c9392c)
	{}

	FlagDaugthers1(GReal_t max,  size_t seed) :
		fMax(max),
		fSeed(seed)
	{}

	__hydra_host__  __hydra_device__
	FlagDaugthers1(FlagDaugthers1 const&other) :
		fMax(other.fMax),
		fSeed(other.fSeed)
	{}

	template<typename T>
	__hydra_host__  __hydra_device__
	bool operator()(T x) {

		size_t index  = hydra_thrust::get<0>(x);
		double weight = hydra_thrust::get<0>(hydra_thrust::get<1>(x));

		hydra::default_random_engine randEng(fSeed);

		randEng.discard(index);

		hydra_thrust::uniform_real_distribution<double> uniDist(0.0, fMax);

		return ( weight> uniDist(randEng)) ;

	}

	GReal_t fMax;
	size_t fSeed;
};

struct FlagDaugthers2
{

	FlagDaugthers2(GReal_t max) :
		 fMax(max),
		 fSeed(0xd5a61266f0c9392c)
	{}
	
	FlagDaugthers2(GReal_t max,  size_t seed) :
		fMax(max),
		fSeed(seed)
	{}

	__hydra_host__  __hydra_device__
	FlagDaugthers2(FlagDaugthers2 const&other) :
		fMax(other.fMax),
		fSeed(other.fSeed)
	{}

	template<typename T>
	__hydra_host__  __hydra_device__
	bool operator()(T x) {

		size_t index  = hydra_thrust::get<0>(x);
		double weight = hydra_thrust::get<1>(x);

		hydra::default_random_engine randEng(fSeed);

		randEng.discard(index);

		hydra_thrust::uniform_real_distribution<double> uniDist(0.0, fMax);

		return ( weight> uniDist(randEng)) ;

	}

	GReal_t fMax;
	size_t fSeed;
};


}  // namespace detail




template<size_t N, detail::Backend BACKEND>
hydra::Range<typename Decays<N, detail::BackendPolicy<BACKEND> >::iterator>
Decays<N, detail::BackendPolicy<BACKEND> >::Unweight(double max_weight, size_t seed)
{
	using hydra_thrust::system::detail::generic::select_system;
	typedef typename hydra_thrust::iterator_system<
			typename Decays<N, detail::BackendPolicy<BACKEND> >::iterator>::type system_t;
	/*
	 * NOTE: the implementation of this function is not the most efficient in terms
	 * of memory usage. Due probably a bug on thust_stable partition implementation
	 * connected with cuda and tbb, counting iterators can't be deployed as stencil.
	 * So...
	 */

	//number of events to trial
	size_t ntrials = this->size();


	//create iterators
	hydra_thrust::counting_iterator < size_t > first(0);
	hydra_thrust::counting_iterator < size_t > last(ntrials);

	auto sequence  = hydra_thrust::get_temporary_buffer<size_t>(system_t(), ntrials);
	hydra_thrust::copy(first, last, sequence.first);

	//get the maximum value
	GReal_t max_value = max_weight>0. ? max_weight:*(hydra_thrust::max_element(fWeights.begin(), fWeights.end()));

	//says if an event passed or not
	detail::FlagDaugthers1 predicate( max_value, seed);

	//re-sort the container to build up un-weighted sample
	auto start = hydra_thrust::make_zip_iterator(hydra_thrust::make_tuple(sequence.first, this->begin()));
	auto stop  = hydra_thrust::make_zip_iterator(hydra_thrust::make_tuple(sequence.first + sequence.second, this->end() ));

	auto middle = hydra_thrust::stable_partition(start, stop, predicate);

	auto end_of_range = hydra_thrust::distance(start, middle);

	hydra_thrust::return_temporary_buffer(system_t(), sequence.first  );

	//done!
	//return (size_t) hydra_thrust::distance(begin(), middle);
	return hydra::make_range(begin(), begin() + end_of_range );
}


template<size_t N, detail::Backend BACKEND>
template<typename FUNCTOR>
hydra::Range<typename Decays<N, detail::BackendPolicy<BACKEND> >::iterator>
Decays<N, detail::BackendPolicy<BACKEND> >::Unweight(FUNCTOR const& functor, double max_weight, size_t seed)
{

	/*
	 * NOTE: the implementation of this function is not the most efficient in terms
	 * of memory usage. Due probably a bug on thust_stable partition implementation
	 * connected with cuda and tbb, counting iterators can't be deployed as stencil.
	 * So...
	 */
	using hydra_thrust::system::detail::generic::select_system;
	typedef typename hydra_thrust::iterator_system<
			typename Decays<N, detail::BackendPolicy<BACKEND> >::const_iterator>::type system_t;

	//number of events to trial
	size_t ntrials = this->size();

	//create iterators
	hydra_thrust::counting_iterator < size_t > first(0);
	hydra_thrust::counting_iterator < size_t > last(ntrials);

	auto sequence  = hydra_thrust::get_temporary_buffer<size_t>(system_t(), ntrials);
	hydra_thrust::copy(first, last, sequence.first);

	//--------------------
	auto values = hydra_thrust::get_temporary_buffer <GReal_t> (system_t(), ntrials);


	detail::EvalOnDaugthers<N, FUNCTOR,
		typename Decays<N, detail::BackendPolicy<BACKEND> >::value_type> predicate1(functor);

	hydra_thrust::copy(system_t(),
			hydra_thrust::make_transform_iterator(this->begin(), predicate1),
			hydra_thrust::make_transform_iterator(this->end(),predicate1),
			values.first);

	GReal_t max_value = max_weight>0.0 ? max_weight: *(hydra_thrust::max_element(values.first,
			values.first + values.second));

	//says if an event passed or not
	detail::FlagDaugthers2 predicate2( max_value, seed);


	//re-sort the container to build up un-weighted sample
	auto start  = hydra_thrust::make_zip_iterator(hydra_thrust::make_tuple(sequence.first,
			values.first,this->begin()));
	auto stop   = hydra_thrust::make_zip_iterator(hydra_thrust::make_tuple(sequence.first + sequence.second,
			values.first + values.second, this->end() ));

	auto middle = hydra_thrust::stable_partition(start, stop, predicate2);

	auto end_of_range = hydra_thrust::distance(start, middle);

	hydra_thrust::return_temporary_buffer(system_t(), sequence.first  );
	hydra_thrust::return_temporary_buffer(system_t(), values.first);

	//done!

	return hydra::make_range(begin(), begin()+end_of_range);

}

template<size_t N, detail::Backend BACKEND>
template<typename FUNCTOR>
void Decays<N, detail::BackendPolicy<BACKEND> >::Reweight(FUNCTOR const& functor)
{

	using hydra_thrust::system::detail::generic::select_system;
	typedef typename hydra_thrust::iterator_system<
			typename Decays<N, detail::BackendPolicy<BACKEND> >::const_iterator>::type system_t;

	detail::EvalOnDaugthers<N, FUNCTOR,
			typename Decays<N, detail::BackendPolicy<BACKEND> >::value_type> predicate1(
			functor);

	hydra_thrust::copy(system_t(),
			hydra_thrust::make_transform_iterator(this->begin(),
					predicate1),
			hydra_thrust::make_transform_iterator(this->end(),
					predicate1), fWeights.begin());

	return;

}

//=======================

template<size_t N1, hydra::detail::Backend BACKEND1, size_t N2,
		hydra::detail::Backend BACKEND2>
bool operator==(const Decays<N1, hydra::detail::BackendPolicy<BACKEND1> >& lhs,
		const Decays<N2, hydra::detail::BackendPolicy<BACKEND2> >& rhs) {

	bool is_same_type = (N1 == N2)
			&& hydra_thrust::detail::is_same<
					hydra::detail::BackendPolicy<BACKEND1>,
					hydra::detail::BackendPolicy<BACKEND2> >::value
			&& lhs.size() == rhs.size();
	bool result = 1;

	auto comp = []__hydra_host__ __hydra_device__(hydra_thrust::tuple<
			typename Decays<N1, hydra::detail::BackendPolicy<BACKEND1>>::value_type,
			typename Decays<N2, hydra::detail::BackendPolicy<BACKEND2>>::value_type> const& values) {
		return hydra_thrust::get<0>(values)== hydra_thrust::get<1>(values);

	};

	if (is_same_type) {
		result = hydra_thrust::all_of(
				hydra_thrust::make_zip_iterator(lhs.begin(),
						rhs.begin()),
				hydra_thrust::make_zip_iterator(lhs.end(),
						rhs.end()), comp);
	}
	return result && is_same_type;

}

template<size_t N1, hydra::detail::Backend BACKEND1, size_t N2,
		hydra::detail::Backend BACKEND2>
bool operator!=(const Decays<N1, hydra::detail::BackendPolicy<BACKEND1> >& lhs,
		const Decays<N2, hydra::detail::BackendPolicy<BACKEND2> >& rhs) {

	bool is_same_type = (N1 == N2)
			&& hydra_thrust::detail::is_same<
					hydra::detail::BackendPolicy<BACKEND1>,
					hydra::detail::BackendPolicy<BACKEND2> >::value
			&& lhs.size() == rhs.size();
	bool result = 1;

	auto comp = []__hydra_host__ __hydra_device__(hydra_thrust::tuple<
			typename Decays<N1, hydra::detail::BackendPolicy<BACKEND1>>::value_type,
			typename Decays<N2, hydra::detail::BackendPolicy<BACKEND2>>::value_type> const& values) {
		return (hydra_thrust::get<0>(values) == hydra_thrust::get<1>(values));

	};

	if (is_same_type) {
		result = hydra_thrust::all_of(
				hydra_thrust::make_zip_iterator(lhs.begin(),
						rhs.begin()),
				hydra_thrust::make_zip_iterator(lhs.end(),
						rhs.end()), comp);
	}
	return (!result) && is_same_type;
}

}  // namespace hydra

#endif /* DECAYS_INL_ */
