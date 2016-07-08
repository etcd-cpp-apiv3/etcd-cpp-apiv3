#include "v3/include/AsyncDeleteRangeResponse.hpp"
#include "v3/include/action_constants.hpp"


void etcdv3::AsyncDeleteRangeResponse::ParseResponse(std::string const& key, bool prefix, DeleteRangeResponse& resp)
{
  index = resp.header().revision();

  if(resp.prev_kvs_size() == 0)
  {
    error_code=100;
    error_message="Key not found";
  }
  else
  {
    action = etcdv3::DELETE_ACTION;
    //get all previous values
    for(int cnt=0; cnt < resp.prev_kvs_size(); cnt++)
    {
      prev_values.push_back(resp.prev_kvs(cnt));
    }

    //copy previous vales to values. We will format this later
    for(unsigned int cnt=0; cnt < prev_values.size(); cnt++)
    {
      auto temp_value = prev_values[cnt];
      temp_value.set_mod_revision(index);
      temp_value.clear_value();
      values.push_back(temp_value); 
    }

    prev_value = prev_values[0];
    value = values[0];

    //get all mod revisions of previous values
    std::vector<int> mod_index;
    for(unsigned int cnt = 0; cnt < prev_values.size(); ++cnt)
    {
      mod_index.push_back(prev_values[cnt].mod_revision());
    }
    //sort it ascending
    std::sort(mod_index.begin(),mod_index.end());
    // use the latest mod index
    prev_value.set_mod_revision(mod_index.back());
  
    //get all created revision of previous values
    std::vector<int> create_index;
    for(unsigned int cnt = 0; cnt < prev_values.size(); ++cnt)
    {
      create_index.push_back(prev_values[cnt].create_revision());
    }
    //sort it ascending
    std::sort(create_index.begin(),create_index.end());

    //use earliest create index
    prev_value.set_create_revision(create_index.front());



    //value modified index should be the same as index
    value.set_mod_revision(index); 
    //value created index should be the same as prev value created index 
    value.set_create_revision(prev_value.create_revision());

    //set key.When prefix delete is done, we should use the prefix provided by 
    //client as the key
    value.set_key(key);
    prev_value.set_key(key);
  
    //clear the value
    value.clear_value();
   
    //if withPrefix, clear previous value also
    if(prefix)
    {
      prev_value.clear_value();
    }
    prev_values.clear();
    values.clear();
  }
}
