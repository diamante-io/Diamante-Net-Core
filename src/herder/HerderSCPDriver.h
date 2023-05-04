#pragma once

// Copyright 2017 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "herder/Herder.h"
#include "herder/TxSetFrame.h"
#include "medida/timer.h"
#include "scp/SCPDriver.h"
#include "xdr/Diamnet-ledger.h"

namespace medida
{
class Counter;
class Meter;
class Histogram;
}

namespace diamnet
{
class Application;
class HerderImpl;
class LedgerManager;
class PendingEnvelopes;
class SCP;
class Upgrades;
class VirtualTimer;
struct DiamnetValue;
struct SCPEnvelope;

class HerderSCPDriver : public SCPDriver
{
  public:
    struct ConsensusData
    {
        uint64_t mConsensusIndex;
        DiamnetValue mConsensusValue;
        ConsensusData(uint64_t index, DiamnetValue const& b)
            : mConsensusIndex(index), mConsensusValue(b)
        {
        }
    };

    HerderSCPDriver(Application& app, HerderImpl& herder,
                    Upgrades const& upgrades,
                    PendingEnvelopes& pendingEnvelopes);
    ~HerderSCPDriver();

    void bootstrap();
    void lostSync();

    Herder::State getState() const;

    ConsensusData*
    trackingSCP() const
    {
        return mTrackingSCP.get();
    }
    ConsensusData*
    lastTrackingSCP() const
    {
        return mLastTrackingSCP.get();
    }

    void restoreSCPState(uint64_t index, DiamnetValue const& value);

    // the ledger index that was last externalized
    uint32
    lastConsensusLedgerIndex() const
    {
        assert(mTrackingSCP->mConsensusIndex <= UINT32_MAX);
        return static_cast<uint32>(mTrackingSCP->mConsensusIndex);
    }

    // the ledger index that we expect to externalize next
    uint32
    nextConsensusLedgerIndex() const
    {
        return lastConsensusLedgerIndex() + 1;
    }

    SCP&
    getSCP()
    {
        return mSCP;
    }

    void recordSCPExecutionMetrics(uint64_t slotIndex);
    void recordSCPEvent(uint64_t slotIndex, bool isNomination);
    void recordSCPExternalizeEvent(uint64_t slotIndex, NodeID const& id,
                                   bool forceUpdateSelf);

    // envelope handling
    SCPEnvelopeWrapperPtr wrapEnvelope(SCPEnvelope const& envelope) override;
    void signEnvelope(SCPEnvelope& envelope) override;
    void emitEnvelope(SCPEnvelope const& envelope) override;

    // value validation
    SCPDriver::ValidationLevel validateValue(uint64_t slotIndex,
                                             Value const& value,
                                             bool nomination) override;
    ValueWrapperPtr extractValidValue(uint64_t slotIndex,
                                      Value const& value) override;

    // value marshaling
    std::string toShortString(PublicKey const& pk) const override;
    std::string getValueString(Value const& v) const override;

    // timer handling
    void setupTimer(uint64_t slotIndex, int timerID,
                    std::chrono::milliseconds timeout,
                    std::function<void()> cb) override;

    // hashing support
    Hash getHashOf(std::vector<xdr::opaque_vec<>> const& vals) const override;

    // core SCP
    ValueWrapperPtr
    combineCandidates(uint64_t slotIndex,
                      ValueWrapperPtrSet const& candidates) override;
    void valueExternalized(uint64_t slotIndex, Value const& value) override;

    // Submit a value to consider for slotIndex
    // previousValue is the value from slotIndex-1
    void nominate(uint64_t slotIndex, DiamnetValue const& value,
                  TxSetFramePtr proposedSet, DiamnetValue const& previousValue);

    SCPQuorumSetPtr getQSet(Hash const& qSetHash) override;

    // listeners
    void ballotDidHearFromQuorum(uint64_t slotIndex,
                                 SCPBallot const& ballot) override;
    void nominatingValue(uint64_t slotIndex, Value const& value) override;
    void updatedCandidateValue(uint64_t slotIndex, Value const& value) override;
    void startedBallotProtocol(uint64_t slotIndex,
                               SCPBallot const& ballot) override;
    void acceptedBallotPrepared(uint64_t slotIndex,
                                SCPBallot const& ballot) override;
    void confirmedBallotPrepared(uint64_t slotIndex,
                                 SCPBallot const& ballot) override;
    void acceptedCommit(uint64_t slotIndex, SCPBallot const& ballot) override;

    optional<VirtualClock::time_point> getPrepareStart(uint64_t slotIndex);

    // converts a Value into a DiamnetValue
    // returns false on error
    bool toDiamnetValue(Value const& v, DiamnetValue& sv);

    // validate close time as much as possible
    bool checkCloseTime(uint64_t slotIndex, uint64_t lastCloseTime,
                        DiamnetValue const& b) const;

    // wraps a *valid* DiamnetValue (throws if it can't find txSet/qSet)
    ValueWrapperPtr wrapDiamnetValue(DiamnetValue const& sv);

    ValueWrapperPtr wrapValue(Value const& sv) override;

    // clean up older slots
    void purgeSlots(uint64_t maxSlotIndex);

    // Does the nomination protocol output a BASIC or a SIGNED
    // DiamnetValue?
    virtual DiamnetValueType compositeValueType() const;

    // Does the current protocol version contain the CAP-0034 closeTime
    // semantics change?
    bool curProtocolPreservesTxSetCloseTimeAffinity() const;

    double getExternalizeLag(NodeID const& id) const;

    Json::Value getQsetLagInfo(bool summary, bool fullKeys);

    void reportCostOutliersForSlot(int64_t slotIndex, bool updateMetrics);

    Json::Value getJsonValidatorCost(bool summary, bool fullKeys,
                                     uint64 index) const;

  private:
    Application& mApp;
    HerderImpl& mHerder;
    LedgerManager& mLedgerManager;
    Upgrades const& mUpgrades;
    PendingEnvelopes& mPendingEnvelopes;
    SCP mSCP;

    static uint32_t const FIRST_PROTOCOL_WITH_TXSET_CLOSETIME_AFFINITY;

    struct SCPMetrics
    {
        medida::Meter& mEnvelopeSign;

        medida::Meter& mValueValid;
        medida::Meter& mValueInvalid;

        // listeners
        medida::Meter& mCombinedCandidates;

        // Timers for nomination and ballot protocols
        medida::Timer& mNominateToPrepare;
        medida::Timer& mPrepareToExternalize;

        // Timers tracking externalize messages
        medida::Timer& mExternalizeLag;
        medida::Timer& mExternalizeDelay;

        // Tracked cost per slot
        medida::Histogram& mCostPerSlot;

        SCPMetrics(Application& app);
    };

    SCPMetrics mSCPMetrics;

    // Nomination timeouts per ledger
    medida::Histogram& mNominateTimeout;
    // Prepare timeouts per ledger
    medida::Histogram& mPrepareTimeout;

    // Externalize lag tracking for nodes in qset
    std::unordered_map<NodeID, medida::Timer> mQSetLag;

    struct SCPTiming
    {
        optional<VirtualClock::time_point> mNominationStart;
        optional<VirtualClock::time_point> mPrepareStart;

        // Nomination timeouts before first prepare
        int64_t mNominationTimeoutCount{0};
        // Prepare timeouts before externalize
        int64_t mPrepareTimeoutCount{0};

        // externalize timing information
        optional<VirtualClock::time_point> mFirstExternalize;
        optional<VirtualClock::time_point> mSelfExternalize;
    };

    // Map of time points for each slot to measure key protocol metrics:
    // * nomination to first prepare
    // * first prepare to externalize
    std::map<uint64_t, SCPTiming> mSCPExecutionTimes;

    uint32_t mLedgerSeqNominating;
    ValueWrapperPtr mCurrentValue;

    // timers used by SCP
    // indexed by slotIndex, timerID
    std::map<uint64_t, std::map<int, std::unique_ptr<VirtualTimer>>> mSCPTimers;

    // if the local instance is tracking the current state of SCP
    // herder keeps track of the consensus index and ballot
    // when not set, it just means that herder will try to snap to any slot that
    // reached consensus
    // on startup, this can be set to a value persisted from the database
    std::unique_ptr<ConsensusData> mTrackingSCP;

    // when losing track of consensus, we remember the consensus value so that
    // we can ignore older ledgers (as we potentially receive old messages)
    // it only tracks actual consensus values (learned when externalizing)
    std::unique_ptr<ConsensusData> mLastTrackingSCP;

    void stateChanged();

    SCPDriver::ValidationLevel validateValueHelper(uint64_t slotIndex,
                                                   DiamnetValue const& sv,
                                                   bool nomination) const;

    void logQuorumInformation(uint64_t index);

    void clearSCPExecutionEvents();

    void timerCallbackWrapper(uint64_t slotIndex, int timerID,
                              std::function<void()> cb);

    void recordLogTiming(VirtualClock::time_point start,
                         VirtualClock::time_point end, medida::Timer& timer,
                         std::string const& logStr,
                         std::chrono::nanoseconds threshold,
                         uint64_t slotIndex);
};
}
