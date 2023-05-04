// Copyright 2017 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "invariant/ConservationOfLumens.h"
#include "invariant/InvariantManager.h"
#include "ledger/LedgerTxn.h"
#include "main/Application.h"
#include <fmt/format.h>
#include <numeric>

namespace diamnet
{

static int64_t
calculateDeltaBalance(LedgerEntry const* current, LedgerEntry const* previous)
{
    assert(current || previous);
    auto let = current ? current->data.type() : previous->data.type();
    if (let == ACCOUNT)
    {
        return (current ? current->data.account().balance : 0) -
               (previous ? previous->data.account().balance : 0);
    }
    if (let == CLAIMABLE_BALANCE)
    {
        auto const& asset = current ? current->data.claimableBalance().asset
                                    : previous->data.claimableBalance().asset;

        if (asset.type() != ASSET_TYPE_NATIVE)
        {
            return 0;
        }

        return ((current ? current->data.claimableBalance().amount : 0) -
                (previous ? previous->data.claimableBalance().amount : 0));
    }
    return 0;
}

static int64_t
calculateDeltaBalance(
    std::shared_ptr<GeneralizedLedgerEntry const> const& genCurrent,
    std::shared_ptr<GeneralizedLedgerEntry const> const& genPrevious)
{
    auto type = genCurrent ? genCurrent->type() : genPrevious->type();
    if (type == GeneralizedLedgerEntryType::LEDGER_ENTRY)
    {
        auto const* current = genCurrent ? &genCurrent->ledgerEntry() : nullptr;
        auto const* previous =
            genPrevious ? &genPrevious->ledgerEntry() : nullptr;
        return calculateDeltaBalance(current, previous);
    }
    return 0;
}

ConservationOfLumens::ConservationOfLumens() : Invariant(false)
{
}

std::shared_ptr<Invariant>
ConservationOfLumens::registerInvariant(Application& app)
{
    return app.getInvariantManager().registerInvariant<ConservationOfLumens>();
}

std::string
ConservationOfLumens::getName() const
{
    return "ConservationOfLumens";
}

std::string
ConservationOfLumens::checkOnOperationApply(Operation const& operation,
                                            OperationResult const& result,
                                            LedgerTxnDelta const& ltxDelta)
{
    auto const& lhCurr = ltxDelta.header.current;
    auto const& lhPrev = ltxDelta.header.previous;

    int64_t deltaTotalCoins = lhCurr.totalCoins - lhPrev.totalCoins;
    int64_t deltaFeePool = lhCurr.feePool - lhPrev.feePool;
    int64_t deltaBalances = std::accumulate(
        ltxDelta.entry.begin(), ltxDelta.entry.end(), static_cast<int64_t>(0),
        [](int64_t lhs, decltype(ltxDelta.entry)::value_type const& rhs) {
            return lhs + diamnet::calculateDeltaBalance(rhs.second.current,
                                                        rhs.second.previous);
        });

    if (result.tr().type() == INFLATION)
    {
        int64_t inflationPayouts =
            std::accumulate(result.tr().inflationResult().payouts().begin(),
                            result.tr().inflationResult().payouts().end(),
                            static_cast<int64_t>(0),
                            [](int64_t lhs, InflationPayout const& rhs) {
                                return lhs + rhs.amount;
                            });
        if (deltaTotalCoins != inflationPayouts + deltaFeePool)
        {
            return fmt::format(
                "LedgerHeader totalCoins change ({}) did not match"
                " feePool change ({}) plus inflation payouts ({})",
                deltaTotalCoins, deltaFeePool, inflationPayouts);
        }
        if (deltaBalances != inflationPayouts)
        {
            return fmt::format("LedgerEntry account balances change ({}) "
                               "did not match inflation payouts ({})",
                               deltaBalances, inflationPayouts);
        }
    }
    else
    {
        if (deltaTotalCoins != 0)
        {
            return fmt::format("LedgerHeader totalCoins changed from {} to"
                               " {} without inflation",
                               lhPrev.totalCoins, lhCurr.totalCoins);
        }
        if (deltaFeePool != 0)
        {
            return fmt::format("LedgerHeader feePool changed from {} to"
                               " {} without inflation",
                               lhPrev.feePool, lhCurr.feePool);
        }
        if (deltaBalances != 0)
        {
            return fmt::format("LedgerEntry account balances changed by"
                               " {} without inflation",
                               deltaBalances);
        }
    }
    return {};
}
}
