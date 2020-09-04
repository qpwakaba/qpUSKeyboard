#pragma once

namespace std {
  template<typename T>
  struct remove_reference {
    typedef T type;
  };
  template<typename T>
  struct remove_reference<T&> {
    typedef T type;
  };
  template<typename T>
  struct remove_reference<T&&> {
    typedef T type;
  };

  template<typename T>
  inline constexpr typename std::remove_reference<T>::type&& move(T&& t) noexcept {
    return static_cast<typename std::remove_reference<T>::type&&>(t);
  }
}

template<class TFunc>
class Finally {
  TFunc fun;
  bool destructed = false;
public:
  Finally(TFunc fun) : fun(fun) {}
  Finally(const Finally& copy) = delete;
  Finally(Finally&& move)
  : fun(std::move(move.fun))
  , destructed(move.destructed) {
    move.destructed = true;
  }
  ~Finally() {
    if (!destructed) {
      fun();
      destructed = true;
    }
  }
};

template<class TFunc>
inline auto finally(TFunc&& func) {
  return Finally<TFunc>(std::move(func));
}

