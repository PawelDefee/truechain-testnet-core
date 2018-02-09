//------------------------------------------------------------------------------
/*
	Copyright (c) 2012, 2013 Skywell Labs Inc.
	Copyright (c) 2017-2018 TrueChain Foundation.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef SKYWELL_PEERFINDER_SOURCE_H_INCLUDED
#define SKYWELL_PEERFINDER_SOURCE_H_INCLUDED

#include <network/peerfinder/Manager.h>
#include <beast/smart_ptr/SharedObject.h>
#include <boost/system/error_code.hpp>

namespace truechain {
namespace PeerFinder {

/** A static or dynamic source of peer addresses.
    These are used as fallbacks when we are bootstrapping and don't have
    a local cache, or when none of our addresses are functioning. Typically
    sources will represent things like static text in the config file, a
    separate local file with addresses, or a remote HTTPS URL that can
    be updated automatically. Another solution is to use a custom DNS server
    that hands out peer IP addresses when name lookups are performed.
*/
class Source : public beast::SharedObject
{
public:
    /** The results of a fetch. */
    struct Results
    {
        // error_code on a failure
        boost::system::error_code error;

        // list of fetched endpoints
        IPAddresses addresses;
    };

    virtual ~Source () { }
    virtual std::string const& name () = 0;
    virtual void cancel () { }
    virtual void fetch (Results& results, beast::Journal journal) = 0;
};

}
}

#endif