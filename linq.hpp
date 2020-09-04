/*
 *
 * Created by King (qpwakaba)
 *
 */

#ifndef _INCLUDE_GUARD_LINQ_HPP
#define _INCLUDE_GUARD_LINQ_HPP

// for size_t
#include <cstdlib>

// for std::declval
#include <utility>

#include <vector>
#include <algorithm>
#include <stdexcept>

namespace linq {
  /* Supports a simple iteration */
  template<typename T, typename FuncMoveNext>
    struct enumerator {
      public:
        /* Advances the enumerator to the next element */
        inline bool move_next() {
          return funcMoveNext(&_current);
        }
        /* Gets the elements at the current position */
        inline T current() const { return _current; }

        enumerator(FuncMoveNext fmn)
          : funcMoveNext(fmn) {}
      private:
        T _current;
        FuncMoveNext funcMoveNext;
    };

  /* prototype declaration because it is used from _ConstructEnumerable function */
  template<typename T, typename FuncEnumeratorGenerator>
  struct enumerable;

  /* construct an enumerable object */
  /* private helper method for template type inference */
  template<typename T, typename FuncEnumeratorGenerator>
  inline static enumerable<T, FuncEnumeratorGenerator> _ConstructEnumerable(FuncEnumeratorGenerator generator) {
    return enumerable<T, FuncEnumeratorGenerator>(generator);
  }

  /* Construct an enumerator object */
  /* private helper method for template type inference */
  template<typename T, typename FuncMoveNext>
  inline static enumerator<T, FuncMoveNext>
  _ConstructEnumerator(FuncMoveNext fmn) {
    return enumerator<T, FuncMoveNext>(fmn);
  }

  template<typename T, typename FuncEnumerator>
  inline static auto make_enumerable(FuncEnumerator func) {
    return _ConstructEnumerable<T>([=]() {
      return _ConstructEnumerator<T>([=](T* current) mutable {
        return func(current);
      });
    });
  }

  /* prototype declaration because it is used from enumerable::order_by function */
  template<typename Iterator>
  auto as_enumerable(const Iterator itr);

  /* Provides methods for querying objects */
  template<typename T, typename FuncEnumeratorGenerator>
  struct enumerable {
    public:
      /* Project each element of a sequence into a new form */
      template<typename Func>
      inline auto select(Func selector) {
        auto enumeratorGenerator = this->funcEnumeratorGenerator;
        return _ConstructEnumerable<decltype(selector(std::declval<T>()))>([=]() -> auto {
          // call when GetEnumerator is called
          auto baseEnumerator = enumeratorGenerator();
          return _ConstructEnumerator<decltype(selector(std::declval<T>()))>([baseEnumerator, selector](auto current) mutable -> auto {
            // call when move_next is called
            // if succeed to move, set *current to next item, otherwise return false
            if (!baseEnumerator.move_next()) return false;
            *current = {selector(baseEnumerator.current())};
            return true;
          });
        });
      }
      /* Returns first element, if no elements are in the sequence, throw std::domain_error */
      inline auto first() {
        auto enumerator = funcEnumeratorGenerator();
        if (enumerator.move_next()) {
          return enumerator.current();
        }

        throw std::domain_error("No such element");
      }
      /* Filters the sequence of values based on a predicate */
      template<typename Func>
      inline auto where(Func predicate) {
        auto enumeratorGenerator = this->funcEnumeratorGenerator;
        return _ConstructEnumerable<decltype(enumeratorGenerator().current())>([=]() -> auto {
          // call when GetEnumerator is called
          auto baseEnumerator = enumeratorGenerator();
          return _ConstructEnumerator<decltype(enumeratorGenerator().current())>([=](auto current) mutable -> auto {
            // call when move_next is called
            // if succeed to move, set *current to next item, otherwise return false
            while (baseEnumerator.move_next()) {
              if (predicate(baseEnumerator.current())) {
                *current = baseEnumerator.current();
                return true;
              }
            }
            return false;
          });
        });
      }

      /* Returns a sequence that has the specified number of elements from start */
      inline auto take(size_t count) {
        auto enumeratorGenerator = this->funcEnumeratorGenerator;
        return _ConstructEnumerable<decltype(first())>([=]() -> auto {
          // call when GetEnumerator is called
          auto baseEnumerator = enumeratorGenerator();
          size_t i = 0;
          return _ConstructEnumerator<decltype(first())>([=](auto current) mutable -> auto {
            // call when move_next is called
            // if succeed to move, set *current to next item, otherwise return false
            if (i >= count) return false;
            i+=1;
            if (!baseEnumerator.move_next()) return false;
            *current = baseEnumerator.current();
            return true;
          });
        });
      }

      /* Sorts the elements of the sequence in ascending order according to the key */
      inline auto order_by() {
        std::vector<decltype(first())> buffer;
        for_each([&](auto t) -> auto { buffer.push_back(t); });
        std::sort(buffer.begin(), buffer.end());
        return as_enumerable(buffer).select([](auto p) -> auto { return *p; });
      }

      /* Sorts the elements of the sequence in ascending order according by using a specified comparer */
      template<typename Compare>
      inline auto order_by(Compare cmp) {
        std::vector<decltype(first())> buffer;
        for_each([&](auto t) -> auto { buffer.push_back(t); });
        std::sort(buffer.begin(), buffer.end(), cmp);
        return as_enumerable(buffer).select([](auto p) -> auto { return *p; });
      }

      /* Sorts the elements of the sequence in descending order according to the key */
      inline auto order_by_descending() {
        std::vector<decltype(first())> buffer;
        for_each([&](auto t) -> auto { buffer.push_back(t); });
        std::sort(buffer.begin(), buffer.end(), [](auto& x, auto& y) -> auto {
            return y < x;
        });
        return as_enumerable(buffer).select([](auto p) -> auto { return *p; });
      }

      /* Sorts the elements of the sequence in descending order according by using a specified comparer */
      template<typename Compare>
      inline auto order_by_descending(Compare cmp) {
        std::vector<decltype(first())> buffer;
        for_each([&](auto t) -> auto { buffer.push_back(t); });
        std::sort(buffer.begin(), buffer.end(), [cmp](auto& x, auto& y) -> auto { return cmp(y, x); });
        return as_enumerable(buffer).select([](auto p) -> auto { return *p; });
      }

      /* Performs the specified action on each element */
      template<typename Action>
      inline void for_each(Action action) {
        auto enumerator = funcEnumeratorGenerator();
        while (enumerator.move_next()) {
          action(enumerator.current());
        }
      }

      enumerable(FuncEnumeratorGenerator generator): funcEnumeratorGenerator(generator) {}
    private:
      const FuncEnumeratorGenerator funcEnumeratorGenerator;
  };

  /* Returns sequence of specified iterator */
  template<typename Iterator>
  inline auto as_enumerable(const Iterator itr) {
    return _ConstructEnumerable<decltype((itr.begin()))>([=]() -> auto {
      auto p = itr.begin();
      auto end = itr.end();
      return _ConstructEnumerator<decltype(p)>([=](auto current) mutable -> auto {
        if (p == end) return false;
        *current = p++;
        return true;
      });
    });//.select([](auto p) -> auto { return *p; });;
  }

  /* Returns sequence of array of specified size, at specified pointer */
  template<typename T>
  inline auto as_enumerable(const T* ptr, size_t size) {
    return _ConstructEnumerable<T>([=]() -> auto {
      size_t i = 0;
      return _ConstructEnumerator<T>([=](T* current) mutable -> auto {
        if (i >= size) return false;
        *current = ptr[i++];
        return true;
      });
    });
  }

  /* Generates sequence of integral number within a specified range */
  inline auto range(size_t start, size_t count) {
    return _ConstructEnumerable<size_t>([=]() -> auto {
      size_t i = 0;
      auto f = [=](size_t* current) mutable -> auto {
        if (i >= count) return false;
        *current = start + i++;
        return true;
      };
      return _ConstructEnumerator<size_t>(f);
    });
  }
}
#endif
