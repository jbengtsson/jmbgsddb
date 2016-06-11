#ifndef UTIL_H
#define UTIL_H

#include <map>
#include <ostream>
#include <stdexcept>

#include <boost/call_traits.hpp>
#include <boost/numeric/ublas/matrix.hpp>

struct key_error : public std::runtime_error
{
    key_error(const std::string& s) : std::runtime_error(s) {}
};

//! Helper for std::map lookup
template<typename M>
bool find(const M& map,
          typename boost::call_traits<typename M::key_type>::param_type key,
          typename M::mapped_type& result)
{
    typename M::const_iterator it = map.find(key);
    if(it==map.end()) return false;
    result = it->second;
    return true;
}

//! An iterator which adapts the second member of a std::pair
//! Allows iteration over a values of a std::map<K,V> to look like
//! iteration over a std::list<V> or std::vector<V>.
template<typename I>
class value_proxy_iterator
{
    I orig;
public:
    typedef typename I::iterator_category iterator_category;
    typedef typename I::value_type::second_type value_type;
    typedef std::ptrdiff_t                   difference_type;
    typedef value_type*                  pointer;
    typedef value_type&                  reference;

    value_proxy_iterator() {}
    explicit value_proxy_iterator(const I& o) :orig(o) {}
    value_proxy_iterator(const value_proxy_iterator& o) :orig(o.orig) {}

    reference operator*() const { return orig->second; }
    pointer operator->() const { return &orig->second; }
    reference operator[](size_t i) const { return orig[i]->second; }

    bool operator==(const value_proxy_iterator& o) const { return orig==o.orig; }
    bool operator!=(const value_proxy_iterator& o) const { return orig!=o.orig; }

    value_proxy_iterator& operator++() { ++orig; return *this; }
    value_proxy_iterator  operator++(int) { return value_proxy_iterator(orig++); }
    value_proxy_iterator& operator--() { --orig; return *this; }
    value_proxy_iterator  operator--(int) { return value_proxy_iterator(orig--); }

    value_proxy_iterator& operator+=(size_t i) { orig+=i; return *this; }
    value_proxy_iterator& operator-=(size_t i) { orig-=i; return *this; }
    value_proxy_iterator  operator+(size_t i) const { return value_proxy_iterator(orig+i); }
    value_proxy_iterator  operator-(size_t i) const { return value_proxy_iterator(orig-i); }
};

template<typename M>
std::ostream& show_mat(std::ostream& strm, const M& m)
{
    strm<<"("<<m.size1()<<","<<m.size2()<<") [";
    for(size_t r=0, rl=m.size1(); r<rl; r++) {
        strm<<"[";
        for(size_t c=0, cl=m.size2(); c<cl; c++) {
            strm<<m(r,c);
        }
    }
    strm<<"]";
    return strm;
}

/** Parse a file containing a single table of numeric values w/ optional column headers
 */
struct numeric_table {
    typedef boost::numeric::ublas::matrix<double,
        boost::numeric::ublas::row_major
    > value_t;

    typedef std::map<std::string, size_t> colnames_t;
    colnames_t colnames;

    value_t table;

    void read(std::istream&);
};

struct SB {
    std::ostringstream strm;
    SB() {}
    operator std::string() const { return strm.str(); }
    template<typename T>
    SB& operator<<(T i) { strm<<i; return *this; }
};

#endif // UTIL_H
