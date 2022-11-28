#include <iostream>
#include <boost/program_options.hpp>
#include "interface_objects.hpp"


namespace args = boost::program_options;


bool collect_arguments(
    int ac, 
    char* av[],
    boost::program_options::variables_map& vm){
  

  args::options_description desc("Allowed Options");
  args::positional_options_description pos;
  
  desc.add_options()
    ("help", "this message")
    ("object", "what to operate on (ip, route)")
    ("action", "what to do (list, set, delete)");

  pos.add("object", 1);
  pos.add("action", 1);

  args::store(args::command_line_parser(ac, av)
      .options(desc)
      .positional(pos)
      .run(), vm);
  
  args::notify(vm);

  return true;
}

bool is_str(args::variables_map& vm, const std::string key, const std::string val){
  return vm[key].as<std::string>().compare(val) == 0;
}

int main(int ac, char* av[]){

  // collect and parse args
  args::variables_map vm;
  // alias the interface_object base class
  typedef std::unique_ptr<interface_addrs::interface_object> iface_obj;
  typedef interface_addrs::address_object addr_obj;
  typedef interface_addrs::route_object route_obj;
  typedef interface_addrs::tun_object tun_obj;

  // create the interface object pointer
  iface_obj obj;

  try {
    if(!collect_arguments(ac, av, vm)) return 1;

    if(vm.count("object")){
      // use addr object, make_unique 
      if(is_str(vm, "object", "addr")
          || is_str(vm, "object", "a"))
        obj = std::make_unique<addr_obj>();
      // use route object
      else if(is_str(vm, "object", "route")
          || is_str(vm, "object", "r"))
        obj = std::make_unique<route_obj>();
      // use tun object
      else if(is_str(vm, "object", "tun")
          || is_str(vm, "object", "t"))
        obj = std::make_unique<tun_obj>();
      else {
        std::cerr << "invalid object selection" << std::endl;
        return 1;
      }
    }

    // determine which function to call
    if(vm.count("action")){
      if(is_str(vm, "action", "list") 
        || is_str(vm, "action", "l"))
          obj->list(vm);
      else if(is_str(vm, "action", "add")
        || is_str(vm, "action", "a"))
          obj->add(vm);
      else if(is_str(vm, "action", "del")
        || is_str(vm, "action", "d"))
          obj->del(vm);
      else{
        std::cerr << "invalid action selection" << std::endl;
        return 1;
      }
    }
  } catch (std::runtime_error& ex){
    std::cerr << "ERROR: " << ex.what() << std::endl;
  }
}
