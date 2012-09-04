#pragma once
#include <sstream>
struct Tier
{
  std::list<double> connectedness;
  uint32_t min_neighbors;
  bool connected;
  uint32_t size;

  static std::unique_ptr<Tier> Parse(ptree const& pt) {
    std::unique_ptr<Tier> tier(new Tier());
    tier->min_neighbors = pt.get<uint32_t>("min_neighbors");
    tier->connected = pt.get<bool>("connected");
    tier->size = pt.get<uint32_t>("size");
    BOOST_FOREACH(const ptree::value_type& p,
                pt.get_child("connectedness")) {
      tier->connectedness.push_back(p.second.get<double>(""));
    }
    return tier;
  }

  void Print() {
    std::cout << "Tier "<< std::endl;
    std::cout << "Neighbors " << min_neighbors << std::endl;
    std::cout << "Connected " << connected << std::endl;
    std::cout << "Size " << size << std::endl;
    std::list<double>::const_iterator it = connectedness.begin();
    std:: cout << "Connectedness: " << *it;
    it++;
    for (; it != connectedness.end(); it++) {
      std::cout << ", "<< *it;
    }
    std::cout << std::endl;
    std::cout << "===================" << std::endl;
  }
};

typedef std::pair<uint32_t, uint32_t> Port;

Port ParsePort(ptree const& pt) {
  uint32_t elt[2];
  uint32_t count = 0;
  BOOST_FOREACH(const ptree::value_type& p,
                pt.get_child("")) {
    assert(count < 2);
    elt[count] = p.second.get<uint32_t>("");
    count++;
  }
  return Port(elt[0], elt[1]);
}

struct Xbar
{
  uint32_t id;
  std::list<uint32_t> children;
  std::list<Port> border_ports;
  std::list<Port> gateway_ports;
  std::list<Port> top_border_ports;
  std::map<uint32_t, std::list<Port>> child_border_ports;
  uint32_t parent;
  uint32_t tier;
  std::list<std::pair<Port, Port>> links;
  std::map<Port, uint32_t> port_owners;
  
  static std::unique_ptr<Xbar> Parse(ptree const& pt) {
    std::unique_ptr<Xbar> xbar(new Xbar());
    xbar->id = pt.get<uint32_t>("ID");
    xbar->parent = pt.get<uint32_t>("parent");
    xbar->tier = pt.get<uint32_t>("tier");
    BOOST_FOREACH(const ptree::value_type& p,
                pt.get_child("children")) {
      xbar->children.emplace_back(p.second.get<uint32_t>(""));
    }
    BOOST_FOREACH(const ptree::value_type& p,
                   pt.get_child("border_ports")) {
      xbar->border_ports.emplace_back(ParsePort(p.second));
    }

    BOOST_FOREACH(const ptree::value_type& p,
                   pt.get_child("gateway_ports")) {
      xbar->gateway_ports.emplace_back(ParsePort(p.second));
    }

    BOOST_FOREACH(const ptree::value_type& p,
                   pt.get_child("top_border_ports")) {
      xbar->top_border_ports.emplace_back(ParsePort(p.second));
    }

    
    BOOST_FOREACH(const ptree::value_type& p,
                    pt.get_child("child_border_ports")) {
      uint32_t child = boost::lexical_cast<uint32_t>(p.first);
      xbar->child_border_ports.insert(std::map<uint32_t, std::list<Port>>::value_type(child, std::list<Port>()));
      BOOST_FOREACH(const ptree::value_type& p2,
                    p.second.get_child("")) {
        xbar->child_border_ports[child].emplace_back(ParsePort(p2.second));
      }
    }

    BOOST_FOREACH(const ptree::value_type& p,
                 pt.get_child("links")) {
      Port ports[2];
      uint32_t count = 0;
      BOOST_FOREACH(const ptree::value_type& p2,
                    p.second.get_child("")) {
        assert(count < 2);
        ports[count] = ParsePort(p2.second);
      }
      xbar->links.emplace_back(std::make_pair(ports[0], ports[1]));
    }

    BOOST_FOREACH(const ptree::value_type& p,
                  pt.get_child("port_owners")) {
      std::string str = p.first;
      std::stringstream ss(std::move(str));
      char sepA, sepB;
      uint32_t first, second;
      ss >> sepA >> first >> sepB >> second;
      xbar->port_owners.insert(std::map<Port, uint32_t>::value_type(Port(first, second), p.second.get<uint32_t>("")));
    }
    return xbar;
  }
};

struct Topology
{
  std::list<std::unique_ptr<Tier>> tiers;
  std::map<uint32_t, std::unique_ptr<Xbar>> xbars;
  static std::unique_ptr<Topology> Parse(ptree const& pt) {
    std::unique_ptr<Topology> topology(new Topology());
    BOOST_FOREACH(const ptree::value_type& p,
                  pt.get_child("tiers")) {
      topology->tiers.emplace_back(Tier::Parse(p.second));
    }
    BOOST_FOREACH(const ptree::value_type& p,
                pt.get_child("data.xbars")) {
      std::unique_ptr<Xbar> xbar = Xbar::Parse(p.second);
      topology->xbars.insert(std::map<uint32_t, std::unique_ptr<Xbar>>::value_type(xbar->id, std::move(xbar)));
    }
    return topology;
  }
};

void print (ptree const& pt) {
  for (boost::property_tree::ptree::const_iterator it = pt.begin(); it != pt.end(); it++) {
    std::cout << it->first << ": " << it->second.get_value<std::string>() << std::endl;
    print(it->second);
  }
}

