#include "etcd/Value.hpp"
#include "proto/kv.pb.h"

etcd::Value::Value()
  : dir(false),
    created(0),
    modified(0)
{
}


etcd::Value::Value(mvccpb::KeyValue const & kvs)
{
  dir=false;
  _key=kvs.key();
  value=kvs.value();
  created=kvs.create_revision();
  modified=kvs.mod_revision();
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
