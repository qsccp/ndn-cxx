/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2013-2017 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#ifndef NDN_TESTS_SECURITY_V2_VALIDATOR_FIXTURE_HPP
#define NDN_TESTS_SECURITY_V2_VALIDATOR_FIXTURE_HPP

#include "security/v2/validator.hpp"
#include "util/dummy-client-face.hpp"

#include "../../identity-management-time-fixture.hpp"

#include <boost/lexical_cast.hpp>

namespace ndn {
namespace security {
namespace v2 {
namespace tests {

template<class ValidationPolicy>
class ValidatorFixture : public ndn::tests::IdentityManagementTimeFixture
{
public:
  ValidatorFixture()
    : face(io, {true, true})
    , validator(make_unique<ValidationPolicy>(), &face)
    , cache(time::days(100))
  {
    processInterest = [this] (const Interest& interest) {
      auto cert = cache.find(interest);
      if (cert != nullptr) {
        face.receive(*cert);
      }
    };
  }

  virtual
  ~ValidatorFixture() = default;

  template<class Packet>
  void
  validate(const Packet& packet, const std::string& msg, bool expectSuccess, int line)
  {
    std::string detailedInfo = msg + " on line " + to_string(line);
    size_t nCallbacks = 0;
    this->validator.validate(packet,
                       [&] (const Packet&) {
                         ++nCallbacks;
                         BOOST_CHECK_MESSAGE(expectSuccess,
                                             (expectSuccess ? "OK: " : "FAILED: ") + detailedInfo);
                       },
                       [&] (const Packet&, const ValidationError& error) {
                         ++nCallbacks;
                         BOOST_CHECK_MESSAGE(!expectSuccess,
                                             (!expectSuccess ? "OK: " : "FAILED: ") + detailedInfo +
                                             (expectSuccess ? " (" + boost::lexical_cast<std::string>(error) + ")" : ""));
                       });

    mockNetworkOperations();
    BOOST_CHECK_EQUAL(nCallbacks, 1);
  }

  void
  mockNetworkOperations()
  {
    util::signal::ScopedConnection connection = face.onSendInterest.connect([this] (const Interest& interest) {
        if (processInterest != nullptr) {
          io.post(bind(processInterest, interest));
        }
      });
    advanceClocks(time::milliseconds(250), 200);
  }

public:
  util::DummyClientFace face;
  std::function<void(const Interest& interest)> processInterest;
  Validator validator;

  CertificateCache cache;
};

template<class ValidationPolicy>
class HierarchicalValidatorFixture : public ValidatorFixture<ValidationPolicy>
{
public:
  HierarchicalValidatorFixture()
  {
    identity = this->addIdentity("/Security/V2/ValidatorFixture");
    subIdentity = this->addSubCertificate("/Security/V2/ValidatorFixture/Sub1", identity);
    subSelfSignedIdentity = this->addIdentity("/Security/V2/ValidatorFixture/Sub1/Sub2");
    otherIdentity = this->addIdentity("/Security/V2/OtherIdentity");

    this->validator.loadAnchor("", Certificate(identity.getDefaultKey().getDefaultCertificate()));

    this->cache.insert(identity.getDefaultKey().getDefaultCertificate());
    this->cache.insert(subIdentity.getDefaultKey().getDefaultCertificate());
    this->cache.insert(subSelfSignedIdentity.getDefaultKey().getDefaultCertificate());
    this->cache.insert(otherIdentity.getDefaultKey().getDefaultCertificate());
  }

public:
  Identity identity;
  Identity subIdentity;
  Identity subSelfSignedIdentity;
  Identity otherIdentity;
};

#define VALIDATE_SUCCESS(packet, message) this->template validate(packet, message, true, __LINE__)
#define VALIDATE_FAILURE(packet, message) this->template validate(packet, message, false, __LINE__)

} // namespace tests
} // namespace v2
} // namespace security
} // namespace ndn

#endif // NDN_TESTS_SECURITY_V2_VALIDATOR_FIXTURE_HPP
