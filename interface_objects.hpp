#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <iomanip>

#include <stdlib.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/route.h>

#include <boost/program_options/variables_map.hpp>

#include "rang.hpp"

#define	nitems(x)	(sizeof((x)) / sizeof((x)[0]))

namespace interface_addrs {
  class interface_object {
    public:
      interface_object() {}
      virtual ~interface_object() {}
      // mutators for the interfaces
      virtual bool add(boost::program_options::variables_map& vm) = 0;
      virtual bool del(boost::program_options::variables_map& vm) = 0;

      // here the address_setting is output not input
      virtual bool list(boost::program_options::variables_map& vm) = 0;
    protected:
      std::pair<std::string, sa_family_t> sockaddr_to_ip(struct sockaddr *sock){
        // where the address will land
        char ip[40];
        
        // collect the family and leave if it's not INET{6}
        auto fam = sock->sa_family;
        if(!(fam == AF_INET || fam == AF_INET6)) 
          return std::make_pair(std::string(), 0);


        // god aweful structure parsing to 
        // get the in_addr struct for inet_ntop
        struct in_addr* addr = &((struct sockaddr_in*)sock)->sin_addr;

        // call ntop and get the formatted address
        inet_ntop(fam, (void*)addr, ip, 40);
        
        return std::make_pair(std::string(ip), fam);
      }
  };

  class address_object : public interface_object {
    // rather than make a new object, just use a simple typedef
    typedef std::map<std::string, std::vector<std::pair<std::string, sa_family_t>>> interfaces_t;
    public:
      address_object() {}
      // creates the defs needed to implement the interface_object type
      bool add(boost::program_options::variables_map& vm) final{
        return true;
      }
      bool del(boost::program_options::variables_map& vm) final{
        return true;
      }
      bool list(boost::program_options::variables_map& vm) final{
        // create the pointer to the linked list
        struct ifaddrs *ifap;
        // get the list head
        getifaddrs(&ifap);
        if(ifap == NULL){ return false; }

        // create a second pointer so we can keep
        // track of the head for freeing later
        struct ifaddrs *if_next = ifap;
        while(if_next != NULL){
          collect_interface_info(if_next);
          if_next = if_next->ifa_next;
        }

        print_addrs();
        // free the LL from the head
        freeifaddrs(ifap);

        return true;
      }
    private:
      interfaces_t interfaces;

      void collect_interface_info(struct ifaddrs *ifap){
        auto entry = sockaddr_to_ip(ifap->ifa_addr);
        // stringify the name
        auto name = std::string(ifap->ifa_name);
        // if we have seen this interface just push back
        if(interfaces.count(name)){
          interfaces[name].push_back(entry);
        }
        // otherwise new vector then push back
        else{
          interfaces[name] = std::vector<std::pair<std::string, sa_family_t>>();
          interfaces[name].push_back(entry);
        }
      }
      void print_addrs(){
        for(auto& iface : interfaces){
          // print the interface name as green
          std::cout << rang::fg::green << iface.first << rang::style::reset << ":\t";
          // this delimiter algorithm is "add first" with the delim starting as
          // the empty string. The delim is then changed to ", " for insertion of
          // commas
          std::string delim = "";
          for(auto& addr : iface.second){
            bool printed = true;
            std::cout << delim;
            // color based on the address family
            switch(addr.second){
              case AF_INET: 
                std::cout << rang::fg::yellow;
                break;
              case AF_INET6:
                std::cout << rang::fg::magenta;
		break;
              default:
                printed = false;
                break;
            }
            // print addr.first = adress with spacing and reset the style
            std::cout << addr.first << rang::style::reset;
            if(printed) delim = ",\t";
          }
          // we are done printing addresses so write endline
          std::cout << std::endl;
        }
      }
  };
  class route_object : public interface_object {
    public:
      route_object() {}
      // creates the defs needed to implement the interface_object type
      bool add(boost::program_options::variables_map& vm) final{
        return true;
      }
      bool del(boost::program_options::variables_map& vm) final{
        return true;
      }
      bool list(boost::program_options::variables_map& vm) final{
        print_route_af(AF_INET);
        print_route_af(AF_INET6);
        return true;
      }
    private:
      void print_route_af(int af){
        size_t needed;
        int mib[7];
        char *buf, *next, *lim;
        struct rt_msghdr *rtm;
        struct sockaddr *sa;

        mib[0] = CTL_NET;
        mib[1] = PF_ROUTE;
        mib[2] = 0;
        mib[3] = af;
        mib[4] = NET_RT_DUMP;
        mib[5] = 0;
        mib[6] = -1;

        if(sysctl(mib, nitems(mib), NULL, &needed, NULL, 0) < 0) return;
        if((buf = (char *)malloc(needed)) == NULL){
          throw std::runtime_error("No Mem");
        }
        if(sysctl(mib, nitems(mib), buf, &needed, NULL, 0) < 0){
          free(buf);
          return;
        }

        lim = buf + needed;
        for(next = buf; next <= lim; next += rtm->rtm_msglen){
          rtm = (struct rt_msghdr *)next;
          if(rtm->rtm_version != RTM_VERSION) break;

          auto num_sockaddrs = (rtm->rtm_msglen - sizeof(struct rt_msghdr)) / sizeof(struct sockaddr);
          std::cout << "sockaddr insts: "<< num_sockaddrs << std::endl;
          sa = (struct sockaddr *)(rtm + 1);

          for(int i = 0; i < num_sockaddrs; i++){
            auto sa_n = &sa[i];
            auto ip_n_fam = sockaddr_to_ip(sa_n);
	    if(ip_n_fam.second == 0) continue;
            std::cout << (int)ip_n_fam.second << ": " << ip_n_fam.first << std::endl;
          }
	  std::cout << "===" << std::endl;
	  std::cout << std::endl;
        }
      }
  };
  class tun_object : public interface_object {
    public:
      tun_object() {}
      // creates the defs needed to implement the interface_object type
      bool add(boost::program_options::variables_map& vm) final{
        return true;
      }
      bool del(boost::program_options::variables_map& vm) final{
        return true;
      }
      bool list(boost::program_options::variables_map& vm) final{
        std::cout << "called list" << std::endl;
        return true;
      }
  };
};
