/**
 * @author: Jeff Thompson
 * See COPYING for copyright and distribution information.
 */

#ifndef NDN_TCPTRANSPORT_HPP
#define	NDN_TCPTRANSPORT_HPP

#include "../c/transport/TcpTransport.h"
#include "Transport.hpp"

namespace ndn {
  
class TcpTransport : public Transport {
public:
  TcpTransport() 
  {
    ndn_TcpTransport_init(&transport_);
    ndn_ = 0;
  }
  
  virtual void connect(NDN &ndn);
  
  virtual void send(unsigned char *data, unsigned int dataLength);

  virtual void tempReceive();
  
private:
  struct ndn_TcpTransport transport_;
  NDN *ndn_;
};

}

#endif
