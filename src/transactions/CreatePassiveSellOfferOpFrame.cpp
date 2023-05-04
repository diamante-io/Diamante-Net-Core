// Copyright 2014 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "transactions/CreatePassiveSellOfferOpFrame.h"

namespace diamnet
{

// change from CreatePassiveOfferOp to ManageOfferOp
ManageSellOfferOpHolder::ManageSellOfferOpHolder(Operation const& op)
{
    mCreateOp.body.type(MANAGE_SELL_OFFER);
    auto& manageOffer = mCreateOp.body.manageSellOfferOp();
    auto const& createPassiveOp = op.body.createPassiveSellOfferOp();
    manageOffer.amount = createPassiveOp.amount;
    manageOffer.buying = createPassiveOp.buying;
    manageOffer.selling = createPassiveOp.selling;
    manageOffer.offerID = 0;
    manageOffer.price = createPassiveOp.price;
    mCreateOp.sourceAccount = op.sourceAccount;
}

CreatePassiveSellOfferOpFrame::CreatePassiveSellOfferOpFrame(
    Operation const& op, OperationResult& res, TransactionFrame& parentTx)
    : ManageSellOfferOpHolder(op)
    , ManageSellOfferOpFrame(mCreateOp, res, parentTx, true)
{
}
}
