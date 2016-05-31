#include "etcd/Response.hpp"
#include "json_constants.hpp"

pplx::task<etcd::Response> etcd::Response::create(pplx::task<web::http::http_response> response_task)
{
  return pplx::task<etcd::Response> ([response_task](){
      auto json_task = response_task.get().extract_json();
      return etcd::Response(response_task.get(), json_task.get());
    });
}

etcd::Response::Response()
  : _error_code(0),
    _index(0)
{
}

etcd::Response::Response(web::http::http_response http_response, web::json::value json_value)
  : _error_code(0),
    _index(0)
{
  if (http_response.headers().has(JSON_ETCD_INDEX))
    _index = atoi(http_response.headers()[JSON_ETCD_INDEX].c_str());

  if (json_value.has_field(JSON_ERROR_CODE))
  {
    _error_code = json_value[JSON_ERROR_CODE].as_number().to_int64();
    _error_message = json_value[JSON_MESSAGE].as_string();
  }

  if (json_value.has_field(JSON_ACTION))
    _action = json_value[JSON_ACTION].as_string();

  if (json_value.has_field(JSON_NODE))
  {
    if (json_value[JSON_NODE].has_field(JSON_NODES))
    {
      std::string prefix = json_value[JSON_NODE][JSON_KEY].as_string();
      for (auto & node : json_value[JSON_NODE][JSON_NODES].as_array())
      {
        _values.push_back(Value(node));
        _keys.push_back(node[JSON_KEY].as_string().substr(prefix.length() + 1));
      }
    }
    else
      _value = Value(json_value.at(JSON_NODE));
  }

  if (json_value.has_field(JSON_PREV_NODE))
    _prev_value = Value(json_value.at(JSON_PREV_NODE));
}

int etcd::Response::error_code() const
{
  return _error_code;
}

std::string const & etcd::Response::error_message() const
{
  return _error_message;
}

int etcd::Response::index() const
{
  return _index;
}

std::string const & etcd::Response::action() const
{
  return _action;
}

bool etcd::Response::is_ok() const
{
  return error_code() == 0;
}

etcd::Value const & etcd::Response::value() const
{
  return _value;
}

etcd::Value const & etcd::Response::prev_value() const
{
  return _prev_value;
}

etcd::Values const & etcd::Response::values() const
{
  return _values;
}

etcd::Value const & etcd::Response::value(int index) const
{
  return _values[index];
}

etcd::Keys const & etcd::Response::keys() const
{
  return _keys;
}

std::string const & etcd::Response::key(int index) const
{
  return _keys[index];
}
