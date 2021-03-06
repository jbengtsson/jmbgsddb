#ifndef SCSI_CONFIG_H
#define SCSI_CONFIG_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} // extern "C"

#include <ostream>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>

#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/call_traits.hpp>
#include <boost/static_assert.hpp>

#include <scsi/util.h>

namespace detail {
// helper to return POD types by value and complex types by const reference
template<typename X>
struct RT { typedef const X& type; };
template<> struct RT<double> { typedef double type; };

template<typename V>
struct buildval {
    static inline V op() { return V(); }
};
template<>
struct buildval<double> {
    static inline double op() { return 0.0; }
};

// helper to ensure that attempts to call Config::get<T> for unsupported T will fail to compile.
template<typename T>
struct is_config_value {
};
#define IS_CONFIG_VALUE(TYPE) \
namespace detail {template<> struct is_config_value<TYPE> { typedef TYPE type; };}
}
IS_CONFIG_VALUE(double)
IS_CONFIG_VALUE(std::string)
IS_CONFIG_VALUE(std::vector<double>)

/** @brief Configuration container
 *
 * String keyed lookup of values.
 * Value types may be:
 *  double
 *  std::vector<double>
 *  std::string
 *  std::vector<Config>
 */
class Config
{
public:
    typedef boost::variant<
        double,
        std::vector<double>,
        std::string,
        std::vector<Config>
    > value_t;

    typedef std::vector<Config> vector_t;

    typedef std::map<std::string, value_t> values_t;
private:
    typedef boost::shared_ptr<values_t> values_pointer;
    typedef std::vector<values_pointer> values_scope_t;
    values_scope_t value_scopes;
    // value_scopes always has at least one element

    void _cow();
    //! Construct from several std::map (several scopes)
    //! Argument is consumed via. swap()
    //! Used by new_scope()
    explicit Config(values_scope_t& V) { value_scopes.swap(V); }
public:
    //! New empty config
    Config();
    //! Build config from a single std::map (single scope)
    explicit Config(const values_t& V);
    //! Copy ctor
    Config(const Config&);
    //! Assignment
    Config& operator=(const Config&);

    /** lookup untyped.
     * @throws key_error if name doesn't refer to an existing parameter
     */
    const value_t& getAny(const std::string& name) const;
    /** add/replace with a new value, untyped
     */
    void setAny(const std::string& name, const value_t& val);
    void swapAny(const std::string& name, value_t& val);

    /** lookup typed.
     * @throws key_error if name doesn't refer to an existing parameter
     * @throws boost::bad_get if the parameter value has a type other than T
     */
    template<typename T>
    typename detail::RT<T>::type
    get(const std::string& name) const {
        return boost::get<typename detail::is_config_value<T>::type>(getAny(name));
    }
    /** lookup typed with default.
     * If 'name' doesn't refer to a parameter, or it has the wrong type,
     * then 'def' is returned instead.
     */
    template<typename T>
    typename detail::RT<T>::type
    get(const std::string& name, typename boost::call_traits<T>::param_type def) const {
        try{
            return boost::get<typename detail::is_config_value<T>::type>(getAny(name));
        } catch(boost::bad_get&) {
        } catch(key_error&) {
        }
        return def;
    }

    /** add/replace with a new value
     */
    template<typename T>
    void set(const std::string& name,
             typename boost::call_traits<typename detail::is_config_value<T>::type>::param_type val)
    {
        _cow();
        (*value_scopes.back())[name] = val;
    }

    template<typename T>
    void swap(const std::string& name,
              typename boost::call_traits<typename detail::is_config_value<T>::type>::reference val)
    {
        value_t temp = detail::buildval<T>::op();
        val.swap(boost::get<T>(temp));
        swapAny(name, temp);
        //val.swap(boost::get<T>(temp)); // TODO: detect insert, make this a real swap
    }

    void swap(Config& c)
    {
        // now _cow() here since no value_t are modified
        value_scopes.swap(c.value_scopes);
    }

    void show(std::ostream&, unsigned indent=0) const;

    typedef values_t::iterator iterator;
    typedef values_t::const_iterator const_iterator;

    // Only iterates inner most scope
    inline const_iterator begin() const { return value_scopes.back()->begin(); }
    inline const_iterator end() const { return value_scopes.back()->end(); }

    inline void reserve(size_t) {}

    size_t depth() const;
    void push_scope();
    void pop_scope();
    Config new_scope() const;
};

IS_CONFIG_VALUE(std::vector<Config>);

inline
std::ostream& operator<<(std::ostream& strm, const Config& c)
{
    c.show(strm);
    return strm;
}

class GLPSParser
{
    class Pvt;
    std::auto_ptr<Pvt> priv;
public:
    GLPSParser();
    ~GLPSParser();

    void setVar(const std::string& name, const Config::value_t& v);

    Config *parse(FILE *fp);
    Config *parse(const char* s, size_t len);
    Config *parse(const std::string& s);
};

void GLPSPrint(std::ostream& strm, const Config&);


#undef IS_CONFIG_VALUE

#endif /* extern "C" */

#endif // SCSI_CONFIG_H
