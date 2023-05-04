#pragma once

// Copyright 2016 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "crypto/SecretKey.h"
#include "transactions/TransactionFrame.h"
#include "xdr/Diamnet-ledger-entries.h"
#include "xdr/Diamnet-transaction.h"

namespace diamnet
{

class Application;

namespace txtest
{
struct SetOptionsArguments;
}

class TestAccount
{
  public:
    static TestAccount createRoot(Application& app);

    explicit TestAccount(Application& app, SecretKey sk, SequenceNumber sn = 0)
        : mApp(app), mSk{std::move(sk)}, mSn{sn}
    {
        mAccountID = KeyUtils::toStrKey(mSk.getPublicKey());
    }

    TransactionFramePtr tx(std::vector<Operation> const& ops,
                           SequenceNumber sn = 0);
    Operation op(Operation operation);

    TestAccount create(SecretKey const& secretKey, uint64_t initialBalance);
    TestAccount create(std::string const& name, uint64_t initialBalance);
    void merge(PublicKey const& into);
    void inflation();

    Asset asset(std::string const& name);
    void changeTrust(Asset const& asset, int64_t limit);
    void allowTrust(Asset const& asset, PublicKey const& trustor,
                    uint32_t flag);
    void allowTrust(Asset const& asset, PublicKey const& trustor);
    void denyTrust(Asset const& asset, PublicKey const& trustor);
    void allowMaintainLiabilities(Asset const& asset, PublicKey const& trustor);

    TrustLineEntry loadTrustLine(Asset const& asset) const;
    bool hasTrustLine(Asset const& asset) const;

    void setOptions(txtest::SetOptionsArguments const& arguments);

    void manageData(std::string const& name, DataValue* value);

    void bumpSequence(SequenceNumber to);

    ClaimableBalanceID
    createClaimableBalance(Asset const& asset, int64_t amount,
                           xdr::xvector<Claimant, 10> const& claimants);

    void claimClaimableBalance(ClaimableBalanceID const& balanceID);

    ClaimableBalanceID getBalanceID(uint32_t opIndex, SequenceNumber sn = 0);

    int64_t
    manageOffer(int64_t offerID, Asset const& selling, Asset const& buying,
                Price const& price, int64_t amount,
                ManageOfferEffect expectedEffect = MANAGE_OFFER_CREATED);

    int64_t
    manageBuyOffer(int64_t offerID, Asset const& selling, Asset const& buying,
                   Price const& price, int64_t amount,
                   ManageOfferEffect expectedEffect = MANAGE_OFFER_CREATED);

    int64_t
    createPassiveOffer(Asset const& selling, Asset const& buying,
                       Price const& price, int64_t amount,
                       ManageOfferEffect expectedEffect = MANAGE_OFFER_CREATED);

    void pay(PublicKey const& destination, int64_t amount);
    void pay(PublicKey const& destination, Asset const& asset, int64_t amount);
    PathPaymentStrictReceiveResult pay(PublicKey const& destination,
                                       Asset const& sendCur, int64_t sendMax,
                                       Asset const& destCur, int64_t destAmount,
                                       std::vector<Asset> const& path,
                                       Asset* noIssuer = nullptr);

    PathPaymentStrictSendResult
    pathPaymentStrictSend(PublicKey const& destination, Asset const& sendCur,
                          int64_t sendAmount, Asset const& destCur,
                          int64_t destMin, std::vector<Asset> const& path,
                          Asset* noIssuer = nullptr);

    operator SecretKey() const
    {
        return getSecretKey();
    }
    operator PublicKey() const
    {
        return getPublicKey();
    }

    SecretKey const&
    getSecretKey() const
    {
        return mSk;
    }
    PublicKey const&
    getPublicKey() const
    {
        return mSk.getPublicKey();
    }

    void
    setSequenceNumber(SequenceNumber sn)
    {
        mSn = sn;
    }

    SequenceNumber
    getLastSequenceNumber()
    {
        updateSequenceNumber();
        return mSn;
    }

    SequenceNumber
    nextSequenceNumber()
    {
        updateSequenceNumber();
        return ++mSn;
    }
    SequenceNumber loadSequenceNumber();

    std::string
    getAccountId()
    {
        return mAccountID;
    }

    int64_t getBalance() const;
    int64_t getAvailableBalance() const;

    bool exists() const;

  private:
    Application& mApp;
    SecretKey mSk;
    std::string mAccountID;
    SequenceNumber mSn;

    void updateSequenceNumber();
};
}
