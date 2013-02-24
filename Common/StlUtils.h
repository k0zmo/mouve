#pragma once

#include <algorithm>

namespace stlu
{
	// Comes pretty handy when `for range loops` are missing (VC10)
	template <class Iteratable, class Pred>
	inline void for_each(Iteratable& iterable, Pred pred)
	{
		std::for_each(iterable.begin(), iterable.end(), pred);
	}

	// Removes (all) items that satisfy given predicate from a generic container
	template <class Container, class Predicate>
	inline bool remove_pred(Container* container, Predicate pred)
	{
		auto it = std::remove_if(container->begin(), container->end(), pred);
		if(it != container->end())
		{
			container->erase(it, container->end());
			return true;
		}

		return false;
	}

	// Removes (all) items of given value from a container
	template <class Container>
	inline bool remove_value(Container* container,
	                         const typename Container::value_type& vt)
	{
		return remove_pred(container,
			[&](decltype(vt) it)
			{
				return vt == it;
			});
	}

	// Removes first item that satisfies given predicate from a generic container
	template <class Container, class Predicate>
	inline bool remove_first_pred(Container* container, Predicate pred)
	{
		for(auto it = container->begin() ; it != container->end(); ++it)
		{
			if(pred(*it))
			{
				container->erase(it);
				return true;
			}
		}

		return false;
	}

	// Removes first item of a given value from a container
	template <class Container>
	inline bool remove_first_value(Container* container,
	                               const typename Container::value_type& vt)
	{
		return remove_first_pred(container,
			[&](decltype(vt) it)
			{
				return vt == it;
			});
	}

	// Removes and deletes items that satisfy
	// given predicate from a generic container
	template <class Container, class Predicate, class DeleteFunc>
	inline void remove_delete(Container* container,
	                          Predicate pred, DeleteFunc deleteFunc)
	{
		for(auto it = container->begin(); it != container->end(); )
		{
			if(pred(*it))
			{
				deleteFunc(*it);
				it = container->erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	// Removes and deletes items that satisfy
	// given predicate from a generic container
	template <class Container, class Predicate>
	inline void remove_delete(Container* container, Predicate pred)
	{
		remove_delete(container, pred,
			[](typename Container::value_type v) { delete v; }
		);
	}

	// Removes items of given value from a container
	template <class Container>
	inline void remove_value_delete(Container* container,
	                                typename Container::value_type& vt)
	{
		remove_delete(container,
			[&](decltype(vt) it) { return vt == it; }
		);
	}

	// Applies releaseFunc on each item and clears the container
	template <class Container, class ReleaseFunc>
	inline void release(Container* container, ReleaseFunc releaseFunc)
	{
		for_each(*container, releaseFunc);
		container->clear();
	}

	// release with default ReleaseFunc
	template <class Container>
	inline void release(Container* container)
	{
		release(container,
			[](typename Container::value_type v) { delete v; }
		);
	}

	template <class Container>
	inline void release_map_values(Container* container)
	{
		release(container,
			[](typename Container::value_type v) { delete v.second; }
		);
	}

	// Adds unique value to the given container
	template <class Container>
	inline bool push_back_unique(Container* container,
	                             const typename Container::value_type& vt)
	{
		for(auto it = container->begin() ; it != container->end(); ++it)
		{
			if(*(it) == vt)
				return false;
		}

		container->push_back(vt);
		return true;
	}

	template <class Container, class Pred>
	inline bool contains_pred(const Container& container, Pred pred)
	{
		return std::find_if(container.begin(), container.end(), pred)
			!= container.end();
	}

	template <class Container>
	inline bool contains_value(const Container& container, 
		const typename Container::value_type& vt)
	{
		return std::find(std::begin(container), std::end(container), vt) 
			!= std::end(container);
	}

	template <class Map>
	inline typename Map::value_type::second_type find_in_map(
			const Map& map,
			const typename Map::value_type::first_type& key,
			const typename Map::value_type::second_type& defaultValue)
	{
		auto iter = map.find(key);
		if(iter != map.end())
		{
			return iter->second;
		}
		else
		{
			return defaultValue;
		}
	}
}
