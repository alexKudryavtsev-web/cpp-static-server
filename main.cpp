#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>

int main() {
  boost::uuids::random_generator generator;

  boost::uuids::uuid uuid = generator();

  std::cout << "UUID: " << uuid << std::endl;

  return 0;
}