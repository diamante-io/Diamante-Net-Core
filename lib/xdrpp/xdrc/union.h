// -*- C++ -*-

#ifndef _UNION_H_
#define _UNION_H_ 1

/** \file union.h Support for unions of types with non-trivial
 * constructors/destructors.
 *
 * Example usage:
 * \code
 *   struct MyType {
 *     union {
 *       union_entry_base base;
 *       union_entry<string> s;
 *       union_entry<int> i;
 *     };
 *     MyType() : base() {}
 *     MyType(const MyType &m) : base(m.base) {}
 *     MyType(MyType &&m) : base(std::move(m.base)) {}
 *     ~MyType() { base.destroy(); }
 *     MyType &operator=(const MyType &m) {
 *       base = m.base;
 *       return *this;
 *     }
 *     MyType &operator=(MyType &&m) {
 *       base = std::move(m.base);
 *       return *this;
 *     }
 *   }
 *
 *   // ...
 *      MyType m;
 *      m.i = 5;
 *      m.s.select();
 *      if (m.s.empty() == true)
 *        // ...
 *
 * \endcode
 */

#include <exception>
#include <iostream>
#include <memory>
#include <new>
#include <typeinfo>

#ifndef UNION_COPY_CONSTRUCT
#define UNION_COPY_CONSTRUCT 1
#endif // !UNION_COPY_CONSTRUCT

//! The supertype of all \c union_entry types.  Provides a copy
//! constructor and move constructor that copy the currently selected
//! (active) member of the union.  Also allows the currently selected
//! member of the union to be destroyed via union_entry_base::destroy.
//!
//! Note you never actually need to use this type directly--it is fine
//! to have a union consisten of only union_entry.  However throwing a
//! \c union_entry_base into the union allows the union to be
//! initialized in a neutral state, in which all fields are
//! deselected.
class union_entry_base {
protected:
  void want_type(const std::type_info &wanted) const {
    const std::type_info &actual = typeid(*this);
    if (wanted != actual) {
      std::cerr << "union_entry: wanted = " << wanted.name()
		<< ", actual = " << actual.name() << std::endl;
      std::terminate();
    }
  }
#if UNION_COPY_CONSTRUCT
  virtual void copy_construct_to(union_entry_base *dest) const {
    new (static_cast<void *>(dest)) union_entry_base;
  }
#endif // UNION_COPY_CONSTRUCT
  virtual void move_construct_to(union_entry_base *dest) {
    new (static_cast<void *>(dest)) union_entry_base;
  }
public:
  union_entry_base() noexcept = default;
  //! Move construct currently active field.  It is a serious error to
  //! call this if the active field in the copied union is not a
  //! member of the constructed union.
  union_entry_base(union_entry_base &&ueb) { ueb.move_construct_to(this); }
#if UNION_COPY_CONSTRUCT
  //! Copy construct currently active field.  It is a serious error to
  //! call this if the active field in the copied union is not a
  //! member of the constructed union.
  union_entry_base(const union_entry_base &ueb) { ueb.copy_construct_to(this); }
  //! Select and assign active field.  Same warnings as for copy
  //! constructor.
  union_entry_base &operator=(const union_entry_base &ueb) {
    destroy();
    try { ueb.copy_construct_to(this); }
    catch(...) { new (static_cast<void *>(this)) union_entry_base; }
    return *this;
  }
#endif // UNION_COPY_CONSTRUCT
  //! Select and move active field.  Same warnings as for move
  //! constructor.
  union_entry_base &operator=(union_entry_base &&ueb) {
    destroy();
    try { ueb.move_construct_to(this); }
    catch(...) { new (static_cast<void *>(this)) union_entry_base; }
    return *this;
  }
  //! Never call this destructor.\ Call union_entry_base::destroy()
  //! instead.  Calling the destructor directly may lead to the wrong
  //! vtable being used.
  virtual ~union_entry_base() {}
  void destroy() volatile { this->~union_entry_base(); }
  //! Reset the union to a state where no field is selected.
  void deselect() {
    destroy();
    new (static_cast<void *>(this)) union_entry_base;
  };
};

//! A union entry for holding an optional type \c T.  The union entry
//! can be selected by calling union_entry::select, or simply by
//! assigning to it.  The union_entry acts like a pointer, even though
//! it holds the type \c T inline (without allocating any memory other
//! then memory allocated by <tt>T</tt>'s constructor).
template<typename T> class union_entry : public union_entry_base {
  T val_;

protected:
#if UNION_COPY_CONSTRUCT
  void copy_construct_to(union_entry_base *dest) const override {
    new (static_cast<void *>(dest)) union_entry{val_};
  }
#endif // UNION_COPY_CONSTRUCT
  void move_construct_to(union_entry_base *dest) override {
    new (static_cast<void *>(dest)) union_entry{std::move(val_)};
  }

public:
  //! Initializes the \c union_entry to type \c T, supplying \c ...a
  //! as the constructor arguments to \c T.
  template<typename ...A> union_entry(A&&...a)
    : val_(std::forward<A>(a)...) {}
  //! Selects \c T (if necessary), and assigns to it.
  template<typename TT> union_entry &operator=(TT &&tt) {
    return assign(std::forward<TT>(tt));
  }
#if UNION_COPY_CONSTRUCT
  union_entry(const union_entry &ue) : val_(*ue.get()) {}
  union_entry &operator=(const union_entry &ue) {
    ue.verify();
    return assign(*ue.get());
  }
#endif // !UNION_COPY_CONSTRUCT
  union_entry(union_entry &&ue) : val_(std::move(*ue.get())) {}
  union_entry &operator=(union_entry &&ue) {
    ue.verify();
    return assign(std::move(*ue.get()));
  }

  void verify() const { want_type(typeid(union_entry)); }

  //! Get a pointer to the enbedded \c T, or dump core if the wrong
  //! type is selected.
  T *get() { verify(); return &val_; }
  const T *get() const { verify(); return &val_; }
  operator T*() { return get(); }
  operator const T*() const { return get(); }
  T *operator->() { return get(); }
  const T *operator->() const { return get(); }
  //! Return the embedded \c T, or dump core if the wrong type is
  //! selected.
  T &operator*() { return *get(); }
  const T &operator*() const { return *get(); }

  //! Returns \c true iff this type is currently selected.
  bool constructed() const { return typeid(union_entry) == typeid(*this); }
  //! If the \c T is not the selected type, then destroy the currently
  //! selected type and select \c T, so that functions like
  //! union_entry::get subsequently work.
  T &select() {
    if (!constructed()) {
      destroy(); 
      try { new (static_cast<void *>(this)) union_entry; }
      catch(...) { new (static_cast<void *>(this)) union_entry_base; throw; }
    }
    return val_;
  }
  //! Destroy the currently selected type and construct a new \c T
  //! using the supplied constructor arguments.
  template<typename ...A> void emplace(A&&...a) {
    destroy();
    try { new (static_cast<void *>(this)) union_entry{std::forward<A>(a)...}; }
    catch(...) { new (static_cast<void *>(this)) union_entry_base; throw; }
  }
  //! Select \c T (if necessary) and assign to it.  Uses <tt>T</tt>'s
  //! copy or move constructor if \c T is not already selected,
  //! otherwise uses <tt>T</tt>'s \c operator= function.
  template<typename TT> union_entry &assign(TT &&tt) {
    if (constructed())
      *get() = std::forward<TT>(tt);
    else
      emplace(std::forward<TT>(tt));
    return *this;
  }
};

template<> class union_entry<void> : public union_entry_base {
public:
  bool constructed() const { return typeid(union_entry) == typeid(*this); }
  void select() {
    if (!constructed()) {
      destroy(); 
      new (static_cast<void *>(this)) union_entry;
    }
  }
};

//! Like a \c union_entry, but instead of storing a \c T, stores a \c
//! std::shared_ptr<T>.  Methods such as \c union_ptr::get return a
//! <TT>T *</tt> rather than a <tt>std::shared_ptr<T> *</tt>.  Use \c
//! union_ptr::get_ptr to get the actual \c std::shared_ptr<T>.
template<typename T> class union_ptr
  : public union_entry<std::shared_ptr<T>> {
  using super = union_entry<std::shared_ptr<T>>;
public:
  using super::super;

  //! Selects this pointer, but also constructs allocates a \c T.  See
  //! union_ptr::reset if you want to avoid alloting a \c T.
  void select() {
    if (!super::constructed()) {
      super::select();
      super::get()->reset(new T);
    }
  }
  //! Selects the underlying \c std::shared_ptr in the union (if
  //! necessary), then calls std::shared_ptr::reset on the supplied
  //! arguments.
  template<typename ...A> void reset(A&&...a) {
    if (!super::constructed())
      super::select();
    get_ptr().reset(std::forward<A>(a)...);
  }

  std::shared_ptr<T> &get_ptr() { return *super::get(); }
  const std::shared_ptr<T> &get_ptr() const { return *super::get(); }

  T *get() { return get_ptr().get(); }
  const T *get() const { return get_ptr().get(); }
  operator T*() { return get(); }
  operator const T*() const { return get(); }
  T *operator->() { return get(); }
  const T *operator->() const { return get(); }
  T &operator*() { return *get(); }
  const T &operator*() const { return *get(); }
};

#endif /* !_UNION_H_ */
