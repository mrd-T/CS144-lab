#include "reassembler.hh"
#include <cstdint>
#include <type_traits>
#include <iostream>
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if ( is_last_substring ) {
    last_substring_inserted = true;
  }

  if ( first_index < front ) {
    const uint64_t to_pop = front - first_index;
    if ( to_pop >= data.size() ) {
      return;
    
    }
    data = data.substr(to_pop);
    // std::cout<<"@!"<<" "<<data<<"\n";
    first_index=0;

  } else {
    first_index = first_index -front;
  }

  buffer.resize( std::max( buffer.size(), first_index + data.size() ) );
  // std::cout<<first_index<<" "<<output_.writer().available_capacity()<<"\n";
  for ( uint64_t i = 0; i < data.size(); ++i ) {
    auto& [validity, byte] = buffer[i + first_index];
    byte = data[i];
    bytes_valid -= validity;
    // cout<< i + first_index<<" "<< output_.writer().available_capacity()<<"\n";
    if( i + first_index < output_.writer().available_capacity())
    {
      validity = true;
    }
    else {
      validity = false;
    }
    bytes_valid += validity;
  }
  data.clear();
  while ( buffer.size() &&buffer.front().first) {
    data.push_back(buffer.front().second);
    front += 1;
    bytes_valid -= 1;
    buffer.pop_front();
  }
  std::cout<<data<<"\n";
  if ( !data.empty() ) {
    output_.writer().push( data );
  }

  if ( last_substring_inserted && buffer.size()==0 ) {
    output_.writer().close();
  }
}

uint64_t Reassembler::count_bytes_pending() const
{
  // Your code here.
  return bytes_valid;
}