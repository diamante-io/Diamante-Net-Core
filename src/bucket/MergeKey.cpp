// Copyright 2019 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "bucket/MergeKey.h"
#include "crypto/Hex.h"
#include <sstream>

namespace diamnet
{

MergeKey::MergeKey(bool keepDeadEntries,
                   std::shared_ptr<Bucket> const& inputCurr,
                   std::shared_ptr<Bucket> const& inputSnap,
                   std::vector<std::shared_ptr<Bucket>> const& inputShadows)
    : mKeepDeadEntries(keepDeadEntries)
    , mInputCurrBucket(inputCurr->getHash())
    , mInputSnapBucket(inputSnap->getHash())
{
    mInputShadowBuckets.reserve(inputShadows.size());
    for (auto const& s : inputShadows)
    {
        mInputShadowBuckets.emplace_back(s->getHash());
    }
}

bool
MergeKey::operator==(MergeKey const& other) const
{
    return mKeepDeadEntries == other.mKeepDeadEntries &&
           mInputCurrBucket == other.mInputCurrBucket &&
           mInputSnapBucket == other.mInputSnapBucket &&
           mInputShadowBuckets == other.mInputShadowBuckets;
}

std::ostream&
operator<<(std::ostream& out, MergeKey const& b)
{
    out << "[curr=" << hexAbbrev(b.mInputCurrBucket)
        << ", snap=" << hexAbbrev(b.mInputSnapBucket) << ", shadows=[";
    bool first = true;
    for (auto const& s : b.mInputShadowBuckets)
    {
        if (!first)
        {
            out << ", ";
        }
        first = false;
        out << hexAbbrev(s);
    }
    out << "]]";
    return out;
}
}

namespace std
{
size_t
hash<diamnet::MergeKey>::operator()(diamnet::MergeKey const& key) const noexcept
{
    std::ostringstream oss;
    oss << key.mKeepDeadEntries << ','
        << diamnet::binToHex(key.mInputCurrBucket) << ','
        << diamnet::binToHex(key.mInputSnapBucket);
    for (auto const& e : key.mInputShadowBuckets)
    {
        oss << diamnet::binToHex(e) << ',';
    }
    std::hash<std::string> h;
    return h(oss.str());
}
}
