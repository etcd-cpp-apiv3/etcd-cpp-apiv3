#ifndef __ETCD_RESPONSE_HPP__
#define __ETCD_RESPONSE_HPP__

#include <cpprest/http_client.h>
#include <string>
#include <vector>

#include "etcd/Value.hpp"

namespace etcd
{
  typedef std::vector<std::string> Keys;

  /**
   * The Reponse object received for the requests of etcd::Client
   */
  class Response
  {
  public:
    static pplx::task<Response> create(pplx::task<web::http::http_response> response_task);

    Response();

    /**
     * Returns true if this is a successful response
     */
    bool is_ok() const;

    /**
     * Returns the error code received from the etcd server. In case of success the error code is 0.
     */
    int error_code() const;

    /**
     * Returns the string representation of the error code
     */
    std::string const & error_message() const;

    /**
     * Returns the action type of the operation that this response belongs to.
     */
    std::string const & action() const;

    /**
     * Returns the current index value of etcd
     */
    int index() const;

    /**
     * Returns the value object of the response to a get/set/modify operation.
     */
    Value const & value() const;

    /**
     * Returns the previous value object of the response to a set/modify/rm operation.
     */
    Value const & prev_value() const;

    /**
     * Returns the index-th value of the response to an 'ls' operation. Equivalent to values()[index]
     */
    Value const & value(int index) const;

    /**
     * Returns the vector of values in a directory in response to an 'ls' operation.
     */
    Values const & values() const;

    /**
     * Returns the vector of keys in a directory in response to an 'ls' operation.
     */
    Keys const & keys() const;

    /**
     * Returns the index-th key in a directory listing. Same as keys()[index]
     */
    std::string const & key(int index) const;

  protected:
    Response(web::http::http_response http_response, web::json::value json_value);

    int         _error_code;
    std::string _error_message;
    int         _index;
    std::string _action;
    Value       _value;
    Value       _prev_value;
    Values      _values;
    Keys        _keys;
  };
}

#endif
