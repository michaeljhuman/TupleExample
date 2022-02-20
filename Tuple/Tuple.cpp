// tuple.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <utility> // std::move, etc.
#include <cassert>
#include <tuple>

using std::cout;
using std::tuple;

auto const nl = "\n";

bool moved = false;

// Many thanks to Fernando García, who wrote this coding example.  
// From the URL: https://medium.com/@mortificador/implementing-std-tuple-in-c-17-3cc5c6da7277
// 
// I made some changes
// for my own clarity, such as changing names ( as they would say in Dragnet, to protect the innnocent)
 
// _TupleVal used to hold the value of a tuple
// I (index) is not used directly, but distinguishes template instances from each other
// In other words, if the I is different in two instances, then the types are not the same.
template <std::size_t I, typename T>
class _TupleVal
{
public:
    _TupleVal()
    {
    }
    _TupleVal(T const &v)
    {
        val = v;
        moved = false;
    }
    _TupleVal(T &&v)
    {
        val = std::move(v);
        moved = true;
    }
    T &get()
    {
        return val;
    }
private:
    T val;
};

// Non specialized _tupleBase
// _tupleBase handles recursion to build up a type that holds all the tuple values.
template <std::size_t I, typename ...restT>
class _TupleBase
{
};

// Template specialization of _tupleBase, with recursion.  This builds up a class hierarchy
// It uses multiple inheritance.
// It inherits from itself ( _TupleBase ) to hold the values for the types in restT.
// It inherits from _TupleVal, to hold the value for the index I and firstT.
template <std::size_t I, typename firstT, typename ...restT>
class _TupleBase<I, firstT, restT...> : 
    public _TupleVal<I, firstT>,
    public _TupleBase<I + 1, restT...>
{
public:
    // Constructor

    template <typename T, typename ...restT>
    _TupleBase(T &&firstArg, restT && ...restArgs) :
        _TupleVal<I, firstT>(std::forward<T>(firstArg)),
        _TupleBase<I + 1, restT...>(std::forward<restT>(restArgs)...) 
    {
    }
};

// Finally, we can define tuple, which works via inheritance of _tupleBase
// We pass in an index of 0.  Assume we have a simple tuple like:
//    tuple<int>
// Then the template creates a class like:
//    _tupleBase<0,int>
// Which in turn creates a class like:
//    _tupleBase<0,int> : public _TupleVal<0,int>,
//       _tupleBase<1>
// Which creates a _TupleVal class that holds a int.
// The _tupleBase<1>, is an empty class, see the generalized template for _tupleBase
template <typename firstT, typename ...restT>
class Tuple : public _TupleBase<0, firstT, restT...>
{
    public:
        template <typename... restTypes>
        Tuple(restTypes && ... restArgs) :
            _TupleBase<0, firstT, restT...>(std::forward<restTypes>(restArgs)...)
        {
        }
};

// Non specialized template to get type of a tuple's member at index _idx
template <std::size_t I, typename firstT, typename ...restT>
struct _getTupleType
{
    using type = typename _getTupleType<I - 1, restT...>::type;
};

// Partial specialization, when index is 0.  This terminates the recursion.
template <typename firstT, typename ...restT>
struct _getTupleType<0, firstT, restT...>
{
    using type = firstT;
};
 
// Template function to get a value from a tuple.  Uses cast, to downcast to the correct
// base class holding the value.  The syntax is pretty ugly as C++ goes.
// For example, say get<0>(t), where t is tuple<int>:
// * Use _getTupleType<> to get the type of the tuple value at index 0 ( 0 is the first value in the tuple)
// * The type is a struct with a type alias called "type"
// * Using "::type" instantiate _TupleVal with 0, and int ( ::type is an int)
// * static_cast the tuple ( tpl) to this type
// * Call the get() function, on the tuple cast to the appropriate base class
// tuples with multiple values, use recursion to get at the correct cast.
template<size_t I, typename ...restT>
auto get(Tuple<restT...>& tpl)
{
    return static_cast<_TupleVal<I, _getTupleType<I, restT...>::type>>(tpl).get();
}

void testTupleVal()
{
    using T1 = _TupleVal<0, int>;
    using T2 = _TupleVal<1, int>;
    T1 x;
    T2 y;
    //x = y; Expect error for this code.  They have different types due to index being different.  
    // This is here just to show how the index ensures a different type, which is critical for when we
    // cast to specific base classes.
    _TupleVal<0, std::string> z("z");
    std::string s{ "s" };
    _TupleVal<0, std::string> a(s);
}

// This tests compilation, we don't call it
void testTupleBase()
{
    _TupleBase<0, int> x(1);
    _TupleBase<0, int, int> y(1, 2);
    static_cast<_TupleVal<0, int>>(x);
    static_cast<_TupleVal<0, int>>(y);
    static_cast<_TupleVal<1, int>>(y);
    _TupleBase<0, std::string> z(std::string("z"));
    std::string sa{ "a" };
    _TupleBase<0, std::string> a( sa);
}

// This tests compilation, we don't call it
void testTupleConstruction()
{
    Tuple<int> x(1);
    Tuple<int, int> y(1, 2);
    Tuple<int, double> z(1, 3.0);
}

void testRvalueRef()
{
    cout << "testRvalueRef" << nl;
    Tuple<std::string> x( std::string("x")); // Should compile, and move string
    cout << get<0>(x) << " moved: " << moved << nl;
    std::string s{ "y" };
    Tuple<std::string> y( s);
    cout << get<0>(y) << " moved: " << moved << nl;
    cout << s << nl;
}

void testTupleGet()
{
    cout << "testTupleGet" << nl;
    Tuple<int, double> z(1, 3.0);
    assert(get<0>(z) == 1);
    assert(get<1>(z) == 3.0);
}

int main()
{
    testRvalueRef();
    testTupleGet();
}
