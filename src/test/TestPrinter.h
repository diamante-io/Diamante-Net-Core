#pragma once

// Copyright 2017 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "catchup/CatchupWork.h"
#include "history/test/HistoryTestsUtils.h"
#include "lib/catch.hpp"
#include "util/XDRCereal.h"
#include "xdrpp/types.h"

namespace diamnet
{
struct OfferState;
}

namespace Catch
{
template <typename T>
struct StringMaker<T, typename std::enable_if<xdr::xdr_traits<T>::valid>::type>
{
    static std::string
    convert(T const& val)
    {
        return xdr_to_string(val);
    }
};

template <> struct StringMaker<diamnet::OfferState>
{
    static std::string convert(diamnet::OfferState const& os);
};

template <> struct StringMaker<diamnet::CatchupRange>
{
    static std::string convert(diamnet::CatchupRange const& cr);
};

template <> struct StringMaker<diamnet::historytestutils::CatchupPerformedWork>
{
    static std::string
    convert(diamnet::historytestutils::CatchupPerformedWork const& cr);
};
}
