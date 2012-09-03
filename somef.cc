#include <iostream>
#include <utility>
#include <algorithm>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/johnson_all_pairs_shortest.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <list>
#include <memory>
#include <map>
#include <cassert>
#include <string>
#include <sys/time.h>
using boost::property_tree::ptree;
#include "tier.hpp"
int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " data" << std::endl;
    return 1;
  }
  else {
    std::cerr << "Using file: " << argv[1] << std::endl;
  }
  timeval openFile;
  gettimeofday(&openFile, NULL);
  ptree pt;
  read_json (argv[1], pt);
  std::unique_ptr<Topology> topo = Topology::Parse(pt);
  // print(pt);
  std::cout << "Topology has " << topo->xbars.size() << std::endl;
  timeval processedTopo;
  gettimeofday(&processedTopo, NULL);
  timeval fileProcessingTime;
  timersub(&processedTopo, &openFile, &fileProcessingTime);
  std::cout << "Reading the file took " << fileProcessingTime.tv_sec + fileProcessingTime.tv_usec/1000000.0 << std::endl;
}
