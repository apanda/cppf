#include <iostream>
#include <utility>
#include <algorithm>
#include <boost/config.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/johnson_all_pairs_shortest.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/multi_array.hpp>
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
#include "rules.hpp"


void TieredF (const std::unique_ptr<Xbar>& xbar, 
              const std::unique_ptr<Topology>& topo,
              LabelAllocator& allocator,
              std::map<uint32_t, std::list<Rule>>& tables) {
  using namespace boost;
  typedef adjacency_list<vecS, vecS, undirectedS, no_property, property<edge_weight_t, uint32_t>> Graph;
  std::vector<uint32_t> children(xbar->children.size());
  std::map<uint32_t, uint32_t> normalizedChildren;
  uint32_t currentChild = 0;
  for (const uint32_t child: xbar->children) {
    children.push_back(child);
    normalizedChildren[child] = currentChild;
    currentChild++;
  }
  //std::cout << "I have " << xbar->links.size() << " links at tier " << xbar->tier << std::endl;
  // for (const uint32_t child: xbar->children) {
  //  TieredF(topo->xbars[child], topo);
  //}
  std::vector<std::pair<uint32_t, uint32_t>> edges(xbar->links.size());
  std::map<uint32_t, std::list<uint32_t>> neighbors;
  
  for (const uint32_t child: xbar->children) {
    //neighbors.insert(std::map<uint32_t, std::list<uint32_t>>::value_type(child, std:list<uint32_t>()));
    neighbors.insert(std::map<uint32_t, std::list<uint32_t> >::value_type(child, std::list<uint32_t>()));
  }
  
  for (const std::pair<Port, Port> link : xbar->links) {
    edges.push_back(std::make_pair(normalizedChildren[xbar->port_owners[link.first]],
                                   normalizedChildren[xbar->port_owners[link.second]]));
    neighbors[xbar->port_owners[link.first]].push_back(xbar->port_owners[link.second]);
    neighbors[xbar->port_owners[link.second]].push_back(xbar->port_owners[link.first]);
  }

  Graph g(edges.begin(), edges.end(), children.size());
  property_map <Graph, edge_weight_t>::type w = get(edge_weight, g);
  graph_traits <Graph>::edge_iterator e, e_end;
  for (boost::tie(e, e_end) = boost::edges(g); e != e_end; ++e) {
    w[*e] = 1;
  }
  std::map<Port, uint32_t> labelStack;
  
  for (const Port& port : xbar->border_ports) {
    labelStack.insert(std::map<Port, uint32_t>::value_type(port, allocator.Allocate()));
  }
  
  // uint32_t distances[children.size()][children.size()];
  std::vector<int> d(children.size(), (std::numeric_limits<int>::max)());
  multi_array<uint32_t, 2> distances(extents[children.size()][children.size()]);
  johnson_all_pairs_shortest_paths(g, distances, distance_map(&d[0]));
  
  for (const uint32_t child: xbar->children) {
    for (const Port& port : xbar->border_ports) {
      if (xbar->port_owners[port] == child) {
        Rule rule;
        rule.label = labelStack[port];
        rule.type = Rule::Output;
        rule.data = port.second;
        tables[child].push_back(rule);
        Rule popRule;
        popRule.label = labelStack[port];
        popRule.type = Rule::Pop;
        popRule.data = labelStack[port];
        tables[child].push_back(popRule);
      }
      else {
        Rule pushRule;
        pushRule.label = labelStack[port];
        pushRule.type = Rule::Push;
        pushRule.data = labelStack[port];
        tables[child].push_back(pushRule);
        uint32_t minDistance = std::numeric_limits<uint32_t>::max();
        uint32_t nextHop = 0;
        for (const uint32_t neighbor: neighbors[child]) {
          if (distances[normalizedChildren[xbar->port_owners[port]]]
                       [normalizedChildren[neighbor]] <= minDistance) {
            nextHop = neighbor;
            minDistance = distances[normalizedChildren[xbar->port_owners[port]]]
                       [normalizedChildren[neighbor]];
          }
        }
        Rule rule;
        rule.label = labelStack[port];
        rule.type = Rule::Output;
        rule.data = nextHop;
        tables[child].push_back(rule);
      }
    }

    for (const Rule rule: tables[xbar->id]) {
      tables[child].push_back(rule);
    }
  }

  for (const uint32_t child: xbar->children) {
    TieredF(topo->xbars[child], topo, allocator, tables);
  }
}

void XbMain (const std::unique_ptr<Topology>& topo) {
  LabelAllocator allocator;
  std::map<uint32_t, std::list<Rule>> tables;
  /*std::map<uint32_t, uint32_t> parent;
  for (const std::unique_ptr<Xbar>& xbar: topo->xbars) {
    parent.insert(std::map<uint32_t, uint32_t>::value_type(xbar->id, xbar->parent));
  }*/
  TieredF(topo->xbars[0], topo, allocator, tables);
}

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
  XbMain(topo);
  timeval fComputation;
  gettimeofday(&fComputation, NULL);
  timeval compTime;
  timersub(&fComputation, &processedTopo, &compTime);
  std::cout << "Running F took " << compTime.tv_sec + compTime.tv_usec/1000000.0 << std::endl;
}
