// Copyright 2017 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "test/TestMarket.h"
#include "ledger/LedgerTxn.h"
#include "ledger/LedgerTxnEntry.h"
#include "lib/catch.hpp"
#include "main/Application.h"
#include "test/TestAccount.h"
#include "test/TxTests.h"
#include "transactions/TransactionUtils.h"
#include "util/XDROperators.h"
#include "xdr/Diamnet-ledger-entries.h"

namespace diamnet
{

using namespace txtest;

bool
operator<(OfferKey const& x, OfferKey const& y)
{
    if (x.sellerID < y.sellerID)
    {
        return true;
    }
    if (y.sellerID < x.sellerID)
    {
        return false;
    }
    return x.offerID < y.offerID;
}

const OfferState OfferState::SAME{};
const OfferState OfferState::DELETED{makeNativeAsset(), makeNativeAsset(),
                                     Price{1, 1}, 0};

OfferState::OfferState(OfferEntry const& entry)
    : selling{entry.selling}
    , buying{entry.buying}
    , price{entry.price}
    , amount{entry.amount}
    , type{(entry.flags & PASSIVE_FLAG) == 0 ? OfferType::ACTIVE
                                             : OfferType::PASSIVE}
{
}

bool
operator==(OfferState const& x, OfferState const& y)
{
    if (!(x.selling == y.selling))
    {
        return false;
    }
    if (!(x.buying == y.buying))
    {
        return false;
    }
    if (!(x.price == y.price))
    {
        return false;
    }
    if (x.amount != y.amount)
    {
        return false;
    }
    if (x.type != y.type)
    {
        return false;
    }
    return true;
}

ClaimOfferAtom
TestMarketOffer::exchanged(int64_t sold, int64_t bought) const
{
    return ClaimOfferAtom{key.sellerID, key.offerID,  state.selling,
                          sold,         state.buying, bought};
}

TestMarket::TestMarket(Application& app) : mApp{app}
{
}

TestMarketOffer
TestMarket::addOffer(TestAccount& account, OfferState const& state,
                     OfferState const& finishedState)
{
    auto newOffersState = mOffers;
    auto deleted = finishedState == OfferState::DELETED;
    auto expectedEffect = deleted ? MANAGE_OFFER_DELETED : MANAGE_OFFER_CREATED;
    auto passive = state.type == OfferType::PASSIVE;
    auto offerId =
        passive
            ? account.createPassiveOffer(state.selling, state.buying,
                                         state.price, state.amount,
                                         expectedEffect)
            : account.manageOffer(0, state.selling, state.buying, state.price,
                                  state.amount, expectedEffect);

    if (deleted)
    {
        REQUIRE(offerId == 0);
    }
    else
    {
        if (mLastAddedID != 0)
        {

            REQUIRE(offerId == mLastAddedID + 1);
        }
        mLastAddedID = offerId;
    }

    return {{account, offerId},
            finishedState == OfferState::SAME ? state : finishedState};
}

TestMarketOffer
TestMarket::updateOffer(TestAccount& account, int64_t id,
                        OfferState const& state,
                        OfferState const& finishedState)
{
    auto newOffersState = mOffers;
    auto deleted = finishedState == OfferState::DELETED;
    auto expectedEffect = deleted ? MANAGE_OFFER_DELETED : MANAGE_OFFER_UPDATED;
    auto offerId =
        account.manageOffer(id, state.selling, state.buying, state.price,
                            state.amount, expectedEffect);

    if (deleted)
    {
        REQUIRE(offerId == 0);
    }
    else
    {
        REQUIRE(offerId == id);
    }

    return {{account, id},
            finishedState == OfferState::SAME ? state : finishedState};
}

void
TestMarket::requireChanges(std::vector<TestMarketOffer> const& changes,
                           std::function<void()> const& f)
{
    auto newOffersState = mOffers;
    try
    {
        f();
    }
    catch (...)
    {
        checkCurrentOffers();
        throw;
    }

    auto deletedOffers = std::vector<OfferKey>{};
    for (auto const& c : changes)
    {
        if (c.state == OfferState::DELETED)
        {
            newOffersState.erase(c.key);
            deletedOffers.push_back(c.key);
        }
        else
        {
            newOffersState[c.key] = c.state;
        }
    }

    checkState(newOffersState, deletedOffers);
    mOffers = newOffersState;
}

TestMarketOffer
TestMarket::requireChangesWithOffer(std::vector<TestMarketOffer> changes,
                                    std::function<TestMarketOffer()> const& f)
{
    auto newOffersState = mOffers;
    auto deletedOffers = std::vector<OfferKey>{};
    TestMarketOffer o;
    try
    {
        o = f();
        changes.push_back(o);
    }
    catch (...)
    {
        checkCurrentOffers();
        throw;
    }

    for (auto const& c : changes)
    {
        if (c.state == OfferState::DELETED)
        {
            newOffersState.erase(c.key);
            deletedOffers.push_back(c.key);
        }
        else
        {
            newOffersState[c.key] = c.state;
        }
    }

    checkState(newOffersState, deletedOffers);
    mOffers = newOffersState;

    return o;
}

void
TestMarket::requireBalances(std::vector<TestMarketBalances> const& balances)
{
    for (auto const& accountBalances : balances)
    {
        auto account = TestAccount{mApp, accountBalances.key};
        for (auto const& assetBalance : accountBalances.balances)
        {
            if (assetBalance.asset.type() == ASSET_TYPE_NATIVE)
            {
                REQUIRE(account.getBalance() == assetBalance.balance);
            }
            else
            {
                auto hasTrustLine = account.hasTrustLine(assetBalance.asset);
                auto trustLineOk = hasTrustLine || assetBalance.balance == 0;
                REQUIRE(trustLineOk);
                if (hasTrustLine)
                {
                    REQUIRE(account.loadTrustLine(assetBalance.asset).balance ==
                            assetBalance.balance);
                }
            }
        }
    }
}

void
TestMarket::checkCurrentOffers()
{
    checkState(mOffers, {});
}

void
TestMarket::checkState(std::map<OfferKey, OfferState> const& offers,
                       std::vector<OfferKey> const& deletedOffers)
{
    LedgerTxn ltx(mApp.getLedgerTxnRoot());
    for (auto const& o : offers)
    {
        auto offer = diamnet::loadOffer(ltx, o.first.sellerID, o.first.offerID);
        REQUIRE(offer);
        REQUIRE(offer.current().data.offer() == o.second);
    }
    for (auto const& o : deletedOffers)
    {
        REQUIRE(!diamnet::loadOffer(ltx, o.sellerID, o.offerID));
    }
}
}
