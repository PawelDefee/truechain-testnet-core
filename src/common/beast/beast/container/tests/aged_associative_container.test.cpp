//------------------------------------------------------------------------------
/*
    This file is part of Beast: https://github.com/vinniefalco/Beast
    Copyright 2013, Vinnie Falco <vinnie.falco@gmail.com>

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <beast/chrono/manual_clock.h>
#include <beast/unit_test/suite.h>

#include <beast/container/aged_set.h>
#include <beast/container/aged_map.h>
#include <beast/container/aged_multiset.h>
#include <beast/container/aged_multimap.h>
#include <beast/container/aged_unordered_set.h>
#include <beast/container/aged_unordered_map.h>
#include <beast/container/aged_unordered_multiset.h>
#include <beast/container/aged_unordered_multimap.h>

#include <vector>
#include <list>

#ifndef BEAST_AGED_UNORDERED_NO_ALLOC_DEFAULTCTOR
# ifdef _MSC_VER
#  define BEAST_AGED_UNORDERED_NO_ALLOC_DEFAULTCTOR 0
# else
#  define BEAST_AGED_UNORDERED_NO_ALLOC_DEFAULTCTOR 1
# endif
#endif

#ifndef BEAST_CONTAINER_EXTRACT_NOREF
# ifdef _MSC_VER
#  define BEAST_CONTAINER_EXTRACT_NOREF 1
# else
#  define BEAST_CONTAINER_EXTRACT_NOREF 1
# endif
#endif

namespace beast {

class aged_associative_container_test_base : public unit_test::suite
{
public:
    template <class T>
    struct CompT
    {
        explicit CompT (int)
        {
        }

        CompT (CompT const&)
        {
        }

        bool operator() (T const& lhs, T const& rhs) const
        {
            return m_less (lhs, rhs);
        }

    private:
        CompT () = delete;
        std::less <T> m_less;
    };

    template <class T>
    class HashT
    {
    public:
        explicit HashT (int)
        {
        }

        std::size_t operator() (T const& t) const
        {
            return m_hash (t);
        }

    private:
        HashT() = delete;
        std::hash <T> m_hash;
    };

    template <class T>
    struct EqualT
    {
    public:
        explicit EqualT (int)
        {
        }

        bool operator() (T const& lhs, T const& rhs) const
        {
            return m_eq (lhs, rhs);
        }

    private:
        EqualT() = delete;
        std::equal_to <T> m_eq;
    };

    template <class T>
    struct AllocT
    {
        typedef T value_type;

        //typedef propagate_on_container_swap : std::true_type::type;

        template <class U>
        struct rebind
        {
            typedef AllocT <U> other;
        };

        explicit AllocT (int)
        {
        }

        AllocT (AllocT const&)
        {
        }

        template <class U>
        AllocT (AllocT <U> const&)
        {
        }

        template <class U>
        bool operator== (AllocT <U> const&) const
        {
            return true;
        }

        T* allocate (std::size_t n, T const* = 0)
        {
            return static_cast <T*> (
                ::operator new (n * sizeof(T)));
        }

        void deallocate (T* p, std::size_t)
        {
            ::operator delete (p);
        }

#if ! BEAST_AGED_UNORDERED_NO_ALLOC_DEFAULTCTOR
        AllocT ()
        {
        }
#else
    private:
        AllocT() = delete;
#endif
    };

    //--------------------------------------------------------------------------

    // ordered
    template <class Base, bool IsUnordered>
    class MaybeUnordered : public Base
    {
    public:
        typedef std::less <typename Base::Key> Comp;
        typedef CompT <typename Base::Key> MyComp;

    protected:
        static std::string name_ordered_part()
        {
            return "";
        }
    };

    // unordered
    template <class Base>
    class MaybeUnordered <Base, true> : public Base
    {
    public:
        typedef std::hash <typename Base::Key> Hash;
        typedef std::equal_to <typename Base::Key> Equal;
        typedef HashT <typename Base::Key> MyHash;
        typedef EqualT <typename Base::Key> MyEqual;

    protected:
        static std::string name_ordered_part()
        {
            return "unordered_";
        }
    };

    // unique
    template <class Base, bool IsMulti>
    class MaybeMulti : public Base
    {
    public:
    protected:
        static std::string name_multi_part()
        {
            return "";
        }
    };

    // multi
    template <class Base>
    class MaybeMulti <Base, true> : public Base
    {
    public:
    protected:
        static std::string name_multi_part()
        {
            return "multi";
        }
    };

    // set
    template <class Base, bool IsMap>
    class MaybeMap : public Base
    {
    public:
        typedef void T;
        typedef typename Base::Key Value;
        typedef std::vector <Value> Values;

        static typename Base::Key const& extract (Value const& value)
        {
            return value;
        }

        static Values values()
        {
            Values v {
                "apple",
                "banana",
                "cherry",
                "grape",
                "orange",
            };
            return v;
        };

    protected:
        static std::string name_map_part()
        {
            return "set";
        }
    };

    // map
    template <class Base>
    class MaybeMap <Base, true> : public Base
    {
    public:
        typedef int T;
        typedef std::pair <typename Base::Key, T> Value;
        typedef std::vector <Value> Values;

        static typename Base::Key const& extract (Value const& value)
        {
            return value.first;
        }

        static Values values()
        {
            Values v {
                std::make_pair ("apple",  1),
                std::make_pair ("banana", 2),
                std::make_pair ("cherry", 3),
                std::make_pair ("grape",  4),
                std::make_pair ("orange", 5)
            };
            return v;
        };

    protected:
        static std::string name_map_part()
        {
            return "map";
        }
    };

    //--------------------------------------------------------------------------

    // ordered
    template <
        class Base,
        bool IsUnordered = Base::is_unordered::value
    >
    struct ContType
    {
        template <
            class Compare = std::less <typename Base::Key>,
            class Allocator = std::allocator <typename Base::Value>
        >
        using Cont = detail::aged_ordered_container <
            Base::is_multi::value, Base::is_map::value, typename Base::Key,
                typename Base::T, typename Base::Clock, Compare, Allocator>;
    };

    // unordered
    template <
        class Base
    >
    struct ContType <Base, true>
    {
        template <
            class Hash = std::hash <typename Base::Key>,
            class KeyEqual = std::equal_to <typename Base::Key>,
            class Allocator = std::allocator <typename Base::Value>
        >
        using Cont = detail::aged_unordered_container <
            Base::is_multi::value, Base::is_map::value,
                typename Base::Key, typename Base::T, typename Base::Clock,
                    Hash, KeyEqual, Allocator>;
    };

    //--------------------------------------------------------------------------

    struct TestTraitsBase
    {
        typedef std::string Key;
        typedef std::chrono::steady_clock Clock;
        typedef manual_clock<Clock> ManualClock;
    };

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    struct TestTraitsHelper
        : MaybeUnordered <MaybeMulti <MaybeMap <
            TestTraitsBase, IsMap>, IsMulti>, IsUnordered>
    {
    private:
        typedef MaybeUnordered <MaybeMulti <MaybeMap <
            TestTraitsBase, IsMap>, IsMulti>, IsUnordered> Base;

    public:
        using typename Base::Key;

        typedef std::integral_constant <bool, IsUnordered> is_unordered;
        typedef std::integral_constant <bool, IsMulti> is_multi;
        typedef std::integral_constant <bool, IsMap> is_map;

        typedef std::allocator <typename Base::Value> Alloc;
        typedef AllocT <typename Base::Value> MyAlloc;

        static std::string name()
        {
            return std::string ("aged_") +
                Base::name_ordered_part() +
                    Base::name_multi_part() +
                        Base::name_map_part();
        }
    };

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    struct TestTraits
        : TestTraitsHelper <IsUnordered, IsMulti, IsMap>
        , ContType <TestTraitsHelper <IsUnordered, IsMulti, IsMap>>
    {
    };

    template <class Cont>
    static std::string name (Cont const&)
    {
        return TestTraits <
            Cont::is_unordered,
            Cont::is_multi,
            Cont::is_map>::name();
    }

    template <class Traits>
    struct equal_value
    {
        bool operator() (typename Traits::Value const& lhs,
            typename Traits::Value const& rhs)
        {
            return Traits::extract (lhs) == Traits::extract (rhs);
        }
    };

    template <class Cont>
    static
    std::vector <typename Cont::value_type>
    make_list (Cont const& c)
    {
        return std::vector <typename Cont::value_type> (
            c.begin(), c.end());
    }

    //--------------------------------------------------------------------------

    template <
        class Container,
        class Values
    >
    typename std::enable_if <
        Container::is_map::value && ! Container::is_multi::value>::type
    checkMapContents (Container& c, Values const& v);

    template <
        class Container,
        class Values
    >
    typename std::enable_if <!
        (Container::is_map::value && ! Container::is_multi::value)>::type
    checkMapContents (Container, Values const&)
    {
    }

    // unordered
    template <
        class C,
        class Values
    >
    typename std::enable_if <
        std::remove_reference <C>::type::is_unordered::value>::type
    checkUnorderedContentsRefRef (C&& c, Values const& v);

    template <
        class C,
        class Values
    >
    typename std::enable_if <!
        std::remove_reference <C>::type::is_unordered::value>::type
    checkUnorderedContentsRefRef (C&&, Values const&)
    {
    }

    template <class C, class Values>
    void checkContentsRefRef (C&& c, Values const& v);

    template <class Cont, class Values>
    void checkContents (Cont& c, Values const& v);

    template <class Cont>
    void checkContents (Cont& c);

    //--------------------------------------------------------------------------

    // ordered
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <! IsUnordered>::type
    testConstructEmpty ();

    // unordered
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <IsUnordered>::type
    testConstructEmpty ();

    // ordered
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <! IsUnordered>::type
    testConstructRange ();

    // unordered
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <IsUnordered>::type
    testConstructRange ();

    // ordered
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <! IsUnordered>::type
    testConstructInitList ();

    // unordered
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <IsUnordered>::type
    testConstructInitList ();

    //--------------------------------------------------------------------------

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    void
    testCopyMove ();

    //--------------------------------------------------------------------------

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    void
    testIterator ();

    // Unordered containers don't have reverse iterators
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <! IsUnordered>::type
    testReverseIterator();

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <IsUnordered>::type
    testReverseIterator()
    {
    }

    //--------------------------------------------------------------------------

    template <class Container, class Values>
    void checkInsertCopy (Container& c, Values const& v);

    template <class Container, class Values>
    void checkInsertMove (Container& c, Values const& v);

    template <class Container, class Values>
    void checkInsertHintCopy (Container& c, Values const& v);

    template <class Container, class Values>
    void checkInsertHintMove (Container& c, Values const& v);

    template <class Container, class Values>
    void checkEmplace (Container& c, Values const& v);

    template <class Container, class Values>
    void checkEmplaceHint (Container& c, Values const& v);

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    void testModifiers();

    //--------------------------------------------------------------------------

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    void
    testChronological ();

    //--------------------------------------------------------------------------

    // map, unordered_map
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <IsMap && ! IsMulti>::type
    testArrayCreate();

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <! (IsMap && ! IsMulti)>::type
    testArrayCreate()
    {
    }

    //--------------------------------------------------------------------------

    // Helpers for erase tests
    template <class Container, class Values>
    void reverseFillAgedContainer(Container& c, Values const& v);

    template <class Iter>
    Iter nextToEndIter (Iter const beginIter, Iter const endItr);

   //--------------------------------------------------------------------------

    template <class Container, class Iter>
    bool doElementErase (Container& c, Iter const beginItr, Iter const endItr);

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    void testElementErase();

    //--------------------------------------------------------------------------

    template <class Container, class BeginEndSrc>
    void doRangeErase (Container& c, BeginEndSrc const& beginEndSrc);

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    void testRangeErase();

    //--------------------------------------------------------------------------

    // ordered
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <! IsUnordered>::type
    testCompare ();

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <IsUnordered>::type
    testCompare ()
    {
    }

    //--------------------------------------------------------------------------

    // ordered
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <! IsUnordered>::type
    testObservers();

    // unordered
    template <bool IsUnordered, bool IsMulti, bool IsMap>
    typename std::enable_if <IsUnordered>::type
    testObservers();

    //--------------------------------------------------------------------------

    template <bool IsUnordered, bool IsMulti, bool IsMap>
    void testMaybeUnorderedMultiMap ();

    template <bool IsUnordered, bool IsMulti>
    void testMaybeUnorderedMulti();

    template <bool IsUnordered>
    void testMaybeUnordered();
};

//------------------------------------------------------------------------------

// Check contents via at() and operator[]
// map, unordered_map
template <
    class Container,
    class Values
>
typename std::enable_if <
    Container::is_map::value && ! Container::is_multi::value>::type
aged_associative_container_test_base::
checkMapContents (Container& c, Values const& v)
{
    if (v.empty())
    {
        expect (c.empty());
        expect (c.size() == 0);
        return;
    }

    try
    {
        // Make sure no exception is thrown
        for (auto const& e : v)
            c.at (e.first);
        for (auto const& e : v)
            expect (c.operator[](e.first) == e.second);
    }
    catch (std::out_of_range const&)
    {
        fail ("caught exception");
    }
}

// unordered
template <
    class C,
    class Values
>
typename std::enable_if <
    std::remove_reference <C>::type::is_unordered::value>::type
aged_associative_container_test_base::
checkUnorderedContentsRefRef (C&& c, Values const& v)
{
    typedef typename std::remove_reference <C>::type Cont;
    typedef TestTraits <
        Cont::is_unordered::value,
            Cont::is_multi::value,
                Cont::is_map::value
                > Traits;
    typedef typename Cont::size_type size_type;
    auto const hash (c.hash_function());
    auto const key_eq (c.key_eq());
    for (size_type i (0); i < c.bucket_count(); ++i)
    {
        auto const last (c.end(i));
        for (auto iter (c.begin (i)); iter != last; ++iter)
        {
            auto const match (std::find_if (v.begin(), v.end(),
                [iter](typename Values::value_type const& e)
                {
                    return Traits::extract (*iter) ==
                        Traits::extract (e);
                }));
            expect (match != v.end());
            expect (key_eq (Traits::extract (*iter),
                Traits::extract (*match)));
            expect (hash (Traits::extract (*iter)) ==
                hash (Traits::extract (*match)));
        }
    }
}

template <class C, class Values>
void
aged_associative_container_test_base::
checkContentsRefRef (C&& c, Values const& v)
{
    typedef typename std::remove_reference <C>::type Cont;
    typedef TestTraits <
        Cont::is_unordered::value,
            Cont::is_multi::value,
                Cont::is_map::value
                > Traits;
    typedef typename Cont::size_type size_type;

    expect (c.size() == v.size());
    expect (size_type (std::distance (
        c.begin(), c.end())) == v.size());
    expect (size_type (std::distance (
        c.cbegin(), c.cend())) == v.size());
    expect (size_type (std::distance (
        c.chronological.begin(), c.chronological.end())) == v.size());
    expect (size_type (std::distance (
        c.chronological.cbegin(), c.chronological.cend())) == v.size());
    expect (size_type (std::distance (
        c.chronological.rbegin(), c.chronological.rend())) == v.size());
    expect (size_type (std::distance (
        c.chronological.crbegin(), c.chronological.crend())) == v.size());

    checkUnorderedContentsRefRef (c, v);
}

template <class Cont, class Values>
void
aged_associative_container_test_base::
checkContents (Cont& c, Values const& v)
{
    checkContentsRefRef (c, v);
    checkContentsRefRef (const_cast <Cont const&> (c), v);
    checkMapContents (c, v);
}

template <class Cont>
void
aged_associative_container_test_base::
checkContents (Cont& c)
{
    typedef TestTraits <
        Cont::is_unordered::value,
            Cont::is_multi::value,
                Cont::is_map::value
                > Traits;
    typedef typename Traits::Values Values;
    checkContents (c, Values());
}

//------------------------------------------------------------------------------
//
// Construction
//
//------------------------------------------------------------------------------

// ordered
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <! IsUnordered>::type
aged_associative_container_test_base::
testConstructEmpty ()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;
    typedef typename Traits::Value Value;
    typedef typename Traits::Key Key;
    typedef typename Traits::T T;
    typedef typename Traits::Clock Clock;
    typedef typename Traits::Comp Comp;
    typedef typename Traits::Alloc Alloc;
    typedef typename Traits::MyComp MyComp;
    typedef typename Traits::MyAlloc MyAlloc;
    typename Traits::ManualClock clock;

    //testcase (Traits::name() + " empty");
    testcase ("empty");

    {
        typename Traits::template Cont <Comp, Alloc> c (
            clock);
        checkContents (c);
    }

    {
        typename Traits::template Cont <MyComp, Alloc> c (
            clock, MyComp(1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <Comp, MyAlloc> c (
            clock, MyAlloc(1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <MyComp, MyAlloc> c (
            clock, MyComp(1), MyAlloc(1));
        checkContents (c);
    }
}

// unordered
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <IsUnordered>::type
aged_associative_container_test_base::
testConstructEmpty ()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;
    typedef typename Traits::Value Value;
    typedef typename Traits::Key Key;
    typedef typename Traits::T T;
    typedef typename Traits::Clock Clock;
    typedef typename Traits::Hash Hash;
    typedef typename Traits::Equal Equal;
    typedef typename Traits::Alloc Alloc;
    typedef typename Traits::MyHash MyHash;
    typedef typename Traits::MyEqual MyEqual;
    typedef typename Traits::MyAlloc MyAlloc;
    typename Traits::ManualClock clock;

    //testcase (Traits::name() + " empty");
    testcase ("empty");
    {
        typename Traits::template Cont <Hash, Equal, Alloc> c (
            clock);
        checkContents (c);
    }

    {
        typename Traits::template Cont <MyHash, Equal, Alloc> c (
            clock, MyHash(1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <Hash, MyEqual, Alloc> c (
            clock, MyEqual (1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <Hash, Equal, MyAlloc> c (
            clock, MyAlloc (1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <MyHash, MyEqual, Alloc> c (
            clock, MyHash(1), MyEqual(1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <MyHash, Equal, MyAlloc> c (
            clock, MyHash(1), MyAlloc(1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <Hash, MyEqual, MyAlloc> c (
            clock, MyEqual(1), MyAlloc(1));
        checkContents (c);
    }

    {
        typename Traits::template Cont <MyHash, MyEqual, MyAlloc> c (
            clock, MyHash(1), MyEqual(1), MyAlloc(1));
        checkContents (c);
    }
}

// ordered
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <! IsUnordered>::type
aged_associative_container_test_base::
testConstructRange ()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;
    typedef typename Traits::Value Value;
    typedef typename Traits::Key Key;
    typedef typename Traits::T T;
    typedef typename Traits::Clock Clock;
    typedef typename Traits::Comp Comp;
    typedef typename Traits::Alloc Alloc;
    typedef typename Traits::MyComp MyComp;
    typedef typename Traits::MyAlloc MyAlloc;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());

    //testcase (Traits::name() + " range");
    testcase ("range");

    {
        typename Traits::template Cont <Comp, Alloc> c (
            v.begin(), v.end(),
            clock);
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <MyComp, Alloc> c (
            v.begin(), v.end(),
            clock, MyComp(1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <Comp, MyAlloc> c (
            v.begin(), v.end(),
            clock, MyAlloc(1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <MyComp, MyAlloc> c (
            v.begin(), v.end(),
            clock, MyComp(1), MyAlloc(1));
        checkContents (c, v);

    }

    // swap

    {
        typename Traits::template Cont <Comp, Alloc> c1 (
            v.begin(), v.end(),
            clock);
        typename Traits::template Cont <Comp, Alloc> c2 (
            clock);
        std::swap (c1, c2);
        checkContents (c2, v);
    }
}

// unordered
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <IsUnordered>::type
aged_associative_container_test_base::
testConstructRange ()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;
    typedef typename Traits::Value Value;
    typedef typename Traits::Key Key;
    typedef typename Traits::T T;
    typedef typename Traits::Clock Clock;
    typedef typename Traits::Hash Hash;
    typedef typename Traits::Equal Equal;
    typedef typename Traits::Alloc Alloc;
    typedef typename Traits::MyHash MyHash;
    typedef typename Traits::MyEqual MyEqual;
    typedef typename Traits::MyAlloc MyAlloc;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());

    //testcase (Traits::name() + " range");
    testcase ("range");

    {
        typename Traits::template Cont <Hash, Equal, Alloc> c (
            v.begin(), v.end(),
            clock);
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <MyHash, Equal, Alloc> c (
            v.begin(), v.end(),
            clock, MyHash(1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <Hash, MyEqual, Alloc> c (
            v.begin(), v.end(),
            clock, MyEqual (1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <Hash, Equal, MyAlloc> c (
            v.begin(), v.end(),
            clock, MyAlloc (1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <MyHash, MyEqual, Alloc> c (
            v.begin(), v.end(),
            clock, MyHash(1), MyEqual(1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <MyHash, Equal, MyAlloc> c (
            v.begin(), v.end(),
            clock, MyHash(1), MyAlloc(1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <Hash, MyEqual, MyAlloc> c (
            v.begin(), v.end(),
            clock, MyEqual(1), MyAlloc(1));
        checkContents (c, v);
    }

    {
        typename Traits::template Cont <MyHash, MyEqual, MyAlloc> c (
            v.begin(), v.end(),
            clock, MyHash(1), MyEqual(1), MyAlloc(1));
        checkContents (c, v);
    }
}

// ordered
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <! IsUnordered>::type
aged_associative_container_test_base::
testConstructInitList ()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;
    typedef typename Traits::Value Value;
    typedef typename Traits::Key Key;
    typedef typename Traits::T T;
    typedef typename Traits::Clock Clock;
    typedef typename Traits::Comp Comp;
    typedef typename Traits::Alloc Alloc;
    typedef typename Traits::MyComp MyComp;
    typedef typename Traits::MyAlloc MyAlloc;
    typename Traits::ManualClock clock;

    //testcase (Traits::name() + " init-list");
    testcase ("init-list");

    //  TODO

    pass();
}

// unordered
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <IsUnordered>::type
aged_associative_container_test_base::
testConstructInitList ()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;
    typedef typename Traits::Value Value;
    typedef typename Traits::Key Key;
    typedef typename Traits::T T;
    typedef typename Traits::Clock Clock;
    typedef typename Traits::Hash Hash;
    typedef typename Traits::Equal Equal;
    typedef typename Traits::Alloc Alloc;
    typedef typename Traits::MyHash MyHash;
    typedef typename Traits::MyEqual MyEqual;
    typedef typename Traits::MyAlloc MyAlloc;
    typename Traits::ManualClock clock;

    //testcase (Traits::name() + " init-list");
    testcase ("init-list");

    //  TODO
    pass();
}

//------------------------------------------------------------------------------
//
// Copy/Move construction and assign
//
//------------------------------------------------------------------------------

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
aged_associative_container_test_base::
testCopyMove ()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;
    typedef typename Traits::Value Value;
    typedef typename Traits::Alloc Alloc;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());

    //testcase (Traits::name() + " copy/move");
    testcase ("copy/move");

    // copy

    {
        typename Traits::template Cont <> c (
            v.begin(), v.end(), clock);
        typename Traits::template Cont <> c2 (c);
        checkContents (c, v);
        checkContents (c2, v);
        expect (c == c2);
        unexpected (c != c2);
    }

    {
        typename Traits::template Cont <> c (
            v.begin(), v.end(), clock);
        typename Traits::template Cont <> c2 (c, Alloc());
        checkContents (c, v);
        checkContents (c2, v);
        expect (c == c2);
        unexpected (c != c2);
    }

    {
        typename Traits::template Cont <> c (
            v.begin(), v.end(), clock);
        typename Traits::template Cont <> c2 (
            clock);
        c2 = c;
        checkContents (c, v);
        checkContents (c2, v);
        expect (c == c2);
        unexpected (c != c2);
    }

    // move

    {
        typename Traits::template Cont <> c (
            v.begin(), v.end(), clock);
        typename Traits::template Cont <> c2 (
            std::move (c));
        checkContents (c2, v);
    }

    {
        typename Traits::template Cont <> c (
            v.begin(), v.end(), clock);
        typename Traits::template Cont <> c2 (
            std::move (c), Alloc());
        checkContents (c2, v);
    }

    {
        typename Traits::template Cont <> c (
            v.begin(), v.end(), clock);
        typename Traits::template Cont <> c2 (
            clock);
        c2 = std::move (c);
        checkContents (c2, v);
    }
}

//------------------------------------------------------------------------------
//
// Iterator construction and assignment
//
//------------------------------------------------------------------------------

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
aged_associative_container_test_base::
testIterator()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;
    typedef typename Traits::Value Value;
    typedef typename Traits::Alloc Alloc;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());

    //testcase (Traits::name() + " iterators");
    testcase ("iterator");

    typename Traits::template Cont <> c {clock};

    using iterator = decltype (c.begin());
    using const_iterator = decltype (c.cbegin());

    // Should be able to construct or assign an iterator from an iterator.
    iterator nnIt_0 {c.begin()};
    iterator nnIt_1 {nnIt_0};
    expect (nnIt_0 == nnIt_1, "iterator constructor failed");
    iterator nnIt_2;
    nnIt_2 = nnIt_1;
    expect (nnIt_1 == nnIt_2, "iterator assignment failed");

    // Should be able to construct or assign a const_iterator from a
    // const_iterator.
    const_iterator ccIt_0 {c.cbegin()};
    const_iterator ccIt_1 {ccIt_0};
    expect (ccIt_0 == ccIt_1, "const_iterator constructor failed");
    const_iterator ccIt_2;
    ccIt_2 = ccIt_1;
    expect (ccIt_1 == ccIt_2, "const_iterator assignment failed");

    // Comparison between iterator and const_iterator is okay
    expect (nnIt_0 == ccIt_0,
        "Comparing an iterator to a const_iterator failed");
    expect (ccIt_1 == nnIt_1,
        "Comparing a const_iterator to an iterator failed");

    // Should be able to construct a const_iterator from an iterator.
    const_iterator ncIt_3 {c.begin()};
    const_iterator ncIt_4 {nnIt_0};
    expect (ncIt_3 == ncIt_4,
        "const_iterator construction from iterator failed");
    const_iterator ncIt_5;
    ncIt_5 = nnIt_2;
    expect (ncIt_5 == ncIt_4,
        "const_iterator assignment from iterator failed");

    // None of these should compile because they construct or assign to a
    // non-const iterator with a const_iterator.

//  iterator cnIt_0 {c.cbegin()};

//  iterator cnIt_1 {ccIt_0};

//  iterator cnIt_2;
//  cnIt_2 = ccIt_2;
}

template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <! IsUnordered>::type
aged_associative_container_test_base::
testReverseIterator()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;
    typedef typename Traits::Value Value;
    typedef typename Traits::Alloc Alloc;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());

    //testcase (Traits::name() + " reverse_iterators");
    testcase ("reverse_iterator");

    typename Traits::template Cont <> c {clock};

    using iterator = decltype (c.begin());
    using const_iterator = decltype (c.cbegin());
    using reverse_iterator = decltype (c.rbegin());
    using const_reverse_iterator = decltype (c.crbegin());

    // Naming decoder ring
    //       constructed from ------+ +----- constructed type
    //                              /\/\  -- character pairs
    //                              xAyBit
    //  r (reverse) or f (forward)--^-^
    //                               ^-^------ C (const) or N (non-const)

    // Should be able to construct or assign a reverse_iterator from a
    // reverse_iterator.
    reverse_iterator rNrNit_0 {c.rbegin()};
    reverse_iterator rNrNit_1 {rNrNit_0};
    expect (rNrNit_0 == rNrNit_1, "reverse_iterator constructor failed");
    reverse_iterator xXrNit_2;
    xXrNit_2 = rNrNit_1;
    expect (rNrNit_1 == xXrNit_2, "reverse_iterator assignment failed");

    // Should be able to construct or assign a const_reverse_iterator from a
    // const_reverse_iterator
    const_reverse_iterator rCrCit_0 {c.crbegin()};
    const_reverse_iterator rCrCit_1 {rCrCit_0};
    expect (rCrCit_0 == rCrCit_1, "reverse_iterator constructor failed");
    const_reverse_iterator xXrCit_2;
    xXrCit_2 = rCrCit_1;
    expect (rCrCit_1 == xXrCit_2, "reverse_iterator assignment failed");

    // Comparison between reverse_iterator and const_reverse_iterator is okay
    expect (rNrNit_0 == rCrCit_0,
        "Comparing an iterator to a const_iterator failed");
    expect (rCrCit_1 == rNrNit_1,
        "Comparing a const_iterator to an iterator failed");

    // Should be able to construct or assign a const_reverse_iterator from a
    // reverse_iterator
    const_reverse_iterator rNrCit_0 {c.rbegin()};
    const_reverse_iterator rNrCit_1 {rNrNit_0};
    expect (rNrCit_0 == rNrCit_1,
        "const_reverse_iterator construction from reverse_iterator failed");
    xXrCit_2 = rNrNit_1;
    expect (rNrCit_1 == xXrCit_2,
        "const_reverse_iterator assignment from reverse_iterator failed");

    // The standard allows these conversions:
    //  o reverse_iterator is explicitly constructible from iterator.
    //  o const_reverse_iterator is explicitly constructible from const_iterator.
    // Should be able to construct or assign reverse_iterators from
    // non-reverse iterators.
    reverse_iterator fNrNit_0 {c.begin()};
    const_reverse_iterator fNrCit_0 {c.begin()};
    expect (fNrNit_0 == fNrCit_0,
        "reverse_iterator construction from iterator failed");
    const_reverse_iterator fCrCit_0 {c.cbegin()};
    expect (fNrCit_0 == fCrCit_0,
        "const_reverse_iterator construction from const_iterator failed");

    // None of these should compile because they construct a non-reverse
    // iterator from a reverse_iterator.
//  iterator rNfNit_0 {c.rbegin()};
//  const_iterator rNfCit_0 {c.rbegin()};
//  const_iterator rCfCit_0 {c.crbegin()};

    // You should not be able to assign an iterator to a reverse_iterator or
    // vise-versa.  So the following lines should not compile.
    iterator xXfNit_0;
//  xXfNit_0 = xXrNit_2;
//  xXrNit_2 = xXfNit_0;
}

//------------------------------------------------------------------------------
//
// Modifiers
//
//------------------------------------------------------------------------------


template <class Container, class Values>
void
aged_associative_container_test_base::
checkInsertCopy (Container& c, Values const& v)
{
    for (auto const& e : v)
        c.insert (e);
    checkContents (c, v);
}

template <class Container, class Values>
void
aged_associative_container_test_base::
checkInsertMove (Container& c, Values const& v)
{
    Values v2 (v);
    for (auto& e : v2)
        c.insert (std::move (e));
    checkContents (c, v);
}

template <class Container, class Values>
void
aged_associative_container_test_base::
checkInsertHintCopy (Container& c, Values const& v)
{
    for (auto const& e : v)
        c.insert (c.cend(), e);
    checkContents (c, v);
}

template <class Container, class Values>
void
aged_associative_container_test_base::
checkInsertHintMove (Container& c, Values const& v)
{
    Values v2 (v);
    for (auto& e : v2)
        c.insert (c.cend(), std::move (e));
    checkContents (c, v);
}

template <class Container, class Values>
void
aged_associative_container_test_base::
checkEmplace (Container& c, Values const& v)
{
    for (auto const& e : v)
        c.emplace (e);
    checkContents (c, v);
}

template <class Container, class Values>
void
aged_associative_container_test_base::
checkEmplaceHint (Container& c, Values const& v)
{
    for (auto const& e : v)
        c.emplace_hint (c.cend(), e);
    checkContents (c, v);
}

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
aged_associative_container_test_base::
testModifiers()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());
    auto const l (make_list (v));

    //testcase (Traits::name() + " modify");
    testcase ("modify");

    {
        typename Traits::template Cont <> c (clock);
        checkInsertCopy (c, v);
    }

    {
        typename Traits::template Cont <> c (clock);
        checkInsertCopy (c, l);
    }

    {
        typename Traits::template Cont <> c (clock);
        checkInsertMove (c, v);
    }

    {
        typename Traits::template Cont <> c (clock);
        checkInsertMove (c, l);
    }

    {
        typename Traits::template Cont <> c (clock);
        checkInsertHintCopy (c, v);
    }

    {
        typename Traits::template Cont <> c (clock);
        checkInsertHintCopy (c, l);
    }

    {
        typename Traits::template Cont <> c (clock);
        checkInsertHintMove (c, v);
    }

    {
        typename Traits::template Cont <> c (clock);
        checkInsertHintMove (c, l);
    }
}

//------------------------------------------------------------------------------
//
// Chronological ordering
//
//------------------------------------------------------------------------------

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
aged_associative_container_test_base::
testChronological ()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;
    typedef typename Traits::Value Value;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());

    //testcase (Traits::name() + " chronological");
    testcase ("chronological");

    typename Traits::template Cont <> c (
        v.begin(), v.end(), clock);

    expect (std::equal (
        c.chronological.cbegin(), c.chronological.cend(),
            v.begin(), v.end(), equal_value <Traits> ()));

    // Test touch() with a non-const iterator.
    for (auto iter (v.crbegin()); iter != v.crend(); ++iter)
    {
        using iterator = typename decltype (c)::iterator;
        iterator found (c.find (Traits::extract (*iter)));

        expect (found != c.cend());
        if (found == c.cend())
            return;
        c.touch (found);
    }

    expect (std::equal (
        c.chronological.cbegin(), c.chronological.cend(),
            v.crbegin(), v.crend(), equal_value <Traits> ()));

    // Test touch() with a const_iterator
    for (auto iter (v.cbegin()); iter != v.cend(); ++iter)
    {
        using const_iterator = typename decltype (c)::const_iterator;
        const_iterator found (c.find (Traits::extract (*iter)));

        expect (found != c.cend());
        if (found == c.cend())
            return;
        c.touch (found);
    }

    expect (std::equal (
        c.chronological.cbegin(), c.chronological.cend(),
            v.cbegin(), v.cend(), equal_value <Traits> ()));

    {
        // Because touch (reverse_iterator pos) is not allowed, the following
        // lines should not compile for any aged_container type.
//      c.touch (c.rbegin());
//      c.touch (c.crbegin());
    }
}

//------------------------------------------------------------------------------
//
// Element creation via operator[]
//
//------------------------------------------------------------------------------

// map, unordered_map
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <IsMap && ! IsMulti>::type
aged_associative_container_test_base::
testArrayCreate()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;
    typename Traits::ManualClock clock;
    auto v (Traits::values());

    //testcase (Traits::name() + " array create");
    testcase ("array create");

    {
        // Copy construct key
        typename Traits::template Cont <> c (clock);
        for (auto e : v)
            c [e.first] = e.second;
        checkContents (c, v);
    }

    {
        // Move construct key
        typename Traits::template Cont <> c (clock);
        for (auto e : v)
            c [std::move (e.first)] = e.second;
        checkContents (c, v);
    }
}

//------------------------------------------------------------------------------
//
// Helpers for erase tests
//
//------------------------------------------------------------------------------

template <class Container, class Values>
void
aged_associative_container_test_base::
reverseFillAgedContainer (Container& c, Values const& values)
{
    // Just in case the passed in container was not empty.
    c.clear();

    // c.clock() returns an abstract_clock, so dynamic_cast to manual_clock.
    //  NOTE This is sketchy
    typedef TestTraitsBase::ManualClock ManualClock;
    ManualClock& clk (dynamic_cast <ManualClock&> (c.clock()));
    clk.set (0);

    Values rev (values);
    std::sort (rev.begin (), rev.end ());
    std::reverse (rev.begin (), rev.end ());
    for (auto& v : rev)
    {
        // Add values in reverse order so they are reversed chronologically.
        ++clk;
        c.insert (v);
    }
}

// Get one iterator before endIter.  We have to use operator++ because you
// cannot use operator-- with unordered container iterators.
template <class Iter>
Iter
aged_associative_container_test_base::
nextToEndIter (Iter beginIter, Iter const endIter)
{
    if (beginIter == endIter)
    {
        fail ("Internal test failure. Cannot advance beginIter");
        return beginIter;
    }

    //
    Iter nextToEnd = beginIter;
    do
    {
        nextToEnd = beginIter++;
    } while (beginIter != endIter);
    return nextToEnd;
}

// Implementation for the element erase tests
//
// This test accepts:
//  o the container from which we will erase elements
//  o iterators into that container defining the range of the erase
//
// This implementation does not declare a pass, since it wants to allow
// the caller to examine the size of the container and the returned iterator
//
// Note that this test works on the aged_associative containers because an
// erase only invalidates references and iterators to the erased element
// (see 23.2.4/13).  Therefore the passed-in end iterator stays valid through
// the whole test.
template <class Container, class Iter>
bool aged_associative_container_test_base::
doElementErase (Container& c, Iter const beginItr, Iter const endItr)
{
    auto it (beginItr);
    size_t count = c.size();
    while (it != endItr)
    {
        auto expectIt = it;
        ++expectIt;
        it = c.erase (it);

        if (it != expectIt)
        {
            fail ("Unexpected returned iterator from element erase");
            return false;
        }

        --count;
        if (count != c.size())
        {
            fail ("Failed to erase element");
            return false;
        }

        if (c.empty ())
        {
            if (it != endItr)
            {
                fail ("Erase of last element didn't produce end");
                return false;
            }
        }
    }
   return true;
}

//------------------------------------------------------------------------------
//
// Erase of individual elements
//
//------------------------------------------------------------------------------

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
aged_associative_container_test_base::
testElementErase ()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;

    //testcase (Traits::name() + " element erase"
    testcase ("element erase");

    // Make and fill the container
    typename Traits::ManualClock clock;
    typename Traits::template Cont <> c {clock};
    reverseFillAgedContainer (c, Traits::values());

    {
        // Test standard iterators
        auto tempContainer (c);
        if (! doElementErase (tempContainer,
            tempContainer.cbegin(), tempContainer.cend()))
            return; // Test failed

        expect (tempContainer.empty(), "Failed to erase all elements");
        pass();
    }
    {
        // Test chronological iterators
        auto tempContainer (c);
        auto& chron (tempContainer.chronological);
        if (! doElementErase (tempContainer, chron.begin(), chron.end()))
            return; // Test failed

        expect (tempContainer.empty(),
            "Failed to chronologically erase all elements");
        pass();
    }
    {
        // Test standard iterator partial erase
        auto tempContainer (c);
        expect (tempContainer.size() > 2,
            "Internal failure.  Container too small.");
        if (! doElementErase (tempContainer, ++tempContainer.begin(),
            nextToEndIter (tempContainer.begin(), tempContainer.end())))
            return; // Test failed

        expect (tempContainer.size() == 2,
            "Failed to erase expected number of elements");
        pass();
    }
    {
        // Test chronological iterator partial erase
        auto tempContainer (c);
        expect (tempContainer.size() > 2,
            "Internal failure.  Container too small.");
        auto& chron (tempContainer.chronological);
        if (! doElementErase (tempContainer, ++chron.begin(),
            nextToEndIter (chron.begin(), chron.end())))
            return; // Test failed

        expect (tempContainer.size() == 2,
            "Failed to chronologically erase expected number of elements");
        pass();
    }
    {
        auto tempContainer (c);
        expect (tempContainer.size() > 4,
            "Internal failure.  Container too small.");
        // erase(reverse_iterator) is not allowed.  None of the following
        // should compile for any aged_container type.
//      c.erase (c.rbegin());
//      c.erase (c.crbegin());
//      c.erase(c.rbegin(), ++c.rbegin());
//      c.erase(c.crbegin(), ++c.crbegin());
    }
}

// Implementation for the range erase tests
//
// This test accepts:
//
//  o A container with more than 2 elements and
//  o An object to ask for begin() and end() iterators in the passed container
//
// This peculiar interface allows either the container itself to be passed as
// the second argument or the container's "chronological" element.  Both
// sources of iterators need to be tested on the container.
//
// The test locates iterators such that a range-based delete leaves the first
// and last elements in the container.  It then validates that the container
// ended up with the expected contents.
//
template <class Container, class BeginEndSrc>
void
aged_associative_container_test_base::
doRangeErase (Container& c, BeginEndSrc const& beginEndSrc)
{
    expect (c.size () > 2,
        "Internal test failure. Container must have more than 2 elements");
    auto itBeginPlusOne (beginEndSrc.begin ());
    auto const valueFront = *itBeginPlusOne;
    ++itBeginPlusOne;

    // Get one iterator before end()
    auto itBack (nextToEndIter (itBeginPlusOne, beginEndSrc.end ()));
    auto const valueBack = *itBack;

    // Erase all elements but first and last
    auto const retIter = c.erase (itBeginPlusOne, itBack);

    expect (c.size() == 2,
        "Unexpected size for range-erased container");

    expect (valueFront == *(beginEndSrc.begin()),
        "Unexpected first element in range-erased container");

    expect (valueBack == *(++beginEndSrc.begin()),
        "Unexpected last element in range-erased container");

    expect (retIter == (++beginEndSrc.begin()),
        "Unexpected return iterator from erase");

    pass ();
}

//------------------------------------------------------------------------------
//
// Erase range of elements
//
//------------------------------------------------------------------------------

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
aged_associative_container_test_base::
testRangeErase ()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;

    //testcase (Traits::name() + " element erase"
    testcase ("range erase");

    // Make and fill the container
    typename Traits::ManualClock clock;
    typename Traits::template Cont <> c {clock};
    reverseFillAgedContainer (c, Traits::values());

    // Not bothering to test range erase with reverse iterators.
    {
        auto tempContainer (c);
        doRangeErase (tempContainer, tempContainer);
    }
    {
        auto tempContainer (c);
        doRangeErase (tempContainer, tempContainer.chronological);
    }
}

//------------------------------------------------------------------------------
//
// Container-wide comparison
//
//------------------------------------------------------------------------------

// ordered
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <! IsUnordered>::type
aged_associative_container_test_base::
testCompare ()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;
    typedef typename Traits::Value Value;
    typename Traits::ManualClock clock;
    auto const v (Traits::values());

    //testcase (Traits::name() + " array create");
    testcase ("array create");

    typename Traits::template Cont <> c1 (
        v.begin(), v.end(), clock);

    typename Traits::template Cont <> c2 (
        v.begin(), v.end(), clock);
    c2.erase (c2.cbegin());

    expect      (c1 != c2);
    unexpected  (c1 == c2);
    expect      (c1 <  c2);
    expect      (c1 <= c2);
    unexpected  (c1 >  c2);
    unexpected  (c1 >= c2);
}

//------------------------------------------------------------------------------
//
// Observers
//
//------------------------------------------------------------------------------

// ordered
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <! IsUnordered>::type
aged_associative_container_test_base::
testObservers()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;
    typename Traits::ManualClock clock;

    //testcase (Traits::name() + " observers");
    testcase ("observers");

    typename Traits::template Cont <> c (clock);
    c.key_comp();
    c.value_comp();

    pass();
}

// unordered
template <bool IsUnordered, bool IsMulti, bool IsMap>
typename std::enable_if <IsUnordered>::type
aged_associative_container_test_base::
testObservers()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;
    typename Traits::ManualClock clock;

    //testcase (Traits::name() + " observers");
    testcase ("observers");

    typename Traits::template Cont <> c (clock);
    c.hash_function();
    c.key_eq();

    pass();
}

//------------------------------------------------------------------------------
//
// Matrix
//
//------------------------------------------------------------------------------

template <bool IsUnordered, bool IsMulti, bool IsMap>
void
aged_associative_container_test_base::
testMaybeUnorderedMultiMap ()
{
    typedef TestTraits <IsUnordered, IsMulti, IsMap> Traits;

    testConstructEmpty      <IsUnordered, IsMulti, IsMap> ();
    testConstructRange      <IsUnordered, IsMulti, IsMap> ();
    testConstructInitList   <IsUnordered, IsMulti, IsMap> ();
    testCopyMove            <IsUnordered, IsMulti, IsMap> ();
    testIterator            <IsUnordered, IsMulti, IsMap> ();
    testReverseIterator     <IsUnordered, IsMulti, IsMap> ();
    testModifiers           <IsUnordered, IsMulti, IsMap> ();
    testChronological       <IsUnordered, IsMulti, IsMap> ();
    testArrayCreate         <IsUnordered, IsMulti, IsMap> ();
    testElementErase        <IsUnordered, IsMulti, IsMap> ();
    testRangeErase          <IsUnordered, IsMulti, IsMap> ();
    testCompare             <IsUnordered, IsMulti, IsMap> ();
    testObservers           <IsUnordered, IsMulti, IsMap> ();
}

//------------------------------------------------------------------------------

class aged_set_test : public aged_associative_container_test_base
{
public:
    // Compile time checks

    typedef std::string Key;
    typedef int T;

    static_assert (std::is_same <
        aged_set <Key>,
        detail::aged_ordered_container <false, false, Key, void>>::value,
            "bad alias: aged_set");

    static_assert (std::is_same <
        aged_multiset <Key>,
        detail::aged_ordered_container <true, false, Key, void>>::value,
            "bad alias: aged_multiset");

    static_assert (std::is_same <
        aged_map <Key, T>,
        detail::aged_ordered_container <false, true, Key, T>>::value,
            "bad alias: aged_map");

    static_assert (std::is_same <
        aged_multimap <Key, T>,
        detail::aged_ordered_container <true, true, Key, T>>::value,
            "bad alias: aged_multimap");

    static_assert (std::is_same <
        aged_unordered_set <Key>,
        detail::aged_unordered_container <false, false, Key, void>>::value,
            "bad alias: aged_unordered_set");

    static_assert (std::is_same <
        aged_unordered_multiset <Key>,
        detail::aged_unordered_container <true, false, Key, void>>::value,
            "bad alias: aged_unordered_multiset");

    static_assert (std::is_same <
        aged_unordered_map <Key, T>,
        detail::aged_unordered_container <false, true, Key, T>>::value,
            "bad alias: aged_unordered_map");

    static_assert (std::is_same <
        aged_unordered_multimap <Key, T>,
        detail::aged_unordered_container <true, true, Key, T>>::value,
            "bad alias: aged_unordered_multimap");

    void run ()
    {
        testMaybeUnorderedMultiMap <false, false, false>();
    }
};

class aged_map_test : public aged_associative_container_test_base
{
public:
    void run ()
    {
        testMaybeUnorderedMultiMap <false, false, true>();
    }
};

class aged_multiset_test : public aged_associative_container_test_base
{
public:
    void run ()
    {
        testMaybeUnorderedMultiMap <false, true, false>();
    }
};

class aged_multimap_test : public aged_associative_container_test_base
{
public:
    void run ()
    {
        testMaybeUnorderedMultiMap <false, true, true>();
    }
};


class aged_unordered_set_test : public aged_associative_container_test_base
{
public:
    void run ()
    {
        testMaybeUnorderedMultiMap <true, false, false>();
    }
};

class aged_unordered_map_test : public aged_associative_container_test_base
{
public:
    void run ()
    {
        testMaybeUnorderedMultiMap <true, false, true>();
    }
};

class aged_unordered_multiset_test : public aged_associative_container_test_base
{
public:
    void run ()
    {
        testMaybeUnorderedMultiMap <true, true, false>();
    }
};

class aged_unordered_multimap_test : public aged_associative_container_test_base
{
public:
    void run ()
    {
        testMaybeUnorderedMultiMap <true, true, true>();
    }
};

BEAST_DEFINE_TESTSUITE(aged_set,container,beast);
BEAST_DEFINE_TESTSUITE(aged_map,container,beast);
BEAST_DEFINE_TESTSUITE(aged_multiset,container,beast);
BEAST_DEFINE_TESTSUITE(aged_multimap,container,beast);
BEAST_DEFINE_TESTSUITE(aged_unordered_set,container,beast);
BEAST_DEFINE_TESTSUITE(aged_unordered_map,container,beast);
BEAST_DEFINE_TESTSUITE(aged_unordered_multiset,container,beast);
BEAST_DEFINE_TESTSUITE(aged_unordered_multimap,container,beast);

} // namespace beast
