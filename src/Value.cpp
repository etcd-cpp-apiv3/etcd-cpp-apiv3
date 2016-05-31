#include "etcd/Value.hpp"
#include "json_constants.hpp"

etcd::Value::Value()
  : dir(false),
    created(0),
    modified(0)
{
}

etcd::Value::Value(web::json::value const & json_value)
  : _key(json_value.has_field(JSON_KEY) ? json_value.at(JSON_KEY).as_string() : ""),
    dir(json_value.has_field(JSON_DIR)),
    value(json_value.has_field(JSON_VALUE) ? json_value.at(JSON_VALUE).as_string() : ""),
    created(json_value.has_field(JSON_CREATED) ? json_value.at(JSON_CREATED).as_number().to_int64() : 0),
    modified(json_value.has_field(JSON_MODIFIED) ? json_value.at(JSON_MODIFIED).as_number().to_int64() : 0)
{
}

std::string const & etcd::Value::key() const
{
  return _key;
}

bool etcd::Value::is_dir() const
{
  return dir;
}

std::string const & etcd::Value::as_string() const
{
  return value;
}

int etcd::Value::created_index() const
{
  return created;
}

int etcd::Value::modified_index() const
{
  return modified;
}
